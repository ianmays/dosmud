#ifndef UNIT_UTIL_H
#define UNIT_UTIL_H

#include "config.h"
#include "game.h"
#include "platform.h"

/* Apply seed-1234 harness world graph (same tables as testharn world_boot). */
void unit_world_boot_graph(struct GameState *game);

/* game_init + world_boot graph; always reseeds libc RNG. */
void unit_game_fresh(struct GameState *game, u32 seed);

/* Redirect stdout to a temp file for render output checks (Linux unit runner). */
int unit_capture_stdout_begin(void);
int unit_capture_stdout_end(char *buf, int bufsize);

#endif
