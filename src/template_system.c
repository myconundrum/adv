#include "template_system.h"
#include <cjson/cJSON.h>
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ecs.h"

// Template storage structure
typedef struct {
    char* name;
    cJSON* data;
} Template;

static Template* templates = NULL;
static int template_count = 0;
static int template_capacity = 0;

int template_system_init(void) {
    templates = NULL;
    template_count = 0;
    template_capacity = 0;
    LOG_INFO("Template system initialized");
    return 0;
}

void template_system_cleanup(void) {
    if (templates) {
        for (int i = 0; i < template_count; i++) {
            if (templates[i].name) {
                free(templates[i].name);
            }
            if (templates[i].data) {
                cJSON_Delete(templates[i].data);
            }
        }
        free(templates);
        templates = NULL;
    }
    template_count = 0;
    template_capacity = 0;
}

int load_templates_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        LOG_ERROR("Failed to open template file: %s", filename);
        return -1;
    }

    // Read file content
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return -1;
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    fclose(file);

    // Parse JSON
    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    
    if (!root) {
        LOG_ERROR("Failed to parse JSON from %s", filename);
        return -1;
    }

    // Get templates array
    cJSON* templates_array = cJSON_GetObjectItem(root, "templates");
    if (!templates_array || !cJSON_IsArray(templates_array)) {
        LOG_ERROR("No 'templates' array found in %s", filename);
        cJSON_Delete(root);
        return -1;
    }

    int array_size = cJSON_GetArraySize(templates_array);
    
    // Ensure capacity
    if (template_count + array_size > template_capacity) {
        int new_capacity = template_capacity == 0 ? array_size : template_capacity * 2;
        while (new_capacity < template_count + array_size) {
            new_capacity *= 2;
        }
        
        Template* new_templates = realloc(templates, new_capacity * sizeof(Template));
        if (!new_templates) {
            cJSON_Delete(root);
            return -1;
        }
        templates = new_templates;
        template_capacity = new_capacity;
    }

    // Process each template
    for (int i = 0; i < array_size; i++) {
        cJSON* template_obj = cJSON_GetArrayItem(templates_array, i);
        if (!cJSON_IsObject(template_obj)) {
            continue;
        }

        cJSON* name_obj = cJSON_GetObjectItem(template_obj, "name");
        if (!name_obj || !cJSON_IsString(name_obj)) {
            continue;
        }

        // Check if template already exists
        int existing_index = -1;
        for (int j = 0; j < template_count; j++) {
            if (strcmp_ci(templates[j].name, name_obj->valuestring) == 0) {
                existing_index = j;
                break;
            }
        }

        if (existing_index >= 0) {
            // Replace existing template
            free(templates[existing_index].name);
            cJSON_Delete(templates[existing_index].data);
            templates[existing_index].name = strdup(name_obj->valuestring);
            templates[existing_index].data = cJSON_Duplicate(template_obj, 1);
        } else {
            // Add new template
            templates[template_count].name = strdup(name_obj->valuestring);
            templates[template_count].data = cJSON_Duplicate(template_obj, 1);
            template_count++;
        }
    }

    cJSON_Delete(root);
    LOG_INFO("Loaded %d templates from %s", array_size, filename);
    return 0;
}

