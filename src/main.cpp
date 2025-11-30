#include <utility>
#include <vector>
#include <cstdlib>  // for rand() and srand()
#include <ctime>    // for time()
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>


static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 900;

// GRID WIDTH
const int GRID_WIDTH = 800;
const int cols = 4;
const int tileWidth = GRID_WIDTH / cols;
// GRID HEIGHT
const int GRID_HEIGHT = 800;
const int rows = 4;
const int tileHeight = GRID_HEIGHT / rows;

typedef struct{
    std::vector<int> grid; // flat vector for contiguous memory = faster access
    int score;
    int high_score;
} GameContext;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    GameContext game_ctx;
    Uint64 last_step;
} AppState;

// Helper function to get a random empty cell index
// Returns -1 if no empty cells found
int GetRandomEmptyCell(const std::vector<int>& grid)
{
    // First, count how many empty cells (value 0) we have
    int emptyCount = 0;
    for (int i = 0; i < (int)grid.size(); i++) {
        if (grid[i] == 0) {
            emptyCount++;
        }
    }
    
    if (emptyCount == 0) {
        return -1; // No empty cells
    }
    
    // Pick a random empty cell
    int targetIndex = rand() % emptyCount;
    int currentIndex = 0;
    
    // Find the targetIndex-th empty cell
    for (int i = 0; i < (int)grid.size(); i++) {
        if (grid[i] == 0) {
            if (currentIndex == targetIndex) {
                return i;
            }
            currentIndex++;
        }
    }
    
    std::unreachable(); // Should never reach here
}

void InitGame(AppState *as)
{
    // Initialize random number generator with current time
    // This ensures we get different random numbers each time we run
    srand((unsigned int)time(NULL));
    
    // Initialize the game context
    GameContext& ctx = as->game_ctx;  // Reference - like an alias, no copy made
    
    // Resize the grid vector to hold 16 cells (4x4 grid)
    // std::vector::resize() changes the size and fills with default value (0 for int)
    ctx.grid.resize(rows * cols, 0);
    
    // Initialize scores
    ctx.score = 0;
    ctx.high_score = 0;
    
    // Spawn 2 initial tiles at random positions (as per README)
    for (int i = 0; i < 2; i++) {
        int cellIndex = GetRandomEmptyCell(ctx.grid);
        if (cellIndex != -1) {
            // Start with value 2 (typical 2048 game)
            ctx.grid[cellIndex] = 2;
        }
    }
}
void UpdateGame(AppState *as)
{

}
// Helper function to get color for a tile value
// Returns RGBA color values based on tile number
void GetTileColor(int value, Uint8& r, Uint8& g, Uint8& b)
{
    // Default empty tile color (light gray)
    r = 205; g = 193; b = 180;
    
    if (value == 0) {
        // Empty cell - slightly darker
        r = 187; g = 173; b = 160;
    } else if (value == 2) {
        r = 238; g = 228; b = 218;  // Light beige
    } else if (value == 4) {
        r = 237; g = 224; b = 200;  // Slightly darker beige
    } else if (value == 8) {
        r = 242; g = 177; b = 121;  // Orange
    } else if (value == 16) {
        r = 245; g = 149; b = 99;   // Darker orange
    } else if (value == 32) {
        r = 246; g = 124; b = 95;   // Red-orange
    } else if (value == 64) {
        r = 246; g = 94; b = 59;    // Red
    } else if (value >= 128) {
        r = 237; g = 204; b = 97;   // Yellow for higher values
    }
}

void DrawGame(AppState *as)
{
    SDL_Renderer* renderer = as->renderer;
    const GameContext& ctx = as->game_ctx;  // const reference - we won't modify it
    
    // Step 1: Clear the screen with background color
    // SDL_SetRenderDrawColor sets the color for subsequent drawing operations
    SDL_SetRenderDrawColor(renderer, 250, 248, 239, 255);  // Light beige background (RGBA)
    SDL_RenderClear(renderer);  // Fill entire screen with current draw color
    
    // Step 2: Draw the grid background (the game board area)
    SDL_FRect gridRect;
    gridRect.x = 0.0f;
    gridRect.y = 0.0f;
    gridRect.w = (float)GRID_WIDTH;
    gridRect.h = (float)GRID_HEIGHT;
    
    // Set color for grid background
    SDL_SetRenderDrawColor(renderer, 187, 173, 160, 255);  // Dark beige
    SDL_RenderFillRect(renderer, &gridRect);  // Fill the rectangle
    
    // Step 3: Draw grid lines to separate cells
    SDL_SetRenderDrawColor(renderer, 150, 140, 130, 255);  // Darker gray for lines
    
    // Draw vertical lines
    for (int i = 1; i < cols; i++) {
        float x = (float)(i * tileWidth);
        SDL_RenderLine(renderer, x, 0.0f, x, (float)GRID_HEIGHT);
    }
    
    // Draw horizontal lines
    for (int i = 1; i < rows; i++) {
        float y = (float)(i * tileHeight);
        SDL_RenderLine(renderer, 0.0f, y, (float)GRID_WIDTH, y);
    }
    
    // Step 4: Draw each tile
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            // Calculate index in flat vector: row * cols + col
            // This converts 2D coordinates to 1D array index
            int index = row * cols + col;
            int value = ctx.grid[index];
            
            // Skip drawing if cell is empty (value 0)
            if (value == 0) {
                continue;  // Skip to next iteration of loop
            }
            
            // Calculate position of this tile
            SDL_FRect tileRect;
            tileRect.x = (float)(col * tileWidth) + 5.0f;   // Small padding (5px)
            tileRect.y = (float)(row * tileHeight) + 5.0f;
            tileRect.w = (float)tileWidth - 10.0f;          // Subtract padding from both sides
            tileRect.h = (float)tileHeight - 10.0f;
            
            // Get color for this tile value
            Uint8 r, g, b;
            GetTileColor(value, r, g, b);
            
            // Draw the tile
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderFillRect(renderer, &tileRect);
        }
    }
    
    // Step 5: Present the rendered frame to the screen
    // This actually displays everything we've drawn
    SDL_RenderPresent(renderer);
}



// SDL STUFF
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    SDL_SetAppMetadata("2048 Game made with SDL 3", "0.0.1", "com.siekwie.2048Game");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) {
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("2048", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_free(as);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Now set the window and renderer in AppState after they're created
    as->window = window;
    as->renderer = renderer;
    *appstate = as;
    
    // Initialize the game
    InitGame(as);


    as->last_step = SDL_GetTicks();
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    // Cast void* to AppState* - we know it's actually an AppState pointer
    AppState *as = (AppState *)appstate;
    UpdateGame(as);
    DrawGame(as);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // SDL_AppEvent is called once per event, no need to poll in a loop
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
        default:
            break;
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}
void SDL_AppQuit(void *appstate, SDL_AppResult result){
    if (appstate != NULL) {
        AppState *as = (AppState *)appstate;
        if (as->renderer) {
            SDL_DestroyRenderer(as->renderer);
        }
        if (as->window) {
            SDL_DestroyWindow(as->window);
        }
        SDL_free(as);
    }
    // SDL_Quit() will be called automatically by SDL, but it's safe to call it here too
    SDL_Quit();
}