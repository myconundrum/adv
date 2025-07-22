#include "spatial.h"
#include "appstate.h"
#include "error.h"
#include "log.h"
#include <math.h>
#include <string.h>
#include <float.h>

// ===== INITIALIZATION AND CLEANUP =====

void spatial_init(SpatialGrid *grid) {
    if (!grid) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "grid cannot be NULL");
        return;
    }

    // Initialize all cells
    for (int x = 0; x < SPATIAL_GRID_WIDTH; x++) {
        for (int y = 0; y < SPATIAL_GRID_HEIGHT; y++) {
            SpatialCell *cell = &grid->cells[x][y];
            cell->entity_count = 0;
            cell->x = x;
            cell->y = y;
            // Clear entity array
            for (uint32_t i = 0; i < MAX_ENTITIES_PER_CELL; i++) {
                cell->entities[i] = INVALID_ENTITY;
            }
        }
    }

    // Initialize performance tracking
    grid->total_queries = 0;
    grid->entities_checked = 0;
    grid->cache_hits = 0;
    grid->initialized = true;

    LOG_INFO("Spatial grid initialized: %dx%d cells, cell size: %d, max entities per cell: %d", 
             SPATIAL_GRID_WIDTH, SPATIAL_GRID_HEIGHT, SPATIAL_CELL_SIZE, MAX_ENTITIES_PER_CELL);
}

void spatial_cleanup(SpatialGrid *grid) {
    if (!grid) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "grid cannot be NULL");
        return;
    }

    if (!grid->initialized) {
        LOG_WARN("Attempting to cleanup uninitialized spatial grid");
        return;
    }

    // Print final stats before cleanup
    spatial_print_stats(grid);

    grid->initialized = false;
    LOG_INFO("Spatial grid cleaned up");
}

// ===== UTILITY FUNCTIONS =====

void spatial_get_cell_coords(float world_x, float world_y, int *cell_x, int *cell_y) {
    if (!cell_x || !cell_y) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "cell coordinate pointers cannot be NULL");
        return;
    }

    *cell_x = (int)(world_x / SPATIAL_CELL_SIZE);
    *cell_y = (int)(world_y / SPATIAL_CELL_SIZE);

    // Clamp to valid range
    if (*cell_x < 0) *cell_x = 0;
    if (*cell_x >= SPATIAL_GRID_WIDTH) *cell_x = SPATIAL_GRID_WIDTH - 1;
    if (*cell_y < 0) *cell_y = 0;
    if (*cell_y >= SPATIAL_GRID_HEIGHT) *cell_y = SPATIAL_GRID_HEIGHT - 1;
}

bool spatial_is_valid_cell(int cell_x, int cell_y) {
    return (cell_x >= 0 && cell_x < SPATIAL_GRID_WIDTH && 
            cell_y >= 0 && cell_y < SPATIAL_GRID_HEIGHT);
}

SpatialCell* spatial_get_cell(SpatialGrid *grid, int cell_x, int cell_y) {
    VALIDATE_NOT_NULL_NULL(grid, "grid");
    
    if (!spatial_is_valid_cell(cell_x, cell_y)) {
        ERROR_SET(RESULT_ERROR_OUT_OF_BOUNDS, "Cell coordinates (%d, %d) out of bounds", cell_x, cell_y);
        return NULL;
    }

    return &grid->cells[cell_x][cell_y];
}

// ===== ENTITY MANAGEMENT =====

