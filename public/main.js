// --- DOM Elements ---
const gridContainer = document.getElementById('grid-container');
const jumpButton = document.getElementById('jump-button');
const scoreDisplay = document.getElementById('score-display'); // ★ 追加

// --- Game Constants ---
const SCREEN_WIDTH = 16;
const SCREEN_HEIGHT = 16;
const PIXEL_COUNT = SCREEN_WIDTH * SCREEN_HEIGHT;

let jump_button_pressed = false;

// --- Pixel Elements ---
const pixels = [];

// --- WASM related variables ---
let gameStatePtr; // Pointer to GameState struct in WASM memory
let init_game_wasm;
let update_game_wasm;

// --- Rendering (called from C++ via EM_JS) ---
window.setPixelInGrid = function(x, y, color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        const pixelIndex = y * SCREEN_WIDTH + x;
        // This check is a small optimization to avoid unnecessary DOM manipulation
        if (pixels[pixelIndex].dataset.color !== color) {
            pixels[pixelIndex].className = `pixel color-${color}`;
            pixels[pixelIndex].dataset.color = color;
        }
    }
};

// --- Score Display (called from C++ via EM_JS) ---
window.updateScoreDisplay = function(score) {
    if (scoreDisplay) {
        scoreDisplay.textContent = `Score: ${score}`;
    }
};

// --- Initialization ---
function init() {
    // Create pixel grid
    for (let i = 0; i < PIXEL_COUNT; i++) {
        const pixel = document.createElement('div');
        const row = Math.floor(i / SCREEN_WIDTH);
        const col = i % SCREEN_WIDTH;
        pixel.id = `pixel-${row}-${col}`;
        pixel.className = 'pixel color-0';
        pixel.dataset.color = 0;
        gridContainer.appendChild(pixel);
        pixels.push(pixel);
    }

    // Add event listeners
    jumpButton.addEventListener('mousedown', () => {
        jump_button_pressed = true;
    });
    jumpButton.addEventListener('mouseup', () => {
        jump_button_pressed = false;
    });
    jumpButton.addEventListener('mouseleave', () => { // In case the user drags the mouse away
        jump_button_pressed = false;
    });
    document.addEventListener('keydown', (e) => {
        if (e.code === 'Space') {
            e.preventDefault(); // Prevent page scroll
            jump_button_pressed = true;
        }
    });
    document.addEventListener('keyup', (e) => {
        if (e.code === 'Space') {
            jump_button_pressed = false;
        }
    });
}

// --- Game Loop ---
function gameLoop() {
    // 1. Update game state and render via WASM
    // The C++ update_game function now also calls render_screen,
    // which in turn calls window.setPixelInGrid for each pixel.
    update_game_wasm(gameStatePtr, jump_button_pressed);

    // The jump_button_pressed flag is now reset by keyup/mouseup events.

    // 2. Request next frame
    requestAnimationFrame(gameLoop);
}

// --- Start the engine when WASM is ready ---
var Module = {
    onRuntimeInitialized: function() {
        console.log("WASM Runtime Initialized.");
        init(); // Initialize DOM elements and event listeners
        

        // Wrap C++ functions
        init_game_wasm = Module.cwrap('init_game', null, ['number']);
        update_game_wasm = Module.cwrap('update_game', null, ['number', 'boolean']);

        // Allocate memory for GameState struct on WASM heap.
        // Even though we don't access it from JS, the C++ code needs it.
        // The exact size is not critical from JS side, but it must be large enough.
        // 256 bytes for screen + ~64 for other members.
        const SIZEOF_GAMESTATE = 320; 
        gameStatePtr = Module._malloc(SIZEOF_GAMESTATE);
        
        if (!gameStatePtr) {
            console.error("Failed to allocate WASM memory for GameState.");
            return;
        }

        // Initialize game state in WASM (which will also do the initial render)
        init_game_wasm(gameStatePtr);

        console.log("Game initialized. Starting loop.");
        gameLoop(); // Start the game loop
    }
};