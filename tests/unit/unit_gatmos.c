#include "greatest.h"
#include <stdlib.h>
#include "config.h"
#include "game.h"
#include "gatmos.h"
#include "gout.h"
#include "grendr.h"
#include "items.h"
#include "platform.h"
#include "unit_util.h"

static void reset_camp(struct GameState *game)
{
    unit_game_fresh(game, 100u);
    game_reset_fixture_baseline(game, WORLD_ROOM_CAMP, 0);
}

static void emit_atmosphere(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    maybe_emit_atmosphere(game, out);
}

static void emit_noise(struct GameState *game, GameEventQueue *out)
{
    game_event_queue_reset(out);
    maybe_emit_animal_noise(game, out);
}

static int inspect_focus(struct GameState *game, int item_arg,
                         GameEventQueue *out)
{
    game_event_queue_reset(out);
    return gatmos_cmd_inspect(game, item_arg, out);
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
    GameEventQueue out;
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
        emit_atmosphere(&game, &out);
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
    GameEventQueue out;
    u32 seed;
    int r_skip;
    int r_first;
    int r_after;
    int r_second;

    /* Observe plat_rand draw ordering through the animal-noise tick gate. */
    reset_camp(&game);
    game.tick = 1;
    emit_noise(&game, &out);
    r_skip = plat_rand();

    reset_camp(&game);
    r_first = plat_rand();
    ASSERT_EQ(r_skip, r_first);

    reset_camp(&game);
    game.tick = 2;
    emit_noise(&game, &out);
    r_after = plat_rand();

    reset_camp(&game);
    r_first = plat_rand();
    r_second = plat_rand();
    ASSERT_EQ(r_after, r_second);
    if (r_skip == r_after) {
        FAILm("tick period gate should call rand() only when tick matches");
    }

    for (seed = 0; seed < 500u; ++seed) {
        reset_camp(&game);
        game.tick = 2;
        plat_seed_rng(seed);
        emit_noise(&game, &out);
    }
    PASS();
}

TEST gatmos_atmosphere_branches(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 seed;
    int found_rustle;
    int found_creak;

    found_rustle = 0;
    found_creak = 0;
    for (seed = 0; seed < 500u; ++seed) {
        reset_camp(&game);
        plat_seed_rng(seed);
        emit_atmosphere(&game, &out);
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
    GameEventQueue out;
    u32 seed;
    int found_water;
    int found_grit;

    found_water = 0;
    found_grit = 0;
    for (seed = 0; seed < 800u; ++seed) {
        reset_camp(&game);
        plat_seed_rng(seed);
        emit_atmosphere(&game, &out);
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
    GameEventQueue out;
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
        emit_atmosphere(&game, &out);
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
    GameEventQueue out;
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
        emit_atmosphere(&game, &out);
        if (game.env_focus_kind == GAME_ENV_RUSTLE &&
                game.room_item[WORLD_ROOM_CAMP][0] == ITEM_BERRY) {
            found = 1;
            break;
        }
    }
    ASSERT_EQ(1, found);
    PASS();
}

TEST gatmos_environment_gust_event(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 seed;
    int found;

    found = 0;
    for (seed = 0; seed < 500u; ++seed) {
        reset_camp(&game);
        plat_seed_rng(seed);
        emit_atmosphere(&game, &out);
        if (out.count == 1 &&
                out.events[0].kind == GAME_EVENT_ENVIRONMENT &&
                out.events[0].arg0 == GAME_ENV_EVENT_GUST) {
            found = 1;
            break;
        }
    }
    ASSERT_EQ(1, found);
    PASS();
}

TEST gatmos_ambient_noise_event(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 seed;
    int found;

    found = 0;
    for (seed = 0; seed < 500u; ++seed) {
        reset_camp(&game);
        game.tick = 2;
        plat_seed_rng(seed);
        emit_noise(&game, &out);
        if (out.count == 1 &&
                out.events[0].kind == GAME_EVENT_AMBIENT_NOISE &&
                out.events[0].text != 0 && out.events[0].text[0] != '\0') {
            found = 1;
            break;
        }
    }
    ASSERT_EQ(1, found);
    PASS();
}

TEST gatmos_item_presence_event(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 seed;
    int found;

    found = 0;
    for (seed = 0; seed < 1000u; ++seed) {
        reset_camp(&game);
        game.room_item[WORLD_ROOM_CAMP][0] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][1] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][2] = ITEM_NONE;
        game.room_item[WORLD_ROOM_CAMP][3] = ITEM_NONE;
        plat_seed_rng(seed);
        emit_atmosphere(&game, &out);
        if (out.count == 1 &&
                out.events[0].kind == GAME_EVENT_ITEM_PRESENCE &&
                out.events[0].arg0 != ITEM_NONE) {
            found = 1;
            break;
        }
    }
    ASSERT_EQ(1, found);
    PASS();
}

