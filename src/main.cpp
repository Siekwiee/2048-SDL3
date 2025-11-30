#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include <utility>
#include <vector>
#include <algorithm>  // for std::find_if, std::count_if
#include <cstdlib>   // for rand() and srand()
#include <ctime>      // for time()
#include <cstdio>     // for sprintf
#include <cstring>    // for strlen
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 900;

// GRID CONSTANTS
const int GRID_WIDTH = 800;
const int GRID_HEIGHT = 800;
const int GRID_COLS = 4;
const int GRID_ROWS = 4;
const float TILE_PADDING = 5.0f;

// Text scaling factor - makes text 2.5x larger (8px * 2.5 = 20px)
// Reduced from 3.0 to prevent overlap
const float TEXT_SCALE = 2.5f;

// Calculate the width of text when rendered with scaling
// This helps position text dynamically to prevent overlap
float GetScaledTextWidth(const char* text, float scale = TEXT_SCALE) {
    // SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE is 8 pixels per character
    // With scaling, each character is 8 * scale pixels wide
    return (float)(strlen(text) * 8 * scale);
}

// Calculate the height of text when rendered with scaling
float GetScaledTextHeight(float scale = TEXT_SCALE) {
    // SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE is 8 pixels tall
    return 8.0f * scale;
}

// Helper function to render scaled text
// This temporarily scales the renderer, draws text, then restores the scale
void RenderScaledText(SDL_Renderer* renderer, float x, float y, const char* text, float scale = TEXT_SCALE) {
    // Save current render scale
    float oldScaleX, oldScaleY;
    SDL_GetRenderScale(renderer, &oldScaleX, &oldScaleY);
    
    // Set render scale to make text larger
    SDL_SetRenderScale(renderer, scale, scale);
    
    // Adjust coordinates for scaled rendering (divide by scale)
    float scaledX = x / scale;
    float scaledY = y / scale;
    
    // Draw the text
    SDL_RenderDebugText(renderer, scaledX, scaledY, text);
    
    // Restore original render scale
    SDL_SetRenderScale(renderer, oldScaleX, oldScaleY);
}

// Tile class - represents a single tile on the grid
class Tile {
public:
    int value;  // 0 means empty, otherwise the tile's number (2, 4, 8, etc.)
    int row;
    int col;
    
    // Constructor - initializes a tile at a position with a value
    Tile(int val = 0, int r = 0, int c = 0) : value(val), row(r), col(c) {}
    
    // Check if this tile is empty
    bool isEmpty() const { return value == 0; }
    
    // Get the color for this tile's value
    void getColor(Uint8& r, Uint8& g, Uint8& b) const {
        // Default empty tile color
        r = 205; g = 193; b = 180;
        
        if (value == 0) {
            r = 187; g = 173; b = 160;  // Empty cell
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
    
    // Get the rectangle for drawing this tile
    SDL_FRect getRect(float tileWidth, float tileHeight) const {
        SDL_FRect rect;
        rect.x = (float)(col * tileWidth) + TILE_PADDING;
        rect.y = (float)(row * tileHeight) + TILE_PADDING;
        rect.w = tileWidth - (TILE_PADDING * 2.0f);
        rect.h = tileHeight - (TILE_PADDING * 2.0f);
        return rect;
    }
    
    // Draw the tile's number text centered on the tile (scaled up)
    void drawText(SDL_Renderer* renderer, float tileWidth, float tileHeight) const {
        if (isEmpty()) {
            return;  // Don't draw text for empty tiles
        }
        
        // Convert number to string
        char text[32];
        snprintf(text, sizeof(text), "%d", value);
        
        // Calculate text position (centered on tile)
        SDL_FRect tileRect = getRect(tileWidth, tileHeight);
        
        // Calculate text dimensions using helper function
        float textWidth = GetScaledTextWidth(text);
        float textHeight = GetScaledTextHeight();
        
        // Center the text
        float textX = tileRect.x + (tileRect.w - textWidth) / 2.0f;
        float textY = tileRect.y + (tileRect.h - textHeight) / 2.0f;
        
        // Set text color - dark for light tiles, light for dark tiles
        if (value <= 4) {
            SDL_SetRenderDrawColor(renderer, 119, 110, 101, 255);  // Dark gray
        } else {
            SDL_SetRenderDrawColor(renderer, 249, 246, 242, 255);  // Light beige
        }
        
        // Draw the text using scaled rendering
        RenderScaledText(renderer, textX, textY, text);
    }
};

// Grid class - manages the 4x4 game grid
class Grid {
private:
    std::vector<Tile> tiles;
    int rows;
    int cols;
    float width;
    float height;
    float tileWidth;
    float tileHeight;
    
public:
    // Constructor - creates a grid with specified dimensions
    Grid(int r, int c, float w, float h) 
        : rows(r), cols(c), width(w), height(h) {
        tileWidth = width / cols;
        tileHeight = height / rows;
        
        // Initialize all tiles as empty
        tiles.reserve(rows * cols);
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                tiles.emplace_back(0, row, col);
            }
        }
    }
    
