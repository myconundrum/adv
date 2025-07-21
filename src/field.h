#ifndef FIELD_H
#define FIELD_H

#include <stdbool.h>
#include <stdint.h>
#include "dungeon.h"

// Forward declaration removed - using dungeon.h definitions
// DUNGEON_WIDTH and DUNGEON_HEIGHT are defined in dungeon.h

// Field of view radius
#define FOV_RADIUS 8

// Compact FOV representation - only store visible area around entity
#define FOV_GRID_SIZE (FOV_RADIUS * 2 + 1)  // 17x17 grid for radius 8

// Field of view component structure
typedef struct {
    bool visible[DUNGEON_WIDTH][DUNGEON_HEIGHT];
    bool explored[DUNGEON_WIDTH][DUNGEON_HEIGHT];
    int radius;
} FieldOfView;

// Compact field of view component structure (alternative)
typedef struct {
    bool visible[FOV_GRID_SIZE][FOV_GRID_SIZE];
    int radius;
    int center_x, center_y;  // Center of the FOV grid
} CompactFieldOfView;

// Shared FOV system - single global instance
typedef struct {
    bool visible[DUNGEON_WIDTH][DUNGEON_HEIGHT];
    bool explored[DUNGEON_WIDTH][DUNGEON_HEIGHT];
    int radius;
    int owner_entity;  // Which entity owns this FOV
} SharedFieldOfView;

// Shared FOV component - just references the global FOV
typedef struct {
    int radius;
    bool has_fov;  // Whether this entity has FOV capability
} SharedFieldOfViewComponent;

// Initialize field of view component
void field_init(FieldOfView *fov, int radius);

// Initialize compact field of view component
void field_init_compact(CompactFieldOfView *fov, int radius);

// Initialize compact field of view component (allocates memory)
CompactFieldOfView *init_compact_field_of_view(int radius);

// Calculate field of view from a given position
void field_calculate_fov(FieldOfView *fov, Dungeon *dungeon, int start_x, int start_y);

// Calculate compact field of view from a given position
void field_calculate_fov_compact(CompactFieldOfView *fov, Dungeon *dungeon, int start_x, int start_y);

// Check if a position is visible
bool field_is_visible(FieldOfView *fov, int x, int y);

// Check if a position is visible (compact version)
bool field_is_visible_compact(CompactFieldOfView *fov, int x, int y);

// Check if a position has been explored
bool field_is_explored(FieldOfView *fov, int x, int y);

// Check if a position has been explored (compact version)
bool field_is_explored_compact(CompactFieldOfView *fov, int x, int y);

// Mark a position as explored
void field_mark_explored(FieldOfView *fov, int x, int y);

// Mark a position as explored (compact version)
void field_mark_explored_compact(CompactFieldOfView *fov, int x, int y);

// Clear all visibility
void field_clear_visibility(FieldOfView *fov);

// Clear all visibility (compact version)
void field_clear_visibility_compact(CompactFieldOfView *fov);

// Get visibility status for rendering
uint8_t field_get_visibility_status(FieldOfView *fov, int x, int y);

// Get visibility status for rendering (compact version)
uint8_t field_get_visibility_status_compact(CompactFieldOfView *fov, int x, int y);

// Initialize shared FOV system
void field_init_shared(SharedFieldOfView *shared_fov, int radius);

// Calculate shared field of view from a given position
void field_calculate_fov_shared(SharedFieldOfView *shared_fov, Dungeon *dungeon, int start_x, int start_y);

// Check if a position is visible (shared version)
bool field_is_visible_shared(SharedFieldOfView *shared_fov, int x, int y);

// Check if a position has been explored (shared version)
bool field_is_explored_shared(SharedFieldOfView *shared_fov, int x, int y);

// Get visibility status for rendering (shared version)
uint8_t field_get_visibility_status_shared(SharedFieldOfView *shared_fov, int x, int y);

// Cleanup field of view component
void field_cleanup(FieldOfView *fov);

// Cleanup compact field of view component
void field_cleanup_compact(CompactFieldOfView *fov);

// Cleanup shared field of view system
void field_cleanup_shared(SharedFieldOfView *shared_fov);

#endif
