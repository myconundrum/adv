#include "field.h"
#include "dungeon.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// Initialize field of view component
void field_init(FieldOfView *fov, int radius) {
    if (!fov) return;
    
    fov->radius = radius;
    
    // Initialize all tiles as not visible and not explored
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            fov->visible[x][y] = false;
            fov->explored[x][y] = false;
        }
    }
}

// Initialize compact field of view component
void field_init_compact(CompactFieldOfView *fov, int radius) {
    if (!fov) return;
    
    fov->radius = radius;
    fov->center_x = 0;
    fov->center_y = 0;
    
    // Initialize compact visible grid
    for (int y = 0; y < FOV_GRID_SIZE; y++) {
        for (int x = 0; x < FOV_GRID_SIZE; x++) {
            fov->visible[x][y] = false;
        }
    }
}

// Initialize compact field of view component (allocates memory)
CompactFieldOfView *init_compact_field_of_view(int radius) {
    CompactFieldOfView *fov = malloc(sizeof(CompactFieldOfView));
    if (fov) {
        field_init_compact(fov, radius);
    }
    return fov;
}

// Check if a position is within bounds
static bool is_in_bounds(int x, int y) {
    return x >= 0 && x < DUNGEON_WIDTH && y >= 0 && y < DUNGEON_HEIGHT;
}

// Check if a position is within compact FOV bounds
static bool is_in_compact_bounds(int x, int y) {
    return x >= 0 && x < FOV_GRID_SIZE && y >= 0 && y < FOV_GRID_SIZE;
}

// Convert world coordinates to compact FOV coordinates
static void world_to_compact_coords(CompactFieldOfView *fov, int world_x, int world_y, int *compact_x, int *compact_y) {
    *compact_x = world_x - fov->center_x + fov->radius;
    *compact_y = world_y - fov->center_y + fov->radius;
}



// Check if a tile blocks line of sight
static bool blocks_sight(Dungeon *dungeon, int x, int y) {
    if (!dungeon || !is_in_bounds(x, y)) {
        return true; // Out of bounds blocks sight
    }
    
    Tile *tile = dungeon_get_tile(dungeon, x, y);
    if (!tile) return true;
    
    // Walls block sight, floors don't
    return tile->type == TILE_TYPE_WALL;
}

