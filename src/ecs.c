#include "ecs.h"
#include <stdio.h>
#include <assert.h>
#include "baseds.h"
#include "field.h"
#include "log.h"
#include "world.h"

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


typedef struct {
    uint32_t component_mask;
    SystemFunction function;
    SystemPreUpdateFunction pre_update_function;
    SystemPostUpdateFunction post_update_function;
    char name[32];
} System;

typedef struct {
    System systems[MAX_SYSTEMS];
    uint32_t system_count;
} SystemRegistry;

typedef struct {
    char name[32];
    uint32_t index;
    uint32_t bit_flag;
    size_t data_size;
} ComponentRegistryEntry;

typedef struct {
    ComponentRegistryEntry component_info[MAX_COMPONENTS];
    void *component_data[MAX_COMPONENTS][MAX_ENTITIES];
    uint32_t component_active[MAX_ENTITIES];
    uint32_t component_count;
    bool initialized;

} ComponentRegistry;

typedef struct _ecs_state {
    stack active_entities;
    stack inactive_entities;    
    ComponentRegistry components;
    SystemRegistry systems;
    bool initialized;
} ecs_state;

static ecs_state g_ecs_state = {0};




// Global quit flag
static bool g_quit_requested = false;

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
static bool entity_is_active(Entity entity) {
    ll_node *current = g_ecs_state.active_entities.list.head;
    while (current != NULL) {
        Entity *active_entity = (Entity *)current->data;
        if (*active_entity == entity) {
            return true;
        }
        current = current->next;
    }
    return false;
}

