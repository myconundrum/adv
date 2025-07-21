#ifndef STATUSVIEW_H
#define STATUSVIEW_H

#include <SDL2/SDL.h>
// Forward declaration
struct AppState;

// Initialize the status view system
void statusview_init(void);

// Render the status line
void statusview_render(SDL_Renderer *renderer, struct AppState *app_state);

// Cleanup the status view system
void statusview_cleanup(void);

#endif
