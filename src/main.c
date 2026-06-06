#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "config.h"
#include "game.h"
#include "grendr.h"
#include "platform.h"
#include "txtres.h"
#ifdef TEST_MODE
#include "testharn.h"
#endif

/*
 * main.c owns shell-level startup, CLI parsing, polling, and the outer loop
 * that bridges platform I/O to game orchestration.
 */

/*
 * DOS has a small default stack. Keep the fixed-size engine output buffer in
 * static storage so command/tick stepping does not consume stack in nested
 * main-loop frames.
 */
static GameEventQueue g_main_out;

static void print_prompt(void)
{
    printf("%s", TXT_MAIN_PROMPT);
    fflush(stdout);
}

static u32 default_rng_seed(void)
{
#ifdef TEST_MODE
    return (u32)CFG_TEST_RAND_SEED;
#else
    return (u32)plat_time_now();
#endif
}

/*
 * Parse optional --seed <value>. Updates *out_seed when --seed is present and valid.
 * Returns 0 on success, -1 on invalid or unknown arguments.
 */
static int parse_cli_seed(int argc, char **argv, u32 *out_seed)
{
    int i;
    int have_seed;

    have_seed = 0;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--seed") == 0) {
            const char *arg;
            char *end;
            unsigned long val;

            if (have_seed) {
                return -1;
            }
            if (i + 1 >= argc) {
                return -1;
            }
            arg = argv[i + 1];
            if (arg[0] == '-' || arg[0] == '+') {
                return -1;
            }
            errno = 0;
            val = strtoul(arg, &end, 10);
            if (end == arg || *end != '\0' || errno == ERANGE) {
                return -1;
            }
#if ULONG_MAX > CFG_SEED_CLI_MAX
            if (val > (unsigned long)CFG_SEED_CLI_MAX) {
                return -1;
            }
#endif
            *out_seed = (u32)val;
            have_seed = 1;
            ++i;
        } else {
            return -1;
        }
    }
    return 0;
}

static int main_parse_args(int argc, char **argv, u32 *out_seed)
{
    *out_seed = default_rng_seed();
    if (parse_cli_seed(argc, argv, out_seed) != 0) {
        fprintf(stderr, "%s\n", TXT_MAIN_USAGE);
        return 1;
    }
    return 0;
}

static void main_startup(struct GameState *game, u32 rng_seed)
{
    game_init(game, rng_seed);
    game_event_queue_reset(&g_main_out);
    printf(TXT_MAIN_TITLE_SEED_FMT, TXT_MAIN_TITLE, (unsigned long)rng_seed);
    printf("%s\n", TXT_MAIN_HELP_HINT);
    game_describe_current_room(game, &g_main_out);
    game_render_output(game, &g_main_out);
    game_render(game);
    print_prompt();
}

static void main_render_and_prompt(struct GameState *game)
{
    if (game->running) {
        game_render(game);
        print_prompt();
    }
}

#ifdef TEST_MODE
static void main_report_testharn_error(int th_rc)
{
    if (th_rc == -2) {
        fprintf(stderr, "test fixture failed\n");
    } else if (th_rc == -3) {
        fprintf(stderr, "invalid @seed\n");
    } else {
        fprintf(stderr, "unknown test fixture\n");
    }
}

static int main_check_output_overflow(void)
{
    if (!g_main_out.overflowed) {
        return 0;
    }
    fprintf(stderr, "game output overflow\n");
    return 1;
}
#endif

/*
 * Process a non-empty input line. Returns 0 on success, 1 on fatal harness error.
 */
static int main_dispatch_line(struct GameState *game, char *line)
{
#ifdef TEST_MODE
    int th_rc;
#endif

    game_event_queue_reset(&g_main_out);

#ifdef TEST_MODE
    th_rc = testharn_apply(game, line);
    if (th_rc < 0) {
        main_report_testharn_error(th_rc);
        return 1;
    }
    if (th_rc == 0) {
        game_process_input(game, line, &g_main_out);
        game_render_output(game, &g_main_out);
        if (main_check_output_overflow() != 0) {
            return 1;
        }
    } else {
        plat_seed_rng(game->seed);
    }
#else
    game_process_input(game, line, &g_main_out);
    game_render_output(game, &g_main_out);
#endif
    return 0;
}

/*
 * Handle a polled line. Returns 0 on success, 1 on fatal harness error.
 * Render only after a non-empty handled line; always re-prompt when running.
 * Idle ticks use main_render_and_prompt instead (empty Enter must not repaint).
 */
static int main_handle_polled_line(struct GameState *game, char *line, time_t *last_tick_time)
{
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] != '\0') {
        if (main_dispatch_line(game, line) != 0) {
            return 1;
        }
        *last_tick_time = plat_time_now();
        if (game->running) {
            game_render(game);
        }
    }
    if (game->running) {
        print_prompt();
    }
    return 0;
}

/*
 * Run idle background ticks while in explore mode.
 * Returns 1 if any tick ran, 0 if none ran, and -1 on fatal overflow.
 */
static int main_run_idle_ticks(struct GameState *game, time_t *last_tick_time,
                               time_t idle_tick_seconds)
{
    time_t now_time;
    int ran_tick;

    now_time = plat_time_now();
    ran_tick = 0;
    while ((now_time - *last_tick_time) >= idle_tick_seconds && game->running) {
        if (game->mode != GAME_MODE_EXPLORE) {
            *last_tick_time = now_time;
            break;
        }
        game_event_queue_reset(&g_main_out);
        game_background_step(game, &g_main_out);
        game_render_output(game, &g_main_out);
#ifdef TEST_MODE
        if (main_check_output_overflow() != 0) {
            return -1;
        }
#endif
        *last_tick_time += idle_tick_seconds;
        ran_tick = 1;
    }
    return ran_tick;
}

int main(int argc, char **argv)
{
    static struct GameState game;
    char line[CFG_INPUT_MAX];
    time_t last_tick_time;
    u32 rng_seed;
    int poll_rc;

    if (main_parse_args(argc, argv, &rng_seed) != 0) {
        return 1;
    }
    plat_seed_rng(rng_seed);

#ifdef TEST_MODE
    printf("%s\n", TXT_MAIN_TEST_MODE);
#endif

    main_startup(&game, rng_seed);
    last_tick_time = plat_time_now();

    while (game.running) {
        poll_rc = plat_poll_line(line, sizeof(line));
        if (poll_rc < 0) {
            break;
        }
        if (poll_rc > 0) {
            if (main_handle_polled_line(&game, line, &last_tick_time) != 0) {
                return 1;
            }
            continue;
        }
        poll_rc = main_run_idle_ticks(&game, &last_tick_time,
                                      (time_t)CFG_MAIN_IDLE_TICK_SECONDS);
        if (poll_rc < 0) {
            return 1;
        }
        if (poll_rc > 0) {
            main_render_and_prompt(&game);
        }
    }

    printf("%s\n", TXT_MAIN_BYE);
    return 0;
}
