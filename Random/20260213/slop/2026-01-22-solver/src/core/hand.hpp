#pragma once

#include "card.hpp"
#include <array>
#include <string>
#include <vector>
#include <optional>

namespace core {

/**
 * Represents a poker hand (2 hole cards for Texas Hold'em).
 * Hands are normalized so that the higher card is always first.
 */
class Hand {
public:
    Hand() = default;
    Hand(const Card& c1, const Card& c2);
    Hand(int card1Value, int card2Value);
    
    // Parse from string (e.g., "AsKh", "AhKd")
    static std::optional<Hand> fromString(const std::string& str);
    
    // Accessors
    const Card& card1() const { return cards_[0]; }
    const Card& card2() const { return cards_[1]; }
    const std::array<Card, 2>& cards() const { return cards_; }
    
    // Properties
    bool isPair() const;
    bool isSuited() const;
    bool isConnector() const;  // Adjacent ranks
    int gapSize() const;       // Gap between ranks (0 = connector)
    
    // Get the canonical hand type (e.g., "AKs", "QQ", "T9o")
    std::string canonicalName() const;
    
    // Full string representation (e.g., "AsKh")
    std::string toString() const;
    
    // Validation
    bool isValid() const;
    
    // Check if hand contains a specific card
    bool contains(const Card& card) const;
    bool contains(int cardValue) const;
    
    // Comparison for sorting
    bool operator<(const Hand& other) const;
    bool operator==(const Hand& other) const;
    bool operator!=(const Hand& other) const;
    
private:
    std::array<Card, 2> cards_;
    
    void normalize();  // Ensure higher card first
};

/**
 * Represents a hand type (combo class) like "AKs", "QQ", "T9o".
 * Used for range notation.
 */
class HandType {
public:
    HandType() = default;
    HandType(Rank rank1, Rank rank2, bool suited);
    
    // Parse from string (e.g., "AKs", "QQ", "T9o")
    static std::optional<HandType> fromString(const std::string& str);
    
    // Accessors
    Rank highRank() const { return highRank_; }
    Rank lowRank() const { return lowRank_; }
    bool isSuited() const { return suited_; }
    bool isPair() const { return highRank_ == lowRank_; }
    
    // Get all specific hands of this type
    std::vector<Hand> getHands() const;
    
    // String representation
    std::string toString() const;
    
    // Comparison
    bool operator<(const HandType& other) const;
    bool operator==(const HandType& other) const;
    
    // Grid position (for 13x13 hand matrix display)
    // Returns (row, col) where AA is (0,0), 22 is (12,12)
    std::pair<int, int> gridPosition() const;
    
private:
    Rank highRank_ = Rank::ACE;
    Rank lowRank_ = Rank::ACE;
    bool suited_ = false;
};

} // namespace core
