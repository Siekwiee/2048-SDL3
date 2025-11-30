#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>


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
    int x;
    int y;
} GameContext;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    GameContext game_ctx;
    Uint64 last_step;
} AppState;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) {
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("2048", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_free(as);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Now set the window and renderer in AppState after they're created
    as->window = window;
    as->renderer = renderer;
    as->last_step = SDL_GetTicks();
    *appstate = as;

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
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