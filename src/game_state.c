#include "game_state.h"
#include "log.h"
#include "ecs.h"
#include "components.h"
#include "template_system.h"
#include "dungeon.h"
#include "field.h"
#include "messages.h"
#include <stdlib.h>

// Forward declarations for helper functions
static int create_entities_and_world(World *world);

// ===== MENU STATE HANDLERS =====

static void menu_state_enter(GameStateManager *manager, World *world) {
    (void)world;  // Suppress unused parameter warning
    LOG_INFO("Entering menu state");
    main_menu_init(&manager->menu_data.main_menu);
}

static void menu_state_exit(GameStateManager *manager, World *world) {
    (void)world;  // Suppress unused parameter warning
    LOG_INFO("Exiting menu state");
    main_menu_cleanup(&manager->menu_data.main_menu);
}

static void menu_state_input(GameStateManager *manager, World *world, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        main_menu_handle_input(&manager->menu_data.main_menu, event->key.keysym.sym);
        
        // Check for menu selection
        if (main_menu_has_selection(&manager->menu_data.main_menu)) {
            MenuOption option = main_menu_get_selection(&manager->menu_data.main_menu);
            switch (option) {
                case MENU_OPTION_NEW_GAME:
                    LOG_INFO("Starting new game - entering character creation");
                    game_state_manager_set_state(manager, world, GAME_STATE_CHARACTER_CREATION);
                    break;
                case MENU_OPTION_LOAD_GAME:
                    LOG_INFO("Load game selected (not implemented yet)");
                    // TODO: Implement load game functionality
                    break;
                case MENU_OPTION_QUIT:
                    world_request_quit(world);
                    break;
                case MENU_OPTION_COUNT:
                default:
                    // Invalid option
                    break;
            }
            main_menu_init(&manager->menu_data.main_menu); // Reset menu
        }
    }
}

static void menu_state_update(GameStateManager *manager, World *world, float delta_time) {
    // Menu doesn't need updates
    (void)manager;
    (void)world;
    (void)delta_time;
}

static void menu_state_render(GameStateManager *manager, World *world) {
    main_menu_render(world, &manager->menu_data.main_menu);
}

// ===== CHARACTER CREATION STATE HANDLERS =====

static void char_creation_state_enter(GameStateManager *manager, World *world) {
    (void)world;  // Suppress unused parameter warning
    LOG_INFO("Entering character creation state");
    character_creation_init(&manager->char_creation_data.char_creation);
}

static void char_creation_state_exit(GameStateManager *manager, World *world) {
    (void)world;  // Suppress unused parameter warning
    LOG_INFO("Exiting character creation state");
    character_creation_cleanup(&manager->char_creation_data.char_creation);
}

