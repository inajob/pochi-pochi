#include "game_fill.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// --- Game Constants ---
const int PLAYER_COLOR_FILL = 2; // Green
const int PROJECTILE_COLOR = 4; // Blue
const int STATIC_BLOCK_COLOR = 7; // White
const int LINE_CLEAR_EFFECT_COLOR = 3; // Yellow

const int PLAYER_MOVE_SPEED = 20;
const int PLAYFIELD_SHIFT_SPEED = 400;


// --- Constructor ---
FillGame::FillGame(GameState& state) {
    state.score = 0;
    state.frame_count = 0;
    
    m_phase = FILL_PHASE_COUNTDOWN;
    m_frame_counter = 0;

    memset(m_playfield, 0, sizeof(m_playfield));

    m_player_x = SCREEN_WIDTH / 2;
    m_player_move_timer = 0;
    m_playfield_shift_timer = PLAYFIELD_SHIFT_SPEED;
    m_line_clear_timer = 0;
    m_line_clear_y = -1;
    m_projectile_active = false;
}

// --- Public Methods ---

void FillGame::draw_title(GameState& state) {
    const char* title_text = "FILL";
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -(float)strlen(title_text) * 6) { // Fix applied here
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, 4); // Original color
}

bool FillGame::update(GameState& state, bool button_pressed) {
    m_frame_counter++;

    switch (m_phase) {
        case FILL_PHASE_COUNTDOWN: {
            const int frames_per_number = 60;
            int number = 3 - (m_frame_counter / frames_per_number);
            if (number > 0) {
                draw_char(state, (char)('0' + number), 6, 5, 7); 
            }
            if (m_frame_counter >= frames_per_number * 3) {
                m_phase = FILL_PHASE_PLAYING;
                m_frame_counter = 0;
            }
            break;
        }

        case FILL_PHASE_PLAYING: {
            if (m_line_clear_timer > 0) {
                m_line_clear_timer--;
                if (m_line_clear_timer == 0) {
                    memset(m_playfield[m_line_clear_y], 0, sizeof(uint8_t) * SCREEN_WIDTH);
                    for (int r = m_line_clear_y + 1; r < SCREEN_HEIGHT; r++) {
                        memset(m_playfield[r], 0, sizeof(uint8_t) * SCREEN_WIDTH);
                    }
                    m_line_clear_y = -1;
                }
            } else {
                // --- Normal game logic ---
                m_player_move_timer++;
                if (m_player_move_timer >= PLAYER_MOVE_SPEED) {
                    m_player_move_timer = 0;
                    m_player_x = (m_player_x + 1) % SCREEN_WIDTH;
                }

                m_playfield_shift_timer++;
                if (m_playfield_shift_timer >= PLAYFIELD_SHIFT_SPEED) {
                    m_playfield_shift_timer = 0;
                    for (int i = 0; i < SCREEN_WIDTH; i++) {
                        if (m_playfield[SCREEN_HEIGHT - 1][i] != 0) {
                            m_phase = FILL_PHASE_GAMEOVER;
                            m_frame_counter = 0;
                            break;
                        }
                    }
                    if (m_phase == FILL_PHASE_GAMEOVER) break;

                    for (int r = SCREEN_HEIGHT - 2; r >= 0; r--) {
                        memcpy(m_playfield[r + 1], m_playfield[r], SCREEN_WIDTH * sizeof(uint8_t));
                    }
                    generate_new_top_row();
                }

                if (button_pressed && !state.was_button_pressed_last_frame && !m_projectile_active) {
                    m_projectile_active = true;
                    m_projectile_x = m_player_x;
                    m_projectile_y = SCREEN_HEIGHT - 2;
                }

                if (m_projectile_active) {
                    if (m_projectile_y < 0 || m_playfield[m_projectile_y][m_projectile_x] != 0) {
                        int final_y = m_projectile_y + 1;
                        if (final_y < SCREEN_HEIGHT) {
                            m_playfield[final_y][m_projectile_x] = STATIC_BLOCK_COLOR;
                            bool line_full = true;
                            for(int i = 0; i < SCREEN_WIDTH; i++) {
                                if (m_playfield[final_y][i] == 0) {
                                    line_full = false;
                                    break;
                                }
                            }
                            if (line_full) {
                                state.score += 10;
                                m_line_clear_timer = 15;
                                m_line_clear_y = final_y;
                                for (int i = 0; i < SCREEN_WIDTH; i++) {
                                    m_playfield[final_y][i] = LINE_CLEAR_EFFECT_COLOR;
                                }
                            }
                        }
                        m_projectile_active = false;
                    } else {
                         m_projectile_y--;
                    }
                }
            }
            // --- Drawing ---
            memcpy(state.screen, m_playfield, sizeof(state.screen));
            if (m_projectile_active) {
                state.screen[m_projectile_y][m_projectile_x] = PROJECTILE_COLOR;
            }
            state.screen[SCREEN_HEIGHT - 1][m_player_x] = PLAYER_COLOR_FILL;
            break;
        }

        case FILL_PHASE_GAMEOVER: {
            m_frame_counter++; // Ensure frame counter is incremented

            const char* game_text = "GAME OVER"; // Combined for easier calculation
            state.text_scroll_offset -= 0.5f;
            if (state.text_scroll_offset < -(float)strlen(game_text) * 6) { // Fix applied here
                state.text_scroll_offset = SCREEN_WIDTH;
            }

            draw_text(state, game_text, (int)state.text_scroll_offset, 1, 1); // Drawing combined text
            draw_score(state, SCREEN_WIDTH / 2, 10, 7);

            const int GAMEOVER_INPUT_DELAY_FRAMES = 30; // Defined locally, same as other games
            if (button_pressed && !state.was_button_pressed_last_frame && m_frame_counter > GAMEOVER_INPUT_DELAY_FRAMES) {
                return true; // Signal to return to title
            }
            break;
        }
    }
    return false; // By default, stay in game
}

// --- Private Methods ---

void FillGame::generate_new_top_row() {
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        m_playfield[0][i] = STATIC_BLOCK_COLOR;
    }
    m_playfield[0][rand() % SCREEN_WIDTH] = 0;
}