// Cast a ray from start to end and mark visible tiles
static void cast_ray(FieldOfView *fov, Dungeon *dungeon, int start_x, int start_y, 
                    int end_x, int end_y) {
    int dx = abs(end_x - start_x);
    int dy = abs(end_y - start_y);
    int sx = (start_x < end_x) ? 1 : -1;
    int sy = (start_y < end_y) ? 1 : -1;
    int err = dx - dy;
    
    int x = start_x;
    int y = start_y;
    
    while (true) {
        // Check if we're within bounds and radius
        if (!is_in_bounds(x, y)) {
            break;
        }
        
        // Calculate distance from start
        int distance = (int)sqrt((x - start_x) * (x - start_x) + (y - start_y) * (y - start_y));
        if (distance > fov->radius) {
            break;
        }
        
        // Mark as visible and explored
        fov->visible[x][y] = true;
        fov->explored[x][y] = true;
        
        // Check if we've reached the end point
        if (x == end_x && y == end_y) {
            break;
        }
        
        // Check if this tile blocks sight
        if (blocks_sight(dungeon, x, y)) {
            break;
        }
        
        // Move along the ray
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

// Cast a ray for compact FOV
static void cast_ray_compact(CompactFieldOfView *fov, Dungeon *dungeon, int start_x, int start_y, 
                           int end_x, int end_y) {
    int dx = abs(end_x - start_x);
    int dy = abs(end_y - start_y);
    int sx = (start_x < end_x) ? 1 : -1;
    int sy = (start_y < end_y) ? 1 : -1;
    int err = dx - dy;
    
    int x = start_x;
    int y = start_y;
    
    while (true) {
        // Check if we're within bounds and radius
        if (!is_in_bounds(x, y)) {
            break;
        }
        
        // Calculate distance from start
        int distance = (int)sqrt((x - start_x) * (x - start_x) + (y - start_y) * (y - start_y));
        if (distance > fov->radius) {
            break;
        }
        
        // Convert to compact coordinates
        int compact_x, compact_y;
        world_to_compact_coords(fov, x, y, &compact_x, &compact_y);
        
        // Mark as visible in compact grid if within bounds
        if (is_in_compact_bounds(compact_x, compact_y)) {
            fov->visible[compact_x][compact_y] = true;
        }
        
        // Mark as explored in dungeon tile
        dungeon_mark_explored(dungeon, x, y);
        
        // Check if we've reached the end point
        if (x == end_x && y == end_y) {
            break;
        }
        
        // Check if this tile blocks sight
        if (blocks_sight(dungeon, x, y)) {
            break;
        }
        
        // Move along the ray
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

// Calculate field of view using raycasting
void field_calculate_fov(FieldOfView *fov, Dungeon *dungeon, int start_x, int start_y) {
    if (!fov || !dungeon) return;
    
    // Clear current visibility
    field_clear_visibility(fov);
    
    // Cast rays in all directions
    for (int angle = 0; angle < 360; angle += 5) { // 5-degree increments
        double rad = angle * M_PI / 180.0;
        int end_x = start_x + (int)(fov->radius * cos(rad));
        int end_y = start_y + (int)(fov->radius * sin(rad));
        
        cast_ray(fov, dungeon, start_x, start_y, end_x, end_y);
    }
    
    // Also cast rays to the 8 cardinal and diagonal directions for better coverage
    int directions[8][2] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0},          {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    };
    
    for (int i = 0; i < 8; i++) {
        int end_x = start_x + directions[i][0] * fov->radius;
        int end_y = start_y + directions[i][1] * fov->radius;
        cast_ray(fov, dungeon, start_x, start_y, end_x, end_y);
    }
}

// Calculate compact field of view using raycasting
void field_calculate_fov_compact(CompactFieldOfView *fov, Dungeon *dungeon, int start_x, int start_y) {
    if (!fov || !dungeon) return;
    
    // Update center position
    fov->center_x = start_x;
    fov->center_y = start_y;
    
    // Clear current visibility
    field_clear_visibility_compact(fov);
    
    // Cast rays in all directions
    for (int angle = 0; angle < 360; angle += 5) { // 5-degree increments
        double rad = angle * M_PI / 180.0;
        int end_x = start_x + (int)(fov->radius * cos(rad));
        int end_y = start_y + (int)(fov->radius * sin(rad));
        
        cast_ray_compact(fov, dungeon, start_x, start_y, end_x, end_y);
    }
    
    // Also cast rays to the 8 cardinal and diagonal directions for better coverage
    int directions[8][2] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0},          {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    };
    
    for (int i = 0; i < 8; i++) {
        int end_x = start_x + directions[i][0] * fov->radius;
        int end_y = start_y + directions[i][1] * fov->radius;
        cast_ray_compact(fov, dungeon, start_x, start_y, end_x, end_y);
    }
}

// Check if a position is visible
bool field_is_visible(FieldOfView *fov, int x, int y) {
    if (!fov || !is_in_bounds(x, y)) {
        return false;
    }
    return fov->visible[x][y];
}

// Check if a position is visible (compact version)
bool field_is_visible_compact(CompactFieldOfView *fov, int x, int y) {
    if (!fov || !is_in_bounds(x, y)) {
        return false;
    }
    
    // Convert world coordinates to compact coordinates
    int compact_x, compact_y;
    world_to_compact_coords(fov, x, y, &compact_x, &compact_y);
    
    // Check if within compact grid bounds
    if (!is_in_compact_bounds(compact_x, compact_y)) {
        return false;
    }
    
    return fov->visible[compact_x][compact_y];
}

// Check if a position has been explored
bool field_is_explored(FieldOfView *fov, int x, int y) {
    if (!fov || !is_in_bounds(x, y)) {
        return false;
    }
    return fov->explored[x][y];
}

// Check if a position has been explored (compact version)
bool field_is_explored_compact(CompactFieldOfView *fov, int x, int y) {
    if (!fov || !is_in_bounds(x, y)) {
        return false;
    }
    // This function now needs a dungeon parameter, but we'll handle it differently
    // For now, return false - the render system will handle this
    return false;
}

// Mark a position as explored
void field_mark_explored(FieldOfView *fov, int x, int y) {
    if (!fov || !is_in_bounds(x, y)) {
        return;
    }
    fov->explored[x][y] = true;
}

// Mark a position as explored (compact version)
void field_mark_explored_compact(CompactFieldOfView *fov, int x, int y) {
    // This function now needs a dungeon parameter, but we'll handle it differently
    // The explored state is now managed by the dungeon
    (void)fov;
    (void)x;
    (void)y;
}

// Clear all visibility
void field_clear_visibility(FieldOfView *fov) {
    if (!fov) return;
    
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            fov->visible[x][y] = false;
        }
    }
}

// Clear all visibility (compact version)
void field_clear_visibility_compact(CompactFieldOfView *fov) {
    if (!fov) return;
    
    for (int y = 0; y < FOV_GRID_SIZE; y++) {
        for (int x = 0; x < FOV_GRID_SIZE; x++) {
            fov->visible[x][y] = false;
        }
    }
}

// Get visibility status for rendering
// Returns: 0 = not visible, 1 = visible, 2 = explored but not visible
uint8_t field_get_visibility_status(FieldOfView *fov, int x, int y) {
    if (!fov || !is_in_bounds(x, y)) {
        return 0;
    }
    
    if (fov->visible[x][y]) {
        return 1; // Currently visible
    } else if (fov->explored[x][y]) {
        return 2; // Explored but not currently visible
    } else {
        return 0; // Not visible and not explored
    }
}

// Get visibility status for rendering (compact version)
uint8_t field_get_visibility_status_compact(CompactFieldOfView *fov, int x, int y) {
    if (!fov || !is_in_bounds(x, y)) {
        return 0;
    }
    
    if (field_is_visible_compact(fov, x, y)) {
        return 1; // Currently visible
    } else {
        return 0; // Not visible (explored state handled by render system)
    }
}

// Cleanup field of view component
void field_cleanup(FieldOfView *fov) {
    // Nothing to clean up for this implementation
    (void)fov;
}

// Cleanup compact field of view component
void field_cleanup_compact(CompactFieldOfView *fov) {
    // Nothing to clean up for this implementation
    (void)fov;
} 
