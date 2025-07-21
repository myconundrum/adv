#ifndef ECS_H
#define ECS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "types.h"
#include "components.h"
#include "world.h"

#define MAX_ENTITIES 1000

// Component system
#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 32




// System function pointer types
typedef void (*SystemFunction)(Entity entity, World *world);

// call before the system functions are called per entity.
typedef void (*SystemPreUpdateFunction)(World *world);
// call after the system functions are called per entity.
typedef void (*SystemPostUpdateFunction)(World *world);

// ECS Core functions
void ecs_init(void);
void ecs_shutdown(void);

// Entity management
Entity entity_create(void);
void entity_destroy(Entity entity);
bool entity_exists(Entity entity);
void * entity_get_component(Entity entity, uint32_t component_id);


// Component management
bool component_add(Entity entity, uint32_t component_id, void *data);
bool component_remove(Entity entity, uint32_t component_id);
void *component_get(Entity entity, uint32_t component_id);
bool component_has(Entity entity, uint32_t component_id);

// Component registration
uint32_t component_register(const char *name, size_t size);
uint32_t component_get_id(const char *name);

// System management
void system_register(const char *name,  uint32_t component_mask,
    SystemFunction function, 
    SystemPreUpdateFunction pre_update_function, 
    SystemPostUpdateFunction post_update_function);

bool system_run_all(World *world);

// Utility functions
int strcmp_ci(const char *s1, const char *s2);



#endif
