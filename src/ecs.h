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

// Compile-time maximum limits (for array declarations)
// Runtime limits are controlled by configuration system
#define MAX_ENTITIES_COMPILE_TIME 10000
#define MAX_COMPONENTS_COMPILE_TIME 64
#define MAX_SYSTEMS_COMPILE_TIME 64

// Backward compatibility defines (use config system when possible)
#define MAX_ENTITIES MAX_ENTITIES_COMPILE_TIME
#define MAX_COMPONENTS MAX_COMPONENTS_COMPILE_TIME
#define MAX_SYSTEMS MAX_SYSTEMS_COMPILE_TIME

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

// System management - Unified registration API
// All parameters after 'function' are optional (can be NULL):
// - pre_update_function, post_update_function: can be NULL
// - priority: can be NULL for SYSTEM_PRIORITY_NORMAL default
// - dependencies: can be NULL if no dependencies
// - dependency_count: should be 0 if dependencies is NULL
void system_register(const char *name, 
                    uint32_t component_mask,
                    SystemFunction function, 
                    SystemPreUpdateFunction pre_update_function,
                    SystemPostUpdateFunction post_update_function,
                    SystemPriority *priority,
                    const char **dependencies,
                    uint32_t dependency_count);

// Legacy functions - deprecated, use system_register instead
void system_register_basic(const char *name,  uint32_t component_mask,
    SystemFunction function, 
    SystemPreUpdateFunction pre_update_function, 
    SystemPostUpdateFunction post_update_function);

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