TEST gatmos_tick_event_order(void)
{
    struct GameState game;
    GameEventQueue out;
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
        emit_atmosphere(&game, &out);
        if (out.count == 2 &&
                out.events[0].kind == GAME_EVENT_ENVIRONMENT &&
                out.events[0].arg0 == GAME_ENV_EVENT_RUSTLE &&
                out.events[1].kind == GAME_EVENT_ENVIRONMENT &&
                out.events[1].arg0 == GAME_ENV_EVENT_BERRY_DROP) {
            found = 1;
            break;
        }
    }
    ASSERT_EQ(1, found);
    PASS();
}

TEST gatmos_cmd_inspect_focus(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.env_focus_active = 1;
    game.env_focus_room = WORLD_ROOM_CAMP;
    game.env_focus_kind = GAME_ENV_WATER;
    game.env_focus_expires_tick = game.tick + 10;
    ASSERT_EQ(1, inspect_focus(&game, GAME_ENV_WATER, &out));
    ASSERT_EQ(2, out.count);
    ASSERT_EQ(GAME_EVENT_OBSERVATION, out.events[0].kind);
    ASSERT_EQ(GAME_OBS_OUTCOME_WATER, out.events[0].arg0);
    ASSERT_EQ(GAME_EVENT_ENV_MENU, out.events[1].kind);
    ASSERT_EQ(GAME_ENV_WATER, out.events[1].arg0);
    ASSERT_EQ(0, game.env_focus_active);
    ASSERT_EQ(1, game.env_interact_active);
    ASSERT_EQ(GAME_ENV_WATER, game.env_interact_kind);
    PASS();
}

TEST gatmos_cmd_inspect_none(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    ASSERT_EQ(1, inspect_focus(&game, 0, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_OBSERVATION, out.events[0].kind);
    ASSERT_EQ(GAME_OBS_OUTCOME_NOTHING, out.events[0].arg0);
    ASSERT_EQ(0, game.env_focus_active);
    PASS();
}

TEST gatmos_cmd_inspect_wrong_focus(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.env_focus_active = 1;
    game.env_focus_room = WORLD_ROOM_CAMP;
    game.env_focus_kind = GAME_ENV_RUSTLE;
    game.env_focus_expires_tick = game.tick + 10;
    ASSERT_EQ(1, inspect_focus(&game, GAME_ENV_WATER, &out));
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_OBSERVATION, out.events[0].kind);
    ASSERT_EQ(GAME_OBS_OUTCOME_WRONG_FOCUS, out.events[0].arg0);
    ASSERT_EQ(1, game.env_focus_active);
    PASS();
}

TEST gatmos_cmd_env_reply_water_drink(void)
{
    struct GameState game;
    GameEventQueue out;
    int hp_before;

    reset_camp(&game);
    game.player_hp = game.max_hp - 2;
    hp_before = game.player_hp;
    game.env_interact_active = 1;
    game.env_interact_kind = GAME_ENV_WATER;
    game.env_interact_room = WORLD_ROOM_CAMP;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, gatmos_cmd_env_reply(&game, 2, &out));
    ASSERT_EQ(0, game.env_interact_active);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENV_RESULT, out.events[0].kind);
    ASSERT_EQ(GAME_ENV_WATER, out.events[0].arg0);
    ASSERT_EQ(2, out.events[0].arg1);
    ASSERT_EQ(GAME_ENV_RESULT_DETAIL_HEALED, out.events[0].arg2);
    ASSERT_EQ(hp_before + CFG_ENV_WATER_HEAL_AMOUNT, game.player_hp);
    PASS();
}

TEST gatmos_cmd_env_reply_invalid_choice(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.env_interact_active = 1;
    game.env_interact_kind = GAME_ENV_RUSTLE;
    game.env_interact_room = WORLD_ROOM_CAMP;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, gatmos_cmd_env_reply(&game, 9, &out));
    ASSERT_EQ(1, game.env_interact_active);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_PICK_123, out.events[0].arg0);
    PASS();
}

