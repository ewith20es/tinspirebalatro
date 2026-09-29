#pragma once
#include "engine.hpp"
#include <cstdint>
#include <string>
namespace bt {
using Color = uint16_t;
constexpr Color rgb(int r, int g, int b) {
    return Color(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}
constexpr Color WHITE = rgb(249, 243, 223), INK = rgb(31, 40, 49), GOLD = rgb(244, 191, 87),
                RED = rgb(222, 77, 83), BLUE = rgb(61, 145, 206), MUTED = rgb(159, 178, 177),
                PANEL = rgb(37, 51, 58);
struct Rect {
    int x, y, w, h;
    bool has(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};
class Canvas {
  public:
    Color pixels[320 * 240] = {};
    void pixel(int x, int y, Color c);
    void rect(int x, int y, int w, int h, Color c);
    void border(int x, int y, int w, int h, Color c);
    void round(int x, int y, int w, int h, Color c, int radius = 3);
    void line(int x0, int y0, int x1, int y1, Color c);
    void circle(int x, int y, int radius, Color c);
    void dim(int amount = 150);
    void text(int x, int y, const std::string &text, Color c = WHITE, int scale = 1);
    void center(int x, int y, int width, const std::string &text, Color c = WHITE, int scale = 1);
    void wrap(int x, int y, int width, const std::string &text, Color c = WHITE, int maxLines = 8);
    void suit(int x, int y, int suit, int scale = 1);
    void card(int x, int y, const Card &card, bool selected = false, bool hover = false,
              bool debuff = false, int w = 29, int h = 42);
    void cardBack(int x, int y, int w = 29, int h = 42);
    void joker(int x, int y, int kind, bool hover = false, int w = 35, int h = 39);
    void felt(int tick);
    bool ppm(const std::string &filename) const;
};
} // namespace bt
