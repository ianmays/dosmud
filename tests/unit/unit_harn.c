#include "greatest.h"
#include "config.h"
#include "game.h"
#include "genc.h"
#include "invent.h"
#include "items.h"
#include "npc.h"
#include "testharn.h"
#include "unit_util.h"

TEST harness_baseline_matches_start_fields(void)
{
    struct GameState game;

    unit_game_fresh(&game, 999u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 5);

    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(WORLD_ROOM_ROAD, game.player.room_id);
    ASSERT_EQ(5u, game.tick);
    ASSERT_EQ(CFG_START_LEVEL, game.level);
    ASSERT_EQ(CFG_START_XP, game.xp);
    ASSERT_EQ(CFG_START_MAX_HP, game.max_hp);
    ASSERT_EQ(CFG_START_MAX_HP, game.player_hp);
    ASSERT_EQ(0, game.bag_count);
    ASSERT_EQ(1, game_roll_inject_fully_consumed(&game));
    PASS();
}

TEST harness_enemy_begin_after_baseline(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 1234u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    enemy_begin_encounter(&game, &out);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    PASS();
}

TEST harness_apply_unknown_fixture(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 1u);
    rc = testharn_apply(&game, "@fixture no_such_thing");
    ASSERT_EQ(-1, rc);
    PASS();
}

TEST harness_apply_bag_full_returns_minus2(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 1u);
    testharn_apply(&game, "@fixture at_camp");
    game.bag_count = game.bag_capacity;
    rc = testharn_apply(&game, "@fixture bag_full_gate");
    ASSERT_EQ(-2, rc);
    PASS();
}

TEST harness_apply_ambient_camp(void)
{
    struct GameState game;
    int rc;
    int slot;

    unit_game_fresh(&game, 4u);
    rc = testharn_apply(&game, "@fixture ambient_camp");
    ASSERT_EQ(1, rc);
    slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_TRAVELER);
    ASSERT_EQ(WORLD_ROOM_CAMP, game.player.room_id);
    ASSERT_EQ(0, game.test_quiet_ticks);
    ASSERT_EQ(0, game.npcs[slot].flags & NPC_FLAG_ACTIVE);
    PASS();
}

TEST harness_apply_quiet_camp_dual_ground(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 2u);
    rc = testharn_apply(&game, "@fixture quiet_camp_dual_ground");
    ASSERT_EQ(1, rc);
    ASSERT_EQ(ITEM_STICK, game.room_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_REED, game.room_item[WORLD_ROOM_CAMP][1]);
    ASSERT_EQ(1, game.test_quiet_ticks);
    PASS();
}

TEST harness_apply_quiet_camp_dual_ground_full_bag(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 3u);
    rc = testharn_apply(&game, "@fixture quiet_camp_dual_ground_full_bag");
    ASSERT_EQ(1, rc);
    ASSERT_EQ(ITEM_STICK, game.room_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_REED, game.room_item[WORLD_ROOM_CAMP][1]);
    ASSERT_EQ(game.bag_capacity, game.bag_count);
    ASSERT_EQ(1, game.test_quiet_ticks);
    PASS();
}

TEST harness_apply_bandit_handover_pick(void)
{
    struct GameState game;
    int rc;
    int slot;

    unit_game_fresh(&game, 5u);
    rc = testharn_apply(&game, "@fixture bandit_handover_pick");
    ASSERT_EQ(1, rc);
    slot = npc_find_by_dialogue(&game, DIALOGUE_ENEMY);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    ASSERT(slot >= 0);
    /* fixture sets slot flag only; handover gating does not use a GameState mirror */
    ASSERT_EQ(NPC_FLAG_HANDOVER_PICK,
        game.npcs[slot].flags & NPC_FLAG_HANDOVER_PICK);
    ASSERT(game.npcs[slot].actor == GAME_DIALOGUE_ACTOR_BANDIT ||
        game.npcs[slot].actor == GAME_DIALOGUE_ACTOR_BANDIT_AMBUSH);
    ASSERT_EQ(1, game_inv_player_has_item(&game, ITEM_STICK));
    PASS();
}

