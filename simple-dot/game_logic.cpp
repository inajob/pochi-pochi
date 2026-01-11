#include "game_logic.h"
#include "font.h" // Include the new font definition file
#include <string.h>
#include <stdio.h> // For sprintf
#include <stdlib.h> // For rand()

// Include the new class-based game headers
#include "game_jump.h"
#include "game_chase.h"
#include "game_fill.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// --- Factory Function ---
// This is the only place with a switch statement for game types.
IGame* create_game_instance(GameSelection selection, GameState& state) {
    switch(selection) {
        case GAME_JUMP: return new JumpGame(state);
        case GAME_CHASE: return new ChaseGame(state);
        case GAME_FILL: return new FillGame(state);
    }
    return nullptr; // Should not happen
}


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
    if (c >= '0' && c <= '9') char_index = c - '0';
    else if (c >= 'A' && c <= 'Z') char_index = c - 'A' + 10;
    else if (c >= 'a' && c <= 'z') char_index = c - 'a' + 10;
    if (char_index != -1) {
        for (int r = 0; r < 5; ++r) {
            for (int col = 0; col < 5; ++col) {
                if ((font_5x5[char_index][r] >> (4 - col)) & 1) {
                    int px = x + col; int py = y + r;
                    if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT && px >= 0 && py >= 0)
                        state.screen[py][px] = color;
                }
            }
        }
    }
}
void draw_text(GameState& state, const char* text, int start_x, int start_y, int color) {
    int x = start_x;
    while (*text) { draw_char(state, *text, x, start_y, color); x += 6; text++; }
}
void draw_score(GameState& state, int x, int y, int color) {
    char score_str[4];
    sprintf(score_str, "%d", state.score);
    int text_width = strlen(score_str) * 6 - 1;
    draw_text(state, score_str, x - text_width / 2, y, color);
}


// --- Public API Functions ---

// Resets state for returning to the title screen, recreating the game instance
void init_game(GameState& state) {
    state.phase = PHASE_TITLE;
    state.button_down_frames = 0;
    state.text_scroll_offset = SCREEN_WIDTH; // Explicitly reset scroll for title screen
    state.was_button_pressed_last_frame = false;
    state.long_press_action_taken = false;
    state.ignore_input_until_release = true;

    // --- FIX: Re-create the game instance ---
    if (state.game_instance) { // Only delete if one exists
        delete state.game_instance;
    }
    state.game_instance = create_game_instance(state.current_selection, state);
}

// Sets up the very first game instance on startup
void set_initial_game(GameState& state) {
    state.current_selection = GAME_JUMP;
    state.game_instance = create_game_instance(state.current_selection, state);
    init_game(state); // Set initial phase to title
}


void update_game(GameState& state, bool button_pressed) {
    clear_screen(state);

    if (state.phase == PHASE_TITLE) {
        const int LONG_PRESS_FRAMES = 20;

        if (state.ignore_input_until_release) {
            if (!button_pressed) {
                state.ignore_input_until_release = false;
            }
            state.button_down_frames = 0;
            state.long_press_action_taken = false;
        } else {
            if (button_pressed) {
                state.button_down_frames++;
                if (state.button_down_frames >= LONG_PRESS_FRAMES && !state.long_press_action_taken) {
                    // Long press: switch game
                    state.current_selection = (GameSelection)((state.current_selection + 1) % NUM_GAMES);
                    delete state.game_instance;
                    state.game_instance = create_game_instance(state.current_selection, state);
                    state.long_press_action_taken = true;
                }
            } else { // Button was released
                if (state.button_down_frames > 0 && !state.long_press_action_taken) {
                    // Short press: start game
                    // The game object will transition its own internal state
                    // The main `phase` is no longer needed to track countdown/playing/gameover
                    state.phase = (GamePhase)1; // Any phase other than TITLE
                }
                state.button_down_frames = 0;
                state.long_press_action_taken = false;
            }
        }

        // Draw the title for the currently selected game
        if (state.game_instance) {
            state.game_instance->draw_title(state);
        }

    } else { // Game is in progress
        if (state.game_instance) {
            bool wants_to_return_to_title = state.game_instance->update(state, button_pressed);
            if (wants_to_return_to_title) {
                init_game(state); // Reset state for returning to title screen
            }
        }
    }

    state.was_button_pressed_last_frame = button_pressed;

#ifdef __EMSCRIPTEN__
    render_screen(state);
#endif
}