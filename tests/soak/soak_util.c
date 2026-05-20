#include <stdio.h>
#include <string.h>
#include "soak_util.h"
#include "config.h"
#include "game.h"

#define SOAK_LIMITS_PATH "tests/benchmarks/soak_limits.txt"
#define SOAK_LIMITS_LINE_MAX 128

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
                game->dialogue > DIALOGUE_ENEMY) {
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

    if (ticks == 0) {
        ticks = 1;
    }
    us = (unsigned long)((elapsed * 1000000UL) / (clock_t)CLOCKS_PER_SEC);
    us_per_tick = us / ticks;
    printf("SOAK_BENCH %s ticks=%lu us_per_tick=%lu\n",
        name, ticks, us_per_tick);
    return us_per_tick;
}

int soak_check_limit(const char *name, unsigned long us_per_tick)
{
    FILE *fp;
    char line[SOAK_LIMITS_LINE_MAX];
    char scenario[64];
    unsigned long limit;
    int found;

    fp = fopen(SOAK_LIMITS_PATH, "r");
    if (fp == NULL) {
        return 0;
    }
    found = 0;
    while (fgets(line, SOAK_LIMITS_LINE_MAX, fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        if (sscanf(line, "%63s %lu", scenario, &limit) != 2) {
            continue;
        }
        if (strcmp(scenario, name) == 0) {
            found = 1;
            if (us_per_tick > limit) {
                fclose(fp);
                return 0;
            }
            break;
        }
    }
    fclose(fp);
    return found;
}
