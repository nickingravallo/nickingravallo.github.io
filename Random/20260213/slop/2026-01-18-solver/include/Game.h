#ifndef GAME_H
#define GAME_H

#include "Card.h"

#define PREFLOP 0
#define FLOP 1
#define TURN 2
#define RIVER 3

typedef enum {
    HIGH_CARD = 0,
    PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8,
    ROYAL_FLUSH = 9,
} HandType;

typedef struct Hand {
    Card cards[2];
    int stack;
} Hand;

typedef struct Board {
    Card flop[3];
    Card turn;
    Card river;
    int pot;
} Board;

typedef struct HandEvaluation {
    HandType hand_type;
    int primary_rank;
    int secondary_rank;
    int kickers[3];
} HandEvaluation;

extern Board* board;
extern Hand** hands;
extern int players;

void game_init(int);
void game_init_with_hands(Hand**, int);
void deal_hand(int);
void print_hands(void);
void print_hand(int);
void print_board(void);
void print_hands_and_board(void);
void monte_carlo_from_position(int);
void deal_flop(void);
void deal_turn(void);
void deal_river(void);
void game_free(void);
int determine_winner(void);

#endif /* GAME_H */

