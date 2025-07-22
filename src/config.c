#include "config.h"
#include "log.h"
#include "error.h"
#include "appstate.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Default configuration values
static const GameConfig DEFAULT_CONFIG = {
    .ecs = {
        .max_entities = 1000,
        .max_components = 32,
        .max_systems = 32,
        .initial_component_capacity = 16
    },
    .dungeon = {
        .width = 100,
        .height = 100,
        .max_rooms = 20,
        .min_room_size = 5,
        .max_room_size = 15
    },
    .render = {
        .cell_size = 16,
        .sidebar_width = 12,
        .game_area_width = 48,
        .game_area_height = 30,
        .status_line_height = 1,
        .window_title = "Adventure Game"
    },
    .fov = {
        .radius = 8,
        .grid_size = 17  // Will be recalculated
    },
    .spatial = {
        .cell_size = 10,
        .grid_width = 10,   // Will be recalculated
        .grid_height = 10,  // Will be recalculated
        .max_entities_per_cell = 32
    },
    .inventory = {
        .max_items = 40
    },
    .message = {
        .queue_length = 100,
        .max_text_length = 512,
        .max_wrapped_lines = 20
    },
    .message_view = {
        .default_width = 400,
        .default_height = 300,
        .min_width = 200,
        .min_height = 150,
        .line_height = 18,
        .margin = 10,
        .scrollbar_width = 20
    },
    .mempool = {
        .initial_chunks_per_pool = 1,
        .max_chunks_per_pool = 64,
        .enable_corruption_detection = true,
        .enable_statistics = true,
        .enable_pool_allocation = true
    },
    .loaded = false,
    .config_file_path = ""
};

// Validation limits
static const struct {
    struct { uint32_t min, max; } max_entities;
    struct { uint32_t min, max; } max_components;
    struct { uint32_t min, max; } max_systems;
    struct { uint32_t min, max; } initial_component_capacity;
} ECS_LIMITS = {
    .max_entities = {100, 10000},
    .max_components = {8, 64},
    .max_systems = {4, 64},
    .initial_component_capacity = {4, 256}
};

static const struct {
    struct { uint32_t min, max; } width;
    struct { uint32_t min, max; } height;
    struct { uint32_t min, max; } max_rooms;
    struct { uint32_t min, max; } room_size;
} DUNGEON_LIMITS = {
    .width = {50, 500},
    .height = {50, 500},
    .max_rooms = {5, 100},
    .room_size = {3, 50}
};

static const struct {
    struct { uint32_t min, max; } cell_size;
    struct { uint32_t min, max; } sidebar_width;
    struct { uint32_t min, max; } game_area_width;
    struct { uint32_t min, max; } game_area_height;
} RENDER_LIMITS = {
    .cell_size = {8, 32},
    .sidebar_width = {8, 30},
    .game_area_width = {20, 100},
    .game_area_height = {15, 50}
};

// Helper function to safely get integer from JSON
static bool json_get_uint32(const cJSON *json, const char *key, uint32_t *value) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsNumber(item)) {
        return false;
    }
    
    double num = cJSON_GetNumberValue(item);
    if (num < 0 || num > UINT32_MAX) {
        return false;
    }
    
    *value = (uint32_t)num;
    return true;
}

// Helper function to safely get string from JSON
static bool json_get_string(const cJSON *json, const char *key, char *buffer, size_t buffer_size) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsString(item)) {
        return false;
    }
    
    const char *str = cJSON_GetStringValue(item);
    if (strlen(str) >= buffer_size) {
        return false;
    }
    
    strcpy(buffer, str);
    return true;
}

// Helper function to safely get boolean from JSON
static bool json_get_bool(const cJSON *json, const char *key, bool *value) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsBool(item)) {
        return false;
    }
    
    *value = cJSON_IsTrue(item);
    return true;
}

