#include "game_logic.h"
#include <string.h> // For memset
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdlib.h> // For rand(), srand()

#ifdef __EMSCRIPTEN__
// Use EM_JS to define a JavaScript function that can be called from C++.
EM_JS(void, js_draw_pixel, (int x, int y, int color), {
    if (window.setPixelInGrid) {
        window.setPixelInGrid(x, y, color);
    }
});

// ★ 追加: C++からJS側にスコアを更新する関数を呼び出す
EM_JS(void, js_update_score, (int score), {
    if (window.updateScoreDisplay) {
        window.updateScoreDisplay(score);
    }
});

// Iterates through the screen buffer and calls the JS drawing function for each pixel.
void render_screen(GameState& state) {
    for (int r = 0; r < SCREEN_HEIGHT; ++r) {
        for (int c = 0; c < SCREEN_WIDTH; ++c) {
            js_draw_pixel(c, r, state.screen[r][c]);
        }
    }
}
#endif

// --- Game Constants ---
const float GRAVITY = 0.2f;
const float JUMP_FORCE = -1.5f;
const int PLAYER_COLOR = 3; // Yellow
const int OBSTACLE_COLOR = 1; // Red
const int BACKGROUND_COLOR = 0; // Black

const float OBSTACLE_SPEED = 0.5f;
const int OBSTACLE_SPACING = 12; // Distance between obstacles
const int OBSTACLE_WIDTH = 2;
// const int MIN_GAP_SIZE = 5; // Removed
// const int MAX_GAP_SIZE = 7; // Removed

// --- Internal Helper Functions ---

void clear_screen(GameState& state) {
    memset(state.screen, BACKGROUND_COLOR, sizeof(state.screen));
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
                // Check if the pixel is part of the gap
                if (y > state.obstacles[i].gap_y - state.obstacles[i].gap_size / 2 &&
                    y < state.obstacles[i].gap_y + state.obstacles[i].gap_size / 2) {
                    continue;
                }
                state.screen[y][obs_x + w] = OBSTACLE_COLOR;
            }
        }
    }
}

void spawn_obstacle(Obstacle& obstacle, float x_pos) {
    obstacle.x = x_pos;
    obstacle.gap_size = 8; // Fixed gap size
    obstacle.gap_y = 10;    // Fixed gap Y position (lower)
    obstacle.scored = false; // ★ 追加: スコアフラグをリセット
}


void update_obstacles(GameState& state) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        state.obstacles[i].x -= OBSTACLE_SPEED;

        // If obstacle is off-screen to the left, respawn it on the right
        if (state.obstacles[i].x + OBSTACLE_WIDTH < 0) {
            float max_x = 0;
            for (int j = 0; j < MAX_OBSTACLES; ++j) {
                if (state.obstacles[j].x > max_x) {
                    max_x = state.obstacles[j].x;
                }
            }
            spawn_obstacle(state.obstacles[i], max_x + OBSTACLE_SPACING);
        }
    }
}

void check_collision(GameState& state) {
    int player_int_y = (int)state.player_y;
    int player_int_x = state.player_x; // Player is 1px wide at player_x

    // Check collision with ground/ceiling
    if (player_int_y >= SCREEN_HEIGHT || player_int_y < 0) {
        state.game_over = true;
        return;
    }

    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        // Check if player's X position overlaps with current obstacle's X position
        int obs_x_start = (int)state.obstacles[i].x;
        int obs_x_end = obs_x_start + OBSTACLE_WIDTH - 1;

        if (player_int_x >= obs_x_start && player_int_x <= obs_x_end) {
            // Player's X overlaps with obstacle. Now check Y.
            int gap_y_start = state.obstacles[i].gap_y - state.obstacles[i].gap_size / 2;
            int gap_y_end = state.obstacles[i].gap_y + state.obstacles[i].gap_size / 2 - 1;

            if (player_int_y < gap_y_start || player_int_y > gap_y_end) {
                // Player's Y is not in the gap, so it's a collision!
                state.game_over = true;
                return;
            }
        }
    }
}


// --- Public API Functions ---

void init_game(GameState& state) {
    // srand(time(NULL)); // This can cause issues in WASM initialization
    state.game_over = false;
    state.score = 0;
    state.frame_count = 0;
    
    state.player_x = 3;
    state.player_y = SCREEN_HEIGHT / 2.0f;
    state.player_velocity_y = 0;

    // Initialize obstacles
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        spawn_obstacle(state.obstacles[i], SCREEN_WIDTH + i * OBSTACLE_SPACING);
    }

    clear_screen(state);
    draw_player(state);
    draw_obstacles(state);
#ifdef __EMSCRIPTEN__
    render_screen(state);
    js_update_score(state.score);
#endif
}

void update_game(GameState& state, bool jump_button_pressed) {
    if (state.game_over) {
        if (jump_button_pressed) {
            init_game(state);
        }
        return;
    }

    state.frame_count++;

    // --- Player Logic ---
    // 1. Handle Input
    if (jump_button_pressed && state.player_y >= SCREEN_HEIGHT - 2) {
        state.player_velocity_y = JUMP_FORCE;
    }

    // 2. Apply physics
    state.player_velocity_y += GRAVITY;
    state.player_y += state.player_velocity_y;

    // 3. Handle boundaries
    if (state.player_y >= SCREEN_HEIGHT - 1) {
        state.player_y = SCREEN_HEIGHT - 1;
        state.player_velocity_y = 0;
    }
    if (state.player_y < 0) {
        state.player_y = 0;
        state.player_velocity_y = 0;
    }
    
    // --- Obstacle Logic ---
    update_obstacles(state);

    // --- Score Logic ---
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        // プレイヤーが障害物を通過し、かつまだスコアが加算されていない場合
        if (!state.obstacles[i].scored && (state.obstacles[i].x + OBSTACLE_WIDTH < state.player_x)) {
            state.score++;
            state.obstacles[i].scored = true;
#ifdef __EMSCRIPTEN__
            js_update_score(state.score); // スコア更新をJS側に通知
#endif
        }
    }
    
    // --- Collision Detection ---
    check_collision(state);
    // ゲームオーバーになったら、その時点でのスコアを再度送信して最終スコアとして確定させる
    if (state.game_over) {
#ifdef __EMSCRIPTEN__
        js_update_score(state.score);
#endif
    }


    // --- Update screen buffer and render ---
    clear_screen(state);
    draw_obstacles(state);
    draw_player(state);
#ifdef __EMSCRIPTEN__
    render_screen(state);
#endif
}
