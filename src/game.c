#include <stdlib.h>
#include "buildid.h"
#include "platform.h"
#include "game.h"
#include "gwhok.h"
#include "invent.h"
#include "items.h"
#include "gatmos.h"
#include "combat.h"
#include "dialogue.h"
#include "genc.h"
#include "npc.h"

/*
 * game.c owns top-level orchestration: it routes commands, advances ticks,
 * and switches between explore, dialogue, and combat without embedding slice
 * rules in the wrong layer.
 */

static void push_dialogue_guard(GameEventQueue *out, int reason)
{
    /* #160: orchestration-layer modal guards; grendr maps reason to copy. */
    game_event_push(out, GAME_EVENT_DIALOGUE_GUARD, reason, 0, 0, 0, 0);
}

/*
 * #240: one classifier owns modal command policy. ModalContext names the
 * active modal owner; ModalDisposition is the verdict apply_command acts on:
 *   KEEP  - run the verb inside the modal; the owning slice keeps its prompt.
 *   BLOCK - reject the verb (guard + prompt replay only); apply_command
 *           returns 0 so no world tick advances (blocked verbs cost no time).
 *   CLOSE - tear the modal down first, then run the verb in explore.
 * The classifier reads state only; teardown and replay stay with the slice.
 */
enum ModalContext {
    MODAL_CONTEXT_NONE = 0,
    MODAL_CONTEXT_COMBAT,
    MODAL_CONTEXT_ENEMY,
    MODAL_CONTEXT_PEACEFUL,
    MODAL_CONTEXT_LOOT,
    MODAL_CONTEXT_ENV
};

enum ModalDisposition {
    MODAL_KEEP = 0,
    MODAL_BLOCK,
    MODAL_CLOSE
};

static int command_is_session(int type)
{
    return type == CMD_HELP || type == CMD_VERSION || type == CMD_QUIT;
}

static int command_is_self_directed(int type)
{
    return type == CMD_BAG || type == CMD_WIELD || type == CMD_UNWIELD ||
        type == CMD_EAT || type == CMD_USE || type == CMD_CRAFT ||
        type == CMD_DROP;
}

static int command_is_world_directed(int type)
{
    return type == CMD_LOOK || type == CMD_MAP || type == CMD_INSPECT ||
        type == CMD_TAKE;
}

static enum ModalContext game_modal_context(const struct GameState *game)
{
    if (game->mode == GAME_MODE_COMBAT) {
        return MODAL_CONTEXT_COMBAT;
    }
    if (game->mode == GAME_MODE_DIALOGUE) {
        if (game->dialogue == DIALOGUE_ENEMY) {
            return MODAL_CONTEXT_ENEMY;
        }
        if (game->dialogue == DIALOGUE_LOOT) {
            return MODAL_CONTEXT_LOOT;
        }
        return MODAL_CONTEXT_PEACEFUL;
    }
    if (game->env_interact_active) {
        return MODAL_CONTEXT_ENV;
    }
    return MODAL_CONTEXT_NONE;
}

