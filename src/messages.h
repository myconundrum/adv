#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Configuration constants
#define MESSAGEQUEUELENGTH 100
#define MAX_MESSAGE_TEXT_LENGTH 512
#define MAX_WRAPPED_LINES 20

// Message structure
typedef struct {
    char text[MAX_MESSAGE_TEXT_LENGTH];
    uint32_t timestamp;
    bool is_valid;
} Message;

// Wrapped line structure for display
typedef struct {
    char line[MAX_MESSAGE_TEXT_LENGTH];
    int message_index; // Which message this line belongs to
} WrappedLine;

// Message queue structure
typedef struct {
    Message messages[MESSAGEQUEUELENGTH];
    int head; // Next position to write
    int count; // Number of valid messages
    
    // Cached wrapped lines for rendering
    WrappedLine wrapped_lines[MESSAGEQUEUELENGTH * MAX_WRAPPED_LINES];
    int total_wrapped_lines;
    bool need_rewrap; // Flag to indicate text needs rewrapping
} MessageQueue;

// Global message queue
extern MessageQueue g_message_queue;

// Message system functions
void messages_init(void);
void messages_shutdown(void);

// Message management
void messages_add(const char *text);
void messages_clear(void);
int messages_get_count(void);

// Text wrapping and display
void messages_rewrap_text(int window_width);
int messages_get_wrapped_line_count(void);
const char* messages_get_wrapped_line(int index);
int messages_get_message_index_for_line(int line_index);

// Message access
const Message* messages_get(int index);
const Message* messages_get_latest(void);

#endif 
