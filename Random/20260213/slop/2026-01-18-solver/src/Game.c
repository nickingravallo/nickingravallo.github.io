#include "Game.h"
#include "Deck.h"
#include "Card.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int players;
Hand** hands;
Board* board;

static int compare_hands(HandEvaluation* h1, HandEvaluation* h2);

void game_init(int _players) {
    int i;

    players = _players;
    hands = (Hand**) malloc(players * sizeof(Hand*));
    for (i = 0; i < players; i++) {
        hands[i] = (Hand*) malloc(sizeof(Hand));
    }

    deck_init();
    shuffle_deck();
    for (i = 0; i < players; i++) {
        deal_hand(i);
    }

    board = (Board*) malloc(sizeof(Board));
    board->turn.rank = -1;
    board->river.rank = -1;
    board->flop[0].rank = -1;
}

/*
Will not copy parameter Hand, will copy it directly
*/
void game_init_with_hands(Hand** _hands, int _players) {
    int i;

    players = _players;
    hands = _hands;

    deck_init();
    shuffle_deck();

    //remove all hole cards
    for (i = 0; i < _players; i++) {
        remove_card_from_deck(hands[i]->cards[0]);
        remove_card_from_deck(hands[i]->cards[1]);
    }

    board = (Board*) malloc(sizeof(Board));
    board->turn.rank = -1;
    board->river.rank = -1;
    board->flop[0].rank = -1;
}

void deal_flop() {
    board->flop[0] = draw_card();
    board->flop[1] = draw_card();
    board->flop[2] = draw_card();
}

void deal_turn() {
    board->turn = draw_card();
}

void deal_river() {
    board->river = draw_card();
}

//TO-DO: cleanup function, proof-of-concept for now
void monte_carlo_from_position(int players) {
    int i, winner, tie = 0;
    int wins[10] = {0}; //ten handed for future use, but we probably won't get there...

    Board* board_backup, *_board;
    Deck* deck_backup, *_deck;
    clock_t start, end;
    double runtime;
    double equity;
    
    _deck = deck;
    _board = board;

    board_backup = (Board*) malloc(sizeof(Board));
    memcpy(board_backup, board, sizeof(Board));
    deck_backup = (Deck*) malloc(sizeof(Deck));
    memcpy(deck_backup, deck, sizeof(Deck));

    start = clock();

    for (i = 0; i < 10000; i++) {
        shuffle_deck();
        deal_flop();
        deal_turn();
        deal_river();
        winner = determine_winner();
        if (winner >= 0 && winner < 10)
            wins[winner]++;
        else if (winner == -1)
            tie++;
        board = board_backup;
        deck = deck_backup;
    }

    end = clock();
    runtime = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("\nPreflop Equity Results:\n");
    for (i = 0; i < 2; i++) {
        equity = (double)wins[i] / 10000 * 100.0;
        printf("Player %d: %.2f%% (%d wins)\n", i, equity, wins[i]);
    }
    equity = (double)tie / 10000 * 100.0;
    printf("Tie: %.2f%% (%d wins)\n", equity, tie);

    printf("\nRuntime: %.2f seconds\n", runtime);

    free(board_backup);
    free(deck_backup);

    deck = _deck;
    board = _board;
}
/*
remember that drawing a card removes it from the deck
this is for the Monte Carlo simulation later on, in case
we to reshuffle from a certain point in the deck
*/
void deal_hand(int player) {
    hands[player]->cards[0] = draw_card();
    hands[player]->cards[1] = draw_card();
}

void print_hands() {
    int i;
    for (i = 0; i < players; i++) {
        print_hand(i);
    }
}

void print_hand(int player) {
    printf("Player %d: ", player);
    printf("%s%s %s%s\n", rank_names[hands[player]->cards[0].rank], suit_names[hands[player]->cards[0].suit], rank_names[hands[player]->cards[1].rank], suit_names[hands[player]->cards[1].suit]);
}

void print_board() {
    printf("Board: ");
    if (board->flop[0].rank != -1) {
        printf("%s%s %s%s %s%s", rank_names[board->flop[0].rank], suit_names[board->flop[0].suit], rank_names[board->flop[1].rank], suit_names[board->flop[1].suit], rank_names[board->flop[2].rank], suit_names[board->flop[2].suit]);
    }
    if (board->turn.rank != -1) {
        printf(" %s%s", rank_names[board->turn.rank], suit_names[board->turn.suit]);
    }
    if (board->river.rank != -1) {
        printf(" %s%s", rank_names[board->river.rank], suit_names[board->river.suit]);
    }
    printf("\n");
}

