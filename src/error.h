#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include <stdint.h>

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
    Result code;                        // Error code
    char message[256];                  // Detailed error message
    const char *file;                   // Source file where error occurred
    int line;                           // Line number where error occurred
    const char *function;               // Function where error occurred
    uint32_t error_id;                  // Unique error instance ID
} ErrorContext;

// Global error context (thread-local in multi-threaded environments)
extern ErrorContext g_last_error;

// Error handling functions
void error_set(Result code, const char *file, int line, const char *function, const char *format, ...);
void error_clear(void);
const ErrorContext* error_get_last(void);
bool error_has_error(void);
const char* error_code_to_string(Result code);

// Convenience macros for error handling
#define ERROR_SET(code, ...) error_set(code, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define ERROR_RETURN(code, ...) do { ERROR_SET(code, __VA_ARGS__); return code; } while(0)
#define ERROR_RETURN_NULL(code, ...) do { ERROR_SET(code, __VA_ARGS__); return NULL; } while(0)
#define ERROR_RETURN_FALSE(code, ...) do { ERROR_SET(code, __VA_ARGS__); return false; } while(0)

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
