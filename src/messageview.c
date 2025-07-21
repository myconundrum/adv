#include "messageview.h"
#include "messages.h"
#include "render_system.h"
#include "log.h"
#include <string.h>

// Global message view state
MessageViewState g_message_view = {0};

bool messageview_init(void) {
    memset(&g_message_view, 0, sizeof(MessageViewState));
    
    // Don't create the window immediately - wait for show()
    g_message_view.window_width = MESSAGE_WINDOW_DEFAULT_WIDTH;
    g_message_view.window_height = MESSAGE_WINDOW_DEFAULT_HEIGHT;
    g_message_view.is_visible = false;
    g_message_view.has_focus = false;
    g_message_view.scroll_position = 0;
    g_message_view.scrollbar_dragging = false;
    g_message_view.initialized = true;
    
    LOG_INFO("Message view initialized");
    return true;
}

void messageview_cleanup(void) {
    if (g_message_view.font) {
        TTF_CloseFont(g_message_view.font);
        g_message_view.font = NULL;
    }
    
    if (g_message_view.renderer) {
        SDL_DestroyRenderer(g_message_view.renderer);
        g_message_view.renderer = NULL;
    }
    
    if (g_message_view.window) {
        SDL_DestroyWindow(g_message_view.window);
        g_message_view.window = NULL;
    }
    
    memset(&g_message_view, 0, sizeof(MessageViewState));
    LOG_INFO("Message view cleanup complete");
}

static bool messageview_create_window(void) {
    if (g_message_view.window) {
        return true; // Already created
    }
    
    // Position the message window to the right of the main window
    int main_window_x, main_window_y;
    SDL_Window *main_window = SDL_GL_GetCurrentWindow();
    if (!main_window) {
        // Fallback: try to get main window from render system
        SDL_Renderer *main_renderer = render_system_get_renderer();
        if (main_renderer) {
            main_window = SDL_RenderGetWindow(main_renderer);
        }
    }
    
    if (main_window) {
        SDL_GetWindowPosition(main_window, &main_window_x, &main_window_y);
        int main_window_w, main_window_h;
        SDL_GetWindowSize(main_window, &main_window_w, &main_window_h);
        
        // Position message window to the right of main window
        main_window_x += main_window_w + 10;
    } else {
        // Fallback position
        main_window_x = 100;
        main_window_y = 100;
    }
    
    // Create message window
    g_message_view.window = SDL_CreateWindow(
        MESSAGE_WINDOW_TITLE,
        main_window_x, main_window_y,
        g_message_view.window_width, g_message_view.window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!g_message_view.window) {
        LOG_ERROR("Failed to create message window: %s", SDL_GetError());
        return false;
    }
    
    // Set minimum window size
    SDL_SetWindowMinimumSize(g_message_view.window, 
                            MESSAGE_WINDOW_MIN_WIDTH, 
                            MESSAGE_WINDOW_MIN_HEIGHT);
    
    // Create renderer
    g_message_view.renderer = SDL_CreateRenderer(g_message_view.window, -1, 
                                                SDL_RENDERER_ACCELERATED);
    if (!g_message_view.renderer) {
        LOG_ERROR("Failed to create message window renderer: %s", SDL_GetError());
        SDL_DestroyWindow(g_message_view.window);
        g_message_view.window = NULL;
        return false;
    }
    
    // Load font - same paths as main render system
    const char* font_paths[] = {
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Courier.ttc",
        "/System/Library/Fonts/Menlo.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        NULL
    };
    
    for (int i = 0; font_paths[i] != NULL; i++) {
        g_message_view.font = TTF_OpenFont(font_paths[i], 14);
        if (g_message_view.font) {
            LOG_INFO("Loaded message window font: %s", font_paths[i]);
            break;
        }
    }
    
    if (!g_message_view.font) {
        LOG_WARN("Could not load font for message window");
    }
    
    // Update layout calculations
    messageview_update_layout();
    
    return true;
}

void messageview_show(void) {
    if (!g_message_view.initialized) {
        LOG_WARN("Message view not initialized");
        return;
    }
    
    if (!messageview_create_window()) {
        LOG_ERROR("Failed to create message window");
        return;
    }
    
    SDL_ShowWindow(g_message_view.window);
    g_message_view.is_visible = true;
    
    // Auto-scroll to bottom when showing
    messageview_scroll_to_bottom();
    
    LOG_INFO("Message window shown");
}

void messageview_hide(void) {
    if (g_message_view.window) {
        SDL_HideWindow(g_message_view.window);
    }
    g_message_view.is_visible = false;
    g_message_view.has_focus = false;
    
    LOG_INFO("Message window hidden");
}

void messageview_toggle(void) {
    if (g_message_view.is_visible) {
        messageview_hide();
    } else {
        messageview_show();
    }
}

bool messageview_is_visible(void) {
    return g_message_view.is_visible;
}

bool messageview_has_focus(void) {
    return g_message_view.has_focus;
}

