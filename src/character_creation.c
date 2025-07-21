#include "character_creation.h"
#include "components.h"
#include "ecs.h"
#include "field.h"
#include "log.h"
#include "render_system.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Initialize random seed (call once at startup)
static bool random_initialized = false;
static TTF_Font *g_char_font = NULL;

// Helper function to render text at a specific position
static void render_text_at_position(SDL_Renderer *renderer, const char *text, int x, int y, SDL_Color color) {
    if (!g_char_font || !text) return;
    
    SDL_Surface *text_surface = TTF_RenderText_Solid(g_char_font, text, color);
    if (text_surface) {
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        if (text_texture) {
            int text_w, text_h;
            SDL_QueryTexture(text_texture, NULL, NULL, &text_w, &text_h);
            
            SDL_Rect text_rect = {x, y, text_w, text_h};
            SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
            
            SDL_DestroyTexture(text_texture);
        }
        SDL_FreeSurface(text_surface);
    }
}

// Initialize random number generator if needed
static void ensure_random_initialized(void) {
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = true;
    }
}

// Roll 3D6 for a single ability score
static uint8_t roll_3d6(void) {
    ensure_random_initialized();
    return (uint8_t)(rand() % 6 + 1) + (rand() % 6 + 1) + (rand() % 6 + 1);
}

// Character creation initialization
void character_creation_init(CharacterCreation *creation) {
    if (!creation) return;
    
    memset(creation, 0, sizeof(CharacterCreation));
    creation->race = RACE_HUMAN; // Default race
    creation->class = CLASS_FIGHTER; // Default class
    strcpy(creation->name, "Adventurer"); // Default name
    
    // Load font if not already loaded
    if (!g_char_font) {
        const char* font_paths[] = {
            "/System/Library/Fonts/Monaco.ttf",
            "/System/Library/Fonts/Courier.ttc",
            "/System/Library/Fonts/Menlo.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
            NULL
        };
        
        for (int i = 0; font_paths[i] != NULL; i++) {
            g_char_font = TTF_OpenFont(font_paths[i], 16);
            if (g_char_font) {
                break;
            }
        }
        
        if (!g_char_font) {
            LOG_WARN("Could not load font for character creation");
        }
    }
}

void character_creation_cleanup(CharacterCreation *creation) {
    (void)creation;
    
    if (g_char_font) {
        TTF_CloseFont(g_char_font);
        g_char_font = NULL;
    }
}

// Roll all ability scores
void character_creation_roll_stats(CharacterCreation *creation) {
    if (!creation) return;
    
    creation->scores.strength = roll_3d6();
    creation->scores.dexterity = roll_3d6();
    creation->scores.constitution = roll_3d6();
    creation->scores.intelligence = roll_3d6();
    creation->scores.wisdom = roll_3d6();
    creation->scores.charisma = roll_3d6();
    
    creation->stats_rolled = true;
    creation->race_selected = false;
    creation->class_selected = false;
    
    LOG_INFO("Rolled stats: STR:%d DEX:%d CON:%d INT:%d WIS:%d CHA:%d",
             creation->scores.strength, creation->scores.dexterity,
             creation->scores.constitution, creation->scores.intelligence,
             creation->scores.wisdom, creation->scores.charisma);
}

void character_creation_reroll_stats(CharacterCreation *creation) {
    character_creation_roll_stats(creation);
}

// Race selection with Basic Fantasy requirements
bool character_creation_can_select_race(const AbilityScores *scores, CharacterRace race) {
    if (!scores) return false;
    
    switch (race) {
        case RACE_HUMAN:
            return true; // Humans have no requirements
        case RACE_DWARF:
            return scores->constitution >= 9; // Dwarves need CON 9+
        case RACE_ELF:
            return scores->intelligence >= 9; // Elves need INT 9+
        case RACE_HALFLING:
            return scores->dexterity >= 9; // Halflings need DEX 9+
        default:
            return false;
    }
}

