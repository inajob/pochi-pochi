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
const float OBSTACLE_SPEED = 0.3f;
const int MIN_OBSTACLE_SPACING = 12;
const int MAX_OBSTACLE_SPACING = 20;
const int OBSTACLE_WIDTH = 2;
const int FIXED_GAP_SIZE = 7;
const int FIXED_GAP_Y = 9;

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
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        spawn_obstacle(m_obstacles[i], SCREEN_WIDTH + i * (MIN_OBSTACLE_SPACING + 2));
    }
#ifdef __EMSCRIPTEN__
    js_update_score(state.score);
#endif
}


// --- Public Methods ---

void JumpGame::draw_title(GameState& state) {
    // Restore original scrolling logic with float cast fix
    const char* title_text = "JUMP";
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -(float)strlen(title_text) * 6) { // Fix applied here
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    // Draw the actual title with original color
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, 3); // Reverted color to 3
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
            if (state.text_scroll_offset < -(float)strlen(game_text) * 6) { // Fix applied here
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
            for (int y = 0; y < SCREEN_HEIGHT; ++y) {
                if (y > m_obstacles[i].gap_y - m_obstacles[i].gap_size / 2 && y < m_obstacles[i].gap_y + m_obstacles[i].gap_size / 2) continue;
                state.screen[y][obs_x + w] = OBSTACLE_COLOR;
            }
        }
    }
}

void JumpGame::spawn_obstacle(Obstacle& obstacle, float x_pos) {
    obstacle.x = x_pos;
    obstacle.gap_size = FIXED_GAP_SIZE;
    obstacle.gap_y = FIXED_GAP_Y;
    obstacle.scored = false;
}

void JumpGame::update_obstacles() {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        m_obstacles[i].x -= OBSTACLE_SPEED;
        if (m_obstacles[i].x + OBSTACLE_WIDTH < 0) {
            float max_x = 0;
            for (int j = 0; j < MAX_OBSTACLES; ++j) {
                if (m_obstacles[j].x > max_x) max_x = m_obstacles[j].x;
            }
            int random_spacing = MIN_OBSTACLE_SPACING + (rand() % (MAX_OBSTACLE_SPACING - MIN_OBSTACLE_SPACING + 1));
            spawn_obstacle(m_obstacles[i], max_x + random_spacing);
        }
    }
}

bool JumpGame::check_collision() {
    int player_y_int = (int)m_player_y;
    if (player_y_int >= SCREEN_HEIGHT || player_y_int < 0) return true;
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        int obs_x_start = (int)m_obstacles[i].x;
        int obs_x_end = obs_x_start + OBSTACLE_WIDTH - 1;
        if (m_player_x >= obs_x_start && m_player_x <= obs_x_end) {
            int gap_y_start = m_obstacles[i].gap_y - m_obstacles[i].gap_size / 2;
            int gap_y_end = m_obstacles[i].gap_y + m_obstacles[i].gap_size / 2 - 1;
            if (player_y_int < gap_y_start || player_y_int > gap_y_end) return true;
        }
    }
    return false;
}
