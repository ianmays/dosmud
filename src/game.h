#ifndef GAME_H
#define GAME_H

#include "command.h"
#include "world.h"

#define CFG_BAG_MAX 5

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
    int env_focus_active;
    int env_focus_room;
    int env_focus_kind;
    unsigned long env_focus_expires_tick;
    int room_item[CFG_ROOM_MAX];
    int bag[CFG_BAG_MAX];
    int bag_count;
    int player_hp;
    int enemy_dialogue;
    int combat_active;
    int enemy_hp;
    int combat_defending;
};

void game_init(struct GameState *game);
void game_render(const struct GameState *game);
void game_print_help(void);
void game_print_location_art(int room_id);
void game_describe_current_room(struct GameState *game);
int game_process_input(struct GameState *game, char *line);
void game_background_step(struct GameState *game);

#endif
