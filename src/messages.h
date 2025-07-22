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

// Forward declaration
struct AppState;

// Message system functions
void messages_init(struct AppState *app_state);
void messages_shutdown(struct AppState *app_state);

// Message management  
void messages_add(struct AppState *app_state, const char *text);
void messages_clear(struct AppState *app_state);
int messages_get_count(struct AppState *app_state);

// Text wrapping and display
void messages_rewrap_text(struct AppState *app_state, int window_width);
int messages_get_wrapped_line_count(struct AppState *app_state);
const char* messages_get_wrapped_line(struct AppState *app_state, int index);
int messages_get_message_index_for_line(struct AppState *app_state, int line_index);

// Message access
const Message* messages_get(struct AppState *app_state, int index);
const Message* messages_get_latest(struct AppState *app_state);

#endif 
