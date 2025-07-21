#include "appstate.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

// Global singleton instance
static AppState *g_appstate = NULL;

AppState* appstate_get(void) {
    return g_appstate;
}

bool appstate_init(void) {
    if (g_appstate != NULL) {
        LOG_WARN("AppState already initialized");
        return true;
    }
    
    g_appstate = malloc(sizeof(AppState));
    if (!g_appstate) {
        LOG_ERROR("Failed to allocate memory for AppState (%zu bytes)", sizeof(AppState));
        return false;
    }
    
    // Initialize all fields to zero/default values
    memset(g_appstate, 0, sizeof(AppState));
    
    // Set initial state values
    g_appstate->initialized = false;
    g_appstate->quit_requested = false;
    g_appstate->current_state = APP_STATE_MENU;
    g_appstate->player = INVALID_ENTITY;
    g_appstate->error_counter = 0;
    
    // Initialize ECS state
    g_appstate->ecs.initialized = false;
    g_appstate->ecs.components.component_count = 0;
    g_appstate->ecs.systems.system_count = 0;
    g_appstate->ecs.systems.needs_sorting = false;
    
    // Initialize message queue
    g_appstate->messages.head = 0;
    g_appstate->messages.count = 0;
    g_appstate->messages.total_wrapped_lines = 0;
    g_appstate->messages.need_rewrap = true;
    
    // Initialize render state
    g_appstate->render.window = NULL;
    g_appstate->render.renderer = NULL;
    g_appstate->render.font = NULL;
    g_appstate->render.initialized = false;
    g_appstate->render.z_buffer_0 = NULL;
    g_appstate->render.z_buffer_1 = NULL;
    g_appstate->render.viewport_x = 0;
    g_appstate->render.viewport_y = 0;
    
    // Initialize message view state
    g_appstate->message_view.window = NULL;
    g_appstate->message_view.renderer = NULL;
    g_appstate->message_view.font = NULL;
    g_appstate->message_view.is_visible = false;
    g_appstate->message_view.has_focus = false;
    g_appstate->message_view.initialized = false;
    
    LOG_INFO("Created AppState instance");
    return true;
}

void appstate_shutdown(void) {
    if (g_appstate == NULL) {
        LOG_WARN("AppState not initialized, nothing to shutdown");
        return;
    }
    
    LOG_INFO("Destroying AppState instance");
    
    // Clean up any allocated resources within the appstate
    // (Individual systems should handle their own cleanup)
    
    free(g_appstate);
    g_appstate = NULL;
}

void appstate_request_quit(void) {
    AppState *as = appstate_get();
    if (!as) {
        LOG_ERROR("AppState not initialized");
        return;
    }
    
    as->quit_requested = true;
    LOG_INFO("Quit requested");
}

bool appstate_should_quit(void) {
    AppState *as = appstate_get();
    if (!as) {
        LOG_ERROR("AppState not initialized");
        return true; // Quit if we can't access state
    }
    
    return as->quit_requested;
}

void appstate_set_state(AppStateEnum state) {
    AppState *as = appstate_get();
    if (!as) {
        LOG_ERROR("AppState not initialized");
        return;
    }
    
    if (state < 0 || state > APP_STATE_GAME_OVER) {
        LOG_ERROR("Invalid app state: %d", state);
        return;
    }
    
    as->current_state = state;
    LOG_INFO("App state changed to %d", state);
}

AppStateEnum appstate_get_state(void) {
    AppState *as = appstate_get();
    if (!as) {
        LOG_ERROR("AppState not initialized");
        return APP_STATE_MENU; // Default fallback
    }
    
    return as->current_state;
} 