static void char_creation_state_input(GameStateManager *manager, World *world, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        CharacterCreation *char_creation = &manager->char_creation_data.char_creation;
        character_creation_handle_input(char_creation, event->key.keysym.sym);
        
        // Check if character creation is complete
        if (char_creation->race_selected && char_creation->class_selected && 
            event->key.keysym.sym == SDLK_RETURN) {
            
            // Create game entities and world
            if (!create_entities_and_world(world)) {
                LOG_ERROR("Failed to create game entities and world");
                game_state_manager_set_state(manager, world, GAME_STATE_MENU);
                return;
            }
            
            // First, remove the old template player entity
            if (world->player != INVALID_ENTITY) {
                // Remove from dungeon tile system
                Position *old_pos = (Position *)entity_get_component(world->player, component_get_id("Position"));
                if (old_pos) {
                    dungeon_remove_entity_from_position(&world->dungeon, world->player, old_pos->x, old_pos->y);
                    LOG_INFO("Removed template player from dungeon at (%d, %d)", (int)old_pos->x, (int)old_pos->y);
                }
                
                // Destroy the old template player entity
                entity_destroy(world->player);
                LOG_INFO("Destroyed template player entity");
            }
            
            // Replace the template player with our created character
            Entity created_player = character_creation_finalize(char_creation, world);
            if (created_player != INVALID_ENTITY) {
                world->player = created_player;
                
                // Position the custom player at the stairs up location
                Position *player_pos = (Position *)entity_get_component(created_player, component_get_id("Position"));
                if (player_pos) {
                    player_pos->x = (float)world->dungeon.stairs_up_x;
                    player_pos->y = (float)world->dungeon.stairs_up_y;
                    // Store player in tile
                    dungeon_place_entity_at_position(&world->dungeon, created_player, player_pos->x, player_pos->y);
                    LOG_INFO("Positioned custom player at (%d, %d)", world->dungeon.stairs_up_x, world->dungeon.stairs_up_y);
                }
                
                // Add some initial messages
                messages_add("Welcome to the Adventure Game!");
                messages_add("Your quest begins in the depths of an ancient dungeon.");
                messages_add("Use arrow keys to move around. Good luck!");
                
                // Start playing
                game_state_manager_set_state(manager, world, GAME_STATE_PLAYING);
            } else {
                LOG_ERROR("Failed to finalize character creation");
                game_state_manager_set_state(manager, world, GAME_STATE_MENU);
            }
        }
    }
}

static void char_creation_state_update(GameStateManager *manager, World *world, float delta_time) {
    // Character creation doesn't need updates
    (void)manager;
    (void)world;
    (void)delta_time;
}

static void char_creation_state_render(GameStateManager *manager, World *world) {
    character_creation_render(world, &manager->char_creation_data.char_creation);
}

// ===== GAMEPLAY STATE HANDLERS =====

static void gameplay_state_enter(GameStateManager *manager, World *world) {
    (void)world;  // Suppress unused parameter warning
    LOG_INFO("Entering gameplay state");
    manager->gameplay_data.gameplay_timer = 0.0f;
}

static void gameplay_state_exit(GameStateManager *manager, World *world) {
    (void)manager;  // Suppress unused parameter warning
    (void)world;    // Suppress unused parameter warning
    LOG_INFO("Exiting gameplay state");
    // Cleanup any gameplay-specific resources if needed
}

static void gameplay_state_input(GameStateManager *manager, World *world, SDL_Event *event) {
    // Let the existing input system handle gameplay input
    // The input system will only process when in PLAYING state
    (void)manager;
    (void)world;
    (void)event;
}

static void gameplay_state_update(GameStateManager *manager, World *world, float delta_time) {
    manager->gameplay_data.gameplay_timer += delta_time;
    
    // Run ECS systems for gameplay
    if (!system_run_all(world)) {
        world_request_quit(world);
    }
}

static void gameplay_state_render(GameStateManager *manager, World *world) {
    // Rendering is handled by the render system through ECS
    (void)manager;
    (void)world;
}

// ===== HELPER FUNCTION =====