// Class selection with Basic Fantasy requirements
bool character_creation_can_select_class(const AbilityScores *scores, CharacterRace race, CharacterClass class) {
    if (!scores) return false;
    
    // Race doesn't affect class requirements in Basic Fantasy RPG
    (void)race; // Suppress unused parameter warning
    
    switch (class) {
        case CLASS_FIGHTER:
            return true; // Fighters have no requirements
        case CLASS_MAGIC_USER:
            return scores->intelligence >= 9; // Magic-Users need INT 9+
        case CLASS_CLERIC:
            return scores->wisdom >= 9; // Clerics need WIS 9+
        case CLASS_THIEF:
            return scores->dexterity >= 9; // Thieves need DEX 9+
        default:
            return false;
    }
}

void character_creation_select_race(CharacterCreation *creation, CharacterRace race) {
    if (!creation) return;
    
    if (character_creation_can_select_race(&creation->scores, race)) {
        creation->race = race;
        creation->race_selected = true;
        LOG_INFO("Selected race: %s", character_creation_get_race_name(race));
    }
}

void character_creation_select_class(CharacterCreation *creation, CharacterClass class) {
    if (!creation) return;
    
    if (character_creation_can_select_class(&creation->scores, creation->race, class)) {
        creation->class = class;
        creation->class_selected = true;
        LOG_INFO("Selected class: %s", character_creation_get_class_name(class));
    }
}

// Name entry
void character_creation_set_name(CharacterCreation *creation, const char *name) {
    if (!creation || !name) return;
    
    strncpy(creation->name, name, sizeof(creation->name) - 1);
    creation->name[sizeof(creation->name) - 1] = '\0';
    creation->name_entered = true;
}

// Finalize character creation and create player entity
Entity character_creation_finalize(CharacterCreation *creation, World *world) {
    if (!creation || !world || !creation->stats_rolled) {
        return INVALID_ENTITY;
    }
    
    // Create player entity
    Entity player = entity_create();
    if (player == INVALID_ENTITY) {
        LOG_ERROR("Failed to create player entity");
        return INVALID_ENTITY;
    }
    
    // Add Position component
    Position pos = {0, 0, INVALID_ENTITY};
    component_add(player, component_get_id("Position"), &pos);
    
    // Add BaseInfo component with character data
    BaseInfo base_info = {0};
    base_info.character = '@';
    base_info.color = 2; // White
    strncpy(base_info.name, creation->name, sizeof(base_info.name) - 1);
    base_info.flags = ENTITY_FLAG_PLAYER | ENTITY_FLAG_CAN_CARRY | ENTITY_FLAG_ALIVE;
    base_info.weight = 150; // Average human weight
    base_info.volume = 50;
    snprintf(base_info.description, sizeof(base_info.description), 
             "A %s %s named %s", 
             character_creation_get_race_name(creation->race),
             character_creation_get_class_name(creation->class),
             creation->name);
    component_add(player, component_get_id("BaseInfo"), &base_info);
    
    // Add Actor component with rolled stats
    Actor actor = {0};
    actor.energy = 100;
    actor.energy_per_turn = 10;
    
    // Convert ability scores to game stats
    actor.hp = 10 + character_creation_get_ability_modifier(creation->scores.constitution);
    actor.max_hp = actor.hp;
    actor.strength = creation->scores.strength;
    actor.attack = 5 + character_creation_get_ability_modifier(creation->scores.strength);
    actor.defense = 10 + character_creation_get_ability_modifier(creation->scores.dexterity);
    actor.damage_dice = 1;
    actor.damage_sides = 6;
    actor.damage_bonus = character_creation_get_ability_modifier(creation->scores.strength);
    
    component_add(player, component_get_id("Actor"), &actor);
    
    // Add Action component
    Action action = {ACTION_NONE, 0};
    component_add(player, component_get_id("Action"), &action);
    
    // Add Inventory component  
    Inventory inventory = {0};
    inventory.max_items = 10 + character_creation_get_ability_modifier(creation->scores.strength);
    component_add(player, component_get_id("Inventory"), &inventory);
    
    // Add FieldOfView component - essential for dungeon rendering
    CompactFieldOfView player_fov;
    field_init_compact(&player_fov, FOV_RADIUS);
    if (!component_add(player, component_get_id("FieldOfView"), &player_fov)) {
        LOG_ERROR("Failed to add FieldOfView component to player");
        entity_destroy(player);
        return INVALID_ENTITY;
    }
    LOG_INFO("Added FieldOfView component to custom player");
    
    creation->creation_complete = true;
    world->player = player;
    
    LOG_INFO("Character creation complete: %s the %s %s (HP: %d, STR: %d)", 
             creation->name,
             character_creation_get_race_name(creation->race),
             character_creation_get_class_name(creation->class),
             actor.hp, actor.strength);
    
    return player;
}