void print_hands_and_board() {
    print_hands();
    print_board();
}

void game_free() {
    int i;
    for (i = 0; i < players; i++) {
        free(hands[i]);
    }
    free(hands);
    free(board);
}

//make it quantify the cards on the board
static void count_ranks(Card* cards, int rank_counts[13], int suit_counts[4]) {
    int i;
    memset(rank_counts, 0, 13 * sizeof(int));
    memset(suit_counts, 0, 4 * sizeof(int));
    
    for (i = 0; i < 5; i++) {
        rank_counts[cards[i].rank]++;
        suit_counts[cards[i].suit]++;
    }
}

static int is_straight(int rank_counts[13]) {
    int i, consecutive = 0;

    //wheel
    if (rank_counts[ACE] && rank_counts[TWO] && rank_counts[THREE] && 
        rank_counts[FOUR] && rank_counts[FIVE]) {
        return 1;
    }
    
    //other straights
    for (i = 0; i < 13; i++) {
        if (rank_counts[i] > 0) {
            consecutive++;
            if (consecutive == 5) return 1;
        } else {
            consecutive = 0;
        }
    }
    return 0;
}

static int get_straight_high_card(int rank_counts[13]) {
    int i, consecutive = 0, high = -1;
    
    if (rank_counts[ACE] && rank_counts[TWO] && rank_counts[THREE] && 
        rank_counts[FOUR] && rank_counts[FIVE]) {
        return FIVE;
    }
    
    for (i = 0; i < 13; i++) {
        if (rank_counts[i] > 0) {
            consecutive++;
            high = i;
            if (consecutive == 5) return high;
        } else {
            consecutive = 0;
        }
    }
    return high;
}

/*
generated with AI, will look this over later
*/
static HandEvaluation evaluate_five_card_hand(Card* cards) {
    HandEvaluation eval = {0};
    int rank_counts[13];
    int suit_counts[4];
    int i, pair_count = 0, three_count = 0, four_count = 0;
    int pair_rank = -1, three_rank = -1, four_rank = -1;
    int flush_suit = -1;
    
    count_ranks(cards, rank_counts, suit_counts);
    
    //flush
    for (i = 0; i < 4; i++) {
        if (suit_counts[i] == 5) {
            flush_suit = i;
            break;
        }
    }
    
    //count pairs, trips, quds
    for (i = 0; i < 13; i++) {
        if (rank_counts[i] == 4) {
            four_count++;
            four_rank = i;
        } else if (rank_counts[i] == 3) {
            three_count++;
            three_rank = i;
        } else if (rank_counts[i] == 2) {
            pair_count++;
            if (pair_rank == -1) pair_rank = i;
        }
    }
    
    int is_straight_hand = is_straight(rank_counts);
    int straight_high = get_straight_high_card(rank_counts);
    
    if (is_straight_hand && flush_suit != -1) {
        if (straight_high == ACE) {
            eval.hand_type = ROYAL_FLUSH;
        } else {
            eval.hand_type = STRAIGHT_FLUSH;
            eval.primary_rank = straight_high;
        }
    } else if (four_count > 0) {
        eval.hand_type = FOUR_OF_A_KIND;
        eval.primary_rank = four_rank;
        //get kicker
        for (i = 0; i < 13; i++) {
            if (rank_counts[i] == 1) {
                eval.secondary_rank = i;
                break;
            }
        }
    } else if (three_count > 0 && pair_count > 0) {
        eval.hand_type = FULL_HOUSE;
        eval.primary_rank = three_rank;
        eval.secondary_rank = pair_rank;
    } else if (flush_suit != -1) {
        eval.hand_type = FLUSH;
        //get highest card in flush
        for (i = 12; i >= 0; i--) {
            if (rank_counts[i] > 0) {
                eval.primary_rank = i;
                break;
            }
        }
    } else if (is_straight_hand) {
        eval.hand_type = STRAIGHT;
        eval.primary_rank = straight_high;
    } else if (three_count > 0) {
        eval.hand_type = THREE_OF_A_KIND;
        eval.primary_rank = three_rank;
        //get kickers
        int kicker_idx = 0;
        for (i = 12; i >= 0; i--) {
            if (rank_counts[i] == 1 && kicker_idx < 2) {
                eval.kickers[kicker_idx++] = i;
            }
        }
    } else if (pair_count == 2) {
        eval.hand_type = TWO_PAIR;
        //find both pairs - higher pair is primary
        int pair1 = -1, pair2 = -1;
        for (i = 12; i >= 0; i--) {
            if (rank_counts[i] == 2) {
                if (pair1 == -1) pair1 = i;
                else {
                    pair2 = i;
                    break;
                }
            }
        }
        eval.primary_rank = pair1;
        eval.secondary_rank = pair2;
        //get kicker
        for (i = 0; i < 13; i++) {
            if (rank_counts[i] == 1) {
                eval.kickers[0] = i;
                break;
            }
        }
    } else if (pair_count == 1) {
        eval.hand_type = PAIR;
        eval.primary_rank = pair_rank;
        //get kickers
        int kicker_idx = 0;
        for (i = 12; i >= 0; i--) {
            if (rank_counts[i] == 1 && kicker_idx < 3) {
                eval.kickers[kicker_idx++] = i;
            }
        }
    } else {
        eval.hand_type = HIGH_CARD;
        //get all cards as kickers
        int kicker_idx = 0;
        for (i = 12; i >= 0; i--) {
            if (rank_counts[i] == 1 && kicker_idx < 5) {
                if (kicker_idx == 0) eval.primary_rank = i;
                else eval.kickers[kicker_idx - 1] = i;
                kicker_idx++;
            }
        }
    }
    
    return eval;
}

