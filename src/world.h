#ifndef WORLD_H
#define WORLD_H

#include "types.h"
#include "dungeon.h"

typedef struct {
    Dungeon dungeon;
    Entity player;
} World;

#endif

