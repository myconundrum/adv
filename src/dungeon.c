#include "dungeon.h"
#include "log.h"
#include "error.h"
#include "components.h"
#include "appstate.h"
#include "ecs.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Tile information lookup table
static TileInfo tile_info_table[TILE_TYPE_COUNT];

// Initialize tile info table
static void init_tile_info_table(void) {
    // Initialize basic tile info
    tile_info_table[TILE_TYPE_WALL] = (TileInfo){.type = TILE_TYPE_WALL, .is_walkable = false, .symbol = '#', .color = 0x07};
    tile_info_table[TILE_TYPE_FLOOR] = (TileInfo){.type = TILE_TYPE_FLOOR, .is_walkable = true, .symbol = '.', .color = 0x07};
    tile_info_table[TILE_TYPE_DOOR] = (TileInfo){.type = TILE_TYPE_DOOR, .is_walkable = true, .symbol = '+', .color = 0x06};
    tile_info_table[TILE_TYPE_WINDOW] = (TileInfo){.type = TILE_TYPE_WINDOW, .is_walkable = false, .symbol = '=', .color = 0x06};
    tile_info_table[TILE_TYPE_STAIRS_UP] = (TileInfo){.type = TILE_TYPE_STAIRS_UP, .is_walkable = true, .symbol = '<', .color = 0x04};
    tile_info_table[TILE_TYPE_STAIRS_DOWN] = (TileInfo){.type = TILE_TYPE_STAIRS_DOWN, .is_walkable = true, .symbol = '>', .color = 0x04};
    
    // Entity stacks will be initialized when tiles are created
}

// Helper function to check if a room overlaps with existing rooms
static bool room_overlaps(Room *new_room, Room *rooms, int room_count) {
    for (int i = 0; i < room_count; i++) {
        // Check if rooms overlap (with 1 tile buffer)
        if (new_room->x < rooms[i].x + rooms[i].width + 1 &&
            new_room->x + new_room->width + 1 > rooms[i].x &&
            new_room->y < rooms[i].y + rooms[i].height + 1 &&
            new_room->y + new_room->height + 1 > rooms[i].y) {
            return true;
        }
    }
    return false;
}

// Helper function to create a room
static void create_room(Dungeon *dungeon, int x, int y, int width, int height) {
    // Fill the room with floor tiles
    for (int i = x; i < x + width; i++) {
        for (int j = y; j < y + height; j++) {
            if (i >= 0 && i < DUNGEON_WIDTH && j >= 0 && j < DUNGEON_HEIGHT) {
                dungeon->tiles[i][j].x = i;
                dungeon->tiles[i][j].y = j;
                dungeon->tiles[i][j].type = TILE_TYPE_FLOOR;
            }
        }
    }
}

// Helper function to create a horizontal corridor
static void create_h_corridor(Dungeon *dungeon, int x1, int x2, int y) {
    int start = (x1 < x2) ? x1 : x2;
    int end = (x1 < x2) ? x2 : x1;
    
    for (int x = start; x <= end; x++) {
        if (x >= 0 && x < DUNGEON_WIDTH && y >= 0 && y < DUNGEON_HEIGHT) {
            dungeon->tiles[x][y].x = x;
            dungeon->tiles[x][y].y = y;
            dungeon->tiles[x][y].type = TILE_TYPE_FLOOR;
        }
    }
}

// Helper function to create a vertical corridor
static void create_v_corridor(Dungeon *dungeon, int y1, int y2, int x) {
    int start = (y1 < y2) ? y1 : y2;
    int end = (y1 < y2) ? y2 : y1;
    
    for (int y = start; y <= end; y++) {
        if (x >= 0 && x < DUNGEON_WIDTH && y >= 0 && y < DUNGEON_HEIGHT) {
            dungeon->tiles[x][y].x = x;
            dungeon->tiles[x][y].y = y;
            dungeon->tiles[x][y].type = TILE_TYPE_FLOOR;
        }
    }
}

// Helper function to connect two rooms with corridors
static void connect_rooms(Dungeon *dungeon, Room *room1, Room *room2) {
    // Get center points of both rooms
    int x1 = room1->x + room1->width / 2;
    int y1 = room1->y + room1->height / 2;
    int x2 = room2->x + room2->width / 2;
    int y2 = room2->y + room2->height / 2;
    
    // Randomly choose corridor pattern (L-shaped or straight)
    if (rand() % 2 == 0) {
        // L-shaped corridor: horizontal then vertical
        create_h_corridor(dungeon, x1, x2, y1);
        create_v_corridor(dungeon, y1, y2, x2);
    } else {
        // L-shaped corridor: vertical then horizontal
        create_v_corridor(dungeon, y1, y2, x1);
        create_h_corridor(dungeon, x1, x2, y2);
    }
}

