#include "graphics.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace bt {
static const uint8_t LETTERS[26][5] = {
    {126, 17, 17, 17, 126}, {127, 73, 73, 73, 54}, {62, 65, 65, 65, 34},  {127, 65, 65, 34, 28},
    {127, 73, 73, 73, 65},  {127, 9, 9, 9, 1},     {62, 65, 73, 73, 122}, {127, 8, 8, 8, 127},
    {0, 65, 127, 65, 0},    {32, 64, 65, 63, 1},   {127, 8, 20, 34, 65},  {127, 64, 64, 64, 64},
    {127, 2, 12, 2, 127},   {127, 4, 8, 16, 127},  {62, 65, 65, 65, 62},  {127, 9, 9, 9, 6},
    {62, 65, 81, 33, 94},   {127, 9, 25, 41, 70},  {38, 73, 73, 73, 50},  {1, 1, 127, 1, 1},
    {63, 64, 64, 64, 63},   {31, 32, 64, 32, 31},  {63, 64, 56, 64, 63},  {99, 20, 8, 20, 99},
    {7, 8, 112, 8, 7},      {97, 81, 73, 69, 67}};
static const uint8_t DIGITS[10][5] = {
    {62, 81, 73, 69, 62},  {0, 66, 127, 64, 0},  {66, 97, 81, 73, 70}, {33, 65, 69, 75, 49},
    {24, 20, 18, 127, 16}, {39, 69, 69, 69, 57}, {60, 74, 73, 73, 48}, {1, 113, 9, 5, 3},
    {54, 73, 73, 73, 54},  {6, 73, 73, 41, 30}};
