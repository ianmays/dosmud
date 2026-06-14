#ifndef CONFIG_H
#define CONFIG_H

#define CFG_INPUT_MAX 80
#define CFG_WORD_MAX  16

#define CFG_ROOM_MAX  16
#define CFG_NAME_MAX  24
#define CFG_DESC_MAX  128

#define CFG_DIR_MAX   4

#define CFG_BAG_MAX   12
#define CFG_GAME_EVENT_MAX 64
/* Max chars for fmt_inv_bag_items output (worst case full bag with stack suffixes). */
#define CFG_FMT_BAG_LIST_MAX 128
/* Max chars for fmt_room_ground_items (header + CFG_AREA_ITEM_SLOTS lines). */
#define CFG_FMT_GROUND_MAX 192
/* Max chars for fmt_exploration_map (header + grid + legend).
 * Theoretical worst case (CFG_ROOM_MAX 16, max collision span per axis):
 * header+legend ~90 + 32*(2*32-1+1) = 2146 - not the configured cap.
 * CFG_FMT_MAP_MAX is the stack buffer size; overflow prints TXT_MAP_TOO_LARGE.
 * 384 covers observed harness/procedural graphs; player-centered viewport in #145. */
#define CFG_FMT_MAP_MAX 384
/* How many objects can sit on the ground in one room (drops, forage, ambient). */
#define CFG_AREA_ITEM_SLOTS 4

/* Uniform roll range for d100-style probability checks (0 .. RANGE-1). */
#define CFG_ROLL_PERCENT_RANGE 100

#ifdef TEST_MODE
/* Injected roll queue for deterministic combat snapshots (see game_roll_inject_*). */
#define CFG_ROLL_INJECT_MAX 8
/* Fixed world graphs: tests/harness/th_world.c (see @fixture world_boot). */
/* equipment bandit_combat_turn1_resolve: player hit spread, enemy damage spread */
#define CFG_TEST_EQUIPMENT_ROLL_PLAYER_HIT 2
#define CFG_TEST_EQUIPMENT_ROLL_ENEMY_DMG 3
/* bandit intimidate: percent roll must be < CFG_BANDIT_INTIMIDATE_SUCCESS_BELOW to pass */
#define CFG_TEST_INTIMIDATE_OK 10
#define CFG_TEST_INTIMIDATE_FAIL 70
/* combat_start enemy HP spread; defend/salve enemy damage spread */
#define CFG_TEST_FIGHT_ENEMY_HP_SPREAD 0
#define CFG_TEST_COMBAT_DEFEND_ENEMY_DMG 0
#define CFG_TEST_COMBAT_SALVE_ENEMY_DMG 0
/* victory: player hit spread, corpse loot percent, kill XP spread */
#define CFG_TEST_VICTORY_HIT_SPREAD 0
#define CFG_TEST_VICTORY_LOOT_SPEAR 10
#define CFG_TEST_VICTORY_LOOT_STICK 30
#define CFG_TEST_VICTORY_LOOT_BERRY 50
#define CFG_TEST_VICTORY_LOOT_HERB 70
#define CFG_TEST_VICTORY_LOOT_FISH 90
#define CFG_TEST_VICTORY_XP_SPREAD 0
/* soak / stress (tests/soak): long-run tick and combat loop counts */
#define CFG_TEST_SOAK_TICKS 10000
#define CFG_TEST_SOAK_COMBAT_ROUNDS 200
#define CFG_TEST_SOAK_CHECK_INTERVAL 1000
/* combat loop is shorter than tick soaks; must be <= CFG_TEST_SOAK_COMBAT_ROUNDS */
#define CFG_TEST_SOAK_COMBAT_CHECK_INTERVAL 50
#define CFG_TEST_SOAK_LIMIT_BACKGROUND_TICKS 50000
#define CFG_TEST_SOAK_LIMIT_COMMAND_TICKS 50000
#define CFG_TEST_SOAK_LIMIT_COMBAT_ROUNDS 200000
/* Default sidecar path when TEST_MODE replay logging is enabled without a path. */
#define CFG_TEST_REPLAY_LOG_DEFAULT "replay.log"
#endif

/* --- Gameplay tuning --- */

/* Progression */
#define CFG_XP_LEVEL_BASE 20
#define CFG_XP_LEVEL_PER_LEVEL 15
#define CFG_LEVELUP_MAX_HP_DELTA 4
#define CFG_LEVELUP_DAMAGE_BONUS_DELTA 1
#define CFG_LEVELUP_BAG_CAPACITY_DELTA 1

/* Starting player */
#define CFG_START_BAG_CAPACITY 5
#define CFG_START_MAX_HP 20
#define CFG_START_LEVEL 1
#define CFG_START_DAMAGE_BONUS 0
#define CFG_START_XP 0

/* Combat */
#define CFG_COMBAT_ENEMY_HP_BASE 8
#define CFG_COMBAT_ENEMY_HP_SPREAD 5
#define CFG_COMBAT_ENEMY_DMG_BASE 1
#define CFG_COMBAT_ENEMY_DMG_SPREAD 4
#define CFG_COMBAT_DEFEND_DAMAGE_REDUCTION 2
#define CFG_COMBAT_PLAYER_HIT_BASE 2
#define CFG_COMBAT_PLAYER_HIT_SPREAD 4
#define CFG_COMBAT_KILL_XP_BASE 12
#define CFG_COMBAT_KILL_XP_SPREAD 5

