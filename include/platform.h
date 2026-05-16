#ifndef PLATFORM_H
#define PLATFORM_H

#include <time.h>

/* Poll stdin without blocking. Returns 1 line ready, 0 none, -1 EOF/error. */
int plat_poll_line(char *out_line, int out_size);

time_t plat_time_now(void);

void plat_seed_rng(void);

#endif /* PLATFORM_H */
