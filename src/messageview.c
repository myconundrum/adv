#include "messageview.h"
#include "messages.h"
#include "render_system.h"
#include "appstate.h"
#include "log.h"
#include <string.h>

bool messageview_init(struct AppState *app_state) {
    memset(&app_state->message_view, 0, sizeof(message_view));
    
    // Don't create the window immediately - wait for show()
    app_state->message_view.window_width = MESSAGE_WINDOW_DEFAULT_WIDTH;
    app_state->message_view.window_height = MESSAGE_WINDOW_DEFAULT_HEIGHT;
    app_state->message_view.is_visible = false;
    app_state->message_view.has_focus = false;
    app_state->message_view.scroll_position = 0;
    app_state->message_view.scrollbar_dragging = false;
    app_state->message_view.initialized = true;
    
    LOG_INFO("Message view initialized");
    return true;
}

void messageview_cleanup(struct AppState *app_state) {
    if (app_state->message_view.font) {
        TTF_CloseFont(app_state->message_view.font);
        app_state->message_view.font = NULL;
    }
    
    if (app_state->message_view.renderer) {
        SDL_DestroyRenderer(app_state->message_view.renderer);
        app_state->message_view.renderer = NULL;
    }
    
    if (app_state->message_view.window) {
        SDL_DestroyWindow(app_state->message_view.window);
        app_state->message_view.window = NULL;
    }
    
    memset(&app_state->message_view, 0, sizeof(message_view));
    LOG_INFO("Message view cleanup complete");
}

static bool messageview_create_window(struct AppState *app_state) {
    if (app_state->message_view.window) {
        return true; // Already created
    }

    // Create message window as child of main window
    SDL_Window *main_window = app_state->render.window;
    if (!main_window) {
        LOG_ERROR("Cannot create message window: main window not available");
        return false;
    }

    // Get main window position
    int main_x, main_y;
    SDL_GetWindowPosition(main_window, &main_x, &main_y);
    
    // Position message window to the right of main window
    int main_width;
    SDL_GetWindowSize(main_window, &main_width, NULL);
    
    app_state->message_view.window = SDL_CreateWindow(
        MESSAGE_WINDOW_TITLE,
        main_x + main_width + 10, main_y + 50,
        app_state->message_view.window_width, app_state->message_view.window_height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN
    );
    
    if (!app_state->message_view.window) {
        LOG_ERROR("Failed to create message window: %s", SDL_GetError());
        return false;
    }
    
    // Set minimum window size
    SDL_SetWindowMinimumSize(app_state->message_view.window,
                           MESSAGE_WINDOW_MIN_WIDTH,
                           MESSAGE_WINDOW_MIN_HEIGHT);
    
    // Create renderer for the message window
    app_state->message_view.renderer = SDL_CreateRenderer(app_state->message_view.window, -1,
                                               SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app_state->message_view.renderer) {
        LOG_ERROR("Failed to create message window renderer: %s", SDL_GetError());
        SDL_DestroyWindow(app_state->message_view.window);
        app_state->message_view.window = NULL;
        return false;
    }
    
    // Load font for message rendering
    // Try multiple font paths
    const char *font_paths[] = {
        "/System/Library/Fonts/Monaco.ttf",     // macOS
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", // Linux
        "C:\\Windows\\Fonts\\consola.ttf"       // Windows
    };
    
    for (int i = 0; i < 3; i++) {
        app_state->message_view.font = TTF_OpenFont(font_paths[i], 14);
        if (app_state->message_view.font) {
            LOG_INFO("Loaded message font: %s", font_paths[i]);
            break;
        }
    }
    
    if (!app_state->message_view.font) {
        LOG_ERROR("Failed to load any font for message window");
        // Continue without font - will just not render text
    }
    
    LOG_INFO("Message window created successfully");
    return true;
}

void messageview_show(struct AppState *app_state) {
    if (!app_state->message_view.initialized) {
        LOG_WARN("Message view not initialized");
        return;
    }
    
    if (!messageview_create_window(app_state)) {
        LOG_ERROR("Failed to create message window");
        return;
    }
    
    SDL_ShowWindow(app_state->message_view.window);
    app_state->message_view.is_visible = true;
    
    // Scroll to bottom when showing
    messageview_scroll_to_bottom(app_state);
    
    LOG_INFO("Message window shown");
}

void messageview_hide(struct AppState *app_state) {
    if (app_state->message_view.window) {
        SDL_HideWindow(app_state->message_view.window);
    }
    app_state->message_view.is_visible = false;
    app_state->message_view.has_focus = false;
    
    LOG_INFO("Message window hidden");
}

void messageview_toggle(struct AppState *app_state) {
    if (messageview_is_visible(app_state)) {
        messageview_hide(app_state);
    } else {
        messageview_show(app_state);
    }
}

bool messageview_is_visible(struct AppState *app_state) {
    return app_state->message_view.is_visible;
}

