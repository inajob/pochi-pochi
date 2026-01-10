#include "game_jump.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Webassembly-specific functions for jump game
#ifdef __EMSCRIPTEN__
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


// --- Game-specific Helper Functions ---

void draw_player(GameState& state) {
    int y = (int)state.g.jump->player_y;
    if (state.g.jump->player_x >= 0 && state.g.jump->player_x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        state.screen[y][state.g.jump->player_x] = PLAYER_COLOR;
    }
}

void draw_obstacles(GameState& state) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        int obs_x = (int)state.g.jump->obstacles[i].x;
        for (int w = 0; w < OBSTACLE_WIDTH; ++w) {
            if (obs_x + w < 0 || obs_x + w >= SCREEN_WIDTH) continue;
            for (int y = 0; y < SCREEN_HEIGHT; ++y) {
                if (y > state.g.jump->obstacles[i].gap_y - state.g.jump->obstacles[i].gap_size / 2 && y < state.g.jump->obstacles[i].gap_y + state.g.jump->obstacles[i].gap_size / 2) continue;
                state.screen[y][obs_x + w] = OBSTACLE_COLOR;
            }
        }
    }
}

void spawn_obstacle(Obstacle& obstacle, float x_pos) {
    obstacle.x = x_pos;
    obstacle.gap_size = FIXED_GAP_SIZE;
    obstacle.gap_y = FIXED_GAP_Y;
    obstacle.scored = false;
}

void update_obstacles(GameState& state) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        state.g.jump->obstacles[i].x -= OBSTACLE_SPEED;
        if (state.g.jump->obstacles[i].x + OBSTACLE_WIDTH < 0) {
            float max_x = 0;
            for (int j = 0; j < MAX_OBSTACLES; ++j) {
                if (state.g.jump->obstacles[j].x > max_x) max_x = state.g.jump->obstacles[j].x;
            }
            int random_spacing = MIN_OBSTACLE_SPACING + (rand() % (MAX_OBSTACLE_SPACING - MIN_OBSTACLE_SPACING + 1));
            spawn_obstacle(state.g.jump->obstacles[i], max_x + random_spacing);
        }
    }
}

bool check_collision(GameState& state) {
    int player_y = (int)state.g.jump->player_y;
    if (player_y >= SCREEN_HEIGHT || player_y < 0) return true;
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        int obs_x_start = (int)state.g.jump->obstacles[i].x;
        int obs_x_end = obs_x_start + OBSTACLE_WIDTH - 1;
        if (state.g.jump->player_x >= obs_x_start && state.g.jump->player_x <= obs_x_end) {
            int gap_y_start = state.g.jump->obstacles[i].gap_y - state.g.jump->obstacles[i].gap_size / 2;
            int gap_y_end = state.g.jump->obstacles[i].gap_y + state.g.jump->obstacles[i].gap_size / 2 - 1;
            if (player_y < gap_y_start || player_y > gap_y_end) return true;
        }
    }
    return false;
}

// --- Public API Functions ---

void draw_jump_title(GameState& state) {
    const char* title_text = "JUMP";
    int text_width = strlen(title_text) * 6;
    
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -text_width) {
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, 3);
}

void init_jump_game(GameState& state) {
    state.phase = PHASE_COUNTDOWN;
    state.score = 0;
    state.frame_count = 0;
    
    state.g.jump = new JumpGameState();
    state.g.jump->player_x = 3;
    state.g.jump->player_y = SCREEN_HEIGHT / 2.0f;
    state.g.jump->player_velocity_y = 0;
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        spawn_obstacle(state.g.jump->obstacles[i], SCREEN_WIDTH + i * (MIN_OBSTACLE_SPACING + 2));
    }
#ifdef __EMSCRIPTEN__
    js_update_score(state.score);
#endif
}

void update_jump_game(GameState& state, bool jump_button_pressed) {
    switch (state.phase) {
        case PHASE_COUNTDOWN: {
            state.frame_count++;
            
            draw_player(state);
            
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

        case PHASE_PLAYING:
            state.frame_count++;
            if (jump_button_pressed && state.g.jump->player_y >= SCREEN_HEIGHT - 2) state.g.jump->player_velocity_y = JUMP_FORCE;
            state.g.jump->player_velocity_y += GRAVITY;
            state.g.jump->player_y += state.g.jump->player_velocity_y;
            if (state.g.jump->player_y >= SCREEN_HEIGHT - 1) { state.g.jump->player_y = SCREEN_HEIGHT - 1; state.g.jump->player_velocity_y = 0; }
            if (state.g.jump->player_y < 0) { state.g.jump->player_y = 0; state.g.jump->player_velocity_y = 0; }
            
            update_obstacles(state);

            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                if (!state.g.jump->obstacles[i].scored && (state.g.jump->obstacles[i].x + OBSTACLE_WIDTH < state.g.jump->player_x)) {
                    state.score++;
                    state.g.jump->obstacles[i].scored = true;
#ifdef __EMSCRIPTEN__
                    js_update_score(state.score);
#endif
                }
            }
            
            if (check_collision(state)) {
                state.phase = PHASE_GAME_OVER;
                state.text_scroll_offset = SCREEN_WIDTH;
                state.frame_count = 0;
#ifdef __EMSCRIPTEN__
                js_update_score(state.score);
#endif
            } else {
                draw_obstacles(state);
                draw_player(state);
            }
            break;

        case PHASE_GAME_OVER: {
            state.frame_count++;

            const char* game_text = "GAME";
            const char* over_text = "OVER";
            int text_width = strlen(game_text) * 6;

            state.text_scroll_offset -= 0.5f;
            if (state.text_scroll_offset < -text_width) {
                state.text_scroll_offset = SCREEN_WIDTH;
            }

            draw_text(state, game_text, (int)state.text_scroll_offset, 2, 1);
            draw_text(state, over_text, (int)state.text_scroll_offset, 8, 1);
            draw_score(state, SCREEN_WIDTH / 2, 10, 7);

            const int GAMEOVER_INPUT_DELAY_FRAMES = 30;
            if (jump_button_pressed && !state.was_button_pressed_last_frame && state.frame_count > GAMEOVER_INPUT_DELAY_FRAMES) {
                delete state.g.jump;
                state.g.jump = nullptr;
                init_game(state);
            }
            break;
        }
        default:
            break;
    }
}