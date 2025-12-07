#include "game_logic.h"
#include "font.h" // Include the new font definition file
#include <string.h>
#include <stdio.h> // For sprintf
#include <stdlib.h> // For rand()

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// --- WebAssembly-specific functions ---
#ifdef __EMSCRIPTEN__
EM_JS(void, js_draw_pixel, (int x, int y, int color), {
    if (window.setPixelInGrid) {
        window.setPixelInGrid(x, y, color);
    }
});
EM_JS(void, js_update_score, (int score), {
    if (window.updateScoreDisplay) {
        window.updateScoreDisplay(score);
    }
});
void render_screen(GameState& state) {
    for (int r = 0; r < SCREEN_HEIGHT; ++r) {
        for (int c = 0; c < SCREEN_WIDTH; ++c) {
            js_draw_pixel(c, r, state.screen[r][c]);
        }
    }
}
#endif

// --- Game Constants ---
const float GRAVITY = 0.15f;
const float JUMP_FORCE = -1.5f;
const int PLAYER_COLOR = 3;
const int OBSTACLE_COLOR = 1;
const int BACKGROUND_COLOR = 0;
const float OBSTACLE_SPEED = 0.3f;
const int MIN_OBSTACLE_SPACING = 12;
const int MAX_OBSTACLE_SPACING = 20;
const int OBSTACLE_WIDTH = 2;
const int FIXED_GAP_SIZE = 7;
const int FIXED_GAP_Y = 9;

// --- Core Drawing & Text Functions ---

void clear_screen(GameState& state) { memset(state.screen, BACKGROUND_COLOR, sizeof(state.screen)); }

void draw_char(GameState& state, char c, int x, int y, int color) {
    int char_index = -1;
    if (c >= '0' && c <= '9') {
        char_index = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        char_index = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'z') {
        char_index = c - 'a' + 10; // Treat lowercase as uppercase
    }

    if (char_index != -1) {
        for (int r = 0; r < 5; ++r) {
            for (int col = 0; col < 5; ++col) {
                if ((font_5x5[char_index][r] >> (4 - col)) & 1) {
                    int px = x + col;
                    int py = y + r;
                    if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT && px >= 0 && py >= 0) {
                        state.screen[py][px] = color;
                    }
                }
            }
        }
    }
}

void draw_text(GameState& state, const char* text, int start_x, int start_y, int color) {
    int x = start_x;
    while (*text) {
        draw_char(state, *text, x, start_y, color);
        x += 6; // 5px char width + 1px spacing
        text++;
    }
}

void draw_player(GameState& state) {
    int y = (int)state.player_y;
    if (state.player_x >= 0 && state.player_x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        state.screen[y][state.player_x] = PLAYER_COLOR;
    }
}

void draw_obstacles(GameState& state) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        int obs_x = (int)state.obstacles[i].x;
        for (int w = 0; w < OBSTACLE_WIDTH; ++w) {
            if (obs_x + w < 0 || obs_x + w >= SCREEN_WIDTH) continue;
            for (int y = 0; y < SCREEN_HEIGHT; ++y) {
                if (y > state.obstacles[i].gap_y - state.obstacles[i].gap_size / 2 && y < state.obstacles[i].gap_y + state.obstacles[i].gap_size / 2) continue;
                state.screen[y][obs_x + w] = OBSTACLE_COLOR;
            }
        }
    }
}

void draw_score(GameState& state, int x, int y, int color) {
    char score_str[4];
    sprintf(score_str, "%d", state.score);
    int text_width = strlen(score_str) * 6 - 1;
    int start_x = x - text_width / 2; // Center horizontally at x
    draw_text(state, score_str, start_x, y, color);
}

void draw_title_screen(GameState& state) {
    clear_screen(state);
    const char* title_text = "JUMP";
    int text_width = strlen(title_text) * 6 - 1;
    draw_text(state, title_text, (SCREEN_WIDTH - text_width) / 2, 5, 3); // Centered text
}

// --- Game Logic Helper Functions ---

void spawn_obstacle(Obstacle& obstacle, float x_pos) {
    obstacle.x = x_pos;
    obstacle.gap_size = FIXED_GAP_SIZE;
    obstacle.gap_y = FIXED_GAP_Y;
    obstacle.scored = false;
}

void update_obstacles(GameState& state) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        state.obstacles[i].x -= OBSTACLE_SPEED;
        if (state.obstacles[i].x + OBSTACLE_WIDTH < 0) {
            float max_x = 0;
            for (int j = 0; j < MAX_OBSTACLES; ++j) {
                if (state.obstacles[j].x > max_x) max_x = state.obstacles[j].x;
            }
            int random_spacing = MIN_OBSTACLE_SPACING + (rand() % (MAX_OBSTACLE_SPACING - MIN_OBSTACLE_SPACING + 1));
            spawn_obstacle(state.obstacles[i], max_x + random_spacing);
        }
    }
}

