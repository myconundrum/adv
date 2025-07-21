#ifndef CHARACTER_CREATION_H
#define CHARACTER_CREATION_H

#include "world.h"
#include "ecs.h"
#include <stdbool.h>
#include <stdint.h>

// Ability scores structure
typedef struct {
    uint8_t strength;
    uint8_t dexterity;
    uint8_t constitution;
    uint8_t intelligence;
    uint8_t wisdom;
    uint8_t charisma;
} AbilityScores;

// Character races (Basic Fantasy RPG)
typedef enum {
    RACE_HUMAN = 0,
    RACE_DWARF,
    RACE_ELF,
    RACE_HALFLING,
    RACE_COUNT
} CharacterRace;

// Character classes (Basic Fantasy RPG)
typedef enum {
    CLASS_FIGHTER = 0,
    CLASS_MAGIC_USER,
    CLASS_CLERIC,
    CLASS_THIEF,
    CLASS_COUNT
} CharacterClass;

// Character creation state
typedef struct {
    AbilityScores scores;
    CharacterRace race;
    CharacterClass class;
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
} CharacterCreation;

// Character creation functions
void character_creation_init(CharacterCreation *creation);
void character_creation_cleanup(CharacterCreation *creation);

// Stat rolling
void character_creation_roll_stats(CharacterCreation *creation);
void character_creation_reroll_stats(CharacterCreation *creation);

// Race and class selection
bool character_creation_can_select_race(const AbilityScores *scores, CharacterRace race);
bool character_creation_can_select_class(const AbilityScores *scores, CharacterRace race, CharacterClass class);
void character_creation_select_race(CharacterCreation *creation, CharacterRace race);
void character_creation_select_class(CharacterCreation *creation, CharacterClass class);

// Name entry
void character_creation_set_name(CharacterCreation *creation, const char *name);

// Finalization
Entity character_creation_finalize(CharacterCreation *creation, World *world);

// UI functions
void character_creation_render(World *world, CharacterCreation *creation);
void character_creation_handle_input(CharacterCreation *creation, int key);

// Utility functions
const char* character_creation_get_race_name(CharacterRace race);
const char* character_creation_get_class_name(CharacterClass class);
const char* character_creation_get_stat_name(int stat_index);
int character_creation_get_ability_modifier(uint8_t ability_score);

#endif
