#ifndef DECK_H
#define DECK_H

#include "Card.h"
#include <stdint.h>

#define DECK_COUNT 52

typedef struct Deck {
    Card** cards;
    uint8_t card_count;
    uint8_t top;
} Deck;

extern Deck* deck;

void deck_init(void);
void remove_card_from_deck(Card);
void deck_free(void);
void shuffle_deck(void);
void print_deck(void);
Card draw_card(void);

#endif /* DECK_H */