    // Get the grid rectangle for drawing the background
    SDL_FRect getRect() const {
        SDL_FRect rect;
        rect.x = 0.0f;
        rect.y = 0.0f;
        rect.w = width;
        rect.h = height;
        return rect;
    }
    
    // Get tile width and height
    float getTileWidth() const { return tileWidth; }
    float getTileHeight() const { return tileHeight; }
    
    // Convert 2D coordinates to 1D index
    int getIndex(int row, int col) const {
        return row * cols + col;
    }
    
    // Get tile at position (row, col)
    Tile& at(int row, int col) {
        return tiles[getIndex(row, col)];
    }
    
    const Tile& at(int row, int col) const {
        return tiles[getIndex(row, col)];
    }
    
    // Get tile by index
    Tile& at(int index) {
        return tiles[index];
    }
    
    const Tile& at(int index) const {
        return tiles[index];
    }
    
    // Get total number of cells
    size_t size() const {
        return tiles.size();
    }
    
    // Find a random empty cell index
    // Returns -1 if no empty cells found
    int findRandomEmptyCell() const {
        // Count empty cells using std::count_if
        int emptyCount = (int)std::count_if(tiles.begin(), tiles.end(),
            [](const Tile& tile) { return tile.isEmpty(); });
        
        if (emptyCount == 0) {
            return -1;
        }
        
        // Pick a random empty cell
        int targetIndex = rand() % emptyCount;
        int currentIndex = 0;
        
        // Find the targetIndex-th empty cell using std::find_if
        for (size_t i = 0; i < tiles.size(); i++) {
            if (tiles[i].isEmpty()) {
                if (currentIndex == targetIndex) {
                    return (int)i;
                }
                currentIndex++;
            }
        }
        
        std::unreachable();
    }
    
    // Spawn a tile at a random empty position
    bool spawnRandomTile(int value = 2) {
        int index = findRandomEmptyCell();
        if (index != -1) {
            tiles[index].value = value;
            return true;
        }
        return false;
    }
    
    // Restart the grid - clear all tiles
    void restart() {
        for (Tile& tile : tiles) {
            tile.value = 0;
        }
    }
    
    // Direction enum for tile movement
    enum Direction {
        UP,
        DOWN,
        LEFT,
        RIGHT
    };
    
