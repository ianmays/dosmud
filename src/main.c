#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "config.h"
#include "buildid.h"
#include "game.h"
#include "grendr.h"
#include "gatmos.h"
#include "gwhok.h"
#include "invent.h"
#include "platform.h"
#include "replay.h"
#include "save.h"
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
/* Save/load staging copy stays static so DOS load does not exhaust the stack. */
static struct GameState g_main_loaded_game;
#ifdef TEST_MODE
/* Optional sidecar log; static like g_main_out so the shell loop stays stack-light. */
static ReplayLog g_replay_log;
static int main_check_output_overflow(void);
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
 * Parse optional CLI args. Updates *out_seed, *out_print_version, and
 * *out_replay_path when present.
 * Returns 0 on success, -1 on invalid or unknown arguments.
 */
static int parse_cli_args(int argc, char **argv, u32 *out_seed,
                          const char **out_replay_path, int *out_print_version)
{
    int i;
    int have_seed;
#ifdef TEST_MODE
    int have_replay_path;
#endif

    have_seed = 0;
    *out_print_version = 0;
#ifdef TEST_MODE
    have_replay_path = 0;
#endif
    if (out_replay_path != 0) {
        *out_replay_path = 0;
    }
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
        } else if (strcmp(argv[i], "--version") == 0) {
            *out_print_version = 1;
        } else if (strcmp(argv[i], "--replay-log") == 0) {
#ifdef TEST_MODE
            if (have_replay_path) {
                return -1;
            }
            if (out_replay_path == 0) {
                return -1;
            }
            *out_replay_path = CFG_TEST_REPLAY_LOG_DEFAULT;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                *out_replay_path = argv[i + 1];
                if ((*out_replay_path)[0] == '\0') {
                    return -1;
                }
                ++i;
            }
            have_replay_path = 1;
#else
            return -1;
#endif
        } else {
            return -1;
        }
    }
    return 0;
}

static int main_parse_args(int argc, char **argv, u32 *out_seed,
                           const char **out_replay_path, int *out_print_version)
{
    *out_seed = default_rng_seed();
    if (parse_cli_args(argc, argv, out_seed, out_replay_path,
            out_print_version) != 0) {
        fprintf(stderr, "%s\n", TXT_MAIN_USAGE);
        return 1;
    }
    return 0;
}

static int main_capture_replay(int step_kind, const char *input,
                               struct GameState *game)
{
#ifdef TEST_MODE
    /* Capture at the shell boundary before the next queue reset drops the step. */
    if (!replay_log_capture(&g_replay_log, step_kind, input, game, &g_main_out)) {
        fprintf(stderr, "replay log write failed: %s\n",
            g_replay_log.path != 0 ? g_replay_log.path : "(unknown)");
        return 1;
    }
#else
    (void)step_kind;
    (void)input;
    (void)game;
#endif
    return 0;
}

static int main_startup(struct GameState *game, u32 rng_seed)
{
    game_init(game, rng_seed);
    game_event_queue_reset(&g_main_out);
    printf(TXT_MAIN_TITLE_SEED_FMT, TXT_MAIN_TITLE, (unsigned long)rng_seed);
    printf("%s\n", TXT_MAIN_HELP_HINT);
    game_describe_current_room(game, &g_main_out);
    if (main_capture_replay(REPLAY_STEP_STARTUP, 0, game) != 0) {
        return 1;
    }
    game_render_output(game, &g_main_out);
    game_render(game);
    print_prompt();
    return 0;
}

static void main_render_and_prompt(struct GameState *game)
{
    if (game->running) {
        game_render(game);
        print_prompt();
    }
}

/* Queue the restored view so load can log before render drains it. */
static void main_queue_loaded_game(struct GameState *game)
{
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_LOOT &&
            game_corpse_queue_view(game, game->player.room_id, &g_main_out)) {
        return;
    }
    game_event_queue_reset(&g_main_out);
    game_describe_current_room(game, &g_main_out);
    gatmos_queue_restored_menu(game, &g_main_out);
}

/*
 * save/load are handled at the shell boundary: file I/O and RNG restore live
 * here; commands do not advance tick or route through game_process_input.
 */
