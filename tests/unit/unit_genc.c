#include "greatest.h"
#include "config.h"
#include "game.h"
#include "genc.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "npc.h"
#include "unit_util.h"

/*
 * out-taking helpers assert #160 encounter events; *_state wraps a local
 * GameEventQueue for mode-only tests that do not inspect the queue.
 */
static void begin_enemy(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    enemy_begin_encounter(game, out);
}

static int enemy_reply(struct GameState *game, int choice, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return genc_cmd_reply(game, choice, out);
}

static int enemy_give(struct GameState *game, int item_id, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return genc_cmd_give(game, item_id, out);
}

static struct NpcState *bandit_npc(struct GameState *game)
{
    int slot;

    slot = npc_find_by_actor(game, GAME_DIALOGUE_ACTOR_BANDIT);
    if (slot < 0) {
        return 0;
    }
    return &game->npcs[slot];
}

static void begin_enemy_state(struct GameState *game)
{
    GameEventQueue out;

    begin_enemy(game, &out);
}

TEST genc_skips_when_busy(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    begin_enemy_state(&game);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    PASS();
}

TEST genc_opens_dialogue(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy(&game, &out);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_BANDIT, out.events[0].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);
    PASS();
}

TEST genc_opens_fixed_bandit_without_moving_slot(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *bandit;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    begin_enemy(&game, &out);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    ASSERT_EQ(WORLD_ROOM_ROAD, bandit->room_id);
    ASSERT_EQ(NPC_FLAG_ACTIVE, bandit->flags & NPC_FLAG_ACTIVE);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);
    PASS();
}

TEST genc_cmd_reply_fight(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    ASSERT_EQ(1, enemy_reply(&game, 1, &out));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[0].kind);
    ASSERT_EQ(GAME_COMBAT_PHASE_START, out.events[0].arg0);
    PASS();
}

TEST genc_cmd_reply_intimidate_ok(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[1];

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    rolls[0] = CFG_TEST_INTIMIDATE_OK;
    game_roll_inject_begin(&game, rolls, 1);
    ASSERT_EQ(1, enemy_reply(&game, 3, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_INTIMIDATE, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_SUCCESS, out.events[0].arg2);
    PASS();
}

TEST genc_cmd_give_wrong_context(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, enemy_give(&game, ITEM_STICK, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_WRONG_CONTEXT, out.events[0].arg2);
    PASS();
}

TEST genc_cmd_give_handover(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *bandit;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    game_inv_bag_add(&game, ITEM_STICK);
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    bandit->flags |= NPC_FLAG_HANDOVER_PICK;
    ASSERT_EQ(1, enemy_give(&game, ITEM_STICK, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_GIVE, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_OK, out.events[0].arg2);
    ASSERT_EQ(ITEM_STICK, out.events[0].arg3);
    PASS();
}

TEST genc_cmd_reply_handover_pick(void)
{
    struct GameState game;
    GameEventQueue out;
    struct NpcState *bandit;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    game_inv_bag_add(&game, ITEM_STICK);
    ASSERT_EQ(1, enemy_reply(&game, 2, &out));
    bandit = bandit_npc(&game);
    ASSERT(bandit != 0);
    ASSERT_EQ(NPC_FLAG_HANDOVER_PICK,
        bandit->flags & NPC_FLAG_HANDOVER_PICK);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_HANDOVER_PROMPT, out.events[0].arg1);
    PASS();
}

TEST genc_cmd_reply_intimidate_fail(void)
{
    struct GameState game;
    GameEventQueue out;
    int rolls[1];

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    rolls[0] = CFG_TEST_INTIMIDATE_FAIL;
    game_roll_inject_begin(&game, rolls, 1);
    ASSERT_EQ(1, enemy_reply(&game, 3, &out));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_FAIL, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[1].kind);
    PASS();
}

TEST genc_cmd_reply_invalid_choice(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 9u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    ASSERT_EQ(1, enemy_reply(&game, 0, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

TEST genc_cmd_reply_bag_empty_then_combat(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    begin_enemy_state(&game);
    ASSERT_EQ(1, enemy_reply(&game, 2, &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_HANDOVER, out.events[0].arg1);
    ASSERT_EQ(GAME_ENCOUNTER_OUTCOME_BAG_EMPTY, out.events[0].arg2);
    ASSERT_EQ(GAME_EVENT_COMBAT, out.events[1].kind);
    PASS();
}

SUITE(genc) {
    RUN_TEST(genc_skips_when_busy);
    RUN_TEST(genc_opens_dialogue);
    RUN_TEST(genc_opens_fixed_bandit_without_moving_slot);
    RUN_TEST(genc_cmd_reply_fight);
    RUN_TEST(genc_cmd_reply_intimidate_ok);
    RUN_TEST(genc_cmd_give_wrong_context);
    RUN_TEST(genc_cmd_give_handover);
    RUN_TEST(genc_cmd_reply_handover_pick);
    RUN_TEST(genc_cmd_reply_intimidate_fail);
    RUN_TEST(genc_cmd_reply_invalid_choice);
    RUN_TEST(genc_cmd_reply_bag_empty_then_combat);
}
