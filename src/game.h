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
    int wanderer_room;
    int wanderer_dialogue;
    int wanderer_need_separation;
};

void game_init(struct GameState *game);
void game_render(const struct GameState *game);
void game_print_help(void);
void game_print_location_art(int room_id);
void game_describe_current_room(struct GameState *game);
int game_process_input(struct GameState *game, char *line);
void game_background_step(struct GameState *game);

#endif