static enum ModalDisposition modal_command_disposition(
    enum ModalContext context, const struct Command *cmd)
{
    /* One classifier owns modal command policy; slices still own semantics. */
    if (context == MODAL_CONTEXT_NONE) {
        return MODAL_KEEP;
    }
    if (cmd->type == CMD_REPLY || command_is_session(cmd->type)) {
        return MODAL_KEEP;
    }
    if (context == MODAL_CONTEXT_LOOT && cmd->type == CMD_LOOT) {
        return MODAL_KEEP;
    }
    if (context == MODAL_CONTEXT_PEACEFUL && cmd->type == CMD_TALK) {
        return MODAL_KEEP;
    }
    if (command_is_self_directed(cmd->type)) {
        return MODAL_KEEP;
    }
    if (command_is_world_directed(cmd->type) || cmd->type == CMD_WAIT) {
        return MODAL_BLOCK;
    }
    if (cmd->type == CMD_MOVE) {
        if (context == MODAL_CONTEXT_COMBAT ||
                context == MODAL_CONTEXT_ENEMY) {
            return MODAL_BLOCK;
        }
        return MODAL_CLOSE;
    }
    if (cmd->type == CMD_GIVE) {
        if (context == MODAL_CONTEXT_LOOT || context == MODAL_CONTEXT_ENV ||
                context == MODAL_CONTEXT_COMBAT) {
            return MODAL_BLOCK;
        }
        /* peaceful/enemy give stays modal: the give slice owns accept vs
         * reject and replays its own prompt on reject (no forced explore). */
        return MODAL_KEEP;
    }
    if (cmd->type == CMD_TALK) {
        if (context == MODAL_CONTEXT_ENEMY ||
                context == MODAL_CONTEXT_COMBAT) {
            return MODAL_BLOCK;
        }
        if (context == MODAL_CONTEXT_ENV) {
            return MODAL_CLOSE;
        }
    }
    /* Any other verb: peaceful/loot/env dismiss and run it in explore;
     * combat/enemy stay modal and block it. */
    if (context == MODAL_CONTEXT_LOOT || context == MODAL_CONTEXT_ENV ||
            context == MODAL_CONTEXT_PEACEFUL) {
        return MODAL_CLOSE;
    }
    return MODAL_BLOCK;
}

/* Handover gating reads NPC_FLAG_HANDOVER_PICK on the active enemy slot. */
static int game_enemy_handover_pick_active(const struct GameState *game)
{
    int slot;

    slot = npc_find_by_dialogue(game, DIALOGUE_ENEMY);
    if (slot < 0) {
        return 0;
    }
    return (game->npcs[slot].flags & NPC_FLAG_HANDOVER_PICK) != 0;
}

void game_set_mode_explore(struct GameState *game)
{
    game->mode = GAME_MODE_EXPLORE;
    game->dialogue = DIALOGUE_NONE;
    /* gatmos post-inspect menu; cleared with other explore overlays. */
    gatmos_env_clear_interact(game);
    /* npc.c session menus; cleared so explore ticks do not reuse stale routing. */
    game->watchman_menu = 0;
    game->herbalist_menu = 0;
    /* Drop combat snapshot so explore ticks do not reuse stale enemy scaling. */
    game->combat.enemy_hp = 0;
    game->combat.enemy_level = 0;
    game->combat.defending = 0;
}

void game_set_mode_dialogue(struct GameState *game, enum DialogueKind kind)
{
    game->mode = GAME_MODE_DIALOGUE;
    game->dialogue = kind;
}

void game_set_mode_combat(struct GameState *game)
{
    game->mode = GAME_MODE_COMBAT;
    game->dialogue = DIALOGUE_NONE;
}

/*
 * game.c owns the cross-slice defeat transaction: invent and gprog mutate
 * first, then orchestration restores camp exploration and queues PLAYER_DEFEAT
 * for grendr (payload layout in gout.h).
 */
void game_handle_player_defeat(struct GameState *game, GameEventQueue *out)
{
    GameEvent *ev;
    u32 xp_lost;
    int defeat_room;
    int old_level;
    int transferred_count;
    int retained_count;
    int retained_item;
    int equipped_item;
    int replaced_count;

    defeat_room = game->player.room_id;
    old_level = game->level;
    game_player_corpse_replace_from_inventory(game, defeat_room,
        &transferred_count, &retained_count, &retained_item, &equipped_item,
        &replaced_count);
    xp_lost = progression_apply_defeat_penalty(game);
    game->player.room_id = WORLD_ROOM_CAMP;
    game->room_explored[WORLD_ROOM_CAMP] = 1;
    game_set_mode_explore(game);
    game->player_hp = game->max_hp;
    game->running = 1;

    ev = game_event_push(out, GAME_EVENT_PLAYER_DEFEAT, (int)xp_lost,
        old_level, game->level, transferred_count, 0);
    if (ev != 0) {
        ev->room_id = defeat_room;
        ev->room_item[0] = equipped_item;
        ev->room_item[1] = retained_item;
        ev->room_item[2] = retained_count;
        ev->room_item[3] = replaced_count;
    }
}

