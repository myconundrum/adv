#include "world.h"
#include <stdlib.h>
#include <string.h>

// Global world instance
World *g_world = NULL;

World* world_create(void) {
    World *world = malloc(sizeof(World));
    if (!world) {
        return NULL;
    }
    
    // Initialize all members to zero/default state
    memset(world, 0, sizeof(World));
    
    // Set initial state
    world->initialized = false;
    world->quit_requested = false;
    world->player = INVALID_ENTITY;
    world->current_state = GAME_STATE_MENU;
    
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
