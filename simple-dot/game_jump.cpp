#include "game_jump.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// Webassembly-specific functions for jump game
EM_JS(void, js_update_score, (int score), {
    if (window.updateScoreDisplay) {
        window.updateScoreDisplay(score);
    }
});
#endif

// --- Game Constants ---
const float GRAVITY = 0.15f;
const float JUMP_FORCE = -1.5f;
const int PLAYER_COLOR = 3;
const int OBSTACLE_COLOR = 1;
const int OBSTACLE_WIDTH = 2;

// --- Difficulty Constants ---
const int MAX_DIFFICULTY_LEVELS = 4; // Levels 0, 1, 2, 3
const float OBSTACLE_SPEED_LEVELS[] = {0.3f, 0.4f, 0.5f, 0.6f};
const int MIN_OBSTACLE_SPACING_LEVELS[] = {12, 10, 8, 6};
const int MAX_OBSTACLE_SPACING_LEVELS[] = {20, 16, 14, 10};
const int OBSTACLE_HEIGHT_MAX_LEVELS[] = {3, 5, 5, 5}; // Max height of random walls
const int SCORE_THRESHOLDS_JUMP[] = {5, 20, 50}; // Score needed to reach Level 1, 2, 3


// --- Constructor ---
JumpGame::JumpGame(GameState& state) {
    // Reset global state
    state.score = 0;
    state.frame_count = 0;
    
    // Set internal state
    m_phase = JUMP_PHASE_COUNTDOWN;
    m_frame_counter = 0;
    m_player_x = 3;
    m_player_y = SCREEN_HEIGHT / 2.0f;
    m_player_velocity_y = 0;
    
    // Initialize difficulty parameters
    m_difficulty_level = 0;
    m_current_obstacle_speed = OBSTACLE_SPEED_LEVELS[0];
    m_current_min_obstacle_spacing = MIN_OBSTACLE_SPACING_LEVELS[0];
    m_current_max_obstacle_spacing = MAX_OBSTACLE_SPACING_LEVELS[0];
    m_current_obstacle_height_max = OBSTACLE_HEIGHT_MAX_LEVELS[0];
    m_next_difficulty_score_threshold = SCORE_THRESHOLDS_JUMP[0];

    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        spawn_obstacle(m_obstacles[i], SCREEN_WIDTH + i * (m_current_min_obstacle_spacing + 2)); // Use current spacing
    }
#ifdef __EMSCRIPTEN__
    js_update_score(state.score);
#endif
}


// --- Public Methods ---

void JumpGame::draw_title(GameState& state) {
    const char* title_text = "JUMP";
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -(float)strlen(title_text) * 6) {
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, 3);
}

