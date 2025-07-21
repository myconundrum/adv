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
