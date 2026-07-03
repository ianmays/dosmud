#include <stdlib.h>
#include "gatmos.h"
#include "config.h"
#include "game.h"
#include "gout.h"
#include "invent.h"
#include "items.h"
#include "platform.h"
#include "world.h"

/*
 * Ambient systems live here: starter ground items, atmospheric focus, and
 * room-scoped incidental events.
 * #161: queues GAME_EVENT_ENVIRONMENT / AMBIENT_NOISE / ITEM_PRESENCE /
 * OBSERVATION; grendr maps to text.
 * #7: post-inspect follow-up menus via env_interact_* and ENV_MENU/RESULT.
 */

static void push_environment(GameEventQueue *out, int kind)
{
    game_event_push(out, GAME_EVENT_ENVIRONMENT, kind, 0, 0, 0, 0);
}

static void push_ambient_noise(GameEventQueue *out, const char *line)
{
    game_event_push(out, GAME_EVENT_AMBIENT_NOISE, 0, 0, 0, 0, line);
}

static void push_item_presence(GameEventQueue *out, int item_id,
                               const char *name)
{
    game_event_push(out, GAME_EVENT_ITEM_PRESENCE, item_id, 0, 0, 0, name);
}

static void push_observation(GameEventQueue *out, int outcome)
{
    game_event_push(out, GAME_EVENT_OBSERVATION, outcome, 0, 0, 0, 0);
}

/*
 * Post-inspect follow-up (#7): env_interact_* is explore-mode state owned here;
 * game.c routes CMD_REPLY and dismisses on other explore verbs.
 */
static void push_env_menu(GameEventQueue *out, int kind, int room_id)
{
    game_event_push(out, GAME_EVENT_ENV_MENU, kind, room_id, 0, 0, 0);
}

static void push_env_result(GameEventQueue *out, int kind, int choice,
                            int detail)
{
    game_event_push(out, GAME_EVENT_ENV_RESULT, kind, choice, detail, 0, 0);
}

static void push_pick_guard(GameEventQueue *out, int max_choice)
{
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_PICK_123, max_choice, 0, 0, 0);
}

static int env_max_choice(int kind)
{
    if (kind == GAME_ENV_WATER) {
        return 3;
    }
    return 2;
}

static int env_is_leave_choice(int kind, int choice)
{
    return choice == env_max_choice(kind);
}

void gatmos_env_clear_interact(struct GameState *game)
{
    game->env_interact_active = 0;
    game->env_interact_kind = GAME_ENV_NONE;
    game->env_interact_room = -1;
}

static void env_open_menu(struct GameState *game, int kind, GameEventQueue *out)
{
    game->env_interact_active = 1;
    game->env_interact_kind = kind;
    game->env_interact_room = game->player.room_id;
    push_env_menu(out, kind, game->player.room_id);
}

void gatmos_env_dismiss(struct GameState *game, GameEventQueue *out)
{
    if (!game->env_interact_active) {
        return;
    }
    gatmos_env_clear_interact(game);
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD,
        GAME_DIALOGUE_GUARD_ENV_MENU_CLOSED, 0, 0, 0, 0);
}

void seed_world_items(struct GameState *game)
{
    int i;
    int s;
    /* Start from a clean room-item grid so seeded placement stays deterministic. */
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
            game->room_item[i][s] = ITEM_NONE;
        }
    }
    game->room_item[WORLD_ROOM_CAMP][0] = ITEM_STICK;
    game->room_item[WORLD_ROOM_ROAD][0] = ITEM_STONE;
    game->room_item[WORLD_ROOM_POND][0] = ITEM_FISH;
    game->room_item[WORLD_ROOM_FOREST][0] = ITEM_HERB;
    game->room_item[WORLD_ROOM_RUINS][0] = ITEM_STONE;
    game->room_item[WORLD_ROOM_STREAM][0] = ITEM_REED;
    game->room_item[WORLD_ROOM_MARSH][0] = ITEM_REED;
    game->room_item[WORLD_ROOM_MEADOW][0] = ITEM_BERRY;
    game->room_item[WORLD_ROOM_ORCHARD][0] = ITEM_BERRY;
    game->room_item[WORLD_ROOM_CANYON][0] = ITEM_STONE;
    game->room_item[WORLD_ROOM_CAVE][0] = ITEM_HERB;
}

