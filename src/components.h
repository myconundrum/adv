#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "field.h"

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

// Component initialization function
void components_init(void);

#endif