// Load ECS configuration from JSON
static bool load_ecs_config(const cJSON *json, ECSConfig *ecs) {
    const cJSON *ecs_json = cJSON_GetObjectItemCaseSensitive(json, "ecs");
    if (!cJSON_IsObject(ecs_json)) {
        LOG_ERROR("Missing or invalid 'ecs' configuration section");
        return false;
    }
    
    if (!json_get_uint32(ecs_json, "max_entities", &ecs->max_entities) ||
        !json_get_uint32(ecs_json, "max_components", &ecs->max_components) ||
        !json_get_uint32(ecs_json, "max_systems", &ecs->max_systems) ||
        !json_get_uint32(ecs_json, "initial_component_capacity", &ecs->initial_component_capacity)) {
        LOG_ERROR("Invalid ECS configuration values");
        return false;
    }
    
    return true;
}

// Load dungeon configuration from JSON
static bool load_dungeon_config(const cJSON *json, DungeonConfig *dungeon) {
    const cJSON *dungeon_json = cJSON_GetObjectItemCaseSensitive(json, "dungeon");
    if (!cJSON_IsObject(dungeon_json)) {
        LOG_ERROR("Missing or invalid 'dungeon' configuration section");
        return false;
    }
    
    if (!json_get_uint32(dungeon_json, "width", &dungeon->width) ||
        !json_get_uint32(dungeon_json, "height", &dungeon->height) ||
        !json_get_uint32(dungeon_json, "max_rooms", &dungeon->max_rooms) ||
        !json_get_uint32(dungeon_json, "min_room_size", &dungeon->min_room_size) ||
        !json_get_uint32(dungeon_json, "max_room_size", &dungeon->max_room_size)) {
        LOG_ERROR("Invalid dungeon configuration values");
        return false;
    }
    
    return true;
}

// Load render configuration from JSON
static bool load_render_config(const cJSON *json, RenderConfig *render) {
    const cJSON *render_json = cJSON_GetObjectItemCaseSensitive(json, "render");
    if (!cJSON_IsObject(render_json)) {
        LOG_ERROR("Missing or invalid 'render' configuration section");
        return false;
    }
    
    if (!json_get_uint32(render_json, "cell_size", &render->cell_size) ||
        !json_get_uint32(render_json, "sidebar_width", &render->sidebar_width) ||
        !json_get_uint32(render_json, "game_area_width", &render->game_area_width) ||
        !json_get_uint32(render_json, "game_area_height", &render->game_area_height) ||
        !json_get_uint32(render_json, "status_line_height", &render->status_line_height)) {
        LOG_ERROR("Invalid render configuration values");
        return false;
    }
    
    // Window title is optional
    if (!json_get_string(render_json, "window_title", render->window_title, sizeof(render->window_title))) {
        strcpy(render->window_title, DEFAULT_CONFIG.render.window_title);
    }
    
    return true;
}

// Load remaining configurations (simplified for brevity)
static bool load_remaining_configs(const cJSON *json, struct AppState *app_state) {
    // FOV
    const cJSON *fov_json = cJSON_GetObjectItemCaseSensitive(json, "fov");
    if (cJSON_IsObject(fov_json)) {
        json_get_uint32(fov_json, "radius", &app_state->config.fov.radius);
    }
    
    // Spatial
    const cJSON *spatial_json = cJSON_GetObjectItemCaseSensitive(json, "spatial");
    if (cJSON_IsObject(spatial_json)) {
        json_get_uint32(spatial_json, "cell_size", &app_state->config.spatial.cell_size);
        json_get_uint32(spatial_json, "max_entities_per_cell", &app_state->config.spatial.max_entities_per_cell);
    }
    
    // Inventory
    const cJSON *inventory_json = cJSON_GetObjectItemCaseSensitive(json, "inventory");
    if (cJSON_IsObject(inventory_json)) {
        json_get_uint32(inventory_json, "max_items", &app_state->config.inventory.max_items);
    }
    
    // Messages
    const cJSON *message_json = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cJSON_IsObject(message_json)) {
        json_get_uint32(message_json, "queue_length", &app_state->config.message.queue_length);
        json_get_uint32(message_json, "max_text_length", &app_state->config.message.max_text_length);
        json_get_uint32(message_json, "max_wrapped_lines", &app_state->config.message.max_wrapped_lines);
    }
    
    // Message View
    const cJSON *msg_view_json = cJSON_GetObjectItemCaseSensitive(json, "message_view");
    if (cJSON_IsObject(msg_view_json)) {
        json_get_uint32(msg_view_json, "default_width", &app_state->config.message_view.default_width);
        json_get_uint32(msg_view_json, "default_height", &app_state->config.message_view.default_height);
        json_get_uint32(msg_view_json, "min_width", &app_state->config.message_view.min_width);
        json_get_uint32(msg_view_json, "min_height", &app_state->config.message_view.min_height);
        json_get_uint32(msg_view_json, "line_height", &app_state->config.message_view.line_height);
        json_get_uint32(msg_view_json, "margin", &app_state->config.message_view.margin);
        json_get_uint32(msg_view_json, "scrollbar_width", &app_state->config.message_view.scrollbar_width);
    }
    
    // Memory Pool
    const cJSON *mempool_json = cJSON_GetObjectItemCaseSensitive(json, "mempool");
    if (cJSON_IsObject(mempool_json)) {
        json_get_uint32(mempool_json, "initial_chunks_per_pool", &app_state->config.mempool.initial_chunks_per_pool);
        json_get_uint32(mempool_json, "max_chunks_per_pool", &app_state->config.mempool.max_chunks_per_pool);
        json_get_bool(mempool_json, "enable_corruption_detection", &app_state->config.mempool.enable_corruption_detection);
        json_get_bool(mempool_json, "enable_statistics", &app_state->config.mempool.enable_statistics);
        json_get_bool(mempool_json, "enable_pool_allocation", &app_state->config.mempool.enable_pool_allocation);
    }
    
    return true;
}