bool JumpGame::update(GameState& state, bool button_pressed) {
    m_frame_counter++;

    switch (m_phase) {
        case JUMP_PHASE_COUNTDOWN: {
            draw_player(state);
            
            const int frames_per_number = 60;
            int number = 3 - (m_frame_counter / frames_per_number);
            
            if (number > 0) {
                draw_char(state, (char)('0' + number), 6, 5, 7); 
            }

            if (m_frame_counter >= frames_per_number * 3) {
                m_phase = JUMP_PHASE_PLAYING;
                m_frame_counter = 0;
            }
            break;
        }

        case JUMP_PHASE_PLAYING: {
            // --- Difficulty Scaling ---
            if (m_difficulty_level < MAX_DIFFICULTY_LEVELS - 1 && state.score >= m_next_difficulty_score_threshold) {
                m_difficulty_level++;
                m_current_obstacle_speed = OBSTACLE_SPEED_LEVELS[m_difficulty_level];
                m_current_min_obstacle_spacing = MIN_OBSTACLE_SPACING_LEVELS[m_difficulty_level];
                m_current_max_obstacle_spacing = MAX_OBSTACLE_SPACING_LEVELS[m_difficulty_level];
                m_current_obstacle_height_max = OBSTACLE_HEIGHT_MAX_LEVELS[m_difficulty_level];
                if (m_difficulty_level < MAX_DIFFICULTY_LEVELS - 1) { // Ensure we don't access out of bounds
                    m_next_difficulty_score_threshold = SCORE_THRESHOLDS_JUMP[m_difficulty_level];
                } else {
                    m_next_difficulty_score_threshold = -1; // No further thresholds
                }
            }
            // --- End Difficulty Scaling ---

            if (button_pressed && m_player_y >= SCREEN_HEIGHT - 2) m_player_velocity_y = JUMP_FORCE;
            m_player_velocity_y += GRAVITY;
            m_player_y += m_player_velocity_y;
            if (m_player_y >= SCREEN_HEIGHT - 1) { m_player_y = SCREEN_HEIGHT - 1; m_player_velocity_y = 0; }
            if (m_player_y < 0) { m_player_y = 0; m_player_velocity_y = 0; }
            
            update_obstacles();

            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                if (!m_obstacles[i].scored && (m_obstacles[i].x + OBSTACLE_WIDTH < m_player_x)) {
                    state.score++;
                    m_obstacles[i].scored = true;
#ifdef __EMSCRIPTEN__
                    js_update_score(state.score);
#endif
                }
            }
            
            if (check_collision()) {
                m_phase = JUMP_PHASE_GAMEOVER;
                m_frame_counter = 0;
                state.text_scroll_offset = SCREEN_WIDTH; // Reset for game over text
#ifdef __EMSCRIPTEN__
                js_update_score(state.score);
#endif
            } else {
                draw_obstacles(state);
                draw_player(state);
            }
            break;
        }

        case JUMP_PHASE_GAMEOVER: {
            const char* game_text = "GAME";
            const char* over_text = "OVER";

            state.text_scroll_offset -= 0.5f;
            if (state.text_scroll_offset < -(float)strlen(game_text) * 6) {
                state.text_scroll_offset = SCREEN_WIDTH;
            }

            draw_text(state, game_text, (int)state.text_scroll_offset, 2, 1);
            draw_text(state, over_text, (int)state.text_scroll_offset, 8, 1);
            draw_score(state, SCREEN_WIDTH / 2, 10, 7);

            const int GAMEOVER_INPUT_DELAY_FRAMES = 30;
            if (button_pressed && !state.was_button_pressed_last_frame && m_frame_counter > GAMEOVER_INPUT_DELAY_FRAMES) {
                return true; // Signal to return to title screen
            }
            break;
        }
    }
    return false; // By default, stay in the game
}


// --- Private Methods ---

void JumpGame::draw_player(GameState& state) {
    int y = (int)m_player_y;
    if (m_player_x >= 0 && m_player_x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        state.screen[y][m_player_x] = PLAYER_COLOR;
    }
}

void JumpGame::draw_obstacles(GameState& state) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        int obs_x = (int)m_obstacles[i].x;
        for (int w = 0; w < OBSTACLE_WIDTH; ++w) {
            if (obs_x + w < 0 || obs_x + w >= SCREEN_WIDTH) continue;
            // Draw wall from floor up to obstacle.height
            for (int y = 0; y < SCREEN_HEIGHT; ++y) {
                if (y >= SCREEN_HEIGHT - m_obstacles[i].height) { // If y is within the wall's height from floor
                    state.screen[y][obs_x + w] = OBSTACLE_COLOR;
                }
            }
        }
    }
}

void JumpGame::spawn_obstacle(Obstacle& obstacle, float x_pos) {
    obstacle.x = x_pos;
    obstacle.height = 1 + (rand() % m_current_obstacle_height_max); // Random height from 1 to m_current_obstacle_height_max
    obstacle.scored = false;
}

void JumpGame::update_obstacles() {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        m_obstacles[i].x -= m_current_obstacle_speed; // Use current speed
        if (m_obstacles[i].x + OBSTACLE_WIDTH < 0) {
            float max_x = 0;
            for (int j = 0; j < MAX_OBSTACLES; ++j) {
                if (m_obstacles[j].x > max_x) max_x = m_obstacles[j].x;
            }
            int random_spacing = m_current_min_obstacle_spacing + (rand() % (m_current_max_obstacle_spacing - m_current_min_obstacle_spacing + 1)); // Use current spacing
            spawn_obstacle(m_obstacles[i], max_x + random_spacing);
        }
    }
}

bool JumpGame::check_collision() {
    int player_y_int = (int)m_player_y;
    if (player_y_int >= SCREEN_HEIGHT || player_y_int < 0) return true; // Boundary collision
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        int obs_x_start = (int)m_obstacles[i].x;
        int obs_x_end = obs_x_start + OBSTACLE_WIDTH - 1;
        if (m_player_x >= obs_x_start && m_player_x <= obs_x_end) { // Player is horizontally within obstacle
            if (player_y_int >= (SCREEN_HEIGHT - m_obstacles[i].height)) { // Player is vertically within wall
                return true; // Collision!
            }
        }
    }
    return false;
}
