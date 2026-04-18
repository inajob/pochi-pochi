#ifndef GAME_UFO_H
#define GAME_UFO_H

#include "game_logic.h"

// --- Internal Phase for the UFO Game ---
enum UfoGamePhase {
    UFO_PHASE_COUNTDOWN,
    UFO_PHASE_MOVING_HORIZONTAL,
    UFO_PHASE_DESCENDING,
    UFO_PHASE_ASCENDING,
    UFO_PHASE_GAMEOVER
};

// --- Prize Structure ---
struct Prize {
    int x;
    int width;
    int points;
    bool caught;
    uint8_t color;
};

// --- UFO Game Class ---
class UfoGame : public IGame {
public:
    UfoGame(GameState& state);
    ~UfoGame() = default;

    bool update(GameState& state, bool button_pressed) override;
    void draw_title(GameState& state) override;

private:
    UfoGamePhase m_phase;
    float m_arm_x;
    float m_arm_y;
    float m_arm_speed;
    int m_frame_counter;
    Prize m_prizes[4];
    int m_num_prizes;
    int m_caught_prize_index;
    int m_success_count;
    int m_display_score; // For animation
    int m_target_score;  // For animation

    void init_prizes();
    void draw_arm(GameState& state);
    void draw_prizes(GameState& state);
    void check_catch(GameState& state);
};

#endif // GAME_UFO_H
