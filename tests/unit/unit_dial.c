#include "greatest.h"
#include "config.h"
#include "dialogue.h"
#include "game.h"
#include "world.h"
#include "unit_util.h"

static int talk_out(struct GameState *game)
{
    struct GameOutput out;

    gout_reset(&out);
    return dialogue_cmd_talk(game, &out);
}

static int reply_out(struct GameState *game, int choice)
{
    struct GameOutput out;

    gout_reset(&out);
    return dialogue_cmd_reply(game, choice, &out);
}

TEST dialogue_npc_in_room(void)
{
    ASSERT_EQ(0, npc_in_room(WORLD_ROOM_CAMP));
    ASSERT_EQ(1, npc_in_room(WORLD_ROOM_TOWER));
    ASSERT_EQ(2, npc_in_room(WORLD_ROOM_ORCHARD));
    ASSERT_EQ(3, npc_in_room(WORLD_ROOM_CATACOMBS));
    PASS();
}

TEST dialogue_frog_render_paths(void)
{
    struct GameOutput out;

    gout_reset(&out);
    frog_dialogue_intro(&out);
    frog_dialogue_branch(1, &out);
    frog_dialogue_branch(2, &out);
    frog_dialogue_branch(3, &out);
    PASS();
}

TEST dialogue_cmd_talk_watchman(void)
{
    struct GameState game;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_EQ(1, talk_out(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_WATCHMAN, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_nobody_camp(void)
{
    struct GameState game;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, talk_out(&game));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_frog(void)
{
    struct GameState game;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out(&game);
    ASSERT_EQ(1, reply_out(&game, 2));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_not_dialogue(void)
{
    struct GameState game;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(0, reply_out(&game, 1));
    PASS();
}

TEST dialogue_cmd_talk_frog(void)
{
    struct GameState game;

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    ASSERT_EQ(1, talk_out(&game));
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
    ASSERT_EQ(1, talk_out(&game));
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_wanderer_waiting(void)
{
    struct GameState game;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_dialogue(&game, DIALOGUE_WANDERER);
    ASSERT_EQ(1, talk_out(&game));
    ASSERT_EQ(DIALOGUE_WANDERER, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_herbalist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    ASSERT_EQ(1, talk_out(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_HERBALIST, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_archivist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CATACOMBS, 0);
    ASSERT_EQ(1, talk_out(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_ARCHIVIST, game.dialogue);
    PASS();
}

TEST dialogue_cmd_reply_frog_invalid(void)
{
    struct GameState game;

    unit_game_fresh(&game, 13u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    talk_out(&game);
    ASSERT_EQ(1, reply_out(&game, 0));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_FROG, game.dialogue);
    PASS();
}

TEST dialogue_cmd_reply_watchman(void)
{
    struct GameState game;

    unit_game_fresh(&game, 14u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    talk_out(&game);
    ASSERT_EQ(1, reply_out(&game, 1));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_herbalist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 15u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ORCHARD, 0);
    talk_out(&game);
    ASSERT_EQ(1, reply_out(&game, 2));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_archivist(void)
{
    struct GameState game;

    unit_game_fresh(&game, 16u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CATACOMBS, 0);
    talk_out(&game);
    ASSERT_EQ(1, reply_out(&game, 3));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

SUITE(dialogue) {
    RUN_TEST(dialogue_npc_in_room);
    RUN_TEST(dialogue_frog_render_paths);
    RUN_TEST(dialogue_cmd_talk_watchman);
    RUN_TEST(dialogue_cmd_talk_nobody_camp);
    RUN_TEST(dialogue_cmd_reply_frog);
    RUN_TEST(dialogue_cmd_reply_not_dialogue);
    RUN_TEST(dialogue_cmd_talk_frog);
    RUN_TEST(dialogue_cmd_talk_bandit_blocks);
    RUN_TEST(dialogue_cmd_talk_wanderer_waiting);
    RUN_TEST(dialogue_cmd_talk_herbalist);
    RUN_TEST(dialogue_cmd_talk_archivist);
    RUN_TEST(dialogue_cmd_reply_frog_invalid);
    RUN_TEST(dialogue_cmd_reply_watchman);
    RUN_TEST(dialogue_cmd_reply_herbalist);
    RUN_TEST(dialogue_cmd_reply_archivist);
}
