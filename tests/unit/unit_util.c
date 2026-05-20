#include "unit_util.h"
#include "items.h"
#include "world.h"

static const int unit_world_exits[CFG_ROOM_MAX][CFG_DIR_MAX] = {
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

static const int unit_world_map_x[CFG_ROOM_MAX] = {
    2, 2, 2, 0, 0, -1, 1, 2, 1, 0, -1, 1, 0, 1, -1, 0
};

static const int unit_world_map_y[CFG_ROOM_MAX] = {
    0, -1, -3, 0, -3, -3, -2, 1, 0, -1, -2, -3, -4, -1, -1, -2
};

void unit_world_boot_graph(struct GameState *game)
{
    world_apply_graph(&game->world,
        unit_world_exits,
        unit_world_map_x,
        unit_world_map_y);
}

void unit_game_fresh(struct GameState *game, u32 seed)
{
    game_init(game, seed);
    unit_world_boot_graph(game);
    plat_seed_rng(seed);
}

void unit_game_baseline(struct GameState *game, int room_id, u32 tick)
{
    game_reset_fixture_baseline(game, room_id, tick);
    plat_seed_rng(game->seed);
}

void unit_bag_fill(struct GameState *game)
{
    int i;

    game->bag_count = 0;
    for (i = 0; i < game->bag_capacity; ++i) {
        game->bag[i] = ITEM_BERRY + (i % 9);
        if (game->bag[i] == ITEM_NONE) {
            game->bag[i] = ITEM_BERRY;
        }
        game->bag_count += 1;
    }
}