bool messageview_has_focus(struct AppState *app_state) {
    return app_state->message_view.has_focus;
}

void messageview_update_layout(struct AppState *app_state) {
    if (!app_state->message_view.window) return;
    
    // If app_state is not available, skip text rewrapping
    if (app_state == NULL) {
        LOG_WARN("messageview_update_layout called with NULL app_state");
        return;
    }
    
    // Update window dimensions
    SDL_GetWindowSize(app_state->message_view.window,
                     &app_state->message_view.window_width,
                     &app_state->message_view.window_height);
    
    // Calculate how many lines fit in the window
    int content_height = app_state->message_view.window_height - (2 * MESSAGE_MARGIN);
    app_state->message_view.lines_per_page = content_height / MESSAGE_LINE_HEIGHT;
    
    // Re-wrap text and get total line count
    if (app_state) {
        messages_rewrap_text(app_state, app_state->message_view.window_width);
        app_state->message_view.total_lines = messages_get_wrapped_line_count(app_state);
    }
    
    // Clamp scroll position to valid range
    int max_scroll = app_state->message_view.total_lines - app_state->message_view.lines_per_page;
    if (max_scroll < 0) max_scroll = 0;
    if (app_state->message_view.scroll_position > max_scroll) {
        app_state->message_view.scroll_position = max_scroll;
    }
}

