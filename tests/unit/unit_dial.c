#include "greatest.h"
#include "config.h"
#include "dialogue.h"
#include "game.h"
#include "gout.h"
#include "world.h"
#include "unit_util.h"

/*
 * Dialogue router tests: #160 GameEvent payloads and mode transitions.
 * npc.c lookup tables and direct open/talk seams live in unit_npc.c.
 */
static int talk_out(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return dialogue_cmd_talk(game, out);
}

static int reply_out(struct GameState *game, int choice, GameEventQueue *out)
{
    game_event_queue_reset(out);
    return dialogue_cmd_reply(game, choice, out);
}

static int talk_out_state(struct GameState *game)
{
    GameEventQueue out;

    return talk_out(game, &out);
}

static int reply_out_state(struct GameState *game, int choice)
{
    GameEventQueue out;

    return reply_out(game, choice, &out);
}

TEST dialogue_cmd_talk_watchman(void)
{
    struct GameState game;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_WATCHMAN, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_nobody_camp(void)
{
    struct GameState game;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_frog(void)
{
    struct GameState game;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 2));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_not_dialogue(void)
{
    struct GameState game;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(0, reply_out_state(&game, 1));
    PASS();
}

TEST dialogue_cmd_talk_frog(void)
{
    struct GameState game;

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_FROG, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_bandit_blocks(void)
{
    struct GameState game;

    unit_game_fresh(&game, 9u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_ENEMY);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_traveler_waiting(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(DIALOGUE_TRAVELER, game.dialogue);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_TRAVELER_WAITING, out.events[0].arg0);
    PASS();
}

TEST dialogue_cmd_talk_herbalist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_HERBALIST, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_archivist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CATACOMBS, 0);
    ASSERT_EQ(1, talk_out_state(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_ARCHIVIST, game.dialogue);
    PASS();
}

TEST dialogue_cmd_reply_frog_invalid(void)
{
    struct GameState game;

    unit_game_fresh(&game, 13u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 0));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_FROG, game.dialogue);
    PASS();
}

TEST dialogue_cmd_reply_watchman(void)
{
    struct GameState game;

    unit_game_fresh(&game, 14u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 1));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_herbalist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 15u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 2));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_archivist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 16u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CATACOMBS, 0);
    talk_out_state(&game);
    ASSERT_EQ(1, reply_out_state(&game, 3));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_talk_watchman_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 20u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WATCHMAN, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[0].arg1);
    PASS();
}

TEST dialogue_cmd_talk_frog_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 24u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[0].arg1);
    PASS();
}

TEST dialogue_cmd_reply_frog_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 25u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out(&game, &out);
    ASSERT_EQ(1, reply_out(&game, 2, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(2, out.events[0].arg2);
    PASS();
}

TEST dialogue_cmd_talk_nobody_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 21u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_NOBODY, out.events[0].arg0);
    PASS();
}

TEST dialogue_cmd_talk_bandit_blocks_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 22u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_ENEMY);
    ASSERT_EQ(1, talk_out(&game, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK, out.events[0].arg0);
    PASS();
}

TEST dialogue_cmd_reply_frog_invalid_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 23u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out(&game, &out);
    ASSERT_EQ(1, reply_out(&game, 0, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

SUITE(dialogue) {
    RUN_TEST(dialogue_cmd_talk_watchman);
    RUN_TEST(dialogue_cmd_talk_nobody_camp);
    RUN_TEST(dialogue_cmd_reply_frog);
    RUN_TEST(dialogue_cmd_reply_not_dialogue);
    RUN_TEST(dialogue_cmd_talk_frog);
    RUN_TEST(dialogue_cmd_talk_bandit_blocks);
    RUN_TEST(dialogue_cmd_talk_traveler_waiting);
    RUN_TEST(dialogue_cmd_talk_herbalist);
    RUN_TEST(dialogue_cmd_talk_archivist);
    RUN_TEST(dialogue_cmd_reply_frog_invalid);
    RUN_TEST(dialogue_cmd_reply_watchman);
    RUN_TEST(dialogue_cmd_reply_herbalist);
    RUN_TEST(dialogue_cmd_reply_archivist);
    RUN_TEST(dialogue_cmd_talk_watchman_event);
    RUN_TEST(dialogue_cmd_talk_frog_event);
    RUN_TEST(dialogue_cmd_reply_frog_event);
    RUN_TEST(dialogue_cmd_talk_nobody_event);
    RUN_TEST(dialogue_cmd_talk_bandit_blocks_event);
    RUN_TEST(dialogue_cmd_reply_frog_invalid_event);
}
