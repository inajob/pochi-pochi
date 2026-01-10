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
        state.g.fill->playfield[0][i] = STATIC_BLOCK_COLOR; // Assume block by default
    }
    state.g.fill->playfield[0][rand() % SCREEN_WIDTH] = 0;
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

    state.g.fill = new FillGameState();
    
    memset(state.screen, 0, sizeof(state.screen));
    memset(state.g.fill->playfield, 0, sizeof(state.g.fill->playfield));

    state.g.fill->player_x = SCREEN_WIDTH / 2;
    state.g.fill->player_move_timer = 0;
    state.g.fill->playfield_shift_timer = 0;
    state.g.fill->line_clear_timer = 0;
    state.g.fill->line_clear_y = -1;
    state.g.fill->projectile_active = false;
}


// --- Game Update ---
void update_fill_game(GameState& state, bool button_pressed) {
    switch (state.phase) {
        case PHASE_COUNTDOWN: {
            state.frame_count++;
            
            const int frames_per_number = 60;
            int number = 3 - (state.frame_count / frames_per_number);
            if (number > 0) {
                draw_char(state, (char)('0' + number), 6, 5, 7); 
            }

            if (state.frame_count >= frames_per_number * 3) {
                state.phase = PHASE_PLAYING;
                state.frame_count = 0;
            }
            break;
        }

        case PHASE_PLAYING: {
            state.frame_count++;
            FillGameState* f_state = state.g.fill;

            if (f_state->line_clear_timer > 0) {
                f_state->line_clear_timer--;

                if (f_state->line_clear_timer == 0) {
                    memset(f_state->playfield[f_state->line_clear_y], 0, sizeof(uint8_t) * SCREEN_WIDTH);
                    
                    for (int r = f_state->line_clear_y + 1; r < SCREEN_HEIGHT; r++) {
                        memset(f_state->playfield[r], 0, sizeof(uint8_t) * SCREEN_WIDTH);
                    }

                    f_state->line_clear_y = -1;
                }
            } else {
                // --- Normal game logic ---

                f_state->player_move_timer++;
                if (f_state->player_move_timer >= PLAYER_MOVE_SPEED) {
                    f_state->player_move_timer = 0;
                    f_state->player_x = (f_state->player_x + 1) % SCREEN_WIDTH;
                }

                f_state->playfield_shift_timer++;
                if (f_state->playfield_shift_timer >= PLAYFIELD_SHIFT_SPEED) {
                    f_state->playfield_shift_timer = 0;

                    for (int i = 0; i < SCREEN_WIDTH; i++) {
                        if (f_state->playfield[SCREEN_HEIGHT - 1][i] != 0) {
                            state.phase = PHASE_GAME_OVER;
                            break;
                        }
                    }
                    if (state.phase == PHASE_GAME_OVER) break;

                    for (int r = SCREEN_HEIGHT - 2; r >= 0; r--) {
                        memcpy(f_state->playfield[r + 1], f_state->playfield[r], SCREEN_WIDTH * sizeof(uint8_t));
                    }

                    generate_new_top_row(state);
                }

                if (button_pressed && !state.was_button_pressed_last_frame && !f_state->projectile_active) {
                    f_state->projectile_active = true;
                    f_state->projectile_x = f_state->player_x;
                    f_state->projectile_y = SCREEN_HEIGHT - 2;
                }

                if (f_state->projectile_active) {
                    if (f_state->projectile_y < 0 || f_state->playfield[f_state->projectile_y][f_state->projectile_x] != 0) {
                        int final_y = f_state->projectile_y + 1;
                        if (final_y < SCREEN_HEIGHT) {
                            f_state->playfield[final_y][f_state->projectile_x] = STATIC_BLOCK_COLOR;

                            bool line_full = true;
                            for(int i = 0; i < SCREEN_WIDTH; i++) {
                                if (f_state->playfield[final_y][i] == 0) {
                                    line_full = false;
                                    break;
                                }
                            }
                            if (line_full) {
                                state.score += 10;
                                f_state->line_clear_timer = 15;
                                f_state->line_clear_y = final_y;
                                for (int i = 0; i < SCREEN_WIDTH; i++) {
                                    f_state->playfield[final_y][i] = LINE_CLEAR_EFFECT_COLOR;
                                }
                            }
                        }
                        f_state->projectile_active = false;
                    } else {
                         f_state->projectile_y--;
                    }
                }
            }


            // --- Drawing ---
            memcpy(state.screen, f_state->playfield, sizeof(state.screen));
            
            if (f_state->projectile_active) {
                state.screen[f_state->projectile_y][f_state->projectile_x] = PROJECTILE_COLOR;
            }

            state.screen[SCREEN_HEIGHT - 1][f_state->player_x] = PLAYER_COLOR_FILL;

            break;
        }

        case PHASE_GAME_OVER: {
            state.frame_count++;
            
            draw_text(state, "GAME OVER", 1, 2, 1);
            draw_score(state, SCREEN_WIDTH / 2, 10, 7);

            const int GAMEOVER_INPUT_DELAY_FRAMES = 30;
            if (button_pressed && !state.was_button_pressed_last_frame && state.frame_count > GAMEOVER_INPUT_DELAY_FRAMES) {
                delete state.g.fill;
                state.g.fill = nullptr;
                init_game(state); // Go back to title
            }
            break;
        }

        default:
            break;
    }
}
