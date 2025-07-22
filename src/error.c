#include "error.h"
#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Include AppState for accessing error fields
// Note: This is safe because error.h doesn't include appstate.h
#include "appstate.h"

// Global error context (fallback when AppState not available)
ErrorContext g_last_error = {RESULT_OK, "", NULL, 0, NULL, 0};

// Global error counter for unique error IDs (fallback)
static uint32_t g_error_counter = 0;

void error_set(Result code, const char *file, int line, const char *function, const char *format, ...) {
    // Try to get AppState for better error tracking
    struct AppState *app_state = appstate_get();
    ErrorContext *error_ctx = app_state ? &app_state->error : &g_last_error;
    uint32_t *error_counter = app_state ? &app_state->error_counter : &g_error_counter;
    
    error_ctx->code = code;
    error_ctx->file = file;
    error_ctx->line = line;
    error_ctx->function = function;
    error_ctx->error_id = ++(*error_counter);
    
    // Format the error message
    if (format) {
        va_list args;
        va_start(args, format);
        vsnprintf(error_ctx->message, sizeof(error_ctx->message), format, args);
        va_end(args);
    } else {
        strncpy(error_ctx->message, error_code_to_string(code), sizeof(error_ctx->message) - 1);
        error_ctx->message[sizeof(error_ctx->message) - 1] = '\0';
    }
    
    // Also update global for backward compatibility when AppState is available
    if (app_state) {
        g_last_error = *error_ctx;
    }
    
    // Log the error
    LOG_ERROR("[Error #%u] %s in %s:%d (%s()): %s", 
              error_ctx->error_id,
              error_code_to_string(code),
              file ? file : "unknown",
              line,
              function ? function : "unknown",
              error_ctx->message);
}

void error_clear(void) {
    // Try to get AppState and clear its error context
    struct AppState *app_state = appstate_get();
    ErrorContext *error_ctx = app_state ? &app_state->error : &g_last_error;
    
    error_ctx->code = RESULT_OK;
    error_ctx->message[0] = '\0';
    error_ctx->file = NULL;
    error_ctx->line = 0;
    error_ctx->function = NULL;
    error_ctx->error_id = 0;
    
    // Also clear global for backward compatibility
    if (app_state) {
        g_last_error = *error_ctx;
    }
}

const ErrorContext* error_get_last(void) {
    // Try to get AppState and return its error context
    struct AppState *app_state = appstate_get();
    return app_state ? &app_state->error : &g_last_error;
}

bool error_has_error(void) {
    // Try to get AppState and check its error context
    struct AppState *app_state = appstate_get();
    ErrorContext *error_ctx = app_state ? &app_state->error : &g_last_error;
    return error_ctx->code != RESULT_OK;
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
