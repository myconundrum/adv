#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "appstate.h"
#include "main_menu.h"
#include "character_creation.h"

// State handler function types
struct GameStateManager;
typedef void (*StateEnterFunction)(struct GameStateManager *manager);
typedef void (*StateExitFunction)(struct GameStateManager *manager);
typedef void (*StateInputFunction)(struct GameStateManager *manager, SDL_Event *event);
typedef void (*StateUpdateFunction)(struct GameStateManager *manager, float delta_time);
typedef void (*StateRenderFunction)(struct GameStateManager *manager);

// State handler structure
typedef struct {
    StateEnterFunction on_enter;
    StateExitFunction on_exit;
    StateInputFunction on_input;
    StateUpdateFunction on_update;
    StateRenderFunction on_render;
} StateHandler;

// State-specific data containers
typedef struct {
    MainMenu main_menu;
} MenuStateData;

typedef struct {
    CharacterCreation char_creation;
} CharacterCreationStateData;

typedef struct {
    // Future: gameplay-specific state data can be added here
    float gameplay_timer;
} GameplayStateData;

// Game state manager structure
typedef struct GameStateManager {
    AppStateEnum current_state;
    AppStateEnum previous_state;
    StateHandler handlers[APP_STATE_GAME_OVER + 1];  // One handler per state
    
    // State-specific data
    MenuStateData menu_data;
    CharacterCreationStateData char_creation_data;
    GameplayStateData gameplay_data;
    
    // State transition flags
    bool state_changed;
    AppStateEnum requested_state;
} GameStateManager;

// Game state manager functions
GameStateManager* game_state_manager_create(void);
void game_state_manager_destroy(GameStateManager *manager);

// State management
void game_state_manager_init(GameStateManager *manager);
void game_state_manager_set_state(GameStateManager *manager, AppStateEnum new_state);
AppStateEnum game_state_manager_get_current_state(GameStateManager *manager);
AppStateEnum game_state_manager_get_previous_state(GameStateManager *manager);

// Main loop integration
void game_state_manager_handle_input(GameStateManager *manager, SDL_Event *event);
void game_state_manager_update(GameStateManager *manager, float delta_time);
void game_state_manager_render(GameStateManager *manager);

// Utility functions
bool game_state_manager_should_quit(GameStateManager *manager);

#endif // GAME_STATE_H
