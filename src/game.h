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
    int env_focus_active;
    int env_focus_room;
    int env_focus_kind;
    unsigned long env_focus_expires_tick;
    int room_item[CFG_ROOM_MAX][CFG_AREA_ITEM_SLOTS];
    int bag[CFG_BAG_MAX];
    int bag_count;
    int bag_capacity;
    int level;
    int xp;
    int max_hp;
    int damage_bonus;
    int player_hp;
    int enemy_dialogue;
    int enemy_handover_pick;
    int combat_active;
    int enemy_hp;
    int combat_defending;
    int corpse_present[CFG_ROOM_MAX];
    int corpse_loot[CFG_ROOM_MAX];
    int npc_dialogue;
    int wanderer_active;
    unsigned long wanderer_return_tick;
    unsigned char room_explored[CFG_ROOM_MAX];
};

void game_init(struct GameState *game);
void game_describe_current_room(struct GameState *game);
int game_process_input(struct GameState *game, char *line);
void game_background_step(struct GameState *game);

/* Total XP required to complete one level-up from the given level (HUD + rules). */
int game_xp_to_next_level(int level);

#endif
