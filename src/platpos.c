/*
 * POSIX platform glue for the GCC/Linux/WSL path. This mirrors the DOS
 * implementation without leaking platform details into core gameplay.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/select.h>
#include <unistd.h>
#include "config.h"
#include "platform.h"

/* libc rand draws since plat_seed_rng; persisted with save files. */
static u32 g_rand_draw_count = 0;

int plat_poll_line(char *out_line, int out_size)
{
    fd_set set;
    struct timeval tv;
    int rc;

    FD_ZERO(&set);
    FD_SET(0, &set);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    rc = select(1, &set, NULL, NULL, &tv);
    if (rc > 0 && FD_ISSET(0, &set)) {
        if (fgets(out_line, out_size, stdin) == NULL) {
            return -1;
        }
        return 1;
    }
    return 0;
}

time_t plat_time_now(void)
{
    return time(NULL);
}

void plat_seed_rng(u32 seed)
{
    srand((unsigned int)seed);
    g_rand_draw_count = 0;
}

int plat_rand(void)
{
    ++g_rand_draw_count;
    return rand();
}

u32 plat_rand_draw_count(void)
{
    return g_rand_draw_count;
}

void plat_rand_advance(u32 draws)
{
    u32 i;

    for (i = 0; i < draws; ++i) {
        (void)plat_rand();
    }
}
