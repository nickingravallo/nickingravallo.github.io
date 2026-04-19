#include "Card.h"

const char* rank_names[] = {
    "2", "3", "4", "5", "6", "7", "8",
    "9", "T", "J", "Q", "K", "A"
};

const char* suit_names[] = {
    "♣", "♦", "♥", "♠"
};

void print_card(Card card) {
    printf("%s%s\n", rank_names[card.rank], suit_names[card.suit]);
}

