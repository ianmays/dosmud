#ifndef UNIT_UTIL_H
#define UNIT_UTIL_H

#include "config.h"
#include "game.h"
#include "platform.h"

/* Apply seed-1234 harness world graph (same tables as testharn world_boot). */
void unit_world_boot_graph(struct GameState *game);

/* game_init + optional world_boot graph; always reseeds libc RNG. */
void unit_game_fresh(struct GameState *game, u32 seed);

/* Baseline reset at room/tick on existing game (graph/seed unchanged). */
void unit_game_baseline(struct GameState *game, int room_id, u32 tick);

/* Fill bag to capacity with distinct items. */
void unit_bag_fill(struct GameState *game);

#endif
