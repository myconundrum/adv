#include "ecs.h"
#include <assert.h>
#include "baseds.h"
#include "field.h"
#include "log.h"
#include "appstate.h"
#include "components.h"
#include "config.h"
#include "mempool.h"
#include "error.h"

// Hash table for component name lookups
#define COMPONENT_HASH_TABLE_SIZE 64

// ComponentHashEntry and ComponentHashTable are now defined in appstate.h

// Simple hash function for component names
static uint32_t hash_component_name(const char *name) {
    uint32_t hash = 5381;
    for (const char *p = name; *p != '\0'; p++) {
        // Convert to lowercase for case-insensitive hash
        char c = (char)tolower((unsigned char)*p);
        hash = ((hash << 5) + hash) + (uint32_t)c;
    }
    return hash % COMPONENT_HASH_TABLE_SIZE;
}

// Initialize hash table
static void component_hash_table_init(ComponentHashTable *table) {
    for (int i = 0; i < COMPONENT_HASH_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
}

// Add entry to hash table
static void component_hash_table_add(ComponentHashTable *table, const char *name, uint32_t component_id, AppState *app_state) {
    uint32_t bucket = hash_component_name(name);
    
    ComponentHashEntry *entry = pool_malloc(sizeof(ComponentHashEntry), app_state);
    if (!entry) {
        LOG_ERROR("Failed to allocate memory for hash table entry");
        return;
    }
    
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->component_id = component_id;
    entry->next = table->buckets[bucket];
    table->buckets[bucket] = entry;
}

// Find entry in hash table
static uint32_t component_hash_table_find(ComponentHashTable *table, const char *name) {
    uint32_t bucket = hash_component_name(name);
    
    ComponentHashEntry *entry = table->buckets[bucket];
    while (entry != NULL) {
        if (strcmp_ci(entry->name, name) == 0) {
            return entry->component_id;
        }
        entry = entry->next;
    }
    
    return INVALID_ENTITY;
}

// Cleanup hash table
static void component_hash_table_cleanup(ComponentHashTable *table, AppState *app_state) {
    for (int i = 0; i < COMPONENT_HASH_TABLE_SIZE; i++) {
        ComponentHashEntry *entry = table->buckets[i];
        while (entry != NULL) {
            ComponentHashEntry *next = entry->next;
            pool_free(entry, app_state);
            entry = next;
        }
        table->buckets[i] = NULL;
    }
}

// Case-insensitive string comparison
int strcmp_ci(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) {
            return diff;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}


// System dependencies and ordering
#define MAX_SYSTEM_DEPENDENCIES 8

// System definition is now in appstate.h

typedef struct {
    System systems[MAX_SYSTEMS];
    uint32_t system_count;
    bool needs_sorting;  // Flag to indicate if systems need re-sorting
} SystemRegistry;

// Sparse component storage for memory efficiency
#define INITIAL_COMPONENT_CAPACITY 16

// SparseComponentArray is now defined in appstate.h

// Forward declarations for sparse array functions
static bool sparse_array_init(SparseComponentArray *array, size_t component_size, AppState *app_state);
static void sparse_array_cleanup(SparseComponentArray *array, AppState *app_state);
static bool sparse_array_resize(SparseComponentArray *array, uint32_t new_capacity);
static bool sparse_array_add(SparseComponentArray *array, Entity entity, void *component_data, AppState *app_state);
static void* sparse_array_get(SparseComponentArray *array, Entity entity);
static bool sparse_array_remove(SparseComponentArray *array, Entity entity, AppState *app_state);

// ComponentRegistryEntry is now defined in appstate.h

// Sparse component array utility functions
static bool sparse_array_init(SparseComponentArray *array, size_t component_size, AppState *app_state) {
    array->sparse = calloc(MAX_ENTITIES, sizeof(uint32_t));
    array->dense_entities = malloc(INITIAL_COMPONENT_CAPACITY * sizeof(uint32_t));
    array->dense_components = malloc(INITIAL_COMPONENT_CAPACITY * sizeof(void*));
    array->count = 0;
    array->capacity = INITIAL_COMPONENT_CAPACITY;
    array->component_size = component_size;
    
    if (!array->sparse || !array->dense_entities || !array->dense_components) {
        sparse_array_cleanup(array, app_state);
        return false;
    }
    
    // Initialize sparse array to invalid indices
    for (uint32_t i = 0; i < MAX_ENTITIES; i++) {
        array->sparse[i] = UINT32_MAX;
    }
    
    return true;
}

static void sparse_array_cleanup(SparseComponentArray *array, AppState *app_state) {
    if (array->sparse) {
        free(array->sparse);
        array->sparse = NULL;
    }
    if (array->dense_entities) {
        free(array->dense_entities);
        array->dense_entities = NULL;
    }
    if (array->dense_components) {
        // Free individual component data using memory pool
        for (uint32_t i = 0; i < array->count; i++) {
            if (array->dense_components[i]) {
                pool_free(array->dense_components[i], app_state);
            }
        }
        free(array->dense_components);
        array->dense_components = NULL;
    }
    array->count = 0;
    array->capacity = 0;
}

static bool sparse_array_resize(SparseComponentArray *array, uint32_t new_capacity) {
    uint32_t *new_entities = realloc(array->dense_entities, new_capacity * sizeof(uint32_t));
    void **new_components = realloc(array->dense_components, new_capacity * sizeof(void*));
    
    if (!new_entities || !new_components) {
        return false;
    }
    
    array->dense_entities = new_entities;
    array->dense_components = new_components;
    array->capacity = new_capacity;
    return true;
}

static bool sparse_array_add(SparseComponentArray *array, Entity entity, void *component_data, AppState *app_state) {
    // Check if entity already has this component
    if (entity < MAX_ENTITIES && array->sparse[entity] != UINT32_MAX) {
        // Update existing component
        memcpy(array->dense_components[array->sparse[entity]], component_data, array->component_size);
        return true;
    }
    
    // Resize if needed
    if (array->count >= array->capacity) {
        uint32_t new_capacity = array->capacity * 2;
        if (!sparse_array_resize(array, new_capacity)) {
            return false;
        }
    }
    
    // Allocate memory for new component using memory pool
    void *new_component = pool_malloc(array->component_size, app_state);
    if (!new_component) {
        return false;
    }
    
    // Copy component data
    memcpy(new_component, component_data, array->component_size);
    
    // Add to dense arrays
    uint32_t dense_index = array->count;
    array->dense_entities[dense_index] = entity;
    array->dense_components[dense_index] = new_component;
    
    // Update sparse mapping
    if (entity < MAX_ENTITIES) {
        array->sparse[entity] = dense_index;
    }
    
    array->count++;
    return true;
}

static void* sparse_array_get(SparseComponentArray *array, Entity entity) {
    if (entity >= MAX_ENTITIES) {
        return NULL;
    }
    
    uint32_t dense_index = array->sparse[entity];
    if (dense_index == UINT32_MAX || dense_index >= array->count) {
        return NULL;
    }
    
    return array->dense_components[dense_index];
}

static bool sparse_array_remove(SparseComponentArray *array, Entity entity, AppState *app_state) {
    if (entity >= MAX_ENTITIES) {
        return false;
    }
    
    uint32_t dense_index = array->sparse[entity];
    if (dense_index == UINT32_MAX || dense_index >= array->count) {
        return false;
    }
    
    // Free the component data using memory pool
    if (array->dense_components[dense_index]) {
        pool_free(array->dense_components[dense_index], app_state);
    }
    
    // Move last element to fill the gap (swap-remove)
    uint32_t last_index = array->count - 1;
    if (dense_index != last_index) {
        array->dense_entities[dense_index] = array->dense_entities[last_index];
        array->dense_components[dense_index] = array->dense_components[last_index];
        
        // Update sparse mapping for moved entity
        Entity moved_entity = array->dense_entities[dense_index];
        if (moved_entity < MAX_ENTITIES) {
            array->sparse[moved_entity] = dense_index;
        }
    }
    
    // Clear sparse mapping for removed entity
    array->sparse[entity] = UINT32_MAX;
    array->count--;
    
    return true;
}

typedef struct {
    ComponentRegistryEntry component_info[MAX_COMPONENTS];
    SparseComponentArray component_arrays[MAX_COMPONENTS];  // Sparse storage arrays
    uint32_t component_active[MAX_ENTITIES];
    
    uint32_t component_count;
    bool initialized;
    
    // Hash table for O(1) component name lookups
    ComponentHashTable name_lookup;

} ComponentRegistry;

typedef struct _ecs_state {
    stack active_entities;
    stack inactive_entities;    
    ComponentRegistry components;
    SystemRegistry systems;
    bool initialized;
} ecs_state;

// Global ECS state removed - now using AppState.ecs

// System dependency management functions (moved here after g_ecs_state definition)
static int find_system_by_name(struct AppState *app_state, const char *name) {
    if (!app_state) return -1;
    
    for (uint32_t i = 0; i < app_state->ecs.systems.system_count; i++) {
        if (strcmp_ci(app_state->ecs.systems.systems[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static bool has_circular_dependency(struct AppState *app_state, uint32_t system_index, uint32_t target_index, bool *visited, bool *rec_stack) {
    if (!app_state) return false;
    
    visited[system_index] = true;
    rec_stack[system_index] = true;
    
    System *system = &app_state->ecs.systems.systems[system_index];
    
    // Check all dependencies of this system
    for (uint32_t i = 0; i < system->dependency_count; i++) {
        int dep_index = find_system_by_name(app_state, system->dependencies[i]);
        if (dep_index >= 0) {
            uint32_t dep_idx = (uint32_t)dep_index;
            
            if (dep_idx == target_index) {
                return true; // Found circular dependency
            }
            
            if (!visited[dep_idx] && has_circular_dependency(app_state, dep_idx, target_index, visited, rec_stack)) {
                return true;
            } else if (rec_stack[dep_idx]) {
                return true;
            }
        }
    }
    
    rec_stack[system_index] = false;
    return false;
}

static bool validate_system_dependencies(struct AppState *app_state) {
    bool visited[MAX_SYSTEMS] = {false};
    bool rec_stack[MAX_SYSTEMS] = {false};
    
    if (!app_state) return false;
    
    // Check for circular dependencies
    for (uint32_t i = 0; i < app_state->ecs.systems.system_count; i++) {
        if (!visited[i]) {
            if (has_circular_dependency(app_state, i, i, visited, rec_stack)) {
                LOG_ERROR("Circular dependency detected in system: %s", app_state->ecs.systems.systems[i].name);
                return false;
            }
        }
        
        // Reset for next check
        for (uint32_t j = 0; j < MAX_SYSTEMS; j++) {
            visited[j] = false;
            rec_stack[j] = false;
        }
    }
    
    // Validate that all dependencies exist
    for (uint32_t i = 0; i < app_state->ecs.systems.system_count; i++) {
        System *system = &app_state->ecs.systems.systems[i];
        for (uint32_t j = 0; j < system->dependency_count; j++) {
            if (find_system_by_name(app_state, system->dependencies[j]) == -1) {
                LOG_ERROR("System '%s' depends on non-existent system '%s'", 
                         system->name, system->dependencies[j]);
                return false;
            }
        }
    }
    
    return true;
}

static void topological_sort_systems(struct AppState *app_state) {
    uint32_t in_degree[MAX_SYSTEMS] = {0};
    uint32_t queue[MAX_SYSTEMS];
    uint32_t queue_front = 0, queue_rear = 0;
    uint32_t sorted_order[MAX_SYSTEMS];
    uint32_t sorted_count = 0;
    
    if (!app_state) return;
    
    // Calculate in-degrees for all systems
    for (uint32_t i = 0; i < app_state->ecs.systems.system_count; i++) {
        System *system = &app_state->ecs.systems.systems[i];
        for (uint32_t j = 0; j < system->dependency_count; j++) {
            int dep_index = find_system_by_name(app_state, system->dependencies[j]);
            if (dep_index >= 0) {
                in_degree[i]++;
            }
        }
    }
    
    // Add systems with no dependencies to queue
    for (uint32_t i = 0; i < app_state->ecs.systems.system_count; i++) {
        if (in_degree[i] == 0) {
            queue[queue_rear++] = i;
        }
    }
    
    // Process queue using Kahn's algorithm
    while (queue_front < queue_rear) {
        uint32_t current = queue[queue_front++];
        sorted_order[sorted_count++] = current;
        
        // For each system that depends on the current system
        for (uint32_t i = 0; i < app_state->ecs.systems.system_count; i++) {
            System *system = &app_state->ecs.systems.systems[i];
            for (uint32_t j = 0; j < system->dependency_count; j++) {
                if (find_system_by_name(app_state, system->dependencies[j]) == (int)current) {
                    in_degree[i]--;
                    if (in_degree[i] == 0) {
                        queue[queue_rear++] = i;
                    }
                    break;
                }
            }
        }
    }
    
    // Apply the sorted order, considering priorities
    for (uint32_t i = 0; i < sorted_count; i++) {
        uint32_t system_index = sorted_order[i];
        app_state->ecs.systems.systems[system_index].execution_order = 
            app_state->ecs.systems.systems[system_index].priority + i;
    }
    
    // Sort systems array by execution order
    for (uint32_t i = 0; i < app_state->ecs.systems.system_count - 1; i++) {
        for (uint32_t j = 0; j < app_state->ecs.systems.system_count - i - 1; j++) {
            if (app_state->ecs.systems.systems[j].execution_order > 
                app_state->ecs.systems.systems[j + 1].execution_order) {
                // Swap systems
                System temp = app_state->ecs.systems.systems[j];
                app_state->ecs.systems.systems[j] = app_state->ecs.systems.systems[j + 1];
                app_state->ecs.systems.systems[j + 1] = temp;
            }
        }
    }
    
    app_state->ecs.systems.needs_sorting = false;
    
    LOG_INFO("Systems sorted by dependencies and priority:");
    for (uint32_t i = 0; i < app_state->ecs.systems.system_count; i++) {
        LOG_INFO("  %d. %s (priority: %d, order: %d)", 
                 i + 1,
                 app_state->ecs.systems.systems[i].name,
                 app_state->ecs.systems.systems[i].priority,
                 app_state->ecs.systems.systems[i].execution_order);
    }
}

/*
ECS system implementation:
 - ComponentRegistry encapsulates all component-related data:
   - component_info[MAX_COMPONENTS]: contains the name, index, bit flag, and data size of each registered component
   - component_data[MAX_COMPONENTS][MAX_ENTITIES]: contains the data for each component
   - component_active[MAX_ENTITIES]: contains a bitmask of active components for each entity
   - component_count: number of registered components
 - SystemRegistry encapsulates all system-related data:
   - systems[MAX_SYSTEMS]: contains registered systems with their component masks and functions
   - system_count: number of registered systems
*/

// Helper function to check if entity is in active stack
static bool entity_is_active(struct AppState *app_state, Entity entity) {
    if (!app_state) return false;
    
    ll_node *current = app_state->ecs.active_entities.list.head;
    while (current != NULL) {
        Entity *active_entity = (Entity *)current->data;
        if (*active_entity == entity) {
            return true;
        }
        current = current->next;
    }
    return false;
}

static void entity_remove_from_active(struct AppState *app_state, Entity entity) {
    if (!app_state) return;
    
    ll_node *current = app_state->ecs.active_entities.list.head;
    ll_node *prev = NULL;
    
    while (current != NULL) {
        Entity *active_entity = (Entity *)current->data;
        if (*active_entity == entity) {
            // Remove this node
            if (prev == NULL) {
                // First node
                app_state->ecs.active_entities.list.head = current->next;
                if (app_state->ecs.active_entities.list.head == NULL) {
                    app_state->ecs.active_entities.list.tail = NULL;
                }
            } else {
                prev->next = current->next;
                if (current->next == NULL) {
                    app_state->ecs.active_entities.list.tail = prev;
                }
            }
            pool_free(current, app_state);
            app_state->ecs.active_entities.list.size--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

void ecs_init(struct AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return;
    }
    
    // Initialize component active masks
    for (int i = 0; i < MAX_ENTITIES; i++) {
        app_state->ecs.components.component_active[i] = 0;
    }
    
    // Initialize component count
    app_state->ecs.components.component_count = 0;
    
    // Initialize system count
    app_state->ecs.systems.system_count = 0;
    
    // Initialize component hash table
    component_hash_table_init(&app_state->ecs.components.name_lookup);
    
    // Register components - all components must be registered during ecs_init
    components_init(app_state);

    // Initialize sparse component arrays after components are registered
    size_t total_memory = 0;
    for (uint32_t i = 0; i < app_state->ecs.components.component_count; i++) {
        if (!sparse_array_init(&app_state->ecs.components.component_arrays[i], 
                              app_state->ecs.components.component_info[i].data_size, app_state)) {
            LOG_ERROR("Failed to initialize sparse array for component %s", 
                      app_state->ecs.components.component_info[i].name);
            return;
        }
        
        // Calculate initial memory usage (just the sparse array overhead)
        size_t component_overhead = (MAX_ENTITIES * sizeof(uint32_t)) + // sparse array
                                   (INITIAL_COMPONENT_CAPACITY * sizeof(uint32_t)) + // dense entities
                                   (INITIAL_COMPONENT_CAPACITY * sizeof(void*)); // dense components
        total_memory += component_overhead;
        
        LOG_INFO("Initialized sparse storage for component '%s' (initial capacity: %d entities)", 
                 app_state->ecs.components.component_info[i].name,
                 INITIAL_COMPONENT_CAPACITY);
    }
    
    LOG_INFO("Total component overhead allocated: %zu bytes (%.2f KB) - sparse storage", 
             total_memory, 
             (double)total_memory / 1024);
    LOG_INFO("Memory savings: ~%.1fMB compared to dense allocation", 
             (684000.0 - total_memory) / (1024 * 1024));

    // Initialize entity stacks
    stack_init(&app_state->ecs.active_entities, sizeof(Entity));
    stack_init(&app_state->ecs.inactive_entities, sizeof(Entity));
    
    // Push all entity IDs to inactive stack (in reverse order so they pop in order)
    for (int i = MAX_ENTITIES - 1; i >= 0; i--) {
        Entity entity_id = i;
        stack_push(&app_state->ecs.inactive_entities, &entity_id);
    }

    app_state->ecs.initialized = true;
    LOG_INFO("ECS initialized with %d components using sparse storage", app_state->ecs.components.component_count);
}

void ecs_shutdown(struct AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return;
    }
    
    // Cleanup sparse component arrays
    for (uint32_t i = 0; i < app_state->ecs.components.component_count; i++) {
        sparse_array_cleanup(&app_state->ecs.components.component_arrays[i], app_state);
    }
    
    // Cleanup component hash table
    component_hash_table_cleanup(&app_state->ecs.components.name_lookup, app_state);
    
    LOG_INFO("ECS shutdown complete - sparse storage cleaned up");
}

Entity entity_create(struct AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return INVALID_ENTITY;
    }
    
    if (!app_state->ecs.initialized) {
        LOG_ERROR("ECS not initialized");
        return INVALID_ENTITY;
    }
    
    // Check if we have any inactive entities
    if (stack_empty(&app_state->ecs.inactive_entities)) {
        LOG_ERROR("Maximum entities reached");
        return INVALID_ENTITY;
    }
    
    // Pop entity ID from inactive stack
    Entity *entity_ptr = (Entity *)stack_top(&app_state->ecs.inactive_entities);
    Entity entity_id = *entity_ptr;
    stack_pop(&app_state->ecs.inactive_entities);
    
    // Add to active stack
    stack_push(&app_state->ecs.active_entities, &entity_id);
    
    return entity_id;
}

void entity_destroy(struct AppState *app_state, Entity entity) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return;
    }
    
    if (entity >= config_get_max_entities(app_state) || !entity_is_active(app_state, entity)) return;
    
    // Clear all component flags for this entity
    app_state->ecs.components.component_active[entity] = 0;
    
    // Remove from active stack
    entity_remove_from_active(app_state, entity);
    
    // Add to inactive stack
    stack_push(&app_state->ecs.inactive_entities, &entity);
}

bool entity_exists(struct AppState *app_state, Entity entity) {
    if (!app_state) return false;
    return entity < config_get_max_entities(app_state) && entity_is_active(app_state, entity);
}

void *entity_get_component(struct AppState *app_state, Entity entity, uint32_t component_id) {
    return component_get(app_state, entity, component_id);
}

uint32_t component_register(struct AppState *app_state, const char *name, size_t size) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return INVALID_ENTITY;
    }

    if (app_state->ecs.initialized) {
        LOG_ERROR("Components must be registered during ecs_init");
        return INVALID_ENTITY;
    }

    if (app_state->ecs.components.component_count >= MAX_COMPONENTS) {
        LOG_ERROR("Maximum components reached");
        return INVALID_ENTITY;
    }
    
    // Check if component already exists using hash table
    uint32_t existing_id = component_hash_table_find(&app_state->ecs.components.name_lookup, name);
    if (existing_id != INVALID_ENTITY) {
        return existing_id;
    }
    
    // Register new component
    uint32_t index = app_state->ecs.components.component_count;
    strncpy(app_state->ecs.components.component_info[index].name, name, sizeof(app_state->ecs.components.component_info[index].name) - 1);
    app_state->ecs.components.component_info[index].name[sizeof(app_state->ecs.components.component_info[index].name) - 1] = '\0';
    app_state->ecs.components.component_info[index].index = index;
    app_state->ecs.components.component_info[index].bit_flag = 1 << index;
    app_state->ecs.components.component_info[index].data_size = size;
    
    // Add to hash table for fast lookup
    component_hash_table_add(&app_state->ecs.components.name_lookup, name, index, app_state);
    
    LOG_INFO("Registered component: %s (ID: %d, Size: %zu)", name, index, size);
    
    app_state->ecs.components.component_count++;
    return index;
}

uint32_t component_get_id(struct AppState *app_state, const char *name) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return INVALID_ENTITY;
    }
    
    // Use hash table for O(1) lookup instead of O(n) linear search
    return component_hash_table_find(&app_state->ecs.components.name_lookup, name);
}


