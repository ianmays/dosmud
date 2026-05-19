#ifndef WORLD_H
#define WORLD_H

#include "base.h"
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
    char animal[CFG_NAME_MAX];
    char animal_noise[CFG_DESC_MAX];
    int exits[CFG_DIR_MAX];
};

struct World {
    struct Room rooms[CFG_ROOM_MAX];
    int room_count;
    /*
     * Logical grid positions for ASCII map (derived during world_init when rooms
     * are linked; may overlap if the random graph folds).
     */
    int map_x[CFG_ROOM_MAX];
    int map_y[CFG_ROOM_MAX];
    u8 map_ready[CFG_ROOM_MAX];
};

void world_init(struct World *world);
#ifdef TEST_MODE
#define WORLD_FIXTURE_SNAPSHOT 0
void world_init_fixture(struct World *world, int preset);
#endif
void world_step(struct World *world, u32 tick);
int world_can_move(struct World *world, int room_id, int dir);
int world_move(struct World *world, int room_id, int dir);
const char *world_dir_name(int dir);
const char *world_room_animal(struct World *world, int room_id);
const char *world_room_animal_noise(struct World *world, int room_id);

#endif
