#include "components.h"
#include "ecs.h"
#include "field.h"
#include "appstate.h"

void components_init(struct AppState *app_state) {
    // Register all component types with the ECS
    component_register(app_state, "Position", sizeof(Position));
    component_register(app_state, "BaseInfo", sizeof(BaseInfo));
    component_register(app_state, "Action", sizeof(Action));
    component_register(app_state, "FieldOfView", sizeof(CompactFieldOfView));
    component_register(app_state, "Actor", sizeof(Actor));
    component_register(app_state, "Inventory", sizeof(Inventory));
}

// Convenience functions for common flag checks
bool entity_is_player(Entity entity) {
    struct AppState *app_state = appstate_get();
    if (!app_state) return false;
    
    BaseInfo *base_info = (BaseInfo *)entity_get_component(app_state, entity, component_get_id(app_state, "BaseInfo"));
    return base_info ? ENTITY_HAS_FLAG(base_info->flags, ENTITY_FLAG_PLAYER) : false;
}

bool entity_can_carry(Entity entity) {
    struct AppState *app_state = appstate_get();
    if (!app_state) return false;
    
    BaseInfo *base_info = (BaseInfo *)entity_get_component(app_state, entity, component_get_id(app_state, "BaseInfo"));
    return base_info ? ENTITY_HAS_FLAG(base_info->flags, ENTITY_FLAG_CAN_CARRY) : false;
}

bool entity_is_carryable(Entity entity) {
    struct AppState *app_state = appstate_get();
    if (!app_state) return false;
    
    BaseInfo *base_info = (BaseInfo *)entity_get_component(app_state, entity, component_get_id(app_state, "BaseInfo"));
    return base_info ? ENTITY_HAS_FLAG(base_info->flags, ENTITY_FLAG_CARRYABLE) : false;
}

bool entity_has_moved(Entity entity) {
    struct AppState *app_state = appstate_get();
    if (!app_state) return false;
    
    BaseInfo *base_info = (BaseInfo *)entity_get_component(app_state, entity, component_get_id(app_state, "BaseInfo"));
    return base_info ? ENTITY_HAS_FLAG(base_info->flags, ENTITY_FLAG_MOVED) : false;
}

void entity_clear_moved_flag(Entity entity) {
    struct AppState *app_state = appstate_get();
    if (!app_state) return;
    
    BaseInfo *base_info = (BaseInfo *)entity_get_component(app_state, entity, component_get_id(app_state, "BaseInfo"));
    if (base_info) {
        ENTITY_CLEAR_FLAG(base_info->flags, ENTITY_FLAG_MOVED);
    }
}
