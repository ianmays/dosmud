#ifndef WORLD_H
#define WORLD_H

#include "config.h"

enum Direction {
    DIR_NORTH = 0,
    DIR_SOUTH = 1,
    DIR_EAST = 2,
    DIR_WEST = 3,
    DIR_NONE = 4
};

struct Room {
    char name[CFG_NAME_MAX];
    char desc[CFG_DESC_MAX];
    int exits[CFG_DIR_MAX];
};

struct World {
    struct Room rooms[CFG_ROOM_MAX];
    int room_count;
};

void world_init(struct World *world);
void world_step(struct World *world, unsigned long tick);
int world_can_move(struct World *world, int room_id, int dir);
int world_move(struct World *world, int room_id, int dir);
const char *world_dir_name(int dir);

#endif
