#ifndef SOAK_UTIL_H
#define SOAK_UTIL_H

#include <time.h>
#include "game.h"

int soak_assert_game_state_ok(const struct GameState *game);
unsigned long soak_print_bench(const char *name, unsigned long ticks, clock_t elapsed);
int soak_check_limit(const char *name, unsigned long us_per_tick);

#endif
