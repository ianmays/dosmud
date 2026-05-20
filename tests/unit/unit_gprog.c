#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gprog.h"
#include "unit_util.h"

TEST gprog_xp_to_next_level(void)
{
    ASSERT_EQ(CFG_XP_LEVEL_BASE, game_xp_to_next_level(1));
    ASSERT_EQ(CFG_XP_LEVEL_BASE + CFG_XP_LEVEL_PER_LEVEL, game_xp_to_next_level(2));
    PASS();
}

TEST gprog_gain_xp_no_level(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    progression_gain_xp(&game, 5);
    ASSERT_EQ(5, game.xp);
    ASSERT_EQ(CFG_START_LEVEL, game.level);
    PASS();
}

TEST gprog_level_up_once(void)
{
    struct GameState game;
    int needed;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    needed = game_xp_to_next_level(game.level);
    progression_gain_xp(&game, needed);
    ASSERT_EQ(2, game.level);
    ASSERT_EQ(CFG_START_MAX_HP + CFG_LEVELUP_MAX_HP_DELTA, game.max_hp);
    ASSERT_EQ(game.max_hp, game.player_hp);
    PASS();
}

SUITE(gprog) {
    RUN_TEST(gprog_xp_to_next_level);
    RUN_TEST(gprog_gain_xp_no_level);
    RUN_TEST(gprog_level_up_once);
}
