/*
 * Direct gwhok slice tests: flag idempotency and room-desc reconcile without
 * routing through npc.c or game_process_input.
 */
#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gwhok.h"
#include "txtres.h"
#include "unit_util.h"
#include "world.h"

TEST gwhok_has_false_before_set(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(0, gwhok_has(&game, WORLD_ADV_ORCHARD_RESTORED));
    ASSERT_EQ(0, gwhok_has(&game, WORLD_ADV_TOWER_MEAL));
    PASS();
}

TEST gwhok_set_sets_flag_and_applies_orchard_desc(void)
{
    struct GameState game;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_STR_EQ(g_room_descs[WORLD_ROOM_ORCHARD],
        game.world.rooms[WORLD_ROOM_ORCHARD].desc);
    ASSERT_EQ(1, gwhok_set(&game, WORLD_ADV_ORCHARD_RESTORED));
    ASSERT_EQ(1, gwhok_has(&game, WORLD_ADV_ORCHARD_RESTORED));
    ASSERT_STR_EQ(TXT_STORY_ORCHARD_DONE_DESC,
        game.world.rooms[WORLD_ROOM_ORCHARD].desc);
    PASS();
}

TEST gwhok_set_is_idempotent(void)
{
    struct GameState game;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(1, gwhok_set(&game, WORLD_ADV_ORCHARD_RESTORED));
    ASSERT_EQ(0, gwhok_set(&game, WORLD_ADV_ORCHARD_RESTORED));
    ASSERT_EQ(WORLD_ADV_ORCHARD_RESTORED, game.world_adv_flags);
    PASS();
}

TEST gwhok_set_applies_tower_desc(void)
{
    struct GameState game;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_STR_EQ(g_room_descs[WORLD_ROOM_TOWER],
        game.world.rooms[WORLD_ROOM_TOWER].desc);
    ASSERT_EQ(1, gwhok_set(&game, WORLD_ADV_TOWER_MEAL));
    ASSERT_EQ(1, gwhok_has(&game, WORLD_ADV_TOWER_MEAL));
    ASSERT_STR_EQ(TXT_STORY_TOWER_FED_DESC,
        game.world.rooms[WORLD_ROOM_TOWER].desc);
    PASS();
}

TEST gwhok_apply_all_restores_baseline_when_flag_cleared(void)
{
    struct GameState game;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(1, gwhok_set(&game, WORLD_ADV_ORCHARD_RESTORED));
    game.world_adv_flags = 0;
    gwhok_apply_all(&game);
    ASSERT_STR_EQ(g_room_descs[WORLD_ROOM_ORCHARD],
        game.world.rooms[WORLD_ROOM_ORCHARD].desc);
    PASS();
}

TEST gwhok_apply_all_reconciles_both_rows(void)
{
    struct GameState game;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.world_adv_flags = WORLD_ADV_ORCHARD_RESTORED | WORLD_ADV_TOWER_MEAL;
    gwhok_apply_all(&game);
    ASSERT_STR_EQ(TXT_STORY_ORCHARD_DONE_DESC,
        game.world.rooms[WORLD_ROOM_ORCHARD].desc);
    ASSERT_STR_EQ(TXT_STORY_TOWER_FED_DESC,
        game.world.rooms[WORLD_ROOM_TOWER].desc);
    PASS();
}

SUITE(gwhok)
{
    RUN_TEST(gwhok_has_false_before_set);
    RUN_TEST(gwhok_set_sets_flag_and_applies_orchard_desc);
    RUN_TEST(gwhok_set_is_idempotent);
    RUN_TEST(gwhok_set_applies_tower_desc);
    RUN_TEST(gwhok_apply_all_restores_baseline_when_flag_cleared);
    RUN_TEST(gwhok_apply_all_reconciles_both_rows);
}
