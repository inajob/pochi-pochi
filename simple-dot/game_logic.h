#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdint.h>

#define SCREEN_WIDTH 16
#define SCREEN_HEIGHT 16

// --- Forward declaration for GameState ---
struct GameState;

// --- Core Enums ---
enum GamePhase {
    PHASE_TITLE,
    PHASE_COUNTDOWN,
    PHASE_PLAYING,
    PHASE_GAME_OVER
};

enum GameSelection {
    GAME_JUMP,
    GAME_CHASE,
    GAME_FILL
};
const int NUM_GAMES = 3;


// --- Game-specific data structures ---
// (Specific to Jump Game, but kept here for now to minimize refactoring)
#define MAX_OBSTACLES 2
struct Obstacle {
    float x;
    int gap_y;
    int gap_size;
    bool scored;
};

// --- Main Game State ---
struct GameState {
    // Screen buffer & core state
    uint8_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];
    GamePhase phase;
    GameSelection current_game;
    int button_down_frames;
    bool was_button_pressed_last_frame;
    bool long_press_action_taken;
    bool ignore_input_until_release;

    // Generic state
    int score;
    int frame_count;
    float text_scroll_offset;

    // Jump Game specific state
    int player_x;
    float player_y;
    float player_velocity_y;
    Obstacle obstacles[MAX_OBSTACLES];

    // Fill Game specific state
    int fill_player_x;
    int fill_player_move_timer;
    int fill_playfield_shift_timer;
    int fill_line_clear_timer;
    int fill_line_clear_y;
    bool fill_projectile_active;
    int fill_projectile_x;
    int fill_projectile_y;
    uint8_t fill_playfield[SCREEN_HEIGHT][SCREEN_WIDTH];
};

#ifdef __cplusplus
extern "C" {
#endif

// --- Core Functions (in game_logic.cpp) ---
void init_game(GameState& state);
void set_initial_game(GameState& state);
void update_game(GameState& state, bool jump_button_pressed);

// --- Drawing helpers (to be used by multiple games) ---
void clear_screen(GameState& state);
void draw_char(GameState& state, char c, int x, int y, int color);
void draw_text(GameState& state, const char* text, int start_x, int start_y, int color);
void draw_score(GameState& state, int x, int y, int color);


#ifdef __cplusplus
}
#endif

#endif // GAME_LOGIC_H