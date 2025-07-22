#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

// Message window configuration
#define MESSAGE_WINDOW_DEFAULT_WIDTH 400
#define MESSAGE_WINDOW_DEFAULT_HEIGHT 300
#define MESSAGE_WINDOW_MIN_WIDTH 200
#define MESSAGE_WINDOW_MIN_HEIGHT 150
#define MESSAGE_WINDOW_TITLE "Messages"

// Scrolling and display constants
#define MESSAGE_LINE_HEIGHT 18
#define MESSAGE_MARGIN 10
#define SCROLLBAR_WIDTH 20

// Message view state
typedef struct {
    // SDL objects
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    
    // Window state
    bool is_visible;
    bool has_focus;
    int window_width;
    int window_height;
    
    // Scrolling state
    int scroll_position;
    int lines_per_page;
    int total_lines;
    
    // UI interaction state
    bool scrollbar_dragging;
    int scrollbar_drag_offset;
    
    // Initialization flag
    bool initialized;
} MessageViewState;

// Forward declaration
struct AppState;

// Core functions 
bool messageview_init(struct AppState *app_state);
void messageview_cleanup(struct AppState *app_state);

// Window management
void messageview_show(struct AppState *app_state);
void messageview_hide(struct AppState *app_state);
void messageview_toggle(struct AppState *app_state);
bool messageview_is_visible(struct AppState *app_state);
bool messageview_has_focus(struct AppState *app_state);

// Rendering
void messageview_render(SDL_Renderer *main_renderer, struct AppState *app_state);

// Event handling
bool messageview_handle_event(SDL_Event *event, struct AppState *app_state);

// Scrolling
void messageview_scroll_up(int lines, struct AppState *app_state);
void messageview_scroll_down(int lines, struct AppState *app_state);
void messageview_scroll_to_bottom(struct AppState *app_state);
void messageview_scroll_to_top(struct AppState *app_state);

// Internal helper functions
void messageview_update_layout(struct AppState *app_state);
void messageview_draw_scrollbar(struct AppState *app_state);
bool messageview_point_in_scrollbar(int x, int y, struct AppState *app_state);
int messageview_scrollbar_position_to_line(int y, struct AppState *app_state);

#endif 
