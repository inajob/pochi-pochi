#include "game_chase.h"
#include <string.h>
#include <stdlib.h> // For rand()
#include <stdio.h> // For sprintf

// --- Chase Game Constants ---
const int PLAYER_Y_POS = 14;
const int LANE_POS[] = {4, 7, 10};
const int NUM_LANES = sizeof(LANE_POS) / sizeof(LANE_POS[0]);
const int CHASE_PLAYER_COLOR = 2; // Green
const int CHASE_WALL_COLOR = 4;   // Blue

// --- Difficulty Constants ---
const int MAX_DIFFICULTY_LEVELS = 4; // Levels 0, 1, 2, 3
const float WALL_SPEED_LEVELS[] = {0.2f, 0.3f, 0.4f, 0.5f};
const int WALL_SPACING_LEVELS[] = {8, 7, 6, 5};
const int SCORE_THRESHOLDS_CHASE[] = {15, 40, 70}; // Score needed to reach Level 1, 2, 3


// --- Constructor ---
ChaseGame::ChaseGame(GameState& state) {
    state.score = 0;
    state.frame_count = 0; // The main frame_count can be used if needed

    m_phase = CHASE_PHASE_PLAYING;
    m_frame_counter = 0;
    m_player_lane_index = 1; // Start in the middle lane

    // Initialize difficulty parameters
    m_difficulty_level = 0;
    m_current_wall_speed = WALL_SPEED_LEVELS[0];
    m_current_wall_spacing = WALL_SPACING_LEVELS[0];
    m_next_difficulty_score_threshold = SCORE_THRESHOLDS_CHASE[0];

    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        spawn_wall(m_walls[i], -i * m_current_wall_spacing); // Use current spacing
    }
}


// --- Public Methods ---

void ChaseGame::draw_title(GameState& state) {
    const char* title_text = "CHASE";
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -(float)strlen(title_text) * 6) { // Fix applied here
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, CHASE_PLAYER_COLOR);
}

bool ChaseGame::update(GameState& state, bool button_pressed) {
    m_frame_counter++;

    switch (m_phase) {
        case CHASE_PHASE_PLAYING: {
            // --- Difficulty Scaling ---
            if (m_difficulty_level < MAX_DIFFICULTY_LEVELS - 1 && state.score >= m_next_difficulty_score_threshold) {
                m_difficulty_level++;
                m_current_wall_speed = WALL_SPEED_LEVELS[m_difficulty_level];
                m_current_wall_spacing = WALL_SPACING_LEVELS[m_difficulty_level];
                if (m_difficulty_level < MAX_DIFFICULTY_LEVELS - 1) { // Ensure we don't access out of bounds
                    m_next_difficulty_score_threshold = SCORE_THRESHOLDS_CHASE[m_difficulty_level];
                } else {
                    m_next_difficulty_score_threshold = -1; // No further thresholds
                }
            }
            // --- End Difficulty Scaling ---

            // --- Handle Input ---
            if (button_pressed && !state.was_button_pressed_last_frame) {
                m_player_lane_index = (m_player_lane_index + 1) % NUM_LANES;
            }

            // --- Update Game State ---
            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                m_walls[i].y_pos += m_current_wall_speed; // Use current speed

                if (!m_walls[i].scored && m_walls[i].y_pos > PLAYER_Y_POS) {
                    state.score++;
                    m_walls[i].scored = true;
                }

                if (m_walls[i].y_pos >= SCREEN_HEIGHT) {
                    spawn_wall(m_walls[i], 0);
                }
            }

            // --- Collision Detection ---
            int player_lane_x = LANE_POS[m_player_lane_index];
            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                int wall_y = (int)m_walls[i].y_pos;
                if (wall_y == PLAYER_Y_POS) {
                    int gap_lane = m_walls[i].gap_lane_index;
                    if (m_player_lane_index != gap_lane) {
                        m_phase = CHASE_PHASE_GAMEOVER;
                        m_frame_counter = 0;
                        state.text_scroll_offset = SCREEN_WIDTH;
                    }
                }
            }

            // --- Drawing ---
            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                int wall_y = (int)m_walls[i].y_pos;
                if (wall_y >= 0 && wall_y < SCREEN_HEIGHT) {
                    int gap_lane_x = LANE_POS[m_walls[i].gap_lane_index];
                    for (int x = 0; x < SCREEN_WIDTH; ++x) {
                        if (x != gap_lane_x) {
                            state.screen[wall_y][x] = CHASE_WALL_COLOR;
                        }
                    }
                }
            }
            state.screen[PLAYER_Y_POS][player_lane_x] = CHASE_PLAYER_COLOR;
            break;
        }
        
        case CHASE_PHASE_GAMEOVER: {
            m_frame_counter++;

            const char* game_text = "GAME";
            const char* over_text = "OVER";
            state.text_scroll_offset -= 0.5f;
            if (state.text_scroll_offset < -(float)strlen(game_text) * 6) { // Fix applied here
                state.text_scroll_offset = SCREEN_WIDTH;
            }

            draw_text(state, game_text, (int)state.text_scroll_offset, 2, 1);
            draw_text(state, over_text, (int)state.text_scroll_offset, 8, 1);
            draw_score(state, SCREEN_WIDTH / 2, 10, 7);

            const int GAMEOVER_INPUT_DELAY_FRAMES = 30;
            if (button_pressed && !state.was_button_pressed_last_frame && m_frame_counter > GAMEOVER_INPUT_DELAY_FRAMES) {
                return true; // Signal to return to title
            }
            break;
        }
    }
    return false; // By default, stay in game
}


// --- Private Methods ---

void ChaseGame::spawn_wall(ChaseObstacle& wall, float y_pos) {
    wall.y_pos = y_pos;
    wall.gap_lane_index = rand() % NUM_LANES;
    wall.scored = false;
}
