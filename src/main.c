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

int main(int argc, char **argv)
{
    static struct GameState game;
    char line[CFG_INPUT_MAX];
    time_t last_tick_time;
    time_t now_time;
    int poll_rc;
    int ran_tick;
    u32 rng_seed;
#ifdef TEST_MODE
    int th_rc;
#endif
    const time_t idle_tick_seconds = (time_t)CFG_MAIN_IDLE_TICK_SECONDS;

    rng_seed = default_rng_seed();
    if (parse_cli_seed(argc, argv, &rng_seed) != 0) {
        fprintf(stderr, "%s\n", TXT_MAIN_USAGE);
        return 1;
    }
    plat_seed_rng(rng_seed);

#ifdef TEST_MODE
    printf("%s\n", TXT_MAIN_TEST_MODE);
#endif

    game_init(&game, rng_seed);
    last_tick_time = plat_time_now();

    printf(TXT_MAIN_TITLE_SEED_FMT, TXT_MAIN_TITLE, (unsigned long)rng_seed);
    printf("%s\n", TXT_MAIN_HELP_HINT);
    game_describe_current_room(&game);
    game_render(&game);
    print_prompt();

    while (game.running) {
        poll_rc = plat_poll_line(line, sizeof(line));
        if (poll_rc < 0) {
            break;
        }
        if (poll_rc > 0) {
            line[strcspn(line, "\r\n")] = '\0';
            if (line[0] != '\0') {
#ifdef TEST_MODE
                th_rc = testharn_apply(&game, line);
                if (th_rc < 0) {
                    fprintf(stderr, "unknown test fixture\n");
                    return 1;
                }
                if (th_rc > 0) {
                    last_tick_time = plat_time_now();
                    if (game.running) {
                        game_render(&game);
                    }
                } else {
                    game_process_input(&game, line);
                    last_tick_time = plat_time_now();
                    if (game.running) {
                        game_render(&game);
                    }
                }
#else
                game_process_input(&game, line);
                last_tick_time = plat_time_now();
                if (game.running) {
                    game_render(&game);
                }
#endif
            }
            if (game.running) {
                print_prompt();
            }
            continue;
        }

        now_time = plat_time_now();
        ran_tick = 0;
        while ((now_time - last_tick_time) >= idle_tick_seconds && game.running) {
            if (game.mode != GAME_MODE_EXPLORE) {
                last_tick_time = now_time;
                break;
            }
            game_background_step(&game);
            last_tick_time += idle_tick_seconds;
            ran_tick = 1;
        }
        if (ran_tick && game.running) {
            game_render(&game);
            print_prompt();
        }
    }

    printf("%s\n", TXT_MAIN_BYE);
    return 0;
}
