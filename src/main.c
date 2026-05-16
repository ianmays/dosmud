#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "game.h"
#include "grendr.h"
#include "txtres.h"

#ifdef __WATCOMC__
#include <conio.h>
#include <dos.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

static void print_prompt(void)
{
    printf("%s", TXT_MAIN_PROMPT);
    fflush(stdout);
}

static int poll_line_nonblocking(char *out_line, int out_size)
{
#ifdef __WATCOMC__
    static char buf[CFG_INPUT_MAX];
    static int len = 0;
    int c;

    if (!kbhit()) {
        delay(100);
        return 0;
    }

    c = getch();
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
    if (c >= 32 && c <= 126) {
        if (len < (CFG_INPUT_MAX - 1)) {
            buf[len] = (char)c;
            len += 1;
            putchar(c);
            fflush(stdout);
        }
    }
    return 0;
#else
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
#endif
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
        srand(CFG_TEST_RAND_SEED);
    #else
        srand(time(NULL));
    #endif
    
    game_init(&game);
    last_tick_time = time(NULL);

    printf("%s\n", TXT_MAIN_TITLE);
    printf("%s\n", TXT_MAIN_HELP_HINT);
    game_describe_current_room(&game);
    game_render(&game);
    print_prompt();

    while (game.running) {
        poll_rc = poll_line_nonblocking(line, sizeof(line));
        if (poll_rc < 0) {
            break;
        }
        if (poll_rc > 0) {
            line[strcspn(line, "\r\n")] = '\0';
            if (line[0] != '\0') {
                game_process_input(&game, line);
                last_tick_time = time(NULL);
                if (game.running) {
                    game_render(&game);
                }
            }
            if (game.running) {
                print_prompt();
            }
            continue;
        }

        now_time = time(NULL);
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
