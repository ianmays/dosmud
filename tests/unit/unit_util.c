#include "unit_util.h"
#include "th_world.h"
#include "items.h"
#include "world.h"

void unit_world_boot_graph(struct GameState *game)
{
    harness_world_boot_graph(game);
}

void unit_game_fresh(struct GameState *game, u32 seed)
{
    game_init(game, seed);
    unit_world_boot_graph(game);
    plat_seed_rng(seed);
}
