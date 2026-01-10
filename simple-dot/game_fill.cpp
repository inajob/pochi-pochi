#include "game_fill.h"
#include <string.h>
#include <stdlib.h>

// --- Game Constants ---
const int PLAYER_COLOR_FILL = 2; // Green
const int PROJECTILE_COLOR = 4; // Blue
const int STATIC_BLOCK_COLOR = 7; // White
const int LINE_CLEAR_EFFECT_COLOR = 3; // Yellow

const int PLAYER_MOVE_SPEED = 20; // Lower is faster, moves every X frames
const int PLAYFIELD_SHIFT_SPEED = 400; // Lower is faster, shifts every X frames

// --- Helper Functions ---
void generate_new_top_row(GameState& state) {
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        state.fill_playfield[0][i] = STATIC_BLOCK_COLOR; // Assume block by default
    }
    state.fill_playfield[0][rand() % SCREEN_WIDTH] = 0;
}


// --- Title ---
void draw_fill_title(GameState& state) {
    const char* title_text = "FILL";
    int text_width = strlen(title_text) * 6;
    
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -text_width) {
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, 4);
}

// --- Game Initialization ---
void init_fill_game(GameState& state) {
    state.phase = PHASE_COUNTDOWN;
    state.score = 0;
    state.frame_count = 0;

    // Clear the play area, but not the whole screen struct potentially
    memset(state.screen, 0, sizeof(state.screen));
    memset(state.fill_playfield, 0, sizeof(state.fill_playfield));

    // Initialize Fill game state
    state.fill_player_x = SCREEN_WIDTH / 2;
    state.fill_player_move_timer = 0;
    state.fill_playfield_shift_timer = 0;
    state.fill_line_clear_timer = 0;
    state.fill_line_clear_y = -1;
    state.fill_projectile_active = false;
}


// --- Game Update ---
void update_fill_game(GameState& state, bool button_pressed) {
    switch (state.phase) {
        case PHASE_COUNTDOWN: {
            state.frame_count++;
            
            // Display countdown number
            const int frames_per_number = 60;
            int number = 3 - (state.frame_count / frames_per_number);
            if (number > 0) {
                draw_char(state, (char)('0' + number), 6, 5, 7); 
            }

            if (state.frame_count >= frames_per_number * 3) {
                state.phase = PHASE_PLAYING;
                state.frame_count = 0; // Reset for game logic
            }
            break;
        }

        case PHASE_PLAYING: {
            state.frame_count++;

            // Check if a line clear animation is in progress
            if (state.fill_line_clear_timer > 0) {
                state.fill_line_clear_timer--;

                // When the animation is over, clear the line and un-pause
                if (state.fill_line_clear_timer == 0) {
                    // Clear the cleared line itself
                    memset(state.fill_playfield[state.fill_line_clear_y], 0, sizeof(uint8_t) * SCREEN_WIDTH);
                    
                    // Clear all rows below the cleared line as a bonus
                    for (int r = state.fill_line_clear_y + 1; r < SCREEN_HEIGHT; r++) {
                        memset(state.fill_playfield[r], 0, sizeof(uint8_t) * SCREEN_WIDTH);
                    }

                    // Reset the clear-in-progress state
                    state.fill_line_clear_y = -1;
                }
            } else {
                // --- Normal game logic (runs only when no animation is playing) ---

                // --- Player Movement ---
                state.fill_player_move_timer++;
                if (state.fill_player_move_timer >= PLAYER_MOVE_SPEED) {
                    state.fill_player_move_timer = 0;
                    state.fill_player_x = (state.fill_player_x + 1) % SCREEN_WIDTH;
                }

                // --- Playfield Shift ---
                state.fill_playfield_shift_timer++;
                if (state.fill_playfield_shift_timer >= PLAYFIELD_SHIFT_SPEED) {
                    state.fill_playfield_shift_timer = 0;

                    // Check for game over BEFORE shifting
                    for (int i = 0; i < SCREEN_WIDTH; i++) {
                        if (state.fill_playfield[SCREEN_HEIGHT - 1][i] != 0) {
                            state.phase = PHASE_GAME_OVER;
                            break;
                        }
                    }
                    if (state.phase == PHASE_GAME_OVER) break;

                    // Shift the entire playfield down
                    for (int r = SCREEN_HEIGHT - 2; r >= 0; r--) {
                        memcpy(state.fill_playfield[r + 1], state.fill_playfield[r], SCREEN_WIDTH * sizeof(uint8_t));
                    }

                    // Generate a new top row
                    generate_new_top_row(state);
                }

                // --- Player Input & Projectile ---
                if (button_pressed && !state.was_button_pressed_last_frame && !state.fill_projectile_active) {
                    state.fill_projectile_active = true;
                    state.fill_projectile_x = state.fill_player_x;
                    state.fill_projectile_y = SCREEN_HEIGHT - 2; // Above player row
                }

                if (state.fill_projectile_active) {
                    // Check for collision on current spot before moving
                    if (state.fill_projectile_y < 0 || state.fill_playfield[state.fill_projectile_y][state.fill_projectile_x] != 0) {
                        int final_y = state.fill_projectile_y + 1;
                        if (final_y < SCREEN_HEIGHT) {
                            state.fill_playfield[final_y][state.fill_projectile_x] = STATIC_BLOCK_COLOR;

                            // Check for line clear
                            bool line_full = true;
                            for(int i = 0; i < SCREEN_WIDTH; i++) {
                                if (state.fill_playfield[final_y][i] == 0) {
                                    line_full = false;
                                    break;
                                }
                            }
                            if (line_full) {
                                state.score += 10;
                                // START the line clear effect
                                state.fill_line_clear_timer = 15; // Effect duration in frames
                                state.fill_line_clear_y = final_y;
                                for (int i = 0; i < SCREEN_WIDTH; i++) {
                                    state.fill_playfield[final_y][i] = LINE_CLEAR_EFFECT_COLOR;
                                }
                            }
                        }
                        state.fill_projectile_active = false;
                    } else {
                         state.fill_projectile_y--;
                    }
                }
            }


            // --- Drawing ---
            // 1. Copy the static playfield to the screen
            memcpy(state.screen, state.fill_playfield, sizeof(state.screen));
            
            // 2. Draw dynamic elements
            // Draw projectile
            if (state.fill_projectile_active) {
                state.screen[state.fill_projectile_y][state.fill_projectile_x] = PROJECTILE_COLOR;
            }

            // Draw player
            state.screen[SCREEN_HEIGHT - 1][state.fill_player_x] = PLAYER_COLOR_FILL;

            break;
        }

        case PHASE_GAME_OVER: {
            state.frame_count++;
            
            draw_text(state, "GAME OVER", 1, 2, 1);
            draw_score(state, SCREEN_WIDTH / 2, 10, 7);

            const int GAMEOVER_INPUT_DELAY_FRAMES = 30;
            if (button_pressed && !state.was_button_pressed_last_frame && state.frame_count > GAMEOVER_INPUT_DELAY_FRAMES) {
                init_game(state); // Go back to title
            }
            break;
        }

        default:
            break;
    }
}