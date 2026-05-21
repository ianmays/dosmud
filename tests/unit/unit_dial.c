#include "greatest.h"
#include "config.h"
#include "dialogue.h"
#include "game.h"
#include "world.h"
#include "unit_util.h"

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
    frog_dialogue_intro();
    frog_dialogue_branch(1);
    frog_dialogue_branch(2);
    frog_dialogue_branch(3);
    PASS();
}

TEST dialogue_cmd_talk_watchman(void)
{
    struct GameState game;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_TOWER, 0);
    ASSERT_EQ(1, dialogue_cmd_talk(&game));
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_NPC_WATCHMAN, game.dialogue);
    PASS();
}

TEST dialogue_cmd_talk_nobody_camp(void)
{
    struct GameState game;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, dialogue_cmd_talk(&game));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_frog(void)
{
    struct GameState game;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_POND, 0);
    dialogue_cmd_talk(&game);
    ASSERT_EQ(1, dialogue_cmd_reply(&game, 2));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST dialogue_cmd_reply_not_dialogue(void)
{
    struct GameState game;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(0, dialogue_cmd_reply(&game, 1));
    PASS();
}

SUITE(dialogue) {
    RUN_TEST(dialogue_npc_in_room);
    RUN_TEST(dialogue_frog_render_paths);
    RUN_TEST(dialogue_cmd_talk_watchman);
    RUN_TEST(dialogue_cmd_talk_nobody_camp);
    RUN_TEST(dialogue_cmd_reply_frog);
    RUN_TEST(dialogue_cmd_reply_not_dialogue);
}
