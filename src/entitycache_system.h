#ifndef ENTITYCACHE_SYSTEM_H
#define ENTITYCACHE_SYSTEM_H

#include "world.h"
#include "ecs.h"

void entitycache_system_register(void);
void entitycache_system_init(void);
void entitycache_system_shutdown(void);
void entitycache_system(Entity entity, void *world);
void entitycache_system_pre_update(void *world);
void entitycache_system_post_update(void *world);

bool entitycache_position_has_entities(int x, int y, stack * stack);
void entitycache_remove_entity(Entity entity);

#endif

