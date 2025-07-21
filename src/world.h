#ifndef WORLD_H
#define WORLD_H

#include "types.h"
#include "dungeon.h"

// Game states for different phases of the game
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_CHARACTER_CREATION,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_GAME_OVER
} GameState;

typedef struct {
    // Core game state
    Dungeon dungeon;
    Entity player;
    
    // Application state
    bool initialized;
    bool quit_requested;
    GameState current_state;
    
    // System-specific states can be added here in the future
    // RenderState render_state;
} World;

// Global world instance
extern World *g_world;

// World management functions
World* world_create(void);
void world_destroy(World *world);
void world_request_quit(World *world);
bool world_should_quit(World *world);

// Game state management
void world_set_state(World *world, GameState state);
GameState world_get_state(World *world);

#endif