static int create_entities_and_world(World *world) {
    // Initialize dungeon
    dungeon_init(&world->dungeon);
    dungeon_generate(&world->dungeon);
    LOG_INFO("Generated dungeon with %d rooms", world->dungeon.room_count);
    
    // Create player entity from template
    world->player = create_entity_from_template("player");
    if (world->player == INVALID_ENTITY) {
        LOG_ERROR("Failed to create player entity from template");
        return 0;
    }
    
    // Add field of view component to player
    CompactFieldOfView player_fov;
    field_init_compact(&player_fov, FOV_RADIUS);
    if (!component_add(world->player, component_get_id("FieldOfView"), &player_fov)) {
        LOG_ERROR("Failed to add FieldOfView component to player");
        return 0;
    }
    LOG_INFO("Added compact field of view component to player");
    
    // Place player at stairs up position
    Position *player_pos = (Position *)entity_get_component(world->player, component_get_id("Position"));
    if (player_pos) {
        player_pos->x = (float)world->dungeon.stairs_up_x;
        player_pos->y = (float)world->dungeon.stairs_up_y;
        // Store player in tile
        dungeon_place_entity_at_position(&world->dungeon, world->player, player_pos->x, player_pos->y);
        LOG_INFO("Placed player at (%d, %d)", world->dungeon.stairs_up_x, world->dungeon.stairs_up_y);
    }
    
    // Create enemy entity from template (orc)
    Entity enemy = create_entity_from_template("enemy");
    if (enemy == INVALID_ENTITY) {
        LOG_ERROR("Failed to create enemy entity from template");
        return 0;
    }
    
    // Place enemy very close to player for debugging
    Position *enemy_pos = (Position *)entity_get_component(enemy, component_get_id("Position"));
    if (enemy_pos && player_pos) {
        enemy_pos->x = player_pos->x + 1; // Right next to player
        enemy_pos->y = player_pos->y;
        // Store enemy in tile
        dungeon_place_entity_at_position(&world->dungeon, enemy, enemy_pos->x, enemy_pos->y);
        LOG_INFO("Placed enemy (orc) at (%d, %d) - right next to player", (int)enemy_pos->x, (int)enemy_pos->y);
    }
    
    // Create gold entity from template (treasure)
    Entity gold = create_entity_from_template("gold");
    if (gold == INVALID_ENTITY) {
        LOG_ERROR("Failed to create gold entity from template");
        return 0;
    }
    
    // Place gold below the player
    Position *gold_pos = (Position *)entity_get_component(gold, component_get_id("Position"));
    if (gold_pos && player_pos) {
        gold_pos->x = player_pos->x;
        gold_pos->y = player_pos->y + 1; // Below player
        // Store gold in tile
        dungeon_place_entity_at_position(&world->dungeon, gold, gold_pos->x, gold_pos->y);
        LOG_INFO("Placed gold (treasure) at (%d, %d) - below player", (int)gold_pos->x, (int)gold_pos->y);
    }
    
    // Create sword entity from template
    Entity sword = create_entity_from_template("sword");
    if (sword == INVALID_ENTITY) {
        LOG_ERROR("Failed to create sword entity from template");
        return 0;
    }
    
    // Place sword to the left of the player
    Position *sword_pos = (Position *)entity_get_component(sword, component_get_id("Position"));
    if (sword_pos && player_pos) {
        sword_pos->x = player_pos->x - 1; // Left of player
        sword_pos->y = player_pos->y;
        // Store sword in tile
        dungeon_place_entity_at_position(&world->dungeon, sword, sword_pos->x, sword_pos->y);
        LOG_INFO("Placed sword at (%d, %d) - left of player", (int)sword_pos->x, (int)sword_pos->y);
    }
    
    return 1;
}

// ===== GAME STATE MANAGER IMPLEMENTATION =====

GameStateManager* game_state_manager_create(void) {
    GameStateManager *manager = malloc(sizeof(GameStateManager));
    if (!manager) {
        LOG_ERROR("Failed to allocate memory for GameStateManager");
        return NULL;
    }
    
    // Initialize state
    manager->current_state = GAME_STATE_MENU;
    manager->previous_state = GAME_STATE_MENU;
    manager->state_changed = false;
    manager->requested_state = GAME_STATE_MENU;
    
    // Initialize state handlers
    
    // Menu state
    manager->handlers[GAME_STATE_MENU].on_enter = menu_state_enter;
    manager->handlers[GAME_STATE_MENU].on_exit = menu_state_exit;
    manager->handlers[GAME_STATE_MENU].on_input = menu_state_input;
    manager->handlers[GAME_STATE_MENU].on_update = menu_state_update;
    manager->handlers[GAME_STATE_MENU].on_render = menu_state_render;
    
    // Character creation state
    manager->handlers[GAME_STATE_CHARACTER_CREATION].on_enter = char_creation_state_enter;
    manager->handlers[GAME_STATE_CHARACTER_CREATION].on_exit = char_creation_state_exit;
    manager->handlers[GAME_STATE_CHARACTER_CREATION].on_input = char_creation_state_input;
    manager->handlers[GAME_STATE_CHARACTER_CREATION].on_update = char_creation_state_update;
    manager->handlers[GAME_STATE_CHARACTER_CREATION].on_render = char_creation_state_render;
    
    // Gameplay state
    manager->handlers[GAME_STATE_PLAYING].on_enter = gameplay_state_enter;
    manager->handlers[GAME_STATE_PLAYING].on_exit = gameplay_state_exit;
    manager->handlers[GAME_STATE_PLAYING].on_input = gameplay_state_input;
    manager->handlers[GAME_STATE_PLAYING].on_update = gameplay_state_update;
    manager->handlers[GAME_STATE_PLAYING].on_render = gameplay_state_render;
    
    // TODO: Add handlers for GAME_STATE_PAUSED and GAME_STATE_GAME_OVER when needed
    
    LOG_INFO("Created GameStateManager");
    return manager;
}

