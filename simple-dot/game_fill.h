#ifndef GAME_FILL_H
#define GAME_FILL_H

#include "game_logic.h"

struct FillGameState {
    int player_x;
    int player_move_timer;
    int playfield_shift_timer;
    int line_clear_timer;
    int line_clear_y;
    bool projectile_active;
    int projectile_x;
    int projectile_y;
    uint8_t playfield[SCREEN_HEIGHT][SCREEN_WIDTH];
};

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
