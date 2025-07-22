#include "character_creation.h"
#include "components.h"
#include "ecs.h"
#include "field.h"
#include "log.h"
#include "render_system.h"
#include "appstate.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cjson/cJSON.h>

// Global character configuration
static CharacterConfig g_character_config = {0};

// Initialize random seed (call once at startup)
static bool random_initialized = false;

// Helper function to render text at a specific position
static void render_text_at_position(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font || !text) return;
    
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, color);
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

// Helper function to parse ability scores from JSON
static AbilityScores parse_ability_scores(cJSON *json) {
    AbilityScores scores = {0};
    
    if (!json) return scores;
    
    cJSON *str = cJSON_GetObjectItem(json, "strength");
    cJSON *dex = cJSON_GetObjectItem(json, "dexterity");
    cJSON *con = cJSON_GetObjectItem(json, "constitution");
    cJSON *intel = cJSON_GetObjectItem(json, "intelligence");
    cJSON *wis = cJSON_GetObjectItem(json, "wisdom");
    cJSON *cha = cJSON_GetObjectItem(json, "charisma");
    
    if (str && cJSON_IsNumber(str)) scores.strength = (uint8_t)str->valueint;
    if (dex && cJSON_IsNumber(dex)) scores.dexterity = (uint8_t)dex->valueint;
    if (con && cJSON_IsNumber(con)) scores.constitution = (uint8_t)con->valueint;
    if (intel && cJSON_IsNumber(intel)) scores.intelligence = (uint8_t)intel->valueint;
    if (wis && cJSON_IsNumber(wis)) scores.wisdom = (uint8_t)wis->valueint;
    if (cha && cJSON_IsNumber(cha)) scores.charisma = (uint8_t)cha->valueint;
    
    return scores;
}

// Helper function to parse special abilities from JSON array
static int parse_special_abilities(cJSON *abilities_array, SpecialAbility *abilities, int max_abilities) {
    if (!abilities_array || !cJSON_IsArray(abilities_array)) return 0;
    
    int count = 0;
    int array_size = cJSON_GetArraySize(abilities_array);
    
    for (int i = 0; i < array_size && count < max_abilities; i++) {
        cJSON *ability = cJSON_GetArrayItem(abilities_array, i);
        if (!cJSON_IsObject(ability)) continue;
        
        cJSON *name = cJSON_GetObjectItem(ability, "name");
        cJSON *desc = cJSON_GetObjectItem(ability, "description");
        
        if (name && cJSON_IsString(name) && desc && cJSON_IsString(desc)) {
            strncpy(abilities[count].name, name->valuestring, sizeof(abilities[count].name) - 1);
            abilities[count].name[sizeof(abilities[count].name) - 1] = '\0';
            
            strncpy(abilities[count].description, desc->valuestring, sizeof(abilities[count].description) - 1);
            abilities[count].description[sizeof(abilities[count].description) - 1] = '\0';
            
            count++;
        }
    }
    
    return count;
}

// Helper function to parse string array (restrictions, languages) - generic version
static int parse_string_array_generic(cJSON *array, void *dest, int max_items, size_t item_size) {
    if (!array || !cJSON_IsArray(array)) return 0;
    
    int count = 0;
    int array_size = cJSON_GetArraySize(array);
    
    for (int i = 0; i < array_size && count < max_items; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        if (cJSON_IsString(item)) {
            char *dest_item = (char *)dest + (count * item_size);
            strncpy(dest_item, item->valuestring, item_size - 1);
            dest_item[item_size - 1] = '\0';
            count++;
        }
    }
    
    return count;
}

// Helper function for restrictions (128 byte strings)
static int parse_restrictions(cJSON *array, char dest[][128], int max_items) {
    return parse_string_array_generic(array, dest, max_items, 128);
}

// Helper function for languages (32 byte strings)  
static int parse_languages(cJSON *array, char dest[][32], int max_items) {
    return parse_string_array_generic(array, dest, max_items, 32);
}

// Load races from JSON file
bool character_config_load_races(CharacterConfig *config, const char *filename) {
    if (!config || !filename) return false;
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        LOG_ERROR("Failed to open race file: %s", filename);
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return false;
    }
    
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    fclose(file);
    
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    
    if (!root) {
        LOG_ERROR("Failed to parse JSON from %s", filename);
        return false;
    }
    
    cJSON *races_array = cJSON_GetObjectItem(root, "races");
    if (!races_array || !cJSON_IsArray(races_array)) {
        LOG_ERROR("No 'races' array found in %s", filename);
        cJSON_Delete(root);
        return false;
    }
    
    config->race_count = 0;
    int array_size = cJSON_GetArraySize(races_array);
    
    for (int i = 0; i < array_size && config->race_count < MAX_RACES; i++) {
        cJSON *race_obj = cJSON_GetArrayItem(races_array, i);
        if (!cJSON_IsObject(race_obj)) continue;
        
        RaceConfig *race = &config->races[config->race_count];
        memset(race, 0, sizeof(RaceConfig));
        
        // Parse basic info
        cJSON *id = cJSON_GetObjectItem(race_obj, "id");
        cJSON *name = cJSON_GetObjectItem(race_obj, "name");
        cJSON *description = cJSON_GetObjectItem(race_obj, "description");
        cJSON *detailed_desc = cJSON_GetObjectItem(race_obj, "detailed_description");
        
        if (id && cJSON_IsString(id)) {
            strncpy(race->id, id->valuestring, sizeof(race->id) - 1);
        }
        if (name && cJSON_IsString(name)) {
            strncpy(race->name, name->valuestring, sizeof(race->name) - 1);
        }
        if (description && cJSON_IsString(description)) {
            strncpy(race->description, description->valuestring, sizeof(race->description) - 1);
        }
        if (detailed_desc && cJSON_IsString(detailed_desc)) {
            strncpy(race->detailed_description, detailed_desc->valuestring, sizeof(race->detailed_description) - 1);
        }
        
        // Parse requirements and modifiers
        cJSON *requirements = cJSON_GetObjectItem(race_obj, "requirements");
        race->requirements = parse_ability_scores(requirements);
        
        cJSON *modifiers = cJSON_GetObjectItem(race_obj, "ability_modifiers");
        race->ability_modifiers = parse_ability_scores(modifiers);
        
        // Parse special abilities
        cJSON *abilities = cJSON_GetObjectItem(race_obj, "special_abilities");
        race->special_ability_count = parse_special_abilities(abilities, race->special_abilities, MAX_SPECIAL_ABILITIES);
        
        // Parse restrictions
        cJSON *restrictions = cJSON_GetObjectItem(race_obj, "restrictions");
        race->restriction_count = parse_restrictions(restrictions, race->restrictions, MAX_RESTRICTIONS);
        
        // Parse other properties
        cJSON *lifespan = cJSON_GetObjectItem(race_obj, "lifespan");
        cJSON *size = cJSON_GetObjectItem(race_obj, "size");
        cJSON *movement = cJSON_GetObjectItem(race_obj, "movement");
        cJSON *languages = cJSON_GetObjectItem(race_obj, "languages");
        
        if (lifespan && cJSON_IsString(lifespan)) {
            strncpy(race->lifespan, lifespan->valuestring, sizeof(race->lifespan) - 1);
        }
        if (size && cJSON_IsString(size)) {
            strncpy(race->size, size->valuestring, sizeof(race->size) - 1);
        }
        if (movement && cJSON_IsNumber(movement)) {
            race->movement = movement->valueint;
        }
        if (languages && cJSON_IsArray(languages)) {
            race->language_count = parse_languages(languages, race->languages, MAX_LANGUAGES);
        }
        
        config->race_count++;
    }
    
    cJSON_Delete(root);
    LOG_INFO("Loaded %d races from %s", config->race_count, filename);
    return true;
}

