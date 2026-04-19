#include "hand_evaluator.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>

namespace ompeval {

// Hand value offsets for each category
// These ensure higher categories always beat lower ones
constexpr uint16_t HIGH_CARD_OFFSET = 0;
constexpr uint16_t PAIR_OFFSET = 1277;          // 1277 possible high card hands
constexpr uint16_t TWO_PAIR_OFFSET = 4137;      // + 2860 possible one pair hands
constexpr uint16_t THREE_KIND_OFFSET = 4995;    // + 858 possible two pair hands
constexpr uint16_t STRAIGHT_OFFSET = 5853;      // + 858 possible trips hands
constexpr uint16_t FLUSH_OFFSET = 5863;         // + 10 possible straights
constexpr uint16_t FULL_HOUSE_OFFSET = 7140;    // + 1277 possible flushes
constexpr uint16_t FOUR_KIND_OFFSET = 7296;     // + 156 possible full houses
constexpr uint16_t STRAIGHT_FLUSH_OFFSET = 7452;// + 156 possible quads

HandEvaluator& HandEvaluator::instance() {
    static HandEvaluator evaluator;
    return evaluator;
}

HandEvaluator::HandEvaluator() {
    initializeTables();
}

void HandEvaluator::initializeTables() {
    generateFlushTable();
    generateUnique5Table();
    generateRankTable();
    initialized_ = true;
}

void HandEvaluator::generateFlushTable() {
    // For each possible 5-card flush (13 choose 5 = 1287 combinations)
    // Store the hand value based on the ranks present
    
    flush_lookup_.fill(0);
    
    for (int bits = 0; bits < 8192; ++bits) {
        int popcount = __builtin_popcount(bits);
        if (popcount < 5) continue;
        
        // Check for straight flush
        uint16_t straightVal = computeStraightValue(bits, true);
        if (straightVal > 0) {
            flush_lookup_[bits] = straightVal;
            continue;
        }
        
        // Regular flush - rank by highest cards
        // Extract the 5 highest bits
        std::array<int, 5> ranks;
        int idx = 0;
        for (int r = 12; r >= 0 && idx < 5; --r) {
            if (bits & (1 << r)) {
                ranks[idx++] = r;
            }
        }
        
        // Calculate flush value (kicker based)
        uint16_t value = FLUSH_OFFSET;
        for (int i = 0; i < 5; ++i) {
            // Weight each card position
            int weight = 1;
            for (int j = i + 1; j < 5; ++j) weight *= 13;
            value += ranks[i] * weight / 13;  // Normalize
        }
        
        // Simpler: use the bit pattern directly as ranking within flushes
        // Higher bits = better flush
        value = FLUSH_OFFSET + (bits & 0x1FFF);
        
        flush_lookup_[bits] = value;
    }
    
    // Recalculate flush values properly
    uint16_t flushRank = 0;
    for (int a = 4; a < 13; ++a) {
        for (int b = 3; b < a; ++b) {
            for (int c = 2; c < b; ++c) {
                for (int d = 1; d < c; ++d) {
                    for (int e = 0; e < d; ++e) {
                        int bits = (1 << a) | (1 << b) | (1 << c) | (1 << d) | (1 << e);
                        
                        // Skip straights
                        bool isStraight = false;
                        for (int start = 0; start <= 9; ++start) {
                            int straightBits = 0x1F << start;
                            if ((bits & straightBits) == straightBits) {
                                isStraight = true;
                                break;
                            }
                        }
                        // Check wheel (A-2-3-4-5)
                        if ((bits & 0x100F) == 0x100F) {
                            isStraight = true;
                        }
                        
                        if (!isStraight) {
                            flush_lookup_[bits] = FLUSH_OFFSET + flushRank;
                            ++flushRank;
                        }
                    }
                }
            }
        }
    }
    
    // Now add straight flushes
    uint16_t sfRank = 0;
    for (int high = 4; high <= 12; ++high) {
        int bits = 0x1F << (high - 4);
        flush_lookup_[bits] = STRAIGHT_FLUSH_OFFSET + sfRank;
        ++sfRank;
    }
    // Wheel straight flush (A-2-3-4-5)
    flush_lookup_[0x100F] = STRAIGHT_FLUSH_OFFSET + sfRank;
}

void HandEvaluator::generateUnique5Table() {
    // For hands with 5 unique ranks and no flush
    // This covers straights and high card hands
    
    unique5_lookup_.fill(0);
    
    // Straights first (higher value than high cards)
    uint16_t straightRank = 0;
    for (int high = 4; high <= 12; ++high) {
        int bits = 0x1F << (high - 4);
        unique5_lookup_[bits] = STRAIGHT_OFFSET + straightRank;
        ++straightRank;
    }
    // Wheel (A-2-3-4-5)
    unique5_lookup_[0x100F] = STRAIGHT_OFFSET + straightRank;
    
    // High card hands (no straight, no pairs)
    uint16_t highCardRank = 0;
    for (int a = 4; a < 13; ++a) {
        for (int b = 3; b < a; ++b) {
            for (int c = 2; c < b; ++c) {
                for (int d = 1; d < c; ++d) {
                    for (int e = 0; e < d; ++e) {
                        int bits = (1 << a) | (1 << b) | (1 << c) | (1 << d) | (1 << e);
                        
                        // Skip if already a straight
                        if (unique5_lookup_[bits] != 0) continue;
                        
                        unique5_lookup_[bits] = HIGH_CARD_OFFSET + highCardRank;
                        ++highCardRank;
                    }
                }
            }
        }
    }
}

void HandEvaluator::generateRankTable() {
    // Generate lookup table for hands with duplicate ranks
    // This uses a hash based on rank counts
    
    // Calculate size needed: we need to handle all possible rank distributions
    // Maximum hash value estimation
    constexpr size_t TABLE_SIZE = 100000;  // Conservative estimate
    rank_lookup_.resize(TABLE_SIZE, 0);
    
    // Generate all possible rank count patterns for 5 cards
    // and assign hand values
    
    // One pair hands
    uint16_t pairRank = 0;
    for (int pair = 12; pair >= 0; --pair) {
        for (int k1 = 12; k1 >= 0; --k1) {
            if (k1 == pair) continue;
            for (int k2 = k1 - 1; k2 >= 0; --k2) {
                if (k2 == pair) continue;
                for (int k3 = k2 - 1; k3 >= 0; --k3) {
                    if (k3 == pair) continue;
                    
                    // Hash for this hand: pair rank * offset + kickers
                    uint32_t hash = pair * 7000 + k1 * 500 + k2 * 40 + k3;
                    if (hash < TABLE_SIZE) {
                        rank_lookup_[hash] = PAIR_OFFSET + pairRank;
                    }
                    ++pairRank;
                }
            }
        }
    }
    
    // Two pair hands
    uint16_t twoPairRank = 0;
    for (int high = 12; high >= 1; --high) {
        for (int low = high - 1; low >= 0; --low) {
            for (int kicker = 12; kicker >= 0; --kicker) {
                if (kicker == high || kicker == low) continue;
                
                uint32_t hash = 50000 + high * 200 + low * 15 + kicker;
                if (hash < TABLE_SIZE) {
                    rank_lookup_[hash] = TWO_PAIR_OFFSET + twoPairRank;
                }
                ++twoPairRank;
            }
        }
    }
    
    // Three of a kind
    uint16_t tripsRank = 0;
    for (int trips = 12; trips >= 0; --trips) {
        for (int k1 = 12; k1 >= 0; --k1) {
            if (k1 == trips) continue;
            for (int k2 = k1 - 1; k2 >= 0; --k2) {
                if (k2 == trips) continue;
                
                uint32_t hash = 60000 + trips * 200 + k1 * 15 + k2;
                if (hash < TABLE_SIZE) {
                    rank_lookup_[hash] = THREE_KIND_OFFSET + tripsRank;
                }
                ++tripsRank;
            }
        }
    }
    
    // Full house
    uint16_t fhRank = 0;
    for (int trips = 12; trips >= 0; --trips) {
        for (int pair = 12; pair >= 0; --pair) {
            if (pair == trips) continue;
            
            uint32_t hash = 70000 + trips * 15 + pair;
            if (hash < TABLE_SIZE) {
                rank_lookup_[hash] = FULL_HOUSE_OFFSET + fhRank;
            }
            ++fhRank;
        }
    }
    
    // Four of a kind
    uint16_t quadsRank = 0;
    for (int quads = 12; quads >= 0; --quads) {
        for (int kicker = 12; kicker >= 0; --kicker) {
            if (kicker == quads) continue;
            
            uint32_t hash = 80000 + quads * 15 + kicker;
            if (hash < TABLE_SIZE) {
                rank_lookup_[hash] = FOUR_KIND_OFFSET + quadsRank;
            }
            ++quadsRank;
        }
    }
}

uint16_t HandEvaluator::computeStraightValue(uint16_t rankBits, bool isFlush) const {
    // Check for straight (5 consecutive bits)
    for (int high = 12; high >= 4; --high) {
        int straightBits = 0x1F << (high - 4);
        if ((rankBits & straightBits) == straightBits) {
            if (isFlush) {
                return STRAIGHT_FLUSH_OFFSET + (high - 4);
            } else {
                return STRAIGHT_OFFSET + (high - 4);
            }
        }
    }
    
    // Check wheel (A-2-3-4-5)
    if ((rankBits & 0x100F) == 0x100F) {
        if (isFlush) {
            return STRAIGHT_FLUSH_OFFSET;  // Lowest straight flush
        } else {
            return STRAIGHT_OFFSET;  // Lowest straight
        }
    }
    
    return 0;  // Not a straight
}

EvalResult HandEvaluator::evaluate5(int c1, int c2, int c3, int c4, int c5) const {
    // Get ranks and suits
    int r1 = cardRank(c1), r2 = cardRank(c2), r3 = cardRank(c3);
    int r4 = cardRank(c4), r5 = cardRank(c5);
    int s1 = cardSuit(c1), s2 = cardSuit(c2), s3 = cardSuit(c3);
    int s4 = cardSuit(c4), s5 = cardSuit(c5);
    
    // Create rank bitmask
    uint16_t rankBits = (1 << r1) | (1 << r2) | (1 << r3) | (1 << r4) | (1 << r5);
    
    // Check for flush
    bool isFlush = (s1 == s2) && (s2 == s3) && (s3 == s4) && (s4 == s5);
    
    EvalResult result;
    
    if (isFlush) {
        result.value = flush_lookup_[rankBits];
        result.rank = getRankCategory(result.value);
        return result;
    }
    
    // Check if all ranks are unique (possible straight or high card)
    int uniqueRanks = __builtin_popcount(rankBits);
    if (uniqueRanks == 5) {
        result.value = unique5_lookup_[rankBits];
        result.rank = getRankCategory(result.value);
        return result;
    }
    
    // Count ranks for paired hands
    std::array<int, 13> rankCounts = {};
    rankCounts[r1]++;
    rankCounts[r2]++;
    rankCounts[r3]++;
    rankCounts[r4]++;
    rankCounts[r5]++;
    
    result.value = computeHandValue(rankCounts, false);
    result.rank = getRankCategory(result.value);
    return result;
}

uint16_t HandEvaluator::computeHandValue(const std::array<int, 13>& rankCounts, bool isFlush) const {
    // Analyze the rank distribution
    int quads = -1, trips = -1, highPair = -1, lowPair = -1;
    std::array<int, 5> kickers;
    int kickerCount = 0;
    
    for (int r = 12; r >= 0; --r) {
        switch (rankCounts[r]) {
            case 4:
                quads = r;
                break;
            case 3:
                trips = r;
                break;
            case 2:
                if (highPair < 0) highPair = r;
                else lowPair = r;
                break;
            case 1:
                if (kickerCount < 5) kickers[kickerCount++] = r;
                break;
        }
    }
    
    // Four of a kind
    if (quads >= 0) {
        uint32_t hash = 80000 + quads * 15 + kickers[0];
        return rank_lookup_[hash];
    }
    
    // Full house
    if (trips >= 0 && highPair >= 0) {
        uint32_t hash = 70000 + trips * 15 + highPair;
        return rank_lookup_[hash];
    }
    
    // Three of a kind
    if (trips >= 0) {
        uint32_t hash = 60000 + trips * 200 + kickers[0] * 15 + kickers[1];
        return rank_lookup_[hash];
    }
    
    // Two pair
    if (highPair >= 0 && lowPair >= 0) {
        uint32_t hash = 50000 + highPair * 200 + lowPair * 15 + kickers[0];
        return rank_lookup_[hash];
    }
    
    // One pair
    if (highPair >= 0) {
        uint32_t hash = highPair * 7000 + kickers[0] * 500 + kickers[1] * 40 + kickers[2];
        return rank_lookup_[hash];
    }
    
    // Should not reach here for paired hands
    return 0;
}

EvalResult HandEvaluator::evaluate6(int c1, int c2, int c3, int c4, int c5, int c6) const {
    // Evaluate all 6 choose 5 = 6 combinations and return best
    std::array<int, 6> cards = {c1, c2, c3, c4, c5, c6};
    EvalResult best = {0, HandRank::HIGH_CARD};
    
    for (int skip = 0; skip < 6; ++skip) {
        int idx = 0;
        std::array<int, 5> hand;
        for (int i = 0; i < 6; ++i) {
            if (i != skip) hand[idx++] = cards[i];
        }
        
        EvalResult eval = evaluate5(hand[0], hand[1], hand[2], hand[3], hand[4]);
        if (eval.value > best.value) {
            best = eval;
        }
    }
    
    return best;
}

EvalResult HandEvaluator::evaluate7(int c1, int c2, int c3, int c4, int c5, int c6, int c7) const {
    // Evaluate all 7 choose 5 = 21 combinations and return best
    std::array<int, 7> cards = {c1, c2, c3, c4, c5, c6, c7};
    EvalResult best = {0, HandRank::HIGH_CARD};
    
    for (int skip1 = 0; skip1 < 7; ++skip1) {
        for (int skip2 = skip1 + 1; skip2 < 7; ++skip2) {
            int idx = 0;
            std::array<int, 5> hand;
            for (int i = 0; i < 7; ++i) {
                if (i != skip1 && i != skip2) hand[idx++] = cards[i];
            }
            
            EvalResult eval = evaluate5(hand[0], hand[1], hand[2], hand[3], hand[4]);
            if (eval.value > best.value) {
                best = eval;
            }
        }
    }
    
    return best;
}

EvalResult HandEvaluator::evaluate(const std::vector<int>& cards) const {
    return evaluate(cards.data(), static_cast<int>(cards.size()));
}

EvalResult HandEvaluator::evaluate(const int* cards, int count) const {
    switch (count) {
        case 5:
            return evaluate5(cards[0], cards[1], cards[2], cards[3], cards[4]);
        case 6:
            return evaluate6(cards[0], cards[1], cards[2], cards[3], cards[4], cards[5]);
        case 7:
            return evaluate7(cards[0], cards[1], cards[2], cards[3], cards[4], cards[5], cards[6]);
        default:
            return {0, HandRank::HIGH_CARD};
    }
}

HandRank HandEvaluator::getRankCategory(uint16_t value) {
    if (value >= STRAIGHT_FLUSH_OFFSET) return HandRank::STRAIGHT_FLUSH;
    if (value >= FOUR_KIND_OFFSET) return HandRank::FOUR_OF_A_KIND;
    if (value >= FULL_HOUSE_OFFSET) return HandRank::FULL_HOUSE;
    if (value >= FLUSH_OFFSET) return HandRank::FLUSH;
    if (value >= STRAIGHT_OFFSET) return HandRank::STRAIGHT;
    if (value >= THREE_KIND_OFFSET) return HandRank::THREE_OF_A_KIND;
    if (value >= TWO_PAIR_OFFSET) return HandRank::TWO_PAIR;
    if (value >= PAIR_OFFSET) return HandRank::PAIR;
    return HandRank::HIGH_CARD;
}

const char* HandEvaluator::rankToString(HandRank rank) {
    switch (rank) {
        case HandRank::HIGH_CARD: return "High Card";
        case HandRank::PAIR: return "Pair";
        case HandRank::TWO_PAIR: return "Two Pair";
        case HandRank::THREE_OF_A_KIND: return "Three of a Kind";
        case HandRank::STRAIGHT: return "Straight";
        case HandRank::FLUSH: return "Flush";
        case HandRank::FULL_HOUSE: return "Full House";
        case HandRank::FOUR_OF_A_KIND: return "Four of a Kind";
        case HandRank::STRAIGHT_FLUSH: return "Straight Flush";
        default: return "Unknown";
    }
}

} // namespace ompeval
