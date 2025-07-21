#ifndef WORLD_H
#define WORLD_H

#include "types.h"
#include "dungeon.h"

typedef struct {
    // Core game state
    Dungeon dungeon;
    Entity player;
    
    // Application state
    bool initialized;
    bool quit_requested;
    
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

#endif

