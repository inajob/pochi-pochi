#include "game_brightness.h"
#include <string.h>
#include <stdio.h> // For sprintf

// --- Game Constants ---
const uint8_t BrightnessGame::BRIGHTNESS_LEVELS[] = {0, 4, 8, 16, 32, 64, 128, 255};
const int BrightnessGame::NUM_BRIGHTNESS_LEVELS = sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]);

const int BRIGHTNESS_DISPLAY_HOLD_FRAMES = 90; // Hold display for 1.5 seconds

// --- Constructor ---
BrightnessGame::BrightnessGame(GameState& state) {
    state.score = 0; // Not used, but reset
    state.frame_count = 0; // Not used
    m_frame_counter = 0;
    m_display_hold_timer = BRIGHTNESS_DISPLAY_HOLD_FRAMES;

    // Find current brightness index
    m_current_brightness_index = 0;
    for(int i = 0; i < NUM_BRIGHTNESS_LEVELS; ++i) {
        if (BRIGHTNESS_LEVELS[i] == state.current_brightness) {
            m_current_brightness_index = i;
            break;
        }
    }
}

// --- Public Methods ---

void BrightnessGame::draw_title(GameState& state) {
    const char* title_text = "BRIGHT";
    const char* title_text2 = "NESS";

    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -(float)strlen(title_text) * 6) {
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    draw_text(state, title_text, (int)state.text_scroll_offset, 2, 7); // White
    draw_text(state, title_text2, (int)state.text_scroll_offset, 8, 7); // White
}


bool BrightnessGame::update(GameState& state, bool button_pressed) {
    m_frame_counter++;

    // Display current brightness for a duration, then show title
    if (m_display_hold_timer > 0) {
        m_display_hold_timer--;
        // Draw brightness value
        char bright_str[8];
        sprintf(bright_str, "L%d", m_current_brightness_index);
        draw_text(state, bright_str, (SCREEN_WIDTH - strlen(bright_str) * 6) / 2, 5, 7); // Centered, white
    } else {
        // After hold, revert to title-like behavior (e.g. show "BRIGHTNESS" scrolling)
        draw_title(state); // Show scrolling title when not actively displaying level
    }


    // Input handling
    if (button_pressed && !state.was_button_pressed_last_frame) {
        m_display_hold_timer = BRIGHTNESS_DISPLAY_HOLD_FRAMES; // Reset timer on input

        // Short press: cycle brightness level
        m_current_brightness_index = (m_current_brightness_index + 1) % NUM_BRIGHTNESS_LEVELS;
        state.current_brightness = BRIGHTNESS_LEVELS[m_current_brightness_index];
    }
    
    // Check if user wants to exit (long press)
    const int LONG_PRESS_FRAMES = 20; // Re-use constant from game_logic.cpp
    if (button_pressed) {
        state.button_down_frames++;
        if (state.button_down_frames >= LONG_PRESS_FRAMES && !state.game_switched_on_long_press) {
            // Long press: exit to title
            state.game_switched_on_long_press = true; // Mark action as taken
            return true; // Signal to return to title
        }
    } else { // Button released
        state.button_down_frames = 0;
        state.game_switched_on_long_press = false;
    }


    return false; // Stay in brightness game
}