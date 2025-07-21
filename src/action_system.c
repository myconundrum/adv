#include "world.h"
#include "action_system.h"
#include "render_system.h"
#include "log.h"
#include "dungeon.h"
#include "entitycache_system.h"
#include "components.h"
#include <stdio.h>

void pickup_item(Entity entity, Entity item) {
    // add the item to the actor's inventory
    Inventory *inventory = (Inventory *)entity_get_component(entity, component_get_id("Inventory"));
    if (inventory) {
        inventory->items[inventory->item_count++] = item;
    }

    // remove the item from the new position
    Position *position_item = (Position *)entity_get_component(item, component_get_id("Position"));
    if (position_item) {
        position_item->x = -1;
        position_item->y = -1;
        position_item->entity = entity;
    }

    // remove the item from the entity cache, since it's no longer on the ground.
    entitycache_remove_entity(item);
}

void action_move_entity(Entity entity, Direction direction, World *world) {
    Position *position = (Position *)entity_get_component(entity, component_get_id("Position"));
    if (!position) {
        return;
    }

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
        position->x = new_x;
        position->y = new_y;
        position->moved = true;
        
        // check if the new position has any entities.
        stack entities_at_position;
        stack_init(&entities_at_position, sizeof(Entity));
        if (entitycache_position_has_entities(new_x, new_y, &entities_at_position)) {
            // if so, check if any of them are carryable.
            for (size_t i = 0; i < entities_at_position.list.size; i++) {
                Entity item = *(Entity *)ll_get(&entities_at_position.list, i);
                BaseInfo *base_info = (BaseInfo *)entity_get_component(item, component_get_id("BaseInfo"));
                if (base_info && base_info->is_carryable) {
                    LOG_INFO("Picked up item: %s", base_info->name);
                    pickup_item(entity, item);
                }
            }
        }
        ll_list_destroy(&entities_at_position.list);
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
