// Core game logic is separated into game_logic.h and game_logic.cpp
#include "game_logic.h"

// NeoPixel Matrix Libraries
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

// --- Hardware Configuration ---
#define PIN 6 // NeoPixel data pin
#define JUMP_BUTTON_PIN 2 // Use pin 2 for jump

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(16, 16, PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

// --- Global Game State ---
GameState gameState;

// --- Helper function to convert game color index to NeoPixel color ---
uint16_t getColorFromIndex(uint8_t index) {
  switch (index) {
    case 0: return matrix.Color(0, 0, 0);         // 0: Black
    case 1: return matrix.Color(255, 0, 0);       // 1: Red
    case 2: return matrix.Color(0, 255, 0);       // 2: Green
    case 3: return matrix.Color(255, 255, 0);     // 3: Yellow
    case 4: return matrix.Color(0, 0, 255);       // 4: Blue
    case 5: return matrix.Color(255, 0, 255);     // 5: Magenta
    case 6: return matrix.Color(0, 255, 255);     // 6: Cyan
    case 7: return matrix.Color(255, 255, 255);   // 7: White
    default: return matrix.Color(0, 0, 0);
  }
}

// --- Arduino Setup ---
void setup() {
  matrix.begin();
  matrix.setBrightness(16);

  // Set up the jump button with an internal pull-up resistor
  pinMode(JUMP_BUTTON_PIN, INPUT_PULLUP);
  
  // Use a disconnected analog pin for a random seed
  randomSeed(analogRead(0));

  // Initialize the game state
  init_game(gameState);
}

// --- Arduino Loop ---
void loop() {
  // 1. Read Input
  // Button is active-low, so digitalRead is LOW when pressed.
  bool jump_pressed = !digitalRead(JUMP_BUTTON_PIN);

  // 2. Update Game State
  // The core game logic is handled by this function.
  update_game(gameState, jump_pressed);

  // 3. Render the screen
  // Loop through the game state's screen buffer and draw to the matrix.
  for (int r = 0; r < SCREEN_HEIGHT; ++r) {
    for (int c = 0; c < SCREEN_WIDTH; ++c) {
      uint16_t color = getColorFromIndex(gameState.screen[r][c]);
      matrix.drawPixel(c, r, color);
    }
  }
  matrix.show(); // Update the display with the new data

  // 4. Delay to control frame rate
  delay(50); // Approximately 14 FPS
}