bool component_add(struct AppState *app_state, Entity entity, uint32_t component_id, void *data) {
    if (!app_state) {
        ERROR_RETURN_FALSE(RESULT_ERROR_NULL_POINTER, "AppState cannot be NULL");
    }
    
    VALIDATE_NOT_NULL_FALSE(data, "component data");
    
    if (entity >= config_get_max_entities(app_state)) {
        ERROR_RETURN_FALSE(RESULT_ERROR_OUT_OF_BOUNDS, "Entity ID %u exceeds maximum %u", entity, config_get_max_entities(app_state));
    }
    
    if (!entity_is_active(app_state, entity)) {
        ERROR_RETURN_FALSE(RESULT_ERROR_ENTITY_INVALID, "Entity %u is not active", entity);
    }
    
    if (component_id >= MAX_COMPONENTS_COMPILE_TIME || component_id >= app_state->ecs.components.component_count) {
        ERROR_RETURN_FALSE(RESULT_ERROR_COMPONENT_NOT_FOUND, "Component ID %u is invalid (max: %u)", component_id, app_state->ecs.components.component_count);
    }
    
    // Add component using sparse storage
    if (!sparse_array_add(&app_state->ecs.components.component_arrays[component_id], entity, data, app_state)) {
        ERROR_RETURN_FALSE(RESULT_ERROR_OUT_OF_MEMORY, "Failed to add component %u to entity %u", component_id, entity);
    }
    
    // Set component flag
    app_state->ecs.components.component_active[entity] |= app_state->ecs.components.component_info[component_id].bit_flag;
    
    return true;
}

