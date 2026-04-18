#include "game_ufo.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, js_update_score_ufo, (int score), {
    if (window.updateScoreDisplay) {
        window.updateScoreDisplay(score);
    }
});
#endif

// --- Game Constants ---
const int ARM_COLOR = 7; // White
const int ARM_WIDTH = 3;
const float BASE_ARM_SPEED = 0.1f;
const float SPEED_INCREMENT = 0.001f;
const float MAX_ARM_SPEED = 1.0f;

// --- Constructor ---
UfoGame::UfoGame(GameState& state) {
    state.score = 0;
    state.frame_count = 0;
    m_phase = UFO_PHASE_COUNTDOWN;
    m_frame_counter = 0;
    m_arm_x = 0;
    m_arm_y = 0;
    m_arm_speed = BASE_ARM_SPEED;
    m_success_count = 0;
    m_caught_prize_index = -1;
    m_display_score = 0;
    m_target_score = 0;
    init_prizes();
    
#ifdef __EMSCRIPTEN__
    js_update_score_ufo(state.score);
#endif
}

void UfoGame::init_prizes() {
    // Determine target number of prizes based on success count
    int base_count = 4 - (m_success_count / 5);
    if (base_count < 1) base_count = 1;
    int target_count = (base_count > 1) ? base_count : (1 + (rand() % 2));

    for (int attempt = 0; attempt < 30; ++attempt) {
        int widths[4];
        int total_width = 0;
        for (int i = 0; i < target_count; ++i) {
            widths[i] = 1 + (rand() % 3);
            total_width += widths[i];
        }

        // Need at least (target_count - 1) 1px-gaps
        int min_gaps = (target_count > 1) ? (target_count - 1) : 0;
        if (total_width + min_gaps > SCREEN_WIDTH) continue;

        // Extra space to distribute
        int extra_space = SCREEN_WIDTH - (total_width + min_gaps);
        
        // Randomly pick a starting offset (0 to extra_space)
        int current_x = rand() % (extra_space + 1);
        
        for (int i = 0; i < target_count; ++i) {
            m_prizes[i].x = current_x;
            m_prizes[i].width = widths[i];
            m_prizes[i].points = 4 - widths[i];
            m_prizes[i].caught = false;
            m_prizes[i].color = 1 + (rand() % 6);
            
            // Move to next position with a 1px gap plus some random extra spacing
            // if we have extra space left
            current_x += widths[i] + 1;
            if (extra_space > 0 && i < target_count - 1) {
                int add = rand() % 2; // Add 0 or 1px extra gap
                current_x += add;
            }
        }
        
        m_num_prizes = target_count;
        return;
    }

    // Fallback: simple tight placement if randomized logic fails
    m_num_prizes = 0;
    int cur_x = 0;
    for (int i = 0; i < target_count; ++i) {
        int w = 1 + (rand() % 2);
        if (cur_x + w > SCREEN_WIDTH) break;
        m_prizes[i].x = cur_x;
        m_prizes[i].width = w;
        m_prizes[i].points = 4 - w;
        m_prizes[i].caught = false;
        m_prizes[i].color = 1 + (rand() % 6);
        cur_x += w + 1;
        m_num_prizes++;
    }
}

void draw_ufo_score(GameState& state, int score, int x, int y, int color) {
    char score_str[4];
    sprintf(score_str, "%d", score);
    int text_width = strlen(score_str) * 6 - 1;
    draw_text(state, score_str, x - text_width / 2, y, color);
}

void UfoGame::draw_title(GameState& state) {
    const char* title_text = "UFO";
    state.text_scroll_offset -= 0.5f;
    if (state.text_scroll_offset < -(float)strlen(title_text) * 6) {
        state.text_scroll_offset = SCREEN_WIDTH;
    }
    draw_text(state, title_text, (int)state.text_scroll_offset, 5, 5); // Magenta title
}

