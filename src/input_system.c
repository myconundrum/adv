#include "input_system.h"
#include <SDL2/SDL.h>
#include "log.h"
#include "error.h"
#include "world.h"
#include "components.h"
#include "messageview.h"

// Key state tracking for proper key press detection
static bool key_was_down[SDL_NUM_SCANCODES] = {false};

void input_system(Entity entity, World *world) {
    if (!world) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "world cannot be NULL");
        return;
    }
    
    if (entity == INVALID_ENTITY) {
        ERROR_SET(RESULT_ERROR_ENTITY_INVALID, "Invalid entity passed to input system");
        return;
    }
    
    // Only process input when in playing state
    if (world_get_state(world) != GAME_STATE_PLAYING) {
        return;
    }
    
    Action *action = (Action *)entity_get_component(entity, component_get_id("Action"));
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
            messageview_toggle();
        }
        key_was_down[SDL_SCANCODE_M] = true;
        return;
    } else {
        key_was_down[SDL_SCANCODE_M] = false;
    }
    
    // Only process movement if message window doesn't have focus
    if (messageview_has_focus()) {
        return;
    }
    
    // Movement input - only trigger on key press (not while held)
    if (keystate[SDL_SCANCODE_UP] && !key_was_down[SDL_SCANCODE_UP]) {
        action->type = ACTION_MOVE;
        action->action_data = DIRECTION_UP;
    } else if (keystate[SDL_SCANCODE_DOWN] && !key_was_down[SDL_SCANCODE_DOWN]) {
        action->type = ACTION_MOVE;
        action->action_data = DIRECTION_DOWN;
    } else if (keystate[SDL_SCANCODE_LEFT] && !key_was_down[SDL_SCANCODE_LEFT]) {
        action->type = ACTION_MOVE;
        action->action_data = DIRECTION_LEFT;
    } else if (keystate[SDL_SCANCODE_RIGHT] && !key_was_down[SDL_SCANCODE_RIGHT]) {
        action->type = ACTION_MOVE;
        action->action_data = DIRECTION_RIGHT;
    }
    
    // Update key states for next frame
    key_was_down[SDL_SCANCODE_UP] = keystate[SDL_SCANCODE_UP];
    key_was_down[SDL_SCANCODE_DOWN] = keystate[SDL_SCANCODE_DOWN];
    key_was_down[SDL_SCANCODE_LEFT] = keystate[SDL_SCANCODE_LEFT];
    key_was_down[SDL_SCANCODE_RIGHT] = keystate[SDL_SCANCODE_RIGHT];
}

void input_system_register(void) {
    // Validate that ECS is initialized before registering
    // We can't directly check this, but component_get_id will return INVALID_ENTITY if not initialized
    if (component_get_id("Action") == INVALID_ENTITY || component_get_id("Actor") == INVALID_ENTITY) {
        ERROR_SET(RESULT_ERROR_INITIALIZATION_FAILED, "Cannot register input system: required components not found");
        return;
    }
    
    uint32_t component_mask = (1 << component_get_id("Action")) | (1 << component_get_id("Actor"));
    
    // Input system should run first since other systems depend on input
    system_register_with_dependencies("InputSystem", component_mask, input_system, NULL, NULL,
                                     SYSTEM_PRIORITY_FIRST, NULL, 0);
    LOG_INFO("Input system registered with FIRST priority");
}

void input_system_init(void) {
    // Initialize key state tracking
    for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
        key_was_down[i] = false;
    }
    LOG_INFO("Input system initialized");
}
