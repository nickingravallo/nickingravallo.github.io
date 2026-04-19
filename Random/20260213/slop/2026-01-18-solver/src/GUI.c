/*
 * GUI.c - Graphical User Interface for TurboFire GTO Solver
 * 
 * Displays strategy grids with color-coded actions:
 * - Blue: Check/Call
 * - Green: Bet/Raise  
 * - Red: Fold
 */

#ifdef USE_GUI
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../include/GUI.h"
#include "../include/MCCFR.h"
#include "../include/RangeParser.h"
#include "../include/HandRanks.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define GRID_SIZE 13
#define CELL_SIZE 40
#define GRID_X_OFFSET 100
#define GRID_Y_OFFSET 150
#define HEADER_HEIGHT 100
#define FOOTER_HEIGHT 50

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static TTF_Font *font = NULL;
static TTF_Font *small_font = NULL;

static GUIStrategySet strategies[3];  // One for each street (flop, turn, river)
static Street current_street = STREET_FLOP;
static int hover_row = -1, hover_col = -1;
static char hover_text[256] = "";
static char sb_range_str[512] = "";
static char bb_range_str[512] = "";
static int board_display[3][5];  // Board cards for each street
static int board_size[3];       // Board size for each street

// Progressive game state
static GameState game_state;
static HandRankTables *hand_ranks = NULL;
static HandRange *sb_range = NULL;
static HandRange *bb_range = NULL;
static MCCFRSolver *current_solver = NULL;
static int solver_needs_update = 1;

// Color definitions
static SDL_Color color_bg = {20, 20, 30, 255};
static SDL_Color color_grid = {60, 60, 80, 255};
static SDL_Color color_text = {255, 255, 255, 255};
static SDL_Color color_check = {100, 255, 100, 255};    // Green - Check/Call
static SDL_Color color_bet = {255, 100, 100, 255};       // Red - Bet/Raise
static SDL_Color color_fold = {100, 150, 255, 255};      // Blue - Fold
static SDL_Color color_hover = {255, 255, 200, 255};

static const char *RANKS = "23456789TJQKA";

int gui_init(void) {
#ifdef USE_GUI
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }
    
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }
    
    window = SDL_CreateWindow("TurboFire GTO Solver",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    
    // Try to load fonts (fallback to default if not found)
    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16);
    if (!font) {
        font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", 16);
    }
    if (!font) {
        font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 16);
    }
    
    small_font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
    if (!small_font) {
        small_font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 12);
    }
    if (!small_font) {
        small_font = font;  // Fallback to main font
    }
    
    // Initialize strategy sets
    for (int i = 0; i < 3; i++) {
        strategies[i].capacity = 200;
        strategies[i].count = 0;
        strategies[i].data = calloc(strategies[i].capacity, sizeof(GUIStrategyData));
        board_size[i] = 0;
        for (int j = 0; j < 5; j++) {
            board_display[i][j] = -1;
        }
    }
    
    // Initialize game state
    memset(&game_state, 0, sizeof(GameState));
    game_state.current_street = STREET_FLOP;
    game_state.current_player = 0;  // Start with OOP (BB)
    game_state.waiting_for_action = 1;
    game_state.selected_turn_card = -1;
    game_state.selected_river_card = -1;
    for (int i = 0; i < 5; i++) {
        game_state.board[i] = -1;
    }
    
    return 1;
#else
    // Stub implementation when GUI is disabled
    return 0;
#endif
}

