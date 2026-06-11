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
    unit_game_fresh(game, 1234U);
    game->player.room_id = WORLD_ROOM_TOWER;
    game->tick = 77U;
    game->mode = GAME_MODE_DIALOGUE;
    game->dialogue = DIALOGUE_NPC_ARCHIVIST;
    game->roaming_npc_actor = GAME_DIALOGUE_ACTOR_TRAVELER;
    game->roaming_npc_dialogue = DIALOGUE_TRAVELER;
    game->roaming_npc_encounter = GAME_ENCOUNTER_TRAVELER;
    game->roaming_npc_room = WORLD_ROOM_MEADOW;
    game->roaming_npc_need_separation = 1;
    game->env_focus_active = 1;
    game->env_focus_room = WORLD_ROOM_TOWER;
    game->env_focus_kind = GAME_ENV_CREAK;
    game->env_focus_expires_tick = 81U;
    game->bag[0] = ITEM_STICK;
    game->bag[1] = ITEM_SALVE;
    game->bag[2] = ITEM_SPEAR;
    game->bag_count = 3;
    game->bag_capacity = 6;
    game->level = 3;
    game->xp = 14;
    game->max_hp = 24;
    game->damage_bonus = 2;
    game->weapon_equipped = ITEM_SPEAR;
    game->player_hp = 19;
    game->enemy_handover_pick = 1;
    game->combat.enemy_hp = 5;
    game->combat.defending = 1;
    game->corpse_present[WORLD_ROOM_ROAD] = 1;
    game->corpse_loot[WORLD_ROOM_ROAD] = ITEM_HERB;
    game->roaming_npc_active = 0;
    game->roaming_npc_return_tick = 103U;
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
            a->roaming_npc_actor != b->roaming_npc_actor ||
            a->roaming_npc_dialogue != b->roaming_npc_dialogue ||
            a->roaming_npc_encounter != b->roaming_npc_encounter ||
            a->roaming_npc_room != b->roaming_npc_room ||
            a->roaming_npc_need_separation != b->roaming_npc_need_separation ||
            a->env_focus_active != b->env_focus_active ||
            a->env_focus_room != b->env_focus_room ||
            a->env_focus_kind != b->env_focus_kind ||
            a->env_focus_expires_tick != b->env_focus_expires_tick ||
            a->bag_count != b->bag_count ||
            a->bag_capacity != b->bag_capacity ||
            a->level != b->level ||
            a->xp != b->xp ||
            a->max_hp != b->max_hp ||
            a->damage_bonus != b->damage_bonus ||
            a->weapon_equipped != b->weapon_equipped ||
            a->player_hp != b->player_hp ||
            a->enemy_handover_pick != b->enemy_handover_pick ||
            a->combat.enemy_hp != b->combat.enemy_hp ||
            a->combat.defending != b->combat.defending ||
            a->roaming_npc_active != b->roaming_npc_active ||
            a->roaming_npc_return_tick != b->roaming_npc_return_tick) {
        return 0;
    }
    if (memcmp(&a->world, &b->world, sizeof(a->world)) != 0 ||
            memcmp(a->room_item, b->room_item, sizeof(a->room_item)) != 0 ||
            memcmp(a->bag, b->bag, sizeof(a->bag)) != 0 ||
            memcmp(a->corpse_present, b->corpse_present,
                sizeof(a->corpse_present)) != 0 ||
            memcmp(a->corpse_loot, b->corpse_loot,
                sizeof(a->corpse_loot)) != 0 ||
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

SUITE(save)
{
    RUN_TEST(save_round_trip_preserves_state_and_rng_count);
    RUN_TEST(save_rejects_bad_magic);
    RUN_TEST(save_rejects_truncated_file);
    RUN_TEST(save_rejects_out_of_range_without_mutating_target);
    RUN_TEST(save_rejects_excessive_rng_draw_count);
    RUN_TEST(save_rejects_write_with_excessive_rng_draw_count);
    RUN_TEST(save_failed_write_preserves_existing_save);
    RUN_TEST(save_rejects_excessive_map_coordinate_span);
}
