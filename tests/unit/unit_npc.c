#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gout.h"
#include "npc.h"
#include "platform.h"
#include "world.h"
#include "unit_util.h"

/*
 * Direct npc.c API tests: room/actor lookup, roaming movement/encounter, and
 * open-room dialogue without going through game_process_input.
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
        npc_dialogue_actor(DIALOGUE_TRAVELER));
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

/* Out-taking helpers assert #160 encounter/dialogue events from npc.c roaming. */
static void begin_roaming_npc(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    npc_roaming_begin_encounter(game, out);
}

static int roaming_npc_reply_out(struct GameState *game, int choice,
                                 GameEventQueue *out)
{
    game_event_queue_reset(out);
    return npc_roaming_cmd_reply(game, choice, out);
}

TEST npc_seed_roaming_traveler_sets_state(void)
{
    struct GameState game;

    unit_game_fresh(&game, 39u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    npc_seed_roaming_traveler(&game);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_TRAVELER, game.roaming_npc_actor);
    ASSERT_EQ(DIALOGUE_TRAVELER, game.roaming_npc_dialogue);
    ASSERT_EQ(GAME_ENCOUNTER_TRAVELER, game.roaming_npc_encounter);
    ASSERT_EQ(WORLD_ROOM_RUINS, game.roaming_npc_room);
    ASSERT_EQ(0, game.roaming_npc_need_separation);
    ASSERT_EQ(1, game.roaming_npc_active);
    ASSERT_EQ(0, game.roaming_npc_return_tick);
    PASS();
}

TEST npc_roaming_separation_clears(void)
{
    struct GameState game;

    unit_game_fresh(&game, 40u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.roaming_npc_room = WORLD_ROOM_ROAD;
    game.roaming_npc_need_separation = 1;
    npc_roaming_update_separation(&game);
    ASSERT_EQ(0, game.roaming_npc_need_separation);
    PASS();
}

TEST npc_roaming_step_moves(void)
{
    struct GameState game;
    int before;

    unit_game_fresh(&game, 41u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.roaming_npc_room = WORLD_ROOM_CAMP;
    plat_seed_rng(42u);
    ASSERT_EQ(0U, plat_rand_draw_count());
    before = game.roaming_npc_room;
    npc_roaming_step(&game);
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_NEQ(before, game.roaming_npc_room);
    PASS();
}

TEST npc_roaming_encounter_guards(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 42u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.roaming_npc_need_separation = 1;
    begin_roaming_npc(&game, &out);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);

    game.roaming_npc_need_separation = 0;
    game.roaming_npc_room = game.player.room_id;
    begin_roaming_npc(&game, &out);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_TRAVELER, game.dialogue);
    PASS();
}

TEST npc_roaming_reply_cmd_explore(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 43u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    plat_seed_rng(game.seed);
    ASSERT_EQ(1, roaming_npc_reply_out(&game, 2, &out));
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, game.roaming_npc_active);
    PASS();
}

TEST npc_roaming_reply_cmd_invalid_choice(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 44u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    game.roaming_npc_active = 1;
    plat_seed_rng(game.seed);
    /* Guard path must not schedule return RNG or clear encounter state. */
    ASSERT_EQ(1, roaming_npc_reply_out(&game, 0, &out));
    ASSERT_EQ(0U, plat_rand_draw_count());
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(1, game.roaming_npc_active);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

TEST npc_roaming_encounter_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 45u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.roaming_npc_need_separation = 0;
    game.roaming_npc_room = game.player.room_id;
    begin_roaming_npc(&game, &out);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_TRAVELER, out.events[0].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);
    PASS();
}

TEST npc_roaming_reply_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 46u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_TRAVELER);
    ASSERT_EQ(1, roaming_npc_reply_out(&game, 2, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_TRAVELER, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(2, out.events[0].arg2);
    PASS();
}

SUITE(npc) {
    RUN_TEST(npc_room_actor_lookup);
    RUN_TEST(npc_dialogue_actor_lookup);
    RUN_TEST(npc_choice_validation);
    RUN_TEST(npc_open_room_dialogue_frog);
    RUN_TEST(npc_open_room_dialogue_watchman);
    RUN_TEST(npc_open_room_dialogue_none);
    RUN_TEST(npc_seed_roaming_traveler_sets_state);
    RUN_TEST(npc_roaming_separation_clears);
    RUN_TEST(npc_roaming_step_moves);
    RUN_TEST(npc_roaming_encounter_guards);
    RUN_TEST(npc_roaming_reply_cmd_explore);
    RUN_TEST(npc_roaming_reply_cmd_invalid_choice);
    RUN_TEST(npc_roaming_encounter_event);
    RUN_TEST(npc_roaming_reply_event);
}