//get best 5-card hand from 7 cards (2 hole + 5 board)
static HandEvaluation get_best_hand(int player) {
    Card all_cards[7];
    Card best_5[5];
    HandEvaluation best_eval, eval;
    int i, j, k, idx;
    int first = 1;
    
    //combine hole cards and board
    all_cards[0] = hands[player]->cards[0];
    all_cards[1] = hands[player]->cards[1];
    all_cards[2] = board->flop[0];
    all_cards[3] = board->flop[1];
    all_cards[4] = board->flop[2];
    all_cards[5] = board->turn;
    all_cards[6] = board->river;
    
    //try all combinations of 5 cards from 7
    for (i = 0; i < 7; i++) {
        for (j = i + 1; j < 7; j++) {
            idx = 0;
            for (k = 0; k < 7; k++) {
                if (k != i && k != j) {
                    best_5[idx++] = all_cards[k];
                }
            }
            
            eval = evaluate_five_card_hand(best_5);
            
            //compare and keep best
            if (first || compare_hands(&eval, &best_eval) > 0) {
                best_eval = eval;
                first = 0;
            }
        }
    }
    
    return best_eval;
}

//compare two hand evaluations
static int compare_hands(HandEvaluation* h1, HandEvaluation* h2) {
    int i;
    
    //compare hand type
    if (h1->hand_type != h2->hand_type) {
        return h1->hand_type - h2->hand_type;
    }
    
    //compare primary rank
    if (h1->primary_rank != h2->primary_rank) {
        return h1->primary_rank - h2->primary_rank;
    }
    
    //compare secondary rank
    if (h1->secondary_rank != h2->secondary_rank) {
        return h1->secondary_rank - h2->secondary_rank;
    }
    
    //compare kickers
    for (i = 0; i < 3; i++) {
        if (h1->kickers[i] != h2->kickers[i]) {
            return h1->kickers[i] - h2->kickers[i];
        }
    }
    
    return 0; //tie
}

//find winner among all players
int determine_winner(void) {
    int i, cmp, winner = 0, tie = 0;
    HandEvaluation best_eval, current_eval;
    
    if (players == 0) 
        return -2; //return -2 for err
    
    best_eval = get_best_hand(0);
    
    for (i = 1; i < players; i++) {
        current_eval = get_best_hand(i);
        cmp = compare_hands(&current_eval, &best_eval);
        if (cmp > 0) {
            best_eval = current_eval;
            winner = i;
            tie = 0;
        } else if (cmp == 0) {
            tie = 1;
        }
    }
    
    if (tie)
        return -1; //return -1 for tie

    return winner;
}