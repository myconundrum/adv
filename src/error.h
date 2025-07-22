#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Standard result codes for consistent error handling
typedef enum {
    RESULT_OK = 0,                      // Success
    RESULT_ERROR_NULL_POINTER,          // Null pointer passed
    RESULT_ERROR_INVALID_PARAMETER,     // Invalid parameter value
    RESULT_ERROR_OUT_OF_MEMORY,         // Memory allocation failed
    RESULT_ERROR_OUT_OF_BOUNDS,         // Array/buffer overflow
    RESULT_ERROR_NOT_FOUND,             // Item/entity not found
    RESULT_ERROR_ALREADY_EXISTS,        // Item already exists
    RESULT_ERROR_INITIALIZATION_FAILED, // System initialization failed
    RESULT_ERROR_FILE_IO,               // File I/O error
    RESULT_ERROR_PARSE_ERROR,           // Parsing/format error
    RESULT_ERROR_SYSTEM_LIMIT,          // System limit reached
    RESULT_ERROR_COMPONENT_NOT_FOUND,   // Component not found
    RESULT_ERROR_ENTITY_INVALID,        // Invalid entity
    RESULT_ERROR_TEMPLATE_ERROR,        // Template system error
    RESULT_ERROR_UNKNOWN                // Unknown/unspecified error
} Result;

// Error context for detailed error information
typedef struct {
    Result result;                      // Error result/code
    char message[256];                  // Detailed error message
    const char *file;                   // Source file where error occurred
    int line;                           // Line number where error occurred
    const char *function;               // Function where error occurred
} ErrorContext;

// Forward declaration
struct AppState;

// Error handling functions - all require AppState
void error_set(Result result, const char *message, const char *file, int line, const char *function, struct AppState *app_state);
void error_clear(struct AppState *app_state);
Result error_get_last(struct AppState *app_state);
bool error_has_error(struct AppState *app_state);
const char* error_code_to_string(Result code);

// Convenience macros
#define ERROR_SET(result, ...) do { \
    char _error_buf[256]; \
    snprintf(_error_buf, sizeof(_error_buf), __VA_ARGS__); \
    error_set((result), _error_buf, __FILE__, __LINE__, __func__, appstate_get()); \
} while(0)

#define ERROR_CLEAR() error_clear(appstate_get())
#define ERROR_GET_LAST() error_get_last(appstate_get())
#define ERROR_HAS_ERROR() error_has_error(appstate_get())
#define ERROR_RETURN(result, ...) do { ERROR_SET((result), __VA_ARGS__); return (result); } while(0)
#define ERROR_RETURN_FALSE(result, ...) do { ERROR_SET((result), __VA_ARGS__); return false; } while(0)
#define ERROR_RETURN_NULL(result, ...) do { ERROR_SET((result), __VA_ARGS__); return NULL; } while(0)

// Success check macros
#define RESULT_OK_OR_RETURN(expr) do { \
    Result _r = (expr); \
    if (_r != RESULT_OK) return _r; \
} while(0)

#define RESULT_OK_OR_RETURN_NULL(expr) do { \
    Result _r = (expr); \
    if (_r != RESULT_OK) return NULL; \
} while(0)

#define RESULT_OK_OR_RETURN_FALSE(expr) do { \
    Result _r = (expr); \
    if (_r != RESULT_OK) return false; \
} while(0)

// Validation macros
#define VALIDATE_NOT_NULL(ptr, name) do { \
    if (!(ptr)) { \
        ERROR_RETURN(RESULT_ERROR_NULL_POINTER, "%s cannot be NULL", name); \
    } \
} while(0)

#define VALIDATE_NOT_NULL_FALSE(ptr, name) do { \
    if (!(ptr)) { \
        ERROR_RETURN_FALSE(RESULT_ERROR_NULL_POINTER, "%s cannot be NULL", name); \
    } \
} while(0)

#define VALIDATE_NOT_NULL_NULL(ptr, name) do { \
    if (!(ptr)) { \
        ERROR_RETURN_NULL(RESULT_ERROR_NULL_POINTER, "%s cannot be NULL", name); \
    } \
} while(0)

#define VALIDATE_BOUNDS(value, min, max, name) do { \
    if ((value) < (min) || (value) > (max)) { \
        ERROR_RETURN(RESULT_ERROR_OUT_OF_BOUNDS, "%s (%d) must be between %d and %d", name, (int)(value), (int)(min), (int)(max)); \
    } \
} while(0)

#endif // ERROR_H 
