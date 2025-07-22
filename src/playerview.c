#include "playerview.h"
#include "render_system.h"
#include "appstate.h"
#include "components.h"
#include "ecs.h"
#include "log.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

void playerview_init(void) {
    // Font is now managed by the render system
    LOG_INFO("Player view initialized");
}

void playerview_cleanup(void) {
    // Font is now managed by the render system
    LOG_INFO("Player view cleaned up");
}

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

void playerview_render(SDL_Renderer *renderer, AppState *app_state) {
    if (!renderer || !app_state) return;
    
    // Draw sidebar background
    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255); // Dark gray background
    SDL_Rect sidebar_rect = {0, 0, SIDEBAR_WIDTH * CELL_SIZE, GAME_AREA_HEIGHT * CELL_SIZE};
    SDL_RenderFillRect(renderer, &sidebar_rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Light gray border
    SDL_RenderDrawRect(renderer, &sidebar_rect);
    
    TTF_Font *font = render_system_get_small_font(app_state);
    if (!font) {
        LOG_WARN("Could not get small font for player view");
        return;
    }
    
    // Get player components
    BaseInfo *player_info = (BaseInfo *)entity_get_component(app_state, app_state->player, component_get_id(app_state, "BaseInfo"));
    Actor *player_actor = (Actor *)entity_get_component(app_state, app_state->player, component_get_id(app_state, "Actor"));
    
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};
    
    int y_offset = 8; // Reduced top padding
    int line_height = 14; // Reduced line height for compactness
    int x_offset = 6; // Reduced left padding
    
    // Player name and symbol (more compact)
    if (player_info) {
        char name_line[16];
        snprintf(name_line, sizeof(name_line), "%c %s", player_info->character, player_info->name);
        render_text_at_position(renderer, font, name_line, x_offset, y_offset, white);
        y_offset += line_height + 4; // Small gap after name
    }
    
    // Player stats (compact format)
    if (player_actor) {
        char stats_line[16];
        
        // HP
        SDL_Color hp_color = player_actor->hp > 70 ? green : (player_actor->hp > 30 ? yellow : red);
        snprintf(stats_line, sizeof(stats_line), "HP:%d", player_actor->hp);
        render_text_at_position(renderer, font, stats_line, x_offset, y_offset, hp_color);
        y_offset += line_height;
        
        // Energy
        snprintf(stats_line, sizeof(stats_line), "En:%d", player_actor->energy);
        render_text_at_position(renderer, font, stats_line, x_offset, y_offset, white);
        y_offset += line_height;
        
        // Strength
        snprintf(stats_line, sizeof(stats_line), "St:%d", player_actor->strength);
        render_text_at_position(renderer, font, stats_line, x_offset, y_offset, white);
        y_offset += line_height;
        
        // Attack/Defense
        snprintf(stats_line, sizeof(stats_line), "At:%d", player_actor->attack);
        render_text_at_position(renderer, font, stats_line, x_offset, y_offset, white);
        y_offset += line_height;
        
        snprintf(stats_line, sizeof(stats_line), "Df:%d", player_actor->defense);
        render_text_at_position(renderer, font, stats_line, x_offset, y_offset, white);
    }
}