bool component_remove(struct AppState *app_state, Entity entity, uint32_t component_id) {
    if (!app_state || entity >= MAX_ENTITIES || !entity_is_active(app_state, entity) || 
        component_id >= MAX_COMPONENTS || component_id >= app_state->ecs.components.component_count) {
        return false;
    }
    
    // Check if component is active
    if (app_state->ecs.components.component_active[entity] & app_state->ecs.components.component_info[component_id].bit_flag) {
        // Remove component using sparse storage
        sparse_array_remove(&app_state->ecs.components.component_arrays[component_id], entity, app_state);
        
        // Clear component flag
        app_state->ecs.components.component_active[entity] &= ~app_state->ecs.components.component_info[component_id].bit_flag;
        return true;
    }
    
    return false;
}

void *component_get(struct AppState *app_state, Entity entity, uint32_t component_id) {
    if (!app_state) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "AppState cannot be NULL");
        return NULL;
    }
    
    if (entity >= MAX_ENTITIES) {
        ERROR_SET(RESULT_ERROR_OUT_OF_BOUNDS, "Entity ID %u exceeds maximum %d", entity, MAX_ENTITIES);
        return NULL;
    }
    
    if (!entity_is_active(app_state, entity)) {
        ERROR_SET(RESULT_ERROR_ENTITY_INVALID, "Entity %u is not active", entity);
        return NULL;
    }
    
    if (component_id >= MAX_COMPONENTS || component_id >= app_state->ecs.components.component_count) {
        ERROR_SET(RESULT_ERROR_COMPONENT_NOT_FOUND, "Component ID %u is invalid (max: %u)", component_id, app_state->ecs.components.component_count);
        return NULL;
    }
    
    // Check if component is active
    if (app_state->ecs.components.component_active[entity] & app_state->ecs.components.component_info[component_id].bit_flag) {
        return sparse_array_get(&app_state->ecs.components.component_arrays[component_id], entity);
    }
    
    // Component not found on entity (this is not necessarily an error)
    return NULL;
}

