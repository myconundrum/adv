#ifndef PLAYERVIEW_H
#define PLAYERVIEW_H

#include <SDL2/SDL.h>
#include "world.h"

// Initialize the player view system
void playerview_init(void);

// Render the player view sidebar
void playerview_render(SDL_Renderer *renderer, World *world);

// Cleanup the player view system
void playerview_cleanup(void);

#endif
