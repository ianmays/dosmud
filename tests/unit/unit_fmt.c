#include <string.h>
#include "greatest.h"
#include "config.h"
#include "fmt.h"
#include "game.h"
#include "invent.h"
#include "items.h"
#include "unit_util.h"

#define UNIT_FMT_BUF 128

TEST fmt_bag_single(void)
{
    struct GameState game;
    char out[UNIT_FMT_BUF];
    int len;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    len = fmt_inv_bag_items(&game, out, (int)sizeof(out));
    ASSERT_EQ(6, len);
    ASSERT(strstr(out, " stick") != NULL);
    ASSERT(strstr(out, "[") == NULL);
    PASS();
}

TEST fmt_bag_stacked(void)
{
    struct GameState game;
    char out[UNIT_FMT_BUF];
    int len;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_inv_bag_add(&game, ITEM_BERRY);
    len = fmt_inv_bag_items(&game, out, (int)sizeof(out));
    ASSERT(len > 0);
    ASSERT(strstr(out, " berry [2]") != NULL);
    PASS();
}

TEST fmt_bag_mixed_order(void)
{
    struct GameState game;
    char out[UNIT_FMT_BUF];
    int len;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_BERRY);
    len = fmt_inv_bag_items(&game, out, (int)sizeof(out));
    ASSERT(len > 0);
    ASSERT(strstr(out, " berry [2], stick") != NULL);
    PASS();
}

TEST fmt_bag_empty(void)
{
    struct GameState game;
    char out[UNIT_FMT_BUF];
    int len;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    out[0] = 'x';
    len = fmt_inv_bag_items(&game, out, (int)sizeof(out));
    ASSERT_EQ(0, len);
    ASSERT_EQ('\0', out[0]);
    PASS();
}

SUITE(fmt) {
    RUN_TEST(fmt_bag_single);
    RUN_TEST(fmt_bag_stacked);
    RUN_TEST(fmt_bag_mixed_order);
    RUN_TEST(fmt_bag_empty);
}
