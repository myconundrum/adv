#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "ecs.h"
#include "components.h"
#include "world.h"
#include "render_system.h"
#include "input_system.h"
#include "action_system.h"
#include "template_system.h"
#include "dungeon.h"
#include "playerview.h"
#include "statusview.h"

// Cleanup function to handle all resources
static void cleanup_resources(void) {
    if (g_world && g_world->initialized) {
        template_system_cleanup();
        ecs_shutdown();
        render_system_cleanup();
        playerview_cleanup();
        statusview_cleanup();
        g_world->initialized = false;
    }
    
    if (g_world) {
        world_destroy(g_world);
        g_world = NULL;
    }
}

// Initialize game systems
static int init_game_systems(void) {
    // Initialize ECS
    ecs_init();
    
    // Initialize and register render system (includes SDL initialization)
    if (!render_system_init()) {
        LOG_ERROR("Failed to initialize render system");
        return 0;
    }
    render_system_register();

    // Initialize view systems
    playerview_init();
    statusview_init();

    // Initialize and register input system
    input_system_init();
    input_system_register();

    // Initialize and register action system
    action_system_init();
    action_system_register();
    
    // Initialize template system
    if (template_system_init() != 0) {
        LOG_ERROR("Failed to initialize template system");
        return 0;
    }
    
    // Load templates from file
    if (load_templates_from_file("data.json") != 0) {
        LOG_ERROR("Failed to load templates");
        return 0;
    }
    
    return 1;
}

// Create game entities and world
static int create_entities_and_world(void) {
    // Initialize dungeon
    dungeon_init(&g_world->dungeon);
    dungeon_generate(&g_world->dungeon);
    LOG_INFO("Generated dungeon with %d rooms", g_world->dungeon.room_count);
    
    // Create player entity from template
    g_world->player = create_entity_from_template("player");
    if (g_world->player == INVALID_ENTITY) {
        LOG_ERROR("Failed to create player entity from template");
        return 0;
    }
    
    // Add field of view component to player
    CompactFieldOfView player_fov;
    field_init_compact(&player_fov, FOV_RADIUS);
    if (!component_add(g_world->player, component_get_id("FieldOfView"), &player_fov)) {
        LOG_ERROR("Failed to add FieldOfView component to player");
        return 0;
    }
    LOG_INFO("Added compact field of view component to player");
    
    // Place player at stairs up position
    Position *player_pos = (Position *)entity_get_component(g_world->player, component_get_id("Position"));
    if (player_pos) {
        player_pos->x = (float)g_world->dungeon.stairs_up_x;
        player_pos->y = (float)g_world->dungeon.stairs_up_y;
        // Store player in tile
        dungeon_place_entity_at_position(&g_world->dungeon, g_world->player, player_pos->x, player_pos->y);
        LOG_INFO("Placed player at (%d, %d)", g_world->dungeon.stairs_up_x, g_world->dungeon.stairs_up_y);
    }
    
    // Create enemy entity from template (orc)
    Entity enemy = create_entity_from_template("enemy");
    if (enemy == INVALID_ENTITY) {
        LOG_ERROR("Failed to create enemy entity from template");
        return 0;
    }
    
    // Place enemy very close to player for debugging
    Position *enemy_pos = (Position *)entity_get_component(enemy, component_get_id("Position"));
    if (enemy_pos && player_pos) {
        enemy_pos->x = player_pos->x + 1; // Right next to player
        enemy_pos->y = player_pos->y;
        // Store enemy in tile
        dungeon_place_entity_at_position(&g_world->dungeon, enemy, enemy_pos->x, enemy_pos->y);
        LOG_INFO("Placed enemy (orc) at (%d, %d) - right next to player", (int)enemy_pos->x, (int)enemy_pos->y);
    }
    
    // Create gold entity from template (treasure)
    Entity gold = create_entity_from_template("gold");
    if (gold == INVALID_ENTITY) {
        LOG_ERROR("Failed to create gold entity from template");
        return 0;
    }
    
    // Place gold very close to player for debugging
    Position *gold_pos = (Position *)entity_get_component(gold, component_get_id("Position"));
    if (gold_pos && player_pos) {
        gold_pos->x = player_pos->x;
        gold_pos->y = player_pos->y + 1; // Below player
        // Store gold in tile
        dungeon_place_entity_at_position(&g_world->dungeon, gold, gold_pos->x, gold_pos->y);
        LOG_INFO("Placed gold (treasure) at (%d, %d) - below player", (int)gold_pos->x, (int)gold_pos->y);
    }
    
    // Create sword entity from template
    Entity sword = create_entity_from_template("sword");
    if (sword == INVALID_ENTITY) {
        LOG_ERROR("Failed to create sword entity from template");
        return 0;
    }
    
    // Place sword very close to player for debugging
    Position *sword_pos = (Position *)entity_get_component(sword, component_get_id("Position"));
    if (sword_pos && player_pos) {
        sword_pos->x = player_pos->x - 1; // Left of player
        sword_pos->y = player_pos->y;
        // Store sword in tile
        dungeon_place_entity_at_position(&g_world->dungeon, sword, sword_pos->x, sword_pos->y);
        LOG_INFO("Placed sword at (%d, %d) - left of player", (int)sword_pos->x, (int)sword_pos->y);
    }
    
    return 1;
}

int main(int argc, char* argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    
    // Initialize logging system
    LogConfig log_config = {
        .min_level = LOG_LEVEL_INFO,
        .use_colors = true,
        .use_timestamps = false,  // Timestamps disabled for cleaner output
        .log_file = NULL  // No file output for now
    };
    log_init(log_config);
    
    LOG_INFO("Starting Adventure Game - ECS");
    
    // Create world instance
    g_world = world_create();
    if (!g_world) {
        LOG_FATAL("Failed to create world instance");
        return 1;
    }
    
    // Initialize game systems
    if (!init_game_systems()) {
        LOG_FATAL("Failed to initialize game systems");
        cleanup_resources();
        return 1;
    }
    
    // Create game entities and world
    if (!create_entities_and_world()) {
        LOG_FATAL("Failed to create game entities and world");
        cleanup_resources();
        return 1;
    }
    
    g_world->initialized = true;
    LOG_INFO("Game initialized successfully, entering main loop");
    
    // Main loop
    while (true) {
        // Run ECS systems (this will handle input, actions, rendering, and frame presentation)
        if (!system_run_all(g_world)) {
            break; // Quit requested
        }
        
        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    LOG_INFO("Shutting down game");
    cleanup_resources();
    log_shutdown();
    
    return 0;
} 