bool component_has(struct AppState *app_state, Entity entity, uint32_t component_id) {
    if (!app_state || entity >= MAX_ENTITIES || !entity_is_active(app_state, entity) || 
        component_id >= MAX_COMPONENTS || component_id >= app_state->ecs.components.component_count) {
        return false;
    }
    
    return (app_state->ecs.components.component_active[entity] & app_state->ecs.components.component_info[component_id].bit_flag) != 0;
}

// Helper function to count NULL-terminated dependency array
static uint32_t count_dependencies(const char **dependencies) {
    if (!dependencies) return 0;
    
    uint32_t count = 0;
    while (dependencies[count] != NULL && count < MAX_SYSTEM_DEPENDENCIES) {
        count++;
    }
    return count;
}

// Clean, unified system registration API using SystemConfig
bool system_register(struct AppState *app_state, const SystemConfig *config) {
    if (!app_state) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "AppState cannot be NULL");
        return false;
    }
    
    if (!config) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "SystemConfig cannot be NULL");
        return false;
    }
    
    if (!config->name || !config->function) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "System name and function are required");
        return false;
    }
    
    if (!app_state->ecs.initialized) {
        ERROR_SET(RESULT_ERROR_INITIALIZATION_FAILED, "ECS must be initialized before registering systems");
        return false;
    }

    if (app_state->ecs.systems.system_count >= MAX_SYSTEMS) {
        ERROR_SET(RESULT_ERROR_SYSTEM_LIMIT, "Maximum systems reached (%d)", MAX_SYSTEMS);
        return false;
    }
    
    // Count dependencies if provided
    uint32_t dependency_count = count_dependencies(config->dependencies);
    if (dependency_count > MAX_SYSTEM_DEPENDENCIES) {
        ERROR_SET(RESULT_ERROR_INVALID_PARAMETER, "Too many dependencies (%d > %d)", dependency_count, MAX_SYSTEM_DEPENDENCIES);
        return false;
    }
    
    System *system = &app_state->ecs.systems.systems[app_state->ecs.systems.system_count];
    
    // Basic system setup
    strncpy(system->name, config->name, sizeof(system->name) - 1);
    system->name[sizeof(system->name) - 1] = '\0';
    system->function = config->function;
    system->pre_update_function = config->pre_update;   // Can be NULL
    system->post_update_function = config->post_update; // Can be NULL
    system->component_mask = config->component_mask;
    
    // Priority and dependency setup
    system->priority = config->priority;
    system->dependency_count = dependency_count;
    system->enabled = true;
    system->execution_order = system->priority; // Initial order, will be refined by sorting
    system->execution_count = 0;
    system->total_execution_time = 0.0f;
    
    // Copy dependencies if provided
    if (config->dependencies && dependency_count > 0) {
        for (uint32_t i = 0; i < dependency_count; i++) {
            strncpy(system->dependencies[i], config->dependencies[i], sizeof(system->dependencies[i]) - 1);
            system->dependencies[i][sizeof(system->dependencies[i]) - 1] = '\0';
        }
    }
    
    app_state->ecs.systems.system_count++;
    app_state->ecs.systems.needs_sorting = true;
    
    LOG_INFO("Registered system: %s (priority: %d, dependencies: %d)", config->name, system->priority, system->dependency_count);
    
    // Validate and sort systems after registration
    if (!validate_system_dependencies(app_state)) {
        LOG_ERROR("System dependency validation failed");
        return false;
    }
    
    topological_sort_systems(app_state);
    return true;
}

