#ifdef TEST_MODE

#include "config.h"
#include "game.h"
#include "world.h"
#include "th_world.h"

/*
 * Seed-1234 room graph (captured from world_init).
 * Exits: index 0..3 = north, south, east, west.
 */
static const int harness_world_exits[CFG_ROOM_MAX][CFG_DIR_MAX] = {
    {  1,  7, -1,  8 },
    { -1,  0, -1, 13 },
    { -1,  8, 14, -1 },
    { -1, -1,  8, -1 },
    { 12, 15, 11, -1 },
    { 15, -1, -1, -1 },
    { -1, -1, -1, 15 },
    {  0, -1, -1, -1 },
    {  2, -1,  0,  3 },
    { -1, 10, 13, -1 },
    {  9, 14, 15, -1 },
    { -1, -1, -1,  4 },
    { -1,  4, -1, -1 },
    { -1, -1,  1,  9 },
    { 10, -1, -1,  2 },
    {  4,  5,  6, 10 }
};

static const int harness_world_map_x[CFG_ROOM_MAX] = {
    2, 2, 2, 0, 0, -1, 1, 2, 1, 0, -1, 1, 0, 1, -1, 0
};

static const int harness_world_map_y[CFG_ROOM_MAX] = {
    0, -1, -3, 0, -3, -3, -2, 1, 0, -1, -2, -3, -4, -1, -1, -2
};

void harness_world_boot_graph(struct GameState *game)
{
    world_apply_graph(&game->world,
        harness_world_exits,
        harness_world_map_x,
        harness_world_map_y);
}

#endif
