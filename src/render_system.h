#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <SDL2/SDL.h>
#include "ecs.h"
#include "world.h"

// Window and cell constants
#define CELL_SIZE 16 // pixels
#define WINDOW_WIDTH 40 // in cells
#define WINDOW_HEIGHT 30 // in cells

#define WINDOW_WIDTH_PX (WINDOW_WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT_PX (WINDOW_HEIGHT * CELL_SIZE)    

#define WINDOW_TITLE "Adventure Game"

// Rendering system function
void render_system(Entity entity, World *world);

// Initialize the rendering system (includes SDL initialization)
int render_system_init(void);

// Cleanup the rendering system (includes SDL cleanup)
void render_system_cleanup(void);

// Get the renderer (for other systems that might need it)
SDL_Renderer* render_system_get_renderer(void);

// Register the rendering system with the ECS
void render_system_register(void);

#endif 