bool spatial_add_entity(SpatialGrid *grid, Entity entity, float x, float y) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    
    if (!grid->initialized) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
    }

    if (entity == INVALID_ENTITY) {
        ERROR_RETURN_FALSE(RESULT_ERROR_ENTITY_INVALID, "Cannot add invalid entity to spatial grid");
    }

    int cell_x, cell_y;
    spatial_get_cell_coords(x, y, &cell_x, &cell_y);
    
    SpatialCell *cell = spatial_get_cell(grid, cell_x, cell_y);
    if (!cell) {
        return false; // Error already set by spatial_get_cell
    }

    // Check if cell is full
    if (cell->entity_count >= MAX_ENTITIES_PER_CELL) {
        ERROR_RETURN_FALSE(RESULT_ERROR_SYSTEM_LIMIT, 
                          "Cell (%d, %d) is full (%u entities), cannot add entity %u",
                          cell_x, cell_y, cell->entity_count, entity);
    }

    // Check if entity already exists in this cell
    for (uint32_t i = 0; i < cell->entity_count; i++) {
        if (cell->entities[i] == entity) {
            ERROR_RETURN_FALSE(RESULT_ERROR_ALREADY_EXISTS, 
                              "Entity %u already exists in cell (%d, %d)", entity, cell_x, cell_y);
        }
    }

    // Add entity to cell
    cell->entities[cell->entity_count] = entity;
    cell->entity_count++;

    LOG_DEBUG("Added entity %u to spatial cell (%d, %d) at world position (%.1f, %.1f)", 
              entity, cell_x, cell_y, x, y);
    return true;
}

bool spatial_remove_entity(SpatialGrid *grid, Entity entity, float x, float y) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    
    if (!grid->initialized) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
    }

    if (entity == INVALID_ENTITY) {
        ERROR_RETURN_FALSE(RESULT_ERROR_ENTITY_INVALID, "Cannot remove invalid entity from spatial grid");
    }

    int cell_x, cell_y;
    spatial_get_cell_coords(x, y, &cell_x, &cell_y);
    
    SpatialCell *cell = spatial_get_cell(grid, cell_x, cell_y);
    if (!cell) {
        return false; // Error already set by spatial_get_cell
    }

    // Find and remove entity from cell
    for (uint32_t i = 0; i < cell->entity_count; i++) {
        if (cell->entities[i] == entity) {
            // Move last entity to this position
            cell->entity_count--;
            cell->entities[i] = cell->entities[cell->entity_count];
            cell->entities[cell->entity_count] = INVALID_ENTITY;
            
            LOG_DEBUG("Removed entity %u from spatial cell (%d, %d)", entity, cell_x, cell_y);
            return true;
        }
    }

    ERROR_RETURN_FALSE(RESULT_ERROR_NOT_FOUND, 
                      "Entity %u not found in cell (%d, %d)", entity, cell_x, cell_y);
}

bool spatial_move_entity(SpatialGrid *grid, Entity entity, float old_x, float old_y, float new_x, float new_y) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    
    if (!grid->initialized) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
    }

    if (entity == INVALID_ENTITY) {
        ERROR_RETURN_FALSE(RESULT_ERROR_ENTITY_INVALID, "Cannot move invalid entity in spatial grid");
    }

    int old_cell_x, old_cell_y, new_cell_x, new_cell_y;
    spatial_get_cell_coords(old_x, old_y, &old_cell_x, &old_cell_y);
    spatial_get_cell_coords(new_x, new_y, &new_cell_x, &new_cell_y);

    // If entity is moving to the same cell, no spatial update needed
    if (old_cell_x == new_cell_x && old_cell_y == new_cell_y) {
        return true;
    }

    // Remove from old cell and add to new cell
    if (!spatial_remove_entity(grid, entity, old_x, old_y)) {
        return false; // Error already set
    }

    if (!spatial_add_entity(grid, entity, new_x, new_y)) {
        // Try to re-add to old cell if new cell add failed
        if (!spatial_add_entity(grid, entity, old_x, old_y)) {
            LOG_ERROR("Failed to restore entity %u to old cell after failed move", entity);
        }
        return false; // Error already set
    }

    LOG_DEBUG("Moved entity %u from cell (%d, %d) to cell (%d, %d)", 
              entity, old_cell_x, old_cell_y, new_cell_x, new_cell_y);
    return true;
}