int game_is_busy_dialogue(struct GameState *game)
{
    /* Any non-explore mode suppresses ambient encounters and background prompts. */
    if (game->mode != GAME_MODE_EXPLORE) {
        return 1;
    }
    /* gatmos env inspect menu suppresses ambient ticks like dialogue mode. */
    if (game->env_interact_active) {
        return 1;
    }
    return 0;
}

int game_heal_player(struct GameState *game, int amount)
{
    if (game->player_hp >= game->max_hp) {
        return 0;
    }
    game->player_hp += amount;
    if (game->player_hp > game->max_hp) {
        game->player_hp = game->max_hp;
    }
    return 1;
}

/*
 * Snapshot current room into a generic look event. arg0 is npc_room_actor for
 * HUD presence; arg1 packs corpse_present (bit 0), weather_kind (bits 1-2),
 * and day_phase (bit 3, #130) at enqueue time so post-tick rolls do not reorder
 * look output.
 */
static int look_arg1_pack(int corpse_present, int weather_kind, int day_phase)
{
    return corpse_present | (weather_kind << 1) | (day_phase << 3);
}

static int queue_has_weather_transition(const GameEventQueue *out)
{
    int i;

    for (i = 0; i < out->count; ++i) {
        if (out->events[i].kind != GAME_EVENT_ENVIRONMENT) {
            continue;
        }
        if (out->events[i].arg0 == GAME_ENV_EVENT_WEATHER_RAIN ||
                out->events[i].arg0 == GAME_ENV_EVENT_WEATHER_FOG ||
                out->events[i].arg0 == GAME_ENV_EVENT_WEATHER_WIND ||
                out->events[i].arg0 == GAME_ENV_EVENT_WEATHER_CLEAR) {
            return 1;
        }
    }
    return 0;
}

static void mark_room_look_weather_suppressed(GameEventQueue *out)
{
    int i;

    /* Reply/encounter slices can queue ROOM_LOOK before the tick runs. Mark
     * those earlier snapshots after weather enqueue so same-step copy does not
     * repeat in the compact footer.
     */
    if (!queue_has_weather_transition(out)) {
        return;
    }
    for (i = 0; i < out->count; ++i) {
        if (out->events[i].kind != GAME_EVENT_ROOM_LOOK) {
            continue;
        }
        out->events[i].arg3 |= GAME_ROOM_LOOK_FLAG_SUPPRESS_WEATHER;
    }
}

static void do_look_flags(struct GameState *game, GameEventQueue *out, int flags)
{
    int i;
    GameEvent *ev;

    if (game_player_corpse_is_in_room(game, game->player.room_id)) {
        flags |= GAME_ROOM_LOOK_FLAG_PLAYER_CORPSE;
    }
    ev = game_event_push(out, GAME_EVENT_ROOM_LOOK,
        npc_room_actor(game->player.room_id),
        look_arg1_pack(game->corpse_present[game->player.room_id],
            game->weather_kind, game->day_phase),
        game->env_room_clues[game->player.room_id],
        flags |
            (queue_has_weather_transition(out) ?
                GAME_ROOM_LOOK_FLAG_SUPPRESS_WEATHER :
                GAME_ROOM_LOOK_FLAG_NONE),
        0);
    if (ev == 0) {
        return;
    }
    ev->room_id = game->player.room_id;
    for (i = 0; i < CFG_AREA_ITEM_SLOTS; ++i) {
        ev->room_item[i] = game->room_item[game->player.room_id][i];
    }
}

static void do_look(struct GameState *game, GameEventQueue *out)
{
    do_look_flags(game, out, GAME_ROOM_LOOK_FLAG_NONE);
}

/* MAP: generic event only; map layout and terminal output stay in grendr. */
static void do_map(GameEventQueue *out)
{
    game_event_push(out, GAME_EVENT_MAP, 0, 0, 0, 0, 0);
}

/*
 * Slice callers enqueue ROOM_LOOK after peaceful modal exit so encounter
 * resolution copy precedes the restored room snapshot.
 */
void game_describe_current_room(struct GameState *game, GameEventQueue *out)
{
    do_look(game, out);
}

