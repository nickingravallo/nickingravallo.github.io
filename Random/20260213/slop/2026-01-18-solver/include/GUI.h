#ifndef GUI_H
#define GUI_H

#include "MCCFR.h"
#include "RangeParser.h"

// Strategy data for GUI display
typedef struct {
    char category[16];
    double strategy[3];  // [check/call, bet/raise, fold]
    int board[5];
    int board_size;
    Street street;
    int player;  // 0 = OOP (BB), 1 = IP (SB)
} GUIStrategyData;

typedef struct {
    GUIStrategyData *data;
    int count;
    int capacity;
} GUIStrategySet;

// Game state for progressive gameplay
typedef struct {
    Street current_street;      // Current street (flop, turn, river)
    int current_player;         // 0 = OOP (BB), 1 = IP (SB)
    int action_history[10];     // Action sequence
    int num_actions;            // Number of actions taken
    int board[5];               // Board cards (0-51, -1 if not dealt)
    int board_size;             // Number of board cards dealt
    int selected_turn_card;     // Selected turn card (-1 if random)
    int selected_river_card;    // Selected river card (-1 if random)
    int waiting_for_action;     // 1 if waiting for user action
    int game_complete;          // 1 if game is over
} GameState;

// GUI functions
int gui_init(void);
void gui_cleanup(void);
void gui_add_strategy(const char *category, double strategy[3], int *board, int board_size, Street street);
void gui_run(void);
void gui_set_ranges(const char *sb_range_str, const char *bb_range_str);
void gui_set_game_state(GameState *state);
GameState* gui_get_game_state(void);
void gui_reset_game(void);
void gui_set_hand_ranks(void *hr);  // Set hand rank tables for strategy computation

#endif /* GUI_H */
