#include "render_system.h"
#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include "log.h"
#include "world.h"
#include "components.h"
#include "playerview.h"
#include "statusview.h"

// Global SDL objects
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static TTF_Font *g_font = NULL;
static bool g_initialized = false;

// Simple z-buffer system using arrays - now sized for game area only
typedef struct {
    char character;
    uint8_t color;
    bool has_content;
} ZBufferCell;

static ZBufferCell *g_z_buffer_0 = NULL;  // Background layer (dungeon tiles)
static ZBufferCell *g_z_buffer_1 = NULL;  // Entity layer (entities)

// Viewport state
static int g_viewport_x = 0;
static int g_viewport_y = 0;
#define VIEWPORT_MARGIN 5
#define CHUNK_X (GAME_AREA_WIDTH - 2 * VIEWPORT_MARGIN)
#define CHUNK_Y (GAME_AREA_HEIGHT - 2 * VIEWPORT_MARGIN)

// Helper function to render a tile at screen coordinates (to a specific renderer)
static void render_tile_at_screen_pos_to_renderer(SDL_Renderer *target_renderer, int screen_x, int screen_y, char symbol, uint8_t color) {
    // Set color based on the tile color
    uint8_t r, g, b;
    switch (color) {
        case 0x01: // Red
            r = 255; g = 0; b = 0;
            break;
        case 0x02: // Green
            r = 0; g = 255; b = 0;
            break;
        case 0x03: // Blue
            r = 0; g = 0; b = 255;
            break;
        case 0x04: // Yellow
            r = 255; g = 255; b = 0;
            break;
        case 0x05: // Magenta
            r = 255; g = 0; b = 255;
            break;
        case 0x06: // Cyan
            r = 0; g = 255; b = 255;
            break;
        case 0x07: // White
            r = 255; g = 255; b = 255;
            break;
        case 0x08: // Dark gray (for explored areas)
            r = 64; g = 64; b = 64;
            break;
        default: // Default to white
            r = 255; g = 255; b = 255;
            break;
    }
    
    // Try to render the character as text
    if (g_font) {
        // Create a string from the character
        char char_str[2] = {symbol, '\0'};
        
        // Set the text color
        SDL_Color text_color = {r, g, b, 255};
        
        // Render the text surface
        SDL_Surface *text_surface = TTF_RenderText_Solid(g_font, char_str, text_color);
        if (text_surface) {
            // Create texture from surface
            SDL_Texture *text_texture = SDL_CreateTextureFromSurface(target_renderer, text_surface);
            if (text_texture) {
                // Get text dimensions
                int text_w, text_h;
                SDL_QueryTexture(text_texture, NULL, NULL, &text_w, &text_h);
                
                // Center the text in the cell
                int text_x = screen_x + (CELL_SIZE - text_w) / 2;
                int text_y = screen_y + (CELL_SIZE - text_h) / 2;
                
                // Render the text
                SDL_Rect text_rect = {text_x, text_y, text_w, text_h};
                SDL_RenderCopy(target_renderer, text_texture, NULL, &text_rect);
                
                // Clean up
                SDL_DestroyTexture(text_texture);
            }
            SDL_FreeSurface(text_surface);
        }
    } else {
        // Fallback to rectangle rendering if font is not available
        SDL_Rect char_rect = {
            screen_x,
            screen_y,
            CELL_SIZE,
            CELL_SIZE
        };
        
        SDL_SetRenderDrawColor(target_renderer, r, g, b, 255);
        SDL_RenderFillRect(target_renderer, &char_rect);
        
        // Draw a border to make it more visible
        SDL_SetRenderDrawColor(target_renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(target_renderer, &char_rect);
    }
    
    // Draw graph paper grid lines (very dark, barely visible)
    SDL_SetRenderDrawColor(target_renderer, 16, 16, 16, 255); // Very dark gray
    SDL_Rect grid_rect = {screen_x, screen_y, CELL_SIZE, CELL_SIZE};
    SDL_RenderDrawRect(target_renderer, &grid_rect);
}

// Helper function to write to z-buffer
static void write_to_z_buffer(ZBufferCell *buffer, int screen_x, int screen_y, char character, uint8_t color) {
    if (screen_x >= 0 && screen_x < GAME_AREA_WIDTH && screen_y >= 0 && screen_y < GAME_AREA_HEIGHT) {
        int index = screen_y * GAME_AREA_WIDTH + screen_x;
        buffer[index].character = character;
        buffer[index].color = color;
        buffer[index].has_content = true;
    }
}



// Helper function to update viewport based on player position
static void update_viewport(World *world) {
    if (!world) return;
    
    // Get player position
    Position *player_pos = (Position *)entity_get_component(world->player, component_get_id("Position"));
    if (!player_pos) return;
    
    int player_x = (int)player_pos->x;
    int player_y = (int)player_pos->y;

    // Left
    if (player_x - g_viewport_x < VIEWPORT_MARGIN) {
        g_viewport_x -= CHUNK_X;
    }
    // Right
    if (player_x - g_viewport_x >= GAME_AREA_WIDTH - VIEWPORT_MARGIN) {
        g_viewport_x += CHUNK_X;
    }
    // Top
    if (player_y - g_viewport_y < VIEWPORT_MARGIN) {
        g_viewport_y -= CHUNK_Y;
    }
    // Bottom
    if (player_y - g_viewport_y >= GAME_AREA_HEIGHT - VIEWPORT_MARGIN) {
        g_viewport_y += CHUNK_Y;
    }

    // Clamp viewport to dungeon bounds
    if (g_viewport_x < 0) g_viewport_x = 0;
    if (g_viewport_y < 0) g_viewport_y = 0;
    if (g_viewport_x > DUNGEON_WIDTH - GAME_AREA_WIDTH) g_viewport_x = DUNGEON_WIDTH - GAME_AREA_WIDTH;
    if (g_viewport_y > DUNGEON_HEIGHT - GAME_AREA_HEIGHT) g_viewport_y = DUNGEON_HEIGHT - GAME_AREA_HEIGHT;
}

// Initialize z-buffer system
static bool init_z_buffers(void) {
    if (!g_renderer) return false;
    
    // Allocate z-buffer arrays for game area only
    size_t buffer_size = GAME_AREA_WIDTH * GAME_AREA_HEIGHT * sizeof(ZBufferCell);
    g_z_buffer_0 = malloc(buffer_size);
    g_z_buffer_1 = malloc(buffer_size);
    
    if (!g_z_buffer_0 || !g_z_buffer_1) {
        LOG_ERROR("Failed to allocate z-buffer arrays");
        return false;
    }
    
    // Initialize buffers
    memset(g_z_buffer_0, 0, buffer_size);
    memset(g_z_buffer_1, 0, buffer_size);
    
    return true;
}

// Clean up z-buffer system
static void cleanup_z_buffers(void) {
    if (g_z_buffer_0) {
        free(g_z_buffer_0);
        g_z_buffer_0 = NULL;
    }
    if (g_z_buffer_1) {
        free(g_z_buffer_1);
        g_z_buffer_1 = NULL;
    }
}

// Render the dungeon background to z-buffer 0
static void render_dungeon_background(World *world) {
    if (!world || !world->dungeon.width || !g_z_buffer_0) return;
    
    // Clear z-buffer 0
    memset(g_z_buffer_0, 0, GAME_AREA_WIDTH * GAME_AREA_HEIGHT * sizeof(ZBufferCell));
    
    for (int screen_y = 0; screen_y < GAME_AREA_HEIGHT; screen_y++) {
        for (int screen_x = 0; screen_x < GAME_AREA_WIDTH; screen_x++) {
            // Calculate dungeon coordinates
            int dungeon_x = g_viewport_x + screen_x;
            int dungeon_y = g_viewport_y + screen_y;
            
            // Check bounds
            if (dungeon_x >= 0 && dungeon_x < DUNGEON_WIDTH && 
                dungeon_y >= 0 && dungeon_y < DUNGEON_HEIGHT) {
                
                // Get visibility status from player's FOV
                uint8_t visibility = 0;
                CompactFieldOfView *player_fov = (CompactFieldOfView *)entity_get_component(world->player, component_get_id("FieldOfView"));
                if (player_fov) {
                    if (field_is_visible_compact(player_fov, dungeon_x, dungeon_y)) {
                        visibility = 1; // Currently visible
                    } else if (dungeon_is_explored(&world->dungeon, dungeon_x, dungeon_y)) {
                        visibility = 2; // Explored but not currently visible
                    } else {
                        visibility = 0; // Not visible and not explored
                    }
                }
                
                // Only render if visible or explored
                if (visibility > 0) {
                    // Get tile from dungeon
                    Tile *tile = dungeon_get_tile(&world->dungeon, dungeon_x, dungeon_y);
                    if (tile) {
                        TileInfo *info = dungeon_get_tile_info(tile->type);
                        if (info) {
                            // Adjust color based on visibility
                            uint8_t color = info->color;
                            if (visibility == 2) { // Explored but not visible
                                // Darken the color for explored areas
                                color = 0x08; // Dark gray
                            }
                            
                            // Write to z-buffer 0
                            write_to_z_buffer(g_z_buffer_0, screen_x, screen_y, info->symbol, color);
                        }
                    }
                }
            }
        }
    }
}

// Pre-update function to handle screen clearing and viewport updates
static void render_system_pre_update(World *world) {
    if (!g_renderer) {
        LOG_ERROR("Renderer not initialized");
        return;
    }
    
    // Update viewport based on player position
    update_viewport(world);
    
    // Calculate field of view from player position
    if (world) {
        Position *player_pos = (Position *)entity_get_component(world->player, component_get_id("Position"));
        CompactFieldOfView *player_fov = (CompactFieldOfView *)entity_get_component(world->player, component_get_id("FieldOfView"));
        if (player_pos && player_fov) {
            field_calculate_fov_compact(player_fov, &world->dungeon, (int)player_pos->x, (int)player_pos->y);
        }
    }
    
    // Clear z-buffer 1 (entity layer)
    if (g_z_buffer_1) {
        memset(g_z_buffer_1, 0, GAME_AREA_WIDTH * GAME_AREA_HEIGHT * sizeof(ZBufferCell));
    }
    
    // Render dungeon background to z-buffer 0
    render_dungeon_background(world);
}

// Post-update function to render from z-buffers and present the frame
static void render_system_post_update(World *world) {
    if (!g_renderer) return;
    
    // Clear main renderer
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);
    
    // Render player view sidebar
    playerview_render(g_renderer, world);
    
    // Render game area from z-buffers: check entity layer first, then background
    for (int screen_y = 0; screen_y < GAME_AREA_HEIGHT; screen_y++) {
        for (int screen_x = 0; screen_x < GAME_AREA_WIDTH; screen_x++) {
            int index = screen_y * GAME_AREA_WIDTH + screen_x;
            
            // Calculate actual screen position (offset by sidebar)
            int actual_screen_x = (screen_x + GAME_AREA_X_OFFSET) * CELL_SIZE;
            int actual_screen_y = (screen_y + GAME_AREA_Y_OFFSET) * CELL_SIZE;
            
            // Check entity layer first (z-buffer 1)
            if (g_z_buffer_1 && g_z_buffer_1[index].has_content) {
                render_tile_at_screen_pos_to_renderer(g_renderer, 
                                                     actual_screen_x, actual_screen_y,
                                                     g_z_buffer_1[index].character, 
                                                     g_z_buffer_1[index].color);
            }
            // Fall back to background layer (z-buffer 0)
            else if (g_z_buffer_0 && g_z_buffer_0[index].has_content) {
                render_tile_at_screen_pos_to_renderer(g_renderer, 
                                                     actual_screen_x, actual_screen_y,
                                                     g_z_buffer_0[index].character, 
                                                     g_z_buffer_0[index].color);
            }
        }
    }
    
    // Render status line LAST to ensure it's on top
    statusview_render(g_renderer, world);
    
    // Present the final frame
    SDL_RenderPresent(g_renderer);
}

