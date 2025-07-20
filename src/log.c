#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// ANSI color codes
#define ANSI_RESET   "\033[0m"
#define ANSI_BLACK   "\033[30m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_BOLD    "\033[1m"

// Global logging state
static struct {
    LogLevel min_level;
    bool use_colors;
    bool use_timestamps;
    FILE *log_file;
    bool initialized;
} g_log_state = {0};

// Log level names
static const char* level_names[] = {
    "TRACE",
    "DEBUG", 
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

// Log level colors (ANSI codes)
static const char* level_colors[] = {
    ANSI_CYAN,    // TRACE
    ANSI_BLUE,    // DEBUG
    ANSI_GREEN,   // INFO
    ANSI_YELLOW,  // WARN
    ANSI_RED,     // ERROR
    ANSI_BOLD ANSI_RED  // FATAL
};

void log_init(LogConfig config) {
    g_log_state.min_level = config.min_level;
    g_log_state.use_colors = config.use_colors;
    g_log_state.use_timestamps = config.use_timestamps;
    
    // Open log file if specified
    if (config.log_file && strlen(config.log_file) > 0) {
        g_log_state.log_file = fopen(config.log_file, "w");
        if (!g_log_state.log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", config.log_file);
        }
    }
    
    g_log_state.initialized = true;
    
    LOG_INFO("Logging system initialized (level: %s, colors: %s)",
             level_names[config.min_level],
             config.use_colors ? "enabled" : "disabled");
}

void log_shutdown(void) {
    if (g_log_state.log_file) {
        fclose(g_log_state.log_file);
        g_log_state.log_file = NULL;
    }
    g_log_state.initialized = false;
}

void log_set_level(LogLevel level) {
    g_log_state.min_level = level;
}

void log_set_colors(bool enable) {
    g_log_state.use_colors = enable;
}

void log_set_timestamps(bool enable) {
    g_log_state.use_timestamps = enable;
}

const char* log_level_name(LogLevel level) {
    if (level >= 0 && level < LOG_LEVEL_COUNT) {
        return level_names[level];
    }
    return "UNKNOWN";
}

const char* log_level_color(LogLevel level) {
    if (level >= 0 && level < LOG_LEVEL_COUNT) {
        return level_colors[level];
    }
    return ANSI_RESET;
}

void log_message(LogLevel level, const char *file, int line, const char *func, const char *format, ...) {
    (void)file;  // Suppress unused parameter warning
    (void)line;  // Suppress unused parameter warning
    (void)func;  // Suppress unused parameter warning
    
    if (!g_log_state.initialized || level < g_log_state.min_level) {
        return;
    }
    

    
    // Format the message
    va_list args;
    va_start(args, format);
    
    // Calculate required buffer size
    va_list args_copy;
    va_copy(args_copy, args);
    int msg_len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (msg_len < 0) {
        va_end(args);
        return;
    }
    
    // Allocate buffer for message
    char *message = malloc(msg_len + 1);
    if (!message) {
        va_end(args);
        return;
    }
    
    vsnprintf(message, msg_len + 1, format, args);
    va_end(args);
    
    // Format the complete log line
    char log_line[1024];
    int offset = 0;
    
    // Add log level only
    offset += snprintf(log_line + offset, sizeof(log_line) - offset, 
                      "[%s] %s", 
                      level_names[level], message);
    
    // Output to console with colors if enabled
    if (g_log_state.use_colors) {
        fprintf(stdout, "%s%s%s\n", level_colors[level], log_line, ANSI_RESET);
    } else {
        fprintf(stdout, "%s\n", log_line);
    }
    
    // Output to file without colors
    if (g_log_state.log_file) {
        fprintf(g_log_state.log_file, "%s\n", log_line);
        fflush(g_log_state.log_file);
    }
    
    free(message);
} 
