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

// System configuration structure for clean registration
typedef struct {
    const char *name;                           // System name (required)
    uint32_t component_mask;                    // Required components (required)
    SystemFunction function;                    // Main system function (required)
    SystemPreUpdateFunction pre_update;        // Optional pre-update function
    SystemPostUpdateFunction post_update;      // Optional post-update function
    SystemPriority priority;                   // System priority (defaults to NORMAL)
    const char **dependencies;                 // Optional dependency names (NULL-terminated array)
} SystemConfig;

// Convenience macro for creating SystemConfig with defaults
#define SYSTEM_CONFIG(name, mask, func) { \
    .name = (name), \
    .component_mask = (mask), \
    .function = (func), \
    .pre_update = NULL, \
    .post_update = NULL, \
    .priority = SYSTEM_PRIORITY_NORMAL, \
    .dependencies = NULL \
}

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

// System management - Clean, unified registration API
bool system_register(struct AppState *app_state, const SystemConfig *config);

bool system_run_all(struct AppState *app_state);

// Utility functions
int strcmp_ci(const char *s1, const char *s2);

#endif
