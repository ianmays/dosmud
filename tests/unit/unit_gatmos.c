#include "greatest.h"
#include "config.h"
#include "game.h"
#include "gatmos.h"
#include "grendr.h"
#include "items.h"
#include "platform.h"
#include "unit_util.h"

static void reset_camp(struct GameState *game)
{
    unit_game_fresh(game, 100u);
    game_reset_fixture_baseline(game, WORLD_ROOM_CAMP, 0);
}

TEST gatmos_seed_world_items(void)
{
    struct GameState game;

    reset_camp(&game);
    ASSERT_EQ(ITEM_STICK, game.room_item[WORLD_ROOM_CAMP][0]);
    ASSERT_EQ(ITEM_FISH, game.room_item[WORLD_ROOM_POND][0]);
    PASS();
}

TEST gatmos_focus_expiry(void)
{
    struct GameState game;
    u32 seed;
    int cleared;

    cleared = 0;
    for (seed = 0; seed < 500u; ++seed) {
        reset_camp(&game);
        game.env_focus_active = 1;
        game.env_focus_room = WORLD_ROOM_CAMP;
        game.env_focus_kind = GAME_ENV_RUSTLE;
        game.env_focus_expires_tick = 1;
        game.tick = 2;
        plat_seed_rng(seed);
        maybe_emit_atmosphere(&game);
        if (game.env_focus_active == 0) {
            cleared = 1;
            break;
        }
    }
    ASSERT_EQ(1, cleared);
    PASS();
}

TEST gatmos_animal_noise_tick_gate(void)
{
    struct GameState game;
    u32 seed;
    int emitted;

    emitted = 0;
    for (seed = 0; seed < 500u; ++seed) {
        reset_camp(&game);
        game.tick = 2;
        plat_seed_rng(seed);
        maybe_emit_animal_noise(&game);
        emitted = 1;
        break;
    }
    ASSERT_EQ(1, emitted);
    PASS();
}

TEST gatmos_atmosphere_branches(void)
{
    struct GameState game;
    u32 seed;
    int found_rustle;
    int found_creak;

    found_rustle = 0;
    found_creak = 0;
    for (seed = 0; seed < 500u; ++seed) {
        reset_camp(&game);
        plat_seed_rng(seed);
        maybe_emit_atmosphere(&game);
        if (game.env_focus_kind == GAME_ENV_RUSTLE) {
            found_rustle = 1;
        }
        if (game.env_focus_kind == GAME_ENV_CREAK) {
            found_creak = 1;
        }
        if (found_rustle && found_creak) {
            break;
        }
    }
    ASSERT_EQ(1, found_rustle);
    ASSERT_EQ(1, found_creak);
    PASS();
}

TEST gatmos_water_and_grit_focus(void)
{
    struct GameState game;
    u32 seed;
    int found_water;
    int found_grit;

    found_water = 0;
    found_grit = 0;
    for (seed = 0; seed < 800u; ++seed) {
        reset_camp(&game);
        plat_seed_rng(seed);
        maybe_emit_atmosphere(&game);
        if (game.env_focus_kind == GAME_ENV_WATER) {
            found_water = 1;
        }
        if (game.env_focus_kind == GAME_ENV_GRIT) {
            found_grit = 1;
        }
        if (found_water && found_grit) {
            break;
        }
    }
    ASSERT_EQ(1, found_water);
    ASSERT_EQ(1, found_grit);
    PASS();
}

TEST gatmos_room_item_spawn_gate(void)
{
    struct GameState game;
    u32 seed;
    int spawned;

    spawned = 0;
    for (seed = 0; seed < 1000u; ++seed) {
        reset_camp(&game);
        game.room_item[WORLD_ROOM_CAMP][0] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][3] = ITEM_NONE;
        plat_seed_rng(seed);
        maybe_emit_atmosphere(&game);
        if (game.room_item[WORLD_ROOM_CAMP][0] != ITEM_NONE ||
                game.room_item[WORLD_ROOM_CAMP][1] != ITEM_NONE) {
            spawned = 1;
            break;
        }
    }
    ASSERT_EQ(1, spawned);
    PASS();
}

TEST gatmos_rustle_berry_drop(void)
{
    struct GameState game;
    u32 seed;
    int found;

    found = 0;
    for (seed = 0; seed < 2000u; ++seed) {
        reset_camp(&game);
        game.room_item[WORLD_ROOM_CAMP][0] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][3] = ITEM_NONE;
        plat_seed_rng(seed);
        maybe_emit_atmosphere(&game);
        if (game.env_focus_kind == GAME_ENV_RUSTLE &&
                game.room_item[WORLD_ROOM_CAMP][0] == ITEM_BERRY) {
            found = 1;
            break;
        }
    }
    ASSERT_EQ(1, found);
    PASS();
}

SUITE(gatmos) {
    RUN_TEST(gatmos_seed_world_items);
    RUN_TEST(gatmos_focus_expiry);
    RUN_TEST(gatmos_animal_noise_tick_gate);
    RUN_TEST(gatmos_atmosphere_branches);
    RUN_TEST(gatmos_water_and_grit_focus);
    RUN_TEST(gatmos_room_item_spawn_gate);
    RUN_TEST(gatmos_rustle_berry_drop);
}