    // Move and merge tiles in the specified direction
    // Returns true if any tiles moved or merged, false otherwise
    // mergeScore is updated with the total value of merged tiles
    bool orderTilesAndMerge(Direction dir, int& mergeScore) {
        mergeScore = 0;
        
        // Save original state to compare later
        std::vector<Tile> originalTiles = tiles;
        std::vector<Tile> newTiles = tiles;
        bool anyChange = false;
        
        // Process each row or column depending on direction
        if (dir == UP || dir == DOWN) {
            // Process each column
            for (int col = 0; col < cols; col++) {
                // Collect non-empty tiles with their original row positions
                std::vector<std::pair<int, int>> columnTiles; // (row, value)
                for (int row = 0; row < rows; row++) {
                    int idx = getIndex(row, col);
                    if (!originalTiles[idx].isEmpty()) {
                        columnTiles.push_back({row, originalTiles[idx].value});
                    }
                }
                
                if (columnTiles.empty()) continue;
                
                // For DOWN, we need to process from bottom to top for merging
                // So we reverse the order, merge, then reverse back
                std::vector<std::pair<int, int>> workingTiles = columnTiles;
                bool wasReversed = false;
                if (dir == DOWN) {
                    std::reverse(workingTiles.begin(), workingTiles.end());
                    wasReversed = true;
                }
                
                // Merge adjacent tiles with same value
                std::vector<int> mergedValues; // Just the values after merging
                for (size_t i = 0; i < workingTiles.size(); i++) {
                    if (i < workingTiles.size() - 1 && 
                        workingTiles[i].second == workingTiles[i + 1].second) {
                        // Merge tiles
                        int mergedValue = workingTiles[i].second * 2;
                        mergedValues.push_back(mergedValue);
                        mergeScore += mergedValue;
                        i++; // Skip next tile as it's been merged
                    } else {
                        // Keep tile as is
                        mergedValues.push_back(workingTiles[i].second);
                    }
                }
                
                // Reverse back to get correct order for placement
                if (wasReversed) {
                    std::reverse(mergedValues.begin(), mergedValues.end());
                }
                
                // Check if this column actually needs to change
                // Build what the final column should look like
                std::vector<int> finalColumn(rows, 0);
                for (size_t i = 0; i < mergedValues.size(); i++) {
                    int finalRow;
                    if (dir == UP) {
                        finalRow = (int)i;  // Place from top (0, 1, 2, ...)
                    } else { // DOWN
                        finalRow = rows - 1 - (int)i;  // Place from bottom (3, 2, 1, ...)
                    }
                    finalColumn[finalRow] = mergedValues[i];
                }
                
                // Compare with original column - check if anything actually changed
                bool columnChanged = false;
                for (int row = 0; row < rows; row++) {
                    int idx = getIndex(row, col);
                    if (finalColumn[row] != originalTiles[idx].value) {
                        columnChanged = true;
                        break;
                    }
                }
                
                // Only modify this column if it changed
                if (columnChanged) {
                    anyChange = true;
                    // Clear this column first
                    for (int row = 0; row < rows; row++) {
                        int idx = getIndex(row, col);
                        newTiles[idx].value = 0;
                    }
                    
                    // Place merged tiles at the edge (top for UP, bottom for DOWN)
                    for (size_t i = 0; i < mergedValues.size(); i++) {
                        int finalRow;
                        size_t valueIndex;
                        if (dir == UP) {
                            finalRow = (int)i;  // Place from top (0, 1, 2, ...)
                            valueIndex = i;
                        } else { // DOWN
                            finalRow = rows - 1 - (int)i;  // Place from bottom (3, 2, 1, ...)
                            valueIndex = mergedValues.size() - 1 - i;  // Reverse order for values
                        }
                        int targetIdx = getIndex(finalRow, col);
                        newTiles[targetIdx].value = mergedValues[valueIndex];
                        newTiles[targetIdx].row = finalRow;
                        newTiles[targetIdx].col = col;
                    }
                }
                // If column didn't change, leave newTiles[col] as is (already copied from original)
            }
        } else { // LEFT or RIGHT
            // Process each row
            for (int row = 0; row < rows; row++) {
                // Collect non-empty tiles with their original column positions
                std::vector<std::pair<int, int>> rowTiles; // (col, value)
                for (int col = 0; col < cols; col++) {
                    int idx = getIndex(row, col);
                    if (!originalTiles[idx].isEmpty()) {
                        rowTiles.push_back({col, originalTiles[idx].value});
                    }
                }
                
                if (rowTiles.empty()) continue;
                
                // For RIGHT, we need to process from right to left for merging
                // So we reverse the order, merge, then reverse back
                std::vector<std::pair<int, int>> workingTiles = rowTiles;
                bool wasReversed = false;
                if (dir == RIGHT) {
                    std::reverse(workingTiles.begin(), workingTiles.end());
                    wasReversed = true;
                }
                
                // Merge adjacent tiles with same value
                std::vector<int> mergedValues; // Just the values after merging
                for (size_t i = 0; i < workingTiles.size(); i++) {
                    if (i < workingTiles.size() - 1 && 
                        workingTiles[i].second == workingTiles[i + 1].second) {
                        // Merge tiles
                        int mergedValue = workingTiles[i].second * 2;
                        mergedValues.push_back(mergedValue);
                        mergeScore += mergedValue;
                        i++; // Skip next tile as it's been merged
                    } else {
                        // Keep tile as is
                        mergedValues.push_back(workingTiles[i].second);
                    }
                }
                
                // Reverse back to get correct order for placement
                if (wasReversed) {
                    std::reverse(mergedValues.begin(), mergedValues.end());
                }
                
                // Check if this row actually needs to change
                // Build what the final row should look like
                std::vector<int> finalRow(cols, 0);
                for (size_t i = 0; i < mergedValues.size(); i++) {
                    int finalCol;
                    if (dir == LEFT) {
                        finalCol = (int)i;  // Place from left (0, 1, 2, ...)
                    } else { // RIGHT
                        finalCol = cols - 1 - (int)i;  // Place from right (3, 2, 1, ...)
                    }
                    finalRow[finalCol] = mergedValues[i];
                }
                
                // Compare with original row - check if anything actually changed
                bool rowChanged = false;
                for (int col = 0; col < cols; col++) {
                    int idx = getIndex(row, col);
                    if (finalRow[col] != originalTiles[idx].value) {
                        rowChanged = true;
                        break;
                    }
                }
                
                // Only modify this row if it changed
                if (rowChanged) {
                    anyChange = true;
                    // Clear this row first
                    for (int col = 0; col < cols; col++) {
                        int idx = getIndex(row, col);
                        newTiles[idx].value = 0;
                    }
                    
                    // Place merged tiles at the edge (left for LEFT, right for RIGHT)
                    for (size_t i = 0; i < mergedValues.size(); i++) {
                        int finalCol;
                        size_t valueIndex;
                        if (dir == LEFT) {
                            finalCol = (int)i;  // Place from left (0, 1, 2, ...)
                            valueIndex = i;
                        } else { // RIGHT
                            finalCol = cols - 1 - (int)i;  // Place from right (3, 2, 1, ...)
                            valueIndex = mergedValues.size() - 1 - i;  // Reverse order for values
                        }
                        int targetIdx = getIndex(row, finalCol);
                        newTiles[targetIdx].value = mergedValues[valueIndex];
                        newTiles[targetIdx].row = row;
                        newTiles[targetIdx].col = finalCol;
                    }
                }
                // If row didn't change, leave newTiles[row] as is (already copied from original)
            }
        }
        
        // Only update if something changed
        if (anyChange) {
            tiles = newTiles;
            return true;
        }
        
        return false;
    }

