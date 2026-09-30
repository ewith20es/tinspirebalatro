#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace bt {
constexpr int MAX_HAND = 10, MAX_JOKERS = 5;
constexpr int64_t SCORE_CAP = 9000000000000LL;
enum HandType {
    HIGH,
    PAIR,
    TWO_PAIR,
    TRIPS,
    STRAIGHT,
    FLUSH,
    FULL_HOUSE,
    QUADS,
    STRAIGHT_FLUSH,
    FIVE_KIND,
    FLUSH_HOUSE,
    FLUSH_FIVE,
    HAND_TYPES
};
enum Phase { BLIND, PLAY, SHOP, WON, LOST };
enum Boss { HEARTS, CLUBS, DIAMONDS, SPADES, FIVE_CARDS, WATER, MANACLE, WALL, BOSSES };
struct Card {
    int rank = 2, suit = 0, bonus = 0;
}; // 0 spades, 1 hearts, 2 clubs, 3 diamonds
struct Evaluation {
    HandType type = HIGH;
    unsigned mask = 0;
    int pairs = 0, maxCount = 0;
};
struct JokerDef {
    const char *name;
    const char *detail;
    int cost;
    int color;
};
enum JokerKind {
    BASIC,
    GREEDY,
    LUSTY,
    WRATHFUL,
    GLUTTON,
    JOLLY,
    ZANY,
    MAD,
    CRAZY,
    DROLL,
    SLY,
    RUNNER,
    ODD_TODD,
    EVEN_STEVEN,
    SCHOLAR,
    FIBONACCI,
    ACROBAT,
    CARD_SHARP,
    SUPERNOVA,
    GOLDEN,
    FORTUNE,
    CONSTELLATION,
    TRIO,
    FAMILY,
    JUGGLER,
    BANNER,
    JOKER_TYPES
};
struct Joker {
    int kind = 0, value = 0;
};
struct Offer {
    int kind = 0, item = 0, price = 0;
    bool sold = false;
}; // joker, planet, enhancement
struct ScoreStep {
    std::string label;
    int64_t chips = 0, mult100 = 100;
    int card = -1, joker = -1;
};
struct Score {
    Evaluation eval;
    std::vector<Card> cards;
    std::vector<ScoreStep> steps;
    int64_t chips = 0, mult100 = 100, total = 0, before = 0;
    int moneyGained = 0;
};
const char *handName(int type);
const char *planetName(int type);
const char *bossName(int boss);
const char *bossRule(int boss);
const JokerDef &jokerDef(int kind);
int cardChips(const Card &card);
Evaluation evaluate(const std::vector<Card> &cards);
std::string number(int64_t n);

class Run {
  public:
    uint32_t rng = 1, seed = 1;
    Phase phase = BLIND;
    int ante = 1, blind = 0, boss = 0, money = 4, hands = 4, discards = 3;
    int drawPos = 0, handCount = 0, jokerCount = 0, reroll = 5, planetsUsed = 0;
    int roundsWon = 0, lastPayout = 0, sortMode = 0;
    int64_t score = 0, bestHand = 0, target = 300;
    Card deck[52];
    int order[52] = {}, hand[MAX_HAND] = {};
    bool selected[MAX_HAND] = {};
    Joker jokers[MAX_JOKERS];
    Offer offers[4];
    int levels[HAND_TYPES] = {}, plays[HAND_TYPES] = {}, roundPlays[HAND_TYPES] = {};
    std::string message;

    Run();
    void start(uint32_t newSeed);
    uint32_t random(uint32_t limit);
    int handLimit() const;
    int selectedCount() const;
    std::vector<Card> selectedCards() const;
    bool debuffed(const Card &card) const;
    bool toggle(int index);
    void sortHand();
    void reorderCard(int from, int to);
    void reorderJoker(int from, int to);
    void beginBlind();
    bool skipBlind();
    bool discard();
    bool play(Score &result);
    bool buy(int slot);
    bool sell(int slot);
    bool rerollShop();
    void leaveShop();
    void endless();
    void fillHand();
    void rollShop();
    void updateTarget();
    bool save(const std::string &filename) const;
    bool load(const std::string &filename);
    bool valid() const;

  private:
    bool loadFile(const std::string &filename);
    void removeSelection();
    void nextBlind();
};
} // namespace bt
