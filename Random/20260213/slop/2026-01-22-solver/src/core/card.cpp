#include "card.hpp"
#include <cctype>
#include <algorithm>

namespace core {

Card::Card(Rank rank, Suit suit)
    : value_(static_cast<int>(suit) * NUM_RANKS + static_cast<int>(rank)) {}

std::optional<Card> Card::fromString(const std::string& str) {
    if (str.length() != 2) return std::nullopt;
    
    auto rank = charToRank(str[0]);
    auto suit = charToSuit(str[1]);
    
    if (!rank || !suit) return std::nullopt;
    
    return Card(*rank, *suit);
}

std::string Card::toString() const {
    if (!isValid()) return "??";
    return std::string(1, rankChar()) + suitChar();
}

char Card::rankChar() const {
    return rankToChar(rank());
}

char Card::suitChar() const {
    return suitToChar(suit());
}

char rankToChar(Rank rank) {
    return RANK_CHARS[static_cast<int>(rank)];
}

char suitToChar(Suit suit) {
    return SUIT_CHARS[static_cast<int>(suit)];
}

std::optional<Rank> charToRank(char c) {
    c = std::toupper(c);
    switch (c) {
        case '2': return Rank::TWO;
        case '3': return Rank::THREE;
        case '4': return Rank::FOUR;
        case '5': return Rank::FIVE;
        case '6': return Rank::SIX;
        case '7': return Rank::SEVEN;
        case '8': return Rank::EIGHT;
        case '9': return Rank::NINE;
        case 'T': return Rank::TEN;
        case 'J': return Rank::JACK;
        case 'Q': return Rank::QUEEN;
        case 'K': return Rank::KING;
        case 'A': return Rank::ACE;
        default: return std::nullopt;
    }
}

std::optional<Suit> charToSuit(char c) {
    c = std::tolower(c);
    switch (c) {
        case 'c': return Suit::CLUBS;
        case 'd': return Suit::DIAMONDS;
        case 'h': return Suit::HEARTS;
        case 's': return Suit::SPADES;
        default: return std::nullopt;
    }
}

} // namespace core
