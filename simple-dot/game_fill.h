#ifndef GAME_FILL_H
#define GAME_FILL_H

#include "game_logic.h"

// --- Internal Phase for the Fill Game ---
enum FillGamePhase {
    FILL_PHASE_COUNTDOWN,
    FILL_PHASE_PLAYING,
    FILL_PHASE_GAMEOVER
};

// --- Fill Game Class ---
class FillGame : public IGame {
public:
    FillGame(GameState& state);
    ~FillGame() = default;

    bool update(GameState& state, bool button_pressed) override;
    void draw_title(GameState& state) override;

private:
    // Game-specific state
    FillGamePhase m_phase;
    int m_player_x;
    int m_player_move_timer;
    int m_playfield_shift_timer;
    int m_line_clear_timer;
    int m_line_clear_y;
    bool m_projectile_active;
    int m_projectile_x;
    int m_projectile_y;
    uint8_t m_playfield[SCREEN_HEIGHT][SCREEN_WIDTH / 8]; // Bit-packed: 1 bit per pixel
    int m_frame_counter;
    int m_current_playfield_shift_speed;
    int m_current_player_move_speed;
    int m_num_gaps_per_row;
    int m_next_difficulty_score_threshold;
    int m_difficulty_level;

    // Private helper methods for bit-packed playfield access
    bool get_pixel_status(int r, int c);
    void set_pixel_status(int r, int c, bool status);

    void generate_new_top_row();
};


#endif // GAME_FILL_H