bool check_collision(GameState& state) {
    int player_y = (int)state.player_y;
    if (player_y >= SCREEN_HEIGHT || player_y < 0) return true;
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        int obs_x_start = (int)state.obstacles[i].x;
        int obs_x_end = obs_x_start + OBSTACLE_WIDTH - 1;
        if (state.player_x >= obs_x_start && state.player_x <= obs_x_end) {
            int gap_y_start = state.obstacles[i].gap_y - state.obstacles[i].gap_size / 2;
            int gap_y_end = state.obstacles[i].gap_y + state.obstacles[i].gap_size / 2 - 1;
            if (player_y < gap_y_start || player_y > gap_y_end) return true;
        }
    }
    return false;
}

void reset_game_for_playing(GameState& state) {
    state.score = 0;
    state.frame_count = 0;
    state.player_x = 3;
    state.player_y = SCREEN_HEIGHT / 2.0f;
    state.player_velocity_y = 0;
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        spawn_obstacle(state.obstacles[i], SCREEN_WIDTH + i * (MIN_OBSTACLE_SPACING + 2));
    }
#ifdef __EMSCRIPTEN__
    js_update_score(state.score);
#endif
}

// --- Public API Functions ---

void init_game(GameState& state) {
    state.phase = PHASE_TITLE;
    state.text_scroll_offset = SCREEN_WIDTH;
}

void update_game(GameState& state, bool jump_button_pressed) {
    clear_screen(state);

    switch (state.phase) {
        case PHASE_TITLE: {
            const char* title_text = "JUMP";
            int text_width = strlen(title_text) * 6;
            state.text_scroll_offset -= 0.5f;
            if (state.text_scroll_offset < -text_width) {
                state.text_scroll_offset = SCREEN_WIDTH;
            }
            draw_text(state, title_text, (int)state.text_scroll_offset, 5, 3);

            if (jump_button_pressed) {
                reset_game_for_playing(state);
                state.phase = PHASE_PLAYING;
            }
            break;
        }

        case PHASE_PLAYING:
            state.frame_count++;
            if (jump_button_pressed && state.player_y >= SCREEN_HEIGHT - 2) state.player_velocity_y = JUMP_FORCE;
            state.player_velocity_y += GRAVITY;
            state.player_y += state.player_velocity_y;
            if (state.player_y >= SCREEN_HEIGHT - 1) { state.player_y = SCREEN_HEIGHT - 1; state.player_velocity_y = 0; }
            if (state.player_y < 0) { state.player_y = 0; state.player_velocity_y = 0; }
            
            update_obstacles(state);

            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                if (!state.obstacles[i].scored && (state.obstacles[i].x + OBSTACLE_WIDTH < state.player_x)) {
                    state.score++;
                    state.obstacles[i].scored = true;
#ifdef __EMSCRIPTEN__
                    js_update_score(state.score);
#endif
                }
            }
            
            if (check_collision(state)) {
                state.phase = PHASE_GAME_OVER;
                state.text_scroll_offset = SCREEN_WIDTH; // Reset scroll for game over screen
                state.frame_count = 0; // Reset frame_count for game over delay
#ifdef __EMSCRIPTEN__
                js_update_score(state.score);
#endif
            } else {
                draw_obstacles(state);
                draw_player(state);
            }
            break;

        case PHASE_GAME_OVER: {
            state.frame_count++; // Use frame_count for input delay

            // Scrolling "GAME OVER" text logic, similar to title
            const char* game_text = "GAME";
            const char* over_text = "OVER";
            int text_width = strlen(game_text) * 6; // Width of "GAME" or "OVER"

            state.text_scroll_offset -= 0.5f;
            if (state.text_scroll_offset < -text_width) {
                state.text_scroll_offset = SCREEN_WIDTH;
            }

            // Drawing logic, incorporating scroll
            clear_screen(state);
            draw_text(state, game_text, (int)state.text_scroll_offset, 2, 1);
            draw_text(state, over_text, (int)state.text_scroll_offset, 8, 1);
            
            // Draw score (fixed at bottom center)
            draw_score(state, SCREEN_WIDTH / 2, 10, 7);

            const int GAMEOVER_INPUT_DELAY_FRAMES = 30; // ~0.5 second delay
            if (jump_button_pressed && state.frame_count > GAMEOVER_INPUT_DELAY_FRAMES) {
                init_game(state);
            }
            break;
        }
    }

#ifdef __EMSCRIPTEN__
    render_screen(state);
#endif
}
