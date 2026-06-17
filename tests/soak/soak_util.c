#include <stdio.h>
#include <string.h>
#include "soak_util.h"
#include "config.h"
#include "game.h"

static unsigned long soak_limit_for_name(const char *name)
{
    if (strcmp(name, "background_ticks") == 0) {
        return CFG_TEST_SOAK_LIMIT_BACKGROUND_TICKS;
    }
    if (strcmp(name, "command_ticks") == 0) {
        return CFG_TEST_SOAK_LIMIT_COMMAND_TICKS;
    }
    if (strcmp(name, "combat_rounds") == 0) {
        return CFG_TEST_SOAK_LIMIT_COMBAT_ROUNDS;
    }
    return 0;
}

int soak_assert_game_state_ok(const struct GameState *game)
{
    if (game->running != 1) {
        return 0;
    }
    if (game->mode < GAME_MODE_EXPLORE || game->mode > GAME_MODE_COMBAT) {
        return 0;
    }
    if (game->max_hp <= 0) {
        return 0;
    }
    if (game->player_hp < 0 || game->player_hp > game->max_hp) {
        return 0;
    }
    if (game->player.room_id < 0 ||
            game->player.room_id >= game->world.room_count) {
        return 0;
    }
    if (game->bag_count < 0 ||
            game->bag_count > game->bag_capacity ||
            game->bag_capacity > CFG_BAG_MAX) {
        return 0;
    }
    if (game->mode == GAME_MODE_DIALOGUE) {
        if (game->dialogue <= DIALOGUE_NONE ||
                game->dialogue > DIALOGUE_LOOT) {
            return 0;
        }
    }
    if (game->mode == GAME_MODE_COMBAT && game->combat.enemy_hp < 0) {
        return 0;
    }
    return 1;
}

unsigned long soak_print_bench(const char *name, unsigned long ticks, clock_t elapsed)
{
    unsigned long us;
    unsigned long us_per_tick;
    unsigned long limit;

    if (ticks == 0) {
        ticks = 1;
    }
    us = (unsigned long)((elapsed * 1000000UL) / (clock_t)CLOCKS_PER_SEC);
    us_per_tick = us / ticks;
    limit = soak_limit_for_name(name);
    printf("SOAK_BENCH %s ticks=%lu us_per_tick=%lu limit=%lu\n",
        name, ticks, us_per_tick, limit);
    return us_per_tick;
}

int soak_check_limit(const char *name, unsigned long us_per_tick)
{
    unsigned long limit;

    limit = soak_limit_for_name(name);
    if (limit == 0) {
        return 0;
    }
    if (us_per_tick > limit) {
        return 0;
    }
    return 1;
}