// ===== QUERY FUNCTIONS =====

bool spatial_query_point(SpatialGrid *grid, float x, float y, SpatialQueryResult *result) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    VALIDATE_NOT_NULL_FALSE(result, "result");
    
    if (!grid->initialized) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
    }

    grid->total_queries++;

    int cell_x, cell_y;
    spatial_get_cell_coords(x, y, &cell_x, &cell_y);
    
    SpatialCell *cell = spatial_get_cell(grid, cell_x, cell_y);
    if (!cell) {
        return false; // Error already set
    }

    // Initialize result
    result->entity_count = 0;
    result->search_radius = 0.0f;
    result->center_x = cell_x;
    result->center_y = cell_y;

    // Copy entities from the cell
    for (uint32_t i = 0; i < cell->entity_count; i++) {
        if (result->entity_count < MAX_ENTITIES_PER_CELL * 9) {
            result->entities[result->entity_count] = cell->entities[i];
            result->entity_count++;
            grid->entities_checked++;
        }
    }

    return true;
}

bool spatial_query_radius(SpatialGrid *grid, float center_x, float center_y, float radius, SpatialQueryResult *result) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    VALIDATE_NOT_NULL_FALSE(result, "result");
    
    if (!grid->initialized) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
    }

    if (radius < 0.0f) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INVALID_PARAMETER, "Search radius cannot be negative: %.2f", radius);
    }

    grid->total_queries++;

    // Initialize result
    result->entity_count = 0;
    result->search_radius = radius;
    spatial_get_cell_coords(center_x, center_y, &result->center_x, &result->center_y);

    // Calculate cell range to check
    int min_cell_x = (int)((center_x - radius) / SPATIAL_CELL_SIZE);
    int max_cell_x = (int)((center_x + radius) / SPATIAL_CELL_SIZE);
    int min_cell_y = (int)((center_y - radius) / SPATIAL_CELL_SIZE);
    int max_cell_y = (int)((center_y + radius) / SPATIAL_CELL_SIZE);

    // Clamp to valid range
    if (min_cell_x < 0) min_cell_x = 0;
    if (max_cell_x >= SPATIAL_GRID_WIDTH) max_cell_x = SPATIAL_GRID_WIDTH - 1;
    if (min_cell_y < 0) min_cell_y = 0;
    if (max_cell_y >= SPATIAL_GRID_HEIGHT) max_cell_y = SPATIAL_GRID_HEIGHT - 1;

    // Check all cells in range
    for (int cell_x = min_cell_x; cell_x <= max_cell_x; cell_x++) {
        for (int cell_y = min_cell_y; cell_y <= max_cell_y; cell_y++) {
            SpatialCell *cell = &grid->cells[cell_x][cell_y];
            
            // Add all entities from this cell
            for (uint32_t i = 0; i < cell->entity_count; i++) {
                if (result->entity_count < MAX_ENTITIES_PER_CELL * 9) {
                    result->entities[result->entity_count] = cell->entities[i];
                    result->entity_count++;
                    grid->entities_checked++;
                }
            }
        }
    }

    LOG_DEBUG("Radius query at (%.1f, %.1f) with radius %.1f found %u entities in %d cells", 
              center_x, center_y, radius, result->entity_count, 
              (max_cell_x - min_cell_x + 1) * (max_cell_y - min_cell_y + 1));

    return true;
}