int render_system_init(void) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 0;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        LOG_ERROR("SDL_ttf could not initialize! TTF_Error: %s", TTF_GetError());
        SDL_Quit();
        return 0;
    }
    
    // Create window
    g_window = SDL_CreateWindow("Adventure Game - ECS", 
                               SDL_WINDOWPOS_UNDEFINED, 
                               SDL_WINDOWPOS_UNDEFINED,
                               WINDOW_WIDTH_PX, 
                               WINDOW_HEIGHT_PX, 
                               SDL_WINDOW_SHOWN);
    if (g_window == NULL) {
        LOG_ERROR("Window could not be created! SDL_Error: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    
    // Create renderer
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (g_renderer == NULL) {
        LOG_ERROR("Renderer could not be created! SDL_Error: %s", SDL_GetError());
        SDL_DestroyWindow(g_window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    
    // Load a monospace font for consistent character rendering
    // Try multiple fixed-width fonts in order of preference
    const char* font_paths[] = {
        // macOS fonts
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Courier.ttc",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Andale Mono.ttf",
        
        // Linux fonts
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
        
        // Windows fonts (if running on Wine or similar)
        "/mnt/c/Windows/Fonts/consola.ttf",
        "/mnt/c/Windows/Fonts/cour.ttf",
        
        // Common fallbacks
        "/usr/share/fonts/truetype/misc/fixed-sys.ttf",
        NULL
    };
    
    g_font = NULL;
    for (int i = 0; font_paths[i] != NULL; i++) {
        g_font = TTF_OpenFont(font_paths[i], 16);
        if (g_font) {
            LOG_INFO("Loaded font: %s", font_paths[i]);
            break;
        }
    }
    
    if (!g_font) {
        LOG_WARN("Could not load any fixed-width font, falling back to rectangle rendering");
    }
    
    // Initialize z-buffer system
    if (!init_z_buffers()) {
        LOG_ERROR("Failed to initialize z-buffer system");
        if (g_font) {
            TTF_CloseFont(g_font);
            g_font = NULL;
        }
        SDL_DestroyRenderer(g_renderer);
        SDL_DestroyWindow(g_window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    
    g_initialized = true;
    LOG_INFO("Render system initialized with z-buffer support");
    return 1;
}

void render_system_cleanup(void) {
    // Clean up z-buffers first
    cleanup_z_buffers();
    
    if (g_font) {
        TTF_CloseFont(g_font);
        g_font = NULL;
    }
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    TTF_Quit();
    SDL_Quit();
    g_initialized = false;
}

SDL_Renderer* render_system_get_renderer(void) {
    return g_renderer;
}

void render_system_register(void) {
    // Register system that requires Position and BaseInfo components
    uint32_t component_mask = (1 << component_get_id("Position")) | (1 << component_get_id("BaseInfo"));
    system_register("Render System", component_mask, render_system, render_system_pre_update, render_system_post_update);
}

void render_system(Entity entity, World *world) {
    if (!g_renderer || !world || !g_z_buffer_1) {
        LOG_ERROR("Renderer, world, or z-buffer not initialized");
        return;
    }
    
    // Get component data
    Position *pos = (Position *)entity_get_component(entity, component_get_id("Position"));
    BaseInfo *base_info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
    
    if (!pos || !base_info) {
        LOG_ERROR("Missing position or base info component");
        return;
    }
    
    // Convert world coordinates to screen coordinates (relative to viewport)
    int screen_x = (int)(pos->x - g_viewport_x);
    int screen_y = (int)(pos->y - g_viewport_y);
    
    // Only process if entity is visible in game area viewport
    if (screen_x >= 0 && screen_x < GAME_AREA_WIDTH && 
        screen_y >= 0 && screen_y < GAME_AREA_HEIGHT) {
        
        // Check if entity is visible (only render if player can see it)
        int dungeon_x = (int)pos->x;
        int dungeon_y = (int)pos->y;
        
        bool entity_visible = false;
        CompactFieldOfView *player_fov = (CompactFieldOfView *)entity_get_component(world->player, component_get_id("FieldOfView"));
        if (player_fov) {
            entity_visible = field_is_visible_compact(player_fov, dungeon_x, dungeon_y);
        }
        
        // Only write to z-buffer if entity is visible
        if (entity_visible) {
            write_to_z_buffer(g_z_buffer_1, screen_x, screen_y, base_info->character, base_info->color);
        }
    }
} 
