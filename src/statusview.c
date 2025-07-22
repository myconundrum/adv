#include "statusview.h"
#include "render_system.h"
#include "appstate.h"
#include "components.h"
#include "ecs.h"
#include "log.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

void statusview_init(void) {
    // Font is now managed by the render system
    LOG_INFO("Status view initialized");
}

void statusview_cleanup(void) {
    // Font is now managed by the render system
    LOG_INFO("Status view cleaned up");
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

void statusview_render(SDL_Renderer *renderer, AppState *app_state) {
    if (!renderer || !app_state) return;
    
    // Calculate status line position and dimensions
    int status_y = STATUS_LINE_Y_OFFSET * CELL_SIZE;
    int status_width = WINDOW_WIDTH * CELL_SIZE;
    int status_height = STATUS_LINE_HEIGHT * CELL_SIZE;
    
    // Draw status line background - ensure it covers the full width
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255); // Darker gray background
    SDL_Rect status_rect = {0, status_y, status_width, status_height};
    SDL_RenderFillRect(renderer, &status_rect);
    
    // Draw top border line
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Light gray border
    SDL_Rect border_rect = {0, status_y, status_width, 1};
    SDL_RenderFillRect(renderer, &border_rect);
    
    TTF_Font *font = render_system_get_small_font(app_state);
    if (!font) {
        LOG_WARN("Could not get font for status line");
        return;
    }
    
    // Get player components
    Position *player_pos = (Position *)entity_get_component(app_state, app_state->player, component_get_id(app_state, "Position"));
    
    SDL_Color white = {255, 255, 255, 255};
    
    int y_offset = status_y + 2; // Small padding from top
    int x_offset = 8;
    
    // Single line status content
    char status_line[256];
    
    if (player_pos) {
        snprintf(status_line, sizeof(status_line), 
                "Dungeon Level: 1  |  Position: (%d, %d)  |  Rooms: %d", 
                player_pos->x, player_pos->y, app_state->dungeon.room_count);
    } else {
        snprintf(status_line, sizeof(status_line), 
                "Dungeon Level: 1  |  Rooms: %d", app_state->dungeon.room_count);
    }
    render_text_at_position(renderer, font, status_line, x_offset, y_offset, white);
} 