// Load classes from JSON file  
bool character_config_load_classes(CharacterConfig *config, const char *filename) {
    if (!config || !filename) return false;
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        LOG_ERROR("Failed to open class file: %s", filename);
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return false;
    }
    
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    fclose(file);
    
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    
    if (!root) {
        LOG_ERROR("Failed to parse JSON from %s", filename);
        return false;
    }
    
    cJSON *classes_array = cJSON_GetObjectItem(root, "classes");
    if (!classes_array || !cJSON_IsArray(classes_array)) {
        LOG_ERROR("No 'classes' array found in %s", filename);
        cJSON_Delete(root);
        return false;
    }
    
    config->class_count = 0;
    int array_size = cJSON_GetArraySize(classes_array);
    
    for (int i = 0; i < array_size && config->class_count < MAX_CLASSES; i++) {
        cJSON *class_obj = cJSON_GetArrayItem(classes_array, i);
        if (!cJSON_IsObject(class_obj)) continue;
        
        ClassConfig *class = &config->classes[config->class_count];
        memset(class, 0, sizeof(ClassConfig));
        
        // Parse basic info
        cJSON *id = cJSON_GetObjectItem(class_obj, "id");
        cJSON *name = cJSON_GetObjectItem(class_obj, "name");
        cJSON *description = cJSON_GetObjectItem(class_obj, "description");
        cJSON *detailed_desc = cJSON_GetObjectItem(class_obj, "detailed_description");
        
        if (id && cJSON_IsString(id)) {
            strncpy(class->id, id->valuestring, sizeof(class->id) - 1);
        }
        if (name && cJSON_IsString(name)) {
            strncpy(class->name, name->valuestring, sizeof(class->name) - 1);
        }
        if (description && cJSON_IsString(description)) {
            strncpy(class->description, description->valuestring, sizeof(class->description) - 1);
        }
        if (detailed_desc && cJSON_IsString(detailed_desc)) {
            strncpy(class->detailed_description, detailed_desc->valuestring, sizeof(class->detailed_description) - 1);
        }
        
        // Parse requirements
        cJSON *requirements = cJSON_GetObjectItem(class_obj, "requirements");
        class->requirements = parse_ability_scores(requirements);
        
        // Parse experience bonus requirements
        cJSON *exp_bonus = cJSON_GetObjectItem(class_obj, "experience_bonus_requirements");
        class->experience_bonus_requirements = parse_ability_scores(exp_bonus);
        
        // Parse prime attributes
        cJSON *prime_attrs = cJSON_GetObjectItem(class_obj, "prime_attributes");
        if (prime_attrs && cJSON_IsArray(prime_attrs)) {
            class->prime_attribute_count = 0;
            int attr_count = cJSON_GetArraySize(prime_attrs);
            for (int j = 0; j < attr_count && class->prime_attribute_count < 3; j++) {
                cJSON *attr = cJSON_GetArrayItem(prime_attrs, j);
                if (cJSON_IsString(attr)) {
                    strncpy(class->prime_attributes[class->prime_attribute_count], attr->valuestring, 15);
                    class->prime_attributes[class->prime_attribute_count][15] = '\0';
                    class->prime_attribute_count++;
                }
            }
        }
        
        // Parse combat info
        cJSON *hit_die = cJSON_GetObjectItem(class_obj, "hit_die");
        cJSON *armor = cJSON_GetObjectItem(class_obj, "armor_allowed");
        cJSON *weapons = cJSON_GetObjectItem(class_obj, "weapons_allowed");
        cJSON *role = cJSON_GetObjectItem(class_obj, "role");
        cJSON *equipment = cJSON_GetObjectItem(class_obj, "starting_equipment");
        
        if (hit_die && cJSON_IsString(hit_die)) {
            strncpy(class->hit_die, hit_die->valuestring, sizeof(class->hit_die) - 1);
        }
        if (armor && cJSON_IsString(armor)) {
            strncpy(class->armor_allowed, armor->valuestring, sizeof(class->armor_allowed) - 1);
        }
        if (weapons && cJSON_IsString(weapons)) {
            strncpy(class->weapons_allowed, weapons->valuestring, sizeof(class->weapons_allowed) - 1);
        }
        if (role && cJSON_IsString(role)) {
            strncpy(class->role, role->valuestring, sizeof(class->role) - 1);
        }
        if (equipment && cJSON_IsString(equipment)) {
            strncpy(class->starting_equipment, equipment->valuestring, sizeof(class->starting_equipment) - 1);
        }
        
        // Parse spell progression
        cJSON *spell_prog = cJSON_GetObjectItem(class_obj, "spell_progression");
        class->has_spell_progression = (spell_prog && !cJSON_IsNull(spell_prog));
        
        // Parse special abilities
        cJSON *abilities = cJSON_GetObjectItem(class_obj, "special_abilities");
        class->special_ability_count = parse_special_abilities(abilities, class->special_abilities, MAX_SPECIAL_ABILITIES);
        
        // Parse restrictions
        cJSON *restrictions = cJSON_GetObjectItem(class_obj, "restrictions");
        class->restriction_count = parse_restrictions(restrictions, class->restrictions, MAX_RESTRICTIONS);
        
        config->class_count++;
    }
    
    cJSON_Delete(root);
    LOG_INFO("Loaded %d classes from %s", config->class_count, filename);
    return true;
}

