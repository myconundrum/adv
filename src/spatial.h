#ifndef SPATIAL_H
#define SPATIAL_H

#include <stdbool.h>
#include <stdint.h>
#include "types.h"
#include "baseds.h"

// Spatial partitioning configuration
#define SPATIAL_CELL_SIZE 10           // Each cell covers 10x10 tiles
#define SPATIAL_GRID_WIDTH 10          // 100 / 10 = 10 cells wide
#define SPATIAL_GRID_HEIGHT 10         // 100 / 10 = 10 cells high
#define MAX_ENTITIES_PER_CELL 32       // Maximum entities per spatial cell

// Spatial partitioning structures
typedef struct {
    Entity entities[MAX_ENTITIES_PER_CELL];
    uint32_t entity_count;
    int x, y;  // Grid coordinates of this cell
} SpatialCell;

typedef struct {
    SpatialCell cells[SPATIAL_GRID_WIDTH][SPATIAL_GRID_HEIGHT];
    bool initialized;
    
    // Performance tracking
    uint32_t total_queries;
    uint32_t entities_checked;
    uint32_t cache_hits;
} SpatialGrid;

// Query results structure
typedef struct {
    Entity entities[MAX_ENTITIES_PER_CELL * 9];  // Up to 9 cells worth of entities
    uint32_t entity_count;
    float search_radius;
    int center_x, center_y;
} SpatialQueryResult;

// Spatial partitioning functions
void spatial_init(SpatialGrid *grid);
void spatial_cleanup(SpatialGrid *grid);

// Entity management
bool spatial_add_entity(SpatialGrid *grid, Entity entity, float x, float y);
bool spatial_remove_entity(SpatialGrid *grid, Entity entity, float x, float y);
bool spatial_move_entity(SpatialGrid *grid, Entity entity, float old_x, float old_y, float new_x, float new_y);

// Query functions
bool spatial_query_point(SpatialGrid *grid, float x, float y, SpatialQueryResult *result);
bool spatial_query_radius(SpatialGrid *grid, float center_x, float center_y, float radius, SpatialQueryResult *result);
bool spatial_query_rect(SpatialGrid *grid, float min_x, float min_y, float max_x, float max_y, SpatialQueryResult *result);

// Utility functions
void spatial_get_cell_coords(float world_x, float world_y, int *cell_x, int *cell_y);
bool spatial_is_valid_cell(int cell_x, int cell_y);
SpatialCell* spatial_get_cell(SpatialGrid *grid, int cell_x, int cell_y);

// Performance and debugging
void spatial_print_stats(SpatialGrid *grid);
void spatial_reset_stats(SpatialGrid *grid);
uint32_t spatial_get_total_entities(SpatialGrid *grid);

// High-level query helpers
bool spatial_find_nearest_entity(SpatialGrid *grid, float x, float y, float max_radius, Entity *result_entity, float *result_distance);
uint32_t spatial_count_entities_in_radius(SpatialGrid *grid, float x, float y, float radius);

#endif // SPATIAL_H 
