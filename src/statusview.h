#ifndef STATUSVIEW_H
#define STATUSVIEW_H

#include <SDL2/SDL.h>
#include "world.h"

// Initialize the status view system
void statusview_init(void);

// Render the status line
void statusview_render(SDL_Renderer *renderer, World *world);

// Cleanup the status view system
void statusview_cleanup(void);

#endif
