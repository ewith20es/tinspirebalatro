#ifdef BT_DESKTOP
#define NOMINMAX
#include "app.hpp"
#include <algorithm>
#include <memory>
#include <windows.h>
#include <windowsx.h>

static bt::Input pending;
static bt::App *game = nullptr;
static bool open = true;
static uint32_t rgbPixels[320 * 240];
static RECT viewport(HWND window) {
    RECT r;
    GetClientRect(window, &r);
    int w = r.right, h = r.bottom;
    int z = std::max(1, std::min(w / 320, h / 240));
    return {(w - 320 * z) / 2, (h - 240 * z) / 2, (w + 320 * z) / 2, (h + 240 * z) / 2};
}
static unsigned mapKey(WPARAM key) {
    switch (key) {
    case 'P':
    case VK_RETURN:
        return bt::K_PLAY;
    case 'D':
        return bt::K_DISCARD;
    case 'S':
        return bt::K_SORT;
    case 'R':
        return bt::K_REROLL;
    case 'H':
    case VK_F1:
        return bt::K_HELP;
    case VK_ESCAPE:
        return bt::K_ESCAPE;
    case VK_TAB:
        return bt::K_TAB;
    case VK_SPACE:
        return bt::K_ACTIVATE;
    default:
        return 0;
    }
}
static LRESULT CALLBACK handler(HWND window, UINT message, WPARAM w, LPARAM l) {
    switch (message) {
    case WM_MOUSEMOVE: {
        RECT v = viewport(window);
        int z = std::max(1, (v.right - v.left) / 320);
        pending.x = std::max(0, std::min(319, (GET_X_LPARAM(l) - v.left) / z));
        pending.y = std::max(0, std::min(239, (GET_Y_LPARAM(l) - v.top) / z));
        return 0;
    }
    case WM_LBUTTONDOWN:
        SetCapture(window);
        pending.down = true;
        pending.pressed = true;
        return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        pending.down = false;
        pending.released = true;
        return 0;
    case WM_KILLFOCUS:
        pending.down = false;
        pending.released = true;
        return 0;
    case WM_KEYDOWN:
        if (!(l & (1LL << 30))) {
            pending.keys |= (w == VK_RETURN && game && game->keyboardFocus >= 0)
                                ? unsigned(bt::K_ACTIVATE)
                                : mapKey(w);
            if (w >= '1' && w <= '9')
                pending.card = int(w - '1');
            if (w == '0')
                pending.card = 9;
        }
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(l) == HTCLIENT) {
            SetCursor(nullptr);
            return TRUE;
        }
        return DefWindowProc(window, message, w, l);
    case WM_CLOSE:
        if (game && game->canResume)
            game->save();
        open = false;
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        open = false;
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        RECT client;
        GetClientRect(window, &client);
        FillRect(dc, &client, (HBRUSH)GetStockObject(BLACK_BRUSH));
        if (game) {
            for (int i = 0; i < 320 * 240; ++i) {
                uint16_t p = game->canvas.pixels[i];
                rgbPixels[i] = ((((p >> 11) & 31) * 255 / 31) << 16) |
                               ((((p >> 5) & 63) * 255 / 63) << 8) | ((p & 31) * 255 / 31);
            }
            BITMAPINFO info = {};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = 320;
            info.bmiHeader.biHeight = -240;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            RECT v = viewport(window);
            SetStretchBltMode(dc, COLORONCOLOR);
            StretchDIBits(dc, v.left, v.top, v.right - v.left, v.bottom - v.top, 0, 0, 320, 240,
                          rgbPixels, &info, DIB_RGB_COLORS, SRCCOPY);
        }
        EndPaint(window, &paint);
        return 0;
    }
    default:
        return DefWindowProc(window, message, w, l);
    }
}
int main(int argc, char **argv) {
    std::string savePath = argc > 1 ? argv[1] : "desktop-save.tns";
    std::unique_ptr<bt::App> app(new bt::App(savePath));
    game = app.get();
    HINSTANCE instance = GetModuleHandle(nullptr);
    WNDCLASSA cls = {};
    cls.lpfnWndProc = handler;
    cls.hInstance = instance;
    cls.lpszClassName = "BalatroNspirePreview";
    cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&cls);
    RECT size = {0, 0, 960, 720};
    AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, FALSE);
    HWND window = CreateWindowA(cls.lpszClassName, "Balatro - TI-Nspire desktop preview",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                size.right - size.left, size.bottom - size.top, nullptr, nullptr,
                                instance, nullptr);
    if (!window)
        return 1;
    DWORD last = GetTickCount();
    while (open && !app->quit) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!open)
            break;
        DWORD now = GetTickCount();
        if (now - last >= 30) {
            app->update(pending, int(now - last));
            last = now;
            pending.pressed = pending.released = false;
            pending.keys = 0;
            pending.card = -1;
            app->draw();
            InvalidateRect(window, nullptr, FALSE);
            UpdateWindow(window);
        }
        Sleep(2);
    }
    if (app->canResume)
        app->save();
    if (IsWindow(window))
        DestroyWindow(window);
    game = nullptr;
    return 0;
}
#else
#include <os.h>
#undef WHITE
#include "app.hpp"
#include <algorithm>
#include <memory>

