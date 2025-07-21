#ifndef ACTION_SYSTEM_H
#define ACTION_SYSTEM_H

#include "ecs.h"

void action_system(Entity entity, World *world);
void action_system_register(void);
void action_system_init(void);
void action_move_entity(Entity entity, Direction direction, World *world);

#endif  