bool UfoGame::update(GameState& state, bool button_pressed) {
    m_frame_counter++;

    // --- Score Animation Logic ---
    if (m_display_score < m_target_score) {
        if (m_frame_counter % 5 == 0) { // Increment every 5 frames
            m_display_score++;
        }
    }

    switch (m_phase) {
        case UFO_PHASE_COUNTDOWN: {
            draw_prizes(state);
            draw_arm(state);
            const int frames_per_number = 40;
            int number = 3 - (m_frame_counter / frames_per_number);
            if (number > 0) {
                draw_char(state, (char)('0' + number), 6, 5, 7); 
            }
            if (m_frame_counter >= frames_per_number * 3) {
                m_phase = UFO_PHASE_MOVING_HORIZONTAL;
                m_frame_counter = 0;
            }
            break;
        }

        case UFO_PHASE_MOVING_HORIZONTAL: {
            m_arm_x += m_arm_speed;
            if (m_arm_x >= SCREEN_WIDTH) m_arm_x = -ARM_WIDTH + 1;
            
            if (button_pressed && !state.was_button_pressed_last_frame) {
                m_phase = UFO_PHASE_DESCENDING;
                m_frame_counter = 0;
            }
            draw_prizes(state);
            draw_arm(state);
            break;
        }

        case UFO_PHASE_DESCENDING: {
            m_arm_y += 0.5f;
            if (m_arm_y >= SCREEN_HEIGHT - 1) {
                m_arm_y = SCREEN_HEIGHT - 1;
                check_catch(state);
                if (m_caught_prize_index != -1) {
                    m_phase = UFO_PHASE_ASCENDING;
                } else {
                    m_phase = UFO_PHASE_GAMEOVER;
                    state.text_scroll_offset = SCREEN_WIDTH;
                }
                m_frame_counter = 0;
            }
            draw_prizes(state);
            draw_arm(state);
            break;
        }

        case UFO_PHASE_ASCENDING: {
            m_arm_y -= 0.5f;
            if (m_arm_y <= 0) {
                m_arm_y = 0;
                m_success_count++;
                m_arm_speed = BASE_ARM_SPEED + (m_success_count * SPEED_INCREMENT);
                if (m_arm_speed > MAX_ARM_SPEED) m_arm_speed = MAX_ARM_SPEED;
                
                // Show score for a moment then reset
                if (m_frame_counter > 60) {
                    m_phase = UFO_PHASE_MOVING_HORIZONTAL;
                    m_caught_prize_index = -1;
                    init_prizes();
                    m_frame_counter = 0;
                }
            }
            draw_prizes(state);
            draw_arm(state);
            draw_ufo_score(state, m_display_score, SCREEN_WIDTH / 2, 6, 3);
            break;
        }

        case UFO_PHASE_GAMEOVER: {
            const char* game_text = "GAME";
            const char* over_text = "OVER";
            state.text_scroll_offset -= 0.5f;
            if (state.text_scroll_offset < -(float)strlen(game_text) * 6) {
                state.text_scroll_offset = SCREEN_WIDTH;
            }
            draw_text(state, game_text, (int)state.text_scroll_offset, 2, 1);
            draw_text(state, over_text, (int)state.text_scroll_offset, 8, 1);
            draw_ufo_score(state, m_display_score, SCREEN_WIDTH / 2, 10, 7);

            const int GAMEOVER_INPUT_DELAY_FRAMES = 30;
            if (button_pressed && !state.was_button_pressed_last_frame && m_frame_counter > GAMEOVER_INPUT_DELAY_FRAMES) {
                return true;
            }
            break;
        }
    }
    return false;
}

void UfoGame::draw_arm(GameState& state) {
    int ay = (int)m_arm_y;
    int ax = (int)m_arm_x;
    int mid_x = ax + 1;
    
    // Draw the "Inverted Y" claws at the bottom
    if (ay >= 0 && ay < SCREEN_HEIGHT) {
        if (ax >= 0 && ax < SCREEN_WIDTH) state.screen[ay][ax] = ARM_COLOR;
        if (ax + 2 >= 0 && ax + 2 < SCREEN_WIDTH) state.screen[ay][ax + 2] = ARM_COLOR;
    }
    if (ay - 1 >= 0 && ay - 1 < SCREEN_HEIGHT) {
        if (mid_x >= 0 && mid_x < SCREEN_WIDTH) state.screen[ay - 1][mid_x] = ARM_COLOR;
    }

    // Draw a single center cable from the top down to the joint
    if (ay > 1) {
        for (int y = 0; y <= ay - 2; ++y) {
           if (mid_x >= 0 && mid_x < SCREEN_WIDTH) {
               state.screen[y][mid_x] = ARM_COLOR;
           }
        }
    }
}

void UfoGame::draw_prizes(GameState& state) {
    for (int i = 0; i < m_num_prizes; ++i) {
        if (m_prizes[i].caught && m_phase != UFO_PHASE_ASCENDING) continue;
        
        int px = m_prizes[i].x;
        int py = (m_prizes[i].caught) ? (int)m_arm_y : SCREEN_HEIGHT - 1;
        
        for (int w = 0; w < m_prizes[i].width; ++w) {
            int wx = px + w;
            if (wx >= 0 && wx < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                state.screen[py][wx] = m_prizes[i].color;
            }
        }
    }
}

void UfoGame::check_catch(GameState& state) {
    int ax_start = (int)m_arm_x;
    int ax_end = ax_start + ARM_WIDTH - 1;
    
    for (int i = 0; i < m_num_prizes; ++i) {
        int px_start = m_prizes[i].x;
        int px_end = px_start + m_prizes[i].width - 1;
        
        // Overlap check
        if (ax_start <= px_end && ax_end >= px_start) {
            m_caught_prize_index = i;
            m_prizes[i].caught = true;
            // Align prize with arm for animation
            m_prizes[i].x = ax_start + (ARM_WIDTH - m_prizes[i].width) / 2;
            
            m_target_score += m_prizes[i].points;
            state.score = m_target_score;
        }
    }
#ifdef __EMSCRIPTEN__
    if (m_caught_prize_index != -1) {
        js_update_score_ufo(state.score);
    }
#endif
}
