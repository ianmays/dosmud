/*
 * Unit tests for save.c round-trip and rejection paths.
 * Verifies plat_rand_draw_count metadata survives save/load.
 */
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "greatest.h"
#include "game.h"
#include "items.h"
#include "npc.h"
#include "platform.h"
#include "save.h"
#include "unit_util.h"

static const char *save_test_path(void)
{
    return "/tmp/dosmud_save_unit.dat";
}

static void save_cleanup_file(void)
{
    remove(save_test_path());
}

static void save_fill_fixture(struct GameState *game)
{
    int slot;

    unit_game_fresh(game, 1234U);
    game->player.room_id = WORLD_ROOM_TOWER;
    game->tick = 77U;
    game->mode = GAME_MODE_DIALOGUE;
    game->dialogue = DIALOGUE_NPC_ARCHIVIST;
    slot = npc_find_by_actor(game, GAME_DIALOGUE_ACTOR_TRAVELER);
    game->npcs[slot].flags |= NPC_FLAG_NEEDS_SEPARATION;
    npc_deactivate_until(game, GAME_DIALOGUE_ACTOR_TRAVELER, 103U);
    (void)npc_spawn(game, GAME_DIALOGUE_ACTOR_NOBODY, DIALOGUE_NONE,
        GAME_ENCOUNTER_NONE, WORLD_ROOM_ROAD, NPC_FLAG_ACTIVE);
    game->env_focus_active = 1;
    game->env_focus_room = WORLD_ROOM_TOWER;
    game->env_focus_kind = GAME_ENV_CREAK;
    game->env_focus_expires_tick = 81U;
    game->herbalist_story = HERBALIST_STORY_REQUESTED;
    game->marsh_root_spawned = 1;
    game->bag[0] = ITEM_STICK;
    game->bag[1] = ITEM_SALVE;
    game->bag[2] = ITEM_SPEAR;
    game->bag[3] = ITEM_MARSH_ROOT;
    game->bag_count = 4;
    game->bag_capacity = 6;
    game->level = 3;
    game->xp = 14;
    game->max_hp = 24;
    game->damage_bonus = 2;
    game->weapon_equipped = ITEM_SPEAR;
    game->player_hp = 19;
    (void)npc_spawn(game, GAME_DIALOGUE_ACTOR_BANDIT, DIALOGUE_ENEMY,
        GAME_ENCOUNTER_BANDIT, WORLD_ROOM_TOWER,
        NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS |
        NPC_FLAG_HANDOVER_PICK);
    game->dialogue = DIALOGUE_ENEMY;
    game->combat.enemy_hp = 5;
    game->combat.enemy_level = 3;
    game->combat.defending = 1;
    game->corpse_present[WORLD_ROOM_ROAD] = 1;
    game->corpse_item[WORLD_ROOM_ROAD][0] = ITEM_HERB;
    game->corpse_item[WORLD_ROOM_ROAD][1] = ITEM_FISH;
    game->corpse_item[WORLD_ROOM_ROAD][2] = ITEM_NONE;
    game->room_explored[WORLD_ROOM_ROAD] = 1;
    game->room_explored[WORLD_ROOM_TOWER] = 1;
    game->room_item[WORLD_ROOM_TOWER][0] = ITEM_FISH;
    game->room_item[WORLD_ROOM_TOWER][1] = ITEM_REED;
#ifdef TEST_MODE
    game->roll_inject_active = 1;
    game->roll_queue[0] = 9;
    game->roll_queue[1] = 3;
    game->roll_queue_len = 2;
    game->roll_queue_i = 1;
    game->test_quiet_ticks = 1;
#endif
}

static int save_worlds_equal(const struct World *a, const struct World *b)
{
    int i;
    int d;

    if (a->room_count != b->room_count) {
        return 0;
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (memcmp(a->rooms[i].name, b->rooms[i].name,
                sizeof(a->rooms[i].name)) != 0 ||
                memcmp(a->rooms[i].desc, b->rooms[i].desc,
                    sizeof(a->rooms[i].desc)) != 0 ||
                memcmp(a->rooms[i].animal, b->rooms[i].animal,
                    sizeof(a->rooms[i].animal)) != 0 ||
                memcmp(a->rooms[i].animal_noise, b->rooms[i].animal_noise,
                    sizeof(a->rooms[i].animal_noise)) != 0 ||
                a->map_x[i] != b->map_x[i] ||
                a->map_y[i] != b->map_y[i] ||
                a->map_ready[i] != b->map_ready[i]) {
            return 0;
        }
        for (d = 0; d < CFG_DIR_MAX; ++d) {
            if (a->rooms[i].exits[d] != b->rooms[i].exits[d]) {
                return 0;
            }
        }
    }
    return 1;
}

