#include "messages.h"
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include "log.h"
#include "appstate.h"

void messages_init(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return;
    }
    
    memset(&app_state->messages, 0, sizeof(MessageQueue));
    app_state->messages.head = 0;
    app_state->messages.count = 0;
    app_state->messages.total_wrapped_lines = 0;
    app_state->messages.need_rewrap = true;
    
    LOG_INFO("Message system initialized");
}

void messages_shutdown(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return;
    }
    
    memset(&app_state->messages, 0, sizeof(MessageQueue));
    LOG_INFO("Message system shutdown");
}

void messages_add(AppState *app_state, const char *text) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return;
    }
    
    if (!text || strlen(text) == 0) {
        return;
    }
    
    // Get the next slot in the circular buffer
    int index = app_state->messages.head;
    
    // Clear the message slot
    memset(&app_state->messages.messages[index], 0, sizeof(Message));
    
    // Copy the text (truncate if too long)
    strncpy(app_state->messages.messages[index].text, text, MAX_MESSAGE_TEXT_LENGTH - 1);
    app_state->messages.messages[index].text[MAX_MESSAGE_TEXT_LENGTH - 1] = '\0';
    
    // Set timestamp and validity
    app_state->messages.messages[index].timestamp = (uint32_t)time(NULL);
    app_state->messages.messages[index].is_valid = true;
    
    // Update queue state
    app_state->messages.head = (app_state->messages.head + 1) % MESSAGEQUEUELENGTH;
    if (app_state->messages.count < MESSAGEQUEUELENGTH) {
        app_state->messages.count++;
    }
    
    // Mark that we need to rewrap text
    app_state->messages.need_rewrap = true;
    
    LOG_INFO("Added message: %.50s%s", text, strlen(text) > 50 ? "..." : "");
}

void messages_clear(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return;
    }
    
    memset(&app_state->messages, 0, sizeof(MessageQueue));
    app_state->messages.head = 0;
    app_state->messages.count = 0;
    app_state->messages.total_wrapped_lines = 0;
    app_state->messages.need_rewrap = true;
}

int messages_get_count(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return 0;
    }
    
    return app_state->messages.count;
}

const Message* messages_get(AppState *app_state, int index) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return NULL;
    }
    
    if (index < 0 || index >= app_state->messages.count) {
        return NULL;
    }
    
    // Calculate the actual index in the circular buffer
    // We want index 0 to be the oldest message
    int actual_index;
    if (app_state->messages.count < MESSAGEQUEUELENGTH) {
        // Buffer not full yet, messages start at 0
        actual_index = index;
    } else {
        // Buffer is full, oldest message is at head
        actual_index = (app_state->messages.head + index) % MESSAGEQUEUELENGTH;
    }
    
    if (app_state->messages.messages[actual_index].is_valid) {
        return &app_state->messages.messages[actual_index];
    }
    
    return NULL;
}

const Message* messages_get_latest(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return NULL;
    }
    
    if (app_state->messages.count == 0) {
        return NULL;
    }
    
    return messages_get(app_state, app_state->messages.count - 1);
}

// Helper function to break a line at word boundaries
static int find_word_break(const char *text, int max_width) {
    if (max_width <= 0) return 0;
    
    int len = strlen(text);
    if (len <= max_width) {
        return len;
    }
    
    // Start from max_width and work backwards to find a space
    for (int i = max_width; i > 0; i--) {
        if (isspace((unsigned char)text[i])) {
            return i;
        }
    }
    
    // No space found, break at max_width
    return max_width;
}

void messages_rewrap_text(AppState *app_state, int window_width) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return;
    }
    
    if (!app_state->messages.need_rewrap && window_width > 0) {
        return;
    }
    
    // Calculate characters per line (approximate, accounting for font width)
    // Assume average character width of 8 pixels for a 16px font
    int chars_per_line = (window_width - 20) / 8; // 20px margin
    if (chars_per_line < 10) chars_per_line = 10; // Minimum width
    
    // Clear existing wrapped lines
    memset(app_state->messages.wrapped_lines, 0, sizeof(app_state->messages.wrapped_lines));
    app_state->messages.total_wrapped_lines = 0;
    
    // Process each message
    for (int msg_idx = 0; msg_idx < app_state->messages.count; msg_idx++) {
        const Message *message = messages_get(app_state, msg_idx);
        if (!message || !message->is_valid) continue;
        
        const char *text = message->text;
        int text_len = strlen(text);
        int pos = 0;
        
        while (pos < text_len && app_state->messages.total_wrapped_lines < (MESSAGEQUEUELENGTH * MAX_WRAPPED_LINES - 1)) {
            // Find where to break this line
            int break_pos = find_word_break(text + pos, chars_per_line);
            if (break_pos == 0) break_pos = 1; // Ensure progress
            
            // Copy the line
            int line_idx = app_state->messages.total_wrapped_lines;
            strncpy(app_state->messages.wrapped_lines[line_idx].line, text + pos, break_pos);
            app_state->messages.wrapped_lines[line_idx].line[break_pos] = '\0';
            app_state->messages.wrapped_lines[line_idx].message_index = msg_idx;
            
            app_state->messages.total_wrapped_lines++;
            pos += break_pos;
            
            // Skip whitespace at the beginning of next line
            while (pos < text_len && isspace((unsigned char)text[pos])) {
                pos++;
            }
        }
    }
    
    app_state->messages.need_rewrap = false;
    LOG_DEBUG("Rewrapped messages: %d lines from %d messages", 
              app_state->messages.total_wrapped_lines, app_state->messages.count);
}

int messages_get_wrapped_line_count(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return 0;
    }
    
    return app_state->messages.total_wrapped_lines;
}

const char* messages_get_wrapped_line(AppState *app_state, int index) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return NULL;
    }
    
    if (index < 0 || index >= app_state->messages.total_wrapped_lines) {
        return NULL;
    }
    
    return app_state->messages.wrapped_lines[index].line;
}

int messages_get_message_index_for_line(AppState *app_state, int line_index) {
    if (!app_state) {
        LOG_ERROR("AppState cannot be NULL");
        return -1;
    }
    
    if (line_index < 0 || line_index >= app_state->messages.total_wrapped_lines) {
        return -1;
    }
    
    return app_state->messages.wrapped_lines[line_index].message_index;
} 

