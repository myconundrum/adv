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
    int scroll_position; // Top line being displayed
    int lines_per_page;  // How many lines fit in the window
    int total_lines;     // Total number of wrapped lines
    
    // Scrollbar state
    bool scrollbar_dragging;
    int scrollbar_drag_offset;
    
    // Input handling
    bool initialized;
} MessageViewState;

// Global message view state
extern MessageViewState g_message_view;

// Initialization and cleanup
bool messageview_init(void);
void messageview_cleanup(void);

// Window management
void messageview_show(void);
void messageview_hide(void);
void messageview_toggle(void);
bool messageview_is_visible(void);
bool messageview_has_focus(void);

// Rendering
void messageview_render(SDL_Renderer *main_renderer);

// Event handling
bool messageview_handle_event(SDL_Event *event);

// Scrolling
void messageview_scroll_up(int lines);
void messageview_scroll_down(int lines);
void messageview_scroll_to_bottom(void);
void messageview_scroll_to_top(void);

// Internal helper functions
void messageview_update_layout(void);
void messageview_draw_scrollbar(void);
bool messageview_point_in_scrollbar(int x, int y);
int messageview_scrollbar_position_to_line(int y);

#endif 
