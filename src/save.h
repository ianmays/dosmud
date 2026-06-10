#ifndef SAVE_H
#define SAVE_H

#include "base.h"

/*
 * Binary persistence for struct GameState. Shell (main.c) supplies
 * plat_rand_draw_count on write and replays it via plat_rand_advance after load;
 * save.c does not seed or advance the platform RNG.
 */

struct GameState;

#define SAVE_PATH_DEFAULT "save.dat"

enum SaveResult {
    SAVE_RESULT_OK = 0,
    SAVE_RESULT_IO,
    SAVE_RESULT_FORMAT,
    SAVE_RESULT_RANGE
};

int save_write_game(const char *path, const struct GameState *game,
                    u32 rng_draw_count);
int save_read_game(const char *path, struct GameState *out_game,
                   u32 *out_rng_draw_count);

#endif
