#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "field.h"

#define MAX_INVENTORY_ITEMS 40 // max number of items in inventory.

// Entity flags - centralized in BaseInfo for commonly used properties
typedef enum {
    ENTITY_FLAG_CARRYABLE = 1 << 0,    // Can this entity be picked up?
    ENTITY_FLAG_PLAYER = 1 << 1,       // Is this entity the player?
    ENTITY_FLAG_CAN_CARRY = 1 << 2,    // Can this entity carry items?
    ENTITY_FLAG_MOVED = 1 << 3,        // Has this entity moved this turn?
    ENTITY_FLAG_ALIVE = 1 << 4,        // Is this entity alive?
    ENTITY_FLAG_HOSTILE = 1 << 5,      // Is this entity hostile?
    ENTITY_FLAG_BLOCKING = 1 << 6,     // Does this entity block movement?
    ENTITY_FLAG_VISIBLE = 1 << 7,      // Is this entity visible?
    // Room for 24 more flags in a 32-bit field
} EntityFlags;

// Helper macros for flag manipulation
#define ENTITY_HAS_FLAG(entity_flags, flag) ((entity_flags) & (flag))
#define ENTITY_SET_FLAG(entity_flags, flag) ((entity_flags) |= (flag))
#define ENTITY_CLEAR_FLAG(entity_flags, flag) ((entity_flags) &= ~(flag))
#define ENTITY_TOGGLE_FLAG(entity_flags, flag) ((entity_flags) ^= (flag))

// Convenience functions for common flag checks (declarations)
bool entity_is_player(Entity entity);
bool entity_can_carry(Entity entity);
bool entity_is_carryable(Entity entity);
bool entity_has_moved(Entity entity);
void entity_clear_moved_flag(Entity entity);

// Component types
typedef struct {
    int x;
    int y;
    Entity entity; // if object is in inventory, this is the entity carrying. otherwise this should be 
                   // invalid entity.
} Position;

typedef struct {
    char character;
    uint8_t color;
    char name[32];
    uint32_t flags;          // Bitfield for entity properties using EntityFlags
    uint8_t weight;          // Weight of the entity (for inventory management)
    uint8_t volume;          // Volume of the entity (for inventory management)
    char description[128];   // Detailed description of the entity
} BaseInfo;



typedef struct {
    uint8_t energy;          // current energy the actor has.
    uint8_t energy_per_turn; // energy gained per turn.
    uint32_t hp;             // current health points.
    uint32_t max_hp;         // maximum health points.
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

// Forward declaration
struct AppState;

// Component initialization function
void components_init(struct AppState *app_state);

#endif
