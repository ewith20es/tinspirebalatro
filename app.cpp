#include "app.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

namespace bt {
static std::string money(int n) {
    return "$" + number(n);
}
static std::string multText(int64_t m) {
    if (m % 100 == 0)
        return number(m / 100);
    char b[32];
    std::snprintf(b, sizeof b, "%.1f", double(m) / 100);
    return b;
}
App::App(std::string path) : savePath(std::move(path)) {
    canResume = run.load(savePath);
    for (int i = 0; i < 52; ++i) {
        cardX[i] = 286;
        cardY[i] = 145;
    }
    draw();
}
int App::hitAt(int x, int y) const {
    for (auto i = hits.rbegin(); i != hits.rend(); ++i)
        if (i->rect.has(x, y))
            return i->action;
    return -1;
}
Rect App::cardRect(int i) const {
    int step = run.handCount <= 1 ? 0 : std::min(28, 201 / (run.handCount - 1));
    int total = (run.handCount - 1) * step + 29;
    int x = 84 + (231 - total) / 2 + i * step;
    int arc = std::abs(2 * i - run.handCount + 1) / 3;
    return {x, 165 + arc - (run.selected[i] ? 10 : 0), 29, 42};
}
Rect App::jokerRect(int i) const {
    return {88 + i * 44, 17, 35, 39};
}
int App::animationDuration() const {
    return 450 + int(lastScore.steps.size()) * 230 + 650;
}
void App::notify(const std::string &text) {
    toast = text;
    toastTime = 2300;
}
void App::save() {
    canResume = true;
    if (!run.save(savePath))
        notify("Save failed - check free space");
}
void App::refreshPreview() {
    preview = Score();
    if (run.phase == PLAY && run.selectedCount()) {
        Run copy = run;
        copy.play(preview);
    }
}
void App::spark(int x, int y, Color color, int count) {
    for (int i = 0; i < count && particles.size() < 70; ++i) {
        float a = float((i * 137 + time) % 360) * 3.14159f / 180.f;
        float v = 20.f + (i % 6) * 8;
        particles.push_back(
            {float(x), float(y), std::cos(a) * v, std::sin(a) * v, 450 + (i % 5) * 80, color});
    }
}
void App::act(int action) {
    if (animation) {
        animation = animationDuration();
        return;
    }
    if (action >= OFFER_ACTION && action < OFFER_ACTION + 4) {
        if (run.buy(action - OFFER_ACTION)) {
            spark(mx, my, GOLD);
            save();
        }
        notify(run.message);
        return;
    }
    if (action >= JOKER_ACTION && action < JOKER_ACTION + run.jokerCount) {
        inspectJoker = action - JOKER_ACTION;
        dialog = 3;
        hoverTime = 0;
        return;
    }
    if (action >= CARD_ACTION && action < CARD_ACTION + run.handCount) {
        run.toggle(action - CARD_ACTION);
        refreshPreview();
        return;
    }
    switch (action) {
    case RESUME:
        title = false;
        dialog = 0;
        refreshPreview();
        break;
    case NEW_RUN:
        if (canResume && run.phase != LOST && run.phase != WON)
            dialog = 4;
        else
            act(CONFIRM_NEW);
        break;
    case CONFIRM_NEW: {
        uint32_t seed = uint32_t(std::time(nullptr)) ^ (uint32_t(time) * 2654435761u) ^ run.rng;
        run.start(seed);
        title = false;
        dialog = 0;
        animation = 0;
        particles.clear();
        flying.clear();
        for (int i = 0; i < 52; ++i) {
            cardX[i] = 286;
            cardY[i] = 145;
        }
        save();
        break;
    }
    case QUIT:
        if (canResume)
            save();
        quit = true;
        break;
    case HELP:
        dialog = 1;
        page = 0;
        break;
    case MENU:
        title = true;
        dialog = 0;
        if (canResume)
            save();
        break;
    case CLOSE:
        dialog = 0;
        break;
    case HELP_PAGE:
        page = (page + 1) % 3;
        break;
    case DECK_VIEW:
        dialog = 2;
        page = 0;
        break;
    case DECK_PREV:
        page = (page + 6) % 7;
        break;
    case DECK_NEXT:
        page = (page + 1) % 7;
        break;
    case SELL:
        if (run.sell(inspectJoker)) {
            dialog = 0;
            save();
            notify(run.message);
        }
        break;
    case START_BLIND:
        run.beginBlind();
        for (int i = 0; i < 52; ++i) {
            cardX[i] = 286;
            cardY[i] = 145;
        }
        save();
        refreshPreview();
        break;
    case SKIP_BLIND:
        if (run.skipBlind()) {
            save();
            notify(run.message);
        }
        break;
    case PLAY_HAND:
        if (run.play(lastScore)) {
            animation = 1;
            save();
            spark(196, 99, GOLD, 18);
            refreshPreview();
        } else if (run.selectedCount())
            notify(run.message);
        else
            notify("Select cards first");
        break;
    case DISCARD_HAND:
        if (run.phase == PLAY && run.discards > 0 && run.selectedCount()) {
            for (int i = 0; i < run.handCount; ++i)
                if (run.selected[i]) {
                    Rect r = cardRect(i);
                    flying.push_back({run.deck[run.hand[i]], float(r.x), float(r.y), 380});
                }
            run.discard();
            save();
            refreshPreview();
        } else
            notify(run.discards <= 0 ? "No discards left" : "Select cards first");
        break;
    case SORT:
        run.sortMode = 1 - run.sortMode;
        run.sortHand();
        refreshPreview();
        notify(run.sortMode ? "Sorted by suit" : "Sorted by rank");
        break;
    case REROLL:
        if (run.rerollShop())
            save();
        notify(run.message);
        break;
    case NEXT_BLIND:
        run.leaveShop();
        save();
        break;
    case ENDLESS:
        run.endless();
        save();
        notify("Endless mode");
        break;
    default:
        break;
    }
    keyboardFocus = -1;
    hoverTime = 0;
}
void App::update(const Input &in, int dt) {
    dt = std::max(1, std::min(100, dt));
    time += dt;
    mx = std::max(0, std::min(319, in.x));
    my = std::max(0, std::min(239, in.y));
    for (auto &p : particles) {
        p.life -= dt;
        p.x += p.vx * dt / 1000.f;
        p.y += p.vy * dt / 1000.f;
        p.vy += dt * .065f;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle &p) { return p.life <= 0; }),
                    particles.end());
    for (auto &f : flying) {
        f.life -= dt;
        f.x += dt * .24f;
        f.y -= dt * .22f;
    }
    flying.erase(std::remove_if(flying.begin(), flying.end(),
                                [](const FlyingCard &f) { return f.life <= 0; }),
                 flying.end());
    if (toastTime > 0)
        toastTime -= dt;
    int newHover = hitAt(mx, my);
    if (newHover != hover) {
        hover = newHover;
        hoverTime = 0;
    } else
        hoverTime += dt;
    for (int i = 0; i < run.handCount; ++i) {
        Rect r = cardRect(i);
        if (hover == CARD_ACTION + i && !run.selected[i])
            r.y -= 3;
        int c = run.hand[i];
        float t = std::min(1.f, dt / 75.f);
        cardX[c] += (r.x - cardX[c]) * t;
        cardY[c] += (r.y - cardY[c]) * t;
    }
    if (animation) {
        int before = (animation - 450) / 230;
        animation += dt;
        int after = (animation - 450) / 230;
        if (after != before && after >= 0 && after < int(lastScore.steps.size()))
            spark(190, 99, lastScore.steps[after].joker >= 0 ? GOLD : BLUE, 5);
        if (in.pressed || (in.keys & (K_PLAY | K_ACTIVATE | K_SPEED)))
            animation = animationDuration();
        if (animation >= animationDuration()) {
            animation = 0;
            if (run.phase == SHOP || run.phase == WON)
                spark(206, 90, GOLD, 45);
            if (run.phase == SHOP)
                notify("Blind cleared! +" + money(lastScore.moneyGained));
        }
        return;
    }
    if (in.keys & K_ESCAPE) {
        if (dialog)
            act(CLOSE);
        else if (!title)
            act(MENU);
        else if (canResume)
            act(RESUME);
        return;
    }
    if (in.keys & K_HELP) {
        if (dialog == 1)
            act(CLOSE);
        else
            act(HELP);
        return;
    }
    if (in.keys & K_TAB) {
        int index = -1;
        for (size_t i = 0; i < hits.size(); ++i)
            if (hits[i].action == keyboardFocus)
                index = int(i);
        if (!hits.empty())
            keyboardFocus = hits[(index + 1) % hits.size()].action;
    }
    if (in.keys & K_ACTIVATE) {
        if (keyboardFocus >= 0)
            act(keyboardFocus);
        else if (hover >= 0)
            act(hover);
    }
    if (!title && !dialog) {
        if (in.keys & K_PLAY) {
            if (run.phase == PLAY)
                act(PLAY_HAND);
            else if (run.phase == BLIND)
                act(START_BLIND);
            else if (run.phase == SHOP)
                act(NEXT_BLIND);
        }
        if (in.keys & K_DISCARD)
            act(DISCARD_HAND);
        if (in.keys & K_SORT)
            act(SORT);
        if (in.keys & K_REROLL)
            act(REROLL);
        if (in.card >= 0 && in.card < run.handCount && run.phase == PLAY)
            act(CARD_ACTION + in.card);
    }
    if (in.pressed) {
        pressedAction = hitAt(mx, my);
        pressX = mx;
        pressY = my;
        drag = -1;
        dragging = false;
        if (!dialog && !title) {
            if (pressedAction >= CARD_ACTION && pressedAction < CARD_ACTION + run.handCount)
                drag = pressedAction;
            if (pressedAction >= JOKER_ACTION && pressedAction < JOKER_ACTION + run.jokerCount)
                drag = pressedAction;
        }
    }
    if (in.down && drag >= 0 && (std::abs(mx - pressX) + std::abs(my - pressY) > 7))
        dragging = true;
    if (in.released) {
        if (dragging && drag >= 0) {
            if (drag >= JOKER_ACTION) {
                int best = 0;
                for (int i = 0; i < run.jokerCount; ++i)
                    if (std::abs(mx - (jokerRect(i).x + 17)) <
                        std::abs(mx - (jokerRect(best).x + 17)))
                        best = i;
                run.reorderJoker(drag - JOKER_ACTION, best);
                save();
            } else {
                int best = 0;
                for (int i = 0; i < run.handCount; ++i)
                    if (std::abs(mx - (cardRect(i).x + 14)) <
                        std::abs(mx - (cardRect(best).x + 14)))
                        best = i;
                run.reorderCard(drag - CARD_ACTION, best);
            }
            refreshPreview();
        } else if (pressedAction >= 0 && pressedAction == hitAt(mx, my))
            act(pressedAction);
        drag = -1;
        dragging = false;
        pressedAction = -1;
    }
}
void App::panel(Rect r, Color color) {
    canvas.round(r.x + 1, r.y + 2, r.w, r.h, rgb(11, 25, 29));
    canvas.round(r.x, r.y, r.w, r.h, color);
}
void App::button(Rect r, const std::string &label, int action, Color color, bool enabled) {
    bool hot = enabled && (hover == action || keyboardFocus == action);
    if (hot)
        canvas.round(r.x - 1, r.y - 1, r.w + 2, r.h + 2, GOLD);
    canvas.round(r.x, r.y + 2, r.w, r.h, rgb(13, 30, 35));
    canvas.round(r.x, r.y, r.w, r.h, enabled ? color : rgb(59, 74, 78));
    canvas.center(r.x, r.y + (r.h - 7) / 2, r.w, label, enabled ? WHITE : rgb(126, 143, 147));
    if (enabled)
        hits.push_back({r, action});
}
void App::hud() {
    panel({7, 7, 69, 224});
    canvas.text(13, 13, "BALATRO", GOLD);
    canvas.line(12, 24, 70, 24, rgb(78, 97, 101));
    canvas.text(13, 31, "ANTE " + number(run.ante) + "/8", WHITE);
    panel({12, 45, 59, 32}, run.blind == 2 ? rgb(133, 65, 67) : rgb(53, 85, 107));
    canvas.center(12, 50, 59, run.blind == 0 ? "SMALL" : run.blind == 1 ? "BIG" : "BOSS");
    canvas.center(12, 62, 59, "BLIND", GOLD);
    canvas.text(13, 84, "TARGET", MUTED);
    canvas.center(11, 97, 61, number(run.target), GOLD);
    canvas.text(13, 114, "SCORE", MUTED);
    int64_t score = run.score;
    if (animation) {
        score = lastScore.before;
        int end = 450 + int(lastScore.steps.size()) * 230;
        if (animation > end)
            score += lastScore.total * std::min(animation - end, 600) / 600;
    }
    canvas.center(11, 127, 61, number(score), WHITE);
    int bar = int(std::min<int64_t>(57, 57 * score / std::max<int64_t>(1, run.target)));
    canvas.round(13, 139, 57, 3, rgb(16, 31, 35), 1);
    canvas.rect(13, 139, bar, 3, GOLD);
    canvas.text(13, 153, "HANDS", MUTED);
    canvas.text(61, 153, number(run.hands), BLUE);
    canvas.text(13, 167, "DISCARD", MUTED);
    canvas.text(61, 167, number(run.discards), RED);
    canvas.text(13, 195, money(run.money), GOLD, 1);
    button({12, 214, 59, 13}, "MENU", MENU, rgb(68, 85, 96));
}
void App::shelf() {
    canvas.text(88, 6, "JOKERS " + number(run.jokerCount) + "/5", MUTED);
    canvas.text(281, 6, "[H] ?", GOLD);
    for (int i = 0; i < MAX_JOKERS; ++i) {
        Rect r = jokerRect(i);
        if (i < run.jokerCount) {
            if (!(dragging && drag == JOKER_ACTION + i))
                canvas.joker(r.x, r.y, run.jokers[i].kind,
                             hover == JOKER_ACTION + i || keyboardFocus == JOKER_ACTION + i);
            hits.push_back({r, JOKER_ACTION + i});
        } else {
            canvas.round(r.x, r.y, r.w, r.h, rgb(21, 58, 55));
            canvas.border(r.x + 1, r.y + 1, r.w - 2, r.h - 2, rgb(55, 99, 87));
            canvas.text(r.x + 14, r.y + 16, "+", rgb(74, 118, 103));
        }
    }
    canvas.line(86, 62, 309, 62, rgb(72, 116, 101));
}
void App::titleScreen() {
    canvas.text(77, 28, "BALATRO", rgb(14, 29, 35), 4);
    canvas.text(74, 24, "BALATRO", RED, 4);
    canvas.text(73, 22, "BALATRO", WHITE, 4);
    canvas.center(0, 60, 320, "POKER ROGUELIKE / CX II EDITION", GOLD);
    float wobble = std::sin(time / 700.f) * 3;
    canvas.cardBack(47, 89 + int(wobble), 42, 61);
    canvas.card(77, 83, {14, 0, 0}, false, false, false, 42, 61);
    canvas.card(108, 91 - int(wobble), {13, 1, 0}, false, false, false, 42, 61);
    canvas.joker(137, 80 + int(wobble), BASIC, false, 43, 62);
    canvas.center(185, 83, 118, "BUILD A HAND.", WHITE);
    canvas.center(185, 96, 118, "BREAK THE SCORE.", MUTED);
    if (canResume)
        button({194, 115, 108, 24}, "RESUME", RESUME, rgb(54, 143, 120));
    button({194, canResume ? 148 : 118, 108, 24}, "NEW RUN", NEW_RUN, RED);
    button({24, 185, 130, 22}, "HOW TO PLAY", HELP, BLUE);
    button({166, 185, 130, 22}, canResume ? "SAVE & QUIT" : "QUIT", QUIT, rgb(75, 88, 101));
    canvas.center(0, 218, 320, "TOUCHPAD = MOUSE   CLICK = SELECT", MUTED);
    canvas.center(0, 230, 320, "FAN-MADE TI-NSPIRE EDITION", rgb(112, 145, 135));
}
void App::table() {
    std::string name =
        run.selectedCount() ? handName(evaluate(run.selectedCards()).type) : "MAKE YOUR HAND";
    canvas.center(83, 71, 232, name, WHITE, name.size() <= 18 ? 2 : 1);
    panel({99, 94, 85, 24}, BLUE);
    panel({205, 94, 85, 24}, RED);
    canvas.text(191, 103, "X", GOLD);
    canvas.center(99, 102, 85, number(preview.chips), WHITE);
    canvas.center(205, 102, 85, multText(preview.mult100), WHITE);
    canvas.center(83, 127, 232,
                  run.selectedCount() ? "PREVIEW  " + number(preview.total) : "SELECT 1 TO 5 CARDS",
                  run.selectedCount() ? GOLD : MUTED);
    canvas.text(87, 146, "HAND " + number(run.handCount) + " / DECK " + number(52 - run.drawPos),
                MUTED);
    canvas.text(265, 146, number(run.selectedCount()) + "/5", GOLD);
    for (int i = 0; i < run.handCount; ++i) {
        int c = run.hand[i];
        Rect r = cardRect(i);
        r.x = int(cardX[c]);
        r.y = int(cardY[c]);
        if (!(dragging && drag == CARD_ACTION + i))
            canvas.card(r.x, r.y, run.deck[c], run.selected[i],
                        hover == CARD_ACTION + i || keyboardFocus == CARD_ACTION + i,
                        run.debuffed(run.deck[c]));
        hits.push_back({r, CARD_ACTION + i});
    }
    button({85, 213, 63, 19}, "PLAY", PLAY_HAND, BLUE, run.selectedCount() > 0);
    button({154, 213, 67, 19}, "DISCARD", DISCARD_HAND, RED,
           run.selectedCount() > 0 && run.discards > 0);
    button({227, 213, 40, 19}, "SORT", SORT, rgb(108, 129, 117));
    button({273, 213, 40, 19}, "DECK", DECK_VIEW, rgb(106, 85, 142));
}
void App::blindScreen() {
    canvas.center(83, 71, 232, "CHOOSE YOUR BLIND", WHITE);
    for (int i = 0; i < 3; ++i) {
        Run sample = run;
        sample.blind = i;
        sample.updateTarget();
        int x = 86 + i * 77;
        bool current = i == run.blind;
        panel({x, 89, 70, 66}, current ? rgb(58, 94, 105) : rgb(26, 60, 62));
        if (current)
            canvas.border(x - 1, 88, 72, 68, GOLD);
        Color color = i == 0 ? BLUE : i == 1 ? rgb(224, 173, 60) : RED;
        canvas.circle(x + 35, 107, 10, color);
        canvas.circle(x + 35, 107, 6, WHITE);
        canvas.circle(x + 35, 107, 3, color);
        canvas.center(x, 123, 70,
                      i == 0   ? "SMALL"
                      : i == 1 ? "BIG"
                               : "BOSS",
                      i < run.blind ? MUTED : WHITE);
        canvas.center(x, 139, 70, i < run.blind ? "CLEARED" : number(sample.target), GOLD);
        if (current)
            hits.push_back({{x, 89, 70, 66}, START_BLIND});
    }
    canvas.wrap(89, 164, 221, std::string(bossName(run.boss)) + ": " + bossRule(run.boss), MUTED,
                3);
    button({87, 207, 133, 24}, "PLAY BLIND", START_BLIND, BLUE);
    button({228, 207, 84, 24}, "SKIP +$4", SKIP_BLIND, rgb(130, 98, 70), run.blind < 2);
}
void App::shop() {
    canvas.text(88, 70, "THE SHOP", GOLD);
    canvas.text(184, 70, "CLEARED +" + money(run.lastPayout), WHITE);
    for (int s = 0; s < 4; ++s) {
        const Offer &o = run.offers[s];
        int x = 87 + (s % 2) * 116, y = 86 + (s / 2) * 61;
        Rect r = {x, y, 107, 54};
        bool hot = hover == OFFER_ACTION + s || keyboardFocus == OFFER_ACTION + s;
        panel(r, o.sold ? rgb(32, 63, 63) : rgb(51, 73, 78));
        if (hot && !o.sold)
            canvas.border(x - 1, y - 1, 109, 56, GOLD);
        if (o.kind == 0)
            canvas.joker(x + 5, y + 6, o.item, false, 26, 39);
        else if (o.kind == 1) {
            canvas.circle(x + 19, y + 22, 12, rgb(143, 106, 188));
            canvas.circle(x + 16, y + 19, 9, rgb(178, 148, 216));
            canvas.line(x + 3, y + 30, x + 33, y + 15, GOLD);
            canvas.pixel(x + 9, y + 5, WHITE);
        } else {
            canvas.cardBack(x + 5, y + 6, 26, 39);
            canvas.text(x + 14, y + 20,
                        o.item == 0   ? "+"
                        : o.item == 1 ? "C"
                        : o.item == 2 ? "M"
                                      : "X",
                        GOLD);
        }
        std::string name = o.kind == 0   ? jokerDef(o.item).name
                           : o.kind == 1 ? planetName(o.item)
                           : o.item == 0 ? "STRENGTH"
                           : o.item == 1 ? "BONUS PACK"
                           : o.item == 2 ? "MULT PACK"
                                         : "GLASS PACK";
        canvas.wrap(x + 36, y + 6, 67, name, WHITE, 2);
        canvas.text(x + 36, y + 39, o.sold ? "SOLD" : money(o.price) + " BUY",
                    o.sold ? MUTED : GOLD);
        if (!o.sold)
            hits.push_back({r, OFFER_ACTION + s});
    }
    button({87, 215, 105, 18}, "REROLL " + money(run.reroll), REROLL, RED, run.money >= run.reroll);
    button({203, 215, 107, 18}, "NEXT BLIND", NEXT_BLIND, BLUE);
}
void App::endScreen() {
    bool won = run.phase == WON;
    canvas.center(83, 82, 232, won ? "YOU WIN!" : "RUN OVER", won ? GOLD : RED, 3);
    canvas.center(83, 118, 232, won ? "ANTE 8 COMPLETE" : "EVERY RUN IS A NEW DECK", WHITE);
    canvas.center(83, 141, 232, "BEST HAND  " + number(run.bestHand), MUTED);
    canvas.center(83, 156, 232, "BLINDS WON  " + number(run.roundsWon), MUTED);
    if (won)
        button({98, 180, 200, 22}, "CONTINUE ENDLESS", ENDLESS, rgb(58, 143, 115));
    button({98, 210, 200, 22}, "NEW RUN", NEW_RUN, BLUE);
}
void App::scoring() {
    int step = std::max(0, std::min(int(lastScore.steps.size()) - 1, (animation - 450) / 230));
    if (lastScore.steps.empty())
        return;
    const ScoreStep &s = lastScore.steps[step];
    panel({84, 65, 230, 80}, rgb(31, 61, 66));
    canvas.center(86, 71, 225, handName(lastScore.eval.type), GOLD);
    panel({95, 87, 91, 23}, BLUE);
    panel({211, 87, 91, 23}, RED);
    canvas.text(196, 95, "X");
    canvas.center(95, 95, 91, number(s.chips));
    canvas.center(211, 95, 91, multText(s.mult100));
    canvas.center(86, 119, 225, s.label, WHITE);
    int count = int(lastScore.cards.size());
    for (int i = 0; i < count; ++i) {
        int x = 194 - count * 18 + i * 36;
        bool active = s.card == i;
        int y = 151 - (active ? 4 : 0);
        canvas.card(x, y, lastScore.cards[i], active, false, run.debuffed(lastScore.cards[i]));
    }
    int finalTime = 450 + int(lastScore.steps.size()) * 230;
    if (animation >= finalTime) {
        panel({102, 197, 192, 33}, rgb(100, 85, 50));
        canvas.center(104, 202, 188, "+" + number(lastScore.total), GOLD, 2);
    } else
        canvas.center(84, 224, 231, "CLICK TO SPEED UP", MUTED);
    if (s.joker >= 0) {
        Rect r = jokerRect(s.joker);
        canvas.border(r.x - 2, r.y - 2, r.w + 4, r.h + 4, GOLD);
    }
    hits.clear();
}
void App::tooltip() {
    if (hoverTime < 650 || dragging || dialog || animation || title)
        return;
    std::string titleText, detail;
    if (hover >= JOKER_ACTION && hover < JOKER_ACTION + run.jokerCount) {
        int i = hover - JOKER_ACTION;
        titleText = jokerDef(run.jokers[i].kind).name;
        detail = jokerDef(run.jokers[i].kind).detail;
        if (run.jokers[i].kind == RUNNER)
            detail += " Current: +" + number(run.jokers[i].value) + " chips.";
    } else if (hover >= OFFER_ACTION && hover < OFFER_ACTION + 4) {
        const Offer &o = run.offers[hover - OFFER_ACTION];
        if (o.kind == 0) {
            titleText = jokerDef(o.item).name;
            detail = jokerDef(o.item).detail;
        } else if (o.kind == 1) {
            titleText = planetName(o.item);
            detail = std::string("Level up ") + handName(o.item) + ". Current level " +
                     number(run.levels[o.item]) + ".";
        } else {
            titleText = "UPGRADE YOUR DECK";
            detail = o.item == 0 ? "3 random cards permanently gain one rank. Ace wraps to 2."
                     : o.item == 1
                         ? "3 random cards gain +30 scoring chips. Replaces prior enhancements."
                     : o.item == 2
                         ? "3 random cards gain +4 scoring Mult. Replaces prior enhancements."
                         : "3 random cards gain X1.5 scoring Mult. Replaces prior enhancements.";
        }
    } else if (hover >= CARD_ACTION && hover < CARD_ACTION + run.handCount) {
        const Card &c = run.deck[run.hand[hover - CARD_ACTION]];
        titleText = number(cardChips(c)) + " CHIPS";
        detail = c.bonus == 1   ? "Bonus card: +30 chips when scored."
                 : c.bonus == 2 ? "Mult card: +4 Mult when scored."
                 : c.bonus == 3
                     ? "Glass card: X1.5 Mult when scored. This version never breaks."
                     : "Click to select. Drag to reorder. Only cards in the poker hand score.";
        if (run.debuffed(c))
            detail = "Debuffed by this boss: no chips or card effects.";
    }
    if (detail.empty())
        return;
    int y = hover >= OFFER_ACTION ? 10 : 68;
    panel({87, y, 223, 68}, rgb(21, 35, 45));
    canvas.text(94, y + 6, titleText, GOLD);
    canvas.wrap(94, y + 20, 208, detail, WHITE, 4);
}
void App::overlay() {
    canvas.dim(155);
    hits.clear();
    panel({22, 22, 276, 197}, rgb(34, 49, 60));
    canvas.border(24, 24, 272, 193, rgb(142, 134, 110));
    if (dialog == 1) {
        canvas.center(25, 33, 270,
                      page == 0   ? "HOW TO PLAY"
                      : page == 1 ? "POKER HANDS"
                                  : "BUILD YOUR ENGINE",
                      GOLD, 2);
        if (page == 0) {
            canvas.wrap(
                35, 62, 250,
                "Make poker hands to beat the blind target. Select up to 5 cards, then PLAY. Blue "
                "chips X red Mult = your score.\n\nTouchpad: move cursor. Press: click.\nDrag "
                "cards or Jokers to reorder.\nP / Enter: play. D: discard. S: sort.\n1-8: select "
                "cards. Space: click.\nH: help. Esc: pause / resume.",
                WHITE, 13);
        } else if (page == 1) {
            const char *a[] = {
                "High card       5 x 1", "Pair           10 x 2", "Two pair       20 x 2",
                "Three alike    30 x 3", "Straight       30 x 4", "Flush          35 x 4",
                "Full house     40 x 4", "Four alike     60 x 7", "Straight flush 100 x 8"};
            for (int i = 0; i < 9; ++i)
                canvas.text(39, 62 + i * 13, a[i], i % 2 ? MUTED : WHITE);
        } else
            canvas.wrap(
                35, 63, 250,
                "Win: earn blind reward + $1 per hand left + $1 interest per $5 held (max "
                "$5).\n\nShop: buy Jokers, planets and deck upgrades. Click a Joker to inspect or "
                "sell it. Drag Jokers: order matters.\n\nBeat 3 blinds per ante, through ante 8. "
                "Bosses change the rules. The run saves after each action.",
                WHITE, 13);
        button({35, 194, 114, 17}, "NEXT PAGE", HELP_PAGE, BLUE);
        button({170, 194, 114, 17}, "BACK", CLOSE, rgb(99, 116, 120));
    } else if (dialog == 2) {
        canvas.center(25, 33, 270, "YOUR DECK  " + number(page + 1) + "/7", GOLD, 2);
        int idx[52];
        for (int i = 0; i < 52; ++i)
            idx[i] = i;
        std::stable_sort(idx, idx + 52, [&](int a, int b) {
            return run.deck[a].suit * 15 + run.deck[a].rank >
                   run.deck[b].suit * 15 + run.deck[b].rank;
        });
        for (int i = 0; i < 8 && page * 8 + i < 52; ++i) {
            int id = idx[page * 8 + i], x = 50 + (i % 4) * 62, y = 63 + (i / 4) * 55;
            canvas.card(x, y, run.deck[id]);
            bool held = false, remaining = false;
            for (int j = 0; j < run.handCount; ++j)
                held |= run.hand[j] == id;
            for (int j = run.drawPos; j < 52; ++j)
                remaining |= run.order[j] == id;
            canvas.text(x + 32, y + 9, held ? "H" : remaining ? "D" : "-", MUTED);
        }
        canvas.center(25, 177, 270, "H = HELD  D = DRAW PILE  - = USED", MUTED);
        button({35, 194, 60, 17}, "<", DECK_PREV, BLUE);
        button({112, 194, 96, 17}, "BACK", CLOSE, rgb(99, 116, 120));
        button({226, 194, 60, 17}, ">", DECK_NEXT, BLUE);
    } else if (dialog == 3) {
        const Joker &j = run.jokers[inspectJoker];
        canvas.center(25, 36, 270, jokerDef(j.kind).name, GOLD);
        canvas.joker(40, 64, j.kind, false, 50, 61);
        canvas.wrap(106, 65, 177, jokerDef(j.kind).detail, WHITE, 7);
        if (j.kind == RUNNER)
            canvas.text(38, 143, "CURRENT: +" + number(j.value) + " CHIPS", GOLD);
        canvas.wrap(38, 163, 240, "Jokers trigger left to right. Drag to reorder.", MUTED, 2);
        button({35, 194, 116, 17}, "BACK", CLOSE, BLUE);
        button({169, 194, 115, 17}, "SELL " + money(std::max(1, jokerDef(j.kind).cost / 2)), SELL,
               RED, run.phase == SHOP);
    } else if (dialog == 4) {
        canvas.center(25, 47, 270, "START A NEW RUN?", GOLD, 2);
        canvas.wrap(44, 91, 234, "This replaces the saved run. Your current progress will be lost.",
                    WHITE, 4);
        button({35, 172, 116, 26}, "CANCEL", CLOSE, BLUE);
        button({169, 172, 115, 26}, "NEW RUN", CONFIRM_NEW, RED);
    }
}
void App::draw() {
    hits.clear();
    canvas.felt(time);
    if (title)
        titleScreen();
    else {
        hud();
        shelf();
        if (animation)
            scoring();
        else
            switch (run.phase) {
            case BLIND:
                blindScreen();
                break;
            case PLAY:
                table();
                break;
            case SHOP:
                shop();
                break;
            default:
                endScreen();
                break;
            }
    }
    for (const auto &f : flying)
        canvas.card(int(f.x), int(f.y), f.card);
    for (const auto &p : particles)
        canvas.rect(int(p.x), int(p.y), p.life > 300 ? 2 : 1, p.life > 300 ? 2 : 1, p.color);
    if (dragging && drag >= JOKER_ACTION && drag < JOKER_ACTION + run.jokerCount)
        canvas.joker(mx - 17, my - 19, run.jokers[drag - JOKER_ACTION].kind, true);
    else if (dragging && drag >= CARD_ACTION && drag < CARD_ACTION + run.handCount)
        canvas.card(mx - 14, my - 20, run.deck[run.hand[drag - CARD_ACTION]], true);
    tooltip();
    if (toastTime > 0 && !title && !dialog && !animation) {
        panel({85, 127, 229, 26}, rgb(23, 38, 45));
        canvas.wrap(91, 131, 219, toast, GOLD, 2);
    }
    if (dialog)
        overlay();
    // High-contrast software pointer; no OS cursor or touchpad arrow emulation.
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x <= y / 2; ++x)
            canvas.pixel(mx + x + 1, my + y + 1, INK);
    for (int y = 0; y < 9; ++y)
        for (int x = 0; x <= y / 2; ++x)
            canvas.pixel(mx + x, my + y, (x == 0 || x == y / 2 || y == 8) ? INK : WHITE);
}
} // namespace bt
