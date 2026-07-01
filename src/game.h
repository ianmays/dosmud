#ifndef GAME_H
#define GAME_H

#include "base.h"
#include "command.h"
#include "gout.h"
#include "world.h"
#include "gprog.h"

/*
 * game.h defines the orchestration-layer state: GameState owns the persistent
 * simulation snapshot that command handling, encounters, rendering, and tests
 * all read or mutate through explicit subsystem APIs.
 * See docs/architecture.md for engine vs game-logic ownership in core.
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
    DIALOGUE_TRAVELER,
    DIALOGUE_ENEMY,
    DIALOGUE_LOOT
};

/* Persisted progress for npc.c herbalist branch (#76); not a generic quest layer. */
enum HerbalistStoryState {
    HERBALIST_STORY_NONE = 0,
    HERBALIST_STORY_REQUESTED,
    HERBALIST_STORY_COMPLETE
};

/*
 * Persisted bit flags for npc.c watchman branch (#8); composable, not exclusive.
 * WARNED and PROMISED are recorded for save/re-talk copy hooks; only FED gates
 * meal handover today.
 */
#define WATCHMAN_FLAG_WARNED 1
#define WATCHMAN_FLAG_FED 2
#define WATCHMAN_FLAG_PROMISED 4

/* Stored in GameState.env_focus_kind; render consumes the same values. */
#define GAME_ENV_NONE 0
#define GAME_ENV_RUSTLE 1
#define GAME_ENV_CREAK 2
#define GAME_ENV_WATER 3
#define GAME_ENV_GRIT 4

struct CombatState {
    int enemy_hp;
    /* Active combat snapshot: copied from the encounter owner for save/load stability. */
    int enemy_level;
    int defending;
};

/*
 * Dynamic NPC roster lives in GameState; npc.c owns spawn, roaming, and flags.
 * Vacant slots use actor GAME_DIALOGUE_ACTOR_NONE; inactive respawn profiles
 * keep actor/dialogue/encounter in place until npc_roaming_activate_due runs.
 * HANDOVER_PICK gates bandit give-during-dialogue on the active enemy slot.
 */
enum NpcFlags {
    NPC_FLAG_ACTIVE = 1,
    NPC_FLAG_ROAMING = 2,
    NPC_FLAG_NEEDS_SEPARATION = 4,
    NPC_FLAG_RESPAWNS = 8,
    NPC_FLAG_HANDOVER_PICK = 16
};

struct NpcState {
    int actor;
    int dialogue;
    int encounter;
    /* Authored enemy difficulty; zero for non-combatant roster entries. */
    int level;
    int room_id;
    int flags;
    u32 return_tick;
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
    /* NPC roster stores active dynamic instances in deterministic slot order. */
    struct NpcState npcs[CFG_NPC_MAX];
    int env_focus_active;
    int env_focus_room;
    int env_focus_kind;
    u32 env_focus_expires_tick;
    /* npc.c herbalist branch (#76); saved in save v10+. */
    int herbalist_story;
    /* Active herbalist talk menu while DIALOGUE_NPC_HERBALIST; saved v13+. */
    int herbalist_menu;
    /* npc.c watchman branch (#8); saved in save v12+. */
    int watchman_flags;
    /* Active watchman talk menu while DIALOGUE_NPC_WATCHMAN; session seam. */
    int watchman_menu;
    /* Story-owned in-play marker: resets when no marsh-root is reachable. */
    int marsh_root_spawned;
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
    struct CombatState combat;
    int corpse_present[CFG_ROOM_MAX];
    /* Per-room corpse loot; invent owns slot layout and compact-on-take (#129). */
    int corpse_item[CFG_ROOM_MAX][CFG_CORPSE_ITEM_SLOTS];
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
void game_describe_current_room(struct GameState *game, GameEventQueue *out);
int game_process_input(struct GameState *game, char *line, GameEventQueue *out);
void game_background_step(struct GameState *game, GameEventQueue *out);

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