TEST harness_apply_traveler_dialogue(void)
{
    struct GameState game;
    int rc;
    int slot;

    unit_game_fresh(&game, 6u);
    rc = testharn_apply(&game, "@fixture traveler_dialogue");
    ASSERT_EQ(1, rc);
    slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_TRAVELER);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_TRAVELER, game.dialogue);
    ASSERT_EQ(NPC_FLAG_ACTIVE, game.npcs[slot].flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(game.player.room_id, game.npcs[slot].room_id);
    PASS();
}

TEST harness_apply_bandit_road(void)
{
    struct GameState game;
    int rc;
    int traveler_slot;
    int bandit_slot;

    unit_game_fresh(&game, 7u);
    rc = testharn_apply(&game, "@fixture bandit_road");
    ASSERT_EQ(1, rc);
    traveler_slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_TRAVELER);
    bandit_slot = npc_find_by_actor(&game, GAME_DIALOGUE_ACTOR_BANDIT);
    ASSERT_EQ(WORLD_ROOM_ROAD, game.player.room_id);
    ASSERT(traveler_slot >= 0);
    ASSERT(bandit_slot >= 0);
    ASSERT_EQ(0, game.npcs[traveler_slot].flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(NPC_FLAG_ACTIVE, game.npcs[bandit_slot].flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(WORLD_ROOM_ROAD, game.npcs[bandit_slot].room_id);
    PASS();
}

TEST harness_apply_env_focus_water(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 7u);
    rc = testharn_apply(&game, "@fixture env_focus_water");
    ASSERT_EQ(1, rc);
    ASSERT_EQ(1, game.env_focus_active);
    ASSERT_EQ(game.player.room_id, game.env_focus_room);
    ASSERT_EQ(GAME_ENV_WATER, game.env_focus_kind);
    PASS();
}

TEST harness_apply_corpse_loot_full_bag_sets_corpse_slot(void)
{
    struct GameState game;
    int rc;

    unit_game_fresh(&game, 8u);
    rc = testharn_apply(&game, "@fixture corpse_loot_full_bag");
    ASSERT_EQ(1, rc);
    ASSERT_EQ(1, game.corpse_present[WORLD_ROOM_CAMP]);
    ASSERT_EQ(ITEM_STICK, game.corpse_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_NONE, game.corpse_item[WORLD_ROOM_CAMP][1]);
    ASSERT_EQ(ITEM_NONE, game.corpse_item[WORLD_ROOM_CAMP][2]);
    PASS();
}

TEST harness_seed_repeatable_rolls(void)
{
    struct GameState game;
    int a;
    int b;

    unit_game_fresh(&game, 77u);
    testharn_apply(&game, "@fixture at_camp");
    plat_seed_rng(game.seed);
    a = game_roll_percent(&game);
    plat_seed_rng(game.seed);
    b = game_roll_percent(&game);
    ASSERT_EQ(a, b);
    PASS();
}

SUITE(harness) {
    RUN_TEST(harness_baseline_matches_start_fields);
    RUN_TEST(harness_enemy_begin_after_baseline);
    RUN_TEST(harness_apply_unknown_fixture);
    RUN_TEST(harness_apply_bag_full_returns_minus2);
    RUN_TEST(harness_apply_ambient_camp);
    RUN_TEST(harness_apply_quiet_camp_dual_ground);
    RUN_TEST(harness_apply_quiet_camp_dual_ground_full_bag);
    RUN_TEST(harness_apply_bandit_handover_pick);
    RUN_TEST(harness_apply_traveler_dialogue);
    RUN_TEST(harness_apply_bandit_road);
    RUN_TEST(harness_apply_env_focus_water);
    RUN_TEST(harness_apply_corpse_loot_full_bag_sets_corpse_slot);
    RUN_TEST(harness_seed_repeatable_rolls);
}