    // Iterate over all tiles - useful for drawing
    // This allows range-based for loops: for (const Tile& tile : grid) { ... }
    auto begin() const { return tiles.begin(); }
    auto end() const { return tiles.end(); }
    auto begin() { return tiles.begin(); }
    auto end() { return tiles.end(); }
    
    // Get all non-empty tiles (useful for drawing only tiles that exist)
    std::vector<const Tile*> getNonEmptyTiles() const {
        std::vector<const Tile*> result;
        for (const Tile& tile : tiles) {
            if (!tile.isEmpty()) {
                result.push_back(&tile);
            }
        }
        return result;
    }
};

// GameContext - holds game state
struct GameContext {
    Grid grid;
    int score;
    int high_score;
    
    // Constructor - initializes the grid
    GameContext() : grid(GRID_ROWS, GRID_COLS, GRID_WIDTH, GRID_HEIGHT), 
                    score(0), high_score(0) {}
};

// AppState - holds application state
struct AppState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    GameContext game_ctx;
    Uint64 last_step;
};

void InitGame(AppState *as)
{
    // Initialize random number generator with current time
    srand((unsigned int)time(NULL));
    
    // Get reference to game context
    GameContext& ctx = as->game_ctx;
    
    // Spawn 2 initial tiles at random positions (as per README)
    for (int i = 0; i < 2; i++) {
        ctx.grid.spawnRandomTile(2);
    }
}

void UpdateGame(AppState *as)
{
    // Game update logic will go here (movement, merging, etc.)
    // update scores
    
}

