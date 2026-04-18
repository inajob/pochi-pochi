#ifndef GAME_JUMP_H
#define GAME_JUMP_H

#include "game_logic.h"

// --- Internal Phase for the Jump Game ---
enum JumpGamePhase {
    JUMP_PHASE_COUNTDOWN,
    JUMP_PHASE_PLAYING,
    JUMP_PHASE_GAMEOVER
};

// --- Data Structures ---
#ifndef OBSTACLE_DEFINED
#define OBSTACLE_DEFINED
#define MAX_OBSTACLES 2
struct Obstacle {
    float x;          // horizontal position
    int height;       // height of the wall from the floor
    bool scored;
};
#endif

// --- Jump Game Class ---
class JumpGame : public IGame {
public:
    JumpGame(GameState& state);
    ~JumpGame() = default;

    bool update(GameState& state, bool button_pressed) override;
    void draw_title(GameState& state) override;

private:
    // Game-specific state
    JumpGamePhase m_phase;
    int m_player_x;
    float m_player_y;
    float m_player_velocity_y;
    Obstacle m_obstacles[MAX_OBSTACLES];
    int m_frame_counter; // Internal frame counter
    float m_current_obstacle_speed;
    int m_current_min_obstacle_spacing;
    int m_current_max_obstacle_spacing;
    int m_current_obstacle_height_max;
    int m_next_difficulty_score_threshold;
    int m_difficulty_level;

    // Private helper methods
    void draw_player(GameState& state);
    void draw_obstacles(GameState& state);
    void update_obstacles();
    bool check_collision();
    void spawn_obstacle(Obstacle& obstacle, float x_pos);
};

#endif // GAME_JUMP_H
