#ifndef GAME_H
#define GAME_H

#include "base.h"
#include "command.h"
#include "world.h"
#include "gprog.h"

enum GameMode {
    GAME_MODE_EXPLORE = 0,
    GAME_MODE_DIALOGUE,
    GAME_MODE_COMBAT
};

enum DialogueKind {
    DIALOGUE_NONE = 0,
    DIALOGUE_NPC_FROG,
    DIALOGUE_NPC_WATCHMAN,
    DIALOGUE_NPC_HERBALIST,
    DIALOGUE_NPC_ARCHIVIST,
    DIALOGUE_WANDERER,
    DIALOGUE_ENEMY
};

struct CombatState {
    int enemy_hp;
    int defending;
};

struct Player {
    int room_id;
};

struct GameState {
    struct World world;
    struct Player player;
    u32 tick;
    u32 seed;
    int running;
    int mode;
    int dialogue;
    int wanderer_room;
    int wanderer_need_separation;
    int env_focus_active;
    int env_focus_room;
    int env_focus_kind;
    u32 env_focus_expires_tick;
    int room_item[CFG_ROOM_MAX][CFG_AREA_ITEM_SLOTS];
    int bag[CFG_BAG_MAX];
    int bag_count;
    int bag_capacity;
    int level;
    int xp;
    int max_hp;
    int damage_bonus;
    int weapon_equipped;
    int player_hp;
    int enemy_handover_pick;
    struct CombatState combat;
    int corpse_present[CFG_ROOM_MAX];
    int corpse_loot[CFG_ROOM_MAX];
    int wanderer_active;
    u32 wanderer_return_tick;
    u8 room_explored[CFG_ROOM_MAX];
};

void game_init(struct GameState *game);
void game_describe_current_room(struct GameState *game);
int game_process_input(struct GameState *game, char *line);
void game_background_step(struct GameState *game);

void game_set_mode_explore(struct GameState *game);
void game_set_mode_dialogue(struct GameState *game, enum DialogueKind kind);
void game_set_mode_combat(struct GameState *game);

/* True while dialogue or combat blocks ambient encounters. */
int game_is_busy_dialogue(struct GameState *game);

#endif
