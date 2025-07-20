#include "input_system.h"
#include <SDL2/SDL.h>
#include "log.h"
#include "world.h"

static bool key_processed_this_frame = false;

void input_system(Entity entity, void *world_ptr) {
    World *world = (World *)world_ptr;
    (void)world; // Unused parameter
    Action *action = (Action *)entity_get_component(entity, component_get_id("Action"));

    // Reset action to none at the start of each frame
    action->type = ACTION_NONE;
    action->action_data = DIRECTION_NONE;
    key_processed_this_frame = false;
    
    // Poll and process all events
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            ecs_request_quit(); // Request quit through ECS
        } else if (event.type == SDL_KEYDOWN && !key_processed_this_frame) {
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
