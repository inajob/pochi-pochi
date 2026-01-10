#ifndef GAME_FILL_H
#define GAME_FILL_H

#include "game_logic.h"

#ifdef __cplusplus
extern "C" {
#endif

// Functions for the "Fill" game mode
void init_fill_game(GameState& state);
void update_fill_game(GameState& state, bool button_pressed);
void draw_fill_title(GameState& state);

#ifdef __cplusplus
}
#endif

#endif // GAME_FILL_H