TEST gatmos_env_dismiss_clears_state(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.env_interact_active = 1;
    game.env_interact_kind = GAME_ENV_GRIT;
    game.env_interact_room = WORLD_ROOM_CAMP;
    game_event_queue_reset(&out);
    gatmos_env_dismiss(&game, &out);
    ASSERT_EQ(0, game.env_interact_active);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED, out.events[0].arg0);
    PASS();
}

TEST gatmos_cmd_env_reply_room_mismatch_dismisses(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.env_interact_active = 1;
    game.env_interact_kind = GAME_ENV_WATER;
    game.env_interact_room = WORLD_ROOM_CAMP;
    game.player.room_id = WORLD_ROOM_ROAD;
    game_event_queue_reset(&out);
    ASSERT_EQ(1, gatmos_cmd_env_reply(&game, 1, &out));
    ASSERT_EQ(0, game.env_interact_active);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_DIALOGUE_GUARD, out.events[0].kind);
    ASSERT_EQ(GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED, out.events[0].arg0);
    PASS();
}

TEST gatmos_queue_restored_menu_requeues_active_state(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.env_interact_active = 1;
    game.env_interact_kind = GAME_ENV_WATER;
    game.env_interact_room = WORLD_ROOM_CAMP;
    game_event_queue_reset(&out);
    gatmos_queue_restored_menu(&game, &out);
    ASSERT_EQ(1, game.env_interact_active);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENV_MENU, out.events[0].kind);
    ASSERT_EQ(GAME_ENV_WATER, out.events[0].arg0);
    ASSERT_EQ(WORLD_ROOM_CAMP, out.events[0].arg1);
    PASS();
}

TEST gatmos_queue_restored_menu_clears_stale_room_pin(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.env_interact_active = 1;
    game.env_interact_kind = GAME_ENV_WATER;
    game.env_interact_room = WORLD_ROOM_ROAD;
    game.player.room_id = WORLD_ROOM_CAMP;
    game_event_queue_reset(&out);
    gatmos_queue_restored_menu(&game, &out);
    ASSERT_EQ(0, game.env_interact_active);
    ASSERT_EQ(0, out.count);
    PASS();
}

/* #51 weather: hash-only rolls (seed/tick); atmosphere tests use plat_rand inject. */
TEST gatmos_weather_tick_rain_at_scheduled_check(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.seed = 1234u; /* pins weather_roll for scheduled first check */
    game.tick = (u32)CFG_WEATHER_INITIAL_DELAY_TICKS;
    game.weather_kind = GAME_WEATHER_NONE;
    game.weather_expires_tick = (u32)CFG_WEATHER_INITIAL_DELAY_TICKS;
    game_event_queue_reset(&out);
    gatmos_weather_tick(&game, &out);
    ASSERT_EQ(GAME_WEATHER_RAIN, game.weather_kind);
    ASSERT_EQ((u32)CFG_WEATHER_INITIAL_DELAY_TICKS + (u32)CFG_WEATHER_DURATION_TICKS,
        game.weather_expires_tick);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENVIRONMENT, out.events[0].kind);
    ASSERT_EQ(GAME_ENV_EVENT_WEATHER_RAIN, out.events[0].arg0);
    PASS();
}

TEST gatmos_weather_tick_skips_before_expiry(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.tick = 1;
    game.weather_expires_tick = (u32)CFG_WEATHER_INITIAL_DELAY_TICKS;
    game_event_queue_reset(&out);
    gatmos_weather_tick(&game, &out);
    ASSERT_EQ(GAME_WEATHER_NONE, game.weather_kind);
    ASSERT_EQ(0, out.count);
    PASS();
}

TEST gatmos_weather_wind_biases_gust_roll(void)
{
    struct GameState game;
    GameEventQueue out;
    int inject;

    reset_camp(&game);
    game.weather_kind = GAME_WEATHER_WIND;
    inject = 40; /* plat_rand path in maybe_emit_atmosphere, not weather_roll */
    game_roll_inject_begin(&game, &inject, 1);
    game_event_queue_reset(&out);
    maybe_emit_atmosphere(&game, &out);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_EVENT_ENVIRONMENT, out.events[0].kind);
    ASSERT_EQ(GAME_ENV_EVENT_GUST, out.events[0].arg0);
    PASS();
}