static uint8_t column(char ch, int n) {
    unsigned char c = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(ch)));
    if (c >= 'A' && c <= 'Z')
        return LETTERS[c - 'A'][n];
    if (c >= '0' && c <= '9')
        return DIGITS[c - '0'][n];
    static const uint8_t dollar[] = {36, 42, 127, 42, 18}, question[] = {2, 1, 81, 9, 6},
                         amp[] = {54, 73, 85, 34, 80};
    switch (c) {
    case '$':
        return dollar[n];
    case '?':
        return question[n];
    case '&':
        return amp[n];
    case '+':
        return n == 2 ? 62 : 8;
    case '-':
        return 8;
    case '=':
        return 20;
    case '.':
        return n == 2 ? 64 : 0;
    case ',':
        return n == 2 ? 96 : 0;
    case ':':
        return n == 2 ? 36 : 0;
    case '/':
        return uint8_t(64 >> n);
    case '!':
        return n == 2 ? 95 : 0;
    case '|':
        return n == 2 ? 127 : 0;
    case '[':
        return n == 1 ? 127 : n == 2 ? 65 : 0;
    case ']':
        return n == 3 ? 127 : n == 2 ? 65 : 0;
    case '(':
        return n == 1 ? 62 : n == 2 ? 65 : 0;
    case ')':
        return n == 3 ? 62 : n == 2 ? 65 : 0;
    case '<':
        return n == 1 ? 8 : n == 2 ? 20 : n == 3 ? 34 : 0;
    case '>':
        return n == 1 ? 34 : n == 2 ? 20 : n == 3 ? 8 : 0;
    case '%':
        return n == 0 ? 99 : n == 1 ? 19 : n == 2 ? 8 : n == 3 ? 100 : 99;
    case '*':
        return n == 2 ? 28 : n == 1 || n == 3 ? 20 : 0;
    case '_':
        return 64;
    case 39:
        return n == 2 ? 3 : 0;
    default:
        return 0;
    }
}
void Canvas::pixel(int x, int y, Color c) {
    if (x >= 0 && x < 320 && y >= 0 && y < 240)
        pixels[y * 320 + x] = c;
}
void Canvas::rect(int x, int y, int w, int h, Color c) {
    int x1 = std::max(0, x), x2 = std::min(320, x + w), y1 = std::max(0, y),
        y2 = std::min(240, y + h);
    if (x1 >= x2 || y1 >= y2)
        return;
    for (int j = y1; j < y2; ++j)
        std::fill(pixels + j * 320 + x1, pixels + j * 320 + x2, c);
}
void Canvas::border(int x, int y, int w, int h, Color c) {
    rect(x, y, w, 1, c);
    rect(x, y + h - 1, w, 1, c);
    rect(x, y, 1, h, c);
    rect(x + w - 1, y, 1, h, c);
}
void Canvas::round(int x, int y, int w, int h, Color c, int r) {
    r = std::min(r, std::min(w / 2, h / 2));
    rect(x + r, y, w - 2 * r, h, c);
    rect(x, y + r, w, h - 2 * r, c);
    for (int j = 0; j < r; ++j)
        for (int i = 0; i < r; ++i)
            if ((r - i) * (r - i) + (r - j) * (r - j) <= r * r + 1) {
                pixel(x + i, y + j, c);
                pixel(x + w - 1 - i, y + j, c);
                pixel(x + i, y + h - 1 - j, c);
                pixel(x + w - 1 - i, y + h - 1 - j, c);
            }
}
void Canvas::line(int x, int y, int x1, int y1, Color c) {
    int dx = std::abs(x1 - x), sx = x < x1 ? 1 : -1, dy = -std::abs(y1 - y), sy = y < y1 ? 1 : -1,
        err = dx + dy;
    for (;;) {
        pixel(x, y, c);
        if (x == x1 && y == y1)
            break;
        int e = 2 * err;
        if (e >= dy) {
            err += dy;
            x += sx;
        }
        if (e <= dx) {
            err += dx;
            y += sy;
        }
    }
}
void Canvas::circle(int x, int y, int r, Color c) {
    for (int j = -r; j <= r; ++j)
        for (int i = -r; i <= r; ++i)
            if (i * i + j * j <= r * r)
                pixel(x + i, y + j, c);
}
void Canvas::dim(int a) {
    int f = 255 - a;
    for (Color &p : pixels) {
        int r = ((p >> 11) & 31) * f / 255, g = ((p >> 5) & 63) * f / 255, b = (p & 31) * f / 255;
        p = Color((r << 11) | (g << 5) | b);
    }
}
void Canvas::text(int x, int y, const std::string &s, Color c, int scale) {
    int origin = x;
    for (char ch : s) {
        if (ch == '\n') {
            x = origin;
            y += 9 * scale;
            continue;
        }
        for (int i = 0; i < 5; ++i) {
            uint8_t bits = column(ch, i);
            for (int j = 0; j < 7; ++j)
                if (bits & (1u << j))
                    rect(x + i * scale, y + j * scale, scale, scale, c);
        }
        x += 6 * scale;
    }
}
void Canvas::center(int x, int y, int width, const std::string &s, Color c, int scale) {
    text(x + (width - int(s.size()) * 6 * scale + scale) / 2, y, s, c, scale);
}
void Canvas::wrap(int x, int y, int width, const std::string &s, Color c, int maxLines) {
    int max = std::max(1, width / 6), lines = 0;
    std::string line, word;
    auto flush = [&]() {
        if (lines < maxLines)
            text(x, y + lines * 10, line, c);
        ++lines;
        line.clear();
    };
    for (size_t i = 0; i <= s.size(); ++i) {
        char ch = i < s.size() ? s[i] : ' ';
        if (ch == ' ' || ch == '\n') {
            if (!word.empty()) {
                if (int(line.size() + word.size() + (!line.empty())) > max && !line.empty())
                    flush();
                if (!line.empty())
                    line += ' ';
                line += word;
                word.clear();
            }
            if (ch == '\n')
                flush();
        } else
            word += ch;
    }
    if (!line.empty())
        flush();
}
void Canvas::suit(int x, int y, int s, int z) {
    // Pixel motifs are original; hearts/diamonds are red, spades/clubs dark blue.
    static const uint8_t masks[4][7] = {{8, 28, 62, 127, 107, 8, 28},
                                        {54, 127, 127, 62, 28, 8, 0},
                                        {28, 62, 28, 107, 127, 8, 28},
                                        {8, 28, 62, 127, 62, 28, 8}};
    Color c = (s == 1 || s == 3) ? RED : INK;
    for (int j = 0; j < 7; ++j)
        for (int i = 0; i < 7; ++i)
            if (masks[s][j] & (1u << i))
                rect(x + i * z, y + j * z, z, z, c);
}
void Canvas::cardBack(int x, int y, int w, int h) {
    round(x + 2, y + 3, w, h, rgb(11, 26, 29));
    round(x, y, w, h, WHITE);
    round(x + 2, y + 2, w - 4, h - 4, rgb(49, 87, 122), 2);
    for (int j = y + 4; j < y + h - 3; j += 4)
        for (int i = x + 4; i < x + w - 3; i += 4)
            pixel(i, j, GOLD);
    border(x + 4, y + 4, w - 8, h - 8, rgb(120, 155, 172));
}
void Canvas::card(int x, int y, const Card &c, bool selected, bool hover, bool debuff, int w,
                  int h) {
    round(x + 2, y + 3, w, h, rgb(10, 28, 29));
    if (selected || hover)
        round(x - 1, y - 1, w + 2, h + 2, selected ? GOLD : rgb(141, 221, 205));
    Color paper = debuff         ? rgb(152, 160, 149)
                  : c.bonus == 1 ? rgb(214, 241, 250)
                  : c.bonus == 2 ? rgb(250, 218, 221)
                  : c.bonus == 3 ? rgb(231, 212, 255)
                                 : WHITE;
    round(x, y, w, h, paper, 2);
    std::string rank = c.rank <= 10   ? number(c.rank)
                       : c.rank == 11 ? "J"
                       : c.rank == 12 ? "Q"
                       : c.rank == 13 ? "K"
                                      : "A";
    text(x + 3, y + 3, rank, c.suit == 1 || c.suit == 3 ? RED : INK);
    suit(x + 3, y + 12, c.suit, 1);
    if (h >= 36)
        suit(x + w / 2 - 5, y + h / 2 - 2, c.suit, 1);
    if (w > 25)
        text(x + w - 7 - int(rank.size() - 1) * 6, y + h - 9, rank,
             c.suit == 1 || c.suit == 3 ? RED : INK);
    if (c.bonus)
        rect(x + 2, y + h - 3, w - 4, 1,
             c.bonus == 1   ? BLUE
             : c.bonus == 2 ? RED
                            : rgb(155, 91, 214));
    if (debuff) {
        line(x + 2, y + 2, x + w - 3, y + h - 3, rgb(95, 108, 109));
        line(x + w - 3, y + 2, x + 2, y + h - 3, rgb(95, 108, 109));
    }
}
void Canvas::joker(int x, int y, int kind, bool hover, int w, int h) {
    const Color palette[] = {RED, GOLD, BLUE, rgb(72, 173, 130), rgb(162, 119, 203)};
    Color c = palette[jokerDef(kind).color];
    round(x + 2, y + 3, w, h, rgb(9, 24, 28));
    if (hover)
        round(x - 1, y - 1, w + 2, h + 2, GOLD);
    round(x, y, w, h, WHITE, 2);
    round(x + 2, y + 2, w - 4, h - 4, c, 2);
    int cx = x + w / 2, cy = y + h / 2;
    circle(cx, cy + 1, 7, WHITE);
    rect(cx - 4, cy - 1, 2, 2, INK);
    rect(cx + 3, cy - 1, 2, 2, INK);
    line(cx - 3, cy + 4, cx, cy + 5, INK);
    line(cx, cy + 5, cx + 4, cy + 3, INK);
    line(cx - 8, cy - 4, cx - 10, cy - 10, WHITE);
    line(cx - 10, cy - 10, cx - 3, cy - 6, WHITE);
    line(cx - 3, cy - 6, cx, cy - 13, WHITE);
    line(cx, cy - 13, cx + 4, cy - 6, WHITE);
    line(cx + 4, cy - 6, cx + 10, cy - 10, WHITE);
    line(cx + 10, cy - 10, cx + 8, cy - 3, WHITE);
    circle(cx - 10, cy - 10, 1, GOLD);
    circle(cx, cy - 13, 1, GOLD);
    circle(cx + 10, cy - 10, 1, GOLD);
    text(x + 3, y + h - 9, number(kind + 1), INK);
    pixel(x + w - 4, y + 4, WHITE);
}
void Canvas::felt(int tick) {
    static Color background[320 * 240];
    static bool ready = false;
    if (!ready) {
        uint32_t noise = 741;
        for (int y = 0; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                noise = noise * 1664525u + 1013904223u;
                int v = int(noise >> 29);
                int glow = std::max(0, 35 - (std::abs(x - 204) + std::abs(y - 106)) / 6);
                background[y * 320 + x] = rgb(13 + v, 51 + glow + v, 51 + glow / 2 + v);
            }
        ready = true;
    }
    std::copy(background, background + 320 * 240, pixels);
    // Subtle moving stitches in the felt, cheap enough for the calculator CPU.
    for (int y = 4; y < 240; y += 18) {
        int off = int(std::sin((tick / 900.0) + (y / 33.0)) * 6);
        for (int x = 84; x < 318; x += 20)
            pixel(x + off, y, rgb(28, 80, 74));
    }
    border(2, 2, 316, 236, rgb(97, 131, 109));
    border(4, 4, 312, 232, rgb(27, 65, 63));
}
bool Canvas::ppm(const std::string &name) const {
    FILE *f = std::fopen(name.c_str(), "wb");
    if (!f)
        return false;
    std::fprintf(f, "P6\n320 240\n255\n");
    for (Color p : pixels) {
        unsigned char b[3] = {static_cast<unsigned char>(((p >> 11) & 31) * 255 / 31),
                              static_cast<unsigned char>(((p >> 5) & 63) * 255 / 63),
                              static_cast<unsigned char>((p & 31) * 255 / 31)};
        std::fwrite(b, 1, 3, f);
    }
    return std::fclose(f) == 0;
}
} // namespace bt
