#ifndef ECS_H
#define ECS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "types.h"
#include "field.h"

#define MAX_ENTITIES 1000

// Component system
#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 32

#define MAX_INVENTORY_ITEMS 40 // max number of items in inventory.

// Component types
typedef struct {
    int x;
    int y;
    bool moved;
    Entity entity; // if object is in inventory, this is the entity carrying. otherwise this should be 
                   // invalid entity.
} Position;

typedef struct {
    
    char character;
    uint8_t color;
    char name[32];
    bool is_carryable;
    
} BaseInfo;


typedef struct {
    bool is_player;          // is the actor the player?
    bool can_carry;          // can the actor carry items?
    uint8_t energy;          // current energy the actor has.
    uint8_t energy_per_turn; // energy gained per turn.
    uint32_t hp;             // current health points.
    uint16_t strength;       // strength of the actor.
    uint16_t attack;         // attack power of the actor.
    uint8_t attack_bonus;    // attack bonus of the actor.
    uint16_t defense;        // defense power of the actor.
    uint8_t defense_bonus;   // defense bonus of the actor.
    uint8_t damage_dice;     // damage dice of the actor.
    uint8_t damage_sides;    // damage sides of the actor.
    uint8_t damage_bonus;    // damage bonus of the actor.
} Actor;

typedef struct {
    uint8_t max_items;       // maximum number of items the object can carry.
    uint8_t max_weight;      // maximum weight the object can carry.
    uint8_t max_volume;      // maximum volume the object can carry.
    uint8_t item_count;      // current number of items in the inventory.
    Entity items[MAX_INVENTORY_ITEMS]; // items in the inventory.
} Inventory;

typedef enum ActionType {
    ACTION_MOVE,
    ACTION_QUIT,
    ACTION_NONE
} ActionType;

typedef enum Direction {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_NONE
} Direction;

typedef struct Action {
    enum ActionType type;
    int action_data;
} Action;




// System function pointer types
typedef void (*SystemFunction)(Entity entity, void *world);

// call before the system functions are called per entity.
typedef void (*SystemPreUpdateFunction)(void *world);
// call after the system functions are called per entity.
typedef void (*SystemPostUpdateFunction)(void *world);

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

bool system_run_all(void *world);

// Quit management
void ecs_request_quit(void);

// Utility functions
int strcmp_ci(const char *s1, const char *s2);



#endif