// Load both race and class configurations
bool character_config_load(CharacterConfig *config) {
    if (!config) return false;
    
    memset(config, 0, sizeof(CharacterConfig));
    
    bool races_loaded = character_config_load_races(config, "race.json");
    bool classes_loaded = character_config_load_classes(config, "class.json");
    
    config->loaded = races_loaded && classes_loaded;
    
    if (config->loaded) {
        LOG_INFO("Character configuration loaded successfully: %d races, %d classes", 
                 config->race_count, config->class_count);
    } else {
        LOG_ERROR("Failed to load character configuration");
    }
    
    return config->loaded;
}

// Cleanup configuration
void character_config_cleanup(CharacterConfig *config) {
    if (config) {
        memset(config, 0, sizeof(CharacterConfig));
    }
}

// Get global character configuration
CharacterConfig* get_character_config(void) {
    if (!g_character_config.loaded) {
        LOG_INFO("Loading character configuration for the first time...");
        if (character_config_load(&g_character_config)) {
            LOG_INFO("Character configuration loaded successfully");
        } else {
            LOG_ERROR("Character configuration failed to load");
        }
    }
    return &g_character_config;
}

// Character creation initialization
void character_creation_init(CharacterCreation *creation) {
    if (!creation) return;
    
    LOG_INFO("Initializing character creation...");
    memset(creation, 0, sizeof(CharacterCreation));
    creation->selected_race = -1;  // No race selected
    creation->selected_class = -1; // No class selected
    strcpy(creation->name, "Adventurer"); // Default name
    
    // Initialize new fields
    creation->current_step = STEP_STATS;
    creation->current_race_selection = 0;
    creation->current_class_selection = 0;
    creation->scroll_offset = 0;
    creation->show_detailed_info = false;
    creation->info_target = -1;
    creation->validation_message[0] = '\0';
    
    // Ensure character configuration is loaded
    CharacterConfig *config = get_character_config();
    if (config && config->loaded) {
        LOG_INFO("Character creation initialized with %d races and %d classes", 
                 config->race_count, config->class_count);
    } else {
        LOG_ERROR("Character creation initialized but configuration not loaded!");
    }
}

void character_creation_cleanup(CharacterCreation *creation) {
    (void)creation;
    // Nothing to cleanup for now
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
    creation->selected_race = -1;
    creation->selected_class = -1;
    
    // Clear any old menu states
    creation->show_race_selection = false;
    creation->show_class_selection = false;
    creation->validation_message[0] = '\0';
    
    LOG_INFO("Rolled stats: STR:%d DEX:%d CON:%d INT:%d WIS:%d CHA:%d",
             creation->scores.strength, creation->scores.dexterity,
             creation->scores.constitution, creation->scores.intelligence,
             creation->scores.wisdom, creation->scores.charisma);
}

void character_creation_reroll_stats(CharacterCreation *creation) {
    character_creation_roll_stats(creation);
}

// Check if a race can be selected based on ability requirements
bool character_creation_can_select_race(const AbilityScores *scores, const RaceConfig *race) {
    if (!scores || !race) return false;
    
    if (race->requirements.strength > 0 && scores->strength < race->requirements.strength) return false;
    if (race->requirements.dexterity > 0 && scores->dexterity < race->requirements.dexterity) return false;
    if (race->requirements.constitution > 0 && scores->constitution < race->requirements.constitution) return false;
    if (race->requirements.intelligence > 0 && scores->intelligence < race->requirements.intelligence) return false;
    if (race->requirements.wisdom > 0 && scores->wisdom < race->requirements.wisdom) return false;
    if (race->requirements.charisma > 0 && scores->charisma < race->requirements.charisma) return false;
    
    return true;
}

// Check if a class can be selected based on ability requirements and race restrictions
bool character_creation_can_select_class(const AbilityScores *scores, const RaceConfig *race, const ClassConfig *class) {
    if (!scores || !class) return false;
    
    // Check ability requirements
    if (class->requirements.strength > 0 && scores->strength < class->requirements.strength) return false;
    if (class->requirements.dexterity > 0 && scores->dexterity < class->requirements.dexterity) return false;
    if (class->requirements.constitution > 0 && scores->constitution < class->requirements.constitution) return false;
    if (class->requirements.intelligence > 0 && scores->intelligence < class->requirements.intelligence) return false;
    if (class->requirements.wisdom > 0 && scores->wisdom < class->requirements.wisdom) return false;
    if (class->requirements.charisma > 0 && scores->charisma < class->requirements.charisma) return false;
    
    // Check racial restrictions
    if (race) {
        for (int i = 0; i < race->restriction_count; i++) {
            // Check if this race restriction applies to this class
            if (strstr(race->restrictions[i], class->name) != NULL && 
                strstr(race->restrictions[i], "Cannot") != NULL) {
                return false;
            }
        }
    }
    
    return true;
}

void character_creation_select_race(CharacterCreation *creation, int race_index) {
    if (!creation) return;
    
    CharacterConfig *config = get_character_config();
    if (!config || race_index < 0 || race_index >= config->race_count) return;
    
    if (character_creation_can_select_race(&creation->scores, &config->races[race_index])) {
        creation->selected_race = race_index;
        creation->race_selected = true;
        LOG_INFO("Selected race: %s", config->races[race_index].name);
    }
}

void character_creation_select_class(CharacterCreation *creation, int class_index) {
    if (!creation) return;
    
    CharacterConfig *config = get_character_config();
    if (!config || class_index < 0 || class_index >= config->class_count) return;
    
    RaceConfig *race = (creation->selected_race >= 0) ? &config->races[creation->selected_race] : NULL;
    
    if (character_creation_can_select_class(&creation->scores, race, &config->classes[class_index])) {
        creation->selected_class = class_index;
        creation->class_selected = true;
        LOG_INFO("Selected class: %s", config->classes[class_index].name);
    }
}

// Name entry
void character_creation_set_name(CharacterCreation *creation, const char *name) {
    if (!creation || !name) return;
    
    strncpy(creation->name, name, sizeof(creation->name) - 1);
    creation->name[sizeof(creation->name) - 1] = '\0';
    creation->name_entered = true;
}