Entity create_entity_from_template(const char* template_name) {
    // Find template
    Template* template = NULL;
    for (int i = 0; i < template_count; i++) {
        if (strcmp_ci(templates[i].name, template_name) == 0) {
            template = &templates[i];
            break;
        }
    }

    if (!template) {
        LOG_ERROR("Template '%s' not found", template_name);
        return INVALID_ENTITY;
    }

    // Create entity
    Entity entity = entity_create();
    if (entity == INVALID_ENTITY) {
        LOG_ERROR("Failed to create entity from template '%s'", template_name);
        return INVALID_ENTITY;
    }

    // Get components array
    cJSON* components = cJSON_GetObjectItem(template->data, "components");
    if (!components || !cJSON_IsArray(components)) {
        LOG_ERROR("No components found in template '%s'", template_name);
        entity_destroy(entity);
        return INVALID_ENTITY;
    }

    // Add each component
    int component_count = cJSON_GetArraySize(components);
    for (int i = 0; i < component_count; i++) {
        cJSON* component_obj = cJSON_GetArrayItem(components, i);
        if (!cJSON_IsObject(component_obj)) {
            continue;
        }

        cJSON* type_obj = cJSON_GetObjectItem(component_obj, "type");
        if (!type_obj || !cJSON_IsString(type_obj)) {
            continue;
        }

        const char* component_type = type_obj->valuestring;
        uint32_t component_id = component_get_id(component_type);
        
        if (component_id == 0xFFFFFFFF) { // INVALID_COMPONENT equivalent
            LOG_WARN("Unknown component type '%s' in template '%s'", component_type, template_name);
            continue;
        }

        // Create component data based on type
        void* component_data = NULL;
        
        if (strcmp_ci(component_type, "Position") == 0) {
            Position* pos = malloc(sizeof(Position));
            cJSON* x_obj = cJSON_GetObjectItem(component_obj, "x");
            cJSON* y_obj = cJSON_GetObjectItem(component_obj, "y");
            pos->x = x_obj ? x_obj->valuedouble : 0.0f;
            pos->y = y_obj ? y_obj->valuedouble : 0.0f;
            component_data = pos;
        }
        else if (strcmp_ci(component_type, "BaseInfo") == 0) {
            BaseInfo* base_info = malloc(sizeof(BaseInfo));
            cJSON* symbol_obj = cJSON_GetObjectItem(component_obj, "symbol");
            cJSON* color_obj = cJSON_GetObjectItem(component_obj, "color");
            cJSON* name_obj = cJSON_GetObjectItem(component_obj, "name");
            cJSON* is_carryable_obj = cJSON_GetObjectItem(component_obj, "is_carryable");
            
            base_info->character = symbol_obj ? symbol_obj->valuestring[0] : '?';
            base_info->color = color_obj ? color_obj->valueint : 0;
            strncpy(base_info->name, name_obj ? name_obj->valuestring : "Unknown", sizeof(base_info->name) - 1);
            base_info->name[sizeof(base_info->name) - 1] = '\0';
            base_info->is_carryable = is_carryable_obj ? is_carryable_obj->valueint : 0;
            component_data = base_info;
        }
        else if (strcmp_ci(component_type, "Actor") == 0) {
            Actor* actor = malloc(sizeof(Actor));
            cJSON* is_player_obj = cJSON_GetObjectItem(component_obj, "is_player");
            cJSON* can_carry_obj = cJSON_GetObjectItem(component_obj, "can_carry");
            cJSON* energy_obj = cJSON_GetObjectItem(component_obj, "energy");
            cJSON* energy_per_turn_obj = cJSON_GetObjectItem(component_obj, "energy_per_turn");
            cJSON* hp_obj = cJSON_GetObjectItem(component_obj, "hp");
            cJSON* strength_obj = cJSON_GetObjectItem(component_obj, "strength");
            cJSON* attack_obj = cJSON_GetObjectItem(component_obj, "attack");
            cJSON* attack_bonus_obj = cJSON_GetObjectItem(component_obj, "attack_bonus");
            cJSON* defense_obj = cJSON_GetObjectItem(component_obj, "defense");
            cJSON* defense_bonus_obj = cJSON_GetObjectItem(component_obj, "defense_bonus");
            cJSON* damage_dice_obj = cJSON_GetObjectItem(component_obj, "damage_dice");
            cJSON* damage_sides_obj = cJSON_GetObjectItem(component_obj, "damage_sides");
            cJSON* damage_bonus_obj = cJSON_GetObjectItem(component_obj, "damage_bonus");
            
            actor->is_player = is_player_obj ? is_player_obj->valueint : 0;
            actor->can_carry = can_carry_obj ? can_carry_obj->valueint : 0;
            actor->energy = energy_obj ? energy_obj->valueint : 100;
            actor->energy_per_turn = energy_per_turn_obj ? energy_per_turn_obj->valueint : 10;
            actor->hp = hp_obj ? hp_obj->valueint : 100;
            actor->strength = strength_obj ? strength_obj->valueint : 10;
            actor->attack = attack_obj ? attack_obj->valueint : 5;
            actor->attack_bonus = attack_bonus_obj ? attack_bonus_obj->valueint : 0;
            actor->defense = defense_obj ? defense_obj->valueint : 5;
            actor->defense_bonus = defense_bonus_obj ? defense_bonus_obj->valueint : 0;
            actor->damage_dice = damage_dice_obj ? damage_dice_obj->valueint : 1;
            actor->damage_sides = damage_sides_obj ? damage_sides_obj->valueint : 6;
            actor->damage_bonus = damage_bonus_obj ? damage_bonus_obj->valueint : 0;
            component_data = actor;
        }
        else if (strcmp_ci(component_type, "Action") == 0) {
            Action* action = malloc(sizeof(Action));
            cJSON* type_obj = cJSON_GetObjectItem(component_obj, "action_type");
            cJSON* data_obj = cJSON_GetObjectItem(component_obj, "action_data");
            action->type = type_obj ? type_obj->valueint : ACTION_NONE;
            action->action_data = data_obj ? data_obj->valueint : DIRECTION_NONE;
            component_data = action;
        }

        if (component_data) {
            if (!component_add(entity, component_id, component_data)) {
                LOG_ERROR("Failed to add component '%s' to entity from template '%s'", component_type, template_name);
                free(component_data);
            }
        }
    }

    LOG_INFO("Created entity %d from template '%s'", entity, template_name);
    return entity;
} 
