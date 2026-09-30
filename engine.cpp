#include "engine.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>

namespace bt {
static const char *HAND_NAMES[] = {"High Card",       "Pair",           "Two Pair",
                                   "Three of a Kind", "Straight",       "Flush",
                                   "Full House",      "Four of a Kind", "Straight Flush",
                                   "Five of a Kind",  "Flush House",    "Flush Five"};
static const char *PLANETS[] = {"Pluto", "Mercury", "Uranus",  "Venus",    "Saturn", "Jupiter",
                                "Earth", "Mars",    "Neptune", "Planet X", "Ceres",  "Eris"};
static const int BASE_CHIPS[] = {5, 10, 20, 30, 30, 35, 40, 60, 100, 120, 140, 160};
static const int BASE_MULT[] = {1, 2, 2, 3, 4, 4, 4, 7, 8, 12, 14, 16};
static const int CHIP_UP[] = {10, 15, 20, 20, 30, 15, 25, 30, 40, 35, 40, 50};
static const int MULT_UP[] = {1, 1, 1, 2, 3, 2, 2, 3, 4, 3, 4, 3};
static const char *BOSS_NAMES[] = {"The Heart", "The Club",  "The Diamond", "The Spade",
                                   "The Five",  "The Water", "The Manacle", "The Wall"};
static const char *BOSS_RULES[] = {"Hearts give no chips or card bonuses.",
                                   "Clubs give no chips or card bonuses.",
                                   "Diamonds give no chips or card bonuses.",
                                   "Spades give no chips or card bonuses.",
                                   "Every played hand must have 5 cards.",
                                   "No discards this round.",
                                   "One fewer card in your hand.",
                                   "The score target is doubled."};
static const JokerDef JOKERS[] = {
    {"Joker", "+4 Mult on every played hand.", 2, 0},
    {"Greedy Joker", "+3 Mult for each scoring diamond.", 5, 1},
    {"Lusty Joker", "+3 Mult for each scoring heart.", 5, 0},
    {"Wrathful Joker", "+3 Mult for each scoring spade.", 5, 2},
    {"Glutton Joker", "+3 Mult for each scoring club.", 5, 3},
    {"Jolly Joker", "+8 Mult if the hand contains a pair.", 4, 1},
    {"Zany Joker", "+12 Mult if the hand contains three of a kind.", 4, 4},
    {"Mad Joker", "+10 Mult if the hand contains two pairs.", 4, 0},
    {"Crazy Joker", "+12 Mult on straights or straight flushes.", 4, 2},
    {"Droll Joker", "+10 Mult on flush hands.", 4, 3},
    {"Sly Joker", "+50 chips if the hand contains a pair.", 4, 2},
    {"Runner", "Gains +15 chips whenever a straight is played.", 5, 3},
    {"Odd Todd", "+31 chips for each scoring A, 3, 5, 7 or 9.", 5, 1},
    {"Even Steven", "+4 Mult for each scoring 2, 4, 6, 8 or 10.", 5, 0},
    {"Scholar", "+20 chips and +4 Mult per scoring ace.", 5, 4},
    {"Fibonacci", "+8 Mult per scoring A, 2, 3, 5 or 8.", 8, 1},
    {"Acrobat", "X3 Mult on the final hand of a round.", 6, 0},
    {"Card Sharp", "X3 Mult if this hand type was already played this round.", 7, 2},
    {"Supernova", "Adds the number of times this hand type was played to Mult.", 5, 4},
    {"Golden Joker", "Earn an extra $4 when a blind is beaten.", 6, 1},
    {"Fortune Teller", "+1 Mult for each planet card bought this run.", 5, 4},
    {"Constellation", "Starts at X1 Mult. Gains X0.1 for each planet bought.", 8, 4},
    {"The Trio", "X3 Mult if the hand contains three of a kind.", 8, 0},
    {"The Family", "X4 Mult if the hand contains four of a kind.", 8, 2},
    {"Juggler", "Hold one extra card, up to 10 cards.", 5, 3},
    {"Banner", "+30 chips for each discard still available.", 5, 2}};
const char *handName(int t) {
    return HAND_NAMES[std::max(0, std::min(t, HAND_TYPES - 1))];
}
const char *planetName(int t) {
    return PLANETS[std::max(0, std::min(t, HAND_TYPES - 1))];
}
const char *bossName(int b) {
    return BOSS_NAMES[std::max(0, std::min(b, BOSSES - 1))];
}
const char *bossRule(int b) {
    return BOSS_RULES[std::max(0, std::min(b, BOSSES - 1))];
}
const JokerDef &jokerDef(int k) {
    return JOKERS[std::max(0, std::min(k, JOKER_TYPES - 1))];
}
int cardChips(const Card &c) {
    return c.rank == 14 ? 11 : std::min(c.rank, 10);
}
std::string number(int64_t n) {
    char b[40];
    if (n >= 1000000000LL)
        std::snprintf(b, sizeof b, "%.2fB", double(n) / 1000000000.0);
    else if (n >= 1000000)
        std::snprintf(b, sizeof b, "%.2fM", double(n) / 1000000.0);
    else if (n >= 100000)
        std::snprintf(b, sizeof b, "%.1fK", double(n) / 1000.0);
    else
        std::snprintf(b, sizeof b, "%lld", (long long)n);
    return b;
}
Evaluation evaluate(const std::vector<Card> &cards) {
    Evaluation e;
    if (cards.empty() || cards.size() > 5)
        return e;
    int counts[15] = {}, suits[4] = {}, high = 2, pairRank = 0, tripRank = 0, quadRank = 0;
    for (const Card &c : cards) {
        if (c.rank < 2 || c.rank > 14 || c.suit < 0 || c.suit > 3)
            return e;
        counts[c.rank]++;
        suits[c.suit]++;
        high = std::max(high, c.rank);
    }
    for (int r = 2; r <= 14; ++r) {
        e.maxCount = std::max(e.maxCount, counts[r]);
        if (counts[r] >= 2) {
            e.pairs++;
            pairRank = r;
        }
        if (counts[r] >= 3)
            tripRank = r;
        if (counts[r] >= 4)
            quadRank = r;
    }
    bool flush = cards.size() == 5 && *std::max_element(suits, suits + 4) == 5;
    bool straight = false;
    if (cards.size() == 5) {
        for (int top = 6; top <= 14; ++top) {
            bool ok = true;
            for (int r = top - 4; r <= top; ++r)
                if (!counts[r])
                    ok = false;
            straight = straight || ok;
        }
        straight = straight || (counts[14] && counts[2] && counts[3] && counts[4] && counts[5]);
    }
    if (flush && e.maxCount == 5)
        e.type = FLUSH_FIVE;
    else if (flush && tripRank && e.pairs == 2)
        e.type = FLUSH_HOUSE;
    else if (e.maxCount == 5)
        e.type = FIVE_KIND;
    else if (straight && flush)
        e.type = STRAIGHT_FLUSH;
    else if (quadRank)
        e.type = QUADS;
    else if (tripRank && e.pairs == 2)
        e.type = FULL_HOUSE;
    else if (flush)
        e.type = FLUSH;
    else if (straight)
        e.type = STRAIGHT;
    else if (tripRank)
        e.type = TRIPS;
    else if (e.pairs >= 2)
        e.type = TWO_PAIR;
    else if (pairRank)
        e.type = PAIR;
    for (unsigned i = 0; i < cards.size(); ++i) {
        int r = cards[i].rank;
        bool scoring = e.type >= STRAIGHT;
        if (e.type == QUADS)
            scoring = r == quadRank;
        else if (e.type == HIGH)
            scoring = r == high;
        else if (e.type == PAIR)
            scoring = r == pairRank;
        else if (e.type == TRIPS)
            scoring = r == tripRank;
        else if (e.type == TWO_PAIR)
            scoring = counts[r] >= 2;
        if (scoring)
            e.mask |= 1u << i;
    }
    return e;
}
Run::Run() {
    start(1);
}
uint32_t Run::random(uint32_t limit) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return limit ? rng % limit : 0;
}
void Run::start(uint32_t newSeed) {
    rng = seed = newSeed ? newSeed : 1;
    phase = BLIND;
    ante = 1;
    blind = 0;
    boss = int(random(BOSSES));
    money = 4;
    hands = 4;
    discards = 3;
    drawPos = handCount = jokerCount = roundsWon = planetsUsed = 0;
    score = bestHand = 0;
    reroll = 5;
    sortMode = 0;
    lastPayout = 0;
    message.clear();
    for (int i = 0; i < 52; ++i) {
        deck[i] = {2 + i % 13, i / 13, 0};
        order[i] = i;
    }
    for (int i = 0; i < HAND_TYPES; ++i) {
        levels[i] = 1;
        plays[i] = roundPlays[i] = 0;
    }
    std::fill(selected, selected + MAX_HAND, false);
    std::fill(hand, hand + MAX_HAND, 0);
    for (auto &j : jokers)
        j = Joker();
    for (auto &o : offers)
        o = Offer();
    updateTarget();
}
void Run::updateTarget() {
    static const int64_t goals[] = {300, 800, 2000, 5000, 11000, 20000, 35000, 50000};
    target = goals[std::min(ante, 8) - 1];
    for (int i = 8; i < ante; ++i)
        target = std::min<int64_t>(SCORE_CAP / 8, target * 5 / 2);
    if (blind == 1)
        target = target * 3 / 2;
    else if (blind == 2)
        target *= 2;
    if (blind == 2 && boss == WALL)
        target *= 2;
}
int Run::handLimit() const {
    int size = 8;
    for (int i = 0; i < jokerCount; ++i)
        if (jokers[i].kind == JUGGLER)
            ++size;
    if (blind == 2 && boss == MANACLE)
        --size;
    return std::min(size, MAX_HAND);
}
int Run::selectedCount() const {
    int n = 0;
    for (int i = 0; i < handCount; ++i)
        n += selected[i] ? 1 : 0;
    return n;
}
std::vector<Card> Run::selectedCards() const {
    std::vector<Card> r;
    for (int i = 0; i < handCount; ++i)
        if (selected[i])
            r.push_back(deck[hand[i]]);
    return r;
}
bool Run::debuffed(const Card &c) const {
    static const int suitMap[] = {1, 2, 3, 0};
    return blind == 2 && boss < 4 && c.suit == suitMap[boss];
}
bool Run::toggle(int i) {
    if (phase != PLAY || i < 0 || i >= handCount)
        return false;
    if (!selected[i] && selectedCount() == 5) {
        message = "Choose at most 5 cards";
        return false;
    }
    selected[i] = !selected[i];
    return true;
}
void Run::reorderCard(int from, int to) {
    if (from < 0 || to < 0 || from >= handCount || to >= handCount)
        return;
    int c = hand[from];
    bool s = selected[from];
    while (from != to) {
        int n = from + (from < to ? 1 : -1);
        hand[from] = hand[n];
        selected[from] = selected[n];
        from = n;
    }
    hand[to] = c;
    selected[to] = s;
}
void Run::reorderJoker(int from, int to) {
    if (from < 0 || to < 0 || from >= jokerCount || to >= jokerCount)
        return;
    Joker j = jokers[from];
    while (from != to) {
        int n = from + (from < to ? 1 : -1);
        jokers[from] = jokers[n];
        from = n;
    }
    jokers[to] = j;
}
void Run::sortHand() {
    for (int i = 0; i < handCount; ++i)
        for (int j = i + 1; j < handCount; ++j) {
            const Card &a = deck[hand[i]];
            const Card &b = deck[hand[j]];
            int ka = sortMode ? a.suit * 20 + a.rank : a.rank * 4 + a.suit;
            int kb = sortMode ? b.suit * 20 + b.rank : b.rank * 4 + b.suit;
            if (kb > ka) {
                std::swap(hand[i], hand[j]);
                std::swap(selected[i], selected[j]);
            }
        }
}
void Run::fillHand() {
    while (handCount < handLimit() && drawPos < 52) {
        selected[handCount] = false;
        hand[handCount++] = order[drawPos++];
    }
    sortHand();
}
void Run::beginBlind() {
    if (phase != BLIND)
        return;
    phase = PLAY;
    score = 0;
    hands = 4;
    discards = (blind == 2 && boss == WATER) ? 0 : 3;
    drawPos = handCount = 0;
    std::fill(selected, selected + MAX_HAND, false);
    std::fill(roundPlays, roundPlays + HAND_TYPES, 0);
    for (int i = 0; i < 52; ++i)
        order[i] = i;
    for (int i = 51; i > 0; --i)
        std::swap(order[i], order[random(i + 1)]);
    updateTarget();
    fillHand();
    message = "Select up to 5 cards";
}
void Run::nextBlind() {
    ++blind;
    if (blind == 3) {
        blind = 0;
        ++ante;
        boss = int(random(BOSSES));
    }
    phase = BLIND;
    score = 0;
    handCount = 0;
    std::fill(selected, selected + MAX_HAND, false);
    updateTarget();
}
bool Run::skipBlind() {
    if (phase != BLIND || blind == 2)
        return false;
    money = std::min(money + 4, 999999);
    nextBlind();
    message = "Blind skipped: +$4";
    return true;
}
void Run::removeSelection() {
    int n = 0;
    for (int i = 0; i < handCount; ++i)
        if (!selected[i])
            hand[n++] = hand[i];
    handCount = n;
    std::fill(selected, selected + MAX_HAND, false);
}
bool Run::discard() {
    if (phase != PLAY || !selectedCount() || discards <= 0)
        return false;
    --discards;
    removeSelection();
    fillHand();
    if (!handCount)
        phase = LOST;
    message = "Cards discarded";
    return true;
}
bool Run::play(Score &result) {
    if (phase != PLAY || !selectedCount() || hands <= 0)
        return false;
    if (blind == 2 && boss == FIVE_CARDS && selectedCount() != 5) {
        message = "The Five requires exactly 5 cards";
        return false;
    }
    result = Score();
    result.before = score;
    result.cards = selectedCards();
    result.eval = evaluate(result.cards);
    int type = result.eval.type, lv = levels[type] - 1;
    int64_t chips = BASE_CHIPS[type] + int64_t(CHIP_UP[type]) * lv;
    int64_t mult = 100LL * (BASE_MULT[type] + MULT_UP[type] * lv);
    auto step = [&](std::string label, int card = -1, int joker = -1) {
        chips = std::min(chips, SCORE_CAP);
        mult = std::min(mult, SCORE_CAP);
        result.steps.push_back({label, chips, mult, card, joker});
    };
    step(handName(type));
    int suits[4] = {}, odd = 0, even = 0, aces = 0, fib = 0;
    for (unsigned i = 0; i < result.cards.size(); ++i) {
        const Card &c = result.cards[i];
        if (!(result.eval.mask & (1u << i)) || debuffed(c))
            continue;
        chips += cardChips(c);
        if (c.bonus == 1)
            chips += 30;
        if (c.bonus == 2)
            mult += 400;
        if (c.bonus == 3)
            mult = mult * 3 / 2;
        suits[c.suit]++;
        odd += (c.rank == 14 || (c.rank <= 9 && c.rank % 2 == 1));
        even += (c.rank <= 10 && c.rank % 2 == 0);
        aces += (c.rank == 14);
        fib += (c.rank == 14 || c.rank == 2 || c.rank == 3 || c.rank == 5 || c.rank == 8);
        step("+" + number(cardChips(c)) +
                 (c.bonus == 1   ? " +30 CHIPS"
                  : c.bonus == 2 ? " +4 MULT"
                  : c.bonus == 3 ? " X1.5 MULT"
                                 : " CHIPS"),
             int(i));
    }
    ++plays[type];
    ++roundPlays[type];
    --hands;
    bool straight = type == STRAIGHT || type == STRAIGHT_FLUSH;
    bool flush =
        type == FLUSH || type == STRAIGHT_FLUSH || type == FLUSH_HOUSE || type == FLUSH_FIVE;
    for (int i = 0; i < jokerCount; ++i) {
        Joker &j = jokers[i];
        int64_t oldC = chips, oldM = mult;
        switch (j.kind) {
        case BASIC:
            mult += 400;
            break;
        case GREEDY:
            mult += 300 * suits[3];
            break;
        case LUSTY:
            mult += 300 * suits[1];
            break;
        case WRATHFUL:
            mult += 300 * suits[0];
            break;
        case GLUTTON:
            mult += 300 * suits[2];
            break;
        case JOLLY:
            if (result.eval.maxCount >= 2)
                mult += 800;
            break;
        case ZANY:
            if (result.eval.maxCount >= 3)
                mult += 1200;
            break;
        case MAD:
            if (result.eval.pairs >= 2)
                mult += 1000;
            break;
        case CRAZY:
            if (straight)
                mult += 1200;
            break;
        case DROLL:
            if (flush)
                mult += 1000;
            break;
        case SLY:
            if (result.eval.maxCount >= 2)
                chips += 50;
            break;
        case RUNNER:
            if (straight)
                j.value = std::min(j.value + 15, 1000000);
            chips += j.value;
            break;
        case ODD_TODD:
            chips += 31 * odd;
            break;
        case EVEN_STEVEN:
            mult += 400 * even;
            break;
        case SCHOLAR:
            chips += 20 * aces;
            mult += 400 * aces;
            break;
        case FIBONACCI:
            mult += 800 * fib;
            break;
        case ACROBAT:
            if (hands == 0)
                mult *= 3;
            break;
        case CARD_SHARP:
            if (roundPlays[type] > 1)
                mult *= 3;
            break;
        case SUPERNOVA:
            mult += 100LL * plays[type];
            break;
        case FORTUNE:
            mult += 100LL * planetsUsed;
            break;
        case CONSTELLATION:
            mult = mult * (100 + 10LL * planetsUsed) / 100;
            break;
        case TRIO:
            if (result.eval.maxCount >= 3)
                mult *= 3;
            break;
        case FAMILY:
            if (result.eval.maxCount >= 4)
                mult *= 4;
            break;
        case BANNER:
            chips += 30 * discards;
            break;
        default:
            break;
        }
        if (chips != oldC || mult != oldM)
            step(jokerDef(j.kind).name, -1, i);
    }
    result.chips = chips;
    result.mult100 = mult;
    result.total = (mult > 0 && chips > SCORE_CAP * 100 / mult) ? SCORE_CAP : chips * mult / 100;
    score = std::min(SCORE_CAP, score + result.total);
    bestHand = std::max(bestHand, result.total);
    removeSelection();
    if (score >= target) {
        lastPayout = 3 + blind + hands + std::min(money / 5, 5);
        for (int i = 0; i < jokerCount; ++i)
            if (jokers[i].kind == GOLDEN)
                lastPayout += 4;
        money = std::min(money + lastPayout, 999999);
        result.moneyGained = lastPayout;
        ++roundsWon;
        if (ante == 8 && blind == 2)
            phase = WON;
        else {
            phase = SHOP;
            reroll = 5;
            rollShop();
        }
    } else if (hands == 0) {
        phase = LOST;
    } else {
        fillHand();
        if (!handCount)
            phase = LOST;
    }
    return true;
}
void Run::rollShop() {
    for (int s = 0; s < 2; ++s) {
        int j = 0;
        for (int tries = 0; tries < 80; ++tries) {
            j = int(random(JOKER_TYPES));
            bool used = false;
            for (int i = 0; i < jokerCount; ++i)
                used |= jokers[i].kind == j;
            if (s == 1)
                used |= offers[0].item == j;
            if (!used)
                break;
        }
        offers[s] = {0, j, jokerDef(j).cost, false};
    }
    int t = int(random(9));
    if (random(2) == 0) {
        t = 0;
        for (int i = 1; i < HAND_TYPES; ++i)
            if (plays[i] > plays[t])
                t = i;
    }
    offers[2] = {1, t, 3, false};
    offers[3] = {2, int(random(4)), 5, false};
}
bool Run::buy(int slot) {
    if (phase != SHOP || slot < 0 || slot >= 4)
        return false;
    Offer &o = offers[slot];
    if (o.sold) {
        message = "Already purchased";
        return false;
    }
    if (money < o.price) {
        message = "Not enough money";
        return false;
    }
    if (o.kind == 0) {
        if (jokerCount == MAX_JOKERS) {
            message = "Joker slots full - sell one first";
            return false;
        }
        jokers[jokerCount++] = {o.item, 0};
        message = std::string("Bought ") + jokerDef(o.item).name;
    }
    if (o.kind == 1) {
        levels[o.item] = std::min(levels[o.item] + 1, 999);
        planetsUsed = std::min(planetsUsed + 1, 10000);
        message = std::string(handName(o.item)) + " level " + number(levels[o.item]);
    }
    if (o.kind == 2) {
        int ids[52];
        for (int i = 0; i < 52; ++i)
            ids[i] = i;
        for (int i = 0; i < 3; ++i) {
            int k = i + int(random(52 - i));
            std::swap(ids[i], ids[k]);
            Card &c = deck[ids[i]];
            if (o.item == 0)
                c.rank = c.rank == 14 ? 2 : c.rank + 1;
            else
                c.bonus = o.item;
        }
        message = o.item == 0   ? "Strength: 3 cards rank up"
                  : o.item == 1 ? "3 cards gain +30 chips"
                  : o.item == 2 ? "3 cards gain +4 Mult"
                                : "3 cards gain X1.5 Mult";
    }
    money -= o.price;
    o.sold = true;
    return true;
}
bool Run::sell(int slot) {
    if (phase != SHOP || slot < 0 || slot >= jokerCount)
        return false;
    money = std::min(money + std::max(1, jokerDef(jokers[slot].kind).cost / 2), 999999);
    for (int i = slot; i < jokerCount - 1; ++i)
        jokers[i] = jokers[i + 1];
    --jokerCount;
    message = "Joker sold";
    return true;
}
bool Run::rerollShop() {
    if (phase != SHOP || money < reroll) {
        message = "Not enough money to reroll";
        return false;
    }
    money -= reroll;
    reroll = std::min(reroll + 1, 999999);
    rollShop();
    message = "New stock";
    return true;
}
void Run::leaveShop() {
    if (phase == SHOP)
        nextBlind();
}
void Run::endless() {
    if (phase == WON) {
        nextBlind();
        message = "Endless mode - keep climbing!";
    }
}

