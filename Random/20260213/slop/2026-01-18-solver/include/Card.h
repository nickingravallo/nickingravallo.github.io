#ifndef CARD_H
#define CARD_H

#include <stdio.h>
#include <time.h>

#define CARD_COUNT 13
#define SUIT_COUNT 4

typedef enum {
    TWO = 0,
    THREE = 1,
    FOUR = 2,
    FIVE = 3,
    SIX = 4,
    SEVEN = 5,
    EIGHT = 6,
    NINE = 7,
    TEN = 8,
    JACK = 9,
    QUEEN = 10,
    KING = 11,
    ACE = 12,
} Rank;

typedef enum {
    CLUBS = 0,
    DIAMONDS = 1,
    HEARTS = 2,
    SPADES = 3,
} Suit;

typedef struct Card {
    Rank rank;
    Suit suit;
} Card;

extern const char* rank_names[];
extern const char* suit_names[];

void print_card(Card card);

#endif /* CARD_H */

