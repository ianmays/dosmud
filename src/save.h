#ifndef SAVE_H
#define SAVE_H

#include "base.h"

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
