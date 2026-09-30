#pragma once
#include "graphics.hpp"

namespace bt {
enum Key {
    K_PLAY = 1,
    K_DISCARD = 2,
    K_SORT = 4,
    K_REROLL = 8,
    K_HELP = 16,
    K_ESCAPE = 32,
    K_TAB = 64,
    K_ACTIVATE = 128,
    K_SPEED = 256
};
struct Input {
    int x = 160, y = 120;
    bool down = false, pressed = false, released = false;
    unsigned keys = 0;
    int card = -1;
};
enum Action {
    RESUME = 1,
    NEW_RUN,
    QUIT,
    HELP,
    MENU,
    PLAY_HAND,
    DISCARD_HAND,
    SORT,
    DECK_VIEW,
    START_BLIND,
    SKIP_BLIND,
    NEXT_BLIND,
    REROLL,
    ENDLESS,
    CLOSE,
    HELP_PAGE,
    DECK_PREV,
    DECK_NEXT,
    CONFIRM_NEW,
    SELL,
    CARD_ACTION = 100,
    JOKER_ACTION = 200,
    OFFER_ACTION = 300
};
struct Hit {
    Rect rect;
    int action;
};
struct Particle {
    float x, y, vx, vy;
    int life;
    Color color;
};
struct FlyingCard {
    Card card;
    float x, y;
    int life;
};
class App {
  public:
    Run run;
    Canvas canvas;
    bool title = true, canResume = false, quit = false;
    int dialog = 0, page = 0, inspectJoker = 0, time = 0, animation = 0;
    int mx = 160, my = 120, hover = -1, hoverTime = 0, keyboardFocus = -1;
    Score lastScore, preview;
    std::vector<Hit> hits;
    std::vector<Particle> particles;
    std::vector<FlyingCard> flying;
    float cardX[52] = {}, cardY[52] = {};
    std::string savePath, toast;
    int toastTime = 0;
    explicit App(std::string path);
    void update(const Input &input, int milliseconds);
    void draw();
    void act(int action);
    void save();
    void refreshPreview();
    int hitAt(int x, int y) const;
    int animationDuration() const;
    Rect cardRect(int index) const;
    Rect jokerRect(int index) const;
    void notify(const std::string &message);

  private:
    int pressedAction = -1, pressX = 0, pressY = 0, drag = -1;
    bool dragging = false;
    void button(Rect r, const std::string &label, int action, Color color = BLUE,
                bool enabled = true);
    void panel(Rect r, Color color = PANEL);
    void hud();
    void shelf();
    void titleScreen();
    void table();
    void blindScreen();
    void shop();
    void endScreen();
    void overlay();
    void tooltip();
    void scoring();
    void spark(int x, int y, Color color, int count = 14);
};
} // namespace bt
