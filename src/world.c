#include "world.h"
#include "log.h"
#include "error.h"
#include <stdlib.h>

// Global world instance
World *g_world = NULL;

World* world_create(void) {
    World *world = malloc(sizeof(World));
    if (!world) {
        ERROR_SET(RESULT_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for World structure (%zu bytes)", sizeof(World));
        return NULL;
    }
    
    // Initialize world state
    world->initialized = false;
    world->quit_requested = false;
    world->current_state = GAME_STATE_MENU;
    world->player = INVALID_ENTITY;
    
    LOG_INFO("Created world instance");
    return world;
}

void world_destroy(World *world) {
    if (world) {
        free(world);
    }
}

void world_request_quit(World *world) {
    if (world) {
        world->quit_requested = true;
    }
}

bool world_should_quit(World *world) {
    if (world) {
        return world->quit_requested;
    }
    return false;
}

// Game state management
void world_set_state(World *world, GameState state) {
    if (world) {
        world->current_state = state;
    }
}

GameState world_get_state(World *world) {
    if (world) {
        return world->current_state;
    }
    return GAME_STATE_MENU;
} 
