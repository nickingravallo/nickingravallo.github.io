#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <memory>

namespace ompeval {

// Hand rankings (higher is better)
enum class HandRank : uint16_t {
    HIGH_CARD = 0,
    PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8
};

// Card representation: 0-51
// suit = card / 13 (0=clubs, 1=diamonds, 2=hearts, 3=spades)
// rank = card % 13 (0=2, 1=3, ..., 12=Ace)
constexpr int NUM_CARDS = 52;
constexpr int NUM_RANKS = 13;
constexpr int NUM_SUITS = 4;

// Result of hand evaluation
struct EvalResult {
    uint16_t value;      // Higher is better, encodes rank category + kickers
    HandRank rank;       // Category of hand
    
    bool operator<(const EvalResult& other) const { return value < other.value; }
    bool operator>(const EvalResult& other) const { return value > other.value; }
    bool operator==(const EvalResult& other) const { return value == other.value; }
    bool operator<=(const EvalResult& other) const { return value <= other.value; }
    bool operator>=(const EvalResult& other) const { return value >= other.value; }
};

/**
 * Fast poker hand evaluator using lookup tables.
 * Singleton pattern ensures tables are only generated once.
 * 
 * Uses the OMPEval algorithm which pre-computes hand rankings
 * for all possible 5, 6, and 7 card combinations using
 * efficient hash functions and lookup tables.
 */
class HandEvaluator {
public:
    // Get singleton instance (initializes lookup tables on first call)
    static HandEvaluator& instance();
    
    // Delete copy/move constructors
    HandEvaluator(const HandEvaluator&) = delete;
    HandEvaluator& operator=(const HandEvaluator&) = delete;
    
    // Evaluate a 5-card hand (cards are 0-51)
    EvalResult evaluate5(int c1, int c2, int c3, int c4, int c5) const;
    
    // Evaluate a 6-card hand (best 5 of 6)
    EvalResult evaluate6(int c1, int c2, int c3, int c4, int c5, int c6) const;
    
    // Evaluate a 7-card hand (best 5 of 7)
    EvalResult evaluate7(int c1, int c2, int c3, int c4, int c5, int c6, int c7) const;
    
    // Evaluate from array of cards
    EvalResult evaluate(const std::vector<int>& cards) const;
    EvalResult evaluate(const int* cards, int count) const;
    
    // Get hand rank category from evaluation value
    static HandRank getRankCategory(uint16_t value);
    
    // Get string representation of hand rank
    static const char* rankToString(HandRank rank);
    
    // Check if lookup tables are initialized
    bool isInitialized() const { return initialized_; }

private:
    HandEvaluator();
    ~HandEvaluator() = default;
    
    void initializeTables();
    
    // Internal evaluation using bit representation
    uint16_t evaluateRanks(uint16_t rankBits, int numCards) const;
    uint16_t evaluateFlush(uint16_t rankBits) const;
    
    // Lookup tables
    std::array<uint16_t, 8192> flush_lookup_;      // For flush hands (13-bit rank bitmask)
    std::array<uint16_t, 8192> unique5_lookup_;    // For 5 unique ranks (no pairs)
    std::vector<uint16_t> rank_lookup_;            // For hands with pairs/trips/quads
    
    // Hash offsets for different rank combinations
    std::array<uint32_t, 13> dp_offsets_;
    
    bool initialized_ = false;
    
    // Helper functions for table generation
    void generateFlushTable();
    void generateUnique5Table();
    void generateRankTable();
    
    uint16_t computeHandValue(const std::array<int, 13>& rankCounts, bool isFlush) const;
    uint16_t computeStraightValue(uint16_t rankBits, bool isFlush) const;
};

// Inline utility functions for card manipulation
inline int cardRank(int card) { return card % NUM_RANKS; }
inline int cardSuit(int card) { return card / NUM_RANKS; }
inline int makeCard(int rank, int suit) { return suit * NUM_RANKS + rank; }

// Rank and suit characters for display
inline char rankChar(int rank) {
    static const char chars[] = "23456789TJQKA";
    return chars[rank];
}

inline char suitChar(int suit) {
    static const char chars[] = "cdhs";
    return chars[suit];
}

} // namespace ompeval