int main(int argc, char **argv) {
    if (!has_colors) {
        show_msgbox("Balatro", "This game needs a TI-Nspire CX or CX II color screen.");
        return 0;
    }
    std::string path = (argc > 0 && argv[0]) ? argv[0] : "balatro.tns";
    size_t slash = path.find_last_of("/\\");
    path = slash == std::string::npos ? "balatro-save.tns"
                                      : path.substr(0, slash + 1) + "balatro-save.tns";
    std::unique_ptr<bt::App> app(new bt::App(path));
    wait_no_key_pressed();
    if (!lcd_init(SCR_320x240_565)) {
        show_msgbox("Balatro", "Could not initialize the color display.");
        return 1;
    }
    touchpad_info_t *info = touchpad_getinfo();
    float px = 160, py = 120;
    int oldX = 0, oldY = 0;
    bool contact = false, wasDown = false;
    unsigned previous = 0, previousNums = 0;
    const t_key *nums[] = {&KEY_NSPIRE_1, &KEY_NSPIRE_2, &KEY_NSPIRE_3, &KEY_NSPIRE_4,
                           &KEY_NSPIRE_5, &KEY_NSPIRE_6, &KEY_NSPIRE_7, &KEY_NSPIRE_8,
                           &KEY_NSPIRE_9, &KEY_NSPIRE_0};
    while (!app->quit) {
        touchpad_report_t report = {};
        bool good = touchpad_scan(&report) == 0;
        bt::Input input;
        if (good && report.contact && info && info->width && info->height) {
            if (contact) {
                int dx = int(report.x) - oldX, dy = int(report.y) - oldY;
                if (std::abs(dx) < info->width / 3 && std::abs(dy) < info->height / 3) {
                    px += dx * 410.f / info->width;
                    py -= dy * 310.f / info->height;
                }
            }
            oldX = report.x;
            oldY = report.y;
            contact = true;
        } else
            contact = false;
        // I/J/K/L remain usable even if the touchpad is unavailable.
        if (isKeyPressed(KEY_NSPIRE_J))
            px -= 4;
        if (isKeyPressed(KEY_NSPIRE_L))
            px += 4;
        if (isKeyPressed(KEY_NSPIRE_I))
            py -= 4;
        if (isKeyPressed(KEY_NSPIRE_K))
            py += 4;
        px = std::max(0.f, std::min(319.f, px));
        py = std::max(0.f, std::min(239.f, py));
        input.x = int(px);
        input.y = int(py);
        bool space = isKeyPressed(KEY_NSPIRE_SPACE);
        input.down = (good && report.pressed) || (space && app->keyboardFocus < 0);
        input.pressed = input.down && !wasDown;
        input.released = !input.down && wasDown;
        wasDown = input.down;
        unsigned now = 0;
        if (isKeyPressed(KEY_NSPIRE_P) || isKeyPressed(KEY_NSPIRE_ENTER))
            now |= bt::K_PLAY;
        if (isKeyPressed(KEY_NSPIRE_D) || isKeyPressed(KEY_NSPIRE_DEL))
            now |= bt::K_DISCARD;
        if (isKeyPressed(KEY_NSPIRE_S))
            now |= bt::K_SORT;
        if (isKeyPressed(KEY_NSPIRE_R))
            now |= bt::K_REROLL;
        if (isKeyPressed(KEY_NSPIRE_H))
            now |= bt::K_HELP;
        if (isKeyPressed(KEY_NSPIRE_ESC) || isKeyPressed(KEY_NSPIRE_MENU))
            now |= bt::K_ESCAPE;
        if (isKeyPressed(KEY_NSPIRE_TAB))
            now |= bt::K_TAB;
        if (isKeyPressed(KEY_NSPIRE_ENTER) && app->keyboardFocus >= 0) {
            now &= ~bt::K_PLAY;
            now |= bt::K_ACTIVATE;
        }
        if (space && app->keyboardFocus >= 0)
            now |= bt::K_ACTIVATE;
        input.keys = now & ~previous;
        previous = now;
        unsigned numeric = 0;
        for (int i = 0; i < 10; ++i)
            if (isKeyPressed((*nums[i]))) {
                numeric |= 1u << i;
                if (!(previousNums & (1u << i)))
                    input.card = i;
            }
        previousNums = numeric;
        if (isKeyPressed(KEY_NSPIRE_CTRL) && isKeyPressed(KEY_NSPIRE_ESC)) {
            if (app->canResume)
                app->save();
            break;
        }
        app->update(input, 33);
        app->draw();
        lcd_blit(app->canvas.pixels, SCR_320x240_565);
        msleep(30);
    }
    if (app->canResume)
        app->save();
    lcd_init(SCR_TYPE_INVALID);
    wait_no_key_pressed();
    return 0;
}
#endif