static void maybe_spawn_room_item(struct GameState *game, GameEventQueue *out)
{
    int room_id;
    int roll;
    int spawned;

    /* Random ground items only appear when the current room still has a free slot. */
    room_id = game->player.room_id;
    if (room_id < 0 || room_id >= game->world.room_count) {
        return;
    }
    if (!game_room_ground_has_space(game, room_id)) {
        return;
    }
    if ((plat_rand() % CFG_ROLL_PERCENT_RANGE) >= CFG_ROOM_ITEM_SPAWN_GATE) {
        return;
    }
    roll = plat_rand() % CFG_ROLL_PERCENT_RANGE;
    if (roll < CFG_ROOM_SPAWN_ROLL_BERRY_BELOW) spawned = ITEM_BERRY;
    else if (roll < CFG_ROOM_SPAWN_ROLL_STICK_BELOW) spawned = ITEM_STICK;
    else if (roll < CFG_ROOM_SPAWN_ROLL_REED_BELOW) spawned = ITEM_REED;
    else if (roll < CFG_ROOM_SPAWN_ROLL_STONE_BELOW) spawned = ITEM_STONE;
    else if (roll < CFG_ROOM_SPAWN_ROLL_HERB_BELOW) spawned = ITEM_HERB;
    else spawned = ITEM_FISH;
    if (!game_room_ground_try_add(game, room_id, spawned)) {
        return;
    }
    push_item_presence(out, spawned, item_name(spawned));
}

void maybe_emit_animal_noise(struct GameState *game, GameEventQueue *out)
{
    if ((game->tick % (u32)CFG_ANIMAL_NOISE_TICK_PERIOD) != 0UL) {
        return;
    }
    if ((plat_rand() % CFG_ROLL_PERCENT_RANGE) >= CFG_ANIMAL_NOISE_SKIP_ROLL_GE) {
        return;
    }
    push_ambient_noise(out,
        world_room_animal_noise(&game->world, game->player.room_id));
}

void maybe_emit_atmosphere(struct GameState *game, GameEventQueue *out)
{
    int roll;

    /* Environmental focus is a short-lived state machine keyed by room, kind, and expiry tick. */
    if (game->env_focus_active && game->tick >= game->env_focus_expires_tick) {
        game->env_focus_active = 0;
        game->env_focus_room = -1;
        game->env_focus_kind = GAME_ENV_NONE;
        game->env_focus_expires_tick = 0;
    }

    roll = plat_rand() % CFG_ROLL_PERCENT_RANGE;
    if (roll < CFG_ATMOSPHERE_ROLL_GUST_BELOW) {
        push_environment(out, GAME_ENV_EVENT_GUST);
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_RUSTLE_BELOW) {
        push_environment(out, GAME_ENV_EVENT_RUSTLE);
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_RUSTLE;
        game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
        if (game_room_ground_has_space(game, game->player.room_id) &&
                (plat_rand() % CFG_ROLL_PERCENT_RANGE) < CFG_ATMOSPHERE_FOCUS_EXTRA_ITEM_BELOW) {
            if (game_room_ground_try_add(game, game->player.room_id, ITEM_BERRY)) {
                push_environment(out, GAME_ENV_EVENT_BERRY_DROP);
            }
        }
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_CREAK_BELOW) {
        push_environment(out, GAME_ENV_EVENT_CREAK);
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_CREAK;
        game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_WATER_BELOW) {
        push_environment(out, GAME_ENV_EVENT_WATER);
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_WATER;
        game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
        if (game_room_ground_has_space(game, game->player.room_id) &&
                (plat_rand() % CFG_ROLL_PERCENT_RANGE) < CFG_ATMOSPHERE_FOCUS_EXTRA_ITEM_BELOW) {
            if (game_room_ground_try_add(game, game->player.room_id, ITEM_REED)) {
                push_environment(out, GAME_ENV_EVENT_REED_DROP);
            }
        }
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_GRIT_BELOW) {
        push_environment(out, GAME_ENV_EVENT_GRIT);
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_GRIT;
        game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
        return;
    }
    maybe_spawn_room_item(game, out);
}

static int env_reply_water(struct GameState *game, int choice,
                           GameEventQueue *out)
{
    int detail;

    detail = GAME_ENV_RESULT_DETAIL_NONE;
    if (choice == 1) {
        /* follow runnel: flavor only */
    } else if (choice == 2) {
        if (game_heal_player(game, CFG_ENV_WATER_HEAL_AMOUNT)) {
            detail = GAME_ENV_RESULT_DETAIL_HEALED;
        } else {
            detail = GAME_ENV_RESULT_DETAIL_HP_FULL;
        }
    }
    push_env_result(out, GAME_ENV_WATER, choice, detail);
    return 1;
}

