#include "app.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

using namespace bt;
static int checks = 0;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
static Run fixture(std::initializer_list<Card> cards) {
    Run r;
    r.start(444);
    r.phase = PLAY;
    r.handCount = int(cards.size());
    r.drawPos = r.handCount;
    int i = 0;
    for (const Card &c : cards) {
        r.deck[i] = c;
        r.hand[i] = i;
        r.selected[i] = true;
        ++i;
    }
    return r;
}
static void exhaustivePoker() {
    const uint64_t expected[] = {1302540, 1098240, 123552, 54912, 10200, 5108, 3744, 624, 40};
    uint64_t counts[HAND_TYPES] = {};
    Card deck[52];
    for (int i = 0; i < 52; ++i)
        deck[i] = {i % 13 + 2, i / 13, 0};
    std::vector<Card> h(5);
    for (int a = 0; a < 48; ++a) {
        h[0] = deck[a];
        for (int b = a + 1; b < 49; ++b) {
            h[1] = deck[b];
            for (int c = b + 1; c < 50; ++c) {
                h[2] = deck[c];
                for (int d = c + 1; d < 51; ++d) {
                    h[3] = deck[d];
                    for (int e = d + 1; e < 52; ++e) {
                        h[4] = deck[e];
                        ++counts[evaluate(h).type];
                    }
                }
            }
        }
    }
    for (int i = 0; i < 9; ++i)
        CHECK(counts[i] == expected[i]);
    for (int i = 9; i < HAND_TYPES; ++i)
        CHECK(counts[i] == 0);
    std::puts("PASS: all 2,598,960 ordinary five-card hands match known poker frequencies");
    CHECK(evaluate({{14, 0}, {2, 1}, {3, 0}, {4, 2}, {5, 3}}).type == STRAIGHT);
    CHECK(evaluate({{14, 0}, {2, 1}, {3, 0}, {4, 2}, {6, 3}}).type == HIGH);
    CHECK(evaluate({{2, 0}, {2, 1}, {13, 2}}).mask == 3);
    CHECK(evaluate({{2, 0}, {2, 1}, {3, 2}, {3, 1}, {14, 0}}).mask == 15);
    CHECK(evaluate({{9, 0}, {9, 1}, {9, 2}, {9, 3}, {14, 0}}).mask == 15);
    CHECK(evaluate({{8, 0}, {8, 1}, {8, 2}, {8, 3}, {8, 0}}).type == FIVE_KIND);
    CHECK(evaluate({{8, 0}, {8, 0}, {8, 0}, {2, 0}, {2, 0}}).type == FLUSH_HOUSE);
    CHECK(evaluate({{8, 0}, {8, 0}, {8, 0}, {8, 0}, {8, 0}}).type == FLUSH_FIVE);
}
static void rules() {
    Score s;
    Run r = fixture({{2, 0}, {2, 1}, {9, 3}});
    CHECK(r.play(s));
    CHECK(s.total == 28);
    CHECK(s.eval.mask == 3);
    CHECK(r.hands == 3);
    r = fixture({{2, 0}, {2, 1}, {9, 3}});
    r.jokers[0] = {BASIC, 0};
    r.jokerCount = 1;
    CHECK(r.play(s));
    CHECK(s.total == 84);
    r = fixture({{10, 0}, {10, 1}, {10, 2}, {5, 0}, {5, 1}});
    CHECK(r.play(s));
    CHECK(s.total == 320);
    CHECK(r.phase == SHOP);
    CHECK(r.lastPayout == 6);
    CHECK(r.money == 10);
    r = fixture({{14, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 0}});
    r.jokerCount = 2;
    r.jokers[0] = {BASIC, 0};
    r.jokers[1] = {ACROBAT, 0};
    r.hands = 1;
    CHECK(r.play(s));
    CHECK(s.total == 55 * 24);
    r = fixture({{14, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 0}});
    r.jokerCount = 2;
    r.jokers[0] = {ACROBAT, 0};
    r.jokers[1] = {BASIC, 0};
    r.hands = 1;
    CHECK(r.play(s));
    CHECK(s.total == 55 * 16);
    r = fixture({{14, 1, 1}, {2, 1, 2}, {3, 1, 0}, {4, 1, 0}, {5, 1, 0}});
    r.blind = 2;
    r.boss = HEARTS;
    r.updateTarget();
    CHECK(r.play(s));
    CHECK(s.total == 100 * 8);
    r = fixture({{10, 0}, {10, 1}});
    r.blind = 2;
    r.boss = FIVE_CARDS;
    CHECK(!r.play(s));
    CHECK(r.hands == 4);
    CHECK(r.selectedCount() == 2);
    r.start(22);
    r.blind = 2;
    r.boss = WATER;
    r.beginBlind();
    CHECK(r.discards == 0);
    r.toggle(0);
    CHECK(!r.discard());
    r.start(23);
    r.blind = 2;
    r.boss = MANACLE;
    r.beginBlind();
    CHECK(r.handCount == 7);
    r.start(23);
    r.blind = 2;
    r.boss = WALL;
    r.updateTarget();
    CHECK(r.target == 1200);
    r.start(24);
    r.beginBlind();
    bool used[52] = {};
    for (int turn = 0; turn < 3; ++turn) {
        for (int i = 0; i < 5; ++i) {
            r.selected[i] = true;
            used[r.hand[i]] = true;
        }
        CHECK(r.discard());
        for (int i = 0; i < r.handCount; ++i) {
            CHECK(r.hand[i] >= 0 && r.hand[i] < 52);
            CHECK(!used[r.hand[i]]);
        }
        CHECK(r.valid());
    }
    CHECK(r.drawPos == 23);
    r.start(31);
    r.beginBlind();
    for (int i = 0; i < 6; ++i)
        r.toggle(i);
    CHECK(r.selectedCount() == 5);
    int chosen = r.hand[0];
    r.reorderCard(0, 7);
    CHECK(r.hand[7] == chosen);
    CHECK(r.selected[7]);
    r.sortMode = 1;
    r.sortHand();
    CHECK(r.selectedCount() == 5);
    r.phase = SHOP;
    r.money = 20;
    r.rollShop();
    r.offers[0] = {0, BASIC, 2, false};
    CHECK(r.buy(0));
    CHECK(r.money == 18 && r.jokerCount == 1);
    CHECK(!r.buy(0));
    CHECK(r.sell(0));
    CHECK(r.money == 19 && r.jokerCount == 0);
    r.offers[2] = {1, FLUSH, 3, false};
    CHECK(r.buy(2));
    CHECK(r.levels[FLUSH] == 2 && r.planetsUsed == 1);
    r.offers[3] = {2, 1, 5, false};
    CHECK(r.buy(3));
    int enhanced = 0;
    for (const Card &c : r.deck)
        enhanced += c.bonus == 1;
    CHECK(enhanced == 3);
    r.money = 0;
    Offer before = r.offers[0];
    CHECK(!r.rerollShop());
    CHECK(r.offers[0].item == before.item);
    r.start(77);
    CHECK(r.skipBlind());
    CHECK(r.blind == 1 && r.money == 8);
    CHECK(r.skipBlind());
    CHECK(r.blind == 2);
    CHECK(!r.skipBlind());
    r = fixture({{14, 0}, {14, 1}, {14, 2}, {14, 3}});
    r.ante = 8;
    r.blind = 2;
    r.boss = WATER;
    r.levels[QUADS] = 99;
    r.updateTarget();
    CHECK(r.play(s));
    CHECK(r.phase == WON);
    r.endless();
    CHECK(r.ante == 9 && r.blind == 0 && r.phase == BLIND);
    CHECK(r.valid());
    r = fixture({{2, 0}});
    r.hands = 1;
    CHECK(r.play(s));
    CHECK(r.phase == LOST);
    std::puts("PASS: scoring masks, joker order, bosses, economy, upgrades, win/loss, endless");
}
static void saves() {
    Run r;
    r.start(123456);
    r.beginBlind();
    r.toggle(2);
    CHECK(r.save("build/test-save.tns"));
    Run restored;
    CHECK(restored.load("build/test-save.tns"));
    CHECK(restored.rng == r.rng && restored.selected[2] && restored.handCount == 8);
    for (int i = 0; i < 52; ++i)
        CHECK(restored.order[i] == r.order[i]);
    CHECK(r.discard());
    CHECK(r.save("build/test-save.tns"));
    CHECK(restored.load("build/test-save.tns"));
    CHECK(restored.discards == 2 && restored.drawPos == 9);
    FILE *f = std::fopen("build/test-save.tns", "rb");
    CHECK(f);
    unsigned char bytes[4096];
    size_t n = std::fread(bytes, 1, sizeof bytes, f);
    std::fclose(f);
    bytes[40] ^= 123;
    f = std::fopen("build/corrupt-save.tns", "wb");
    CHECK(f);
    CHECK(std::fwrite(bytes, 1, n, f) == n);
    std::fclose(f);
    uint32_t seed = restored.seed;
    CHECK(!restored.load("build/corrupt-save.tns"));
    CHECK(restored.seed == seed);
    f = std::fopen("build/truncated-save.tns", "wb");
    CHECK(f);
    std::fwrite(bytes, 1, 17, f);
    std::fclose(f);
    CHECK(!restored.load("build/truncated-save.tns"));
    CHECK(r.save("build/recovery-save.tns.bak.tns"));
    CHECK(restored.load("build/recovery-save.tns"));
    CHECK(restored.seed == r.seed);
    std::puts("PASS: save/resume, atomic replacement, corrupt and truncated save rejection");
}
static void settle(App &app) {
    Input in;
    in.x = 315;
    in.y = 235;
    for (int i = 0; i < 18; ++i) {
        app.update(in, 33);
        app.draw();
    }
}
static void click(App &a, int x, int y) {
    Input in;
    in.x = x;
    in.y = y;
    in.down = in.pressed = true;
    a.update(in, 33);
    a.draw();
    in.down = in.pressed = false;
    in.released = true;
    a.update(in, 33);
    a.draw();
}
static void ui() {
    App a("build/ui-save.tns");
    a.run.start(22);
    a.canResume = false;
    a.title = true;
    a.draw();
    click(a, 242, 130);
    CHECK(!a.title && a.run.phase == BLIND);
    a.act(START_BLIND);
    settle(a);
    Rect r = a.cardRect(0);
    click(a, r.x + 10, r.y + 20);
    CHECK(a.run.selectedCount() == 1);
    int old = a.run.hand[0];
    r = a.cardRect(0);
    Rect destination = a.cardRect(6);
    Input in;
    in.x = r.x + 10;
    in.y = r.y + 20;
    in.down = in.pressed = true;
    a.update(in, 33);
    a.draw();
    in.pressed = false;
    in.x = destination.x + 10;
    in.y = destination.y + 20;
    a.update(in, 33);
    a.draw();
    in.released = true;
    in.down = false;
    a.update(in, 33);
    a.draw();
    CHECK(a.run.hand[6] == old);
    CHECK(a.run.selected[6]);
    in = Input();
    in.keys = K_ESCAPE;
    a.update(in, 33);
    a.draw();
    CHECK(a.title);
    in.keys = K_ESCAPE;
    a.update(in, 33);
    a.draw();
    CHECK(!a.title);
    a.act(HELP);
    a.draw();
    CHECK(a.dialog == 1);
    a.act(HELP_PAGE);
    CHECK(a.page == 1);
    a.act(CLOSE);
    a.draw();
    CHECK(a.dialog == 0);
    a.act(PLAY_HAND);
    CHECK(a.animation > 0);
    in = Input();
    in.pressed = true;
    a.update(in, 33);
    CHECK(a.animation == 0);
    CHECK(a.run.hands == 3);
    std::puts("PASS: real pointer selection/dragging, pause/resume, help, scoring fast-forward");
}
static void renders() {
    mkdir("build/screens", 0777);
    App a("build/render-save-unused.tns");
    a.canResume = false;
    a.title = true;
    a.draw();
    CHECK(a.canvas.ppm("build/screens/title.ppm"));
    a.title = false;
    a.run.start(1784);
    a.run.beginBlind();
    a.run.jokerCount = 4;
    a.run.jokers[0] = {BASIC, 0};
    a.run.jokers[1] = {FIBONACCI, 0};
    a.run.jokers[2] = {CARD_SHARP, 0};
    a.run.jokers[3] = {CONSTELLATION, 0};
    a.run.deck[a.run.hand[0]] = {14, 0, 0};
    a.run.deck[a.run.hand[1]] = {14, 1, 0};
    a.run.deck[a.run.hand[2]] = {14, 2, 1};
    a.run.selected[0] = a.run.selected[1] = a.run.selected[2] = true;
    a.refreshPreview();
    settle(a);
    CHECK(a.canvas.ppm("build/screens/table.ppm"));
    a.run.phase = BLIND;
    a.run.blind = 2;
    a.run.boss = MANACLE;
    a.run.updateTarget();
    a.draw();
    CHECK(a.canvas.ppm("build/screens/blind.ppm"));
    a.run.phase = SHOP;
    a.run.money = 18;
    a.run.lastPayout = 9;
    a.run.rollShop();
    a.run.offers[0] = {0, JOLLY, 4, false};
    a.run.offers[1] = {0, ACROBAT, 6, false};
    a.run.offers[2] = {1, TRIPS, 3, false};
    a.run.offers[3] = {2, 2, 5, false};
    a.draw();
    CHECK(a.canvas.ppm("build/screens/shop.ppm"));
    a.dialog = 1;
    a.page = 0;
    a.draw();
    CHECK(a.canvas.ppm("build/screens/help.ppm"));
    a.dialog = 2;
    a.draw();
    CHECK(a.canvas.ppm("build/screens/deck.ppm"));
    a.dialog = 3;
    a.inspectJoker = 3;
    a.draw();
    CHECK(a.canvas.ppm("build/screens/joker.ppm"));
    a.dialog = 0;
    a.run.phase = PLAY;
    a.run.blind = 0;
    a.run.hands = 4;
    a.run.play(a.lastScore);
    a.animation = 1000;
    a.draw();
    CHECK(a.canvas.ppm("build/screens/scoring.ppm"));
    a.animation = 0;
    a.run.phase = WON;
    a.draw();
    CHECK(a.canvas.ppm("build/screens/win.ppm"));
    std::puts("PASS: nine screenshots rendered from the calculator framebuffer renderer");
}
int main() {
    exhaustivePoker();
    rules();
    saves();
    ui();
    renders();
    std::printf("%d checks passed.\n", checks);
    return 0;
}