void dungeon_init(Dungeon *dungeon) {
    // Initialize tile info table
    init_tile_info_table();
    
    // Initialize dungeon dimensions
    dungeon->width = DUNGEON_WIDTH;
    dungeon->height = DUNGEON_HEIGHT;
    dungeon->room_count = 0;
    
    // Initialize all tiles as walls
    for (int x = 0; x < DUNGEON_WIDTH; x++) {
        for (int y = 0; y < DUNGEON_HEIGHT; y++) {
            dungeon->tiles[x][y].x = x;
            dungeon->tiles[x][y].y = y;
            dungeon->tiles[x][y].type = TILE_TYPE_WALL;
            dungeon->tiles[x][y].explored = false;
            dungeon->tiles[x][y].actor = INVALID_ENTITY;
            dungeon->tiles[x][y].item = INVALID_ENTITY;
        }
    }
    
    // Initialize room array
    memset(dungeon->rooms, 0, sizeof(dungeon->rooms));
    
    // Initialize stairs positions
    dungeon->stairs_up_x = -1;
    dungeon->stairs_up_y = -1;
    dungeon->stairs_down_x = -1;
    dungeon->stairs_down_y = -1;
}

void dungeon_generate(Dungeon *dungeon) {
    // Seed random number generator
    srand(time(NULL));
    
    // Generate rooms
    int attempts = 0;
    const int max_attempts = 1000;
    
    while (dungeon->room_count < MAX_ROOMS && attempts < max_attempts) {
        // Generate random room dimensions
        int width = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
        int height = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
        
        // Generate random position (with some margin from edges)
        int x = 2 + rand() % (DUNGEON_WIDTH - width - 4);
        int y = 2 + rand() % (DUNGEON_HEIGHT - height - 4);
        
        // Create temporary room for overlap checking
        Room temp_room = {x, y, width, height};
        
        // Check if room overlaps with existing rooms
        if (!room_overlaps(&temp_room, dungeon->rooms, dungeon->room_count)) {
            // Add room to dungeon
            dungeon->rooms[dungeon->room_count] = temp_room;
            
            // Create the room in the dungeon
            create_room(dungeon, x, y, width, height);
            
            dungeon->room_count++;
        }
        
        attempts++;
    }
    
    // Connect rooms with corridors
    for (int i = 0; i < dungeon->room_count - 1; i++) {
        connect_rooms(dungeon, &dungeon->rooms[i], &dungeon->rooms[i + 1]);
    }
    
    // Add some additional random connections for more interesting layout
    for (int i = 0; i < dungeon->room_count / 2; i++) {
        int room1 = rand() % dungeon->room_count;
        int room2 = rand() % dungeon->room_count;
        
        if (room1 != room2) {
            connect_rooms(dungeon, &dungeon->rooms[room1], &dungeon->rooms[room2]);
        }
    }
    
    // Place stairs up in the first room
    if (dungeon->room_count > 0) {
        Room *first_room = &dungeon->rooms[0];
        dungeon->stairs_up_x = first_room->x + first_room->width / 2;
        dungeon->stairs_up_y = first_room->y + first_room->height / 2;
        
        dungeon->tiles[dungeon->stairs_up_x][dungeon->stairs_up_y].type = TILE_TYPE_STAIRS_UP;
    }
    
    // Place stairs down in the last room
    if (dungeon->room_count > 1) {
        Room *last_room = &dungeon->rooms[dungeon->room_count - 1];
        dungeon->stairs_down_x = last_room->x + last_room->width / 2;
        dungeon->stairs_down_y = last_room->y + last_room->height / 2;
        
        dungeon->tiles[dungeon->stairs_down_x][dungeon->stairs_down_y].type = TILE_TYPE_STAIRS_DOWN;
    }
    
    // Add some doors at room entrances for variety
    for (int i = 0; i < dungeon->room_count; i++) {
        Room *room = &dungeon->rooms[i];
        
        // Add doors at room entrances (simplified - just add some doors randomly)
        if (rand() % 3 == 0) { // 33% chance
            int door_x = room->x + rand() % room->width;
            int door_y = room->y + rand() % room->height;
            
            if (door_x >= 0 && door_x < DUNGEON_WIDTH && door_y >= 0 && door_y < DUNGEON_HEIGHT) {
                dungeon->tiles[door_x][door_y].type = TILE_TYPE_DOOR;
            }
        }
    }
}

