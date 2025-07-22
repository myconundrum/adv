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

// Forward declarations to avoid circular includes
struct AppState;

// System types are defined in appstate.h, but we need forward declarations here
// Use guards to prevent redefinition when both headers are included
#ifndef SYSTEM_TYPES_DEFINED
#define SYSTEM_TYPES_DEFINED

typedef enum {
    SYSTEM_PRIORITY_FIRST = 0,
    SYSTEM_PRIORITY_EARLY = 100,
    SYSTEM_PRIORITY_NORMAL = 200,
    SYSTEM_PRIORITY_LATE = 300,
    SYSTEM_PRIORITY_LAST = 400
} SystemPriority;

typedef void (*SystemFunction)(Entity entity, struct AppState *app_state);
typedef void (*SystemPreUpdateFunction)(struct AppState *app_state);
typedef void (*SystemPostUpdateFunction)(struct AppState *app_state);

#endif /* SYSTEM_TYPES_DEFINED */

// Compile-time maximum limits (for array declarations)
// Runtime limits are controlled by configuration system
#define MAX_ENTITIES_COMPILE_TIME 10000
#define MAX_COMPONENTS_COMPILE_TIME 64
#define MAX_SYSTEMS_COMPILE_TIME 64

// Backward compatibility defines (use config system when possible)
#define MAX_ENTITIES MAX_ENTITIES_COMPILE_TIME
#define MAX_COMPONENTS MAX_COMPONENTS_COMPILE_TIME
#define MAX_SYSTEMS MAX_SYSTEMS_COMPILE_TIME

// System types are now defined in appstate.h to avoid circular dependencies

// ECS Core functions
void ecs_init(struct AppState *app_state);
void ecs_shutdown(struct AppState *app_state);

// Entity management
Entity entity_create(struct AppState *app_state);
void entity_destroy(struct AppState *app_state, Entity entity);
bool entity_exists(struct AppState *app_state, Entity entity);
void * entity_get_component(struct AppState *app_state, Entity entity, uint32_t component_id);


// Component management
bool component_add(struct AppState *app_state, Entity entity, uint32_t component_id, void *data);
bool component_remove(struct AppState *app_state, Entity entity, uint32_t component_id);
void *component_get(struct AppState *app_state, Entity entity, uint32_t component_id);
bool component_has(struct AppState *app_state, Entity entity, uint32_t component_id);

// Component registration
uint32_t component_register(struct AppState *app_state, const char *name, size_t size);
uint32_t component_get_id(struct AppState *app_state, const char *name);

// System management - Unified registration API
// All parameters after 'function' are optional (can be NULL):
// - pre_update_function, post_update_function: can be NULL
// - priority: can be NULL for SYSTEM_PRIORITY_NORMAL default
// - dependencies: can be NULL if no dependencies
// - dependency_count: should be 0 if dependencies is NULL
void system_register(struct AppState *app_state,
                    const char *name, 
                    uint32_t component_mask,
                    SystemFunction function, 
                    SystemPreUpdateFunction pre_update_function,
                    SystemPostUpdateFunction post_update_function,
                    SystemPriority *priority,
                    const char **dependencies,
                    uint32_t dependency_count);

// Legacy functions - deprecated, use system_register instead
void system_register_basic(struct AppState *app_state,
                          const char *name,  uint32_t component_mask,
                          SystemFunction function, 
                          SystemPreUpdateFunction pre_update_function, 
                          SystemPostUpdateFunction post_update_function);

void system_register_with_dependencies(struct AppState *app_state,
                                      const char *name, uint32_t component_mask,
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
