#include <string.h>
#include "greatest.h"
#include "config.h"
#include "fmt.h"
#include "game.h"
#include "invent.h"
#include "items.h"
#include "txtres.h"
#include "unit_util.h"

#define UNIT_FMT_BUF 128
#define UNIT_FMT_MAP_BUF CFG_FMT_MAP_MAX

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
    ASSERT_STR_EQ(" stick", out);
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
    ASSERT_EQ(10, len);
    ASSERT_STR_EQ(" berry [2]", out);
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
    ASSERT_EQ(17, len);
    ASSERT_STR_EQ(" berry [2], stick", out);
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

TEST fmt_bag_bad_args(void)
{
    struct GameState game;
    char out[8];

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(-1, fmt_inv_bag_items(&game, 0, 8));
    ASSERT_EQ(-1, fmt_inv_bag_items(&game, out, 0));
    PASS();
}

TEST fmt_bag_buf_too_small(void)
{
    struct GameState game;
    char out[4];

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_inv_bag_add(&game, ITEM_BERRY);
    ASSERT_EQ(-1, fmt_inv_bag_items(&game, out, (int)sizeof(out)));
    PASS();
}

TEST fmt_ground_empty(void)
{
    struct GameState game;
    char out[UNIT_FMT_BUF];
    int len;

    unit_game_fresh(&game, 10u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_NONE;
    game.room_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
    game.room_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
    game.room_item[WORLD_ROOM_CAMP][3] = ITEM_NONE;
    len = fmt_room_ground_items(&game, WORLD_ROOM_CAMP, out, (int)sizeof(out));
    ASSERT_EQ(0, len);
    ASSERT_EQ('\0', out[0]);
    PASS();
}

TEST fmt_ground_single(void)
{
    struct GameState game;
    char out[UNIT_FMT_BUF];
    int len;

    unit_game_fresh(&game, 11u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    len = fmt_room_ground_items(&game, WORLD_ROOM_CAMP, out, (int)sizeof(out));
    ASSERT_EQ(35, len);
    ASSERT_STR_EQ("On the ground: stick. (take stick)\n", out);
    PASS();
}

TEST fmt_ground_many(void)
{
    struct GameState game;
    char out[UNIT_FMT_BUF];
    int len;

    unit_game_fresh(&game, 12u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    game.room_item[WORLD_ROOM_CAMP][1] = ITEM_BERRY;
    len = fmt_room_ground_items(&game, WORLD_ROOM_CAMP, out, (int)sizeof(out));
    ASSERT_EQ(57, len);
    ASSERT_STR_EQ(
        "On the ground:\n"
        "  stick (take stick)\n"
        "  berry (take berry)\n",
        out);
    PASS();
}

TEST fmt_ground_bad_args(void)
{
    struct GameState game;
    char out[8];

    unit_game_fresh(&game, 13u);
    ASSERT_EQ(-1, fmt_room_ground_items(&game, WORLD_ROOM_CAMP, 0, 8));
    ASSERT_EQ(-1, fmt_room_ground_items(&game, WORLD_ROOM_CAMP, out, 0));
    ASSERT_EQ(-1, fmt_room_ground_items(0, WORLD_ROOM_CAMP, out, 8));
    ASSERT_EQ(-1, fmt_room_ground_items(&game, -1, out, 8));
    PASS();
}

TEST fmt_ground_buf_too_small(void)
{
    struct GameState game;
    char out[8];

    unit_game_fresh(&game, 14u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    ASSERT_EQ(-1, fmt_room_ground_items(&game, WORLD_ROOM_CAMP, out, (int)sizeof(out)));
    PASS();
}

TEST fmt_map_none_explored(void)
{
    struct GameState game;
    char out[UNIT_FMT_MAP_BUF];
    int len;

    int i;

    unit_game_fresh(&game, 20u);
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        game.room_explored[i] = 0;
    }
    len = fmt_exploration_map(&game, out, (int)sizeof(out));
    ASSERT_STR_EQ(TXT_MAP_NONE_EXPLORED, out);
    ASSERT_EQ((int)strlen(TXT_MAP_NONE_EXPLORED), len);
    PASS();
}

TEST fmt_map_camp_only(void)
{
    struct GameState game;
    char out[UNIT_FMT_MAP_BUF];
    int len;
    static const char expect[] =
        "Explored locations:\n"
        "@\n"
        "(@ = you, letter = first initial of a visited place.)\n";

    unit_game_fresh(&game, 21u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_explored[WORLD_ROOM_CAMP] = 1;
    len = fmt_exploration_map(&game, out, (int)sizeof(out));
    ASSERT_EQ((int)(sizeof(expect) - 1), len);
    ASSERT_STR_EQ(expect, out);
    PASS();
}

TEST fmt_map_camp_and_road(void)
{
    struct GameState game;
    char out[UNIT_FMT_MAP_BUF];
    int len;
    static const char expect[] =
        "Explored locations:\n"
        "@\n"
        "C\n"
        "(@ = you, letter = first initial of a visited place.)\n";

    unit_game_fresh(&game, 22u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_ROAD, 0);
    game.room_explored[WORLD_ROOM_CAMP] = 1;
    game.room_explored[WORLD_ROOM_ROAD] = 1;
    len = fmt_exploration_map(&game, out, (int)sizeof(out));
    ASSERT_EQ((int)(sizeof(expect) - 1), len);
    ASSERT_STR_EQ(expect, out);
    PASS();
}

TEST fmt_map_bad_args(void)
{
    struct GameState game;
    char out[8];

    unit_game_fresh(&game, 23u);
    ASSERT_EQ(-1, fmt_exploration_map(&game, 0, 8));
    ASSERT_EQ(-1, fmt_exploration_map(&game, out, 0));
    ASSERT_EQ(-1, fmt_exploration_map(0, out, 8));
    PASS();
}

TEST fmt_map_buf_too_small(void)
{
    struct GameState game;
    char out[8];

    unit_game_fresh(&game, 24u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game.room_explored[WORLD_ROOM_CAMP] = 1;
    ASSERT_EQ(-1, fmt_exploration_map(&game, out, (int)sizeof(out)));
    PASS();
}

TEST fmt_map_all_explored_fits(void)
{
    struct GameState game;
    char out[UNIT_FMT_MAP_BUF];
    int len;
    int i;

    unit_game_fresh(&game, 25u);
    for (i = 0; i < game.world.room_count; ++i) {
        game.room_explored[i] = 1;
    }
    len = fmt_exploration_map(&game, out, (int)sizeof(out));
    ASSERT(len >= 0);
    ASSERT(len < (int)sizeof(out));
    PASS();
}

SUITE(fmt) {
    RUN_TEST(fmt_bag_single);
    RUN_TEST(fmt_bag_stacked);
    RUN_TEST(fmt_bag_mixed_order);
    RUN_TEST(fmt_bag_empty);
    RUN_TEST(fmt_bag_bad_args);
    RUN_TEST(fmt_bag_buf_too_small);
    RUN_TEST(fmt_ground_empty);
    RUN_TEST(fmt_ground_single);
    RUN_TEST(fmt_ground_many);
    RUN_TEST(fmt_ground_bad_args);
    RUN_TEST(fmt_ground_buf_too_small);
    RUN_TEST(fmt_map_none_explored);
    RUN_TEST(fmt_map_camp_only);
    RUN_TEST(fmt_map_camp_and_road);
    RUN_TEST(fmt_map_bad_args);
    RUN_TEST(fmt_map_buf_too_small);
    RUN_TEST(fmt_map_all_explored_fits);
}
