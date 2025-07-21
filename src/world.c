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
    if (!world) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "world cannot be NULL");
        return;
    }
    
    LOG_INFO("Destroying world instance");
    free(world);
}

void world_request_quit(World *world) {
    if (!world) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "world cannot be NULL");
        return;
    }
    
    world->quit_requested = true;
    LOG_INFO("Quit requested");
}

bool world_should_quit(World *world) {
    VALIDATE_NOT_NULL_FALSE(world, "world");
    
    return world->quit_requested;
}

// Game state management
void world_set_state(World *world, GameState state) {
    if (!world) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "world cannot be NULL");
        return;
    }
    
    if (state < 0 || state > GAME_STATE_GAME_OVER) {
        ERROR_SET(RESULT_ERROR_INVALID_PARAMETER, "Invalid game state: %d", state);
        return;
    }
    
    world->current_state = state;
    LOG_INFO("Game state changed to %d", state);
}

GameState world_get_state(World *world) {
    VALIDATE_NOT_NULL_FALSE(world, "world");
    
    return world->current_state;
} 
