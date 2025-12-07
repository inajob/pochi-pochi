#ifndef GAME_CHASE_H
#define GAME_CHASE_H

#include "game_logic.h"

// Functions for the Chase Game
void init_chase_game(GameState& state);
void update_chase_game(GameState& state, bool button_pressed);
void draw_chase_title(GameState& state);

#endif // GAME_CHASE_H
