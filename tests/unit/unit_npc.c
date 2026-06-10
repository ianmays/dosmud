#include "greatest.h"
#include "game.h"
#include "gout.h"
#include "npc.h"
#include "world.h"
#include "unit_util.h"

/*
 * Direct npc.c API tests: room/actor lookup and open-room dialogue without
 * going through dialogue_cmd_talk / game_process_input.
 */

TEST npc_room_actor_lookup(void)
{
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_NONE, npc_room_actor(WORLD_ROOM_CAMP));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, npc_room_actor(WORLD_ROOM_POND));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WATCHMAN, npc_room_actor(WORLD_ROOM_TOWER));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_HERBALIST, npc_room_actor(WORLD_ROOM_ORCHARD));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_ARCHIVIST,
        npc_room_actor(WORLD_ROOM_CATACOMBS));
    PASS();
}

TEST npc_dialogue_actor_lookup(void)
{
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_NONE, npc_dialogue_actor(DIALOGUE_NONE));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, npc_dialogue_actor(DIALOGUE_NPC_FROG));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WATCHMAN,
        npc_dialogue_actor(DIALOGUE_NPC_WATCHMAN));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_HERBALIST,
        npc_dialogue_actor(DIALOGUE_NPC_HERBALIST));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_ARCHIVIST,
        npc_dialogue_actor(DIALOGUE_NPC_ARCHIVIST));
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_NONE,
        npc_dialogue_actor(DIALOGUE_WANDERER));
    PASS();
}

TEST npc_choice_validation(void)
{
    ASSERT_EQ(0, npc_choice_is_valid(0));
    ASSERT_EQ(1, npc_choice_is_valid(1));
    ASSERT_EQ(1, npc_choice_is_valid(3));
    ASSERT_EQ(0, npc_choice_is_valid(4));
    PASS();
}

TEST npc_open_room_dialogue_frog(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 30u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_FROG, game.dialogue);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_FROG, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[0].arg1);
    PASS();
}

TEST npc_open_room_dialogue_watchman(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 31u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(1, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_WATCHMAN, game.dialogue);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WATCHMAN, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_TALK, out.events[0].arg1);
    PASS();
}

TEST npc_open_room_dialogue_none(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 32u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    ASSERT_EQ(0, npc_open_room_dialogue(&game, &out));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, out.count);
    PASS();
}

SUITE(npc) {
    RUN_TEST(npc_room_actor_lookup);
    RUN_TEST(npc_dialogue_actor_lookup);
    RUN_TEST(npc_choice_validation);
    RUN_TEST(npc_open_room_dialogue_frog);
    RUN_TEST(npc_open_room_dialogue_watchman);
    RUN_TEST(npc_open_room_dialogue_none);
}
