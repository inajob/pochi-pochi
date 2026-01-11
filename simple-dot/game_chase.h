#ifndef GAME_CHASE_H
#define GAME_CHASE_H

#include "game_logic.h"

// --- Internal Phase for the Chase Game ---
enum ChaseGamePhase {
    CHASE_PHASE_PLAYING,
    CHASE_PHASE_GAMEOVER
};

// --- Data Structures ---
// Note: This is the same Obstacle struct from Jump, but used differently.
// In a future refactor, this could be a more generic "GameObject".
#ifndef OBSTACLE_DEFINED
#define OBSTACLE_DEFINED
#define MAX_OBSTACLES 2
struct Obstacle {
    float x;
    int gap_y;
    int gap_size;
    bool scored;
};
#endif


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
    Obstacle m_walls[MAX_OBSTACLES];
    int m_frame_counter;

    // Private helper methods
    void spawn_wall(Obstacle& wall, float y_pos);
};


#endif // GAME_CHASE_H
