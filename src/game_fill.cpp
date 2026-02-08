#include "game_fill.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// --- Game Constants ---
const int PLAYER_COLOR_FILL = 2; // Green
const int PROJECTILE_COLOR = 4; // Blue
const int STATIC_BLOCK_COLOR = 7; // White
const int LINE_CLEAR_EFFECT_COLOR = 3; // Yellow

const int PLAYER_MOVE_SPEED_LEVELS[] = {20, 18, 16, 10};
const int PLAYFIELD_SHIFT_SPEED_LEVELS[] = {400, 300, 200, 150};
const int NUM_GAPS_PER_ROW_LEVELS[] = {1, 2, 3, 4}; // More gaps = harder (user's definition)
const int SCORE_THRESHOLDS[] = {5, 15, 30}; // Score needed to reach Level 1, 2, 3


// --- Helper Functions for Bit Packing ---

// Calculate the byte index for a given column
#define GET_BYTE_INDEX(c) ((c) / 8)
// Calculate the bit position within that byte
#define GET_BIT_POS(c)    ((c) % 8)

// --- Constructor ---
FillGame::FillGame(GameState& state) {
    state.score = 0;
    state.frame_count = 0;
    
    m_phase = FILL_PHASE_PLAYING;
    m_frame_counter = 0;

    memset(m_playfield, 0, sizeof(m_playfield)); // Initialize all bits to 0 (false)

    // Initialize difficulty parameters
    m_difficulty_level = 0;
    m_current_playfield_shift_speed = PLAYFIELD_SHIFT_SPEED_LEVELS[0];
    m_current_player_move_speed = PLAYER_MOVE_SPEED_LEVELS[0];
    m_num_gaps_per_row = NUM_GAPS_PER_ROW_LEVELS[0];
    m_next_difficulty_score_threshold = SCORE_THRESHOLDS[0];

    // Generate initial 5 rows
    for (int r = 0; r < 5; ++r) {
        // Shift existing blocks down to make space for the new top row
        for (int row_idx = SCREEN_HEIGHT - 2; row_idx >= 0; --row_idx) {
            // Shift bit-packed rows
            for(int byte_idx = 0; byte_idx < (SCREEN_WIDTH / 8); ++byte_idx) {
                m_playfield[row_idx + 1][byte_idx] = m_playfield[row_idx][byte_idx];
            }
        }
        generate_new_top_row(); // Generates a new row at the very top (row 0)
    }

    m_player_x = SCREEN_WIDTH / 2;
    m_player_move_timer = 0;
    m_playfield_shift_timer = PLAYFIELD_SHIFT_SPEED_LEVELS[0]; // Initialize with level 0 speed
    m_line_clear_timer = 0;
    m_line_clear_y = -1;
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        m_projectiles[i].active = false;
    }
}

// --- Public Methods ---

void FillGame::draw_title(GameState& state) {
    const char* title_text = "FILL";
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -(float)strlen(title_text) * 6) {
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, 4); // Original color
}

