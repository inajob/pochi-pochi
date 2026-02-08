#ifndef GAME_BRIGHTNESS_H
#define GAME_BRIGHTNESS_H

#include "game_logic.h"

// --- Brightness Game Class ---
class BrightnessGame : public IGame {
public:
    BrightnessGame(GameState& state);
    ~BrightnessGame() = default;

    bool update(GameState& state, bool button_pressed) override;
    void draw_title(GameState& state) override;

private:
    // Constants for brightness levels
    static const uint8_t BRIGHTNESS_LEVELS[];
    static const int NUM_BRIGHTNESS_LEVELS;

    int m_frame_counter;
    int m_display_hold_timer; // To keep the BRT value on screen for a bit
    int m_current_brightness_index; // Index into BRIGHTNESS_LEVELS
};

#endif // GAME_BRIGHTNESS_H
