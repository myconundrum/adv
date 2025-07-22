#include "messages.h"
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include "log.h"

// Global message queue
MessageQueue g_message_queue = {0};

void messages_init(void) {
    memset(&g_message_queue, 0, sizeof(MessageQueue));
    g_message_queue.head = 0;
    g_message_queue.count = 0;
    g_message_queue.total_wrapped_lines = 0;
    g_message_queue.need_rewrap = true;
    
    LOG_INFO("Message system initialized");
}

void messages_shutdown(void) {
    memset(&g_message_queue, 0, sizeof(MessageQueue));
    LOG_INFO("Message system shutdown");
}

void messages_add(const char *text) {
    if (!text || strlen(text) == 0) {
        return;
    }
    
    // Get the next slot in the circular buffer
    int index = g_message_queue.head;
    
    // Clear the message slot
    memset(&g_message_queue.messages[index], 0, sizeof(Message));
    
    // Copy the text (truncate if too long)
    strncpy(g_message_queue.messages[index].text, text, MAX_MESSAGE_TEXT_LENGTH - 1);
    g_message_queue.messages[index].text[MAX_MESSAGE_TEXT_LENGTH - 1] = '\0';
    
    // Set timestamp and validity
    g_message_queue.messages[index].timestamp = (uint32_t)time(NULL);
    g_message_queue.messages[index].is_valid = true;
    
    // Update queue state
    g_message_queue.head = (g_message_queue.head + 1) % MESSAGEQUEUELENGTH;
    if (g_message_queue.count < MESSAGEQUEUELENGTH) {
        g_message_queue.count++;
    }
    
    // Mark that we need to rewrap text
    g_message_queue.need_rewrap = true;
    
    LOG_INFO("Added message: %.50s%s", text, strlen(text) > 50 ? "..." : "");
}

void messages_clear(void) {
    memset(&g_message_queue, 0, sizeof(MessageQueue));
    g_message_queue.head = 0;
    g_message_queue.count = 0;
    g_message_queue.total_wrapped_lines = 0;
    g_message_queue.need_rewrap = true;
}

int messages_get_count(void) {
    return g_message_queue.count;
}

const Message* messages_get(int index) {
    if (index < 0 || index >= g_message_queue.count) {
        return NULL;
    }
    
    // Calculate the actual index in the circular buffer
    // We want index 0 to be the oldest message
    int actual_index;
    if (g_message_queue.count < MESSAGEQUEUELENGTH) {
        // Buffer not full yet, messages start at 0
        actual_index = index;
    } else {
        // Buffer is full, oldest message is at head
        actual_index = (g_message_queue.head + index) % MESSAGEQUEUELENGTH;
    }
    
    if (g_message_queue.messages[actual_index].is_valid) {
        return &g_message_queue.messages[actual_index];
    }
    
    return NULL;
}

const Message* messages_get_latest(void) {
    if (g_message_queue.count == 0) {
        return NULL;
    }
    
    return messages_get(g_message_queue.count - 1);
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

void messages_rewrap_text(int window_width) {
    if (!g_message_queue.need_rewrap && window_width > 0) {
        return;
    }
    
    // Calculate characters per line (approximate, accounting for font width)
    // Assume average character width of 8 pixels for a 16px font
    int chars_per_line = (window_width - 20) / 8; // 20px margin
    if (chars_per_line < 10) chars_per_line = 10; // Minimum width
    
    // Clear existing wrapped lines
    memset(g_message_queue.wrapped_lines, 0, sizeof(g_message_queue.wrapped_lines));
    g_message_queue.total_wrapped_lines = 0;
    
    // Process each message
    for (int msg_idx = 0; msg_idx < g_message_queue.count; msg_idx++) {
        const Message *message = messages_get(msg_idx);
        if (!message || !message->is_valid) continue;
        
        const char *text = message->text;
        int text_len = strlen(text);
        int pos = 0;
        
        while (pos < text_len && g_message_queue.total_wrapped_lines < (MESSAGEQUEUELENGTH * MAX_WRAPPED_LINES - 1)) {
            // Find where to break this line
            int break_pos = find_word_break(text + pos, chars_per_line);
            if (break_pos == 0) break_pos = 1; // Ensure progress
            
            // Copy the line
            int line_idx = g_message_queue.total_wrapped_lines;
            strncpy(g_message_queue.wrapped_lines[line_idx].line, text + pos, break_pos);
            g_message_queue.wrapped_lines[line_idx].line[break_pos] = '\0';
            g_message_queue.wrapped_lines[line_idx].message_index = msg_idx;
            
            g_message_queue.total_wrapped_lines++;
            pos += break_pos;
            
            // Skip whitespace at the beginning of next line
            while (pos < text_len && isspace((unsigned char)text[pos])) {
                pos++;
            }
        }
    }
    
    g_message_queue.need_rewrap = false;
    LOG_DEBUG("Rewrapped messages: %d lines from %d messages", 
              g_message_queue.total_wrapped_lines, g_message_queue.count);
}

int messages_get_wrapped_line_count(void) {
    return g_message_queue.total_wrapped_lines;
}

const char* messages_get_wrapped_line(int index) {
    if (index < 0 || index >= g_message_queue.total_wrapped_lines) {
        return NULL;
    }
    
    return g_message_queue.wrapped_lines[index].line;
}

int messages_get_message_index_for_line(int line_index) {
    if (line_index < 0 || line_index >= g_message_queue.total_wrapped_lines) {
        return -1;
    }
    
    return g_message_queue.wrapped_lines[line_index].message_index;
} 