void dungeon_cleanup(Dungeon *dungeon) {
    // Nothing to clean up for now, but this function is available for future use
    (void)dungeon;
}

Tile* dungeon_get_tile(Dungeon *dungeon, int x, int y) {
    if (x >= 0 && x < DUNGEON_WIDTH && y >= 0 && y < DUNGEON_HEIGHT) {
        return &dungeon->tiles[x][y];
    }
    return NULL;
}

TileInfo* dungeon_get_tile_info(TileType type) {
    if (type >= 0 && type < TILE_TYPE_COUNT) {
        return &tile_info_table[type];
    }
    return NULL;
}

bool dungeon_is_walkable(Dungeon *dungeon, int x, int y) {
    Tile *tile = dungeon_get_tile(dungeon, x, y);
    if (tile) {
        TileInfo *info = dungeon_get_tile_info(tile->type);
        return info ? info->is_walkable : false;
    }
    return false;
}

// Check if a position has been explored
bool dungeon_is_explored(Dungeon *dungeon, int x, int y) {
    Tile *tile = dungeon_get_tile(dungeon, x, y);
    return tile ? tile->explored : false;
}

// Mark a position as explored
void dungeon_mark_explored(Dungeon *dungeon, int x, int y) {
    Tile *tile = dungeon_get_tile(dungeon, x, y);
    if (tile) {
        tile->explored = true;
    }
}

// Tile-based entity management functions
void dungeon_place_entity_at_position(Dungeon *dungeon, Entity entity, int x, int y) {
    if (!dungeon) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "dungeon cannot be NULL");
        return;
    }
    
    if (entity == INVALID_ENTITY) {
        ERROR_SET(RESULT_ERROR_ENTITY_INVALID, "Cannot place invalid entity");
        return;
    }
    
    if (x < 0 || x >= DUNGEON_WIDTH || y < 0 || y >= DUNGEON_HEIGHT) {
        ERROR_SET(RESULT_ERROR_OUT_OF_BOUNDS, "Position (%d, %d) is outside dungeon bounds (0--%d, 0--%d)", 
                  x, y, DUNGEON_WIDTH-1, DUNGEON_HEIGHT-1);
        return;
    }
    
    Tile *tile = &dungeon->tiles[x][y];
    
    // Determine entity type and place in appropriate slot
    AppState *app_state = appstate_get();
    if (!app_state) return;
    
    Actor *actor = (Actor *)entity_get_component(app_state, entity, component_get_id(app_state, "Actor"));
    if (actor) {
        tile->actor = entity;
    } else {
        tile->item = entity;
    }
}

void dungeon_remove_entity_from_position(Dungeon *dungeon, Entity entity, int x, int y) {
    if (!dungeon) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "dungeon cannot be NULL");
        return;
    }
    
    if (entity == INVALID_ENTITY) {
        ERROR_SET(RESULT_ERROR_ENTITY_INVALID, "Cannot remove invalid entity");
        return;
    }
    
    if (x < 0 || x >= DUNGEON_WIDTH || y < 0 || y >= DUNGEON_HEIGHT) {
        ERROR_SET(RESULT_ERROR_OUT_OF_BOUNDS, "Position (%d, %d) is outside dungeon bounds (0--%d, 0--%d)", 
                  x, y, DUNGEON_WIDTH-1, DUNGEON_HEIGHT-1);
        return;
    }
    
    Tile *tile = &dungeon->tiles[x][y];
    
    // Remove entity from appropriate slot
    if (tile->actor == entity) {
        tile->actor = INVALID_ENTITY;
    }
    if (tile->item == entity) {
        tile->item = INVALID_ENTITY;
    }
}

bool dungeon_get_entities_at_position(Dungeon *dungeon, int x, int y, Entity *actor_out, Entity *item_out) {
    VALIDATE_NOT_NULL_FALSE(dungeon, "dungeon");
    
    if (x < 0 || x >= DUNGEON_WIDTH || y < 0 || y >= DUNGEON_HEIGHT) {
        ERROR_SET(RESULT_ERROR_OUT_OF_BOUNDS, "Position (%d, %d) is outside dungeon bounds (0--%d, 0--%d)", 
                  x, y, DUNGEON_WIDTH-1, DUNGEON_HEIGHT-1);
        if (actor_out) *actor_out = INVALID_ENTITY;
        if (item_out) *item_out = INVALID_ENTITY;
        return false;
    }
    
    Tile *tile = &dungeon->tiles[x][y];
    
    if (actor_out) *actor_out = tile->actor;
    if (item_out) *item_out = tile->item;
    
    return (tile->actor != INVALID_ENTITY || tile->item != INVALID_ENTITY);
}
