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