static int main_handle_save_load(struct GameState *game, struct Command *cmd,
                                 int *out_rendered)
{
    int rc;
    u32 rng_draw_count;

    *out_rendered = 0;
    if (cmd->type == CMD_SAVE) {
        rc = save_write_game(SAVE_PATH_DEFAULT, game, plat_rand_draw_count());
        if (rc == SAVE_RESULT_OK) {
            printf(TXT_SAVE_OK_FMT, SAVE_PATH_DEFAULT);
        } else {
            printf(TXT_SAVE_IO_FMT, SAVE_PATH_DEFAULT);
        }
        return 0;
    }
    if (cmd->type != CMD_LOAD) {
        return 0;
    }

    rc = save_read_game(SAVE_PATH_DEFAULT, &g_main_loaded_game,
                        &rng_draw_count);
    if (rc == SAVE_RESULT_OK) {
        *game = g_main_loaded_game;
        /* save v14+ stores flags only; reconcile room desc before gameplay. */
        gwhok_apply_all(game);
        /* Restore libc stream position for the loaded seed before new rolls. */
        plat_seed_rng(game->seed);
        plat_rand_advance(rng_draw_count);
        printf(TXT_LOAD_OK_FMT, SAVE_PATH_DEFAULT);
        main_queue_loaded_game(game);
    } else if (rc == SAVE_RESULT_IO) {
        printf(TXT_LOAD_IO_FMT, SAVE_PATH_DEFAULT);
    } else if (rc == SAVE_RESULT_RANGE) {
        printf("%s", TXT_LOAD_BAD_RANGE);
    } else {
        printf("%s", TXT_LOAD_BAD_FORMAT);
    }
    return 0;
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
static int main_dispatch_line(struct GameState *game, char *line,
                              int *out_rendered)
{
#ifdef TEST_MODE
    int th_rc;
#endif
    struct Command cmd;
    int parsed;

    *out_rendered = 0;

    game_event_queue_reset(&g_main_out);

#ifdef TEST_MODE
    th_rc = testharn_apply(game, line);
    if (th_rc < 0) {
        main_report_testharn_error(th_rc);
        return 1;
    }
    if (th_rc == 0) {
        parsed = command_parse(line, &cmd);
        /* Intercept before game_process_input so save/load never advance time. */
        if (parsed && (cmd.type == CMD_SAVE || cmd.type == CMD_LOAD)) {
            if (main_handle_save_load(game, &cmd, out_rendered) != 0) {
                return 1;
            }
            if (main_capture_replay(REPLAY_STEP_INPUT, line, game) != 0) {
                return 1;
            }
            if (cmd.type == CMD_LOAD) {
                game_render_output(game, &g_main_out);
                if (main_check_output_overflow() != 0) {
                    return 1;
                }
            }
            return 0;
        }
        game_process_input(game, line, &g_main_out);
        if (main_capture_replay(REPLAY_STEP_INPUT, line, game) != 0) {
            return 1;
        }
        game_render_output(game, &g_main_out);
        if (main_check_output_overflow() != 0) {
            return 1;
        }
    } else {
        /* Harness @fixture/@seed lines adjust state only; they are not replay steps. */
        plat_seed_rng(game->seed);
    }
#else
    parsed = command_parse(line, &cmd);
    /* Intercept before game_process_input so save/load never advance time. */
    if (parsed && (cmd.type == CMD_SAVE || cmd.type == CMD_LOAD)) {
        if (main_handle_save_load(game, &cmd, out_rendered) != 0) {
            return 1;
        }
        if (main_capture_replay(REPLAY_STEP_INPUT, line, game) != 0) {
            return 1;
        }
        if (cmd.type == CMD_LOAD) {
            game_render_output(game, &g_main_out);
        }
        return 0;
    }
    game_process_input(game, line, &g_main_out);
    if (main_capture_replay(REPLAY_STEP_INPUT, line, game) != 0) {
        return 1;
    }
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
    int rendered;

    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] != '\0') {
        if (main_dispatch_line(game, line, &rendered) != 0) {
            return 1;
        }
        *last_tick_time = plat_time_now();
        /* load already rendered via main_render_loaded_game. */
        if (game->running && !rendered) {
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
        if (game_is_busy_dialogue(game)) {
            *last_tick_time = now_time;
            break;
        }
        game_event_queue_reset(&g_main_out);
        game_background_step(game, &g_main_out);
        if (main_capture_replay(REPLAY_STEP_IDLE, 0, game) != 0) {
            return -1;
        }
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
    int print_version;
    int poll_rc;
#ifdef TEST_MODE
    const char *replay_path;
    replay_log_reset(&g_replay_log);
#endif
#ifdef TEST_MODE
    if (main_parse_args(argc, argv, &rng_seed, &replay_path,
            &print_version) != 0) {
#else
    if (main_parse_args(argc, argv, &rng_seed, 0, &print_version) != 0) {
#endif
        return 1;
    }
    if (print_version) {
        /* shell-only early exit; in-game version uses GAME_EVENT_VERSION. */
        printf("%s\n", build_version_line());
        return 0;
    }
#ifdef TEST_MODE
    if (replay_path != 0 && !replay_log_open(&g_replay_log, replay_path, rng_seed)) {
        fprintf(stderr, "cannot open replay log: %s\n", replay_path);
        return 1;
    }
#endif
    plat_seed_rng(rng_seed);

#ifdef TEST_MODE
    printf("%s\n", TXT_MAIN_TEST_MODE);
#endif

    if (main_startup(&game, rng_seed) != 0) {
#ifdef TEST_MODE
        replay_log_close(&g_replay_log);
#endif
        return 1;
    }
    last_tick_time = plat_time_now();

    while (game.running) {
        poll_rc = plat_poll_line(line, sizeof(line));
        if (poll_rc < 0) {
            break;
        }
        if (poll_rc > 0) {
            if (main_handle_polled_line(&game, line, &last_tick_time) != 0) {
#ifdef TEST_MODE
                replay_log_close(&g_replay_log);
#endif
                return 1;
            }
            continue;
        }
        poll_rc = main_run_idle_ticks(&game, &last_tick_time,
                                      (time_t)CFG_MAIN_IDLE_TICK_SECONDS);
        if (poll_rc < 0) {
#ifdef TEST_MODE
            replay_log_close(&g_replay_log);
#endif
            return 1;
        }
        if (poll_rc > 0) {
            main_render_and_prompt(&game);
        }
    }

#ifdef TEST_MODE
    replay_log_close(&g_replay_log);
#endif
    printf("%s\n", TXT_MAIN_BYE);
    return 0;
}
