#ifndef GAME_CHASE_H
#define GAME_CHASE_H

#include "game_logic.h"

// --- Internal Phase for the Chase Game ---
enum ChaseGamePhase {
    CHASE_PHASE_PLAYING,
    CHASE_PHASE_GAMEOVER
};

// --- Data Structures ---
#define MAX_OBSTACLES 2
struct ChaseObstacle {
    float y_pos;          // vertical position of the wall
    int gap_lane_index;   // the lane where the gap is
    bool scored;
};

// --- Chase Game Class ---
class ChaseGame : public IGame {
public:
    ChaseGame(GameState& state);
    ~ChaseGame() = default;

    bool update(GameState& state, bool button_pressed) override;
    void draw_title(GameState& state) override;

private:
    // Game-specific state
    ChaseGamePhase m_phase;
    int m_player_lane_index;
    ChaseObstacle m_walls[MAX_OBSTACLES]; // Use ChaseObstacle
    int m_frame_counter;

    // Private helper methods
    void spawn_wall(ChaseObstacle& wall, float y_pos); // Update signature
};


#endif // GAME_CHASE_H
