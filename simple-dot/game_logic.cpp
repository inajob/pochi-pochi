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
void render_screen(GameState& state) {
    for (int r = 0; r < SCREEN_HEIGHT; ++r) {
        for (int c = 0; c < SCREEN_WIDTH; ++c) {
            js_draw_pixel(c, r, state.screen[r][c]);
        }
    }
}
#endif

// --- Game Constants ---
const int BACKGROUND_COLOR = 0;


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



void draw_score(GameState& state, int x, int y, int color) {
    char score_str[4];
    sprintf(score_str, "%d", state.score);
    int text_width = strlen(score_str) * 6 - 1;
    int start_x = x - text_width / 2; // Center horizontally at x
    draw_text(state, score_str, start_x, y, color);
}



#include "game_jump.h"
#include "game_chase.h"

// --- Public API Functions ---

void init_game(GameState& state) {
    state.phase = PHASE_TITLE;
    state.current_game = GAME_JUMP;
    state.button_down_frames = 0;
    state.text_scroll_offset = SCREEN_WIDTH;
}

void update_game(GameState& state, bool jump_button_pressed) {
    clear_screen(state);

    const int LONG_PRESS_FRAMES = 20; // ~1/3 second for a long press

    switch (state.phase) {
        case PHASE_TITLE: {
            if (jump_button_pressed) {
                state.button_down_frames++;
            } else {
                // Button was released
                if (state.button_down_frames > 0 && state.button_down_frames < LONG_PRESS_FRAMES) {
                    // It was a short press, switch game
                    state.current_game = (GameSelection)((state.current_game + 1) % NUM_GAMES);
                }
                state.button_down_frames = 0;
            }

            // Check for a long press to start the game
            if (state.button_down_frames >= LONG_PRESS_FRAMES) {
                switch(state.current_game) {
                    case GAME_JUMP:
                        init_jump_game(state); // This will set phase to PLAYING
                        break;
                    case GAME_CHASE:
                        init_chase_game(state); // This will set phase to PLAYING
                        break;
                }
                // Reset button press counter after starting a game
                state.button_down_frames = 0;
                // The game's own init function handles the phase change.
                // The switch below will then immediately start running the game logic.
            }

            // Draw the title for the currently selected game
            // (unless we just started one)
            if (state.phase == PHASE_TITLE) {
                 switch(state.current_game) {
                    case GAME_JUMP:
                        draw_jump_title(state);
                        break;
                    case GAME_CHASE:
                        draw_chase_title(state);
                        break;
                }
            }
            break;
        }

        case PHASE_COUNTDOWN:
        case PHASE_PLAYING:
        case PHASE_GAME_OVER: {
            switch(state.current_game) {
                case GAME_JUMP:
                    update_jump_game(state, jump_button_pressed);
                    break;
                case GAME_CHASE:
                    update_chase_game(state, jump_button_pressed);
                    break;
            }
            break;
        }
    }

#ifdef __EMSCRIPTEN__
    render_screen(state);
#endif
}
