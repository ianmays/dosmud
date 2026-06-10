/*
 * Windows console platform glue for the cross-compiled WSL -> Windows path.
 * This mirrors DOS line editing closely enough to preserve prompt semantics
 * without pulling platform details into gameplay code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include "config.h"
#include "platform.h"

/* libc rand draws since plat_seed_rng; persisted with save files. */
static u32 g_rand_draw_count = 0;

int plat_poll_line(char *out_line, int out_size)
{
    static char buf[CFG_INPUT_MAX];
    static int len = 0;
    int c;

    if (!_kbhit()) {
        Sleep(100);
        return 0;
    }

    c = _getch();
    if (c == '\r' || c == '\n') {
        putchar('\n');
        buf[len] = '\0';
        strncpy(out_line, buf, (unsigned int)out_size - 1U);
        out_line[out_size - 1] = '\0';
        len = 0;
        return 1;
    }
    if (c == 8 || c == 127) {
        if (len > 0) {
            len -= 1;
            printf("\b \b");
            fflush(stdout);
        }
        return 0;
    }
    if (c == 0 || c == 224) {
        (void)_getch();
        return 0;
    }
    if (c >= 32 && c <= 126) {
        if (len < (CFG_INPUT_MAX - 1)) {
            buf[len] = (char)c;
            len += 1;
            putchar(c);
            fflush(stdout);
        }
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