// Utility functions
const char* character_creation_get_race_name(CharacterRace race) {
    switch (race) {
        case RACE_HUMAN: return "Human";
        case RACE_DWARF: return "Dwarf";
        case RACE_ELF: return "Elf";
        case RACE_HALFLING: return "Halfling";
        default: return "Unknown";
    }
}

const char* character_creation_get_class_name(CharacterClass class) {
    switch (class) {
        case CLASS_FIGHTER: return "Fighter";
        case CLASS_MAGIC_USER: return "Magic-User";
        case CLASS_CLERIC: return "Cleric";
        case CLASS_THIEF: return "Thief";
        default: return "Unknown";
    }
}

const char* character_creation_get_stat_name(int stat_index) {
    switch (stat_index) {
        case 0: return "Strength";
        case 1: return "Dexterity";
        case 2: return "Constitution";
        case 3: return "Intelligence";
        case 4: return "Wisdom";
        case 5: return "Charisma";
        default: return "Unknown";
    }
}

int character_creation_get_ability_modifier(uint8_t ability_score) {
    // Basic Fantasy RPG ability modifiers
    if (ability_score <= 3) return -3;
    if (ability_score <= 5) return -2;
    if (ability_score <= 8) return -1;
    if (ability_score <= 12) return 0;
    if (ability_score <= 15) return 1;
    if (ability_score <= 17) return 2;
    return 3; // 18+
}

// Input handling for character creation
void character_creation_handle_input(CharacterCreation *creation, int key) {
    if (!creation) return;
    
    if (creation->show_race_selection) {
        // Race selection input
        switch (key) {
            case SDLK_1:
                if (character_creation_can_select_race(&creation->scores, RACE_HUMAN)) {
                    character_creation_select_race(creation, RACE_HUMAN);
                    creation->show_race_selection = false;
                }
                break;
            case SDLK_2:
                if (character_creation_can_select_race(&creation->scores, RACE_DWARF)) {
                    character_creation_select_race(creation, RACE_DWARF);
                    creation->show_race_selection = false;
                }
                break;
            case SDLK_3:
                if (character_creation_can_select_race(&creation->scores, RACE_ELF)) {
                    character_creation_select_race(creation, RACE_ELF);
                    creation->show_race_selection = false;
                }
                break;
            case SDLK_4:
                if (character_creation_can_select_race(&creation->scores, RACE_HALFLING)) {
                    character_creation_select_race(creation, RACE_HALFLING);
                    creation->show_race_selection = false;
                }
                break;
        }
    } else if (creation->show_class_selection) {
        // Class selection input
        switch (key) {
            case SDLK_1:
                if (character_creation_can_select_class(&creation->scores, creation->race, CLASS_FIGHTER)) {
                    character_creation_select_class(creation, CLASS_FIGHTER);
                    creation->show_class_selection = false;
                }
                break;
            case SDLK_2:
                if (character_creation_can_select_class(&creation->scores, creation->race, CLASS_MAGIC_USER)) {
                    character_creation_select_class(creation, CLASS_MAGIC_USER);
                    creation->show_class_selection = false;
                }
                break;
            case SDLK_3:
                if (character_creation_can_select_class(&creation->scores, creation->race, CLASS_CLERIC)) {
                    character_creation_select_class(creation, CLASS_CLERIC);
                    creation->show_class_selection = false;
                }
                break;
            case SDLK_4:
                if (character_creation_can_select_class(&creation->scores, creation->race, CLASS_THIEF)) {
                    character_creation_select_class(creation, CLASS_THIEF);
                    creation->show_class_selection = false;
                }
                break;
        }
    } else {
        // Main character creation input
        switch (key) {
            case SDLK_SPACE:
            case SDLK_RETURN:
                if (!creation->stats_rolled) {
                    character_creation_roll_stats(creation);
                } else if (creation->race_selected && creation->class_selected) {
                    // Finalization happens in main.c
                    LOG_INFO("Character creation complete - starting game");
                }
                break;
            case SDLK_r:
                if (creation->stats_rolled) {
                    character_creation_reroll_stats(creation);
                }
                break;
            case SDLK_a:
                if (creation->stats_rolled && !creation->race_selected) {
                    creation->show_race_selection = true;
                }
                break;
            case SDLK_c:
                if (creation->stats_rolled && !creation->class_selected) {
                    creation->show_class_selection = true;
                }
                break;
        }
    }
}

