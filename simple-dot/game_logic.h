#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdint.h>

#define SCREEN_WIDTH 16
#define SCREEN_HEIGHT 16

// --- Forward declaration for GameState ---
struct GameState;

// --- Abstract Base Class for Games ---
class IGame {
public:
    virtual ~IGame() = default;

    // Main update function for a game. Returns true if it wants to exit to title.
    virtual bool update(GameState& state, bool button_pressed) = 0;

    // Draws the game-specific title screen.
    virtual void draw_title(GameState& state) = 0;
};


// --- Core Enums ---
enum GamePhase {
    PHASE_TITLE,
    // The individual game objects will now manage their own internal phases
    // like countdown, playing, gameover.
};

enum GameSelection {
    GAME_JUMP,
    GAME_CHASE,
    GAME_FILL
};
const int NUM_GAMES = 3;


// --- Main Game State ---
struct GameState {
    // Screen buffer & core state
    uint8_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];
    GamePhase phase;
    GameSelection current_selection;
    IGame* game_instance;

    // Input and generic state
    int button_down_frames;
    bool was_button_pressed_last_frame;
    bool long_press_action_taken;
    bool ignore_input_until_release;
    int score;
    int frame_count;
    float text_scroll_offset;
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