static void reset_mutable_state(struct GameState *game, int room_id, u32 tick)
{
    int i;
    int j;

    /* Reset only per-run state here; world topology is rebuilt separately. */
    game_set_mode_explore(game);
    game->player.room_id = room_id;
    game->tick = tick;
    npc_clear_all(game);
    npc_seed_profiles(game);
    /* gatmos.c per-room inspect clue bits (#234); no active-focus room/kind
     * to reset since clues are keyed by room_id, not a single slot. */
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        game->env_room_clues[i] = 0;
    }
    game->env_deferred_extra_event = 0;
    game->env_interact_active = 0;
    game->env_interact_kind = GAME_ENV_NONE;
    game->env_interact_room = -1;
    /* gatmos.c defers first weather roll until tick reaches expires_tick. */
    game->weather_kind = GAME_WEATHER_NONE;
    game->weather_expires_tick = (u32)CFG_WEATHER_INITIAL_DELAY_TICKS;
    /* gatmos.c defers first night until tick reaches day_expires_tick. */
    game->day_phase = GAME_DAY;
    game->day_expires_tick = (u32)CFG_DAYNIGHT_INITIAL_DELAY_TICKS;
    game->night_lost = 0;
    game->herbalist_story = HERBALIST_STORY_NONE;
    game->herbalist_menu = 0;
    game->watchman_flags = 0;
    game->watchman_menu = 0;
    game->marsh_root_spawned = 0;
    game->world_adv_flags = 0; /* gwhok.c advancement bits; cleared on fresh reset */
    gwhok_apply_all(game); /* reconcile room desc when fixtures reuse World */
    game->bag_count = 0;
    game->bag_capacity = CFG_START_BAG_CAPACITY;
    game->level = CFG_START_LEVEL;
    game->xp = CFG_START_XP;
    game->max_hp = CFG_START_MAX_HP;
    game->damage_bonus = CFG_START_DAMAGE_BONUS;
    game->weapon_equipped = ITEM_NONE;
    game->player_hp = CFG_START_MAX_HP;
    game->combat.enemy_hp = 0;
    game->combat.enemy_level = 0;
    game->combat.defending = 0;
    for (j = 0; j < CFG_BAG_MAX; ++j) {
        game->bag[j] = ITEM_NONE;
    }
#ifdef TEST_MODE
    game_roll_inject_clear(game);
    game->test_quiet_ticks = 0;
#endif
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        game->corpse_present[i] = 0;
        for (j = 0; j < CFG_CORPSE_ITEM_SLOTS; ++j) {
            game->corpse_item[i][j] = ITEM_NONE;
        }
        game->room_explored[i] = 0;
    }
    game->player_corpse_present = 0;
    game->player_corpse_room = -1;
    game->player_corpse_item_count = 0;
    for (j = 0; j < CFG_PLAYER_CORPSE_ITEM_SLOTS; ++j) {
        game->player_corpse_item[j] = ITEM_NONE;
    }
    seed_world_items(game);
    game->room_explored[room_id] = 1;
}

void game_init(struct GameState *game, u32 seed)
{
    world_init(&game->world);
    game->seed = seed;
    game->running = 1;
    reset_mutable_state(game, WORLD_ROOM_CAMP, 0);
}

#ifdef TEST_MODE
void game_reset_fixture_baseline(struct GameState *game, int room_id, u32 tick)
{
    reset_mutable_state(game, room_id, tick);
}
#endif

#ifdef TEST_MODE
static int game_roll_draw(struct GameState *game)
{
    /* Injected draws bypass plat_rand so tests stay deterministic without
     * advancing the save/load draw counter. */
    if (game->roll_inject_active) {
        if (game->roll_queue_i >= game->roll_queue_len) {
            /* Past end: count the draw so fully_consumed fails (over-consumption). */
            game->roll_queue_i++;
            return 0;
        }
        return game->roll_queue[game->roll_queue_i++];
    }
    return plat_rand();
}