// Apply racial ability score modifiers
AbilityScores character_creation_apply_racial_modifiers(const AbilityScores *base_scores, const RaceConfig *race) {
    AbilityScores modified = *base_scores;
    
    if (!race) return modified;
    
    modified.strength += race->ability_modifiers.strength;
    modified.dexterity += race->ability_modifiers.dexterity;
    modified.constitution += race->ability_modifiers.constitution;
    modified.intelligence += race->ability_modifiers.intelligence;
    modified.wisdom += race->ability_modifiers.wisdom;
    modified.charisma += race->ability_modifiers.charisma;
    
    // Ensure scores don't go below 3 or above 18
    if (modified.strength < 3) modified.strength = 3;
    if (modified.strength > 18) modified.strength = 18;
    if (modified.dexterity < 3) modified.dexterity = 3;
    if (modified.dexterity > 18) modified.dexterity = 18;
    if (modified.constitution < 3) modified.constitution = 3;
    if (modified.constitution > 18) modified.constitution = 18;
    if (modified.intelligence < 3) modified.intelligence = 3;
    if (modified.intelligence > 18) modified.intelligence = 18;
    if (modified.wisdom < 3) modified.wisdom = 3;
    if (modified.wisdom > 18) modified.wisdom = 18;
    if (modified.charisma < 3) modified.charisma = 3;
    if (modified.charisma > 18) modified.charisma = 18;
    
    return modified;
}

// Finalize character creation and create player entity
Entity character_creation_finalize(CharacterCreation *creation) {
    if (!creation || !creation->stats_rolled || creation->selected_race < 0 || creation->selected_class < 0) {
        return INVALID_ENTITY;
    }
    
    CharacterConfig *config = get_character_config();
    if (!config) return INVALID_ENTITY;
    
    AppState *app_state = appstate_get();
    if (!app_state) {
        LOG_ERROR("AppState not initialized");
        return INVALID_ENTITY;
    }
    
    RaceConfig *race = &config->races[creation->selected_race];
    ClassConfig *class = &config->classes[creation->selected_class];
    
    // Apply racial modifiers to ability scores
    AbilityScores final_scores = character_creation_apply_racial_modifiers(&creation->scores, race);
    
    // Create player entity
    Entity player = entity_create(app_state);
    if (player == INVALID_ENTITY) {
        LOG_ERROR("Failed to create player entity");
        return INVALID_ENTITY;
    }
    
    // Add Position component
    Position pos = {0, 0, INVALID_ENTITY};
    component_add(app_state, player, component_get_id(app_state, "Position"), &pos);
    
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
             race->name, class->name, creation->name);
    component_add(app_state, player, component_get_id(app_state, "BaseInfo"), &base_info);
    
    // Add Actor component with rolled stats
    Actor actor = {0};
    actor.energy = 100;
    actor.energy_per_turn = 10;
    
    // Convert ability scores to game stats
    actor.hp = 10 + character_creation_get_ability_modifier(final_scores.constitution);
    actor.max_hp = actor.hp;
    actor.strength = final_scores.strength;
    actor.attack = 5 + character_creation_get_ability_modifier(final_scores.strength);
    actor.defense = 10 + character_creation_get_ability_modifier(final_scores.dexterity);
    actor.damage_dice = 1;
    actor.damage_sides = 6;
    actor.damage_bonus = character_creation_get_ability_modifier(final_scores.strength);
    
    component_add(app_state, player, component_get_id(app_state, "Actor"), &actor);
    
    // Add Action component
    Action action = {ACTION_NONE, 0};
    component_add(app_state, player, component_get_id(app_state, "Action"), &action);
    
    // Add Inventory component  
    Inventory inventory = {0};
    inventory.max_items = 10 + character_creation_get_ability_modifier(final_scores.strength);
    component_add(app_state, player, component_get_id(app_state, "Inventory"), &inventory);
    
    // Add FieldOfView component - essential for dungeon rendering
    CompactFieldOfView player_fov;
    field_init_compact(&player_fov, FOV_RADIUS);
    if (!component_add(app_state, player, component_get_id(app_state, "FieldOfView"), &player_fov)) {
        LOG_ERROR("Failed to add FieldOfView component to player");
        entity_destroy(app_state, player);
        return INVALID_ENTITY;
    }
    LOG_INFO("Added FieldOfView component to custom player");
    
    creation->creation_complete = true;
    app_state->player = player;
    
    LOG_INFO("Character creation complete: %s the %s %s (HP: %d, STR: %d)", 
             creation->name, race->name, class->name, actor.hp, actor.strength);
    
    return player;
}

// Navigation functions
void character_creation_next_step(CharacterCreation *creation) {
    if (!creation) return;
    
    switch (creation->current_step) {
        case STEP_STATS:
            if (creation->stats_rolled) {
                creation->current_step = STEP_RACE;
                character_creation_update_validation_message(creation);
            }
            break;
        case STEP_RACE:
            if (creation->race_selected) {
                creation->current_step = STEP_CLASS;
                character_creation_update_validation_message(creation);
            }
            break;
        case STEP_CLASS:
            if (creation->class_selected) {
                creation->current_step = STEP_REVIEW;
            }
            break;
        case STEP_REVIEW:
            // Already at final step
            break;
    }
}

void character_creation_previous_step(CharacterCreation *creation) {
    if (!creation) return;
    
    switch (creation->current_step) {
        case STEP_STATS:
            // Already at first step
            break;
        case STEP_RACE:
            creation->current_step = STEP_STATS;
            break;
        case STEP_CLASS:
            creation->current_step = STEP_RACE;
            character_creation_update_validation_message(creation);
            break;
        case STEP_REVIEW:
            creation->current_step = STEP_CLASS;
            character_creation_update_validation_message(creation);
            break;
    }
}

void character_creation_goto_step(CharacterCreation *creation, CreationStep step) {
    if (!creation) return;
    
    // Only allow going to valid steps based on current progress
    switch (step) {
        case STEP_STATS:
            creation->current_step = STEP_STATS;
            break;
        case STEP_RACE:
            if (creation->stats_rolled) {
                creation->current_step = STEP_RACE;
                character_creation_update_validation_message(creation);
            }
            break;
        case STEP_CLASS:
            if (creation->stats_rolled && creation->race_selected) {
                creation->current_step = STEP_CLASS;
                character_creation_update_validation_message(creation);
            }
            break;
        case STEP_REVIEW:
            if (creation->stats_rolled && creation->race_selected && creation->class_selected) {
                creation->current_step = STEP_REVIEW;
            }
            break;
    }
}

