#include "playerview.h"
#include "render_system.h"
#include "components.h"
#include "ecs.h"
#include "log.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

static TTF_Font *g_sidebar_font = NULL;

void playerview_init(void) {
    // Try to load a font for the sidebar (same as render system)
    const char* font_paths[] = {
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Courier.ttc",
        "/System/Library/Fonts/Menlo.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        NULL
    };
    
    g_sidebar_font = NULL;
    for (int i = 0; font_paths[i] != NULL; i++) {
        g_sidebar_font = TTF_OpenFont(font_paths[i], 14);
        if (g_sidebar_font) {
            break;
        }
    }
    
    if (!g_sidebar_font) {
        LOG_WARN("Could not load font for sidebar");
    }
}

void playerview_cleanup(void) {
    if (g_sidebar_font) {
        TTF_CloseFont(g_sidebar_font);
        g_sidebar_font = NULL;
    }
}

// Helper function to render text at a specific position
static void render_text_at_position(SDL_Renderer *renderer, const char *text, int x, int y, SDL_Color color) {
    if (!g_sidebar_font || !text) return;
    
    SDL_Surface *text_surface = TTF_RenderText_Solid(g_sidebar_font, text, color);
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

void playerview_render(SDL_Renderer *renderer, World *world) {
    if (!renderer || !world) return;
    
    // Draw sidebar background
    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255); // Dark gray background
    SDL_Rect sidebar_rect = {0, 0, SIDEBAR_WIDTH * CELL_SIZE, GAME_AREA_HEIGHT * CELL_SIZE};
    SDL_RenderFillRect(renderer, &sidebar_rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Light gray border
    SDL_RenderDrawRect(renderer, &sidebar_rect);
    
    if (!g_sidebar_font) return;
    
    // Get player components
    BaseInfo *player_info = (BaseInfo *)entity_get_component(world->player, component_get_id("BaseInfo"));
    Actor *player_actor = (Actor *)entity_get_component(world->player, component_get_id("Actor"));
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};
    
    int y_offset = 10;
    int line_height = 16;
    int x_offset = 8;
    
    // Player name and symbol
    if (player_info) {
        char name_line[32];
        snprintf(name_line, sizeof(name_line), "%c %s", player_info->character, player_info->name);
        render_text_at_position(renderer, name_line, x_offset, y_offset, white);
        y_offset += line_height * 2;
    }
    
    // Player stats
    if (player_actor) {
        char stats_line[32];
        
        // HP
        SDL_Color hp_color = player_actor->hp > 70 ? green : (player_actor->hp > 30 ? yellow : red);
        snprintf(stats_line, sizeof(stats_line), "HP: %d", player_actor->hp);
        render_text_at_position(renderer, stats_line, x_offset, y_offset, hp_color);
        y_offset += line_height;
        
        // Energy
        snprintf(stats_line, sizeof(stats_line), "Energy: %d", player_actor->energy);
        render_text_at_position(renderer, stats_line, x_offset, y_offset, white);
        y_offset += line_height;
        
        // Strength
        snprintf(stats_line, sizeof(stats_line), "Str: %d", player_actor->strength);
        render_text_at_position(renderer, stats_line, x_offset, y_offset, white);
        y_offset += line_height;
        
        // Attack/Defense
        snprintf(stats_line, sizeof(stats_line), "Att: %d", player_actor->attack);
        render_text_at_position(renderer, stats_line, x_offset, y_offset, white);
        y_offset += line_height;
        
        snprintf(stats_line, sizeof(stats_line), "Def: %d", player_actor->defense);
        render_text_at_position(renderer, stats_line, x_offset, y_offset, white);
    }
}
