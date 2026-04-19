#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <optional>

namespace core {

// Card constants
constexpr int NUM_CARDS = 52;
constexpr int NUM_RANKS = 13;
constexpr int NUM_SUITS = 4;

// Rank enumeration (0 = 2, 12 = Ace)
enum class Rank : uint8_t {
    TWO = 0, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT,
    NINE, TEN, JACK, QUEEN, KING, ACE
};

// Suit enumeration
enum class Suit : uint8_t {
    CLUBS = 0, DIAMONDS, HEARTS, SPADES
};

/**
 * Represents a single playing card.
 * Cards are stored as integers 0-51 internally.
 * Card = suit * 13 + rank
 */
class Card {
public:
    // Default constructor (invalid card)
    Card() : value_(-1) {}
    
    // Construct from integer (0-51)
    explicit Card(int value) : value_(value) {}
    
    // Construct from rank and suit
    Card(Rank rank, Suit suit);
    
    // Construct from string (e.g., "As", "Kh", "2c")
    static std::optional<Card> fromString(const std::string& str);
    
    // Accessors
    int value() const { return value_; }
    Rank rank() const { return static_cast<Rank>(value_ % NUM_RANKS); }
    Suit suit() const { return static_cast<Suit>(value_ / NUM_RANKS); }
    int rankIndex() const { return value_ % NUM_RANKS; }
    int suitIndex() const { return value_ / NUM_RANKS; }
    
    // Validation
    bool isValid() const { return value_ >= 0 && value_ < NUM_CARDS; }
    
    // String representation
    std::string toString() const;
    char rankChar() const;
    char suitChar() const;
    
    // Comparison
    bool operator==(const Card& other) const { return value_ == other.value_; }
    bool operator!=(const Card& other) const { return value_ != other.value_; }
    bool operator<(const Card& other) const { return value_ < other.value_; }
    
private:
    int value_;
};

// Rank/suit character conversion utilities
char rankToChar(Rank rank);
char suitToChar(Suit suit);
std::optional<Rank> charToRank(char c);
std::optional<Suit> charToSuit(char c);

// All rank and suit characters
constexpr std::array<char, NUM_RANKS> RANK_CHARS = {'2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'};
constexpr std::array<char, NUM_SUITS> SUIT_CHARS = {'c', 'd', 'h', 's'};

} // namespace core
