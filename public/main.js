// --- DOM Elements ---
const gridContainer = document.getElementById('grid-container');
const jumpButton = document.getElementById('jump-button');


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

    // --- Event Listeners ---
    const press = (e) => {
        e.preventDefault();
        jump_button_pressed = true;
    };
    const release = (e) => {
        e.preventDefault();
        jump_button_pressed = false;
    };

    // Mouse events
    jumpButton.addEventListener('mousedown', press);
    gridContainer.addEventListener('mousedown', press);
    jumpButton.addEventListener('mouseup', release);
    gridContainer.addEventListener('mouseup', release);
    jumpButton.addEventListener('mouseleave', release);
    gridContainer.addEventListener('mouseleave', release);

    // Touch events
    jumpButton.addEventListener('touchstart', press);
    gridContainer.addEventListener('touchstart', press);
    jumpButton.addEventListener('touchend', release);
    gridContainer.addEventListener('touchend', release);
    jumpButton.addEventListener('touchcancel', release);
    gridContainer.addEventListener('touchcancel', release);

    // Keyboard events
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
    //requestAnimationFrame(gameLoop);
    setTimeout(gameLoop, 25)
}

// --- Start the engine when WASM is ready ---
var Module = {
    onRuntimeInitialized: function() {
        console.log("WASM Runtime Initialized.");
        init(); // Initialize DOM elements and event listeners
        

        // Wrap C++ functions
        init_game_wasm = Module.cwrap('init_game', null, ['number']);
        update_game_wasm = Module.cwrap('update_game', null, ['number', 'boolean']);
        set_initial_game_wasm = Module.cwrap('set_initial_game', null, ['number']);

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
        set_initial_game_wasm(gameStatePtr); // Set initial game to JUMP

        console.log("Game initialized. Starting loop.");
        gameLoop(); // Start the game loop
    }
};