bool system_run_all(AppState *app_state) {
    if (!app_state) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "AppState pointer is NULL");
        return false;
    }
    
    // Sort systems if needed
    if (app_state->ecs.systems.needs_sorting) {
        if (!validate_system_dependencies(app_state)) {
            LOG_ERROR("System dependency validation failed during execution");
            return false;
        }
        topological_sort_systems(app_state);
    }
    
    for (uint32_t sys = 0; sys < app_state->ecs.systems.system_count; sys++) {
        System *system = &app_state->ecs.systems.systems[sys];
        
        // Skip disabled systems
        if (!system->enabled) {
            continue;
        }
        
        // Call pre-update function if it exists
        if (system->pre_update_function) {
            system->pre_update_function(app_state);
        }
        
        // Iterate through all active entities
        ll_node *current = app_state->ecs.active_entities.list.head;
        uint32_t entities_processed = 0;
        
        while (current != NULL) {
            Entity entity = *(Entity *)current->data;
            
            // Check if entity has all required components
            bool has_all_components = true;
            
            for (uint32_t comp = 0; comp < app_state->ecs.components.component_count; comp++) {
                if (system->component_mask & app_state->ecs.components.component_info[comp].bit_flag) {
                    if (!(app_state->ecs.components.component_active[entity] & app_state->ecs.components.component_info[comp].bit_flag)) {
                        has_all_components = false;
                        break;
                    }
                }
            }
            
            if (has_all_components) {
                system->function(entity, app_state);
                entities_processed++;
            }
            
            current = current->next;
        }
        
        // Call post-update function if it exists
        if (system->post_update_function) {
            system->post_update_function(app_state);
        }
        
        // Update performance tracking
        system->execution_count++;
        
        // Log detailed execution info for debugging (only occasionally)
        if (system->execution_count % 1000 == 0) {
            LOG_DEBUG("System '%s' executed 1000 times, processed %d entities this frame", 
                     system->name, entities_processed);
        }
    }
    
    // Check if quit was requested
    if (appstate_should_quit()) {
        return false;
    }
    
    return true;
}
