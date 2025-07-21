#include "appstate.h"
#include "action_system.h"
#include "render_system.h"
#include "log.h"
#include "dungeon.h"
#include "components.h"
#include "messages.h"
#include <stdio.h>

void pickup_item(Entity entity, Entity item) {
    // add the item to the actor's inventory
    Inventory *inventory = (Inventory *)entity_get_component(entity, component_get_id("Inventory"));
    if (inventory) {
        inventory->items[inventory->item_count++] = item;
    }

    // remove the item from the tile
    Position *position_item = (Position *)entity_get_component(item, component_get_id("Position"));
    if (position_item) {
        // Remove item from its current tile position
        AppState *app_state = appstate_get();
        if (app_state) {
            dungeon_remove_entity_from_position(&app_state->dungeon, item, position_item->x, position_item->y);
        }
        
        // Set position to indicate it's in inventory
        position_item->entity = entity;
    }
    
    // Print pickup message
    BaseInfo *item_info = (BaseInfo *)entity_get_component(item, component_get_id("BaseInfo"));
    if (item_info) {
        char pickup_message[256];
        snprintf(pickup_message, sizeof(pickup_message), "You picked up: %s", item_info->name);
        messages_add(pickup_message);
        LOG_INFO("Picked up item: %s", item_info->name);
    }
}

void action_move_entity(Entity entity, Direction direction, AppState *app_state) {
    Position *position = (Position *)entity_get_component(entity, component_get_id("Position"));
    if (!position || !app_state) {
        return;
    }

    int old_x = position->x;
    int old_y = position->y;
    int new_x = old_x;
    int new_y = old_y;

    // Calculate new position based on direction
    switch (direction) {
        case DIRECTION_UP:
            new_y--;
            break;
        case DIRECTION_DOWN:
            new_y++;
            break;
        case DIRECTION_LEFT:
            new_x--;
            break;
        case DIRECTION_RIGHT:
            new_x++;
            break;
        case DIRECTION_NONE:
            return; // No movement
    }

    // Check if the new position is walkable
    if (dungeon_is_walkable(&app_state->dungeon, new_x, new_y)) {
        // Remove entity from old tile position
        dungeon_remove_entity_from_position(&app_state->dungeon, entity, old_x, old_y);
        
        // Update entity position
        position->x = new_x;
        position->y = new_y;
        
        // Mark entity as moved using flags in BaseInfo
        BaseInfo *base_info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
        if (base_info) {
            ENTITY_SET_FLAG(base_info->flags, ENTITY_FLAG_MOVED);
        }
        
        // Place entity at new tile position
        dungeon_place_entity_at_position(&app_state->dungeon, entity, new_x, new_y);
        
        // Check if the new position has any entities
        Entity actor_at_pos, item_at_pos;
        if (dungeon_get_entities_at_position(&app_state->dungeon, new_x, new_y, &actor_at_pos, &item_at_pos)) {
            // Check if there's a carryable item at this position
            if (item_at_pos != INVALID_ENTITY && entity_is_carryable(item_at_pos)) {
                pickup_item(entity, item_at_pos);
            }
        }
    }
}

void action_quit(void) {
    AppState *app_state = appstate_get();
    if (app_state) {
        app_state->quit_requested = true;
        LOG_INFO("Quit action requested");
    }
}

void action_system(Entity entity, AppState *app_state) {
    Action *action = (Action *)entity_get_component(entity, component_get_id("Action"));

    switch (action->type) {
        case ACTION_MOVE:
            action_move_entity(entity, (Direction)action->action_data, app_state);
            break;
        case ACTION_QUIT:
            action_quit();
            break;
        case ACTION_NONE:
            break;
    }
}

void action_system_register(void) {
    uint32_t component_mask = (1 << component_get_id("Action")) | (1 << component_get_id("Position"));
    
    // Action system depends on input system and should run early
    const char* dependencies[] = {"InputSystem"};
    system_register_with_dependencies("ActionSystem", component_mask, action_system, NULL, NULL,
                                     SYSTEM_PRIORITY_EARLY, dependencies, 1);
    LOG_INFO("Action system registered with EARLY priority, depends on InputSystem");
}

void action_system_init(void) {
    LOG_INFO("Action system initialized");
}
