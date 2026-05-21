#include "greatest.h"
#include "config.h"
#include "game.h"
#include "genc.h"
#include "invent.h"
#include "items.h"
#include "unit_util.h"

TEST genc_skips_when_busy(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_set_mode_combat(&game);
    enemy_begin_encounter(&game);
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    PASS();
}

TEST genc_opens_dialogue(void)
{
    struct GameState game;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    enemy_begin_encounter(&game);
    ASSERT_EQ(GAME_MODE_DIALOGUE, game.mode);
    ASSERT_EQ(DIALOGUE_ENEMY, game.dialogue);
    PASS();
}

TEST genc_cmd_reply_fight(void)
{
    struct GameState game;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    enemy_begin_encounter(&game);
    ASSERT_EQ(1, genc_cmd_reply(&game, 1));
    ASSERT_EQ(GAME_MODE_COMBAT, game.mode);
    PASS();
}

TEST genc_cmd_reply_intimidate_ok(void)
{
    struct GameState game;
    int rolls[1];

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    enemy_begin_encounter(&game);
    rolls[0] = CFG_TEST_INTIMIDATE_OK;
    game_roll_inject_begin(&game, rolls, 1);
    ASSERT_EQ(1, genc_cmd_reply(&game, 3));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST genc_cmd_give_wrong_context(void)
{
    struct GameState game;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(1, genc_cmd_give(&game, ITEM_STICK));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

TEST genc_cmd_give_handover(void)
{
    struct GameState game;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    enemy_begin_encounter(&game);
    game_inv_bag_add(&game, ITEM_STICK);
    game.enemy_handover_pick = 1;
    ASSERT_EQ(1, genc_cmd_give(&game, ITEM_STICK));
    ASSERT_EQ(GAME_MODE_EXPLORE, game.mode);
    PASS();
}

SUITE(genc) {
    RUN_TEST(genc_skips_when_busy);
    RUN_TEST(genc_opens_dialogue);
    RUN_TEST(genc_cmd_reply_fight);
    RUN_TEST(genc_cmd_reply_intimidate_ok);
    RUN_TEST(genc_cmd_give_wrong_context);
    RUN_TEST(genc_cmd_give_handover);
}