bool spatial_query_rect(SpatialGrid *grid, float min_x, float min_y, float max_x, float max_y, SpatialQueryResult *result) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    VALIDATE_NOT_NULL_FALSE(result, "result");
    
    if (!grid->initialized) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
    }

    if (min_x > max_x || min_y > max_y) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INVALID_PARAMETER, 
                          "Invalid rectangle bounds: min(%.1f, %.1f) max(%.1f, %.1f)", 
                          min_x, min_y, max_x, max_y);
    }

    grid->total_queries++;

    // Initialize result
    result->entity_count = 0;
    result->search_radius = 0.0f;
    spatial_get_cell_coords((min_x + max_x) / 2, (min_y + max_y) / 2, &result->center_x, &result->center_y);

    // Calculate cell range
    int min_cell_x = (int)(min_x / SPATIAL_CELL_SIZE);
    int max_cell_x = (int)(max_x / SPATIAL_CELL_SIZE);
    int min_cell_y = (int)(min_y / SPATIAL_CELL_SIZE);
    int max_cell_y = (int)(max_y / SPATIAL_CELL_SIZE);

    // Clamp to valid range
    if (min_cell_x < 0) min_cell_x = 0;
    if (max_cell_x >= SPATIAL_GRID_WIDTH) max_cell_x = SPATIAL_GRID_WIDTH - 1;
    if (min_cell_y < 0) min_cell_y = 0;
    if (max_cell_y >= SPATIAL_GRID_HEIGHT) max_cell_y = SPATIAL_GRID_HEIGHT - 1;

    // Check all cells in rectangle
    for (int cell_x = min_cell_x; cell_x <= max_cell_x; cell_x++) {
        for (int cell_y = min_cell_y; cell_y <= max_cell_y; cell_y++) {
            SpatialCell *cell = &grid->cells[cell_x][cell_y];
            
            // Add all entities from this cell
            for (uint32_t i = 0; i < cell->entity_count; i++) {
                if (result->entity_count < MAX_ENTITIES_PER_CELL * 9) {
                    result->entities[result->entity_count] = cell->entities[i];
                    result->entity_count++;
                    grid->entities_checked++;
                }
            }
        }
    }

    return true;
}

// ===== PERFORMANCE AND DEBUGGING =====

void spatial_print_stats(SpatialGrid *grid) {
    if (!grid) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "grid cannot be NULL");
        return;
    }

    if (!grid->initialized) {
        LOG_WARN("Cannot print stats for uninitialized spatial grid");
        return;
    }

    uint32_t total_entities = spatial_get_total_entities(grid);
    uint32_t occupied_cells = 0;
    uint32_t max_entities_in_cell = 0;
    uint32_t total_cell_capacity = SPATIAL_GRID_WIDTH * SPATIAL_GRID_HEIGHT * MAX_ENTITIES_PER_CELL;

    for (int x = 0; x < SPATIAL_GRID_WIDTH; x++) {
        for (int y = 0; y < SPATIAL_GRID_HEIGHT; y++) {
            SpatialCell *cell = &grid->cells[x][y];
            if (cell->entity_count > 0) {
                occupied_cells++;
                if (cell->entity_count > max_entities_in_cell) {
                    max_entities_in_cell = cell->entity_count;
                }
            }
        }
    }

    float occupancy_rate = (total_entities > 0) ? (float)total_entities / total_cell_capacity * 100.0f : 0.0f;
    float cell_utilization = (occupied_cells > 0) ? (float)occupied_cells / (SPATIAL_GRID_WIDTH * SPATIAL_GRID_HEIGHT) * 100.0f : 0.0f;

    LOG_INFO("=== Spatial Grid Statistics ===");
    LOG_INFO("Grid size: %dx%d cells (%d total)", SPATIAL_GRID_WIDTH, SPATIAL_GRID_HEIGHT, SPATIAL_GRID_WIDTH * SPATIAL_GRID_HEIGHT);
    LOG_INFO("Cell size: %dx%d world units", SPATIAL_CELL_SIZE, SPATIAL_CELL_SIZE);
    LOG_INFO("Total entities: %u", total_entities);
    LOG_INFO("Occupied cells: %u (%.1f%% utilization)", occupied_cells, cell_utilization);
    LOG_INFO("Max entities in single cell: %u", max_entities_in_cell);
    LOG_INFO("Overall occupancy: %.2f%%", occupancy_rate);
    LOG_INFO("Query performance: %u queries, %u entities checked", grid->total_queries, grid->entities_checked);
    if (grid->total_queries > 0) {
        float avg_entities_per_query = (float)grid->entities_checked / grid->total_queries;
        LOG_INFO("Average entities checked per query: %.1f", avg_entities_per_query);
    }
}

