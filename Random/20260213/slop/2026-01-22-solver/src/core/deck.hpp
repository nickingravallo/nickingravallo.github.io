#pragma once

#include "card.hpp"
#include <vector>
#include <array>
#include <random>
#include <bitset>

namespace core {

/**
 * Represents a deck of cards with support for dealing and tracking dead cards.
 * Uses a bitset for O(1) dead card checking.
 */
class Deck {
public:
    Deck();
    
    // Reset deck to full 52 cards
    void reset();
    
    // Mark cards as dead (removed from deck)
    void markDead(const Card& card);
    void markDead(const std::vector<Card>& cards);
    void markDead(int cardValue);
    
    // Check if card is available
    bool isAvailable(const Card& card) const;
    bool isAvailable(int cardValue) const;
    
    // Get remaining cards
    std::vector<Card> getRemainingCards() const;
    int remainingCount() const;
    
    // Deal random card from remaining
    Card dealRandom(std::mt19937& rng);
    
    // Get all cards (regardless of dead status)
    static std::array<Card, NUM_CARDS> allCards();
    
    // Board representation
    void setBoard(const std::vector<Card>& board);
    const std::vector<Card>& getBoard() const { return board_; }
    
private:
    std::bitset<NUM_CARDS> dead_;
    std::vector<Card> board_;
};

} // namespace core
