#include "hand.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace core {

Hand::Hand(const Card& c1, const Card& c2) : cards_{c1, c2} {
    normalize();
}

Hand::Hand(int card1Value, int card2Value) 
    : cards_{Card(card1Value), Card(card2Value)} {
    normalize();
}

void Hand::normalize() {
    // Higher rank first; if equal, higher suit first
    if (cards_[1].rankIndex() > cards_[0].rankIndex() ||
        (cards_[1].rankIndex() == cards_[0].rankIndex() && 
         cards_[1].suitIndex() > cards_[0].suitIndex())) {
        std::swap(cards_[0], cards_[1]);
    }
}

std::optional<Hand> Hand::fromString(const std::string& str) {
    if (str.length() != 4) return std::nullopt;
    
    auto c1 = Card::fromString(str.substr(0, 2));
    auto c2 = Card::fromString(str.substr(2, 2));
    
    if (!c1 || !c2) return std::nullopt;
    if (c1->value() == c2->value()) return std::nullopt;  // Same card twice
    
    return Hand(*c1, *c2);
}

bool Hand::isPair() const {
    return cards_[0].rankIndex() == cards_[1].rankIndex();
}

bool Hand::isSuited() const {
    return cards_[0].suitIndex() == cards_[1].suitIndex();
}

bool Hand::isConnector() const {
    return gapSize() == 0;
}

int Hand::gapSize() const {
    int diff = std::abs(cards_[0].rankIndex() - cards_[1].rankIndex());
    return diff - 1;  // 0 for connectors, 1 for one-gappers, etc.
}

std::string Hand::canonicalName() const {
    std::string name;
    name += rankToChar(cards_[0].rank());
    name += rankToChar(cards_[1].rank());
    
    if (!isPair()) {
        name += isSuited() ? 's' : 'o';
    }
    
    return name;
}

std::string Hand::toString() const {
    return cards_[0].toString() + cards_[1].toString();
}

bool Hand::isValid() const {
    return cards_[0].isValid() && cards_[1].isValid() &&
           cards_[0].value() != cards_[1].value();
}

bool Hand::contains(const Card& card) const {
    return cards_[0] == card || cards_[1] == card;
}

bool Hand::contains(int cardValue) const {
    return cards_[0].value() == cardValue || cards_[1].value() == cardValue;
}

bool Hand::operator<(const Hand& other) const {
    if (cards_[0].value() != other.cards_[0].value())
        return cards_[0].value() < other.cards_[0].value();
    return cards_[1].value() < other.cards_[1].value();
}

bool Hand::operator==(const Hand& other) const {
    return cards_[0] == other.cards_[0] && cards_[1] == other.cards_[1];
}

bool Hand::operator!=(const Hand& other) const {
    return !(*this == other);
}

// HandType implementation

HandType::HandType(Rank rank1, Rank rank2, bool suited) {
    if (static_cast<int>(rank1) >= static_cast<int>(rank2)) {
        highRank_ = rank1;
        lowRank_ = rank2;
    } else {
        highRank_ = rank2;
        lowRank_ = rank1;
    }
    suited_ = suited;
    
    // Pairs can't be suited in the usual sense
    if (highRank_ == lowRank_) {
        suited_ = false;
    }
}

std::optional<HandType> HandType::fromString(const std::string& str) {
    if (str.length() < 2 || str.length() > 3) return std::nullopt;
    
    auto rank1 = charToRank(str[0]);
    auto rank2 = charToRank(str[1]);
    
    if (!rank1 || !rank2) return std::nullopt;
    
    bool suited = false;
    if (str.length() == 3) {
        if (str[2] == 's' || str[2] == 'S') suited = true;
        else if (str[2] == 'o' || str[2] == 'O') suited = false;
        else return std::nullopt;
    }
    
    return HandType(*rank1, *rank2, suited);
}

std::vector<Hand> HandType::getHands() const {
    std::vector<Hand> hands;
    
    if (isPair()) {
        // 6 combinations for pairs
        for (int s1 = 0; s1 < NUM_SUITS; ++s1) {
            for (int s2 = s1 + 1; s2 < NUM_SUITS; ++s2) {
                Card c1(highRank_, static_cast<Suit>(s1));
                Card c2(lowRank_, static_cast<Suit>(s2));
                hands.emplace_back(c1, c2);
            }
        }
    } else if (suited_) {
        // 4 combinations for suited hands
        for (int s = 0; s < NUM_SUITS; ++s) {
            Card c1(highRank_, static_cast<Suit>(s));
            Card c2(lowRank_, static_cast<Suit>(s));
            hands.emplace_back(c1, c2);
        }
    } else {
        // 12 combinations for offsuit hands
        for (int s1 = 0; s1 < NUM_SUITS; ++s1) {
            for (int s2 = 0; s2 < NUM_SUITS; ++s2) {
                if (s1 == s2) continue;
                Card c1(highRank_, static_cast<Suit>(s1));
                Card c2(lowRank_, static_cast<Suit>(s2));
                hands.emplace_back(c1, c2);
            }
        }
    }
    
    return hands;
}

std::string HandType::toString() const {
    std::string name;
    name += rankToChar(highRank_);
    name += rankToChar(lowRank_);
    
    if (!isPair()) {
        name += suited_ ? 's' : 'o';
    }
    
    return name;
}

bool HandType::operator<(const HandType& other) const {
    // Sort by high rank, then low rank, then suited
    if (highRank_ != other.highRank_)
        return static_cast<int>(highRank_) < static_cast<int>(other.highRank_);
    if (lowRank_ != other.lowRank_)
        return static_cast<int>(lowRank_) < static_cast<int>(other.lowRank_);
    return suited_ < other.suited_;
}

bool HandType::operator==(const HandType& other) const {
    return highRank_ == other.highRank_ && 
           lowRank_ == other.lowRank_ && 
           suited_ == other.suited_;
}

std::pair<int, int> HandType::gridPosition() const {
    // In the grid, AA is at (0,0), 22 is at (12,12)
    // Rows go from A to 2 (top to bottom)
    // Cols go from A to 2 (left to right)
    // Suited hands are above the diagonal, offsuit below
    
    int highIdx = 12 - static_cast<int>(highRank_);  // A=0, 2=12
    int lowIdx = 12 - static_cast<int>(lowRank_);
    
    if (isPair()) {
        return {highIdx, highIdx};
    } else if (suited_) {
        // Suited: row = low rank, col = high rank
        return {lowIdx, highIdx};
    } else {
        // Offsuit: row = high rank, col = low rank
        return {highIdx, lowIdx};
    }
}

} // namespace core