void game_roll_inject_begin(struct GameState *game, const int *values, int count)
{
    int i;

    game_roll_inject_clear(game);
    if (values == 0 || count <= 0) {
        return;
    }
    if (count > CFG_ROLL_INJECT_MAX) {
        count = CFG_ROLL_INJECT_MAX;
    }
    for (i = 0; i < count; ++i) {
        game->roll_queue[i] = values[i];
    }
    game->roll_queue_len = count;
    game->roll_inject_active = 1;
}

void game_roll_inject_clear(struct GameState *game)
{
    int i;

    game->roll_inject_active = 0;
    game->roll_queue_len = 0;
    game->roll_queue_i = 0;
    for (i = 0; i < CFG_ROLL_INJECT_MAX; ++i) {
        game->roll_queue[i] = 0;
    }
}

int game_roll_inject_fully_consumed(const struct GameState *game)
{
    if (!game->roll_inject_active) {
        return 1;
    }
    return game->roll_queue_i == game->roll_queue_len;
}
#endif /* TEST_MODE */

int game_roll_spread(struct GameState *game, int spread)
{
    int roll;

    if (spread <= 0) {
        return 0;
    }
#ifdef TEST_MODE
    roll = game_roll_draw(game);
#else
    /* Release builds draw via plat_rand so save/load can track libc usage. */
    (void)game;
    roll = plat_rand();
#endif
    if (roll < 0) {
        roll = -roll;
    }
    return roll % spread;
}

int game_roll_percent(struct GameState *game)
{
    return game_roll_spread(game, CFG_ROLL_PERCENT_RANGE);
}

/*
 * Per-context replay seam: each modal owner re-emits its live prompt so a
 * blocked (or KEEP repeat) verb leaves the menu visible; emits events only,
 * never mutates state or advances a tick.
 */
static int replay_active_modal_prompt(struct GameState *game,
                                      enum ModalContext context,
                                      GameEventQueue *out)
{
    switch (context) {
    case MODAL_CONTEXT_COMBAT:
        combat_replay_menu(out);
        return 1;
    case MODAL_CONTEXT_ENEMY:
        return genc_replay_active_prompt(game, out);
    case MODAL_CONTEXT_PEACEFUL:
        return npc_replay_active_prompt(game, out);
    case MODAL_CONTEXT_LOOT:
        return game_corpse_queue_view(game, game->player.room_id, out);
    case MODAL_CONTEXT_ENV:
        gatmos_queue_restored_menu(game, out);
        return 1;
    default:
        break;
    }
    return 0;
}

/*
 * BLOCK feedback: an orchestration guard reason (grendr maps it to copy) plus
 * the owning slice's prompt replay. Pairs with apply_command returning 0, so
 * the rejected verb produces output without changing state or passing time.
 */
static void push_modal_block_feedback(struct GameState *game,
                                      enum ModalContext context,
                                      const struct Command *cmd,
                                      GameEventQueue *out)
{
    if (context == MODAL_CONTEXT_ENEMY || context == MODAL_CONTEXT_COMBAT) {
        if (cmd->type == CMD_TALK) {
            push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_BLOCKS_TALK);
        } else if (game_enemy_handover_pick_active(game)) {
            push_dialogue_guard(out,
                GAME_DIALOGUE_GUARD_BANDIT_WAITING_HANDOVER_PICK);
        } else {
            push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY);
        }
        (void)replay_active_modal_prompt(game, context, out);
        return;
    }
    if (context == MODAL_CONTEXT_LOOT) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_LOOT_WAITING_REPLY);
        (void)replay_active_modal_prompt(game, context, out);
        return;
    }
    if (context == MODAL_CONTEXT_ENV) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_ENV_MENU_WAITING);
        (void)replay_active_modal_prompt(game, context, out);
        return;
    }
    push_dialogue_guard(out, GAME_DIALOGUE_GUARD_FRIENDLY_DIALOGUE_WAITING);
    (void)replay_active_modal_prompt(game, context, out);
}

static void close_modal_before_command(struct GameState *game,
                                       enum ModalContext context,
                                       GameEventQueue *out)
{
    /* CLOSE only classifies intent; teardown stays with the owning modal. */
    if (context == MODAL_CONTEXT_PEACEFUL) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED);
        game_set_mode_explore(game);
        return;
    }
    if (context == MODAL_CONTEXT_LOOT) {
        (void)game_inv_cmd_loot(game, 0, out);
        return;
    }
    if (context == MODAL_CONTEXT_ENV) {
        gatmos_env_dismiss(game, out);
    }
}

