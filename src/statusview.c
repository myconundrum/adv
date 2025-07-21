#include "statusview.h"
#include "render_system.h"
#include "components.h"
#include "ecs.h"
#include "log.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

static TTF_Font *g_status_font = NULL;

void statusview_init(void) {
    // Try to load a font for the status line (same as render system)
    const char* font_paths[] = {
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Courier.ttc",
        "/System/Library/Fonts/Menlo.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        NULL
    };
    
    g_status_font = NULL;
    for (int i = 0; font_paths[i] != NULL; i++) {
        g_status_font = TTF_OpenFont(font_paths[i], 14);
        if (g_status_font) {
            break;
        }
    }
    
    if (!g_status_font) {
        LOG_WARN("Could not load font for status line");
    }
}

void statusview_cleanup(void) {
    if (g_status_font) {
        TTF_CloseFont(g_status_font);
        g_status_font = NULL;
    }
}

// Helper function to render text at a specific position
static void render_text_at_position(SDL_Renderer *renderer, const char *text, int x, int y, SDL_Color color) {
    if (!g_status_font || !text) return;
    
    SDL_Surface *text_surface = TTF_RenderText_Solid(g_status_font, text, color);
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

void statusview_render(SDL_Renderer *renderer, World *world) {
    if (!renderer || !world) return;
    
    // Draw status line background
    int status_y = STATUS_LINE_Y_OFFSET * CELL_SIZE;
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255); // Darker gray background
    SDL_Rect status_rect = {0, status_y, WINDOW_WIDTH * CELL_SIZE, STATUS_LINE_HEIGHT * CELL_SIZE};
    SDL_RenderFillRect(renderer, &status_rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Light gray border
    SDL_RenderDrawRect(renderer, &status_rect);
    
    if (!g_status_font) return;
    
    // Get player components
    Position *player_pos = (Position *)entity_get_component(world->player, component_get_id("Position"));
    
    SDL_Color white = {255, 255, 255, 255};
    
    int y_offset = status_y + 4;
    int x_offset = 8;
    
    // Single line status content
    char status_line[256];
    
    if (player_pos) {
        snprintf(status_line, sizeof(status_line), 
                "Dungeon Level: 1  |  Position: (%d, %d)  |  Rooms: %d", 
                player_pos->x, player_pos->y, world->dungeon.room_count);
    } else {
        snprintf(status_line, sizeof(status_line), 
                "Dungeon Level: 1  |  Rooms: %d", world->dungeon.room_count);
    }
    render_text_at_position(renderer, status_line, x_offset, y_offset, white);
} 
