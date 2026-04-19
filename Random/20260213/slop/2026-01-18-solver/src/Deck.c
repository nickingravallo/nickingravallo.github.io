#include "Deck.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

Deck* deck;

void deck_init() {
    int suit, rank, card_index;

    deck = (Deck*) malloc(sizeof(Deck));
    deck->cards = (Card**) malloc(DECK_COUNT * sizeof(Card*));
    deck->card_count = DECK_COUNT;
    deck->top = 0;
    
    card_index = 0;
    for (suit = 0; suit < SUIT_COUNT; suit++) {
        for (rank = 0; rank < CARD_COUNT; rank++) {
            deck->cards[card_index] = (Card*) malloc(sizeof(Card));
            deck->cards[card_index]->rank = rank;
            deck->cards[card_index]->suit = suit;
            card_index++;
        }
    }
}

void remove_card_from_deck(Card card) {
    int i, found=0;

    for (i = 0; i < deck->card_count; i++) {
        if (found) {
            deck->cards[i-1] = deck->cards[i];
            continue;
        }
        if (card.rank == deck->cards[i]->rank && card.suit == deck->cards[i]->suit) {
            free(deck->cards[i]);
            deck->cards[i] = NULL;
            found = 1;
        }
    }

    deck->cards[deck->card_count-1] = NULL;
    deck->card_count--;
}

void deck_free() {
    int i;

    if (deck == NULL) {
        return;
    }

    if (deck->cards != NULL) {
        for (i = 0; i < deck->card_count; i++) {
            free(deck->cards[i]);
        }
        free(deck->cards);
    }

    free(deck);
    deck = NULL;
}


Card draw_card() {
    return *deck->cards[deck->top++];
}

void shuffle_deck() {
    int i, j;
    Card* temp;
    static int seeded = 0;

    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }

    // Fisher-Yates shuffle algorithm
    for (i = deck->card_count - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = temp;
    }
    
    deck->top = 0;
}

void print_deck() {
    int i;

    for (i = 0; i < deck->card_count; i++) {
        printf("%d: ", i);
        print_card(*deck->cards[i]);
    }
}

