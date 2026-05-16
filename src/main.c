#include <stdio.h>
#include <string.h>
#include "config.h"
#include "game.h"
#include "grendr.h"
#include "platform.h"
#include "txtres.h"

static void print_prompt(void)
{
    printf("%s", TXT_MAIN_PROMPT);
    fflush(stdout);
}

int main(void)
{
    static struct GameState game;
    char line[CFG_INPUT_MAX];
    time_t last_tick_time;
    time_t now_time;
    int poll_rc;
    int ran_tick;
    const time_t idle_tick_seconds = (time_t)CFG_MAIN_IDLE_TICK_SECONDS;

#ifdef TEST_MODE
    printf("%s\n", TXT_MAIN_TEST_MODE);
#endif
    plat_seed_rng();

    game_init(&game);
    last_tick_time = plat_time_now();

    printf("%s\n", TXT_MAIN_TITLE);
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
                game_process_input(&game, line);
                last_tick_time = plat_time_now();
                if (game.running) {
                    game_render(&game);
                }
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