bool FillGame::update(GameState& state, bool button_pressed) {
    m_frame_counter++;

    switch (m_phase) {
        case FILL_PHASE_COUNTDOWN: { // Should be unreachable with current constructor logic
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
                    // Clear the cleared line itself
                    for(int c = 0; c < SCREEN_WIDTH; ++c) {
                        set_pixel_status(m_line_clear_y, c, false);
                    }
                    // Clear all rows below the cleared line as a bonus
                    for (int r = m_line_clear_y + 1; r < SCREEN_HEIGHT; r++) {
                        for(int c = 0; c < SCREEN_WIDTH; ++c) {
                            set_pixel_status(r, c, false);
                        }
                    }
                    m_line_clear_y = -1;
                }
            } else {
                // --- Normal game logic ---

                // --- Difficulty Scaling ---
                if (m_difficulty_level < 3 && state.score >= m_next_difficulty_score_threshold) {
                    m_difficulty_level++;
                    m_current_playfield_shift_speed = PLAYFIELD_SHIFT_SPEED_LEVELS[m_difficulty_level];
                    m_current_player_move_speed = PLAYER_MOVE_SPEED_LEVELS[m_difficulty_level];
                    m_num_gaps_per_row = NUM_GAPS_PER_ROW_LEVELS[m_difficulty_level];
                    if (m_difficulty_level < 3) { // Ensure we don't access out of bounds for SCORE_THRESHOLDS
                        m_next_difficulty_score_threshold = SCORE_THRESHOLDS[m_difficulty_level];
                    } else {
                        m_next_difficulty_score_threshold = -1; // No further thresholds
                    }
                }


                m_player_move_timer++;
                if (m_player_move_timer >= m_current_player_move_speed) { // Use current speed
                    m_player_move_timer = 0;
                    m_player_x = (m_player_x + 1) % SCREEN_WIDTH;
                }

                // --- Fire Projectile ---
                if (button_pressed && !state.was_button_pressed_last_frame) {
                    for (int i = 0; i < MAX_PROJECTILES; ++i) {
                        if (!m_projectiles[i].active) {
                            m_projectiles[i].active = true;
                            m_projectiles[i].x = m_player_x;
                            m_projectiles[i].y = SCREEN_HEIGHT - 2;
                            break; // Exit after firing one projectile per button press
                        }
                    }
                }

                // --- Update Projectiles ---
                for (int i = 0; i < MAX_PROJECTILES; ++i) {
                    if (m_projectiles[i].active) {
                        if (m_projectiles[i].y < 0 || get_pixel_status(m_projectiles[i].y, m_projectiles[i].x)) { // Use helper
                            int final_y = m_projectiles[i].y + 1;
                            if (final_y < SCREEN_HEIGHT) {
                                set_pixel_status(final_y, m_projectiles[i].x, true); // Use helper
                                bool line_full = true;
                                for(int c = 0; c < SCREEN_WIDTH; c++) {
                                    if (!get_pixel_status(final_y, c)) { // Use helper
                                        line_full = false;
                                        break;
                                    }
                                }
                                if (line_full) {
                                    state.score += 1;
                                    m_line_clear_timer = 15;
                                    m_line_clear_y = final_y;
                                    // Draw line clear effect directly, m_playfield doesn't store color
                                }
                            }
                            m_projectiles[i].active = false;
                        } else {
                             m_projectiles[i].y--;
                        }
                    }
                }
                
                // --- Shift Playfield ---
                m_playfield_shift_timer++;
                if (m_playfield_shift_timer >= m_current_playfield_shift_speed) { // Use current speed
                    m_playfield_shift_timer = 0;
                    for (int i = 0; i < SCREEN_WIDTH; i++) {
                        if (get_pixel_status(SCREEN_HEIGHT - 1, i)) { // Use helper
                            m_phase = FILL_PHASE_GAMEOVER;
                            m_frame_counter = 0;
                            break;
                        }
                    }
                    if (m_phase == FILL_PHASE_GAMEOVER) break;

                    // Shift bit-packed rows down
                    for (int r = SCREEN_HEIGHT - 2; r >= 0; r--) {
                        for(int byte_idx = 0; byte_idx < (SCREEN_WIDTH / 8); ++byte_idx) {
                            m_playfield[r + 1][byte_idx] = m_playfield[r][byte_idx];
                        }
                    }
                    generate_new_top_row();
                }
            }
            // --- Drawing ---
            for (int r = 0; r < SCREEN_HEIGHT; ++r) {
                for (int c = 0; c < SCREEN_WIDTH; ++c) {
                    uint8_t pixel_color = 0;
                    if (get_pixel_status(r, c)) { // If there's a block
                        pixel_color = STATIC_BLOCK_COLOR;
                    }

                    // Check for line clear effect
                    if (m_line_clear_timer > 0 && r == m_line_clear_y) {
                        pixel_color = LINE_CLEAR_EFFECT_COLOR;
                    }
                    state.screen[r][c] = pixel_color;
                }
            }

            // Draw projectiles
            for (int i = 0; i < MAX_PROJECTILES; ++i) {
                if (m_projectiles[i].active) {
                    state.screen[m_projectiles[i].y][m_projectiles[i].x] = PROJECTILE_COLOR;
                }
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

// Helper to get pixel status (block/no block)
bool FillGame::get_pixel_status(int r, int c) {
    if (r < 0 || r >= SCREEN_HEIGHT || c < 0 || c >= SCREEN_WIDTH) return false;
    int byte_idx = GET_BYTE_INDEX(c);
    int bit_pos = GET_BIT_POS(c);
    return (m_playfield[r][byte_idx] >> bit_pos) & 1;
}

// Helper to set pixel status (block/no block)
void FillGame::set_pixel_status(int r, int c, bool status) {
    if (r < 0 || r >= SCREEN_HEIGHT || c < 0 || c >= SCREEN_WIDTH) return;
    int byte_idx = GET_BYTE_INDEX(c);
    int bit_pos = GET_BIT_POS(c);
    if (status) {
        m_playfield[r][byte_idx] |= (1 << bit_pos); // Set bit
    } else {
        m_playfield[r][byte_idx] &= ~(1 << bit_pos); // Clear bit
    }
}

void FillGame::generate_new_top_row() {
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        set_pixel_status(0, i, true); // Assume block by default
    }
    for (int k = 0; k < m_num_gaps_per_row; ++k) {
        int gap_x = rand() % SCREEN_WIDTH;
        set_pixel_status(0, gap_x, false); // Make 'm_num_gaps_per_row' gaps
    }
}