TEST gatmos_weather_fog_blocks_roaming_encounter_roll(void)
{
    struct GameState game;

    reset_camp(&game);
    game.seed = 1234u;
    game.tick = 4; /* hash-only fog encounter block at this seed/tick */
    game.weather_kind = GAME_WEATHER_FOG;
    ASSERT_EQ(1, gatmos_weather_blocks_roaming_encounter(&game));
    game.weather_kind = GAME_WEATHER_RAIN;
    ASSERT_EQ(0, gatmos_weather_blocks_roaming_encounter(&game));
    PASS();
}

TEST gatmos_weather_fog_allows_roaming_encounter_low_roll(void)
{
    struct GameState game;

    reset_camp(&game);
    game.seed = 0u;
    game.tick = 5;
    game.weather_kind = GAME_WEATHER_FOG;
    ASSERT_EQ(0, gatmos_weather_blocks_roaming_encounter(&game));
    PASS();
}

TEST gatmos_weather_rain_biases_water_roll(void)
{
    struct GameState game;
    GameEventQueue out;
    u32 seed;
    int found;

    found = 0;
    for (seed = 0; seed < 2000u; ++seed) {
        reset_camp(&game);
        game.weather_kind = GAME_WEATHER_RAIN;
        plat_seed_rng(seed);
        emit_atmosphere(&game, &out);
        if (out.count > 0 && out.events[0].arg0 == GAME_ENV_EVENT_WATER) {
            reset_camp(&game);
            game.weather_kind = GAME_WEATHER_NONE;
            plat_seed_rng(seed);
            game_event_queue_reset(&out);
            maybe_emit_atmosphere(&game, &out);
            if (out.count > 0 && out.events[0].arg0 != GAME_ENV_EVENT_WATER) {
                found = 1;
                break;
            }
        }
    }
    ASSERT_EQ(1, found);
    PASS();
}

TEST gatmos_weather_tick_clear_after_wind(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.seed = 1234u;
    game.tick = 54;
    game.weather_kind = GAME_WEATHER_WIND;
    game.weather_expires_tick = 54;
    game_event_queue_reset(&out);
    gatmos_weather_tick(&game, &out);
    ASSERT_EQ(GAME_WEATHER_NONE, game.weather_kind);
    ASSERT_EQ(1, out.count);
    ASSERT_EQ(GAME_ENV_EVENT_WEATHER_CLEAR, out.events[0].arg0);
    PASS();
}

TEST gatmos_weather_roll_u32_wrap_tick13(void)
{
    struct GameState game;
    GameEventQueue out;

    reset_camp(&game);
    game.seed = 1234u;
    game.tick = 13;
    game.weather_kind = GAME_WEATHER_NONE;
    game.weather_expires_tick = 13;
    game_event_queue_reset(&out);
    gatmos_weather_tick(&game, &out);
    ASSERT_EQ(GAME_WEATHER_WIND, game.weather_kind);
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
    RUN_TEST(gatmos_environment_gust_event);
    RUN_TEST(gatmos_ambient_noise_event);
    RUN_TEST(gatmos_item_presence_event);
    RUN_TEST(gatmos_tick_event_order);
    RUN_TEST(gatmos_cmd_inspect_focus);
    RUN_TEST(gatmos_cmd_inspect_none);
    RUN_TEST(gatmos_cmd_inspect_wrong_focus);
    RUN_TEST(gatmos_cmd_env_reply_water_drink);
    RUN_TEST(gatmos_cmd_env_reply_invalid_choice);
    RUN_TEST(gatmos_env_dismiss_clears_state);
    RUN_TEST(gatmos_cmd_env_reply_room_mismatch_dismisses);
    RUN_TEST(gatmos_queue_restored_menu_requeues_active_state);
    RUN_TEST(gatmos_queue_restored_menu_clears_stale_room_pin);
    RUN_TEST(gatmos_weather_tick_rain_at_scheduled_check);
    RUN_TEST(gatmos_weather_tick_skips_before_expiry);
    RUN_TEST(gatmos_weather_wind_biases_gust_roll);
    RUN_TEST(gatmos_weather_fog_blocks_roaming_encounter_roll);
    RUN_TEST(gatmos_weather_fog_allows_roaming_encounter_low_roll);
    RUN_TEST(gatmos_weather_rain_biases_water_roll);
    RUN_TEST(gatmos_weather_tick_clear_after_wind);
    RUN_TEST(gatmos_weather_roll_u32_wrap_tick13);
}
