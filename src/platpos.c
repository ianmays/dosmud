/* POSIX platform implementation (GCC / Linux / WSL). */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/select.h>
#include <unistd.h>
#include "config.h"
#include "platform.h"

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
}