// Character creation UI rendering
void character_creation_render(World *world, CharacterCreation *creation) {
    (void)world; // Suppress unused parameter warning
    if (!creation) return;
    
    // Get renderer from render system
    SDL_Renderer *renderer = render_system_get_renderer();
    if (!renderer) return;
    
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Colors
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color blue = {0, 0, 255, 255};
    SDL_Color gray = {128, 128, 128, 255};
    
    // Title
    render_text_at_position(renderer, "CHARACTER CREATION - Basic Fantasy RPG", 250, 30, white);
    
    if (!creation->stats_rolled) {
        // Initial state - waiting for stat roll
        render_text_at_position(renderer, "Press SPACE or RETURN to roll your ability scores", 250, 150, yellow);
        render_text_at_position(renderer, "You will roll 3D6 for each of the six abilities:", 250, 200, white);
        render_text_at_position(renderer, "Strength, Dexterity, Constitution,", 250, 250, gray);
        render_text_at_position(renderer, "Intelligence, Wisdom, and Charisma", 250, 280, gray);
    } else {
        // Show rolled stats
        char stat_text[64];
        int y_start = 120;
        int line_height = 25;
        
        render_text_at_position(renderer, "Your Ability Scores:", 50, 80, white);
        
        snprintf(stat_text, sizeof(stat_text), "Strength:     %2d (%+d)", 
                creation->scores.strength, character_creation_get_ability_modifier(creation->scores.strength));
        render_text_at_position(renderer, stat_text, 50, y_start, white);
        
        snprintf(stat_text, sizeof(stat_text), "Dexterity:    %2d (%+d)", 
                creation->scores.dexterity, character_creation_get_ability_modifier(creation->scores.dexterity));
        render_text_at_position(renderer, stat_text, 50, y_start + line_height, white);
        
        snprintf(stat_text, sizeof(stat_text), "Constitution: %2d (%+d)", 
                creation->scores.constitution, character_creation_get_ability_modifier(creation->scores.constitution));
        render_text_at_position(renderer, stat_text, 50, y_start + 2 * line_height, white);
        
        snprintf(stat_text, sizeof(stat_text), "Intelligence: %2d (%+d)", 
                creation->scores.intelligence, character_creation_get_ability_modifier(creation->scores.intelligence));
        render_text_at_position(renderer, stat_text, 50, y_start + 3 * line_height, white);
        
        snprintf(stat_text, sizeof(stat_text), "Wisdom:       %2d (%+d)", 
                creation->scores.wisdom, character_creation_get_ability_modifier(creation->scores.wisdom));
        render_text_at_position(renderer, stat_text, 50, y_start + 4 * line_height, white);
        
        snprintf(stat_text, sizeof(stat_text), "Charisma:     %2d (%+d)", 
                creation->scores.charisma, character_creation_get_ability_modifier(creation->scores.charisma));
        render_text_at_position(renderer, stat_text, 50, y_start + 5 * line_height, white);
        
        // Current selections
        render_text_at_position(renderer, "Current Character:", 50, 300, white);
        
        char selection_text[64];
        snprintf(selection_text, sizeof(selection_text), "Race: %s", 
                character_creation_get_race_name(creation->race));
        render_text_at_position(renderer, selection_text, 50, 330, 
                               creation->race_selected ? green : gray);
        
        snprintf(selection_text, sizeof(selection_text), "Class: %s", 
                character_creation_get_class_name(creation->class));
        render_text_at_position(renderer, selection_text, 50, 360, 
                               creation->class_selected ? green : gray);
        
        snprintf(selection_text, sizeof(selection_text), "Name: %s", creation->name);
        render_text_at_position(renderer, selection_text, 50, 390, white);
        
        // Show appropriate menu based on state
        if (creation->show_race_selection) {
            // Race selection menu
            render_text_at_position(renderer, "Select Your Race:", 400, 120, yellow);
            
            const char* race_info[] = {
                "1. Human      (No requirements)",
                "2. Dwarf      (Constitution 9+)", 
                "3. Elf        (Intelligence 9+)",
                "4. Halfling   (Dexterity 9+)"
            };
            
            for (int i = 0; i < RACE_COUNT; i++) {
                bool can_select = character_creation_can_select_race(&creation->scores, (CharacterRace)i);
                SDL_Color color = can_select ? white : red;
                if (can_select && i == RACE_HUMAN) color = green; // Highlight always-available option
                
                render_text_at_position(renderer, race_info[i], 400, 160 + i * 30, color);
                
                if (!can_select) {
                    render_text_at_position(renderer, " (Requirement not met)", 600, 160 + i * 30, red);
                }
            }
            
            render_text_at_position(renderer, "Press 1-4 to select a race", 400, 300, blue);
            
        } else if (creation->show_class_selection) {
            // Class selection menu
            render_text_at_position(renderer, "Select Your Class:", 400, 120, yellow);
            
            const char* class_info[] = {
                "1. Fighter     (No requirements)",
                "2. Magic-User  (Intelligence 9+)",
                "3. Cleric      (Wisdom 9+)",
                "4. Thief       (Dexterity 9+)"
            };
            
            for (int i = 0; i < CLASS_COUNT; i++) {
                bool can_select = character_creation_can_select_class(&creation->scores, creation->race, (CharacterClass)i);
                SDL_Color color = can_select ? white : red;
                if (can_select && i == CLASS_FIGHTER) color = green; // Highlight always-available option
                
                render_text_at_position(renderer, class_info[i], 400, 160 + i * 30, color);
                
                if (!can_select) {
                    render_text_at_position(renderer, " (Requirement not met)", 600, 160 + i * 30, red);
                }
            }
            
            render_text_at_position(renderer, "Press 1-4 to select a class", 400, 300, blue);
            
        } else {
            // Main character creation menu
            render_text_at_position(renderer, "Options:", 400, 120, yellow);
            render_text_at_position(renderer, "R - Reroll ability scores", 400, 160, white);
            render_text_at_position(renderer, "A - Select race", 400, 190, 
                                   creation->race_selected ? gray : white);
            render_text_at_position(renderer, "C - Select class", 400, 220, 
                                   creation->class_selected ? gray : white);
            
            if (creation->race_selected && creation->class_selected) {
                render_text_at_position(renderer, "RETURN - Start your adventure!", 400, 280, green);
            } else {
                render_text_at_position(renderer, "Select race and class to continue", 400, 280, gray);
            }
        }
    }
    
    SDL_RenderPresent(renderer);
} 