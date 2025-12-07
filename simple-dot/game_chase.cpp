#include "game_chase.h"
#include <string.h>
#include <stdlib.h> // For rand()

// --- Chase Game Constants ---
const int PLAYER_Y_POS = 14;
const int LANE_POS[] = {4, 7, 10};
const int NUM_LANES = sizeof(LANE_POS) / sizeof(LANE_POS[0]);
const float WALL_SPEED = 0.2f;
const int WALL_SPACING = 8;
const int CHASE_PLAYER_COLOR = 2; // Green
const int CHASE_WALL_COLOR = 4;   // Blue

// --- Forward Declarations ---
void spawn_wall(Obstacle& wall, float y_pos);

// --- Game-specific Functions ---

void draw_chase_title(GameState& state) {
    const char* title_text = "CHASE";
    int text_width = strlen(title_text) * 6;
    
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -text_width) {
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, CHASE_PLAYER_COLOR);
}

void init_chase_game(GameState& state) {
    state.phase = PHASE_PLAYING;
    state.score = 0;
    state.frame_count = 0;
    
    // Use player_x to store current lane INDEX (0, 1, or 2)
    state.player_x = 1; // Start in the middle lane

    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        spawn_wall(state.obstacles[i], -i * WALL_SPACING);
    }
}

void spawn_wall(Obstacle& wall, float y_pos) {
    wall.x = y_pos; // Using .x for the Y-position of the wall
    wall.gap_y = rand() % NUM_LANES; // Using .gap_y for the gap's lane index
    wall.scored = false;
}

void update_chase_game(GameState& state, bool button_pressed) {
    switch (state.phase) {
        case PHASE_PLAYING: {
            // --- Handle Input ---
            if (button_pressed && !state.was_button_pressed_last_frame) {
                state.player_x = (state.player_x + 1) % NUM_LANES;
            }

            // --- Update Game State ---
            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                state.obstacles[i].x += WALL_SPEED; // Move wall down

                // Check for scoring
                if (!state.obstacles[i].scored && state.obstacles[i].x > PLAYER_Y_POS) {
                    state.score++;
                    state.obstacles[i].scored = true;
                }

                // Respawn wall if it's off-screen
                if (state.obstacles[i].x >= SCREEN_HEIGHT) {
                    spawn_wall(state.obstacles[i], 0);
                }
            }

            // --- Collision Detection ---
            int player_lane_x = LANE_POS[state.player_x];
            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                int wall_y = (int)state.obstacles[i].x;
                if (wall_y == PLAYER_Y_POS) {
                    int gap_lane = state.obstacles[i].gap_y;
                    if (state.player_x != gap_lane) {
                        state.phase = PHASE_GAME_OVER;
                        state.frame_count = 0; // For game over delay
                        state.text_scroll_offset = SCREEN_WIDTH;
                    }
                }
            }

            // --- Drawing ---
            // Draw walls
            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                int wall_y = (int)state.obstacles[i].x;
                if (wall_y >= 0 && wall_y < SCREEN_HEIGHT) {
                    int gap_lane_x = LANE_POS[state.obstacles[i].gap_y];
                    for (int x = 0; x < SCREEN_WIDTH; ++x) {
                        if (x != gap_lane_x) {
                            state.screen[wall_y][x] = CHASE_WALL_COLOR;
                        }
                    }
                }
            }
            // Draw player
            state.screen[PLAYER_Y_POS][player_lane_x] = CHASE_PLAYER_COLOR;
            break;
        }
        
        case PHASE_GAME_OVER: {
            state.frame_count++;

            // Scrolling "GAME OVER" text
            const char* game_text = "GAME";
            const char* over_text = "OVER";
            int text_width = strlen(game_text) * 6;

            state.text_scroll_offset -= 0.5f;
            if (state.text_scroll_offset < -text_width) {
                state.text_scroll_offset = SCREEN_WIDTH;
            }

            draw_text(state, game_text, (int)state.text_scroll_offset, 2, 1);
            draw_text(state, over_text, (int)state.text_scroll_offset, 8, 1);
            draw_score(state, SCREEN_WIDTH / 2, 10, 7); // Corrected Y position

            if (button_pressed && !state.was_button_pressed_last_frame && state.frame_count > 30) {
                init_game(state); // Go back to main title screen
            }
            break;
        }
        default:
            // PHASE_TITLE and PHASE_COUNTDOWN are handled by the main dispatcher,
            // so update_chase_game should not be called in these phases.
            // Do nothing for other unexpected phases.
            break;
    }
}
