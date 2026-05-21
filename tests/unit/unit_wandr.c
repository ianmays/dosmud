#include "greatest.h"
#include "config.h"
#include "game.h"
#include "wanderer.h"
#include "unit_util.h"

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
    before = game.wanderer_room;
    wanderer_step(&game);
    ASSERT_NEQ(before, game.wanderer_room);
    PASS();
}

TEST wanderer_encounter_guards(void)
{
    struct GameState game;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.wanderer_need_separation = 1;
    wanderer_begin_encounter(&game);
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);

    game.wanderer_need_separation = 0;
    game.wanderer_room = game.player.room_id;
    wanderer_begin_encounter(&game);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_WANDERER, game.dialogue);
    PASS();
}

TEST wanderer_reply_cmd_explore(void)
{
    struct GameState game;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_WANDERER);
    ASSERT_EQ(1, wanderer_cmd_reply(&game, 2));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    ASSERT_EQ(0, game.wanderer_active);
    PASS();
}

SUITE(wanderer) {
    RUN_TEST(wanderer_separation_clears);
    RUN_TEST(wanderer_step_moves);
    RUN_TEST(wanderer_encounter_guards);
    RUN_TEST(wanderer_reply_cmd_explore);
}
