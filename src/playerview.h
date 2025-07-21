#ifndef PLAYERVIEW_H
#define PLAYERVIEW_H

#include <SDL2/SDL.h>
// Forward declaration
struct AppState;

// Initialize the player view system
void playerview_init(void);

// Render the player view sidebar
void playerview_render(SDL_Renderer *renderer, struct AppState *app_state);

// Cleanup the player view system
void playerview_cleanup(void);

#endif