void game_state_manager_destroy(GameStateManager *manager) {
    if (!manager) return;
    
    LOG_INFO("Destroying GameStateManager");
    free(manager);
}

void game_state_manager_init(GameStateManager *manager, World *world) {
    if (!manager || !world) return;
    
    LOG_INFO("Initializing GameStateManager");
    
    // Enter initial state
    if (manager->handlers[manager->current_state].on_enter) {
        manager->handlers[manager->current_state].on_enter(manager, world);
    }
}

void game_state_manager_set_state(GameStateManager *manager, World *world, GameState new_state) {
    if (!manager || !world) return;
    
    if (new_state != manager->current_state) {
        LOG_INFO("State transition: %d -> %d", manager->current_state, new_state);
        
        // Exit current state
        if (manager->handlers[manager->current_state].on_exit) {
            manager->handlers[manager->current_state].on_exit(manager, world);
        }
        
        // Update state
        manager->previous_state = manager->current_state;
        manager->current_state = new_state;
        manager->state_changed = true;
        
        // Enter new state
        if (manager->handlers[manager->current_state].on_enter) {
            manager->handlers[manager->current_state].on_enter(manager, world);
        }
        
        // Update world state for compatibility
        world_set_state(world, new_state);
    }
}

GameState game_state_manager_get_current_state(GameStateManager *manager) {
    return manager ? manager->current_state : GAME_STATE_MENU;
}

GameState game_state_manager_get_previous_state(GameStateManager *manager) {
    return manager ? manager->previous_state : GAME_STATE_MENU;
}

void game_state_manager_handle_input(GameStateManager *manager, World *world, SDL_Event *event) {
    if (!manager || !world || !event) return;
    
    // Handle global input (quit)
    if (event->type == SDL_QUIT || (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)) {
        world_request_quit(world);
        return;
    }
    
    // Delegate to current state handler
    if (manager->handlers[manager->current_state].on_input) {
        manager->handlers[manager->current_state].on_input(manager, world, event);
    }
}

void game_state_manager_update(GameStateManager *manager, World *world, float delta_time) {
    if (!manager || !world) return;
    
    // Delegate to current state handler
    if (manager->handlers[manager->current_state].on_update) {
        manager->handlers[manager->current_state].on_update(manager, world, delta_time);
    }
    
    // Reset state change flag
    manager->state_changed = false;
}

void game_state_manager_render(GameStateManager *manager, World *world) {
    if (!manager || !world) return;
    
    // Delegate to current state handler
    if (manager->handlers[manager->current_state].on_render) {
        manager->handlers[manager->current_state].on_render(manager, world);
    }
}

bool game_state_manager_should_quit(GameStateManager *manager, World *world) {
    (void)manager;  // Suppress unused parameter warning
    if (!world) return true;
    return world_should_quit(world);
} 