void gui_cleanup(void) {
#ifdef USE_GUI
    for (int i = 0; i < 3; i++) {
        if (strategies[i].data) {
            free(strategies[i].data);
        }
    }
    
    if (font) TTF_CloseFont(font);
    if (small_font && small_font != font) TTF_CloseFont(small_font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
#endif
}

void gui_set_ranges(const char *sb_range_str_in, const char *bb_range_str_in) {
#ifdef USE_GUI
    strncpy(sb_range_str, sb_range_str_in ? sb_range_str_in : "", sizeof(sb_range_str) - 1);
    strncpy(bb_range_str, bb_range_str_in ? bb_range_str_in : "", sizeof(bb_range_str) - 1);
    sb_range_str[sizeof(sb_range_str) - 1] = '\0';
    bb_range_str[sizeof(bb_range_str) - 1] = '\0';
    
    // Parse ranges and store them
    if (sb_range) free_range(sb_range);
    if (bb_range) free_range(bb_range);
    sb_range = parse_range(sb_range_str);
    bb_range = parse_range(bb_range_str);
    
    // Verify ranges were parsed correctly
    if (!sb_range || !sb_range->hands || sb_range->count == 0) {
        fprintf(stderr, "Warning: Failed to parse SB range or range is empty\n");
    } else {
        fprintf(stderr, "SB range parsed: %d hands\n", sb_range->count);
    }
    if (!bb_range || !bb_range->hands || bb_range->count == 0) {
        fprintf(stderr, "Warning: Failed to parse BB range or range is empty\n");
    } else {
        fprintf(stderr, "BB range parsed: %d hands\n", bb_range->count);
    }
    
    solver_needs_update = 1;
#else
    // Stub - no-op when GUI disabled
    (void)sb_range_str_in;
    (void)bb_range_str_in;
#endif
}

void gui_set_game_state(GameState *state) {
#ifdef USE_GUI
    if (state) {
        memcpy(&game_state, state, sizeof(GameState));
        current_street = game_state.current_street;
    }
#else
    (void)state;
#endif
}

GameState* gui_get_game_state(void) {
#ifdef USE_GUI
    return &game_state;
#else
    return NULL;
#endif
}

void gui_reset_game(void) {
#ifdef USE_GUI
    memset(&game_state, 0, sizeof(GameState));
    game_state.current_street = STREET_FLOP;
    game_state.current_player = 0;  // Start with OOP (BB)
    game_state.waiting_for_action = 1;
    game_state.selected_turn_card = -1;
    game_state.selected_river_card = -1;
    for (int i = 0; i < 5; i++) {
        game_state.board[i] = -1;
    }
    game_state.board_size = 0;
    current_street = STREET_FLOP;
    solver_needs_update = 1;
#endif
}

void gui_set_hand_ranks(void *hr) {
#ifdef USE_GUI
    hand_ranks = (HandRankTables *)hr;
    solver_needs_update = 1;
#else
    (void)hr;
#endif
}

void gui_add_strategy(const char *category, double strategy[3], int *board, int board_size_in, Street street) {
#ifdef USE_GUI
    if (street < 0 || street > 2) return;
    
    GUIStrategySet *set = &strategies[street];
    if (set->count >= set->capacity) {
        set->capacity *= 2;
        set->data = realloc(set->data, set->capacity * sizeof(GUIStrategyData));
    }
    
    GUIStrategyData *data = &set->data[set->count++];
    strncpy(data->category, category, 15);
    data->category[15] = '\0';
    memcpy(data->strategy, strategy, sizeof(double) * 3);
    data->street = street;
    data->board_size = board_size_in;
    data->player = game_state.current_player;  // Track which player this strategy is for
    for (int i = 0; i < 5; i++) {
        data->board[i] = (i < board_size_in) ? board[i] : -1;
    }
    
    // Update board display for this street
    if (board_size_in > 0) {
        board_size[street] = board_size_in;
        for (int i = 0; i < board_size_in && i < 5; i++) {
            board_display[street][i] = board[i];
        }
    }
#else
    // Stub - no-op when GUI disabled
    (void)category;
    (void)strategy;
    (void)board;
    (void)board_size_in;
    (void)street;
#endif
}

static void card_str(int card, char *out) {
    if (card < 0 || card >= 52) {
        out[0] = '\0';
        return;
    }
    out[0] = RANKS[card >> 2];
    const char *suits = "cdhs";
    out[1] = suits[card & 3];
    out[2] = '\0';
}

#ifdef USE_GUI
static void render_text(const char *text, int x, int y, TTF_Font *f, SDL_Color color) {
    if (!f) return;
    
    SDL_Surface *surface = TTF_RenderText_Solid(f, text, color);
    if (!surface) return;
    
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}
#endif

static void get_hand_coords(const char *category, int *row, int *col, int *is_suited, int *is_pair) {
    *row = *col = -1;
    *is_suited = *is_pair = 0;
    
    int len = strlen(category);
    if (len < 2) return;
    
    int r1 = -1, r2 = -1;
    for (int i = 0; i < 13; i++) {
        if (RANKS[i] == category[0]) r1 = i;
        if (RANKS[i] == category[1]) r2 = i;
    }
    
    if (r1 < 0 || r2 < 0) return;
    
    if (r1 == r2) {
        *is_pair = 1;
        *row = *col = r1;
    } else {
        int high = (r1 > r2) ? r1 : r2;
        int low = (r1 < r2) ? r1 : r2;
        
        if (len > 2 && category[2] == 's') {
            *is_suited = 1;
            *row = low;
            *col = high;
        } else {
            *row = high;
            *col = low;
        }
    }
}

#ifdef USE_GUI
static void render_grid(void) {
    GUIStrategySet *set = &strategies[game_state.current_street];
    
    // Draw grid background
    SDL_Rect grid_rect = {GRID_X_OFFSET - 5, GRID_Y_OFFSET - 5,
                          GRID_SIZE * CELL_SIZE + 10, GRID_SIZE * CELL_SIZE + 10};
    SDL_SetRenderDrawColor(renderer, color_grid.r, color_grid.g, color_grid.b, 255);
    SDL_RenderFillRect(renderer, &grid_rect);
    
    // Draw rank labels (reversed: A at top, 2 at bottom)
    for (int i = 0; i < 13; i++) {
        char label[2] = {RANKS[12 - i], '\0'};  // Reverse: A=12, 2=0
        render_text(label, GRID_X_OFFSET - 20, GRID_Y_OFFSET + i * CELL_SIZE + CELL_SIZE/2 - 8, font, color_text);
        render_text(label, GRID_X_OFFSET + i * CELL_SIZE + CELL_SIZE/2 - 4, GRID_Y_OFFSET - 25, font, color_text);
    }
    
    // Draw "s" and "o" labels
    render_text("s", GRID_X_OFFSET - 40, GRID_Y_OFFSET + 2, small_font, color_text);
    render_text("o", GRID_X_OFFSET - 40, GRID_Y_OFFSET + GRID_SIZE * CELL_SIZE - 12, small_font, color_text);
    
    // Draw cells (reversed: AA at top-left, 22 at bottom-right)
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            int x = GRID_X_OFFSET + col * CELL_SIZE;
            int y = GRID_Y_OFFSET + row * CELL_SIZE;
            
            // Convert display coordinates to rank indices (reversed)
            int display_row = 12 - row;  // Row 0 = A, Row 12 = 2
            int display_col = 12 - col;  // Col 0 = A, Col 12 = 2
            
            // Determine cell type
            int is_pair = (display_row == display_col);
            int is_suited = (display_row < display_col);  // Upper triangle
            int is_offsuit = (display_row > display_col); // Lower triangle
            
            // Find matching strategy data
            double cell_strategy[3] = {0.33, 0.33, 0.34};
            int has_data = 0;
            char cell_category[16] = "";
            
            for (int i = 0; i < set->count; i++) {
                // Only show strategy for the current player
                if (set->data[i].player != game_state.current_player) continue;
                
                int s_row, s_col, s_suited, s_pair;
                get_hand_coords(set->data[i].category, &s_row, &s_col, &s_suited, &s_pair);
                
                // Match against reversed coordinates
                if (s_row == display_row && s_col == display_col) {
                    if ((is_pair && s_pair) || (is_suited && s_suited) || (is_offsuit && !s_suited && !s_pair)) {
                        memcpy(cell_strategy, set->data[i].strategy, sizeof(cell_strategy));
                        strncpy(cell_category, set->data[i].category, 15);
                        has_data = 1;
                        break;
                    }
                }
            }
            
            // Draw cell with split colors based on strategy percentages
            SDL_Rect cell_rect = {x, y, CELL_SIZE - 1, CELL_SIZE - 1};
            
            if (has_data) {
                // Draw split rectangles based on strategy percentages
                // Strategy: [0] = check/call (green), [1] = bet/raise (red), [2] = fold (blue)
                
                // Calculate pixel widths for each action
                int cell_width = CELL_SIZE - 1;
                int check_width = (int)(cell_width * cell_strategy[0]);
                int bet_width = (int)(cell_width * cell_strategy[1]);
                int fold_width = cell_width - check_width - bet_width;  // Remaining to avoid rounding errors
                
                // Draw check/call (green) - left portion
                if (check_width > 0) {
                    SDL_Rect check_rect = {x, y, check_width, CELL_SIZE - 1};
                    SDL_SetRenderDrawColor(renderer, color_check.r, color_check.g, color_check.b, 255);
                    SDL_RenderFillRect(renderer, &check_rect);
                }
                
                // Draw bet/raise (red) - middle portion
                if (bet_width > 0) {
                    SDL_Rect bet_rect = {x + check_width, y, bet_width, CELL_SIZE - 1};
                    SDL_SetRenderDrawColor(renderer, color_bet.r, color_bet.g, color_bet.b, 255);
                    SDL_RenderFillRect(renderer, &bet_rect);
                }
                
                // Draw fold (blue) - right portion
                if (fold_width > 0) {
                    SDL_Rect fold_rect = {x + check_width + bet_width, y, fold_width, CELL_SIZE - 1};
                    SDL_SetRenderDrawColor(renderer, color_fold.r, color_fold.g, color_fold.b, 255);
                    SDL_RenderFillRect(renderer, &fold_rect);
                }
                
                // Highlight hover with border
                if (hover_row >= 0 && hover_col >= 0) {
                    int hover_display_row = 12 - hover_row;
                    int hover_display_col = 12 - hover_col;
                    if (hover_display_row == display_row && hover_display_col == display_col) {
                        SDL_SetRenderDrawColor(renderer, color_hover.r, color_hover.g, color_hover.b, 255);
                        SDL_RenderDrawRect(renderer, &cell_rect);
                    }
                }
            } else {
                // No data - draw gray cell
                SDL_Color cell_color = (SDL_Color){40, 40, 50, 255};
                SDL_SetRenderDrawColor(renderer, cell_color.r, cell_color.g, cell_color.b, 255);
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
    }
}
#endif

// Generate random board card that doesn't conflict with existing cards
static int generate_random_card(int *used_cards, int num_used) {
    int deck[52];
    int deck_size = 0;
    
    // Build deck excluding used cards
    for (int c = 0; c < 52; c++) {
        int is_used = 0;
        for (int i = 0; i < num_used; i++) {
            if (c == used_cards[i]) {
                is_used = 1;
                break;
            }
        }
        if (!is_used) {
            deck[deck_size++] = c;
        }
    }
    
    if (deck_size == 0) return -1;
    return deck[rand() % deck_size];
}

// Check if cards overlap
static int cards_overlap(int c0, int c1, int c2, int c3, int *board, int board_size) {
    int cards[9] = {c0, c1, c2, c3};
    for (int i = 0; i < board_size && i < 5; i++) {
        cards[4 + i] = board[i];
    }
    
    int total_cards = 4 + board_size;
    for (int i = 0; i < total_cards; i++) {
        for (int j = i + 1; j < total_cards; j++) {
            if (cards[i] == cards[j] && cards[i] >= 0) {
                return 1;
            }
        }
    }
    return 0;
}

// Compute strategies for current game state
static void compute_strategies_for_current_state(void) {
    if (!sb_range || !bb_range || !hand_ranks) return;
    if (game_state.board_size < 3) return;
    
    // Clear existing strategies for current street
    strategies[game_state.current_street].count = 0;
    
    // Get current player's range
    // Player 0 = OOP (BB), Player 1 = IP (SB)
    HandRange *current_range = (game_state.current_player == 0) ? bb_range : sb_range;
    HandRange *opponent_range = (game_state.current_player == 0) ? sb_range : bb_range;
    
    if (!current_range || !current_range->hands || current_range->count == 0) return;
    if (!opponent_range || !opponent_range->hands || opponent_range->count == 0) return;
    
    // Debug: Verify which range is being used
    const char *player_name = (game_state.current_player == 0) ? "OOP (BB)" : "IP (SB)";
    fprintf(stderr, "Computing strategies for %s: Using range with %d hands\n", 
            player_name, current_range->count);
    
    // Compute strategy for each individual hand (accounts for all suit combinations)
    // Limit to reasonable number to avoid performance issues
    int max_hands_to_compute = current_range->count;
    if (max_hands_to_compute > 200) {
        max_hands_to_compute = 200;  // Limit to 200 hands for performance
        fprintf(stderr, "Limiting to first 200 hands for performance\n");
    }
    
    for (int hand_idx = 0; hand_idx < max_hands_to_compute; hand_idx++) {
        int player_c0 = current_range->hands[hand_idx][0];
        int player_c1 = current_range->hands[hand_idx][1];
        const char *target_cat = hand_category(player_c0, player_c1);
        
        if (player_c0 < 0 || player_c1 < 0) continue;
        
        fprintf(stderr, "Computing strategy for hand %d: %s\n", hand_idx, target_cat);
        
        // Use multiple opponent hands and average strategies for better accuracy
        double strategy_sum[3] = {0.0, 0.0, 0.0};
        int num_opponent_samples = 0;
        int max_samples = 5;  // Sample up to 5 opponent hands
        
        for (int opp_idx = 0; opp_idx < opponent_range->count && num_opponent_samples < max_samples; opp_idx++) {
            int opp_c0 = opponent_range->hands[opp_idx][0];
            int opp_c1 = opponent_range->hands[opp_idx][1];
            
            // Skip if cards overlap
            if (cards_overlap(player_c0, player_c1, opp_c0, opp_c1, game_state.board, game_state.board_size)) {
                continue;
            }
            
            num_opponent_samples++;
            
            // Create solver
            int p0_c0, p0_c1, p1_c0, p1_c1;
            if (game_state.current_player == 0) {
                // OOP is player 0
                p0_c0 = player_c0;
                p0_c1 = player_c1;
                p1_c0 = opp_c0;
                p1_c1 = opp_c1;
            } else {
                // IP is player 1
                p0_c0 = opp_c0;
                p0_c1 = opp_c1;
                p1_c0 = player_c0;
                p1_c1 = player_c1;
            }
            
            MCCFRSolver *solver = mccfr_create(p0_c0, p0_c1, p1_c0, p1_c1, hand_ranks);
            mccfr_set_board(solver, game_state.board, game_state.current_street);
            
            // Run solver with more iterations for better convergence
            // Need many iterations for MCCFR to find balanced strategies with proper bluffing frequencies
            // Also need enough iterations to explore both players' decision nodes
            mccfr_solve(solver, 5000);  // Increased to 5000 to ensure IP nodes are explored enough
            
            // Get strategy for current player
            // Note: Solver always starts with player 0, so if current player is 1 (IP),
            // we need to query after player 0 has checked (to get to player 1's decision point)
            InfoSet iset;
            memset(&iset, 0, sizeof(InfoSet));
            iset.street = game_state.current_street;
            
            // If IP (player 1) is acting, we need to represent that OOP (player 0) checked first
            // The info set for IP's decision has: player=1, num_actions=1, action_history[0]=OOP's check
            if (game_state.current_player == 1) {
                iset.player = 1;  // IP is acting
                iset.num_actions = 1;
                iset.action_history[0] = ACTION_CHECK_CALL;  // OOP checked, now IP acts
            } else {
                iset.player = 0;  // OOP acts first (root node)
                iset.num_actions = 0;
            }
            
            for (int i = 0; i < 5; i++) {
                iset.board_cards[i] = game_state.board[i];
            }
            
            // Debug: Verify info set matches what solver creates
            fprintf(stderr, "Querying strategy for Player=%d, Street=%d, Actions=%d\n",
                    iset.player, iset.street, iset.num_actions);
            
            InfoSetData *data = mccfr_get_or_create(solver, &iset);
            if (data) {
                // Debug: Output MCCFR result
                fprintf(stderr, "MCCFR Query: Player=%d, Street=%d, Actions=%d, Visits=%lu\n",
                        iset.player, iset.street, iset.num_actions, data->visits);
                if (iset.num_actions > 0) {
                    fprintf(stderr, "  Action history: ");
                    for (int i = 0; i < iset.num_actions; i++) {
                        fprintf(stderr, "%d ", iset.action_history[i]);
                    }
                    fprintf(stderr, "\n");
                }
                fprintf(stderr, "  Strategy: Check=%.3f, Bet=%.3f, Fold=%.3f\n",
                        data->strategy[0], data->strategy[1], data->strategy[2]);
                fprintf(stderr, "  Strategy sum: Check=%.3f, Bet=%.3f, Fold=%.3f\n",
                        data->strategy_sum[0], data->strategy_sum[1], data->strategy_sum[2]);
                
                // Use the normalized strategy (computed in mccfr_solve)
                double sum = 0.0;
                for (int a = 0; a < 3; a++) {
                    sum += data->strategy[a];
                }
                
                if (sum > 0.001) {  // Use small epsilon to avoid floating point issues
                    // Strategy is normalized - use it directly
                    fprintf(stderr, "  Using normalized strategy (sum=%.3f)\n", sum);
                    for (int a = 0; a < 3; a++) {
                        strategy_sum[a] += data->strategy[a];
                    }
                } else {
                    // Strategy not normalized - check if node was visited
                    fprintf(stderr, "  Warning: Strategy sum is %.3f, visits=%lu\n", sum, data->visits);
                    if (data->visits > 0) {
                        // Node was visited but strategy_sum is zero - use uniform as fallback
                        fprintf(stderr, "  Using uniform fallback (node was visited)\n");
                        for (int a = 0; a < 3; a++) {
                            strategy_sum[a] += 1.0 / 3.0;
                        }
                    } else {
                        // Node wasn't visited - use uniform strategy
                        fprintf(stderr, "  Using uniform fallback (node not visited)\n");
                        for (int a = 0; a < 3; a++) {
                            strategy_sum[a] += 1.0 / 3.0;
                        }
                    }
                }
            } else {
                fprintf(stderr, "  Error: Could not get strategy data!\n");
            }
            
            mccfr_free(solver);
        }
        
        if (num_opponent_samples > 0) {
            // Average strategies across opponent samples
            double strategy[3];
            for (int a = 0; a < 3; a++) {
                strategy[a] = strategy_sum[a] / num_opponent_samples;
            }
            
            // Normalize to ensure they sum to 1
            double total = strategy[0] + strategy[1] + strategy[2];
            if (total > 0) {
                for (int a = 0; a < 3; a++) {
                    strategy[a] /= total;
                }
            } else {
                for (int a = 0; a < 3; a++) {
                    strategy[a] = 1.0 / 3.0;
                }
            }
            
            // Add strategy for this specific hand (includes suit information via the actual cards)
            fprintf(stderr, "  Final averaged strategy for %s: Check=%.1f%%, Bet=%.1f%%, Fold=%.1f%%\n",
                    target_cat, strategy[0]*100, strategy[1]*100, strategy[2]*100);
            gui_add_strategy(target_cat, strategy, game_state.board, game_state.board_size, game_state.current_street);
        }
    }
    
    fprintf(stderr, "Total strategies computed: %d\n", strategies[game_state.current_street].count);
    solver_needs_update = 0;
}

// Process action and advance game state
static void process_action(int action) {
    if (!game_state.waiting_for_action || game_state.game_complete) return;
    
    // Add action to history
    if (game_state.num_actions < 10) {
        game_state.action_history[game_state.num_actions++] = action;
    }
    
    // Check if both players have acted (check/check or bet/call or bet/fold)
    int both_acted = 0;
    if (game_state.num_actions >= 2) {
        int last = game_state.action_history[game_state.num_actions - 1];
        int second_last = game_state.action_history[game_state.num_actions - 2];
        
        // Both checked/called
        if ((last == ACTION_CHECK_CALL && second_last == ACTION_CHECK_CALL) ||
            (last == ACTION_CHECK_CALL && second_last == ACTION_BET_RAISE) ||
            (last == ACTION_FOLD)) {
            both_acted = 1;
        }
    }
    
    if (both_acted) {
        // Advance to next street or end game
        if (game_state.current_street == STREET_FLOP) {
            // Move to turn
            game_state.current_street = STREET_TURN;
            game_state.current_player = 0;  // OOP acts first on turn
            
            // Deal turn card
            int used_cards[7] = {0};  // Placeholder - would need actual player hands
            int turn_card = (game_state.selected_turn_card >= 0) ? 
                           game_state.selected_turn_card : 
                           generate_random_card(used_cards, game_state.board_size);
            if (turn_card >= 0) {
                game_state.board[3] = turn_card;
                game_state.board_size = 4;
            }
            
            // Reset action history for new street
            game_state.num_actions = 0;
            game_state.waiting_for_action = 1;
            solver_needs_update = 1;
        } else if (game_state.current_street == STREET_TURN) {
            // Move to river
            game_state.current_street = STREET_RIVER;
            game_state.current_player = 0;  // OOP acts first on river
            
            // Deal river card
            int used_cards[7] = {0};  // Placeholder
            int river_card = (game_state.selected_river_card >= 0) ? 
                            game_state.selected_river_card : 
                            generate_random_card(used_cards, game_state.board_size);
            if (river_card >= 0) {
                game_state.board[4] = river_card;
                game_state.board_size = 5;
            }
            
            // Reset action history for new street
            game_state.num_actions = 0;
            game_state.waiting_for_action = 1;
            solver_needs_update = 1;
        } else {
            // River - game complete
            game_state.game_complete = 1;
            game_state.waiting_for_action = 0;
        }
    } else {
        // Switch to other player
        game_state.current_player = 1 - game_state.current_player;
        game_state.waiting_for_action = 1;
        solver_needs_update = 1;  // Need to recompute strategies for new player
    }
    
    current_street = game_state.current_street;
}

static void render_header(void) {
#ifdef USE_GUI
    // Title
    render_text("TurboFire GTO Solver", 20, 10, font, color_text);
    
    // Board display (moved up to avoid overlap)
    char board_text[64] = "Board: ";
    if (game_state.board_size > 0) {
        for (int i = 0; i < game_state.board_size; i++) {
            if (game_state.board[i] >= 0) {
                char card[3];
                card_str(game_state.board[i], card);
                strcat(board_text, card);
                strcat(board_text, " ");
            }
        }
    } else {
        strcat(board_text, "No cards");
    }
    render_text(board_text, 350, 10, small_font, color_text);
    
    // Action buttons (only show if waiting for action)
    if (game_state.waiting_for_action && !game_state.game_complete) {
        int btn_y = 40;
        int btn_width = 100;
        int btn_height = 35;
        int btn_spacing = 110;
        
        // Check/Call button (Green)
        SDL_Color btn_color = color_check;
        SDL_Rect btn_rect = {20, btn_y, btn_width, btn_height};
        SDL_SetRenderDrawColor(renderer, btn_color.r, btn_color.g, btn_color.b, 255);
        SDL_RenderFillRect(renderer, &btn_rect);
        render_text("Check/Call", 25, btn_y + 8, small_font, color_text);
        
        // Bet/Raise button (Red)
        btn_color = color_bet;
        btn_rect.x = 20 + btn_spacing;
        SDL_SetRenderDrawColor(renderer, btn_color.r, btn_color.g, btn_color.b, 255);
        SDL_RenderFillRect(renderer, &btn_rect);
        render_text("Bet/Raise", 25 + btn_spacing, btn_y + 8, small_font, color_text);
        
        // Fold button (Blue)
        btn_color = color_fold;
        btn_rect.x = 20 + btn_spacing * 2;
        SDL_SetRenderDrawColor(renderer, btn_color.r, btn_color.g, btn_color.b, 255);
        SDL_RenderFillRect(renderer, &btn_rect);
        render_text("Fold", 25 + btn_spacing * 2, btn_y + 8, small_font, color_text);
    }
    
    // Game state info (moved below buttons to avoid overlap)
    char state_text[128];
    const char *player_name = (game_state.current_player == 0) ? "OOP (BB)" : "IP (SB)";
    const char *street_names[] = {"Flop", "Turn", "River"};
    snprintf(state_text, sizeof(state_text), "%s - %s", player_name, street_names[game_state.current_street]);
    int state_y = (game_state.waiting_for_action && !game_state.game_complete) ? 80 : 40;
    render_text(state_text, 20, state_y, small_font, color_text);
    
    // Game complete message
    if (game_state.game_complete) {
        render_text("Game Complete!", 20, state_y + 25, font, color_text);
    }
    
    // Legend (moved to bottom right)
    render_text("Check/Call", WINDOW_WIDTH - 200, WINDOW_HEIGHT - 60, small_font, color_check);
    render_text("Bet/Raise", WINDOW_WIDTH - 200, WINDOW_HEIGHT - 40, small_font, color_bet);
    render_text("Fold", WINDOW_WIDTH - 200, WINDOW_HEIGHT - 20, small_font, color_fold);
#endif
}

static void render_hover_tooltip(void) {
#ifdef USE_GUI
    if (hover_row < 0 || hover_col < 0 || strlen(hover_text) == 0) return;
    
    int x = GRID_X_OFFSET + hover_col * CELL_SIZE + CELL_SIZE;
    int y = GRID_Y_OFFSET + hover_row * CELL_SIZE;
    
    if (x + 200 > WINDOW_WIDTH) x = GRID_X_OFFSET + hover_col * CELL_SIZE - 200;
    
    // Count lines to size tooltip properly
    int num_lines = 1;
    for (int i = 0; hover_text[i] != '\0'; i++) {
        if (hover_text[i] == '\n') num_lines++;
    }
    
    int line_height = 16;
    int tooltip_height = num_lines * line_height + 20;
    if (y + tooltip_height > WINDOW_HEIGHT) y = GRID_Y_OFFSET + hover_row * CELL_SIZE - tooltip_height;
    
    SDL_Rect tooltip_rect = {x, y, 200, tooltip_height};
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 240);
    SDL_RenderFillRect(renderer, &tooltip_rect);
    
    SDL_SetRenderDrawColor(renderer, color_text.r, color_text.g, color_text.b, 255);
    SDL_RenderDrawRect(renderer, &tooltip_rect);
    
    // Render each line separately (handle newlines)
    char hover_copy[256];
    strncpy(hover_copy, hover_text, sizeof(hover_copy) - 1);
    hover_copy[sizeof(hover_copy) - 1] = '\0';
    
    char *line_start = hover_copy;
    int current_y = y + 10;
    for (int i = 0; hover_copy[i] != '\0'; i++) {
        if (hover_copy[i] == '\n') {
            hover_copy[i] = '\0';  // Temporarily null-terminate this line
            if (strlen(line_start) > 0) {
                render_text(line_start, x + 10, current_y, small_font, color_text);
                current_y += line_height;
            }
            line_start = &hover_copy[i + 1];
        }
    }
    // Render last line (if no trailing newline)
    if (*line_start != '\0' && strlen(line_start) > 0) {
        render_text(line_start, x + 10, current_y, small_font, color_text);
    }
}
#endif

