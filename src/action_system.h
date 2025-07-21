#ifndef ACTION_SYSTEM_H
#define ACTION_SYSTEM_H

#include "ecs.h"

// Forward declaration
struct AppState;

void action_system(Entity entity, struct AppState *app_state);
void action_system_register(void);
void action_system_init(void);
void action_move_entity(Entity entity, Direction direction, struct AppState *app_state);

#endif  
