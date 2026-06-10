#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gout.h"
#include "wanderer.h"
#include "unit_util.h"

/* out-taking helpers assert #160 encounter/dialogue events from wanderer.c. */
static void begin_wanderer(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    wanderer_begin_encounter(game, out);
}

static int wanderer_reply_out(struct GameState *game, int choice,
                              GameEventQueue *out)
{
    game_event_queue_reset(out);
    return wanderer_cmd_reply(game, choice, out);
}

TEST wanderer_separation_clears(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.wanderer_room = WORLD_ROOM_ROAD;
    game.wanderer_need_separation = 1;
    wanderer_update_separation(&game);
    ASSERT_EQ(0, game.wanderer_need_separation);
    PASS();
}

TEST wanderer_step_moves(void)
{
    struct GameState game;
    int before;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.wanderer_room = WORLD_ROOM_CAMP;
    plat_seed_rng(42u);
    ASSERT_EQ(0U, plat_rand_draw_count());
    before = game.wanderer_room;
    wanderer_step(&game); /* one plat_rand for exit pick */
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_NEQ(before, game.wanderer_room);
    PASS();
}

TEST wanderer_encounter_guards(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.wanderer_need_separation = 1;
    begin_wanderer(&game, &out);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);

    game.wanderer_need_separation = 0;
    game.wanderer_room = game.player.room_id;
    begin_wanderer(&game, &out);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_WANDERER, game.dialogue);
    PASS();
}

TEST wanderer_reply_cmd_explore(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_WANDERER);
    plat_seed_rng(game.seed);
    ASSERT_EQ(1, wanderer_reply_out(&game, 2, &out));
    ASSERT_EQ(1U, plat_rand_draw_count());
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, game.wanderer_active);
    PASS();
}

TEST wanderer_reply_cmd_invalid_choice(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_WANDERER);
    game.wanderer_active = 1;
    plat_seed_rng(game.seed);
    ASSERT_EQ(1, wanderer_reply_out(&game, 0, &out));
    ASSERT_EQ(0U, plat_rand_draw_count());
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(1, game.wanderer_active);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

TEST wanderer_encounter_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.wanderer_need_separation = 0;
    game.wanderer_room = game.player.room_id;
    begin_wanderer(&game, &out);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENCOUNTER, out.events[0].kind);
    ASSERT_EQ(GAME_ENCOUNTER_WANDERER, out.events[0].arg0);
    ASSERT_EQ(GAME_ENCOUNTER_ACTION_OPEN, out.events[0].arg1);
    PASS();
}

TEST wanderer_reply_event(void)
{
    struct GameState game;
    GameEventQueue out;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_WANDERER);
    ASSERT_EQ(1, wanderer_reply_out(&game, 2, &out));
    ASSERT_EQ(GAME_EVENT_DIALOGUE, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_ACTOR_WANDERER, out.events[0].arg0);
    ASSERT_EQ(GAME_DIALOGUE_PHASE_REPLY, out.events[0].arg1);
    ASSERT_EQ(2, out.events[0].arg2);
    PASS();
}

SUITE(wanderer) {
    RUN_TEST(wanderer_separation_clears);
    RUN_TEST(wanderer_step_moves);
    RUN_TEST(wanderer_encounter_guards);
    RUN_TEST(wanderer_reply_cmd_explore);
    RUN_TEST(wanderer_reply_cmd_invalid_choice);
    RUN_TEST(wanderer_encounter_event);
    RUN_TEST(wanderer_reply_event);
}