static int save_npcs_equal(const struct NpcState *a, const struct NpcState *b)
{
    int i;

    for (i = 0; i < CFG_NPC_MAX; ++i) {
        if (a[i].actor != b[i].actor ||
                a[i].dialogue != b[i].dialogue ||
                a[i].encounter != b[i].encounter ||
                a[i].level != b[i].level ||
                a[i].room_id != b[i].room_id ||
                a[i].flags != b[i].flags ||
                a[i].return_tick != b[i].return_tick) {
            return 0;
        }
    }
    return 1;
}

static int save_games_equal(const struct GameState *a,
                            const struct GameState *b)
{
    if (a->world.room_count != b->world.room_count ||
            a->player.room_id != b->player.room_id ||
            a->tick != b->tick ||
            a->seed != b->seed ||
            a->running != b->running ||
            a->mode != b->mode ||
            a->dialogue != b->dialogue ||
            a->env_focus_active != b->env_focus_active ||
            a->env_focus_room != b->env_focus_room ||
            a->env_focus_kind != b->env_focus_kind ||
            a->env_focus_expires_tick != b->env_focus_expires_tick ||
            a->herbalist_story != b->herbalist_story ||
            a->marsh_root_spawned != b->marsh_root_spawned ||
            a->bag_count != b->bag_count ||
            a->bag_capacity != b->bag_capacity ||
            a->level != b->level ||
            a->xp != b->xp ||
            a->max_hp != b->max_hp ||
            a->damage_bonus != b->damage_bonus ||
            a->weapon_equipped != b->weapon_equipped ||
            a->player_hp != b->player_hp ||
            a->combat.enemy_hp != b->combat.enemy_hp ||
            a->combat.enemy_level != b->combat.enemy_level ||
            a->combat.defending != b->combat.defending) {
        return 0;
    }
    if (!save_worlds_equal(&a->world, &b->world) ||
            !save_npcs_equal(a->npcs, b->npcs) ||
            memcmp(a->room_item, b->room_item, sizeof(a->room_item)) != 0 ||
            memcmp(a->bag, b->bag, sizeof(a->bag)) != 0 ||
            memcmp(a->corpse_present, b->corpse_present,
                sizeof(a->corpse_present)) != 0 ||
            memcmp(a->corpse_item, b->corpse_item,
                sizeof(a->corpse_item)) != 0 ||
            memcmp(a->room_explored, b->room_explored,
                sizeof(a->room_explored)) != 0) {
        return 0;
    }
#ifdef TEST_MODE
    if (a->roll_inject_active != b->roll_inject_active ||
            a->roll_queue_len != b->roll_queue_len ||
            a->roll_queue_i != b->roll_queue_i ||
            a->test_quiet_ticks != b->test_quiet_ticks ||
            memcmp(a->roll_queue, b->roll_queue, sizeof(a->roll_queue)) != 0) {
        return 0;
    }
#endif
    return 1;
}

static int save_write_u16_le(FILE *fp, unsigned int value)
{
    unsigned char bytes[2];

    bytes[0] = (unsigned char)(value & 0xFFU);
    bytes[1] = (unsigned char)((value >> 8) & 0xFFU);
    return fwrite(bytes, 1, 2, fp) == 2;
}

static int save_write_s16_le(FILE *fp, int value)
{
    return save_write_u16_le(fp, (unsigned int)((unsigned short)(short)value));
}

static int save_write_version(FILE *fp, unsigned int version)
{
    return fseek(fp, 4L, SEEK_SET) == 0L &&
        save_write_u16_le(fp, version);
}

static int save_write_u32_le(FILE *fp, unsigned long value)
{
    unsigned char bytes[4];

    bytes[0] = (unsigned char)(value & 0xFFUL);
    bytes[1] = (unsigned char)((value >> 8) & 0xFFUL);
    bytes[2] = (unsigned char)((value >> 16) & 0xFFUL);
    bytes[3] = (unsigned char)((value >> 24) & 0xFFUL);
    return fwrite(bytes, 1, 4, fp) == 4;
}

static long save_rng_draw_offset(void)
{
    unsigned long offset;
    unsigned long room_size;

    room_size = (unsigned long)CFG_NAME_MAX +
        (unsigned long)CFG_DESC_MAX +
        (unsigned long)CFG_NAME_MAX +
        (unsigned long)CFG_DESC_MAX +
        ((unsigned long)DIR_NONE * 2UL);
    offset = 6UL;
    offset += 2UL;
    offset += room_size * (unsigned long)CFG_ROOM_MAX;
    offset += ((2UL + 2UL + 1UL) * (unsigned long)CFG_ROOM_MAX);
    offset += 2UL;
    offset += 4UL;
    offset += 4UL;
    return (long)offset;
}

