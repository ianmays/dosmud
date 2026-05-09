#ifndef CONFIG_H
#define CONFIG_H

#define CFG_INPUT_MAX 80
#define CFG_WORD_MAX  16

#define CFG_ROOM_MAX  16
#define CFG_NAME_MAX  24
#define CFG_DESC_MAX  128

#define CFG_DIR_MAX   4

#define CFG_BAG_MAX   12

/* Uniform roll range for d100-style probability checks (0 .. RANGE-1). */
#define CFG_ROLL_PERCENT_RANGE 100

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
/* Stored on GameState; not the libc RNG seed (see main.c / TEST_MODE). */
#define CFG_GAME_INIT_SEED 1UL

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
#define CFG_SALVE_HEAL_AMOUNT 5

/*
 * Corpse loot: rand() % CFG_COMBAT_CORPSE_LOOT_COIN_SIDES picks the branch.
 * Value 2 is a fair two-outcome split; other values change the distribution.
 */
#define CFG_COMBAT_CORPSE_LOOT_COIN_SIDES 2

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

/* Wanderer */
#define CFG_WANDERER_RETURN_DELAY_BASE 8
#define CFG_WANDERER_RETURN_DELAY_SPREAD 16

/* --- Main loop and test harness --- */

#define CFG_MAIN_IDLE_TICK_SECONDS 20

/*
 * libc RNG seed when building with -DTEST_MODE (see Makefile).
 * May become a command-line argument later; do not assume header-only forever.
 */
#define CFG_TEST_RAND_SEED 1234

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