// Explicit little-endian serialization keeps desktop and ARM save files compatible.
static void put(std::vector<uint8_t> &b, uint64_t x, int n = 4) {
    while (n--) {
        b.push_back(uint8_t(x));
        x >>= 8;
    }
}
static uint32_t checksum(const std::vector<uint8_t> &b, size_t count) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < count; ++i) {
        h ^= b[i];
        h *= 16777619u;
    }
    return h;
}
bool Run::save(const std::string &filename) const {
    if (!valid())
        return false;
    std::vector<uint8_t> b;
    put(b, 0x42544C52);
    put(b, 1);
    const int fields[] = {int(phase), ante,        blind,     boss,       money,
                          hands,      discards,    drawPos,   handCount,  jokerCount,
                          reroll,     planetsUsed, roundsWon, lastPayout, sortMode};
    put(b, rng);
    put(b, seed);
    for (int v : fields)
        put(b, uint32_t(v));
    put(b, score, 8);
    put(b, bestHand, 8);
    put(b, target, 8);
    for (const Card &c : deck) {
        put(b, c.rank);
        put(b, c.suit);
        put(b, c.bonus);
    }
    for (int v : order)
        put(b, v);
    for (int v : hand)
        put(b, v);
    for (bool v : selected)
        put(b, v);
    for (const Joker &j : jokers) {
        put(b, j.kind);
        put(b, j.value);
    }
    for (const Offer &o : offers) {
        put(b, o.kind);
        put(b, o.item);
        put(b, o.price);
        put(b, o.sold);
    }
    for (int v : levels)
        put(b, v);
    for (int v : plays)
        put(b, v);
    for (int v : roundPlays)
        put(b, v);
    put(b, checksum(b, b.size()));
    std::string tmp = filename + ".tmp.tns";
    FILE *f = std::fopen(tmp.c_str(), "wb");
    if (!f)
        return false;
    bool ok = std::fwrite(b.data(), 1, b.size(), f) == b.size();
    if (std::fclose(f) != 0)
        ok = false;
    if (ok && std::rename(tmp.c_str(), filename.c_str()) == 0)
        return true;
    // Some calculator OS versions cannot rename over an existing destination.
    // Preserve the previous complete file until the new file is in place.
    if (ok) {
        std::string backup = filename + ".bak.tns";
        std::remove(backup.c_str());
        if (std::rename(filename.c_str(), backup.c_str()) == 0) {
            if (std::rename(tmp.c_str(), filename.c_str()) == 0)
                return true;
            std::rename(backup.c_str(), filename.c_str());
        }
    }
    std::remove(tmp.c_str());
    return false;
}
bool Run::load(const std::string &filename) {
    return loadFile(filename) || loadFile(filename + ".bak.tns");
}
bool Run::loadFile(const std::string &filename) {
    FILE *f = std::fopen(filename.c_str(), "rb");
    if (!f)
        return false;
    uint8_t raw[4096];
    size_t count = std::fread(raw, 1, sizeof raw, f);
    std::fclose(f);
    if (count < 100 || count == sizeof raw)
        return false;
    std::vector<uint8_t> b(raw, raw + count);
    size_t p = 0;
    bool ok = true;
    auto get = [&](int n = 4) -> uint64_t {
        if (p + size_t(n) > b.size()) {
            ok = false;
            return 0;
        }
        uint64_t v = 0;
        for (int i = 0; i < n; ++i)
            v |= uint64_t(b[p++]) << (8 * i);
        return v;
    };
    if (get() != 0x42544C52 || get() != 1)
        return false;
    Run r;
    r.rng = uint32_t(get());
    r.seed = uint32_t(get());
    r.phase = Phase(get());
    int *fields[] = {&r.ante,        &r.blind,     &r.boss,       &r.money,      &r.hands,
                     &r.discards,    &r.drawPos,   &r.handCount,  &r.jokerCount, &r.reroll,
                     &r.planetsUsed, &r.roundsWon, &r.lastPayout, &r.sortMode};
    for (int *v : fields)
        *v = int(get());
    r.score = int64_t(get(8));
    r.bestHand = int64_t(get(8));
    r.target = int64_t(get(8));
    for (Card &c : r.deck) {
        c.rank = int(get());
        c.suit = int(get());
        c.bonus = int(get());
    }
    for (int &v : r.order)
        v = int(get());
    for (int &v : r.hand)
        v = int(get());
    for (bool &v : r.selected)
        v = get() != 0;
    for (Joker &j : r.jokers) {
        j.kind = int(get());
        j.value = int(get());
    }
    for (Offer &o : r.offers) {
        o.kind = int(get());
        o.item = int(get());
        o.price = int(get());
        o.sold = get() != 0;
    }
    for (int &v : r.levels)
        v = int(get());
    for (int &v : r.plays)
        v = int(get());
    for (int &v : r.roundPlays)
        v = int(get());
    uint32_t hash = uint32_t(get());
    if (!ok || p != count || hash != checksum(b, count - 4) || !r.valid())
        return false;
    *this = r;
    return true;
}
bool Run::valid() const {
    if (phase < BLIND || phase > LOST || ante < 1 || ante > 100 || blind < 0 || blind > 2 ||
        boss < 0 || boss >= BOSSES || money < 0 || money > 999999 || hands < 0 || hands > 4 ||
        discards < 0 || discards > 3 || drawPos < 0 || drawPos > 52 || handCount < 0 ||
        handCount > MAX_HAND || jokerCount < 0 || jokerCount > MAX_JOKERS || reroll < 5 ||
        reroll > 999999 || planetsUsed < 0 || planetsUsed > 10000 || sortMode < 0 || sortMode > 1 ||
        roundsWon < 0 || roundsWon > 300 || lastPayout < 0 || lastPayout > 100 || score < 0 ||
        score > SCORE_CAP || bestHand < 0 || bestHand > SCORE_CAP || target < 300 ||
        target > SCORE_CAP || !rng || !seed)
        return false;
    bool seen[52] = {};
    for (int v : order) {
        if (v < 0 || v >= 52 || seen[v])
            return false;
        seen[v] = true;
    }
    std::fill(seen, seen + 52, false);
    for (int i = 0; i < MAX_HAND; ++i) {
        if (hand[i] < 0 || hand[i] >= 52)
            return false;
        if (i < handCount) {
            if (seen[hand[i]])
                return false;
            seen[hand[i]] = true;
        }
    }
    for (const Card &c : deck)
        if (c.rank < 2 || c.rank > 14 || c.suit < 0 || c.suit > 3 || c.bonus < 0 || c.bonus > 3)
            return false;
    for (const Joker &j : jokers)
        if (j.kind < 0 || j.kind >= JOKER_TYPES || j.value < 0 || j.value > 1000000)
            return false;
    for (const Offer &o : offers)
        if (o.kind < 0 || o.kind > 2 || o.item < 0 ||
            o.item >= (o.kind == 0   ? JOKER_TYPES
                       : o.kind == 1 ? HAND_TYPES
                                     : 4) ||
            o.price < 0 || o.price > 100)
            return false;
    for (int i = 0; i < HAND_TYPES; ++i)
        if (levels[i] < 1 || levels[i] > 999 || plays[i] < 0 || plays[i] > 100000 ||
            roundPlays[i] < 0 || roundPlays[i] > 4)
            return false;
    return selectedCount() <= 5;
}
} // namespace bt
