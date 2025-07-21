#include "error.h"
#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Global error context
ErrorContext g_last_error = {RESULT_OK, "", NULL, 0, NULL, 0};

// Global error counter for unique error IDs
static uint32_t g_error_counter = 0;

void error_set(Result code, const char *file, int line, const char *function, const char *format, ...) {
    g_last_error.code = code;
    g_last_error.file = file;
    g_last_error.line = line;
    g_last_error.function = function;
    g_last_error.error_id = ++g_error_counter;
    
    // Format the error message
    if (format) {
        va_list args;
        va_start(args, format);
        vsnprintf(g_last_error.message, sizeof(g_last_error.message), format, args);
        va_end(args);
    } else {
        strncpy(g_last_error.message, error_code_to_string(code), sizeof(g_last_error.message) - 1);
        g_last_error.message[sizeof(g_last_error.message) - 1] = '\0';
    }
    
    // Log the error
    LOG_ERROR("[Error #%u] %s in %s:%d (%s()): %s", 
              g_last_error.error_id,
              error_code_to_string(code),
              file ? file : "unknown",
              line,
              function ? function : "unknown",
              g_last_error.message);
}

void error_clear(void) {
    g_last_error.code = RESULT_OK;
    g_last_error.message[0] = '\0';
    g_last_error.file = NULL;
    g_last_error.line = 0;
    g_last_error.function = NULL;
    g_last_error.error_id = 0;
}

const ErrorContext* error_get_last(void) {
    return &g_last_error;
}

bool error_has_error(void) {
    return g_last_error.code != RESULT_OK;
}

const char* error_code_to_string(Result code) {
    switch (code) {
        case RESULT_OK:
            return "Success";
        case RESULT_ERROR_NULL_POINTER:
            return "Null pointer";
        case RESULT_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case RESULT_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case RESULT_ERROR_OUT_OF_BOUNDS:
            return "Out of bounds";
        case RESULT_ERROR_NOT_FOUND:
            return "Not found";
        case RESULT_ERROR_ALREADY_EXISTS:
            return "Already exists";
        case RESULT_ERROR_INITIALIZATION_FAILED:
            return "Initialization failed";
        case RESULT_ERROR_FILE_IO:
            return "File I/O error";
        case RESULT_ERROR_PARSE_ERROR:
            return "Parse error";
        case RESULT_ERROR_SYSTEM_LIMIT:
            return "System limit reached";
        case RESULT_ERROR_COMPONENT_NOT_FOUND:
            return "Component not found";
        case RESULT_ERROR_ENTITY_INVALID:
            return "Invalid entity";
        case RESULT_ERROR_TEMPLATE_ERROR:
            return "Template error";
        case RESULT_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
} 
