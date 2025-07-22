#include <SDL2/SDL.h>
#include "log.h"
#include "config.h"
#include "mempool.h"
#include "ecs.h"
#include "appstate.h"
#include "render_system.h"
#include "input_system.h"
#include "action_system.h"
#include "template_system.h"
#include "playerview.h"
#include "statusview.h"
#include "messages.h"
#include "messageview.h"
#include "game_state.h"

// Cleanup function to handle all resources
static void cleanup_resources(void) {
    AppState *as = appstate_get();
    if (as && as->initialized) {
        template_system_cleanup();
        ecs_shutdown(as);
        
        // Clean up view systems before render system (which calls TTF_Quit)
        playerview_cleanup();
        statusview_cleanup();
        messageview_cleanup();
        
        render_system_cleanup();
        
        // Clean up message system
        messages_shutdown();
        
        as->initialized = false;
    }
    
    // Shutdown the appstate singleton
    appstate_shutdown();
    
    // Cleanup memory pool system
    if (mempool_is_initialized()) {
        mempool_cleanup();
    }
    
    // Cleanup configuration system
    config_cleanup();
}

// Initialize game systems
static int init_game_systems(void) {
    // Get AppState for ECS initialization
    AppState *as = appstate_get();
    if (!as) {
        LOG_ERROR("AppState not initialized");
        return false;
    }
    
    // Initialize ECS
    ecs_init(as);
    
    // Initialize render system (includes SDL initialization) but don't register yet
    if (!render_system_init()) {
        LOG_ERROR("Failed to initialize render system");
        return false;
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
        return false;
    }
    
    // Load templates from file
    if (load_templates_from_file("data.json") != 0) {
        LOG_ERROR("Failed to load templates");
        return false;
    }
    
    return true;
}

bool init_all(void) {

 
    // Initialize logging system
    LogConfig log_config = {
        .min_level = LOG_LEVEL_INFO,
        .use_colors = true,
        .use_timestamps = false,  // Timestamps disabled for cleaner output
        .log_file = NULL  // No file output for now
    };
    log_init(log_config);
    
    LOG_INFO("Starting Adventure Game - ECS");
    
    // Initialize configuration system
    if (!config_init()) {
        LOG_FATAL("Failed to initialize configuration system");
        return false;
    }
    
    // Load configuration from file
    if (!config_load_from_file("adv_config.json")) {
        LOG_WARN("Failed to load adv_config.json, using defaults");
    }
    
    // Initialize memory pool system
    const GameConfig *config = config_get();
    if (config->mempool.enable_pool_allocation) {
        mempool_set_chunk_count(config->mempool.initial_chunks_per_pool, 
                               config->mempool.max_chunks_per_pool);
        mempool_enable_corruption_detection(config->mempool.enable_corruption_detection);
        mempool_enable_statistics(config->mempool.enable_statistics);
        
        if (!mempool_init()) {
            LOG_FATAL("Failed to initialize memory pool");
            return false;
        }
        LOG_INFO("Memory pool system initialized");
    } else {
        LOG_INFO("Memory pool allocation disabled by configuration");
    }
    
    // Initialize appstate singleton
    if (!appstate_init()) {
        LOG_FATAL("Failed to initialize AppState");
        return false;
    }
    
    // Initialize game systems
    if (!init_game_systems()) {
        LOG_FATAL("Failed to initialize game systems");
        cleanup_resources();
        return false;
    }
    
    AppState *as = appstate_get();
    as->initialized = true;
    LOG_INFO("Game initialized successfully");

    return true;
}


int main(int argc, char* argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
   
    if (!init_all()) {
        LOG_FATAL("Failed to initialize all systems");
        return 1;
    }
    
    // Create and initialize state manager
    GameStateManager *state_manager = game_state_manager_create();
    if (!state_manager) {
        LOG_FATAL("Failed to create game state manager");
        cleanup_resources();
        return 1;
    }
    
    // Initialize the state manager with initial state
    game_state_manager_init(state_manager);
    
    // Timing variables for delta time
    Uint32 last_time = SDL_GetTicks();
    
    // Main loop - much simpler now!
    while (!game_state_manager_should_quit(state_manager)) {
        // Calculate delta time
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        // Handle input events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            game_state_manager_handle_input(state_manager, &event);
        }
        
        // Update current state
        game_state_manager_update(state_manager, delta_time);
        
        // Render current state
        game_state_manager_render(state_manager);
        
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