static int env_reply_rustle(struct GameState *game, int choice,
                            GameEventQueue *out)
{
    int detail;
    int room_id;

    detail = GAME_ENV_RESULT_DETAIL_NONE;
    if (choice == 1) {
        room_id = game->env_interact_room;
        if (room_id >= 0 &&
                game_room_ground_try_add(game, room_id, ITEM_BERRY)) {
            detail = GAME_ENV_RESULT_DETAIL_ITEM_SPAWNED;
        } else {
            detail = GAME_ENV_RESULT_DETAIL_ITEM_FAILED;
        }
    }
    push_env_result(out, GAME_ENV_RUSTLE, choice, detail);
    return 1;
}

static int env_reply_creak(struct GameState *game, int choice,
                           GameEventQueue *out)
{
    int detail;
    int room_id;

    detail = GAME_ENV_RESULT_DETAIL_NONE;
    if (choice == 1) {
        room_id = game->env_interact_room;
        if (room_id >= 0 &&
                game_room_ground_try_add(game, room_id, ITEM_STICK)) {
            detail = GAME_ENV_RESULT_DETAIL_ITEM_SPAWNED;
        } else {
            detail = GAME_ENV_RESULT_DETAIL_ITEM_FAILED;
        }
    }
    push_env_result(out, GAME_ENV_CREAK, choice, detail);
    return 1;
}

static int env_reply_grit(struct GameState *game, int choice,
                          GameEventQueue *out)
{
    (void)game;
    push_env_result(out, GAME_ENV_GRIT, choice, GAME_ENV_RESULT_DETAIL_NONE);
    return 1;
}

/*
 * Reply handler for the env menu. Returns 0 when inactive or player left the
 * pinned room; game.c falls through to other reply routers on 0.
 */
int gatmos_cmd_env_reply(struct GameState *game, int choice,
                         GameEventQueue *out)
{
    int kind;
    int max_choice;

    if (!game->env_interact_active ||
            game->env_interact_room != game->player.room_id) {
        return 0;
    }
    kind = game->env_interact_kind;
    max_choice = env_max_choice(kind);
    if (choice < 1 || choice > max_choice) {
        push_pick_guard(out, max_choice);
        return 1;
    }
    if (env_is_leave_choice(kind, choice)) {
        gatmos_env_clear_interact(game);
        push_env_result(out, kind, choice, GAME_ENV_RESULT_DETAIL_NONE);
        return 1;
    }
    if (kind == GAME_ENV_WATER) {
        (void)env_reply_water(game, choice, out);
    } else if (kind == GAME_ENV_RUSTLE) {
        (void)env_reply_rustle(game, choice, out);
    } else if (kind == GAME_ENV_CREAK) {
        (void)env_reply_creak(game, choice, out);
    } else if (kind == GAME_ENV_GRIT) {
        (void)env_reply_grit(game, choice, out);
    } else {
        gatmos_env_clear_interact(game);
        return 0;
    }
    gatmos_env_clear_interact(game);
    return 1;
}

int gatmos_cmd_inspect(struct GameState *game, int item_arg, GameEventQueue *out)
{
    int kind;

    /* Inspection only succeeds while the current room still has an active ambient focus. */
    if (!game->env_focus_active ||
            game->env_focus_room != game->player.room_id ||
            game->tick >= game->env_focus_expires_tick) {
        push_observation(out, GAME_OBS_OUTCOME_NOTHING);
        game->env_focus_active = 0;
        game->env_focus_room = -1;
        game->env_focus_kind = GAME_ENV_NONE;
        game->env_focus_expires_tick = 0;
        return 1;
    }
    if (item_arg != 0 && item_arg != game->env_focus_kind) {
        push_observation(out, GAME_OBS_OUTCOME_WRONG_FOCUS);
        return 1;
    }
    kind = game->env_focus_kind;
    if (kind == GAME_ENV_RUSTLE) {
        push_observation(out, GAME_OBS_OUTCOME_RUSTLE);
    } else if (kind == GAME_ENV_CREAK) {
        push_observation(out, GAME_OBS_OUTCOME_CREAK);
    } else if (kind == GAME_ENV_WATER) {
        push_observation(out, GAME_OBS_OUTCOME_WATER);
    } else if (kind == GAME_ENV_GRIT) {
        push_observation(out, GAME_OBS_OUTCOME_GRIT);
    }
    game->env_focus_active = 0;
    game->env_focus_room = -1;
    game->env_focus_kind = GAME_ENV_NONE;
    game->env_focus_expires_tick = 0;
    /* consume focus then open pinned-room menu; cleared on reply or dismiss. */
    env_open_menu(game, kind, out);
    return 1;
}