// Validation and information functions
void character_creation_update_validation_message(CharacterCreation *creation) {
    if (!creation) return;
    
    creation->validation_message[0] = '\0';
    
    CharacterConfig *config = get_character_config();
    if (!config) return;
    
    if (creation->current_step == STEP_RACE) {
        // Check which races are available
        int available_count = 0;
        for (int i = 0; i < config->race_count; i++) {
            if (character_creation_can_select_race(&creation->scores, &config->races[i])) {
                available_count++;
            }
        }
        if (available_count == 0) {
            strcpy(creation->validation_message, "No races available with current ability scores! Consider rerolling.");
        }
    } else if (creation->current_step == STEP_CLASS) {
        // Check which classes are available
        RaceConfig *race = (creation->selected_race >= 0) ? &config->races[creation->selected_race] : NULL;
        int available_count = 0;
        for (int i = 0; i < config->class_count; i++) {
            if (character_creation_can_select_class(&creation->scores, race, &config->classes[i])) {
                available_count++;
            }
        }
        if (available_count == 0 && race) {
            snprintf(creation->validation_message, sizeof(creation->validation_message),
                    "No classes available for %s with current ability scores!", race->name);
        }
    }
}

void character_creation_get_race_requirements_text(const RaceConfig *race, char *buffer, size_t buffer_size) {
    if (!race || !buffer || buffer_size == 0) return;
    
    buffer[0] = '\0';
    bool has_requirements = false;
    
    if (race->requirements.strength > 0 || race->requirements.dexterity > 0 || 
        race->requirements.constitution > 0 || race->requirements.intelligence > 0 ||
        race->requirements.wisdom > 0 || race->requirements.charisma > 0) {
        
        strcat(buffer, "Requirements: ");
        has_requirements = true;
        
        if (race->requirements.strength > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "STR %d ", race->requirements.strength);
            strcat(buffer, temp);
        }
        if (race->requirements.dexterity > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "DEX %d ", race->requirements.dexterity);
            strcat(buffer, temp);
        }
        if (race->requirements.constitution > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "CON %d ", race->requirements.constitution);
            strcat(buffer, temp);
        }
        if (race->requirements.intelligence > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "INT %d ", race->requirements.intelligence);
            strcat(buffer, temp);
        }
        if (race->requirements.wisdom > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "WIS %d ", race->requirements.wisdom);
            strcat(buffer, temp);
        }
        if (race->requirements.charisma > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "CHA %d ", race->requirements.charisma);
            strcat(buffer, temp);
        }
    }
    
    if (!has_requirements) {
        strcpy(buffer, "No special requirements");
    }
}

void character_creation_get_class_requirements_text(const ClassConfig *class, const RaceConfig *race, char *buffer, size_t buffer_size) {
    if (!class || !buffer || buffer_size == 0) return;
    
    buffer[0] = '\0';
    bool has_requirements = false;
    
    // Ability requirements
    if (class->requirements.strength > 0 || class->requirements.dexterity > 0 || 
        class->requirements.constitution > 0 || class->requirements.intelligence > 0 ||
        class->requirements.wisdom > 0 || class->requirements.charisma > 0) {
        
        strcat(buffer, "Requirements: ");
        has_requirements = true;
        
        if (class->requirements.strength > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "STR %d ", class->requirements.strength);
            strcat(buffer, temp);
        }
        if (class->requirements.dexterity > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "DEX %d ", class->requirements.dexterity);
            strcat(buffer, temp);
        }
        if (class->requirements.constitution > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "CON %d ", class->requirements.constitution);
            strcat(buffer, temp);
        }
        if (class->requirements.intelligence > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "INT %d ", class->requirements.intelligence);
            strcat(buffer, temp);
        }
        if (class->requirements.wisdom > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "WIS %d ", class->requirements.wisdom);
            strcat(buffer, temp);
        }
        if (class->requirements.charisma > 0) {
            char temp[32];
            snprintf(temp, sizeof(temp), "CHA %d ", class->requirements.charisma);
            strcat(buffer, temp);
        }
    }
    
    // Check racial restrictions
    if (race) {
        for (int i = 0; i < race->restriction_count; i++) {
            if (strstr(race->restrictions[i], class->name) != NULL && 
                strstr(race->restrictions[i], "Cannot") != NULL) {
                if (has_requirements) strcat(buffer, " | ");
                strcat(buffer, "Restricted by race");
                break;
            }
        }
    }
    
    if (!has_requirements && buffer[0] == '\0') {
        strcpy(buffer, "No special requirements");
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
    
    CharacterConfig *config = get_character_config();
    if (!config) return;
    
    // Global navigation keys (work from any step)
    switch (key) {
        case SDLK_TAB:  // Next step
            character_creation_next_step(creation);
            return;
        case SDLK_BACKSPACE:  // Previous step
            character_creation_previous_step(creation);
            return;
        case SDLK_F1:  // Go to stats step
            character_creation_goto_step(creation, STEP_STATS);
            return;
        case SDLK_F2:  // Go to race step
            character_creation_goto_step(creation, STEP_RACE);
            return;
        case SDLK_F3:  // Go to class step
            character_creation_goto_step(creation, STEP_CLASS);
            return;
        case SDLK_F4:  // Go to review step
            character_creation_goto_step(creation, STEP_REVIEW);
            return;
        case SDLK_i:  // Toggle detailed info
            creation->show_detailed_info = !creation->show_detailed_info;
            return;
    }
    
    // Step-specific input handling
    switch (creation->current_step) {
        case STEP_STATS:
            switch (key) {
                case SDLK_r:
                case SDLK_SPACE:
                    character_creation_reroll_stats(creation);
                    character_creation_update_validation_message(creation);
                    break;
                case SDLK_RETURN:
                    if (creation->stats_rolled) {
                        character_creation_next_step(creation);
                    }
                    break;
            }
            break;
            
        case STEP_RACE:
            if (key >= SDLK_1 && key <= SDLK_9) {
                int race_index = key - SDLK_1;
                if (race_index < config->race_count) {
                    if (character_creation_can_select_race(&creation->scores, &config->races[race_index])) {
                        character_creation_select_race(creation, race_index);
                        character_creation_next_step(creation);
                    } else {
                        // Show why this race can't be selected
                        snprintf(creation->validation_message, sizeof(creation->validation_message),
                                "Cannot select %s: insufficient ability scores", config->races[race_index].name);
                    }
                }
            }
            switch (key) {
                case SDLK_UP:
                    creation->current_race_selection = (creation->current_race_selection - 1 + config->race_count) % config->race_count;
                    creation->info_target = creation->current_race_selection;
                    break;
                case SDLK_DOWN:
                    creation->current_race_selection = (creation->current_race_selection + 1) % config->race_count;
                    creation->info_target = creation->current_race_selection;
                    break;
                case SDLK_RETURN:
                    if (character_creation_can_select_race(&creation->scores, &config->races[creation->current_race_selection])) {
                        character_creation_select_race(creation, creation->current_race_selection);
                        character_creation_next_step(creation);
                    } else {
                        snprintf(creation->validation_message, sizeof(creation->validation_message),
                                "Cannot select %s: insufficient ability scores", 
                                config->races[creation->current_race_selection].name);
                    }
                    break;
            }
            break;
            
        case STEP_CLASS:
            if (key >= SDLK_1 && key <= SDLK_9) {
                int class_index = key - SDLK_1;
                if (class_index < config->class_count) {
                    RaceConfig *race = (creation->selected_race >= 0) ? &config->races[creation->selected_race] : NULL;
                    if (character_creation_can_select_class(&creation->scores, race, &config->classes[class_index])) {
                        character_creation_select_class(creation, class_index);
                        character_creation_next_step(creation);
                    } else {
                        char requirements[256];
                        character_creation_get_class_requirements_text(&config->classes[class_index], race, requirements, sizeof(requirements));
                        snprintf(creation->validation_message, sizeof(creation->validation_message),
                                "Cannot select %s: %s", config->classes[class_index].name, requirements);
                    }
                }
            }
            switch (key) {
                case SDLK_UP:
                    creation->current_class_selection = (creation->current_class_selection - 1 + config->class_count) % config->class_count;
                    creation->info_target = creation->current_class_selection;
                    break;
                case SDLK_DOWN:
                    creation->current_class_selection = (creation->current_class_selection + 1) % config->class_count;
                    creation->info_target = creation->current_class_selection;
                    break;
                case SDLK_RETURN:
                    {
                        RaceConfig *race = (creation->selected_race >= 0) ? &config->races[creation->selected_race] : NULL;
                        if (character_creation_can_select_class(&creation->scores, race, &config->classes[creation->current_class_selection])) {
                            character_creation_select_class(creation, creation->current_class_selection);
                            character_creation_next_step(creation);
                        } else {
                            char requirements[256];
                            character_creation_get_class_requirements_text(&config->classes[creation->current_class_selection], race, requirements, sizeof(requirements));
                            snprintf(creation->validation_message, sizeof(creation->validation_message),
                                    "Cannot select %s: %s", config->classes[creation->current_class_selection].name, requirements);
                        }
                    }
                    break;
            }
            break;
            
        case STEP_REVIEW:
            switch (key) {
                case SDLK_RETURN:
                    if (creation->stats_rolled && creation->race_selected && creation->class_selected) {
                        creation->creation_complete = true;
                        LOG_INFO("Character creation marked as complete");
                    }
                    break;
            }
            break;
    }
}