void spatial_reset_stats(SpatialGrid *grid) {
    if (!grid) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "grid cannot be NULL");
        return;
    }

    grid->total_queries = 0;
    grid->entities_checked = 0;
    grid->cache_hits = 0;
}

uint32_t spatial_get_total_entities(SpatialGrid *grid) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    
    if (!grid->initialized) {
        ERROR_SET(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
        return 0;
    }

    uint32_t total = 0;
    for (int x = 0; x < SPATIAL_GRID_WIDTH; x++) {
        for (int y = 0; y < SPATIAL_GRID_HEIGHT; y++) {
            total += grid->cells[x][y].entity_count;
        }
    }
    return total;
}

// ===== HIGH-LEVEL QUERY HELPERS =====

bool spatial_find_nearest_entity(SpatialGrid *grid, float x, float y, float max_radius, Entity *result_entity, float *result_distance) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    VALIDATE_NOT_NULL_FALSE(result_entity, "result_entity");
    VALIDATE_NOT_NULL_FALSE(result_distance, "result_distance");
    
    if (!grid->initialized) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
    }

    if (max_radius < 0.0f) {
        ERROR_RETURN_FALSE(RESULT_ERROR_INVALID_PARAMETER, "max_radius cannot be negative: %.2f", max_radius);
    }

    *result_entity = INVALID_ENTITY;
    *result_distance = FLT_MAX;

    // Start with a small radius and expand if needed
    float search_radius = SPATIAL_CELL_SIZE;
    if (search_radius > max_radius) {
        search_radius = max_radius;
    }

    SpatialQueryResult query_result;
    bool found_any = false;

    while (search_radius <= max_radius && !found_any) {
        if (!spatial_query_radius(grid, x, y, search_radius, &query_result)) {
            return false; // Error already set
        }

        // Check all entities in the query result for the nearest one
        for (uint32_t i = 0; i < query_result.entity_count; i++) {
            Entity entity = query_result.entities[i];
            if (entity == INVALID_ENTITY) continue;

            // For this example, we'll assume entities are at their cell center
            // In a real implementation, you'd get the actual position from the Position component
            int entity_cell_x, entity_cell_y;
            spatial_get_cell_coords(x, y, &entity_cell_x, &entity_cell_y);
            
            float entity_x = entity_cell_x * SPATIAL_CELL_SIZE + SPATIAL_CELL_SIZE / 2;
            float entity_y = entity_cell_y * SPATIAL_CELL_SIZE + SPATIAL_CELL_SIZE / 2;
            
            float dx = entity_x - x;
            float dy = entity_y - y;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance <= max_radius && distance < *result_distance) {
                *result_entity = entity;
                *result_distance = distance;
                found_any = true;
            }
        }

        // Expand search radius
        search_radius += SPATIAL_CELL_SIZE;
    }

    return found_any;
}

uint32_t spatial_count_entities_in_radius(SpatialGrid *grid, float x, float y, float radius) {
    VALIDATE_NOT_NULL_FALSE(grid, "grid");
    
    if (!grid->initialized) {
        ERROR_SET(RESULT_ERROR_INITIALIZATION_FAILED, "Spatial grid not initialized");
        return 0;
    }

    if (radius < 0.0f) {
        ERROR_SET(RESULT_ERROR_INVALID_PARAMETER, "radius cannot be negative: %.2f", radius);
        return 0;
    }

    SpatialQueryResult result;
    if (!spatial_query_radius(grid, x, y, radius, &result)) {
        return 0; // Error already set
    }

    // In a more sophisticated implementation, you would filter by actual distance
    // For now, we return the count from the spatial query
    return result.entity_count;
} 