static void render_text_line(const char *text, int x, int y, SDL_Color color, struct AppState *app_state) {
    if (!app_state->message_view.font || !text || strlen(text) == 0) {
        return;
    }
    
    SDL_Surface *text_surface = TTF_RenderText_Solid(app_state->message_view.font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(app_state->message_view.renderer, text_surface);
    SDL_FreeSurface(text_surface);
    
    if (text_texture) {
        SDL_Rect dest_rect = {x, y, 0, 0};
        SDL_QueryTexture(text_texture, NULL, NULL, &dest_rect.w, &dest_rect.h);
        SDL_RenderCopy(app_state->message_view.renderer, text_texture, NULL, &dest_rect);
        SDL_DestroyTexture(text_texture);
    }
}

void messageview_draw_scrollbar(struct AppState *app_state) {
    if (app_state->message_view.total_lines <= app_state->message_view.lines_per_page) {
        return; // No scrollbar needed
    }
    
    // Draw scrollbar background
    int scrollbar_x = app_state->message_view.window_width - SCROLLBAR_WIDTH;
    SDL_Rect scrollbar_bg = {scrollbar_x, 0, SCROLLBAR_WIDTH, app_state->message_view.window_height};
    
    SDL_SetRenderDrawColor(app_state->message_view.renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(app_state->message_view.renderer, &scrollbar_bg);
    
    // Calculate thumb size and position
    float thumb_ratio = (float)app_state->message_view.lines_per_page / app_state->message_view.total_lines;
    int thumb_height = (int)(app_state->message_view.window_height * thumb_ratio);
    if (thumb_height < 10) thumb_height = 10; // Minimum thumb size
    
    float scroll_ratio = (float)app_state->message_view.scroll_position /
                        (app_state->message_view.total_lines - app_state->message_view.lines_per_page);
    int thumb_y = (int)((app_state->message_view.window_height - thumb_height) * scroll_ratio);
    
    SDL_Rect thumb_rect = {scrollbar_x, thumb_y, SCROLLBAR_WIDTH, thumb_height};
    SDL_SetRenderDrawColor(app_state->message_view.renderer, 180, 180, 180, 255);
    SDL_RenderFillRect(app_state->message_view.renderer, &thumb_rect);
}

void messageview_render(SDL_Renderer *main_renderer, struct AppState *app_state) {
    (void)main_renderer; // Unused - we render to our own window
    
    // Only render if visible and window exists
    if (!app_state->message_view.is_visible || !app_state->message_view.window || !app_state->message_view.renderer) {
        return;
    }
    
    // Update layout in case window was resized
    messageview_update_layout(app_state);
    
    // Clear the message window
    SDL_SetRenderDrawColor(app_state->message_view.renderer, 30, 30, 30, 255);
    SDL_RenderClear(app_state->message_view.renderer);
    
    // Render visible messages
    SDL_Color text_color = {255, 255, 255, 255}; // White text
    int y_pos = MESSAGE_MARGIN;
    
    for (int i = 0; i < app_state->message_view.lines_per_page; i++) {
        int line_index = app_state->message_view.scroll_position + i;
        
        if (line_index >= app_state->message_view.total_lines) {
            break; // No more lines to display
        }
        
        // Get the wrapped line text
        const char *line_text = messages_get_wrapped_line(app_state, line_index);
        if (line_text) {
            render_text_line(line_text, MESSAGE_MARGIN, y_pos, text_color, app_state);
        }
        
        y_pos += MESSAGE_LINE_HEIGHT;
    }
    
    // Draw scrollbar
    messageview_draw_scrollbar(app_state);
    
    // Present the rendered frame
    SDL_RenderPresent(app_state->message_view.renderer);
}

static bool messageview_mouse_in_window(struct AppState *app_state) {
    if (!app_state->message_view.window) {
        return false;
    }
    
    int mouse_x, mouse_y;
    SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
    
    int window_x, window_y, window_w, window_h;
    SDL_GetWindowPosition(app_state->message_view.window, &window_x, &window_y);
    SDL_GetWindowSize(app_state->message_view.window, &window_w, &window_h);
    
    return (mouse_x >= window_x && mouse_x < window_x + window_w &&
            mouse_y >= window_y && mouse_y < window_y + window_h);
}

bool messageview_point_in_scrollbar(int x, int y, struct AppState *app_state) {
    int scrollbar_x = app_state->message_view.window_width - SCROLLBAR_WIDTH;
    return (x >= scrollbar_x && x < app_state->message_view.window_width &&
            y >= 0 && y < app_state->message_view.window_height);
}

int messageview_scrollbar_position_to_line(int y, struct AppState *app_state) {
    if (app_state->message_view.total_lines <= app_state->message_view.lines_per_page) {
        return 0;
    }
    
    float ratio = (float)y / app_state->message_view.window_height;
    int max_scroll = app_state->message_view.total_lines - app_state->message_view.lines_per_page;
    int line = (int)(ratio * max_scroll);
    
    if (line < 0) line = 0;
    if (line > max_scroll) line = max_scroll;
    
    return line;
}

bool messageview_handle_event(SDL_Event *event, struct AppState *app_state) {
    if (!app_state->message_view.is_visible) {
        return false; // Don't handle events when not visible
    }
    
    switch (event->type) {
        case SDL_WINDOWEVENT:
            if (event->window.windowID == SDL_GetWindowID(app_state->message_view.window)) {
                if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
                    messageview_update_layout(app_state);
                }
                if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
                    messageview_hide(app_state);
                    return true; // Event consumed
                }
            }
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if ((app_state->message_view.has_focus || app_state->message_view.scrollbar_dragging || messageview_mouse_in_window(app_state)) &&
                event->button.button == SDL_BUTTON_LEFT) {
                
                if (event->button.clicks == 1) {
                    messageview_scroll_up(3, app_state);
                } else if (event->button.clicks == 2) {
                    messageview_scroll_down(3, app_state);
                }
                return true;
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                int mouse_x = event->button.x;
                int mouse_y = event->button.y;
                
                if (messageview_point_in_scrollbar(mouse_x, mouse_y, app_state)) {
                    // Jump to clicked position
                    app_state->message_view.scroll_position = messageview_scrollbar_position_to_line(mouse_y, app_state);
                    return true;
                }
                
                app_state->message_view.scrollbar_dragging = false;
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (app_state->message_view.scrollbar_dragging) {
                int mouse_y = event->motion.y;
                app_state->message_view.scroll_position = messageview_scrollbar_position_to_line(mouse_y, app_state);
                return true;
            }
            break;
            
        case SDL_KEYDOWN:
            if (app_state->message_view.has_focus) {
                switch (event->key.keysym.sym) {
                    case SDLK_UP:
                        messageview_scroll_up(1, app_state);
                        return true;
                    case SDLK_DOWN:
                        messageview_scroll_down(1, app_state);
                        return true;
                    case SDLK_PAGEUP:
                        messageview_scroll_up(app_state->message_view.lines_per_page - 1, app_state);
                        return true;
                    case SDLK_PAGEDOWN:
                        messageview_scroll_down(app_state->message_view.lines_per_page - 1, app_state);
                        return true;
                    case SDLK_HOME:
                        messageview_scroll_to_top(app_state);
                        return true;
                    case SDLK_END:
                        messageview_scroll_to_bottom(app_state);
                        return true;
                }
            }
            break;
    }
    
    return false; // Event not handled
}

void messageview_scroll_up(int lines, struct AppState *app_state) {
    app_state->message_view.scroll_position -= lines;
    if (app_state->message_view.scroll_position < 0) {
        app_state->message_view.scroll_position = 0;
    }
}

void messageview_scroll_down(int lines, struct AppState *app_state) {
    int max_scroll = app_state->message_view.total_lines - app_state->message_view.lines_per_page;
    if (max_scroll < 0) max_scroll = 0;
    
    app_state->message_view.scroll_position += lines;
    if (app_state->message_view.scroll_position > max_scroll) {
        app_state->message_view.scroll_position = max_scroll;
    }
}

void messageview_scroll_to_bottom(struct AppState *app_state) {
    int max_scroll = app_state->message_view.total_lines - app_state->message_view.lines_per_page;
    if (max_scroll < 0) max_scroll = 0;
    app_state->message_view.scroll_position = max_scroll;
}

void messageview_scroll_to_top(struct AppState *app_state) {
    app_state->message_view.scroll_position = 0;
} 