void gui_run(void) {
#ifdef USE_GUI
    if (!window || !renderer) return;
    
    SDL_Event e;
    int running = 1;
    
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x;
                int y = e.button.y;
                
                // Check action buttons (only if waiting for action)
                if (game_state.waiting_for_action && !game_state.game_complete) {
                    int btn_y = 40;
                    int btn_width = 100;
                    int btn_height = 35;
                    int btn_spacing = 110;
                    
                    if (y >= btn_y && y <= btn_y + btn_height) {
                        // Check/Call button
                        if (x >= 20 && x <= 20 + btn_width) {
                            process_action(ACTION_CHECK_CALL);
                        }
                        // Bet/Raise button
                        else if (x >= 20 + btn_spacing && x <= 20 + btn_spacing + btn_width) {
                            process_action(ACTION_BET_RAISE);
                        }
                        // Fold button
                        else if (x >= 20 + btn_spacing * 2 && x <= 20 + btn_spacing * 2 + btn_width) {
                            process_action(ACTION_FOLD);
                        }
                    }
                }
                
                // Check for card selection (for turn/river)
                // This would be implemented as a card picker interface
                // For now, we'll skip this and use random cards
                
            } else if (e.type == SDL_MOUSEMOTION) {
                int x = e.motion.x;
                int y = e.motion.y;
                
                // Check if mouse is over grid
                if (x >= GRID_X_OFFSET && x < GRID_X_OFFSET + GRID_SIZE * CELL_SIZE &&
                    y >= GRID_Y_OFFSET && y < GRID_Y_OFFSET + GRID_SIZE * CELL_SIZE) {
                    hover_col = (x - GRID_X_OFFSET) / CELL_SIZE;
                    hover_row = (y - GRID_Y_OFFSET) / CELL_SIZE;
                    
                    // Build hover text (convert display coords to rank indices)
                    int display_row = 12 - hover_row;
                    int display_col = 12 - hover_col;
                    GUIStrategySet *set = &strategies[game_state.current_street];
                    hover_text[0] = '\0';
                    
                    int is_pair = (display_row == display_col);
                    int is_suited = (display_row < display_col);
                    
                    for (int i = 0; i < set->count; i++) {
                        // Only show strategy for the current player
                        if (set->data[i].player != game_state.current_player) continue;
                        
                        int s_row, s_col, s_suited, s_pair;
                        get_hand_coords(set->data[i].category, &s_row, &s_col, &s_suited, &s_pair);
                        
                        if (s_row == display_row && s_col == display_col) {
                            if ((is_pair && s_pair) || (is_suited && s_suited) || (!is_suited && !s_pair)) {
                                snprintf(hover_text, sizeof(hover_text),
                                        "%s\n\nCheck: %.1f%%\nBet: %.1f%%\nFold: %.1f%%",
                                        set->data[i].category,
                                        set->data[i].strategy[0] * 100,
                                        set->data[i].strategy[1] * 100,
                                        set->data[i].strategy[2] * 100);
                                break;
                            }
                        }
                    }
                    
                    if (hover_text[0] == '\0') {
                        char hand_label[8];
                        if (display_row == display_col) {
                            snprintf(hand_label, sizeof(hand_label), "%c%c", RANKS[display_row], RANKS[display_row]);
                        } else if (display_row < display_col) {
                            snprintf(hand_label, sizeof(hand_label), "%c%cs", RANKS[display_col], RANKS[display_row]);
                        } else {
                            snprintf(hand_label, sizeof(hand_label), "%c%co", RANKS[display_row], RANKS[display_col]);
                        }
                        snprintf(hover_text, sizeof(hover_text), "%s\nNo data", hand_label);
                    }
                } else {
                    hover_row = hover_col = -1;
                    hover_text[0] = '\0';
                }
            }
        }
        
        // Compute strategies if needed
        if (solver_needs_update && hand_ranks && sb_range && bb_range && game_state.board_size >= 3) {
            compute_strategies_for_current_state();
        }
        
        // Render
        SDL_SetRenderDrawColor(renderer, color_bg.r, color_bg.g, color_bg.b, 255);
        SDL_RenderClear(renderer);
        
        render_header();
        render_grid();
        render_hover_tooltip();
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }
#else
    // Stub - no-op when GUI disabled
#endif
}
