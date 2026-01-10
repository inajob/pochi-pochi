#ifndef GAME_JUMP_H
#define GAME_JUMP_H

#include "game_logic.h"

struct JumpGameState {
    int player_x;
    float player_y;
    float player_velocity_y;
    Obstacle obstacles[MAX_OBSTACLES];
};


// Functions for the Jump Game
void init_jump_game(GameState& state);
void update_jump_game(GameState& state, bool button_pressed);
void draw_jump_title(GameState& state);

#endif // GAME_JUMP_H