// Helper function to remove entity from active stack
static void entity_remove_from_active(Entity entity) {
    ll_node *current = g_ecs_state.active_entities.list.head;
    ll_node *prev = NULL;
    
    while (current != NULL) {
        Entity *active_entity = (Entity *)current->data;
        if (*active_entity == entity) {
            // Remove this node
            if (prev == NULL) {
                // First node
                g_ecs_state.active_entities.list.head = current->next;
                if (g_ecs_state.active_entities.list.head == NULL) {
                    g_ecs_state.active_entities.list.tail = NULL;
                }
            } else {
                prev->next = current->next;
                if (current->next == NULL) {
                    g_ecs_state.active_entities.list.tail = prev;
                }
            }
            free(current);
            g_ecs_state.active_entities.list.size--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

void ecs_init(void) {
    // Initialize component active masks
    for (int i = 0; i < MAX_ENTITIES; i++) {
        g_ecs_state.components.component_active[i] = 0;
    }
    
    // Initialize component count
    g_ecs_state.components.component_count = 0;
    
    // Initialize system count
    g_ecs_state.systems.system_count = 0;
    
    // Register components - all components must be registered during ecs_init
    component_register("Position", sizeof(Position));
    component_register("BaseInfo", sizeof(BaseInfo));
    component_register("Action", sizeof(Action));
    component_register("FieldOfView", sizeof(CompactFieldOfView));
    component_register("Actor", sizeof(Actor));
    component_register("Inventory", sizeof(Inventory));

    // Allocate memory for component data
    size_t total_memory = 0;
    for (uint32_t i = 0; i < g_ecs_state.components.component_count; i++) {
        size_t component_memory = g_ecs_state.components.component_info[i].data_size * MAX_ENTITIES;
        total_memory += component_memory;
        
        for (uint32_t j = 0; j < MAX_ENTITIES; j++) {
            g_ecs_state.components.component_data[i][j] = malloc(g_ecs_state.components.component_info[i].data_size);
            if (!g_ecs_state.components.component_data[i][j]) {
                LOG_ERROR("Failed to allocate memory for component %s", g_ecs_state.components.component_info[i].name);
                return;
            }
        }
        
        LOG_INFO("Allocated %zu bytes for component '%s' (%zu bytes per entity × %d entities)", 
                 component_memory, 
                 g_ecs_state.components.component_info[i].name,
                 g_ecs_state.components.component_info[i].data_size,
                 MAX_ENTITIES);
    }
    
    LOG_INFO("Total component memory allocated: %zu bytes (%.2f MB)", 
             total_memory, 
             (double)total_memory / (1024 * 1024));

    // Initialize entity stacks
    stack_init(&g_ecs_state.active_entities, sizeof(Entity));
    stack_init(&g_ecs_state.inactive_entities, sizeof(Entity));
    
    // Push all entity IDs to inactive stack (in reverse order so they pop in order)
    for (int i = MAX_ENTITIES - 1; i >= 0; i--) {
        Entity entity_id = i;
        stack_push(&g_ecs_state.inactive_entities, &entity_id);
    }

    g_ecs_state.initialized = true;
    LOG_INFO("ECS initialized with %d components", g_ecs_state.components.component_count);

    // now lets initialize the systems
}

void ecs_shutdown(void) {
    // Free all component data
    for (uint32_t i = 0; i < g_ecs_state.components.component_count; i++) {
        for (uint32_t j = 0; j < MAX_ENTITIES; j++) {
            if (g_ecs_state.components.component_data[i][j]) {
                free(g_ecs_state.components.component_data[i][j]);
                g_ecs_state.components.component_data[i][j] = NULL;
            }
        }
    }
    
    LOG_INFO("ECS shutdown complete");
}

Entity entity_create(void) {
    if (!g_ecs_state.initialized) {
        LOG_ERROR("ECS not initialized");
        return INVALID_ENTITY;
    }
    
    // Check if we have any inactive entities
    if (stack_empty(&g_ecs_state.inactive_entities)) {
        LOG_ERROR("Maximum entities reached");
        return INVALID_ENTITY;
    }
    
    // Pop entity ID from inactive stack
    Entity *entity_ptr = (Entity *)stack_top(&g_ecs_state.inactive_entities);
    Entity entity_id = *entity_ptr;
    stack_pop(&g_ecs_state.inactive_entities);
    
    // Add to active stack
    stack_push(&g_ecs_state.active_entities, &entity_id);
    
    return entity_id;
}

void entity_destroy(Entity entity) {
    if (entity >= MAX_ENTITIES || !entity_is_active(entity)) return;
    
    // Clear all component flags for this entity
    g_ecs_state.components.component_active[entity] = 0;
    
    // Remove from active stack
    entity_remove_from_active(entity);
    
    // Add to inactive stack
    stack_push(&g_ecs_state.inactive_entities, &entity);
}

bool entity_exists(Entity entity) {
    return entity < MAX_ENTITIES && entity_is_active(entity);
}

void * entity_get_component(Entity entity, uint32_t component_id) {

    // return the component data if the entity has the component
    if (g_ecs_state.components.component_active[entity] & g_ecs_state.components.component_info[component_id].bit_flag) {
        return g_ecs_state.components.component_data[component_id][entity];
    }

    return NULL;
}

uint32_t component_register(const char *name, size_t size) {

    if (g_ecs_state.initialized) {
        LOG_ERROR("Components must be registered during ecs_init");
        return INVALID_ENTITY;
    }

    if (g_ecs_state.components.component_count >= MAX_COMPONENTS) {
        LOG_ERROR("Maximum components reached");
        return INVALID_ENTITY;
    }
    
    // Check if component already exists
    for (unsigned int i = 0; i < g_ecs_state.components.component_count; i++) {
        if (strcmp_ci(g_ecs_state.components.component_info[i].name, name) == 0) {
            return g_ecs_state.components.component_info[i].index;
        }
    }
    
    // Register new component
    uint32_t index = g_ecs_state.components.component_count;
    strncpy(g_ecs_state.components.component_info[index].name, name, sizeof(g_ecs_state.components.component_info[index].name) - 1);
    g_ecs_state.components.component_info[index].name[sizeof(g_ecs_state.components.component_info[index].name) - 1] = '\0';
    g_ecs_state.components.component_info[index].index = index;
    g_ecs_state.components.component_info[index].bit_flag = 1 << index;
    g_ecs_state.components.component_info[index].data_size = size;
    
    LOG_INFO("Registered component: %s (ID: %d, Size: %zu)", name, index, size);
    
    g_ecs_state.components.component_count++;
    return index;
}

uint32_t component_get_id(const char *name) {
    for (unsigned int i = 0; i < g_ecs_state.components.component_count; i++) {
        if (strcmp_ci(g_ecs_state.components.component_info[i].name, name) == 0) {
            return g_ecs_state.components.component_info[i].index;
        }
    }
    return INVALID_ENTITY;
}


bool component_add(Entity entity, uint32_t component_id, void *data) {
    if (entity >= MAX_ENTITIES || !entity_is_active(entity) || 
        component_id >= MAX_COMPONENTS || component_id >= g_ecs_state.components.component_count) {
        return false;
    }
    
    // Copy data to component storage
    memcpy(g_ecs_state.components.component_data[component_id][entity], data, g_ecs_state.components.component_info[component_id].data_size);
    
    // Set component flag
    g_ecs_state.components.component_active[entity] |= g_ecs_state.components.component_info[component_id].bit_flag;
    
    return true;
}

bool component_remove(Entity entity, uint32_t component_id) {
    if (entity >= MAX_ENTITIES || !entity_is_active(entity) || 
        component_id >= MAX_COMPONENTS || component_id >= g_ecs_state.components.component_count) {
        return false;
    }
    
    // Check if component is active
    if (g_ecs_state.components.component_active[entity] & g_ecs_state.components.component_info[component_id].bit_flag) {
        // Clear component flag
        g_ecs_state.components.component_active[entity] &= ~g_ecs_state.components.component_info[component_id].bit_flag;
        return true;
    }
    
    return false;
}

void *component_get(Entity entity, uint32_t component_id) {
    if (entity >= MAX_ENTITIES || !entity_is_active(entity) || 
        component_id >= MAX_COMPONENTS || component_id >= g_ecs_state.components.component_count) {
        return NULL;
    }
    
    // Check if component is active
    if (g_ecs_state.components.component_active[entity] & g_ecs_state.components.component_info[component_id].bit_flag) {
        return g_ecs_state.components.component_data[component_id][entity];
    }
    
    return NULL;
}

bool component_has(Entity entity, uint32_t component_id) {
    if (entity >= MAX_ENTITIES || !entity_is_active(entity) || 
        component_id >= MAX_COMPONENTS || component_id >= g_ecs_state.components.component_count) {
        return false;
    }
    return (g_ecs_state.components.component_active[entity] & g_ecs_state.components.component_info[component_id].bit_flag) != 0;
}

void system_register(const char *name, uint32_t component_mask,
    SystemFunction function, 
    SystemPreUpdateFunction pre_update_function, 
    SystemPostUpdateFunction post_update_function) {

    if (!g_ecs_state.initialized) {
        LOG_ERROR("Components must be registered before systems can be registered.");
        return;
    }

    if (g_ecs_state.systems.system_count >= MAX_SYSTEMS) {
        LOG_ERROR("Maximum systems reached");
        return;
    }
    
    strncpy(g_ecs_state.systems.systems[g_ecs_state.systems.system_count].name, name, sizeof(g_ecs_state.systems.systems[g_ecs_state.systems.system_count].name) - 1);
    g_ecs_state.systems.systems[g_ecs_state.systems.system_count].name[sizeof(g_ecs_state.systems.systems[g_ecs_state.systems.system_count].name) - 1] = '\0';
    g_ecs_state.systems.systems[g_ecs_state.systems.system_count].function = function;
    g_ecs_state.systems.systems[g_ecs_state.systems.system_count].pre_update_function = pre_update_function;
    g_ecs_state.systems.systems[g_ecs_state.systems.system_count].post_update_function = post_update_function;
    g_ecs_state.systems.systems[g_ecs_state.systems.system_count].component_mask = component_mask;
    g_ecs_state.systems.system_count++;
    LOG_INFO("Registered system: %s", name);
}

// Function to request quit (called by input system)
void ecs_request_quit(void) {
    g_quit_requested = true;
}

bool system_run_all(void *world) {
    for (uint32_t sys = 0; sys < g_ecs_state.systems.system_count; sys++) {
        System *system = &g_ecs_state.systems.systems[sys];
        
        // Call pre-update function if it exists
        if (system->pre_update_function) {
            system->pre_update_function(world);
        }
        
        // Iterate through all active entities
        ll_node *current = g_ecs_state.active_entities.list.head;
        while (current != NULL) {
            Entity entity = *(Entity *)current->data;
            
            // Check if entity has all required components
            bool has_all_components = true;
            
            for (uint32_t comp = 0; comp < g_ecs_state.components.component_count; comp++) {
                if (system->component_mask & g_ecs_state.components.component_info[comp].bit_flag) {
                    if (!(g_ecs_state.components.component_active[entity] & g_ecs_state.components.component_info[comp].bit_flag)) {
                        has_all_components = false;
                        break;
                    }
                }
            }
            
            if (has_all_components) {
                system->function(entity, world);
            }
            
            current = current->next;
        }
        
        // Call post-update function if it exists
        if (system->post_update_function) {
            system->post_update_function(world);
        }
    }
    
    // Check if quit was requested
    if (g_quit_requested) {
        g_quit_requested = false; // Reset for next frame
        return false;
    }
    
    return true;
}
