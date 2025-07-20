#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdbool.h>
#include <stdint.h>
#include "baseds.h"
#include "types.h"

#define DUNGEON_WIDTH 100
#define DUNGEON_HEIGHT 100
#define MAX_ROOMS 20
#define MIN_ROOM_SIZE 5
#define MAX_ROOM_SIZE 15

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

// Dungeon position entity management functions
// Remove all entity management function declarations

#endif