static int game_cmd_session(struct GameState *game, struct Command *cmd,
                            GameEventQueue *out)
{
    if (cmd->type == CMD_HELP) {
        /* HELP topic in arg0; copy and render owned by grendr (game_print_help). */
        game_event_push(out, GAME_EVENT_HELP, cmd->arg, 0, 0, 0, 0);
        return 1;
    }
    if (cmd->type == CMD_QUIT) {
        game->running = 0;
        return 1;
    }
    if (cmd->type == CMD_VERSION) {
        /* build identity from buildid; render owned by grendr (render_msg_version). */
        game_event_push(out, GAME_EVENT_VERSION, 0, 0, 0, 0,
            build_version_line());
        return 1;
    }
    return 0;
}

static int game_cmd_observe(struct GameState *game, struct Command *cmd,
                            GameEventQueue *out)
{
    if (cmd->type == CMD_LOOK) {
        do_look(game, out);
        return 1;
    }
    if (cmd->type == CMD_MAP) {
        do_map(out);
        return 1;
    }
    return 0;
}

static int game_cmd_pass_time(struct GameState *game, struct Command *cmd,
                              GameEventQueue *out)
{
    (void)game;
    if (cmd->type != CMD_WAIT) {
        return 0;
    }
    /* WAIT: generic hold message; tick advance is separate orchestration. */
    game_event_push(out, GAME_EVENT_WAIT, 0, 0, 0, 0, 0);
    return 1;
}

static int game_cmd_move(struct GameState *game, struct Command *cmd,
                         GameEventQueue *out)
{
    int from_room;

    if (cmd->type != CMD_MOVE) {
        return 0;
    }
    if (!world_can_move(&game->world, game->player.room_id, cmd->dir)) {
        /* blocked exit: direction in text; no state change */
        game_event_push(out, GAME_EVENT_CANNOT_MOVE, 0, 0, 0, 0,
            world_dir_name(cmd->dir));
        return 0;
    }
    from_room = game->player.room_id;
    game->player.room_id = world_move(&game->world, game->player.room_id, cmd->dir);
    /* game.c owns the move; gatmos.c owns dropping uninspected clues left behind. */
    gatmos_clear_departed_room_clues(game, from_room);
    game->room_explored[game->player.room_id] = 1;
    if (game->mode == GAME_MODE_DIALOGUE) {
        game_set_mode_explore(game);
    }
    /* Night-lost env event before MOVE/ROOM_LOOK so announcement precedes look. */
    gatmos_try_night_lost_on_move(game, out);
    /* room_id already updated; MOVE precedes any deferred look after the tick. */
    game_event_push(out, GAME_EVENT_MOVE, 0, 0, 0, 0, world_dir_name(cmd->dir));
    return 1;
}

static int game_cmd_inventory(struct GameState *game, struct Command *cmd,
                              GameEventQueue *out)
{
    if (cmd->type == CMD_LOOT) {
        return game_inv_cmd_loot(game, cmd->arg == CMD_LOOT_ALL, out);
    }
    if (cmd->type == CMD_TAKE) {
        if (cmd->arg == CMD_TAKE_ALL) {
            return game_inv_cmd_take_all(game, out);
        }
        return game_inv_cmd_take(game, cmd->arg, out);
    }
    if (cmd->type == CMD_DROP) {
        return game_inv_cmd_drop(game, cmd->arg, out);
    }
    if (cmd->type == CMD_BAG) {
        return game_inv_cmd_bag(game, out);
    }
    if (cmd->type == CMD_WIELD) {
        return game_inv_cmd_wield(game, cmd->arg, out);
    }
    if (cmd->type == CMD_UNWIELD) {
        return game_inv_cmd_unwield(game, out);
    }
    if (cmd->type == CMD_EAT) {
        if (game->mode == GAME_MODE_COMBAT) {
            return combat_cmd_eat(game, cmd->arg, out);
        }
        return game_inv_cmd_eat(game, cmd->arg, out);
    }
    if (cmd->type == CMD_USE) {
        if (game->mode == GAME_MODE_COMBAT) {
            return combat_cmd_use(game, cmd->arg, out);
        }
        return game_inv_cmd_use(game, cmd->arg, out);
    }
    if (cmd->type == CMD_CRAFT) {
        return game_inv_cmd_craft(game, cmd->arg, out);
    }
    return 0;
}