static long save_world_map_offset(int room_index, int field_index)
{
    unsigned long offset;
    unsigned long room_size;

    room_size = (unsigned long)CFG_NAME_MAX +
        (unsigned long)CFG_DESC_MAX +
        (unsigned long)CFG_NAME_MAX +
        (unsigned long)CFG_DESC_MAX +
        ((unsigned long)DIR_NONE * 2UL);
    offset = 6UL;
    offset += 2UL;
    offset += room_size * (unsigned long)CFG_ROOM_MAX;
    offset += ((unsigned long)room_index * 5UL);
    offset += (unsigned long)field_index * 2UL;
    return (long)offset;
}

TEST save_round_trip_preserves_state_and_rng_count(void)
{
    struct GameState game;
    struct GameState loaded;
    u32 saved_draws;
    u32 loaded_draws;
    int rc;

    save_cleanup_file();
    save_fill_fixture(&game);
    (void)plat_rand();
    (void)plat_rand();
    saved_draws = plat_rand_draw_count();

    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, saved_draws));
    rc = save_read_game(save_test_path(), &loaded, &loaded_draws);
    ASSERT_EQ(SAVE_RESULT_OK, rc);
    ASSERT_EQ(saved_draws, loaded_draws);
    ASSERT(save_games_equal(&game, &loaded));

    save_cleanup_file();
    PASS();
}

TEST save_rejects_bad_magic(void)
{
    FILE *fp;
    struct GameState loaded;
    u32 loaded_draws;

    save_cleanup_file();
    fp = fopen(save_test_path(), "wb");
    ASSERT(fp != 0);
    ASSERT_EQ(4, fwrite("BAD!", 1, 4, fp));
    ASSERT(save_write_u16_le(fp, 1U));
    fclose(fp);

    ASSERT_EQ(SAVE_RESULT_FORMAT,
        save_read_game(save_test_path(), &loaded, &loaded_draws));
    save_cleanup_file();
    PASS();
}

TEST save_rejects_truncated_file(void)
{
    FILE *fp;
    struct GameState loaded;
    u32 loaded_draws;

    save_cleanup_file();
    fp = fopen(save_test_path(), "wb");
    ASSERT(fp != 0);
    ASSERT_EQ(4, fwrite("DMSV", 1, 4, fp));
    ASSERT(save_write_u16_le(fp, 1U));
    fclose(fp);

    ASSERT_EQ(SAVE_RESULT_FORMAT,
        save_read_game(save_test_path(), &loaded, &loaded_draws));
    save_cleanup_file();
    PASS();
}

/* prior SAVE_VERSION values fail with SAVE_RESULT_FORMAT and leave out_game untouched */
TEST save_rejects_prior_version_without_mutating_target(void)
{
    struct GameState game;
    struct GameState loaded;
    struct GameState before;
    FILE *fp;
    u32 loaded_draws;

    save_cleanup_file();
    save_fill_fixture(&game);
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, 7U));

    fp = fopen(save_test_path(), "r+b");
    ASSERT(fp != 0);
    ASSERT(save_write_version(fp, 4U));
    fclose(fp);

    unit_game_fresh(&loaded, 77U);
    before = loaded;
    loaded_draws = 555U;
    ASSERT_EQ(SAVE_RESULT_FORMAT,
        save_read_game(save_test_path(), &loaded, &loaded_draws));
    ASSERT(save_games_equal(&before, &loaded));
    ASSERT_EQ(555U, loaded_draws);

    save_cleanup_file();
    PASS();
}

TEST save_rejects_out_of_range_without_mutating_target(void)
{
    struct GameState game;
    struct GameState target;
    struct GameState before;
    FILE *fp;
    u32 loaded_draws;

    save_cleanup_file();
    save_fill_fixture(&game);
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, plat_rand_draw_count()));

    fp = fopen(save_test_path(), "r+b");
    ASSERT(fp != 0);
    ASSERT_EQ(0L, fseek(fp, 6L, SEEK_SET));
    ASSERT(save_write_u16_le(fp, 99U));
    fclose(fp);

    unit_game_fresh(&target, 77U);
    before = target;
    ASSERT_EQ(SAVE_RESULT_RANGE,
        save_read_game(save_test_path(), &target, &loaded_draws));
    ASSERT(save_games_equal(&before, &target));

    save_cleanup_file();
    PASS();
}

