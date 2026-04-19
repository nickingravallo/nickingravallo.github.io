#include "deck.hpp"
#include <algorithm>

namespace core {

Deck::Deck() {
    reset();
}

void Deck::reset() {
    dead_.reset();
    board_.clear();
}

void Deck::markDead(const Card& card) {
    if (card.isValid()) {
        dead_.set(card.value());
    }
}

void Deck::markDead(const std::vector<Card>& cards) {
    for (const auto& card : cards) {
        markDead(card);
    }
}

void Deck::markDead(int cardValue) {
    if (cardValue >= 0 && cardValue < NUM_CARDS) {
        dead_.set(cardValue);
    }
}

bool Deck::isAvailable(const Card& card) const {
    return card.isValid() && !dead_.test(card.value());
}

bool Deck::isAvailable(int cardValue) const {
    return cardValue >= 0 && cardValue < NUM_CARDS && !dead_.test(cardValue);
}

std::vector<Card> Deck::getRemainingCards() const {
    std::vector<Card> remaining;
    remaining.reserve(NUM_CARDS - dead_.count());
    
    for (int i = 0; i < NUM_CARDS; ++i) {
        if (!dead_.test(i)) {
            remaining.emplace_back(i);
        }
    }
    
    return remaining;
}

int Deck::remainingCount() const {
    return NUM_CARDS - static_cast<int>(dead_.count());
}

Card Deck::dealRandom(std::mt19937& rng) {
    auto remaining = getRemainingCards();
    if (remaining.empty()) {
        return Card();  // Invalid card
    }
    
    std::uniform_int_distribution<size_t> dist(0, remaining.size() - 1);
    Card dealt = remaining[dist(rng)];
    markDead(dealt);
    return dealt;
}

std::array<Card, NUM_CARDS> Deck::allCards() {
    std::array<Card, NUM_CARDS> cards;
    for (int i = 0; i < NUM_CARDS; ++i) {
        cards[i] = Card(i);
    }
    return cards;
}

void Deck::setBoard(const std::vector<Card>& board) {
    board_ = board;
    markDead(board);
}

} // namespace core