static int game_cmd_reply(struct GameState *game, struct Command *cmd,
                          GameEventQueue *out)
{
    if (cmd->type != CMD_REPLY) {
        return 0;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        combat_resolve_reply(game, cmd->arg, out);
        return 1;
    }
    /* gatmos-owned numbered reply while env_interact_active (#7). */
    if (game->env_interact_active) {
        if (gatmos_cmd_env_reply(game, cmd->arg, out)) {
            return 1;
        }
        gatmos_env_dismiss(game, out);
        return 1;
    }
    /* Corpse take/leave replies are invent-owned, not dialogue.c actors. */
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_LOOT) {
        return game_inv_cmd_loot_reply(game, cmd->arg, out);
    }
    if (dialogue_cmd_reply(game, cmd->arg, out)) {
        return 1;
    }
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
        return genc_cmd_reply(game, cmd->arg, out);
    }
    /* Traveler and future roaming actors route through npc.c after room/enemy. */
    if (npc_roaming_cmd_reply(game, cmd->arg, out)) {
        return 1;
    }
    push_dialogue_guard(out, GAME_DIALOGUE_GUARD_NOBODY_WAITING_REPLY);
    return 1;
}

static int apply_command(struct GameState *game, struct Command *cmd,
                         GameEventQueue *out)
{
    enum ModalContext context;
    enum ModalDisposition disposition;

    /*
     * Modal gate precedes slice routing: BLOCK returns 0 so the caller skips
     * advance_world_tick (no time on a rejected verb); CLOSE tears the modal
     * down, then the verb runs below as a normal explore command.
     */
    context = game_modal_context(game);
    disposition = modal_command_disposition(context, cmd);
    if (disposition == MODAL_BLOCK) {
        push_modal_block_feedback(game, context, cmd, out);
        return 0;
    }
    if (disposition == MODAL_CLOSE) {
        close_modal_before_command(game, context, out);
    }
    if (game_cmd_session(game, cmd, out)) {
        return 1;
    }
    if (game_cmd_observe(game, cmd, out)) {
        return 1;
    }
    if (game_cmd_pass_time(game, cmd, out)) {
        return 1;
    }
    if (game_cmd_move(game, cmd, out)) {
        return 1;
    }
    if (game_cmd_inventory(game, cmd, out)) {
        return 1;
    }
    if (cmd->type == CMD_INSPECT) {
        return gatmos_cmd_inspect(game, cmd->arg, out);
    }
    if (cmd->type == CMD_TALK) {
        return dialogue_cmd_talk(game, out);
    }
    if (game_cmd_reply(game, cmd, out)) {
        return 1;
    }
    /*
     * Enemy handovers keep modal ownership while DIALOGUE_ENEMY is active.
     * Room NPC exchange handles only non-enemy gives in authored NPC rooms.
     */
    if (cmd->type == CMD_GIVE) {
        if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
            return genc_cmd_give(game, cmd->arg, out);
        }
        if (npc_cmd_give(game, cmd->arg, out)) {
            return 1;
        }
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_GIVE_NO_TARGET);
        return 1;
    }
    return 0;
}