/* Item effects (used in combat and inventory) */
#define CFG_BERRY_HEAL_AMOUNT 1
#define CFG_FISH_HEAL_AMOUNT 2
#define CFG_SALVE_HEAL_AMOUNT 5
/* Added to attack damage when wielding a melee weapon (reply 1 in combat). */
#define CFG_WEAPON_STICK_DAMAGE_BONUS 1
#define CFG_WEAPON_SPEAR_DAMAGE_BONUS 3

/*
 * Bandit corpse loot: portable items an ambusher might carry (weapon, rations,
 * small forage). Same d100 cumulative threshold scheme as room spawns; fish is
 * the remainder bucket. Heavy ambient junk like loose stone is not rolled here.
 */
#define CFG_COMBAT_CORPSE_LOOT_SPEAR_BELOW 26
#define CFG_COMBAT_CORPSE_LOOT_STICK_BELOW 46
#define CFG_COMBAT_CORPSE_LOOT_BERRY_BELOW 66
#define CFG_COMBAT_CORPSE_LOOT_HERB_BELOW 86

/* Bandit / ambient encounters */
#define CFG_BANDIT_INTIMIDATE_SUCCESS_BELOW 60
#define CFG_BANDIT_ENCOUNTER_CHANCE_BELOW 14

/* Room item spawn (tick hook) */
#define CFG_ROOM_ITEM_SPAWN_GATE 20
/*
 * Cumulative thresholds for maybe_spawn_room_item: must be strictly
 * increasing, each < CFG_ROLL_PERCENT_RANGE; fish is the remainder bucket.
 */
#define CFG_ROOM_SPAWN_ROLL_BERRY_BELOW 25
#define CFG_ROOM_SPAWN_ROLL_STICK_BELOW 45
#define CFG_ROOM_SPAWN_ROLL_REED_BELOW 65
#define CFG_ROOM_SPAWN_ROLL_STONE_BELOW 80
#define CFG_ROOM_SPAWN_ROLL_HERB_BELOW 92

/* Atmosphere & ambient */
#define CFG_ANIMAL_NOISE_TICK_PERIOD 2
#define CFG_ANIMAL_NOISE_SKIP_ROLL_GE 75
#define CFG_ATMOSPHERE_ROLL_GUST_BELOW 35
#define CFG_ATMOSPHERE_ROLL_RUSTLE_BELOW 55
#define CFG_ATMOSPHERE_ROLL_CREAK_BELOW 70
#define CFG_ATMOSPHERE_ROLL_WATER_BELOW 82
#define CFG_ATMOSPHERE_ROLL_GRIT_BELOW 92
#define CFG_ENV_FOCUS_DURATION_TICKS 3
#define CFG_ATMOSPHERE_FOCUS_EXTRA_ITEM_BELOW 50

/* Dynamic NPC roster cap (save payload and per-tick roaming walks). */
#define CFG_NPC_MAX 4

/* --- Main loop and test harness --- */

#define CFG_MAIN_IDLE_TICK_SECONDS 20

/*
 * Default libc RNG seed when building with -DTEST_MODE (see Makefile).
 * Override at runtime with: dosmud --seed <unsigned>
 */
#define CFG_TEST_RAND_SEED 1234UL
/* Maximum --seed value (fits in u32 on all supported targets). */
#define CFG_SEED_CLI_MAX 4294967295UL
/*
 * Persisted RNG draw count ceiling for save/load. Keeps corrupt saves from
 * forcing a huge plat_rand_advance() replay loop on small DOS machines while
 * leaving long normal sessions headroom.
 */
#define CFG_SAVE_RNG_DRAW_MAX 1000000UL

/* --- World generation --- */

#define CFG_WORLD_WILDS_COUNT 7
#define CFG_WORLD_RUINS_COUNT 5
/*
 * Must match world_init: count of wilds[] entries placed on base_path before
 * the wild-branch loop (currently two: wilds[0], wilds[1]).
 */
#define CFG_WORLD_WILD_BRANCH_START_INDEX 2
/*
 * Must match world_init: count of ruins[] entries on the spine before ruin
 * branches (currently three: ruins[0]..ruins[2] on base_path).
 */
#define CFG_WORLD_RUIN_BRANCH_START_INDEX 3
/*
 * Spine ruins used as anchors for rand() % pool in the ruin-branch loop;
 * must match the ruin count on the spine above (currently 3).
 */
#define CFG_WORLD_RUIN_ANCHOR_POOL 3

#define WORLD_ROOM_CAMP 0
#define WORLD_ROOM_ROAD 1
#define WORLD_ROOM_POND 2
#define WORLD_ROOM_FOREST 3
#define WORLD_ROOM_RUINS 4
#define WORLD_ROOM_STREAM 5
#define WORLD_ROOM_CLIFF 6
#define WORLD_ROOM_MARSH 7
#define WORLD_ROOM_GROVE 8
#define WORLD_ROOM_BRIDGE 9
#define WORLD_ROOM_CATACOMBS 10
#define WORLD_ROOM_MEADOW 11
#define WORLD_ROOM_CANYON 12
#define WORLD_ROOM_TOWER 13
#define WORLD_ROOM_ORCHARD 14
#define WORLD_ROOM_CAVE 15

#endif
