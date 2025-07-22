#ifndef CHARACTER_CREATION_H
#define CHARACTER_CREATION_H

#include "appstate.h"
#include "ecs.h"
#include <stdbool.h>
#include <stdint.h>
#include <cjson/cJSON.h>

// Maximum number of races and classes supported
#define MAX_RACES 10
#define MAX_CLASSES 10
#define MAX_SPECIAL_ABILITIES 10
#define MAX_RESTRICTIONS 10
#define MAX_LANGUAGES 5

// Ability scores structure
typedef struct {
    uint8_t strength;
    uint8_t dexterity;
    uint8_t constitution;
    uint8_t intelligence;
    uint8_t wisdom;
    uint8_t charisma;
} AbilityScores;

// Special ability structure
typedef struct {
    char name[64];
    char description[256];
} SpecialAbility;

// Race configuration structure
typedef struct {
    char id[32];
    char name[64];
    char description[512];
    char detailed_description[1024];
    
    // Requirements (ability score minimums)
    AbilityScores requirements;
    
    // Ability score modifiers
    AbilityScores ability_modifiers;
    
    // Special abilities
    SpecialAbility special_abilities[MAX_SPECIAL_ABILITIES];
    int special_ability_count;
    
    // Restrictions
    char restrictions[MAX_RESTRICTIONS][128];
    int restriction_count;
    
    // Other properties
    char lifespan[64];
    char size[16];
    int movement;
    char languages[MAX_LANGUAGES][32];
    int language_count;
} RaceConfig;

// Class configuration structure  
typedef struct {
    char id[32];
    char name[64];
    char description[512];
    char detailed_description[1024];
    
    // Requirements (ability score minimums)
    AbilityScores requirements;
    
    // Prime attributes for experience bonuses
    char prime_attributes[3][16]; // Up to 3 prime attributes
    int prime_attribute_count;
    
    // Combat and equipment
    char hit_die[8];
    char armor_allowed[128];
    char weapons_allowed[256];
    
    // Special abilities
    SpecialAbility special_abilities[MAX_SPECIAL_ABILITIES];
    int special_ability_count;
    
    // Restrictions
    char restrictions[MAX_RESTRICTIONS][128];
    int restriction_count;
    
    // Experience bonus requirements
    AbilityScores experience_bonus_requirements;
    
    // Other properties
    char role[64];
    char starting_equipment[256];
    bool has_spell_progression;
} ClassConfig;

// Configuration manager for races and classes
typedef struct {
    RaceConfig races[MAX_RACES];
    int race_count;
    
    ClassConfig classes[MAX_CLASSES];
    int class_count;
    
    bool loaded;
} CharacterConfig;

// Navigation and step management
typedef enum {
    STEP_STATS,
    STEP_RACE,
    STEP_CLASS,
    STEP_REVIEW
} CreationStep;

// Character creation state
typedef struct {
    AbilityScores scores;
    int selected_race;      // Index into races array (-1 if none selected)
    int selected_class;     // Index into classes array (-1 if none selected)
    char name[32];
    
    // UI state
    bool stats_rolled;
    bool race_selected;
    bool class_selected;
    bool name_entered;
    bool creation_complete;
    
    // Menu states
    bool show_race_selection;
    bool show_class_selection;
    
    // Current selection for UI navigation
    int current_race_selection;
    int current_class_selection;
    
    // Navigation and step management
    CreationStep current_step;
    
    // UI enhancement
    int scroll_offset;          // For scrolling through long lists
    bool show_detailed_info;    // Whether to show detailed race/class info
    int info_target;           // Which race/class to show info for (-1 = none)
    char validation_message[256]; // Message explaining why a selection is invalid
} CharacterCreation;

// Configuration functions
bool character_config_load(CharacterConfig *config);
void character_config_cleanup(CharacterConfig *config);
bool character_config_load_races(CharacterConfig *config, const char *filename);
bool character_config_load_classes(CharacterConfig *config, const char *filename);

// Character creation functions
void character_creation_init(CharacterCreation *creation);
void character_creation_cleanup(CharacterCreation *creation);

// Stat rolling
void character_creation_roll_stats(CharacterCreation *creation);
void character_creation_reroll_stats(CharacterCreation *creation);

// Race and class selection
bool character_creation_can_select_race(const AbilityScores *scores, const RaceConfig *race);
bool character_creation_can_select_class(const AbilityScores *scores, const RaceConfig *race, const ClassConfig *class);
void character_creation_select_race(CharacterCreation *creation, int race_index);
void character_creation_select_class(CharacterCreation *creation, int class_index);

// Name entry
void character_creation_set_name(CharacterCreation *creation, const char *name);

// Finalization
Entity character_creation_finalize(CharacterCreation *creation);

// UI functions
void character_creation_render(CharacterCreation *creation, SDL_Renderer *renderer);
void character_creation_handle_input(CharacterCreation *creation, int key);

// Navigation functions
void character_creation_next_step(CharacterCreation *creation);
void character_creation_previous_step(CharacterCreation *creation);
void character_creation_goto_step(CharacterCreation *creation, CreationStep step);

// Validation and information functions
void character_creation_update_validation_message(CharacterCreation *creation);
void character_creation_get_race_requirements_text(const RaceConfig *race, char *buffer, size_t buffer_size);
void character_creation_get_class_requirements_text(const ClassConfig *class, const RaceConfig *race, char *buffer, size_t buffer_size);

// Utility functions
int character_creation_get_ability_modifier(uint8_t ability_score);
AbilityScores character_creation_apply_racial_modifiers(const AbilityScores *base_scores, const RaceConfig *race);

// Global character configuration access
CharacterConfig* get_character_config(void);

#endif