static void advance_world_tick(struct GameState *game, GameEventQueue *out)
{
    int encounter_opened;
    int busy_before_tick;

    /*
     * Tick order: increment tick, advance global weather (#51) and day/night
     * (#130), roster maintenance, then roaming and fixed encounter opens (fog
     * blocks opens only) before world_step. An opened encounter skips ambient
     * output for this tick so MOVE plus encounter copy stay ahead of ROOM_LOOK.
     */
    game->tick += 1;
    gatmos_weather_tick(game, out);
    gatmos_daynight_tick(game, out);
    npc_roaming_update_separation(game);
    npc_story_tick(game);
    busy_before_tick = game_is_busy_dialogue(game);
#ifdef TEST_MODE
    if (game->test_quiet_ticks) {
        /* Quiet fixtures keep time moving but suppress ambient randomness. */
        world_step(&game->world, game->tick);
        return;
    }
#endif
    npc_roaming_activate_due(game);

    /*
     * Roaming encounters claim the room before movement when already
     * co-located; otherwise the roster gets one movement step, then a second
     * encounter check in the new room layout. Fog blocks encounter opens only.
     */
    encounter_opened = 0;
    if (!busy_before_tick) {
        if (!gatmos_weather_blocks_roaming_encounter(game)) {
            encounter_opened = npc_roaming_begin_encounter_in_room(
                game, game->player.room_id, out);
        }
        if (!encounter_opened) {
            npc_roaming_step(game);
            if (!gatmos_weather_blocks_roaming_encounter(game)) {
                encounter_opened = npc_roaming_begin_encounter_in_room(game,
                    game->player.room_id, out);
            }
        }
        if (!encounter_opened) {
            encounter_opened = npc_fixed_begin_encounter_in_room(game,
                game->player.room_id, out);
        }
    }

    world_step(&game->world, game->tick);
    if (busy_before_tick || encounter_opened) {
        return;
    }
    maybe_emit_atmosphere(game, out);
    maybe_emit_animal_noise(game, out);
    gatmos_emit_deferred_atmosphere_extras(game, out);
    if (!game_is_busy_dialogue(game)) {
        /*
         * Preserve the old ambient RNG cadence after removing the player-site
         * ambush spawn so unchanged seeds keep their non-encounter outputs.
         */
        (void)(plat_rand() % CFG_ROLL_PERCENT_RANGE);
    }
}

int game_process_input(struct GameState *game, char *line, GameEventQueue *out)
{
    struct Command cmd;
    int parsed;
    int applied;
    int prior_mode;
    int prior_dialogue;
    int player_defeated;
    int i;

    parsed = command_parse(line, &cmd);
    if (!parsed) {
        /* parse failure: generic unknown-command; no slice handler runs */
        game_event_push(out, GAME_EVENT_UNKNOWN_COMMAND, 0, 0, 0, 0, 0);
        return 0;
    }

    prior_mode = game->mode;
    prior_dialogue = game->dialogue;
    applied = apply_command(game, &cmd, out);
    if (!applied) {
        return 0;
    }

    /*
     * CMD_LOOT is quiet via command_advances_time (menu open and replies);
     * the DIALOGUE_LOOT guard below remains defense-in-depth. Defeat also
     * consumes the combat turn itself, so its camp respawn must not run a
     * second ambient tick in the same command step.
     */
    player_defeated = 0;
    for (i = 0; i < out->count; ++i) {
        if (out->events[i].kind == GAME_EVENT_PLAYER_DEFEAT) {
            player_defeated = 1;
            break;
        }
    }
    if (!player_defeated && command_advances_time(cmd.type) &&
            !(prior_mode == GAME_MODE_COMBAT &&
                (cmd.type == CMD_EAT || cmd.type == CMD_USE)) &&
            !(cmd.type == CMD_LOOT &&
                prior_mode == GAME_MODE_DIALOGUE &&
                prior_dialogue == DIALOGUE_LOOT)) {
        advance_world_tick(game, out);
        mark_room_look_weather_suppressed(out);
        if (prior_mode == GAME_MODE_COMBAT &&
                game->mode == GAME_MODE_COMBAT &&
                (cmd.type == CMD_CRAFT || cmd.type == CMD_DROP)) {
            combat_replay_menu(out);
        }
        /* defer look until after tick so encounter-open moves skip ROOM_LOOK */
        if (cmd.type == CMD_MOVE && game->mode == GAME_MODE_EXPLORE) {
            do_look(game, out);
        }
    }

    return 1;
}

void game_background_step(struct GameState *game, GameEventQueue *out)
{
    advance_world_tick(game, out);
}
