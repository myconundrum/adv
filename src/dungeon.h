#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdbool.h>
#include <stdint.h>
#include "baseds.h"
#include "types.h"

// Compile-time maximum limits (for array declarations)
// Runtime limits are controlled by configuration system
#define DUNGEON_WIDTH_COMPILE_TIME 500
#define DUNGEON_HEIGHT_COMPILE_TIME 500

// Backward compatibility defines (runtime uses config system)
#define DUNGEON_WIDTH DUNGEON_WIDTH_COMPILE_TIME
#define DUNGEON_HEIGHT DUNGEON_HEIGHT_COMPILE_TIME

// Room generation limits (runtime configurable)
#define MAX_ROOMS_COMPILE_TIME 100
#define MAX_ROOMS MAX_ROOMS_COMPILE_TIME

// Room size limits (use config system at runtime)
#define MIN_ROOM_SIZE 3
#define MAX_ROOM_SIZE 50

typedef enum {
    TILE_TYPE_WALL,
    TILE_TYPE_FLOOR,
    TILE_TYPE_DOOR,
    TILE_TYPE_WINDOW,
    TILE_TYPE_STAIRS_UP,
    TILE_TYPE_STAIRS_DOWN,
    TILE_TYPE_COUNT
} TileType;

typedef struct {
    TileType type;
    bool is_walkable;
    char symbol;
    uint8_t color;
} TileInfo;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Room;

typedef struct {
    int x;
    int y;
    TileType type;
    bool explored;
    Entity actor; // there can be only one actor per tile.
    Entity item; // there can be only one item per tile -> but this may be a stack.
    // entity_count and entity fields removed
} Tile;

typedef struct {
    int width;
    int height;
    Tile tiles[DUNGEON_WIDTH][DUNGEON_HEIGHT];
    Room rooms[MAX_ROOMS];
    int room_count;
    int stairs_up_x;
    int stairs_up_y;
    int stairs_down_x;
    int stairs_down_y;
} Dungeon;

void dungeon_init(Dungeon *dungeon);
void dungeon_generate(Dungeon *dungeon);
void dungeon_cleanup(Dungeon *dungeon);

Tile* dungeon_get_tile(Dungeon *dungeon, int x, int y);
TileInfo* dungeon_get_tile_info(TileType type);
bool dungeon_is_walkable(Dungeon *dungeon, int x, int y);

// Explored map functions
bool dungeon_is_explored(Dungeon *dungeon, int x, int y);
void dungeon_mark_explored(Dungeon *dungeon, int x, int y);

// Tile-based entity management functions
void dungeon_place_entity_at_position(Dungeon *dungeon, Entity entity, int x, int y);
void dungeon_remove_entity_from_position(Dungeon *dungeon, Entity entity, int x, int y);
bool dungeon_get_entities_at_position(Dungeon *dungeon, int x, int y, Entity *actor_out, Entity *item_out);

// Dungeon position entity management functions
// Remove all entity management function declarations

#endif
