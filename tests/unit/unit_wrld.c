#include "greatest.h"
#include "config.h"
#include "world.h"
#include "unit_util.h"

TEST world_apply_graph_move(void)
{
    struct GameState game;

    unit_game_fresh(&game, 1u);
    ASSERT_EQ(1, world_can_move(&game.world, WORLD_ROOM_CAMP, DIR_NORTH));
    ASSERT_EQ(WORLD_ROOM_ROAD, world_move(&game.world, WORLD_ROOM_CAMP, DIR_NORTH));
    ASSERT_EQ(0, world_can_move(&game.world, WORLD_ROOM_CAMP, DIR_EAST));
    PASS();
}

TEST world_invalid_room_and_dir(void)
{
    struct World world;

    world_init(&world);
    ASSERT_EQ(0, world_can_move(&world, -1, DIR_NORTH));
    ASSERT_EQ(0, world_can_move(&world, 99, DIR_NORTH));
    ASSERT_EQ(0, world_can_move(&world, WORLD_ROOM_CAMP, DIR_NONE));
    ASSERT_EQ(WORLD_ROOM_CAMP, world_move(&world, WORLD_ROOM_CAMP, DIR_EAST));
    PASS();
}

TEST world_dir_and_animal_fallback(void)
{
    struct GameState game;

    unit_game_fresh(&game, 2u);
    ASSERT_STR_EQ("north", world_dir_name(DIR_NORTH));
    ASSERT(world_room_animal(&game.world, -1) != 0);
    ASSERT(world_room_animal(&game.world, WORLD_ROOM_CAMP) != 0);
    PASS();
}

TEST world_step_no_crash(void)
{
    struct World world;

    world_init(&world);
    world_step(&world, 5);
    PASS();
}

TEST world_animal_noise_fallback(void)
{
    struct GameState game;

    unit_game_fresh(&game, 3u);
    ASSERT(world_room_animal_noise(&game.world, -1) != 0);
    PASS();
}

SUITE(world) {
    RUN_TEST(world_apply_graph_move);
    RUN_TEST(world_invalid_room_and_dir);
    RUN_TEST(world_dir_and_animal_fallback);
    RUN_TEST(world_step_no_crash);
    RUN_TEST(world_animal_noise_fallback);
}
