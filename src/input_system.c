#include "input_system.h"
#include <SDL2/SDL.h>
#include "log.h"
#include "error.h"
#include "appstate.h"
#include "components.h"
#include "messageview.h"

// Key state tracking for proper key press detection
static bool key_was_down[SDL_NUM_SCANCODES] = {false};

void input_system(Entity entity, AppState *app_state) {
    if (!app_state) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "app_state cannot be NULL");
        return;
    }
    
    if (entity == INVALID_ENTITY) {
        ERROR_SET(RESULT_ERROR_ENTITY_INVALID, "Invalid entity passed to input system");
        return;
    }
    
    // Only process input when in playing state
    if (appstate_get_state() != APP_STATE_PLAYING) {
        return;
    }
    
    Action *action = (Action *)entity_get_component(app_state, entity, component_get_id(app_state, "Action"));
    if (!action) {
        // Not having an Action component is not an error for input system
        return;
    }

    // Reset action to none at the start of each frame
    action->type = ACTION_NONE;
    action->action_data = DIRECTION_NONE;
    
    // Get current keyboard state
    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    
    // Handle message window toggle hotkey (Ctrl+M)
    if (keystate[SDL_SCANCODE_M] && (SDL_GetModState() & KMOD_CTRL)) {
        if (!key_was_down[SDL_SCANCODE_M]) {
            messageview_toggle(app_state);
        }
        key_was_down[SDL_SCANCODE_M] = true;
        return;
    } else {
        key_was_down[SDL_SCANCODE_M] = false;
    }
    
    // Only process movement if message window doesn't have focus
    if (messageview_has_focus(app_state)) {
        return;
    }
    
    // Movement input - only trigger on key press (not while held)
    if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W]) {
        if (!key_was_down[SDL_SCANCODE_UP] && !key_was_down[SDL_SCANCODE_W]) {
            action->type = ACTION_MOVE;
            action->action_data = DIRECTION_UP;
        }
        key_was_down[SDL_SCANCODE_UP] = true;
        key_was_down[SDL_SCANCODE_W] = true;
    } else {
        key_was_down[SDL_SCANCODE_UP] = false;
        key_was_down[SDL_SCANCODE_W] = false;
    }
    
    if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S]) {
        if (!key_was_down[SDL_SCANCODE_DOWN] && !key_was_down[SDL_SCANCODE_S]) {
            action->type = ACTION_MOVE;
            action->action_data = DIRECTION_DOWN;
        }
        key_was_down[SDL_SCANCODE_DOWN] = true;
        key_was_down[SDL_SCANCODE_S] = true;
    } else {
        key_was_down[SDL_SCANCODE_DOWN] = false;
        key_was_down[SDL_SCANCODE_S] = false;
    }
    
    if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A]) {
        if (!key_was_down[SDL_SCANCODE_LEFT] && !key_was_down[SDL_SCANCODE_A]) {
            action->type = ACTION_MOVE;
            action->action_data = DIRECTION_LEFT;
        }
        key_was_down[SDL_SCANCODE_LEFT] = true;
        key_was_down[SDL_SCANCODE_A] = true;
    } else {
        key_was_down[SDL_SCANCODE_LEFT] = false;
        key_was_down[SDL_SCANCODE_A] = false;
    }
    
    if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) {
        if (!key_was_down[SDL_SCANCODE_RIGHT] && !key_was_down[SDL_SCANCODE_D]) {
            action->type = ACTION_MOVE;
            action->action_data = DIRECTION_RIGHT;
        }
        key_was_down[SDL_SCANCODE_RIGHT] = true;
        key_was_down[SDL_SCANCODE_D] = true;
    } else {
        key_was_down[SDL_SCANCODE_RIGHT] = false;
        key_was_down[SDL_SCANCODE_D] = false;
    }
}

void input_system_register(void) {
    AppState *app_state = appstate_get();
    if (!app_state) {
        LOG_ERROR("AppState not available for input system registration");
        return;
    }
    
    uint32_t component_mask = (1 << component_get_id(app_state, "Action"));
    
    SystemConfig config = {
        .name = "InputSystem",
        .component_mask = component_mask,
        .function = input_system,
        .pre_update = NULL,
        .post_update = NULL,
        .priority = SYSTEM_PRIORITY_NORMAL,
        .dependencies = NULL
    };
    
    if (system_register(app_state, &config)) {
        LOG_INFO("Input system registered successfully");
    } else {
        LOG_ERROR("Failed to register input system");
    }
}

void input_system_init(void) {
    LOG_INFO("Input system initialized");
}
