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

// Forward declaration to avoid circular includes
struct AppState;

#define MAX_ENTITIES 1000
#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 32

// System priorities for execution ordering
typedef enum {
    SYSTEM_PRIORITY_FIRST = 0,      // Must run first (e.g., input)
    SYSTEM_PRIORITY_EARLY = 100,    // Early execution (e.g., game logic)
    SYSTEM_PRIORITY_NORMAL = 200,   // Normal execution (e.g., physics)
    SYSTEM_PRIORITY_LATE = 300,     // Late execution (e.g., animation)
    SYSTEM_PRIORITY_LAST = 400      // Must run last (e.g., rendering)
} SystemPriority;


// System function pointer types
typedef void (*SystemFunction)(Entity entity, struct AppState *app_state);

// call before the system functions are called per entity.
typedef void (*SystemPreUpdateFunction)(struct AppState *app_state);
// call after the system functions are called per entity.
typedef void (*SystemPostUpdateFunction)(struct AppState *app_state);

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

// Enhanced system registration with dependencies and priorities
void system_register_with_dependencies(const char *name, uint32_t component_mask,
    SystemFunction function, 
    SystemPreUpdateFunction pre_update_function, 
    SystemPostUpdateFunction post_update_function,
    SystemPriority priority,
    const char **dependencies,
    uint32_t dependency_count);

bool system_run_all(struct AppState *app_state);

// Utility functions
int strcmp_ci(const char *s1, const char *s2);



#endif
