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
#include "game_fill.h"

// --- Game Function Pointer Table ---
GameVTable game_vtable[NUM_GAMES] = {
    { .init = init_jump_game, .update = update_jump_game, .draw_title = draw_jump_title }, // GAME_JUMP
    { .init = init_chase_game, .update = update_chase_game, .draw_title = draw_chase_title }, // GAME_CHASE
    { .init = init_fill_game, .update = update_fill_game, .draw_title = draw_fill_title } // GAME_FILL
};


// --- Public API Functions ---

void init_game(GameState& state) {
    state.phase = PHASE_TITLE;
    state.button_down_frames = 0;
    state.text_scroll_offset = SCREEN_WIDTH;
    state.was_button_pressed_last_frame = false;
    state.long_press_action_taken = false;
    state.ignore_input_until_release = true; // Ignore input until first release
}

void set_initial_game(GameState& state) {
    state.current_game = GAME_JUMP;
}


void update_game(GameState& state, bool jump_button_pressed) {
    clear_screen(state);

    const int LONG_PRESS_FRAMES = 20; // ~1/3 second for a long press

    switch (state.phase) {
        case PHASE_TITLE: {
            if (state.ignore_input_until_release) {
                // Wait for the button to be released after coming back to the title screen
                if (!jump_button_pressed) {
                    state.ignore_input_until_release = false;
                }
                // Crucially, skip all other input processing for this frame
                // Reset counters just in case they were left in a bad state by the old game phase
                state.button_down_frames = 0;
                state.long_press_action_taken = false;
            } else {
                // Normal title screen input processing
                if (jump_button_pressed) {
                    state.button_down_frames++;

                    // Check for a long press that hasn't been actioned yet
                    if (state.button_down_frames >= LONG_PRESS_FRAMES && !state.long_press_action_taken) {
                        // This is a long press, so SWITCH the game
                        state.current_game = (GameSelection)((state.current_game + 1) % NUM_GAMES);
                        state.long_press_action_taken = true; // Mark action as taken for this press
                    }
                } else { // Button was released
                    // Check if it was a short press (released before long press action was taken)
                    if (state.button_down_frames > 0 && !state.long_press_action_taken) {
                         // This is a short press, so START the game
                         game_vtable[state.current_game].init(state);
                    }
                    // Reset everything on release for the next press
                    state.button_down_frames = 0;
                    state.long_press_action_taken = false;
                }
            }

            // Draw the title for the currently selected game
            if (state.phase == PHASE_TITLE) { // Check phase again in case it changed to PLAYING
                 game_vtable[state.current_game].draw_title(state);
            }
            break;
        }

        case PHASE_COUNTDOWN:
        case PHASE_PLAYING:
        case PHASE_GAME_OVER: {
            game_vtable[state.current_game].update(state, jump_button_pressed);
            break;
        }
    }

    state.was_button_pressed_last_frame = jump_button_pressed;

#ifdef __EMSCRIPTEN__
    render_screen(state);
#endif
}