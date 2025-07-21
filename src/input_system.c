#include "input_system.h"
#include <SDL2/SDL.h>
#include "log.h"
#include "world.h"
#include "components.h"
#include "messageview.h"

static bool key_processed_this_frame = false;

void input_system(Entity entity, World *world) {
    (void)world; // Unused parameter
    Action *action = (Action *)entity_get_component(entity, component_get_id("Action"));

    // Reset action to none at the start of each frame
    action->type = ACTION_NONE;
    action->action_data = DIRECTION_NONE;
    key_processed_this_frame = false;
    
    // Poll and process all events
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        // Let message window handle its events first
        if (messageview_handle_event(&event)) {
            continue; // Event was handled by message window
        }
        
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            world_request_quit(world); // Request quit through World
        } else if (event.type == SDL_KEYDOWN && !key_processed_this_frame) {
            // Handle message window toggle hotkey
            if (event.key.keysym.sym == SDLK_m && (event.key.keysym.mod & KMOD_CTRL)) {
                messageview_toggle();
                key_processed_this_frame = true;
                continue;
            }
            
            // Only process movement if message window doesn't have focus
            if (messageview_has_focus()) {
                continue; // Let message window handle input
            }
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        action->type = ACTION_MOVE;
                        action->action_data = DIRECTION_UP;
                        key_processed_this_frame = true;
                        break;
                    case SDLK_DOWN:
                        action->type = ACTION_MOVE;
                        action->action_data = DIRECTION_DOWN;
                        key_processed_this_frame = true;
                        break;
                    case SDLK_LEFT:
                        action->type = ACTION_MOVE;
                        action->action_data = DIRECTION_LEFT;
                        key_processed_this_frame = true;
                        break;
                    case SDLK_RIGHT:
                        action->type = ACTION_MOVE;
                        action->action_data = DIRECTION_RIGHT;
                        key_processed_this_frame = true;
                        break;
                }
        }
    }
}

void input_system_register(void) {
    uint32_t component_mask = (1 << component_get_id("Action")) | (1 << component_get_id("Actor"));
    system_register("Input System", component_mask, input_system, NULL, NULL);
}

void input_system_init(void) {
    LOG_INFO("Input system initialized");
}
