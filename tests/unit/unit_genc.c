#include "greatest.h"
#include "game.h"
#include "genc.h"
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

SUITE(genc) {
    RUN_TEST(genc_skips_when_busy);
    RUN_TEST(genc_opens_dialogue);
}
