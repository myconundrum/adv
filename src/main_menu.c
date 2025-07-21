#include "main_menu.h"
#include "log.h"
#include "render_system.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>

static TTF_Font *g_menu_font = NULL;

// Helper function to render text at a specific position
static void render_text_at_position(SDL_Renderer *renderer, const char *text, int x, int y, SDL_Color color) {
    if (!g_menu_font || !text) return;
    
    SDL_Surface *text_surface = TTF_RenderText_Solid(g_menu_font, text, color);
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

// Main menu initialization
void main_menu_init(MainMenu *menu) {
    if (!menu) return;
    
    memset(menu, 0, sizeof(MainMenu));
    menu->selected_option = MENU_OPTION_NEW_GAME;
    menu->option_selected = false;
    
    // Load font if not already loaded
    if (!g_menu_font) {
        const char* font_paths[] = {
            "/System/Library/Fonts/Monaco.ttf",
            "/System/Library/Fonts/Courier.ttc",
            "/System/Library/Fonts/Menlo.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
            NULL
        };
        
        for (int i = 0; font_paths[i] != NULL; i++) {
            g_menu_font = TTF_OpenFont(font_paths[i], 18);
            if (g_menu_font) {
                break;
            }
        }
        
        if (!g_menu_font) {
            LOG_WARN("Could not load font for main menu");
        }
    }
}

void main_menu_cleanup(MainMenu *menu) {
    (void)menu;
    
    if (g_menu_font) {
        TTF_CloseFont(g_menu_font);
        g_menu_font = NULL;
    }
}

// Input handling for main menu
void main_menu_handle_input(MainMenu *menu, int key) {
    if (!menu) return;
    
    switch (key) {
        case SDLK_UP:
            if (menu->selected_option > 0) {
                menu->selected_option--;
            }
            break;
        case SDLK_DOWN:
            if (menu->selected_option < MENU_OPTION_COUNT - 1) {
                menu->selected_option++;
            }
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
            menu->option_selected = true;
            LOG_INFO("Main menu option selected: %d", menu->selected_option);
            break;
        case SDLK_1:
            menu->selected_option = MENU_OPTION_NEW_GAME;
            menu->option_selected = true;
            break;
        case SDLK_2:
            menu->selected_option = MENU_OPTION_LOAD_GAME;
            menu->option_selected = true;
            break;
        case SDLK_3:
        case SDLK_q:
            menu->selected_option = MENU_OPTION_QUIT;
            menu->option_selected = true;
            break;
    }
}

// Main menu rendering
void main_menu_render(MainMenu *menu) {
    if (!menu) return;
    
    // Get renderer from render system
    SDL_Renderer *renderer = render_system_get_renderer();
    if (!renderer) return;
    
    // Clear screen with dark blue background
    SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
    SDL_RenderClear(renderer);
    
    // Colors
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color gray = {128, 128, 128, 255};
    
    // Get screen dimensions
    int screen_width = WINDOW_WIDTH * CELL_SIZE;
    int screen_height = WINDOW_HEIGHT * CELL_SIZE;
    
    // Title
    int title_y = screen_height / 4;
    render_text_at_position(renderer, "ADVENTURE GAME", screen_width / 2 - 100, title_y, white);
    render_text_at_position(renderer, "Basic Fantasy RPG", screen_width / 2 - 80, title_y + 30, gray);
    
    // Menu options
    const char* menu_texts[] = {
        "1. New Game",
        "2. Load Game", 
        "3. Quit"
    };
    
    int menu_start_y = screen_height / 2;
    int line_height = 40;
    
    for (int i = 0; i < (int)MENU_OPTION_COUNT; i++) {
        SDL_Color color = (i == (int)menu->selected_option) ? yellow : white;
        int y_pos = menu_start_y + i * line_height;
        
        // Add selection indicator
        if (i == (int)menu->selected_option) {
            render_text_at_position(renderer, ">", screen_width / 2 - 120, y_pos, yellow);
        }
        
        render_text_at_position(renderer, menu_texts[i], screen_width / 2 - 100, y_pos, color);
    }
    
    // Instructions
    render_text_at_position(renderer, "Use arrow keys to navigate, Enter to select", 
                           screen_width / 2 - 200, screen_height - 100, gray);
    
    SDL_RenderPresent(renderer);
}

// Get selected option
MenuOption main_menu_get_selection(const MainMenu *menu) {
    if (!menu) return MENU_OPTION_NEW_GAME;
    return menu->selected_option;
}

bool main_menu_has_selection(const MainMenu *menu) {
    if (!menu) return false;
    return menu->option_selected;
}
