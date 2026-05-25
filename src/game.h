#ifndef GAME_H
#define GAME_H

#include "base.h"
#include "command.h"
#include "world.h"
#include "gprog.h"

/*
 * game.h defines the orchestration-layer state: GameState owns the persistent
 * simulation snapshot that command handling, encounters, rendering, and tests
 * all read or mutate through explicit subsystem APIs.
 */

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
#ifdef TEST_MODE
    int roll_inject_active;
    int roll_queue[CFG_ROLL_INJECT_MAX];
    int roll_queue_len;
    int roll_queue_i;
    int test_quiet_ticks;
#endif
};

void game_init(struct GameState *game, u32 seed);
void game_describe_current_room(struct GameState *game);
int game_process_input(struct GameState *game, char *line);
void game_background_step(struct GameState *game);

void game_set_mode_explore(struct GameState *game);
void game_set_mode_dialogue(struct GameState *game, enum DialogueKind kind);
void game_set_mode_combat(struct GameState *game);

/* True while dialogue or combat blocks ambient encounters. */
int game_is_busy_dialogue(struct GameState *game);

int game_roll_spread(struct GameState *game, int spread);
int game_roll_percent(struct GameState *game);

/* returns 1 if heal applied, 0 if player was already at max_hp */
int game_heal_player(struct GameState *game, int amount);

#ifdef TEST_MODE
void game_roll_inject_begin(struct GameState *game, const int *values, int count);
void game_roll_inject_clear(struct GameState *game);
int game_roll_inject_fully_consumed(const struct GameState *game);
/*
 * Reset all mutable simulation fields to the same values game_init applies
 * (world graph and seed are unchanged). Used by test fixtures for a clean slate.
 */
void game_reset_fixture_baseline(struct GameState *game, int room_id, u32 tick);
#endif

#endif
