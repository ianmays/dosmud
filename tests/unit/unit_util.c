#include "unit_util.h"
#include "th_world.h"
#include "items.h"
#include "world.h"
#include <stdio.h>
#include <unistd.h>

#define UNIT_CAPTURE_PATH "tests/unit/build/render_cap.txt"

static int g_saved_stdout_fd = -1;

void unit_world_boot_graph(struct GameState *game)
{
    harness_world_boot_graph(game);
}

void unit_game_fresh(struct GameState *game, u32 seed)
{
    game_init(game, seed);
    unit_world_boot_graph(game);
    plat_seed_rng(seed);
}

int unit_capture_stdout_begin(void)
{
    fflush(stdout);
    g_saved_stdout_fd = dup(STDOUT_FILENO);
    if (g_saved_stdout_fd < 0) {
        return 0;
    }
    if (freopen(UNIT_CAPTURE_PATH, "w", stdout) == NULL) {
        close(g_saved_stdout_fd);
        g_saved_stdout_fd = -1;
        return 0;
    }
    return 1;
}

int unit_capture_stdout_end(char *buf, int bufsize)
{
    long n;
    FILE *f;

    if (g_saved_stdout_fd < 0) {
        return 0;
    }
    fflush(stdout);
    dup2(g_saved_stdout_fd, STDOUT_FILENO);
    close(g_saved_stdout_fd);
    g_saved_stdout_fd = -1;
    clearerr(stdout);
    f = fopen(UNIT_CAPTURE_PATH, "r");
    if (f == NULL) {
        return 0;
    }
    n = (long)fread(buf, 1, (size_t)(bufsize - 1), f);
    if (n < 0) {
        n = 0;
    }
    buf[n] = '\0';
    fclose(f);
    remove(UNIT_CAPTURE_PATH);
    return 1;
}
