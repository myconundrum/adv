#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>
#include "types.h"
#include "dungeon.h"
#include "messages.h"
#include "error.h"
#include "baseds.h"
#include "field.h"

// Forward declarations
struct ComponentHashEntry;
struct AppState;

// Sparse component array definition (simplified for now)
typedef struct SparseComponentArray {
    uint32_t *sparse;           // Entity -> dense index mapping (size: MAX_ENTITIES)
    uint32_t *dense_entities;   // Dense array of entity IDs that have this component
    void **dense_components;    // Dense array of component data
    uint32_t count;             // Number of components currently stored
    uint32_t capacity;          // Current capacity of dense arrays
    size_t component_size;      // Size of individual component
} SparseComponentArray;

// App states for different phases of the game
typedef enum {
    APP_STATE_MENU,
    APP_STATE_CHARACTER_CREATION,
    APP_STATE_PLAYING,
    APP_STATE_PAUSED,
    APP_STATE_GAME_OVER
} AppStateEnum;

// Z-buffer cell for rendering
typedef struct {
    char character;
    uint8_t color;
    bool has_content;
} ZBufferCell;

// ECS State (consolidated from ecs.c)
typedef struct {
    // Component registry
    struct {
        struct {
            char name[32];
            uint32_t index;
            uint32_t bit_flag;
            size_t data_size;
        } component_info[32]; // MAX_COMPONENTS
        
        SparseComponentArray component_arrays[32]; // MAX_COMPONENTS
        uint32_t component_active[1000]; // MAX_ENTITIES
        uint32_t component_count;
        bool initialized;
        
        // Hash table for component name lookups
        struct {
            struct ComponentHashEntry *buckets[64]; // COMPONENT_HASH_TABLE_SIZE
        } name_lookup;
    } components;
    
    // System registry
    struct {
        struct {
            uint32_t component_mask;
            void (*function)(Entity entity, struct AppState *appstate);
            void (*pre_update_function)(struct AppState *appstate);
            void (*post_update_function)(struct AppState *appstate);
            char name[32];
            char dependencies[8][32]; // MAX_SYSTEM_DEPENDENCIES
            uint32_t dependency_count;
            int priority; // SystemPriority
            bool enabled;
            uint32_t execution_order;
            uint32_t execution_count;
            float total_execution_time;
        } systems[32]; // MAX_SYSTEMS
        uint32_t system_count;
        bool needs_sorting;
    } systems;
    
    // Entity management
    stack active_entities;
    stack inactive_entities;
    bool initialized;
} ECSState;

// Main AppState structure - consolidates all global state
typedef struct AppState {
    // Core game state (from World)
    Dungeon dungeon;
    Entity player;
    bool initialized;
    bool quit_requested;
    AppStateEnum current_state;
    
    // ECS state (from g_ecs_state)
    ECSState ecs;
    
    // Message system (from g_message_queue)
    MessageQueue messages;
    
    // Error handling (from g_last_error)
    ErrorContext error;
    uint32_t error_counter;
    
    // Render system globals
    struct {
        SDL_Window *window;
        SDL_Renderer *renderer;
        TTF_Font *font;
        bool initialized;
        
        // Z-buffers
        ZBufferCell *z_buffer_0;  // Background layer
        ZBufferCell *z_buffer_1;  // Entity layer
        
        // Viewport state
        int viewport_x;
        int viewport_y;
    } render;
    
    // Message view state
    struct {
        SDL_Window *window;
        SDL_Renderer *renderer;
        TTF_Font *font;
        bool is_visible;
        bool has_focus;
        int window_width;
        int window_height;
        int scroll_position;
        int lines_per_page;
        int total_lines;
        bool scrollbar_dragging;
        int scrollbar_drag_offset;
        bool initialized;
    } message_view;
    
} AppState;

// Singleton access functions
AppState* appstate_get(void);
bool appstate_init(void);
void appstate_shutdown(void);

// Convenience functions for common operations
void appstate_request_quit(void);
bool appstate_should_quit(void);
void appstate_set_state(AppStateEnum state);
AppStateEnum appstate_get_state(void);

#endif // APPSTATE_H
