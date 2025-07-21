#include "components.h"
#include "ecs.h"
#include "field.h"

void components_init(void) {
    // Register all component types with the ECS
    component_register("Position", sizeof(Position));
    component_register("BaseInfo", sizeof(BaseInfo));
    component_register("Action", sizeof(Action));
    component_register("FieldOfView", sizeof(CompactFieldOfView));
    component_register("Actor", sizeof(Actor));
    component_register("Inventory", sizeof(Inventory));
}

// Convenience functions for common flag checks
bool entity_is_player(Entity entity) {
    BaseInfo *base_info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
    return base_info ? ENTITY_HAS_FLAG(base_info->flags, ENTITY_FLAG_PLAYER) : false;
}

bool entity_can_carry(Entity entity) {
    BaseInfo *base_info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
    return base_info ? ENTITY_HAS_FLAG(base_info->flags, ENTITY_FLAG_CAN_CARRY) : false;
}

bool entity_is_carryable(Entity entity) {
    BaseInfo *base_info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
    return base_info ? ENTITY_HAS_FLAG(base_info->flags, ENTITY_FLAG_CARRYABLE) : false;
}

bool entity_has_moved(Entity entity) {
    BaseInfo *base_info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
    return base_info ? ENTITY_HAS_FLAG(base_info->flags, ENTITY_FLAG_MOVED) : false;
}

void entity_clear_moved_flag(Entity entity) {
    BaseInfo *base_info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
    if (base_info) {
        ENTITY_CLEAR_FLAG(base_info->flags, ENTITY_FLAG_MOVED);
    }
}