TEST save_rejects_excessive_rng_draw_count(void)
{
    struct GameState game;
    struct GameState target;
    struct GameState before;
    FILE *fp;
    u32 loaded_draws;

    save_cleanup_file();
    save_fill_fixture(&game);
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, plat_rand_draw_count()));

    fp = fopen(save_test_path(), "r+b");
    ASSERT(fp != 0);
    ASSERT_EQ(0L, fseek(fp, save_rng_draw_offset(), SEEK_SET));
    ASSERT(save_write_u32_le(fp, (unsigned long)CFG_SAVE_RNG_DRAW_MAX + 1UL));
    fclose(fp);

    unit_game_fresh(&target, 77U);
    before = target;
    loaded_draws = 777U;
    ASSERT_EQ(SAVE_RESULT_RANGE,
        save_read_game(save_test_path(), &target, &loaded_draws));
    ASSERT(save_games_equal(&before, &target));
    ASSERT_EQ(777U, loaded_draws);

    save_cleanup_file();
    PASS();
}

TEST save_rejects_write_with_excessive_rng_draw_count(void)
{
    struct GameState game;
    FILE *fp;

    save_cleanup_file();
    save_fill_fixture(&game);

    ASSERT_EQ(SAVE_RESULT_RANGE,
        save_write_game(save_test_path(), &game,
            (u32)((unsigned long)CFG_SAVE_RNG_DRAW_MAX + 1UL)));

    fp = fopen(save_test_path(), "rb");
    ASSERT(fp == 0);

    save_cleanup_file();
    PASS();
}

TEST save_failed_write_preserves_existing_save(void)
{
    struct GameState original;
    struct GameState changed;
    struct GameState loaded;
    u32 loaded_draws;

    save_cleanup_file();

    save_fill_fixture(&original);
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &original, 7U));

    save_fill_fixture(&changed);
    changed.player.room_id = WORLD_ROOM_CAVE;
    changed.tick = 123U;

    ASSERT_EQ(SAVE_RESULT_RANGE,
        save_write_game(save_test_path(), &changed,
            (u32)((unsigned long)CFG_SAVE_RNG_DRAW_MAX + 1UL)));

    ASSERT_EQ(SAVE_RESULT_OK,
        save_read_game(save_test_path(), &loaded, &loaded_draws));
    ASSERT_EQ(7U, loaded_draws);
    ASSERT(save_games_equal(&original, &loaded));

    save_cleanup_file();
    PASS();
}

TEST save_rejects_excessive_map_coordinate_span(void)
{
    struct GameState game;
    struct GameState target;
    struct GameState before;
    FILE *fp;
    u32 loaded_draws;

    save_cleanup_file();
    save_fill_fixture(&game);
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, 7U));

    fp = fopen(save_test_path(), "r+b");
    ASSERT(fp != 0);
    ASSERT_EQ(0L, fseek(fp, save_world_map_offset(0, 0), SEEK_SET));
    ASSERT(save_write_s16_le(fp, -32768));
    ASSERT_EQ(0L, fseek(fp, save_world_map_offset(1, 0), SEEK_SET));
    ASSERT(save_write_s16_le(fp, 32767));
    fclose(fp);

    unit_game_fresh(&target, 77U);
    before = target;
    loaded_draws = 111U;
    ASSERT_EQ(SAVE_RESULT_RANGE,
        save_read_game(save_test_path(), &target, &loaded_draws));
    ASSERT(save_games_equal(&before, &target));
    ASSERT_EQ(111U, loaded_draws);

    save_cleanup_file();
    PASS();
}

TEST save_rejects_combat_midfight_without_enemy_level(void)
{
    struct GameState game;
    struct GameState loaded;
    u32 loaded_draws;

    save_cleanup_file();
    save_fill_fixture(&game);
    game.mode = GAME_MODE_COMBAT;
    game.combat.enemy_hp = 5;
    game.combat.enemy_level = 0;
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, 7U));
    ASSERT_EQ(SAVE_RESULT_RANGE,
        save_read_game(save_test_path(), &loaded, &loaded_draws));
    save_cleanup_file();
    PASS();
}

TEST save_rejects_oversized_bandit_level(void)
{
    struct GameState game;
    struct GameState loaded;
    int slot;
    u32 loaded_draws;

    save_cleanup_file();
    save_fill_fixture(&game);
    slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT);
    ASSERT(slot >= 0);
    game.npcs[slot].level = 99;
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, 7U));
    ASSERT_EQ(SAVE_RESULT_RANGE,
        save_read_game(save_test_path(), &loaded, &loaded_draws));
    save_cleanup_file();
    PASS();
}

