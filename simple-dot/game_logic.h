#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdint.h>

#define SCREEN_WIDTH 16
#define SCREEN_HEIGHT 16

#define MAX_OBSTACLES 2

struct Obstacle {
    float x;
    int gap_y;
    int gap_size;
    bool scored; // ★ 追加
};

struct GameState {
    // 0: Black, 1: Red, 2: Green, 3: Yellow, 4: Blue, 5: Magenta, 6: Cyan, 7: White
    uint8_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];
    bool game_over;
    int score;

    // Player
    int player_x;
    float player_y;
    float player_velocity_y;

    // Obstacles
    int frame_count;
    Obstacle obstacles[MAX_OBSTACLES];
};

#ifdef __cplusplus
extern "C" {
#endif

void init_game(GameState& state);
void update_game(GameState& state, bool jump_button_pressed);

#ifdef __cplusplus
}
#endif

#endif // GAME_LOGIC_H