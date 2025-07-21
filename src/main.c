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
#include "field.h"
#include "playerview.h"
#include "statusview.h"
#include "messages.h"
#include "messageview.h"
#include "main_menu.h"
#include "character_creation.h"
#include "game_state.h"

// Cleanup function to handle all resources
static void cleanup_resources(void) {
    if (g_world && g_world->initialized) {
        template_system_cleanup();
        ecs_shutdown();
        
        // Clean up view systems before render system (which calls TTF_Quit)
        playerview_cleanup();
        statusview_cleanup();
        messageview_cleanup();
        
        render_system_cleanup();
        
        // Clean up message system
        messages_shutdown();
        
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
    
    // Initialize render system (includes SDL initialization) but don't register yet
    if (!render_system_init()) {
        LOG_ERROR("Failed to initialize render system");
        return 0;
    }

    // Initialize view systems
    playerview_init();
    statusview_init();
    
    // Initialize message systems
    messages_init();
    messageview_init();

    // Initialize and register input system (first - no dependencies)
    input_system_init();
    input_system_register();

    // Initialize and register action system (second - depends on input)
    action_system_init();
    action_system_register();
    
    // Register render system last (depends on input and action systems)
    render_system_register();
    
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
    
    g_world->initialized = true;
    LOG_INFO("Game initialized successfully");
    
    // Create and initialize state manager
    GameStateManager *state_manager = game_state_manager_create();
    if (!state_manager) {
        LOG_FATAL("Failed to create game state manager");
        cleanup_resources();
        return 1;
    }
    
    // Initialize the state manager with initial state
    game_state_manager_init(state_manager, g_world);
    
    // Timing variables for delta time
    Uint32 last_time = SDL_GetTicks();
    
    // Main loop - much simpler now!
    while (!game_state_manager_should_quit(state_manager, g_world)) {
        // Calculate delta time
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        // Handle input events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            game_state_manager_handle_input(state_manager, g_world, &event);
        }
        
        // Update current state
        game_state_manager_update(state_manager, g_world, delta_time);
        
        // Render current state
        game_state_manager_render(state_manager, g_world);
        
        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    LOG_INFO("Shutting down game");
    game_state_manager_destroy(state_manager);
    cleanup_resources();
    log_shutdown();
    
    return 0;
} 
