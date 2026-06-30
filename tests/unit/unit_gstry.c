#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gstory.h"
#include "invent.h"
#include "items.h"
#include "unit_util.h"
#include "world.h"

TEST gstry_fetch_scene_not_started(void)
{
    ASSERT_EQ(STORY_FETCH_NOT_STARTED,
        story_fetch_scene(STORY_PROGRESS_NONE, 0));
    ASSERT_EQ(STORY_FETCH_NOT_STARTED,
        story_fetch_scene(STORY_PROGRESS_NONE, 1));
    PASS();
}

TEST gstry_fetch_scene_active(void)
{
    ASSERT_EQ(STORY_FETCH_ACTIVE,
        story_fetch_scene(STORY_PROGRESS_ACTIVE, 0));
    ASSERT_EQ(STORY_FETCH_READY,
        story_fetch_scene(STORY_PROGRESS_ACTIVE, 1));
    PASS();
}

TEST gstry_fetch_scene_done(void)
{
    ASSERT_EQ(STORY_FETCH_DONE,
        story_fetch_scene(STORY_PROGRESS_DONE, 0));
    ASSERT_EQ(STORY_FETCH_DONE,
        story_fetch_scene(STORY_PROGRESS_DONE, 1));
    PASS();
}

TEST gstry_fetch_scene_invalid_progress(void)
{
    ASSERT_EQ(STORY_FETCH_NOT_STARTED, story_fetch_scene(99, 0));
    PASS();
}

TEST gstry_world_has_item_bag_only(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(0, story_world_has_item(&game, ITEM_MARSH_ROOT));
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_MARSH_ROOT));
    ASSERT_EQ(1, story_world_has_item(&game, ITEM_MARSH_ROOT));
    PASS();
}

TEST gstry_world_has_item_room_only(void)
{
    struct GameState game;

    unit_game_fresh(&game, 2u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_MARSH, 0);
    game.room_item[WORLD_ROOM_MARSH][0] = ITEM_MARSH_ROOT;
    ASSERT_EQ(1, story_world_has_item(&game, ITEM_MARSH_ROOT));
    PASS();
}

TEST gstry_world_has_item_neither(void)
{
    struct GameState game;

    unit_game_fresh(&game, 3u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_CAMP, 0);
    ASSERT_EQ(0, story_world_has_item(&game, ITEM_MARSH_ROOT));
    PASS();
}

TEST gstry_seed_recoverable_already_carried(void)
{
    struct GameState game;
    int spawned;
    int slot;

    unit_game_fresh(&game, 4u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_MARSH, 0);
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        game.room_item[WORLD_ROOM_MARSH][slot] = ITEM_NONE;
    }
    ASSERT_EQ(1, game_inv_bag_add(&game, ITEM_MARSH_ROOT));
    spawned = 0;
    ASSERT_EQ(1, story_seed_recoverable_item(&game, WORLD_ROOM_MARSH,
        ITEM_MARSH_ROOT, &spawned));
    ASSERT_EQ(1, spawned);
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        ASSERT_EQ(ITEM_NONE, game.room_item[WORLD_ROOM_MARSH][slot]);
    }
    PASS();
}

TEST gstry_seed_recoverable_already_on_ground(void)
{
    struct GameState game;
    int spawned;

    unit_game_fresh(&game, 5u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_MARSH, 0);
    game.room_item[WORLD_ROOM_MARSH][1] = ITEM_MARSH_ROOT;
    spawned = 0;
    ASSERT_EQ(1, story_seed_recoverable_item(&game, WORLD_ROOM_MARSH,
        ITEM_MARSH_ROOT, &spawned));
    ASSERT_EQ(1, spawned);
    ASSERT_EQ(ITEM_MARSH_ROOT, game.room_item[WORLD_ROOM_MARSH][1]);
    PASS();
}

TEST gstry_seed_recoverable_places_in_empty_slot(void)
{
    struct GameState game;
    int spawned;
    int slot;

    unit_game_fresh(&game, 6u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_MARSH, 0);
    spawned = 0;
    ASSERT_EQ(1, story_seed_recoverable_item(&game, WORLD_ROOM_MARSH,
        ITEM_MARSH_ROOT, &spawned));
    ASSERT_EQ(1, spawned);
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        if (game.room_item[WORLD_ROOM_MARSH][slot] == ITEM_MARSH_ROOT) {
            PASS();
        }
    }
    FAILm("marsh-root was not placed in marsh room");
}

TEST gstry_seed_recoverable_full_room(void)
{
    struct GameState game;
    int spawned;
    int slot;

    unit_game_fresh(&game, 7u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_MARSH, 0);
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        game.room_item[WORLD_ROOM_MARSH][slot] = ITEM_REED;
    }
    spawned = 1;
    ASSERT_EQ(0, story_seed_recoverable_item(&game, WORLD_ROOM_MARSH,
        ITEM_MARSH_ROOT, &spawned));
    ASSERT_EQ(0, spawned);
    PASS();
}

TEST gstry_seed_recoverable_null_spawned_out(void)
{
    struct GameState game;

    unit_game_fresh(&game, 8u);
    game_reset_fixture_baseline(&game, WORLD_ROOM_MARSH, 0);
    ASSERT_EQ(1, story_seed_recoverable_item(&game, WORLD_ROOM_MARSH,
        ITEM_MARSH_ROOT, 0));
    PASS();
}

SUITE(gstry) {
    RUN_TEST(gstry_fetch_scene_not_started);
    RUN_TEST(gstry_fetch_scene_active);
    RUN_TEST(gstry_fetch_scene_done);
    RUN_TEST(gstry_fetch_scene_invalid_progress);
    RUN_TEST(gstry_world_has_item_bag_only);
    RUN_TEST(gstry_world_has_item_room_only);
    RUN_TEST(gstry_world_has_item_neither);
    RUN_TEST(gstry_seed_recoverable_already_carried);
    RUN_TEST(gstry_seed_recoverable_already_on_ground);
    RUN_TEST(gstry_seed_recoverable_places_in_empty_slot);
    RUN_TEST(gstry_seed_recoverable_full_room);
    RUN_TEST(gstry_seed_recoverable_null_spawned_out);
}