// Helper function to wrap text within a specified width
static void render_wrapped_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, int max_width, SDL_Color color, int *final_y) {
    if (!text || !font) {
        if (final_y) *final_y = y;
        return;
    }
    
    char *text_copy = strdup(text);
    char *line_start = text_copy;
    char *current_pos = text_copy;
    int current_y = y;
    int line_height = TTF_FontHeight(font);
    
    while (*current_pos) {
        char *word_end = current_pos;
        
        // Find the end of the current word
        while (*word_end && *word_end != ' ' && *word_end != '\n') {
            word_end++;
        }
        
        // Check if this word fits on the current line
        char saved_char = *word_end;
        *word_end = '\0';
        
        int text_width, text_height;
        TTF_SizeText(font, line_start, &text_width, &text_height);
        
        if (text_width > max_width && current_pos != line_start) {
            // Word doesn't fit, render current line and start new one
            render_text_at_position(renderer, font, line_start, x, current_y, color);
            current_y += line_height + 2;
            line_start = current_pos;
            *word_end = saved_char;
        } else {
            *word_end = saved_char;
            current_pos = word_end;
            
            if (*current_pos == '\n') {
                // Force new line
                *current_pos = '\0';
                render_text_at_position(renderer, font, line_start, x, current_y, color);
                current_y += line_height + 2;
                current_pos++;
                line_start = current_pos;
            } else if (*current_pos == ' ') {
                current_pos++;
            }
        }
    }
    
    // Render final line if there's text remaining
    if (line_start < current_pos) {
        render_text_at_position(renderer, font, line_start, x, current_y, color);
        current_y += line_height + 2;
    }
    
    if (final_y) *final_y = current_y;
    free(text_copy);
}

