#include "world.h"
#include "action_system.h"
#include "render_system.h"
#include "log.h"
#include "dungeon.h"
#include "entitycache_system.h"
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

    // remove the item from the cache 
    entitycache_remove_entity(item);
}


void action_move_entity(Entity entity, Direction direction, void *world_ptr) {
    World *world = (World *)world_ptr;
    Position *position = (Position *)entity_get_component(entity, component_get_id("Position"));
    stack entities_at_new_position = {0};
    
    if (!position || !world) return;
    
    // Calculate new position
    float new_x = position->x;
    float new_y = position->y;
    
    switch (direction) {
        case DIRECTION_UP:
            new_y -= 1;
            break;
        case DIRECTION_DOWN:
            new_y += 1;
            break;
        case DIRECTION_LEFT:
            new_x -= 1;
            break;
        case DIRECTION_RIGHT:
            new_x += 1;
            break;
        case DIRECTION_NONE:
            return;
    }
    
    // Check bounds
    if (new_x < 0 || new_x >= DUNGEON_WIDTH || new_y < 0 || new_y >= DUNGEON_HEIGHT) {
        return;
    }

    if (dungeon_is_walkable(&world->dungeon, (int)new_x, (int)new_y)) {
        // is there an entity at the new position?
        stack_init(&entities_at_new_position, sizeof(Entity));
        if (entitycache_position_has_entities((int)new_x, (int)new_y, &entities_at_new_position)) {
            // if there are actors at the new position, we can't move there
            // in the future, we will add attack/etc logic here. 
            //If there are items then we should
            // pick them up.
        
            // enumerate the entities at the new position. 
            for (size_t i = 0; i < entities_at_new_position.list.size; i++) {
                Entity entity_at_new_position = *(Entity *)ll_get(&entities_at_new_position.list, i);
                // if the entity is an actor, we can't move there
                if (entity_get_component(entity_at_new_position, component_get_id("Actor"))) {
                    return;
                }

                BaseInfo *base_info_item = (BaseInfo *)entity_get_component(entity_at_new_position, component_get_id("BaseInfo"));
                Actor *actor = (Actor *)entity_get_component(entity, component_get_id("Actor"));
                if (base_info_item && base_info_item->is_carryable && actor && actor->can_carry) {
                    pickup_item(entity, entity_at_new_position);
                    LOG_INFO("Picked up item: %s", base_info_item->name);
                }
            }
        }
        
        // Move to the new position
        position->x = new_x;
        position->y = new_y;
    }
}

void action_quit(void) {
    exit(0);
}



void action_system(Entity entity, void *world_ptr) {
    Action *action = (Action *)entity_get_component(entity, component_get_id("Action"));

    switch (action->type) {
        case ACTION_MOVE:
            action_move_entity(entity, (Direction)action->action_data, world_ptr);
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