void DrawGame(AppState *as)
{
    SDL_Renderer* renderer = as->renderer;
    const Grid& grid = as->game_ctx.grid;
    
    // Step 1: Clear the screen with background color
    SDL_SetRenderDrawColor(renderer, 250, 248, 239, 255);  // Light beige background
    SDL_RenderClear(renderer);
    
    // Step 2: Draw the grid background
    SDL_FRect gridRect = grid.getRect();
    SDL_SetRenderDrawColor(renderer, 187, 173, 160, 255);  // Dark beige
    SDL_RenderFillRect(renderer, &gridRect);
    
    // Step 3: Draw grid lines to separate cells
    SDL_SetRenderDrawColor(renderer, 150, 140, 130, 255);  // Darker gray for lines
    float tileWidth = grid.getTileWidth();
    float tileHeight = grid.getTileHeight();
    
    // Draw vertical lines
    for (int i = 1; i < GRID_COLS; i++) {
        float x = (float)(i * tileWidth);
        SDL_RenderLine(renderer, x, 0.0f, x, GRID_HEIGHT);
    }
    
    // Draw horizontal lines
    for (int i = 1; i < GRID_ROWS; i++) {
        float y = (float)(i * tileHeight);
        SDL_RenderLine(renderer, 0.0f, y, GRID_WIDTH, y);
    }
    
    // Step 4: Draw all non-empty tiles using range-based for loop
    // This is much cleaner than nested loops!
    for (const Tile* tilePtr : grid.getNonEmptyTiles()) {
        const Tile& tile = *tilePtr;
        
        // Get the rectangle and color for this tile
        SDL_FRect tileRect = tile.getRect(tileWidth, tileHeight);
        Uint8 r, g, b;
        tile.getColor(r, g, b);
        
        // Draw the tile background
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderFillRect(renderer, &tileRect);
        
        // Draw the tile's number text
        tile.drawText(renderer, tileWidth, tileHeight);
    }
    
    // Step 5: Draw score and high score below the grid
    const GameContext& ctx = as->game_ctx;
    float scoreY = GRID_HEIGHT + 20.0f;  // 20px below the grid
    float labelX = 20.0f;  // Left margin for labels
    float spacing = 15.0f;  // Space between label and value
    
    // Draw "Score:" label (using scaled text)
    SDL_SetRenderDrawColor(renderer, 119, 110, 101, 255);  // Dark gray
    RenderScaledText(renderer, labelX, scoreY, "Score:");
    
    // Calculate position for score value based on actual text width
    float scoreLabelWidth = GetScaledTextWidth("Score:");
    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "%d", ctx.score);
    float scoreValueX = labelX + scoreLabelWidth + spacing;  // Dynamic positioning
    RenderScaledText(renderer, scoreValueX, scoreY, scoreText);
    
    // Draw "High Score:" label
    float textHeight = GetScaledTextHeight();
    float highScoreY = scoreY + textHeight + 10.0f;  // Dynamic spacing based on text height
    RenderScaledText(renderer, labelX, highScoreY, "High Score:");
    
    // Calculate position for high score value based on actual text width
    float highScoreLabelWidth = GetScaledTextWidth("High Score:");
    char highScoreText[32];
    snprintf(highScoreText, sizeof(highScoreText), "%d", ctx.high_score);
    float highScoreValueX = labelX + highScoreLabelWidth + spacing;  // Dynamic positioning
    RenderScaledText(renderer, highScoreValueX, highScoreY, highScoreText);
    
    // Step 6: Present the rendered frame to the screen
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

    // Use placement new to properly construct AppState (which contains GameContext with constructor)
    // SDL_calloc zero-initializes memory but doesn't call constructors
    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) {
        SDL_Quit();
        return SDL_APP_FAILURE;
    }
    // Use placement new to call the GameContext constructor
    new (&as->game_ctx) GameContext();

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
        case SDL_EVENT_KEY_DOWN: {
            // Handle arrow key presses for tile movement
            AppState *as = (AppState *)appstate;
            SDL_Keycode key = event->key.key;
            
            Grid::Direction dir;
            bool validKey = false;
            
            switch(key) {
            case SDLK_UP:
                dir = Grid::UP;
                validKey = true;
                break;
            case SDLK_DOWN:
                dir = Grid::DOWN;
                validKey = true;
                break;
            case SDLK_LEFT: 
                dir = Grid::LEFT;
                validKey = true;
                break;
            case SDLK_RIGHT: 
                dir = Grid::RIGHT;
                validKey = true;
                break;
            case SDLK_R:
                // Restart the game
                as->game_ctx.grid.restart();
                as->game_ctx.score = 0;
                // Keep high_score - don't reset it
                InitGame(as);  // Spawn initial tiles
                break;
            default:
                // Unknown key - do nothing, don't log every key press
                break;
            }
            
            if (validKey) {
                int mergeScore = 0;
                bool moved = as->game_ctx.grid.orderTilesAndMerge(dir, mergeScore);
                
                if (moved) {
                    // Update score with merge points
                    as->game_ctx.score += mergeScore;
                    
                    // Update high score if needed
                    if (as->game_ctx.score > as->game_ctx.high_score) {
                        as->game_ctx.high_score = as->game_ctx.score;
                    }
                    
                    // Spawn a new tile (90% chance of 2, 10% chance of 4)
                    int newTileValue = (rand() % 10 == 0) ? 4 : 2;
                    as->game_ctx.grid.spawnRandomTile(newTileValue);
                }
            }
            break;
        }
        default:
            break;
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
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