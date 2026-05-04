#ifndef GAME_H
#define GAME_H

#include "command.h"
#include "world.h"

struct Player {
    int room_id;
};

struct GameState {
    struct World world;
    struct Player player;
    unsigned long tick;
    unsigned long seed;
    int running;
    int pond_dialogue;
};

void game_init(struct GameState *game);
void game_render(const struct GameState *game);
void game_print_help(void);
int game_process_input(struct GameState *game, char *line);

#endif
