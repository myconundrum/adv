#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>
#include "ecs.h"

// Forward declaration
struct AppState;

// Window and cell constants
#define CELL_SIZE 16 // pixels

// Layout constants
#define SIDEBAR_WIDTH 12 // in cells (reduced from 20 for better proportions)
#define GAME_AREA_WIDTH 48 // in cells (increased from 40)
#define GAME_AREA_HEIGHT 30 // in cells
#define STATUS_LINE_HEIGHT 1 // in cells

// Total window dimensions
#define WINDOW_WIDTH (SIDEBAR_WIDTH + GAME_AREA_WIDTH) // 60 cells total
#define WINDOW_HEIGHT (GAME_AREA_HEIGHT + STATUS_LINE_HEIGHT) // 31 cells total

#define WINDOW_WIDTH_PX (WINDOW_WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT_PX (WINDOW_HEIGHT * CELL_SIZE)    

// Layout positions
#define GAME_AREA_X_OFFSET SIDEBAR_WIDTH // Game area starts after sidebar
#define GAME_AREA_Y_OFFSET 0
#define STATUS_LINE_Y_OFFSET GAME_AREA_HEIGHT // Status line starts after game area

#define WINDOW_TITLE "Adventure Game"

// Rendering system function
void render_system(Entity entity, struct AppState *app_state);

// Initialize the rendering system (includes SDL initialization)
int render_system_init(struct AppState *app_state);

// Cleanup the rendering system (includes SDL cleanup)
void render_system_cleanup(struct AppState *app_state);

// Get the renderer (for other systems that might need it)
SDL_Renderer* render_system_get_renderer(struct AppState *app_state);

// Font management - centralized font access for all view systems
TTF_Font* render_system_get_small_font(struct AppState *app_state);   // 14pt
TTF_Font* render_system_get_medium_font(struct AppState *app_state);  // 16pt  
TTF_Font* render_system_get_large_font(struct AppState *app_state);   // 18pt

// Register the rendering system with the ECS
void render_system_register(void);

#endif 
