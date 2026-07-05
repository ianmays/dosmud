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
 * Ambient systems live here: starter ground items, per-room inspect clues, and
 * room-scoped incidental events.
 * #51: global weather_kind / weather_expires_tick on GameState; transitions
 * and fog roaming gate use hash-only rolls (see weather_roll).
 * #130: day_phase / day_expires_tick / night_lost; phase alternates on expiry.
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

static void push_observation(GameEventQueue *out, int outcome, int kind)
{
    game_event_push(out, GAME_EVENT_OBSERVATION, outcome, kind, 0, 0, 0);
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

/*
 * Per-room inspect clues (#234): env_room_clues[room_id] is a bit set (bit
 * kind-1 for RUSTLE..GRIT), not the old single-slot env_focus_* state
 * machine. Several clue kinds can be active in one room at once; each bit
 * persists until gatmos_cmd_inspect consumes it or the player leaves the
 * room (gatmos_clear_departed_room_clues).
 */
static u8 env_clue_bit(int kind)
{
    if (kind < GAME_ENV_RUSTLE || kind > GAME_ENV_GRIT) {
        return 0;
    }
    return (u8)(1u << (kind - 1));
}

/* Returns 1 when the clue bit was newly set, 0 if already active. */
static int gatmos_room_clue_set(struct GameState *game, int room_id, int kind)
{
    u8 bit;
    u8 prior;

    if (room_id < 0 || room_id >= CFG_ROOM_MAX) {
        return 0;
    }
    bit = env_clue_bit(kind);
    if (bit == 0) {
        return 0;
    }
    prior = game->env_room_clues[room_id];
    if ((prior & bit) != 0) {
        return 0;
    }
    game->env_room_clues[room_id] = (u8)(prior | bit);
    return 1;
}

static int gatmos_room_clue_has(const struct GameState *game, int room_id,
                                int kind)
{
    u8 bit;

    if (room_id < 0 || room_id >= CFG_ROOM_MAX) {
        return 0;
    }
    bit = env_clue_bit(kind);
    if (bit == 0) {
        return 0;
    }
    return (game->env_room_clues[room_id] & bit) != 0;
}

static void gatmos_room_clue_clear(struct GameState *game, int room_id,
                                   int kind)
{
    u8 bit;

    if (room_id < 0 || room_id >= CFG_ROOM_MAX) {
        return;
    }
    bit = env_clue_bit(kind);
    game->env_room_clues[room_id] &= (u8)~bit;
}

static int gatmos_room_clue_only_kind(const struct GameState *game,
                                      int room_id)
{
    int kind;
    int found;

    found = GAME_ENV_NONE;
    for (kind = GAME_ENV_RUSTLE; kind <= GAME_ENV_GRIT; ++kind) {
        if (gatmos_room_clue_has(game, room_id, kind)) {
            if (found != GAME_ENV_NONE) {
                return GAME_ENV_NONE;
            }
            found = kind;
        }
    }
    return found;
}

/* game_cmd_move calls this with the room the player just left; gatmos owns
 * the decision to drop uninspected clues rather than let them roam with the
 * player. */
void gatmos_clear_departed_room_clues(struct GameState *game, int room_id)
{
    if (room_id >= 0 && room_id < CFG_ROOM_MAX) {
        game->env_room_clues[room_id] = 0;
    }
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

/*
 * #51 weather rolls: seed^tick^salt only; never plat_rand so save/replay stay
 * stable independent of ambient RNG consumption order. Multiply/xor mask to
 * 32 bits so LP64 Linux test builds match DOS/Open Watcom u32 wrap.
 */
#define WEATHER_HASH_MUL 2654435761u
#define WEATHER_HASH_MASK 0xFFFFFFFFUL

static u32 weather_hash32(u32 value)
{
    return value & WEATHER_HASH_MASK;
}

static u32 weather_hash(const struct GameState *game, u32 salt)
{
    u32 tick_mix;

    tick_mix = weather_hash32(game->tick * WEATHER_HASH_MUL);
    return weather_hash32(game->seed ^ tick_mix ^ salt);
}

static int weather_roll(const struct GameState *game, u32 salt)
{
    u32 x;

    x = weather_hash(game, salt);
    return (int)((x >> 16) % (u32)CFG_ROLL_PERCENT_RANGE);
}

static int weather_kind_from_roll(int roll)
{
    if (roll < CFG_WEATHER_ROLL_RAIN_BELOW) {
        return GAME_WEATHER_RAIN;
    }
    if (roll < CFG_WEATHER_ROLL_FOG_BELOW) {
        return GAME_WEATHER_FOG;
    }
    if (roll < CFG_WEATHER_ROLL_WIND_BELOW) {
        return GAME_WEATHER_WIND;
    }
    return GAME_WEATHER_NONE;
}

static int weather_event_for_kind(int kind)
{
    if (kind == GAME_WEATHER_RAIN) {
        return GAME_ENV_EVENT_WEATHER_RAIN;
    }
    if (kind == GAME_WEATHER_FOG) {
        return GAME_ENV_EVENT_WEATHER_FOG;
    }
    if (kind == GAME_WEATHER_WIND) {
        return GAME_ENV_EVENT_WEATHER_WIND;
    }
    return GAME_ENV_EVENT_WEATHER_CLEAR;
}

static int atmosphere_threshold(int base, int bias, int cap)
{
    int v;

    v = base + bias;
    if (v > cap) {
        v = cap;
    }
    return v;
}

/* Rain/wind shift plat_rand atmosphere thresholds only; roll source unchanged. */
static int atmosphere_gust_below(const struct GameState *game)
{
    if (game->weather_kind == GAME_WEATHER_WIND) {
        return atmosphere_threshold(CFG_ATMOSPHERE_ROLL_GUST_BELOW,
            CFG_WEATHER_WIND_GUST_BIAS, CFG_ATMOSPHERE_ROLL_RUSTLE_BELOW);
    }
    return CFG_ATMOSPHERE_ROLL_GUST_BELOW;
}

static int atmosphere_rustle_below(const struct GameState *game)
{
    if (game->weather_kind == GAME_WEATHER_WIND) {
        return atmosphere_threshold(CFG_ATMOSPHERE_ROLL_RUSTLE_BELOW,
            CFG_WEATHER_WIND_RUSTLE_BIAS, CFG_ATMOSPHERE_ROLL_CREAK_BELOW);
    }
    return CFG_ATMOSPHERE_ROLL_RUSTLE_BELOW;
}

static int atmosphere_water_below(const struct GameState *game)
{
    if (game->weather_kind == GAME_WEATHER_RAIN) {
        return atmosphere_threshold(CFG_ATMOSPHERE_ROLL_WATER_BELOW,
            CFG_WEATHER_RAIN_WATER_BIAS, CFG_ATMOSPHERE_ROLL_GRIT_BELOW);
    }
    return CFG_ATMOSPHERE_ROLL_WATER_BELOW;
}

static int atmosphere_grit_below(const struct GameState *game)
{
    if (game->weather_kind == GAME_WEATHER_RAIN) {
        return atmosphere_threshold(CFG_ATMOSPHERE_ROLL_GRIT_BELOW,
            CFG_WEATHER_RAIN_GRIT_BIAS, CFG_ROLL_PERCENT_RANGE);
    }
    return CFG_ATMOSPHERE_ROLL_GRIT_BELOW;
}

/*
 * Called from game.c advance_world_tick after tick++; advances weather_kind on
 * expiry and queues GAME_EVENT_ENVIRONMENT when the kind changes.
 */
void gatmos_weather_tick(struct GameState *game, GameEventQueue *out)
{
    int roll;
    int new_kind;
    int prior_kind;

    if (game->tick < game->weather_expires_tick) {
        return;
    }
    prior_kind = game->weather_kind;
    roll = weather_roll(game, 0x51u);
    new_kind = weather_kind_from_roll(roll);
    game->weather_kind = new_kind;
    game->weather_expires_tick = game->tick + (u32)CFG_WEATHER_DURATION_TICKS;
    if (new_kind != prior_kind) {
        push_environment(out, weather_event_for_kind(new_kind));
    }
}

/*
 * Fog roaming seam: game.c asks before npc_roaming_begin_encounter_in_room;
 * hash-only per check so fog does not consume plat_rand.
 */
int gatmos_weather_blocks_roaming_encounter(struct GameState *game)
{
    int roll;

    if (game->weather_kind != GAME_WEATHER_FOG) {
        return 0;
    }
    roll = weather_roll(game, 0xf09u);
    return roll >= CFG_WEATHER_FOG_ENCOUNTER_ALLOW_BELOW ? 1 : 0;
}

/* Wielded or bag-held torch; invent.c owns item presence checks. */
static int player_has_torch(const struct GameState *game)
{
    if (game->weapon_equipped == ITEM_TORCH) {
        return 1;
    }
    return game_inv_player_has_item(game, ITEM_TORCH);
}

static u32 daynight_duration_ticks(int phase)
{
    if (phase == GAME_NIGHT) {
        return (u32)CFG_NIGHT_DURATION_TICKS;
    }
    return (u32)CFG_DAY_DURATION_TICKS;
}

/*
 * Called from game.c advance_world_tick after gatmos_weather_tick; alternates
 * day_phase on expiry, clears night_lost at dawn, and queues transition env
 * events when the phase changes.
 */
void gatmos_daynight_tick(struct GameState *game, GameEventQueue *out)
{
    if (game->tick < game->day_expires_tick) {
        return;
    }
    if (game->day_phase == GAME_DAY) {
        game->day_phase = GAME_NIGHT;
        push_environment(out, GAME_ENV_EVENT_NIGHT_FALL);
    } else {
        game->day_phase = GAME_DAY;
        game->night_lost = 0;
        push_environment(out, GAME_ENV_EVENT_DAY_BREAK);
    }
    game->day_expires_tick = game->tick + daynight_duration_ticks(game->day_phase);
}

/*
 * Render seam for the map command: true when night_lost is set and the player
 * lacks a torch; dawn or torch clears night_lost and restores the grid.
 */
int gatmos_night_map_blanked(const struct GameState *game)
{
    if (game->day_phase != GAME_NIGHT) {
        return 0;
    }
    if (player_has_torch(game)) {
        return 0;
    }
    return game->night_lost ? 1 : 0;
}

int gatmos_night_torch_lights_map(const struct GameState *game)
{
    if (game->day_phase != GAME_NIGHT) {
        return 0;
    }
    return player_has_torch(game) ? 1 : 0;
}

void gatmos_clear_night_lost_with_torch(struct GameState *game)
{
    if (game->day_phase != GAME_NIGHT) {
        return;
    }
    if (!player_has_torch(game)) {
        return;
    }
    game->night_lost = 0;
}

/*
 * game.c calls after a successful move during night; hash-only roll when the
 * player lacks a torch and night_lost is clear. Sets night_lost and queues
 * GAME_ENV_EVENT_NIGHT_LOST once per night episode until dawn or torch.
 */
void gatmos_try_night_lost_on_move(struct GameState *game, GameEventQueue *out)
{
    int roll;

    if (game->day_phase != GAME_NIGHT) {
        return;
    }
    if (player_has_torch(game)) {
        return;
    }
    if (game->night_lost) {
        return;
    }
#ifdef TEST_MODE
    /* Quiet fixtures suppress hash rolls; advance_world_tick already gated. */
    if (game->test_quiet_ticks) {
        return;
    }
#endif
    /* Reuses weather_hash (salt 0x130u); never plat_rand for save/replay stability. */
    roll = weather_roll(game, 0x130u);
    if (roll >= CFG_NIGHT_LOST_ROLL_BELOW) {
        return;
    }
    game->night_lost = 1;
    push_environment(out, GAME_ENV_EVENT_NIGHT_LOST);
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

void gatmos_queue_restored_menu(struct GameState *game, GameEventQueue *out)
{
    if (!game->env_interact_active ||
            game->env_interact_room != game->player.room_id ||
            game->env_interact_kind <= GAME_ENV_NONE ||
            game->env_interact_kind > GAME_ENV_GRIT) {
        gatmos_env_clear_interact(game);
        return;
    }
    push_env_menu(out, game->env_interact_kind, game->env_interact_room);
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
    /* Fog: lower skip threshold than clear weather (fewer animal-noise events). */
    if ((plat_rand() % CFG_ROLL_PERCENT_RANGE) >=
            (game->weather_kind == GAME_WEATHER_FOG ?
                CFG_WEATHER_FOG_NOISE_SKIP_GE : CFG_ANIMAL_NOISE_SKIP_ROLL_GE)) {
        return;
    }
    push_ambient_noise(out,
        world_room_animal_noise(&game->world, game->player.room_id));
}

void maybe_emit_atmosphere(struct GameState *game, GameEventQueue *out)
{
    int roll;
    int room_id;

    room_id = game->player.room_id;
    roll = plat_rand() % CFG_ROLL_PERCENT_RANGE;
    if (roll < atmosphere_gust_below(game)) {
        push_environment(out, GAME_ENV_EVENT_GUST);
        return;
    }
    if (roll < atmosphere_rustle_below(game)) {
        if (gatmos_room_clue_set(game, room_id, GAME_ENV_RUSTLE)) {
            push_environment(out, GAME_ENV_EVENT_RUSTLE);
            if (game_room_ground_has_space(game, game->player.room_id) &&
                    (plat_rand() % CFG_ROLL_PERCENT_RANGE) < CFG_ATMOSPHERE_FOCUS_EXTRA_ITEM_BELOW) {
                if (game_room_ground_try_add(game, game->player.room_id, ITEM_BERRY)) {
                    push_environment(out, GAME_ENV_EVENT_BERRY_DROP);
                }
            }
        }
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_CREAK_BELOW) {
        if (gatmos_room_clue_set(game, room_id, GAME_ENV_CREAK)) {
            push_environment(out, GAME_ENV_EVENT_CREAK);
        }
        return;
    }
    if (roll < atmosphere_water_below(game)) {
        if (gatmos_room_clue_set(game, room_id, GAME_ENV_WATER)) {
            push_environment(out, GAME_ENV_EVENT_WATER);
            if (game_room_ground_has_space(game, game->player.room_id) &&
                    (plat_rand() % CFG_ROLL_PERCENT_RANGE) < CFG_ATMOSPHERE_FOCUS_EXTRA_ITEM_BELOW) {
                if (game_room_ground_try_add(game, game->player.room_id, ITEM_REED)) {
                    push_environment(out, GAME_ENV_EVENT_REED_DROP);
                }
            }
        }
        return;
    }
    if (roll < atmosphere_grit_below(game)) {
        if (gatmos_room_clue_set(game, room_id, GAME_ENV_GRIT)) {
            push_environment(out, GAME_ENV_EVENT_GRIT);
        }
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
 * Reply handler for the env menu. Returns 0 only when inactive; room mismatch
 * dismisses the menu and returns 1 so game.c does not fall through silently.
 */
int gatmos_cmd_env_reply(struct GameState *game, int choice,
                         GameEventQueue *out)
{
    int kind;
    int max_choice;

    if (!game->env_interact_active) {
        return 0;
    }
    if (game->env_interact_room != game->player.room_id) {
        gatmos_env_dismiss(game, out);
        return 1;
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
    int room_id;
    int kind;
    u8 clues;

    room_id = game->player.room_id;
    clues = game->env_room_clues[room_id];
    if (item_arg != 0) {
        kind = item_arg;
        if (!gatmos_room_clue_has(game, room_id, kind)) {
            if (clues != 0) {
                push_observation(out, GAME_OBS_OUTCOME_LEAD_SPENT, kind);
            } else {
                push_observation(out, GAME_OBS_OUTCOME_NOTHING, 0);
            }
            return 1;
        }
    } else {
        kind = gatmos_room_clue_only_kind(game, room_id);
        if (kind == GAME_ENV_NONE) {
            if (clues != 0) {
                push_observation(out, GAME_OBS_OUTCOME_CHOOSE_KIND, 0);
            } else {
                push_observation(out, GAME_OBS_OUTCOME_NOTHING, 0);
            }
            return 1;
        }
    }
    if (kind == GAME_ENV_RUSTLE) {
        push_observation(out, GAME_OBS_OUTCOME_RUSTLE, 0);
    } else if (kind == GAME_ENV_CREAK) {
        push_observation(out, GAME_OBS_OUTCOME_CREAK, 0);
    } else if (kind == GAME_ENV_WATER) {
        push_observation(out, GAME_OBS_OUTCOME_WATER, 0);
    } else if (kind == GAME_ENV_GRIT) {
        push_observation(out, GAME_OBS_OUTCOME_GRIT, 0);
    }
    gatmos_room_clue_clear(game, room_id, kind);
    env_open_menu(game, kind, out);
    return 1;
}
