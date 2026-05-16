#ifndef PLATFORM_H
#define PLATFORM_H

#include <time.h>
#include "base.h"

/* Poll stdin without blocking. Returns 1 line ready, 0 none, -1 EOF/error. */
int plat_poll_line(char *out_line, int out_size);

time_t plat_time_now(void);

/* Applies seed to libc rand(); may use fewer bits than u32 on some targets. */
void plat_seed_rng(u32 seed);

#endif /* PLATFORM_H */