// Calculate derived values
static void calculate_derived_values(struct AppState *app_state) {
    // FOV grid size
    app_state->config.fov.grid_size = app_state->config.fov.radius * 2 + 1;
    
    // Spatial grid dimensions
    app_state->config.spatial.grid_width = (app_state->config.dungeon.width + app_state->config.spatial.cell_size - 1) / app_state->config.spatial.cell_size;
    app_state->config.spatial.grid_height = (app_state->config.dungeon.height + app_state->config.spatial.cell_size - 1) / app_state->config.spatial.cell_size;
}

// Validate configuration ranges
static bool validate_ranges(struct AppState *app_state) {
    bool valid = true;
    
    // Validate ECS limits
    if (app_state->config.ecs.max_entities < ECS_LIMITS.max_entities.min || 
        app_state->config.ecs.max_entities > ECS_LIMITS.max_entities.max) {
        LOG_ERROR("max_entities (%u) out of range [%u, %u]", 
                  app_state->config.ecs.max_entities, ECS_LIMITS.max_entities.min, ECS_LIMITS.max_entities.max);
        valid = false;
    }
    
    if (app_state->config.ecs.max_components < ECS_LIMITS.max_components.min || 
        app_state->config.ecs.max_components > ECS_LIMITS.max_components.max) {
        LOG_ERROR("max_components (%u) out of range [%u, %u]", 
                  app_state->config.ecs.max_components, ECS_LIMITS.max_components.min, ECS_LIMITS.max_components.max);
        valid = false;
    }
    
    // Validate dungeon limits
    if (app_state->config.dungeon.width < DUNGEON_LIMITS.width.min || 
        app_state->config.dungeon.width > DUNGEON_LIMITS.width.max) {
        LOG_ERROR("dungeon width (%u) out of range [%u, %u]", 
                  app_state->config.dungeon.width, DUNGEON_LIMITS.width.min, DUNGEON_LIMITS.width.max);
        valid = false;
    }
    
    if (app_state->config.dungeon.min_room_size >= app_state->config.dungeon.max_room_size) {
        LOG_ERROR("min_room_size (%u) must be less than max_room_size (%u)", 
                  app_state->config.dungeon.min_room_size, app_state->config.dungeon.max_room_size);
        valid = false;
    }
    
    // Validate render limits
    if (app_state->config.render.cell_size < RENDER_LIMITS.cell_size.min || 
        app_state->config.render.cell_size > RENDER_LIMITS.cell_size.max) {
        LOG_ERROR("cell_size (%u) out of range [%u, %u]", 
                  app_state->config.render.cell_size, RENDER_LIMITS.cell_size.min, RENDER_LIMITS.cell_size.max);
        valid = false;
    }
    
    return valid;
}

