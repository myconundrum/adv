#include "world.h"
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
        dungeon_remove_entity_from_position(&g_world->dungeon, item, position_item->x, position_item->y);
        
        // Mark item as carried
        position_item->x = -1;
        position_item->y = -1;
        position_item->entity = entity;
    }
}

void action_move_entity(Entity entity, Direction direction, World *world) {
    Position *position = (Position *)entity_get_component(entity, component_get_id("Position"));
    if (!position) {
        return;
    }

    int old_x = position->x;
    int old_y = position->y;
    int new_x = position->x;
    int new_y = position->y;

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
    if (dungeon_is_walkable(&world->dungeon, new_x, new_y)) {
        // Remove entity from old tile position
        dungeon_remove_entity_from_position(&world->dungeon, entity, old_x, old_y);
        
        // Update entity position
        position->x = new_x;
        position->y = new_y;
        position->moved = true;
        
        // Place entity at new tile position
        dungeon_place_entity_at_position(&world->dungeon, entity, new_x, new_y);
        
        // Check if the new position has any entities
        Entity actor_at_pos, item_at_pos;
        if (dungeon_get_entities_at_position(&world->dungeon, new_x, new_y, &actor_at_pos, &item_at_pos)) {
            // Check if there's a carryable item at this position
            if (item_at_pos != INVALID_ENTITY && item_at_pos != entity) {
                BaseInfo *base_info = (BaseInfo *)entity_get_component(item_at_pos, component_get_id("BaseInfo"));
                if (base_info && base_info->is_carryable) {
                    LOG_INFO("Picked up item: %s", base_info->name);
                    
                    // Add message to message system
                    char pickup_message[256];
                    snprintf(pickup_message, sizeof(pickup_message), "You picked up: %s", base_info->name);
                    messages_add(pickup_message);
                    
                    pickup_item(entity, item_at_pos);
                }
            }
        }
    } else {
        LOG_INFO("Cannot move to (%d, %d) - blocked", new_x, new_y);
    }
}

void action_quit(void) {
    if (g_world) {
        world_request_quit(g_world);
    }
}



void action_system(Entity entity, World *world) {
    Action *action = (Action *)entity_get_component(entity, component_get_id("Action"));

    switch (action->type) {
        case ACTION_MOVE:
            action_move_entity(entity, (Direction)action->action_data, world);
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
    system_register("Action System", component_mask, action_system, NULL, NULL);
}

void action_system_init(void) {
    LOG_INFO("Action system initialized");
}
