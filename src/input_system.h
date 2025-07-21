#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include "ecs.h"
#include "world.h"

// Process input for a specific entity
void input_system(Entity entity, World *world);

// Initialize input system
void input_system_init(void);

// Register input system with ECS
void input_system_register(void);

#endif  
