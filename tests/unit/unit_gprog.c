#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gout.h"
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
    GameEventQueue out;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_event_queue_reset(&out);
    progression_gain_xp(&game, 5, &out);
    ASSERT_EQ(5, game.xp);
    ASSERT_EQ(CFG_START_LEVEL, game.level);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_XP_GAIN, out.events[0].kind);
    ASSERT_EQ(5, out.events[0].arg0);
    PASS();
}

TEST gprog_level_up_once(void)
{
    struct GameState game;
    GameEventQueue out;
    int needed;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    needed = game_xp_to_next_level(game.level);
    game_event_queue_reset(&out);
    progression_gain_xp(&game, needed, &out);
    ASSERT_EQ(2, game.level);
    ASSERT_EQ(CFG_START_MAX_HP + CFG_LEVELUP_MAX_HP_DELTA, game.max_hp);
    ASSERT_EQ(game.max_hp, game.player_hp);
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_XP_GAIN, out.events[0].kind);
    ASSERT_EQ(GAME_EVENT_STAT_CHANGE, out.events[1].kind);
    ASSERT_EQ(2, out.events[1].arg0);
    PASS();
}

TEST gprog_enemy_xp_reward_scales_with_level(void)
{
    ASSERT_EQ(CFG_COMBAT_KILL_XP_BASE + CFG_TEST_VICTORY_XP_SPREAD,
        progression_enemy_xp_reward(1, CFG_TEST_VICTORY_XP_SPREAD));
    ASSERT_EQ(CFG_COMBAT_KILL_XP_BASE + CFG_COMBAT_KILL_XP_PER_LEVEL +
            CFG_TEST_VICTORY_XP_SPREAD,
        progression_enemy_xp_reward(2, CFG_TEST_VICTORY_XP_SPREAD));
    PASS();
}

SUITE(gprog) {
    RUN_TEST(gprog_xp_to_next_level);
    RUN_TEST(gprog_gain_xp_no_level);
    RUN_TEST(gprog_level_up_once);
    RUN_TEST(gprog_enemy_xp_reward_scales_with_level);
}
