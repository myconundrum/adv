#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// Forward declaration  
struct AppState;

// Configuration categories for better organization
typedef struct {
    uint32_t max_entities;
    uint32_t max_components;
    uint32_t max_systems;
    uint32_t initial_component_capacity;
} ECSConfig;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t max_rooms;
    uint32_t min_room_size;
    uint32_t max_room_size;
} DungeonConfig;

typedef struct {
    uint32_t cell_size;           // pixels per cell
    uint32_t sidebar_width;       // in cells
    uint32_t game_area_width;     // in cells
    uint32_t game_area_height;    // in cells
    uint32_t status_line_height;  // in cells
    char window_title[64];
} RenderConfig;

typedef struct {
    uint32_t radius;
    uint32_t grid_size;           // calculated: radius * 2 + 1
} FieldOfViewConfig;

typedef struct {
    uint32_t cell_size;           // tiles per spatial cell
    uint32_t grid_width;          // calculated: dungeon_width / cell_size
    uint32_t grid_height;         // calculated: dungeon_height / cell_size
    uint32_t max_entities_per_cell;
} SpatialConfig;

typedef struct {
    uint32_t max_items;
} InventoryConfig;

typedef struct {
    uint32_t queue_length;
    uint32_t max_text_length;
    uint32_t max_wrapped_lines;
} MessageConfig;

typedef struct {
    uint32_t default_width;
    uint32_t default_height;
    uint32_t min_width;
    uint32_t min_height;
    uint32_t line_height;
    uint32_t margin;
    uint32_t scrollbar_width;
} MessageViewConfig;

typedef struct {
    uint32_t initial_chunks_per_pool;
    uint32_t max_chunks_per_pool;
    bool enable_corruption_detection;
    bool enable_statistics;
    bool enable_pool_allocation;     // Global enable/disable for pool allocation
} MemoryPoolConfig;

// Main configuration structure
typedef struct {
    ECSConfig ecs;
    DungeonConfig dungeon;
    RenderConfig render;
    FieldOfViewConfig fov;
    SpatialConfig spatial;
    InventoryConfig inventory;
    MessageConfig message;
    MessageViewConfig message_view;
    MemoryPoolConfig mempool;
    
    // Metadata
    bool loaded;
    char config_file_path[256];
} GameConfig;

// Configuration system functions
bool config_init(struct AppState *app_state);
void config_cleanup(struct AppState *app_state);
bool config_load_from_file(const char *filename, struct AppState *app_state);
bool config_validate(struct AppState *app_state);

// Configuration accessors
const GameConfig* config_get(struct AppState *app_state);

// Convenience accessors for frequently used values
uint32_t config_get_max_entities(struct AppState *app_state);
uint32_t config_get_dungeon_width(struct AppState *app_state);
uint32_t config_get_dungeon_height(struct AppState *app_state);
uint32_t config_get_cell_size(struct AppState *app_state);
uint32_t config_get_fov_radius(struct AppState *app_state);
uint32_t config_get_window_width_px(struct AppState *app_state);
uint32_t config_get_window_height_px(struct AppState *app_state);

// Calculated derived values
uint32_t config_get_window_width_cells(struct AppState *app_state);
uint32_t config_get_window_height_cells(struct AppState *app_state);
uint32_t config_get_fov_grid_size(struct AppState *app_state);
uint32_t config_get_spatial_grid_width(struct AppState *app_state);
uint32_t config_get_spatial_grid_height(struct AppState *app_state);

#endif