// Character creation render function
void character_creation_render(CharacterCreation *creation, SDL_Renderer *renderer) {
    if (!creation || !renderer) {
        LOG_ERROR("Invalid parameters for character creation render");
        return;
    }
    
    // Clear screen with dark blue background
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);
    
    AppState *app_state = appstate_get();
    if (!app_state) {
        LOG_ERROR("AppState not available for character creation render");
        return;
    }
    
    TTF_Font *font = render_system_get_medium_font(app_state);
    if (!font) {
        LOG_ERROR("Font not available for character creation render");
        return;
    }
    
    // Define colors
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color green = {100, 255, 100, 255};
    SDL_Color red = {255, 100, 100, 255};
    SDL_Color cyan = {100, 255, 255, 255};
    SDL_Color gray = {150, 150, 150, 255};
    SDL_Color light_blue = {150, 200, 255, 255};
    
    // Check if configuration is loaded
    CharacterConfig *config = get_character_config();
    if (!config || !config->loaded) {
        render_text_at_position(renderer, font, "ERROR: Character configuration not loaded!", 20, 60, red);
        render_text_at_position(renderer, font, "Check race.json and class.json files", 20, 100, white);
        return;
    }
    
    // Header
    render_text_at_position(renderer, font, "CHARACTER CREATION - Basic Fantasy RPG", 20, 10, cyan);
    
    // Step indicators
    int step_y = 40;
    char *step_names[] = {"1. Stats", "2. Race", "3. Class", "4. Review"};
    SDL_Color step_colors[] = {gray, gray, gray, gray};
    step_colors[creation->current_step] = yellow;
    
    for (int i = 0; i < 4; i++) {
        render_text_at_position(renderer, font, step_names[i], 20 + i * 120, step_y, step_colors[i]);
    }
    
    // Main content area starts at y=80
    int content_y = 80;
    
    switch (creation->current_step) {
        case STEP_STATS:
            render_text_at_position(renderer, font, "ABILITY SCORES", 20, content_y, yellow);
            content_y += 30;
            
            if (creation->stats_rolled) {
                char stats_text[256];
                snprintf(stats_text, sizeof(stats_text), 
                        "STR: %2d (%+d)  DEX: %2d (%+d)  CON: %2d (%+d)",
                        creation->scores.strength, character_creation_get_ability_modifier(creation->scores.strength),
                        creation->scores.dexterity, character_creation_get_ability_modifier(creation->scores.dexterity),
                        creation->scores.constitution, character_creation_get_ability_modifier(creation->scores.constitution));
                render_text_at_position(renderer, font, stats_text, 20, content_y, white);
                content_y += 25;
                
                snprintf(stats_text, sizeof(stats_text), 
                        "INT: %2d (%+d)  WIS: %2d (%+d)  CHA: %2d (%+d)",
                        creation->scores.intelligence, character_creation_get_ability_modifier(creation->scores.intelligence),
                        creation->scores.wisdom, character_creation_get_ability_modifier(creation->scores.wisdom),
                        creation->scores.charisma, character_creation_get_ability_modifier(creation->scores.charisma));
                render_text_at_position(renderer, font, stats_text, 20, content_y, white);
                content_y += 40;
                
                render_text_at_position(renderer, font, "Press R/SPACE to reroll stats", 20, content_y, light_blue);
                content_y += 20;
                render_text_at_position(renderer, font, "Press ENTER or TAB to continue to race selection", 20, content_y, light_blue);
            } else {
                render_text_at_position(renderer, font, "Press R or SPACE to roll your ability scores", 20, content_y, light_blue);
                content_y += 40;
                
                render_text_at_position(renderer, font, "Ability scores determine your character's capabilities:", 20, content_y, white);
                content_y += 25;
                render_text_at_position(renderer, font, "STR - Physical strength, melee damage", 40, content_y, gray);
                content_y += 20;
                render_text_at_position(renderer, font, "DEX - Agility, missile accuracy, armor class", 40, content_y, gray);
                content_y += 20;
                render_text_at_position(renderer, font, "CON - Health, hit points, endurance", 40, content_y, gray);
                content_y += 20;
                render_text_at_position(renderer, font, "INT - Reasoning, magic-user spells", 40, content_y, gray);
                content_y += 20;
                render_text_at_position(renderer, font, "WIS - Perception, cleric spells", 40, content_y, gray);
                content_y += 20;
                render_text_at_position(renderer, font, "CHA - Leadership, reaction rolls", 40, content_y, gray);
            }
            break;
            
        case STEP_RACE:
            render_text_at_position(renderer, font, "SELECT RACE", 20, content_y, yellow);
            content_y += 30;
            
            for (int i = 0; i < config->race_count; i++) {
                bool can_select = character_creation_can_select_race(&creation->scores, &config->races[i]);
                bool is_selected = (creation->selected_race == i);
                bool is_highlighted = (creation->current_race_selection == i);
                
                SDL_Color color = is_selected ? green : (can_select ? white : red);
                if (is_highlighted && !is_selected) color = yellow;
                
                char race_line[256];
                snprintf(race_line, sizeof(race_line), "%d. %s", i + 1, config->races[i].name);
                if (is_selected) strcat(race_line, " [SELECTED]");
                if (!can_select) strcat(race_line, " [UNAVAILABLE]");
                
                render_text_at_position(renderer, font, race_line, 20, content_y, color);
                content_y += 25;
                
                // Show basic description
                if (creation->show_detailed_info || is_highlighted) {
                    int final_y;
                    render_wrapped_text(renderer, font, config->races[i].description, 40, content_y, 700, gray, &final_y);
                    content_y = final_y + 10;
                    
                    if (creation->show_detailed_info) {
                        // Show modifiers
                        if (config->races[i].ability_modifiers.strength != 0 || config->races[i].ability_modifiers.dexterity != 0 ||
                            config->races[i].ability_modifiers.constitution != 0 || config->races[i].ability_modifiers.intelligence != 0 ||
                            config->races[i].ability_modifiers.wisdom != 0 || config->races[i].ability_modifiers.charisma != 0) {
                            
                            char modifiers[256] = "Ability Modifiers: ";
                            if (config->races[i].ability_modifiers.strength != 0) {
                                char temp[32];
                                snprintf(temp, sizeof(temp), "STR %+d ", config->races[i].ability_modifiers.strength);
                                strcat(modifiers, temp);
                            }
                            if (config->races[i].ability_modifiers.dexterity != 0) {
                                char temp[32];
                                snprintf(temp, sizeof(temp), "DEX %+d ", config->races[i].ability_modifiers.dexterity);
                                strcat(modifiers, temp);
                            }
                            if (config->races[i].ability_modifiers.constitution != 0) {
                                char temp[32];
                                snprintf(temp, sizeof(temp), "CON %+d ", config->races[i].ability_modifiers.constitution);
                                strcat(modifiers, temp);
                            }
                            if (config->races[i].ability_modifiers.intelligence != 0) {
                                char temp[32];
                                snprintf(temp, sizeof(temp), "INT %+d ", config->races[i].ability_modifiers.intelligence);
                                strcat(modifiers, temp);
                            }
                            if (config->races[i].ability_modifiers.wisdom != 0) {
                                char temp[32];
                                snprintf(temp, sizeof(temp), "WIS %+d ", config->races[i].ability_modifiers.wisdom);
                                strcat(modifiers, temp);
                            }
                            if (config->races[i].ability_modifiers.charisma != 0) {
                                char temp[32];
                                snprintf(temp, sizeof(temp), "CHA %+d ", config->races[i].ability_modifiers.charisma);
                                strcat(modifiers, temp);
                            }
                            
                            render_text_at_position(renderer, font, modifiers, 40, content_y, light_blue);
                            content_y += 20;
                        }
                        
                        // Show special abilities
                        if (config->races[i].special_ability_count > 0) {
                            render_text_at_position(renderer, font, "Special Abilities:", 40, content_y, light_blue);
                            content_y += 20;
                            for (int j = 0; j < config->races[i].special_ability_count; j++) {
                                char ability_text[512];
                                snprintf(ability_text, sizeof(ability_text), "• %s: %s", 
                                        config->races[i].special_abilities[j].name,
                                        config->races[i].special_abilities[j].description);
                                render_wrapped_text(renderer, font, ability_text, 60, content_y, 680, gray, &content_y);
                                content_y += 5;
                            }
                        }
                        
                        content_y += 15;
                    }
                }
            }
            
            render_text_at_position(renderer, font, "Use UP/DOWN arrows to browse, numbers 1-9 or ENTER to select", 20, content_y + 20, light_blue);
            break;
            
        case STEP_CLASS:
            render_text_at_position(renderer, font, "SELECT CLASS", 20, content_y, yellow);
            content_y += 30;
            
            RaceConfig *selected_race = (creation->selected_race >= 0) ? &config->races[creation->selected_race] : NULL;
            
            for (int i = 0; i < config->class_count; i++) {
                bool can_select = character_creation_can_select_class(&creation->scores, selected_race, &config->classes[i]);
                bool is_selected = (creation->selected_class == i);
                bool is_highlighted = (creation->current_class_selection == i);
                
                SDL_Color color = is_selected ? green : (can_select ? white : red);
                if (is_highlighted && !is_selected) color = yellow;
                
                char class_line[256];
                snprintf(class_line, sizeof(class_line), "%d. %s", i + 1, config->classes[i].name);
                if (is_selected) strcat(class_line, " [SELECTED]");
                if (!can_select) strcat(class_line, " [UNAVAILABLE]");
                
                render_text_at_position(renderer, font, class_line, 20, content_y, color);
                content_y += 25;
                
                // Show basic description
                if (creation->show_detailed_info || is_highlighted) {
                    int final_y;
                    render_wrapped_text(renderer, font, config->classes[i].description, 40, content_y, 700, gray, &final_y);
                    content_y = final_y + 10;
                    
                    if (creation->show_detailed_info) {
                        // Show requirements and other details
                        char requirements[256];
                        character_creation_get_class_requirements_text(&config->classes[i], selected_race, requirements, sizeof(requirements));
                        render_text_at_position(renderer, font, requirements, 40, content_y, light_blue);
                        content_y += 20;
                        
                        char details[256];
                        snprintf(details, sizeof(details), "Hit Die: %s | Role: %s", 
                                config->classes[i].hit_die, config->classes[i].role);
                        render_text_at_position(renderer, font, details, 40, content_y, light_blue);
                        content_y += 20;
                        
                        // Show special abilities
                        if (config->classes[i].special_ability_count > 0) {
                            render_text_at_position(renderer, font, "Special Abilities:", 40, content_y, light_blue);
                            content_y += 20;
                            for (int j = 0; j < config->classes[i].special_ability_count; j++) {
                                char ability_text[512];
                                snprintf(ability_text, sizeof(ability_text), "• %s: %s", 
                                        config->classes[i].special_abilities[j].name,
                                        config->classes[i].special_abilities[j].description);
                                render_wrapped_text(renderer, font, ability_text, 60, content_y, 680, gray, &content_y);
                                content_y += 5;
                            }
                        }
                        
                        content_y += 15;
                    }
                }
            }
            
            render_text_at_position(renderer, font, "Use UP/DOWN arrows to browse, numbers 1-9 or ENTER to select", 20, content_y + 20, light_blue);
            break;
            
        case STEP_REVIEW:
            render_text_at_position(renderer, font, "CHARACTER REVIEW", 20, content_y, yellow);
            content_y += 40;
            
            // Show final character stats
            if (creation->selected_race >= 0) {
                AbilityScores final_scores = character_creation_apply_racial_modifiers(&creation->scores, &config->races[creation->selected_race]);
                
                char char_summary[256];
                snprintf(char_summary, sizeof(char_summary), "%s the %s %s", 
                        creation->name, 
                        config->races[creation->selected_race].name,
                        config->classes[creation->selected_class].name);
                render_text_at_position(renderer, font, char_summary, 20, content_y, green);
                content_y += 40;
                
                render_text_at_position(renderer, font, "Final Ability Scores (including racial modifiers):", 20, content_y, white);
                content_y += 25;
                
                char stats_text[256];
                snprintf(stats_text, sizeof(stats_text), 
                        "STR: %2d (%+d)  DEX: %2d (%+d)  CON: %2d (%+d)",
                        final_scores.strength, character_creation_get_ability_modifier(final_scores.strength),
                        final_scores.dexterity, character_creation_get_ability_modifier(final_scores.dexterity),
                        final_scores.constitution, character_creation_get_ability_modifier(final_scores.constitution));
                render_text_at_position(renderer, font, stats_text, 20, content_y, white);
                content_y += 25;
                
                snprintf(stats_text, sizeof(stats_text), 
                        "INT: %2d (%+d)  WIS: %2d (%+d)  CHA: %2d (%+d)",
                        final_scores.intelligence, character_creation_get_ability_modifier(final_scores.intelligence),
                        final_scores.wisdom, character_creation_get_ability_modifier(final_scores.wisdom),
                        final_scores.charisma, character_creation_get_ability_modifier(final_scores.charisma));
                render_text_at_position(renderer, font, stats_text, 20, content_y, white);
                content_y += 40;
                
                render_text_at_position(renderer, font, "Press ENTER to begin your adventure!", 20, content_y, green);
                render_text_at_position(renderer, font, "Use BACKSPACE or F1-F3 to go back and modify your character", 20, content_y + 25, light_blue);
            }
            break;
    }
    
    // Show validation message if present
    if (creation->validation_message[0] != '\0') {
        render_text_at_position(renderer, font, creation->validation_message, 20, 550, red);
    }
    
    // Navigation help
    render_text_at_position(renderer, font, "Navigation: TAB=Next, BACKSPACE=Previous, F1-F4=Jump to step, I=Toggle detail view", 20, 580, gray);
    
    // Present the rendered frame to the screen
    SDL_RenderPresent(renderer);
}