// Initialize configuration system
bool config_init(struct AppState *app_state) {
    // Start with default configuration
    app_state->config = DEFAULT_CONFIG;
    
    LOG_INFO("Configuration system initialized with defaults");
    return true;
}

// Cleanup configuration system
void config_cleanup(struct AppState *app_state) {
    memset(&app_state->config, 0, sizeof(GameConfig));
    LOG_INFO("Configuration system cleaned up");
}

// Load configuration from JSON file
bool config_load_from_file(const char *filename, struct AppState *app_state) {
    if (!filename) {
        ERROR_SET(RESULT_ERROR_NULL_POINTER, "Filename cannot be NULL");
        return false;
    }
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        LOG_ERROR("Cannot open config file: %s", filename);
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        LOG_ERROR("Invalid config file size: %ld", file_size);
        return false;
    }
    
    // Read file content
    char *content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        LOG_ERROR("Failed to allocate memory for config file");
        return false;
    }
    
    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';
    fclose(file);
    
    // Parse JSON
    cJSON *json = cJSON_Parse(content);
    free(content);
    
    if (!json) {
        LOG_ERROR("Failed to parse config JSON: %s", cJSON_GetErrorPtr());
        return false;
    }
    
    // Load configuration sections
    bool success = true;
    success &= load_ecs_config(json, &app_state->config.ecs);
    success &= load_dungeon_config(json, &app_state->config.dungeon);
    success &= load_render_config(json, &app_state->config.render);
    success &= load_remaining_configs(json, app_state);
    
    cJSON_Delete(json);
    
    if (!success) {
        LOG_ERROR("Failed to load configuration from %s", filename);
        return false;
    }
    
    // Calculate derived values and validate
    calculate_derived_values(app_state);
    
    if (!config_validate(app_state)) {
        LOG_ERROR("Configuration validation failed");
        return false;
    }
    
    // Save config file path and mark as loaded
    strncpy(app_state->config.config_file_path, filename, sizeof(app_state->config.config_file_path) - 1);
    app_state->config.config_file_path[sizeof(app_state->config.config_file_path) - 1] = '\0';
    app_state->config.loaded = true;
    
    LOG_INFO("Successfully loaded configuration from %s", filename);
    return true;
}

// Validate configuration
bool config_validate(struct AppState *app_state) {
    return validate_ranges(app_state);
}

// Get configuration instance
const GameConfig* config_get(struct AppState *app_state) {
    return &app_state->config;
}

// Convenience accessors
uint32_t config_get_max_entities(struct AppState *app_state) { return app_state->config.ecs.max_entities; }
uint32_t config_get_dungeon_width(struct AppState *app_state) { return app_state->config.dungeon.width; }
uint32_t config_get_dungeon_height(struct AppState *app_state) { return app_state->config.dungeon.height; }
uint32_t config_get_cell_size(struct AppState *app_state) { return app_state->config.render.cell_size; }
uint32_t config_get_fov_radius(struct AppState *app_state) { return app_state->config.fov.radius; }

uint32_t config_get_window_width_px(struct AppState *app_state) {
    return config_get_window_width_cells(app_state) * app_state->config.render.cell_size;
}

uint32_t config_get_window_height_px(struct AppState *app_state) {
    return config_get_window_height_cells(app_state) * app_state->config.render.cell_size;
}

// Calculated derived values
uint32_t config_get_window_width_cells(struct AppState *app_state) {
    return app_state->config.render.sidebar_width + app_state->config.render.game_area_width;
}

uint32_t config_get_window_height_cells(struct AppState *app_state) {
    return app_state->config.render.game_area_height + app_state->config.render.status_line_height;
}

uint32_t config_get_fov_grid_size(struct AppState *app_state) { return app_state->config.fov.grid_size; }
uint32_t config_get_spatial_grid_width(struct AppState *app_state) { return app_state->config.spatial.grid_width; }
uint32_t config_get_spatial_grid_height(struct AppState *app_state) { return app_state->config.spatial.grid_height; } 
