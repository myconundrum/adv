#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdbool.h>

// Log levels
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_COUNT
} LogLevel;

// Log configuration
typedef struct {
    LogLevel min_level;
    bool use_colors;
    bool use_timestamps;
    const char *log_file;
} LogConfig;

// Initialize logging system
void log_init(LogConfig config);

// Shutdown logging system
void log_shutdown(void);

// Set minimum log level
void log_set_level(LogLevel level);

// Enable/disable colors
void log_set_colors(bool enable);

// Enable/disable timestamps
void log_set_timestamps(bool enable);

// Main logging function
void log_message(LogLevel level, const char *file, int line, const char *func, const char *format, ...);

// Convenience macros for logging
#define LOG_TRACE(...) log_message(LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)  log_message(LOG_LEVEL_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)  log_message(LOG_LEVEL_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Get log level name
const char* log_level_name(LogLevel level);

// Get log level color code
const char* log_level_color(LogLevel level);

#endif // LOG_H 
