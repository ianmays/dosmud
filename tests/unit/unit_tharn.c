#include "greatest.h"
#include "config.h"
#include "game.h"
#include "testharn.h"
#include "world.h"
#include "unit_util.h"

TEST testharn_seed_directive(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 1u);
    rc = testharn_apply(&game, "@seed 4242");
    ASSERT_EQ(1, rc);
    ASSERT_EQ(4242u, game.seed);
    PASS();
}

TEST testharn_seed_invalid(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 2u);
    rc = testharn_apply(&game, "@seed -1");
    ASSERT_EQ(-3, rc);
    PASS();
}

TEST testharn_seed_missing_value(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 21u);
    rc = testharn_apply(&game, "@seed");
    ASSERT_EQ(-3, rc);
    PASS();
}

TEST testharn_fixture_missing_name(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 22u);
    rc = testharn_apply(&game, "@fixture");
    ASSERT_EQ(-1, rc);
    PASS();
}

TEST testharn_unknown_at_directive_is_not_fixture(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 23u);
    rc = testharn_apply(&game, "@bogus");
    ASSERT_EQ(0, rc);
    PASS();
}

TEST testharn_fixture_at_camp(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 3u);
    rc = testharn_apply(&game, "@fixture at_camp");
    ASSERT_EQ(1, rc);
    ASSERT_EQ(WORLD_ROOM_CAMP, game.player.room_id);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST testharn_fixture_world_boot(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 4u);
    rc = testharn_apply(&game, "@fixture world_boot");
    ASSERT_EQ(1, rc);
    ASSERT_EQ(1, world_can_move(&game.world, WORLD_ROOM_CAMP, DIR_NORTH));
    PASS();
}

TEST testharn_fixture_quiet_explore(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 5u);
    rc = testharn_apply(&game, "@fixture quiet_explore");
    ASSERT_EQ(1, rc);
    ASSERT_EQ(1, game.test_quiet_ticks);
    ASSERT_EQ(0, game.roaming_npc_active);
    PASS();
}

TEST testharn_bag_full_gate(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 7u);
    testharn_apply(&game, "@fixture at_camp");
    game.bag_count = game.bag_capacity;
    rc = testharn_apply(&game, "@fixture bag_full_gate");
    ASSERT_EQ(-2, rc);
    PASS();
}

TEST testharn_not_harness_line(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 6u);
    rc = testharn_apply(&game, "look");
    ASSERT_EQ(0, rc);
    PASS();
}

TEST testharn_fixture_sweep(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 20u);
    rc = testharn_apply(&game, "@fixture world_boot");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_dialogue");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_dialogue_empty");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_handover_pick");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_wielded_pick");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture at_road");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture at_marsh_reed");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture at_pond");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture at_tower");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture at_orchard");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture at_catacombs");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture quiet_explore");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture quiet_camp_dual_ground");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture quiet_camp_dual_ground_full_bag");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture traveler_dialogue");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_berry");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_stacked");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_berry_low_hp");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_fish_low_hp");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_salve");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_torch");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_craft_salve");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture corpse_stripped");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture env_focus_creak");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture env_focus_rustle");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_intimidate_fail");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_intimidate_ok");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_fight_ready");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_combat_defend_ready");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_combat_level_ready");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_victory_berry");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_victory_spear");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_victory_stick");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_victory_herb");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_victory_fish");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_combat_turn1_resolve");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bandit_combat_salve_ready");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture corpse_loot_full_bag");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_fish");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_spear");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_stone");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture bag_stick");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture env_focus_water");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture env_focus_grit");
    ASSERT_EQ(1, rc);
    rc = testharn_apply(&game, "@fixture world_linear");
    ASSERT_EQ(1, rc);
    PASS();
}

SUITE(testharn) {
    RUN_TEST(testharn_seed_directive);
    RUN_TEST(testharn_seed_invalid);
    RUN_TEST(testharn_seed_missing_value);
    RUN_TEST(testharn_fixture_missing_name);
    RUN_TEST(testharn_unknown_at_directive_is_not_fixture);
    RUN_TEST(testharn_fixture_at_camp);
    RUN_TEST(testharn_fixture_world_boot);
    RUN_TEST(testharn_fixture_quiet_explore);
    RUN_TEST(testharn_bag_full_gate);
    RUN_TEST(testharn_fixture_sweep);
    RUN_TEST(testharn_not_harness_line);
}