TEST save_rejects_oversized_combat_enemy_level(void)
{
    struct GameState game;
    struct GameState loaded;
    u32 loaded_draws;

    save_cleanup_file();
    save_fill_fixture(&game);
    game.mode = GAME_MODE_COMBAT;
    game.combat.enemy_hp = 5;
    game.combat.enemy_level = 99;
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, 7U));
    ASSERT_EQ(SAVE_RESULT_RANGE,
        save_read_game(save_test_path(), &loaded, &loaded_draws));
    save_cleanup_file();
    PASS();
}

TEST save_round_trip_preserves_seeded_roaming_bandit(void)
{
    struct GameState game;
    struct GameState loaded;
    int bandit_slot;
    int loaded_slot;
    u32 loaded_draws;

    save_cleanup_file();
    unit_game_fresh(&game, 222U);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    bandit_slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT);
    ASSERT(bandit_slot >= 0);
    ASSERT_EQ(DIALOGUE_NONE, game.npcs[bandit_slot].dialogue);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, game.npcs[bandit_slot].encounter);
    ASSERT_EQ(WORLD_ROOM_ROAD, game.npcs[bandit_slot].room_id);
    ASSERT_EQ(NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS,
        game.npcs[bandit_slot].flags);
    ASSERT_EQ(0U, game.npcs[bandit_slot].return_tick);

    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, plat_rand_draw_count()));
    ASSERT_EQ(SAVE_RESULT_OK,
        save_read_game(save_test_path(), &loaded, &loaded_draws));

    loaded_slot = npc_find_by_actor(&loaded, GAME_DIALOGUE_ACTOR_BANDIT);
    ASSERT(loaded_slot >= 0);
    ASSERT_EQ(DIALOGUE_NONE, loaded.npcs[loaded_slot].dialogue);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, loaded.npcs[loaded_slot].encounter);
    ASSERT_EQ(WORLD_ROOM_ROAD, loaded.npcs[loaded_slot].room_id);
    ASSERT_EQ(NPC_FLAG_ACTIVE | NPC_FLAG_ROAMING | NPC_FLAG_RESPAWNS,
        loaded.npcs[loaded_slot].flags);
    ASSERT_EQ(0U, loaded.npcs[loaded_slot].return_tick);
    ASSERT_EQ(0U, loaded_draws);

    save_cleanup_file();
    PASS();
}

TEST save_round_trip_preserves_herbalist_reward_on_ground(void)
{
    struct GameState game;
    struct GameState loaded;
    u32 loaded_draws;

    unit_game_fresh(&game, 987u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    game.herbalist_story = HERBALIST_STORY_COMPLETE;
    game.room_item[WORLD_ROOM_ORCHARD][0] = ITEM_SALVE;

    save_cleanup_file();
    ASSERT_EQ(SAVE_RESULT_OK,
        save_write_game(save_test_path(), &game, plat_rand_draw_count()));
    ASSERT_EQ(SAVE_RESULT_OK,
        save_read_game(save_test_path(), &loaded, &loaded_draws));
    ASSERT_EQ(HERBALIST_STORY_COMPLETE, loaded.herbalist_story);
    ASSERT_EQ(ITEM_SALVE, loaded.room_item[WORLD_ROOM_ORCHARD][0]);
    ASSERT_EQ(0U, loaded_draws);
    save_cleanup_file();
    PASS();
}

SUITE(save)
{
    RUN_TEST(save_round_trip_preserves_state_and_rng_count);
    RUN_TEST(save_rejects_bad_magic);
    RUN_TEST(save_rejects_truncated_file);
    RUN_TEST(save_rejects_prior_version_without_mutating_target);
    RUN_TEST(save_rejects_out_of_range_without_mutating_target);
    RUN_TEST(save_rejects_excessive_rng_draw_count);
    RUN_TEST(save_rejects_write_with_excessive_rng_draw_count);
    RUN_TEST(save_failed_write_preserves_existing_save);
    RUN_TEST(save_rejects_excessive_map_coordinate_span);
    RUN_TEST(save_rejects_combat_midfight_without_enemy_level);
    RUN_TEST(save_rejects_oversized_bandit_level);
    RUN_TEST(save_rejects_oversized_combat_enemy_level);
    RUN_TEST(save_round_trip_preserves_seeded_roaming_bandit);
    RUN_TEST(save_round_trip_preserves_herbalist_reward_on_ground);
}
