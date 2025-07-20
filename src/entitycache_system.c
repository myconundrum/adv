#include "ecs.h"
#include "entitycache_system.h"
#include "baseds.h"
#include "field.h"


ll_list g_entity_cache = {0};
CompactFieldOfView *g_field = NULL;

void entitycache_system_init(void) {
    ll_init(&g_entity_cache, sizeof(Entity));
}

void entitycache_system_register(void) {
    system_register("entitycache", 
        component_get_id("Position"), entitycache_system, 
        entitycache_system_pre_update, entitycache_system_post_update);
}

void entitycache_system_shutdown(void) {
    ll_list_destroy(&g_entity_cache);
    field_cleanup_compact(g_field);
}

void entitycache_system_pre_update(void *world_ptr) {
    World *world = (World *)world_ptr;
    
    if (g_entity_cache.size > 0) {
        // remove all entities from the cache.
        ll_list_remove_all(&g_entity_cache);
    }

    // cast a radius five field from the player.
    g_field = init_compact_field_of_view(5);
    
    // Get player position component
    Position *player_pos = (Position *)entity_get_component(world->player, component_get_id("Position"));
    if (player_pos) {
        field_calculate_fov_compact(g_field, &world->dungeon, player_pos->x, player_pos->y);
    }
}


void entitycache_system(Entity entity, void *world_ptr) {
    (void)world_ptr; // Suppress unused variable warning

    // if this entity has a position component and is in the compact field of view, 
    // add it to the entity cache   
    Position *position = (Position *)entity_get_component(entity, component_get_id("Position"));
    if (position) {
        if (field_is_visible_compact(g_field, position->x, position->y)) {
            ll_push(&g_entity_cache, &entity);
        }
    }
}

void entitycache_system_post_update(void *world_ptr) {
    (void)world_ptr;
    
    // clean up field of view.
    field_cleanup_compact(g_field);
}

bool entitycache_position_has_entities(int x, int y, stack * stack) {
    
    // iterate over all entities in teh cache and check if they are at the  position
    for (size_t i = 0; i < g_entity_cache.size; i++) {
        Entity entity = *(Entity *)ll_get(&g_entity_cache, i);
        Position *position = (Position *)entity_get_component(entity, component_get_id("Position"));
        if (position->x == x && position->y == y) {
            stack_push(stack, &entity);
            
        }
    }
    return stack->list.size > 0;
}

void entitycache_remove_entity(Entity entity) {
    // iterate over all entities in the cache and remove the entity
    for (size_t i = 0; i < g_entity_cache.size; i++) {
        Entity entity_in_cache = *(Entity *)ll_get(&g_entity_cache, i);
        if (entity_in_cache == entity) {
            ll_remove(&g_entity_cache, i);
        }
    }
}

