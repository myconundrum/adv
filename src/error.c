#include "error.h"
#include "appstate.h"
#include "log.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

void error_set(Result result, const char *message, const char *file, int line, const char *function, AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState is NULL in error_set - cannot store error");
        return;
    }
    
    app_state->error.result = result;
    app_state->error_counter++;
    
    // Store error context
    app_state->error.file = file;
    app_state->error.line = line;
    app_state->error.function = function;
    
    // Copy message
    if (message) {
        strncpy(app_state->error.message, message, sizeof(app_state->error.message) - 1);
        app_state->error.message[sizeof(app_state->error.message) - 1] = '\0';
    } else {
        app_state->error.message[0] = '\0';
    }
}

void error_clear(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState is NULL in error_clear");
        return;
    }
    
    memset(&app_state->error, 0, sizeof(ErrorContext));
    app_state->error.result = RESULT_OK;
}

Result error_get_last(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState is NULL in error_get_last");
        return RESULT_ERROR_NULL_POINTER;
    }
    
    return app_state->error.result;
}

bool error_has_error(AppState *app_state) {
    if (!app_state) {
        LOG_ERROR("AppState is NULL in error_has_error");
        return true; // Assume error if AppState is NULL
    }
    
    return app_state->error.result != RESULT_OK;
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
