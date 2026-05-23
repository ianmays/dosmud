#include <string.h>
#include "greatest.h"
#include "config.h"
#include "game.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"
#include "unit_util.h"

#define UNIT_GREND_BUF 256

static int unit_grend_render_bag(struct GameState *game, char *buf, int bufsize)
{
    if (!unit_capture_stdout_begin()) {
        return 0;
    }
    render_set_suppress(0);
    render_inv_bag(game);
    render_set_suppress(1);
    return unit_capture_stdout_end(buf, bufsize);
}

TEST grendr_bag_single(void)
{
    struct GameState game;
    char out[UNIT_GREND_BUF];

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_STICK);
    ASSERT(unit_grend_render_bag(&game, out, (int)sizeof(out)));
    ASSERT(strstr(out, " stick") != NULL);
    ASSERT(strstr(out, "[") == NULL);
    PASS();
}

TEST grendr_bag_stacked(void)
{
    struct GameState game;
    char out[UNIT_GREND_BUF];

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_inv_bag_add(&game, ITEM_BERRY);
    ASSERT(unit_grend_render_bag(&game, out, (int)sizeof(out)));
    ASSERT(strstr(out, " berry [2]") != NULL);
    PASS();
}

TEST grendr_bag_mixed_order(void)
{
    struct GameState game;
    char out[UNIT_GREND_BUF];

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_inv_bag_add(&game, ITEM_STICK);
    game_inv_bag_add(&game, ITEM_BERRY);
    ASSERT(unit_grend_render_bag(&game, out, (int)sizeof(out)));
    ASSERT(strstr(out, " berry [2], stick") != NULL);
    PASS();
}

TEST grendr_bag_empty(void)
{
    struct GameState game;
    char out[UNIT_GREND_BUF];

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT(unit_grend_render_bag(&game, out, (int)sizeof(out)));
    ASSERT(strstr(out, " empty") != NULL);
    PASS();
}

TEST grendr_bag_wielding(void)
{
    struct GameState game;
    char out[UNIT_GREND_BUF];

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    game_inv_bag_add(&game, ITEM_BERRY);
    game_inv_bag_add(&game, ITEM_BERRY);
    game.weapon_equipped = ITEM_STICK;
    ASSERT(unit_grend_render_bag(&game, out, (int)sizeof(out)));
    ASSERT(strstr(out, " berry [2]") != NULL);
    ASSERT(strstr(out, "Wielding: stick") != NULL);
    PASS();
}

SUITE(grendr) {
    RUN_TEST(grendr_bag_single);
    RUN_TEST(grendr_bag_stacked);
    RUN_TEST(grendr_bag_mixed_order);
    RUN_TEST(grendr_bag_empty);
    RUN_TEST(grendr_bag_wielding);
}
