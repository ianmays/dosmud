#include <stdio.h>
#include <string.h>
#include "config.h"
#include "game.h"

int main(void)
{
    struct GameState game;
    char line[CFG_INPUT_MAX];

    game_init(&game);

    printf("dosmud prototype\n");
    printf("Type 'help' for commands.\n");
    game_render(&game);

    while (game.running) {
        printf("\n> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        /* Remove trailing newline for parser simplicity. */
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') {
            continue;
        }

        game_process_input(&game, line);
        if (game.running) {
            game_render(&game);
        }
    }

    printf("bye\n");
    return 0;
}