void messageview_update_layout(void) {
    if (!g_message_view.window) return;
    
    // Get current window size
    SDL_GetWindowSize(g_message_view.window, 
                     &g_message_view.window_width, 
                     &g_message_view.window_height);
    
    // Calculate how many lines fit in the window
    int content_height = g_message_view.window_height - (2 * MESSAGE_MARGIN);
    g_message_view.lines_per_page = content_height / MESSAGE_LINE_HEIGHT;
    
    // Update text wrapping for new window width
    messages_rewrap_text(g_message_view.window_width);
    g_message_view.total_lines = messages_get_wrapped_line_count();
    
    // Clamp scroll position
    int max_scroll = g_message_view.total_lines - g_message_view.lines_per_page;
    if (max_scroll < 0) max_scroll = 0;
    if (g_message_view.scroll_position > max_scroll) {
        g_message_view.scroll_position = max_scroll;
    }
}

static void render_text_line(const char *text, int x, int y, SDL_Color color) {
    if (!g_message_view.font || !text || strlen(text) == 0) {
        return;
    }
    
    SDL_Surface *text_surface = TTF_RenderText_Solid(g_message_view.font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(g_message_view.renderer, text_surface);
    SDL_FreeSurface(text_surface);
    
    if (text_texture) {
        int text_w, text_h;
        SDL_QueryTexture(text_texture, NULL, NULL, &text_w, &text_h);
        
        SDL_Rect dest_rect = {x, y, text_w, text_h};
        SDL_RenderCopy(g_message_view.renderer, text_texture, NULL, &dest_rect);
        SDL_DestroyTexture(text_texture);
    }
}

void messageview_draw_scrollbar(void) {
    if (g_message_view.total_lines <= g_message_view.lines_per_page) {
        return; // No scrollbar needed
    }
    
    // Scrollbar background
    int scrollbar_x = g_message_view.window_width - SCROLLBAR_WIDTH;
    int scrollbar_y = 0;
    int scrollbar_height = g_message_view.window_height;
    
    SDL_SetRenderDrawColor(g_message_view.renderer, 100, 100, 100, 255);
    SDL_Rect scrollbar_bg = {scrollbar_x, scrollbar_y, SCROLLBAR_WIDTH, scrollbar_height};
    SDL_RenderFillRect(g_message_view.renderer, &scrollbar_bg);
    
    // Scrollbar thumb
    float thumb_ratio = (float)g_message_view.lines_per_page / g_message_view.total_lines;
    int thumb_height = (int)(scrollbar_height * thumb_ratio);
    if (thumb_height < 20) thumb_height = 20; // Minimum thumb size
    
    float scroll_ratio = (float)g_message_view.scroll_position / 
                        (g_message_view.total_lines - g_message_view.lines_per_page);
    int thumb_y = (int)((scrollbar_height - thumb_height) * scroll_ratio);
    
    SDL_SetRenderDrawColor(g_message_view.renderer, 180, 180, 180, 255);
    SDL_Rect thumb_rect = {scrollbar_x + 2, thumb_y, SCROLLBAR_WIDTH - 4, thumb_height};
    SDL_RenderFillRect(g_message_view.renderer, &thumb_rect);
}

void messageview_render(SDL_Renderer *main_renderer, World *world) {
    (void)main_renderer; // Unused - we render to our own window
    (void)world; // Unused for now
    
    if (!g_message_view.is_visible || !g_message_view.window || !g_message_view.renderer) {
        return;
    }
    
    // Update layout in case window was resized
    messageview_update_layout();
    
    // Clear window
    SDL_SetRenderDrawColor(g_message_view.renderer, 30, 30, 30, 255);
    SDL_RenderClear(g_message_view.renderer);
    
    // Render messages
    SDL_Color text_color = {255, 255, 255, 255}; // White text
    
    int y = MESSAGE_MARGIN;
    int content_width = g_message_view.window_width - MESSAGE_MARGIN - SCROLLBAR_WIDTH;
    
    for (int i = 0; i < g_message_view.lines_per_page; i++) {
        int line_index = g_message_view.scroll_position + i;
        
        if (line_index >= g_message_view.total_lines) {
            break;
        }
        
        const char *line_text = messages_get_wrapped_line(line_index);
        if (line_text) {
            // Check if this is the start of a new message (for visual separation)
            if (line_index > 0) {
                int prev_msg_idx = messages_get_message_index_for_line(line_index - 1);
                int curr_msg_idx = messages_get_message_index_for_line(line_index);
                if (prev_msg_idx != curr_msg_idx) {
                    // Draw a subtle separator line
                    SDL_SetRenderDrawColor(g_message_view.renderer, 60, 60, 60, 255);
                    SDL_RenderDrawLine(g_message_view.renderer, 
                                     MESSAGE_MARGIN, y - 2, 
                                     content_width, y - 2);
                }
            }
            
            render_text_line(line_text, MESSAGE_MARGIN, y, text_color);
        }
        
        y += MESSAGE_LINE_HEIGHT;
    }
    
    // Draw scrollbar
    messageview_draw_scrollbar();
    
    // Present the frame
    SDL_RenderPresent(g_message_view.renderer);
}

bool messageview_point_in_scrollbar(int x, int y) {
    if (!g_message_view.window) return false;
    
    int scrollbar_x = g_message_view.window_width - SCROLLBAR_WIDTH;
    return (x >= scrollbar_x && x < g_message_view.window_width && 
            y >= 0 && y < g_message_view.window_height);
}

int messageview_scrollbar_position_to_line(int y) {
    if (g_message_view.total_lines <= g_message_view.lines_per_page) {
        return 0;
    }
    
    float ratio = (float)y / g_message_view.window_height;
    int line = (int)(ratio * (g_message_view.total_lines - g_message_view.lines_per_page));
    
    // Clamp to valid range
    if (line < 0) line = 0;
    int max_scroll = g_message_view.total_lines - g_message_view.lines_per_page;
    if (line > max_scroll) line = max_scroll;
    
    return line;
}

bool messageview_handle_event(SDL_Event *event) {
    if (!g_message_view.is_visible || !g_message_view.window) {
        return false;
    }
    
    // Check if this event is for our window
    if (event->type == SDL_WINDOWEVENT && 
        event->window.windowID == SDL_GetWindowID(g_message_view.window)) {
        
        switch (event->window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                g_message_view.has_focus = true;
                return true;
                
            case SDL_WINDOWEVENT_FOCUS_LOST:
                g_message_view.has_focus = false;
                g_message_view.scrollbar_dragging = false;
                return true;
                
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                messageview_update_layout();
                return true;
                
            case SDL_WINDOWEVENT_CLOSE:
                messageview_hide();
                return true;
        }
    }
    
    // Handle mouse events if window has focus or we're dragging
    if ((g_message_view.has_focus || g_message_view.scrollbar_dragging) && 
        (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP || 
         event->type == SDL_MOUSEMOTION || event->type == SDL_MOUSEWHEEL)) {
        
        if (event->type == SDL_MOUSEWHEEL) {
            // Mouse wheel scrolling
            if (event->wheel.y > 0) {
                messageview_scroll_up(3);
            } else if (event->wheel.y < 0) {
                messageview_scroll_down(3);
            }
            return true;
        }
        
        if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
            int mouse_x, mouse_y;
            SDL_GetMouseState(&mouse_x, &mouse_y);
            
            // Convert to window-relative coordinates
            int window_x, window_y;
            SDL_GetWindowPosition(g_message_view.window, &window_x, &window_y);
            mouse_x -= window_x;
            mouse_y -= window_y;
            
            if (messageview_point_in_scrollbar(mouse_x, mouse_y)) {
                g_message_view.scrollbar_dragging = true;
                g_message_view.scrollbar_drag_offset = mouse_y;
                g_message_view.scroll_position = messageview_scrollbar_position_to_line(mouse_y);
                return true;
            }
        }
        
        if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT) {
            g_message_view.scrollbar_dragging = false;
            return true;
        }
        
        if (event->type == SDL_MOUSEMOTION && g_message_view.scrollbar_dragging) {
            int mouse_x, mouse_y;
            SDL_GetMouseState(&mouse_x, &mouse_y);
            
            // Convert to window-relative coordinates
            int window_x, window_y;
            SDL_GetWindowPosition(g_message_view.window, &window_x, &window_y);
            mouse_y -= window_y;
            
            g_message_view.scroll_position = messageview_scrollbar_position_to_line(mouse_y);
            return true;
        }
    }
    
    // Handle keyboard events if window has focus
    if (g_message_view.has_focus && event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_UP:
                messageview_scroll_up(1);
                return true;
                
            case SDLK_DOWN:
                messageview_scroll_down(1);
                return true;
                
            case SDLK_PAGEUP:
                messageview_scroll_up(g_message_view.lines_per_page - 1);
                return true;
                
            case SDLK_PAGEDOWN:
                messageview_scroll_down(g_message_view.lines_per_page - 1);
                return true;
                
            case SDLK_HOME:
                messageview_scroll_to_top();
                return true;
                
            case SDLK_END:
                messageview_scroll_to_bottom();
                return true;
        }
    }
    
    return false;
}

void messageview_scroll_up(int lines) {
    g_message_view.scroll_position -= lines;
    if (g_message_view.scroll_position < 0) {
        g_message_view.scroll_position = 0;
    }
}

void messageview_scroll_down(int lines) {
    g_message_view.scroll_position += lines;
    int max_scroll = g_message_view.total_lines - g_message_view.lines_per_page;
    if (max_scroll < 0) max_scroll = 0;
    if (g_message_view.scroll_position > max_scroll) {
        g_message_view.scroll_position = max_scroll;
    }
}

void messageview_scroll_to_bottom(void) {
    int max_scroll = g_message_view.total_lines - g_message_view.lines_per_page;
    if (max_scroll < 0) max_scroll = 0;
    g_message_view.scroll_position = max_scroll;
}

void messageview_scroll_to_top(void) {
    g_message_view.scroll_position = 0;
} 
