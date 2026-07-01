#include <stdlib.h>
#include "buildid.h"
#include "platform.h"
#include "game.h"
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
 * Menu-native verbs keep the modal open (numbered reply, repeat talk, repeat
 * loot-to-leave, and the active Herbalist give/offering prompt); help/version/
 * quit stay allowed like combat menus.
 */
static int cmd_preserves_noncombat_menu(const struct GameState *game,
                                        const struct Command *cmd)
{
    if (cmd->type == CMD_REPLY) {
        return 1;
    }
    if (cmd->type == CMD_HELP ||
            cmd->type == CMD_VERSION ||
            cmd->type == CMD_QUIT) {
        return 1;
    }
    if (game->dialogue != DIALOGUE_LOOT && cmd->type == CMD_TALK) {
        return 1;
    }
    if (game->dialogue == DIALOGUE_LOOT && cmd->type == CMD_LOOT) {
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_HERBALIST && cmd->type == CMD_GIVE) {
        return 1;
    }
    return 0;
}

/*
 * #205: dismiss non-enemy dialogue before mode guards so explore verbs run
 * after close. Enemy menus keep combat-like gating in game_cmd_allowed_in_mode.
 */
static void maybe_close_noncombat_menu(struct GameState *game,
                                       const struct Command *cmd,
                                       GameEventQueue *out)
{
    if (game->mode != GAME_MODE_DIALOGUE || game->dialogue == DIALOGUE_ENEMY) {
        return;
    }
    if (cmd_preserves_noncombat_menu(game, cmd)) {
        return;
    }

    if (game->dialogue == DIALOGUE_LOOT) {
        /* invent-owned leave; same path as loot with no corpse index. */
        (void)game_inv_cmd_loot(game, 0, out);
        return;
    }
    push_dialogue_guard(out, GAME_DIALOGUE_GUARD_DIALOGUE_CLOSED);
    game_set_mode_explore(game);
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

int game_is_busy_dialogue(struct GameState *game)
{
    /* Any non-explore mode suppresses ambient encounters and background prompts. */
    if (game->mode != GAME_MODE_EXPLORE) {
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
 * HUD presence; render reads room_id/items from the event body.
 */
static void do_look(struct GameState *game, GameEventQueue *out)
{
    int i;
    GameEvent *ev;

    ev = game_event_push(out, GAME_EVENT_ROOM_LOOK,
        npc_room_actor(game->player.room_id),
        game->corpse_present[game->player.room_id],
        game->env_focus_active &&
            game->env_focus_room == game->player.room_id &&
            game->tick < game->env_focus_expires_tick,
        game->env_focus_kind, 0);
    if (ev == 0) {
        return;
    }
    ev->room_id = game->player.room_id;
    for (i = 0; i < CFG_AREA_ITEM_SLOTS; ++i) {
        ev->room_item[i] = game->room_item[game->player.room_id][i];
    }
}

/* MAP: generic event only; map layout and terminal output stay in grendr. */
static void do_map(GameEventQueue *out)
{
    game_event_push(out, GAME_EVENT_MAP, 0, 0, 0, 0, 0);
}

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
    game->env_focus_active = 0;
    game->env_focus_room = -1;
    game->env_focus_kind = GAME_ENV_NONE;
    game->env_focus_expires_tick = 0;
    game->herbalist_story = HERBALIST_STORY_NONE;
    game->watchman_story = WATCHMAN_STORY_NONE;
    game->marsh_root_spawned = 0;
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

static int game_cmd_allowed_in_mode(struct GameState *game, struct Command *cmd,
                                    GameEventQueue *out)
{
    /*
     * Combat and enemy handover are narrow modal states; only the small
     * command set that preserves the branch is allowed until the player
     * resolves it.
     */
    if (game->mode == GAME_MODE_COMBAT &&
            cmd->type != CMD_REPLY &&
            cmd->type != CMD_LOOK &&
            cmd->type != CMD_MAP &&
            cmd->type != CMD_BAG &&
            cmd->type != CMD_WIELD &&
            cmd->type != CMD_UNWIELD &&
            cmd->type != CMD_VERSION &&
            cmd->type != CMD_HELP &&
            cmd->type != CMD_QUIT) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY);
        return 0;
    }

    /*
     * Corpse menu is modal like combat; loot again while open acts as "leave".
     * Other explore verbs exit via maybe_close_noncombat_menu before this guard.
     */
    if (game->mode == GAME_MODE_DIALOGUE &&
            game->dialogue == DIALOGUE_LOOT &&
            cmd->type != CMD_REPLY &&
            cmd->type != CMD_LOOT &&
            cmd->type != CMD_LOOK &&
            cmd->type != CMD_MAP &&
            cmd->type != CMD_BAG &&
            cmd->type != CMD_DROP &&
            cmd->type != CMD_VERSION &&
            cmd->type != CMD_HELP &&
            cmd->type != CMD_QUIT) {
        push_dialogue_guard(out, GAME_DIALOGUE_GUARD_LOOT_WAITING_REPLY);
        return 0;
    }

    if (game->mode == GAME_MODE_DIALOGUE &&
            game->dialogue == DIALOGUE_ENEMY &&
            cmd->type != CMD_REPLY &&
            cmd->type != CMD_LOOK &&
            cmd->type != CMD_MAP &&
            cmd->type != CMD_BAG &&
            cmd->type != CMD_WIELD &&
            cmd->type != CMD_UNWIELD &&
            cmd->type != CMD_VERSION &&
            cmd->type != CMD_HELP &&
            cmd->type != CMD_QUIT &&
            !(game_enemy_handover_pick_active(game) && cmd->type == CMD_GIVE)) {
        if (game_enemy_handover_pick_active(game)) {
            push_dialogue_guard(out,
                GAME_DIALOGUE_GUARD_BANDIT_WAITING_HANDOVER_PICK);
        } else {
            push_dialogue_guard(out, GAME_DIALOGUE_GUARD_BANDIT_WAITING_REPLY);
        }
        return 0;
    }

    return 1;
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
    if (cmd->type != CMD_MOVE) {
        return 0;
    }
    if (!world_can_move(&game->world, game->player.room_id, cmd->dir)) {
        /* blocked exit: direction in text; no state change */
        game_event_push(out, GAME_EVENT_CANNOT_MOVE, 0, 0, 0, 0,
            world_dir_name(cmd->dir));
        return 0;
    }
    game->player.room_id = world_move(&game->world, game->player.room_id, cmd->dir);
    game->room_explored[game->player.room_id] = 1;
    if (game->mode == GAME_MODE_DIALOGUE) {
        game_set_mode_explore(game);
    }
    /* room_id already updated; MOVE then ROOM_LOOK preserve render enqueue order */
    game_event_push(out, GAME_EVENT_MOVE, 0, 0, 0, 0, world_dir_name(cmd->dir));
    do_look(game, out);
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
        return game_inv_cmd_eat(game, cmd->arg, out);
    }
    if (cmd->type == CMD_USE) {
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
    /* Menu exit must precede mode guards so the verb is not blocked. */
    maybe_close_noncombat_menu(game, cmd, out);

    if (!game_cmd_allowed_in_mode(game, cmd, out)) {
        return 0;
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
    int fixed_opened;
    int roll;

    /*
     * Tick order is deliberate: resolve any roaming co-location first, then
     * move roaming actors when no encounter opened, then emit ambient events,
     * then open authored fixed encounters. This keeps room-based enemies
     * world-owned instead of spawning at the player site.
     */
    game->tick += 1;
    npc_roaming_update_separation(game);
    npc_story_tick(game);
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
     * encounter check in the new room layout.
     */
    if (!npc_roaming_begin_encounter_in_room(game, game->player.room_id, out) &&
            !game_is_busy_dialogue(game)) {
        npc_roaming_step(game);
        npc_roaming_begin_encounter_in_room(game, game->player.room_id, out);
    }

    world_step(&game->world, game->tick);
    maybe_emit_animal_noise(game, out);
    maybe_emit_atmosphere(game, out);
    if (!game_is_busy_dialogue(game)) {
        fixed_opened = npc_fixed_begin_encounter_in_room(game,
            game->player.room_id, out);
        /*
         * Preserve the old ambient RNG cadence after removing the player-site
         * ambush spawn so unchanged seeds keep their non-encounter outputs.
         */
        roll = plat_rand() % CFG_ROLL_PERCENT_RANGE;
        (void)fixed_opened;
        (void)roll;
    }
}

int game_process_input(struct GameState *game, char *line, GameEventQueue *out)
{
    struct Command cmd;
    int parsed;
    int applied;
    int prior_mode;
    int prior_dialogue;

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
     * Loot replies during DIALOGUE_LOOT are menu picks, not world time; skip
     * the tick that CMD_LOOT would otherwise advance after apply_command.
     */
    if (command_advances_time(cmd.type) &&
            !(cmd.type == CMD_LOOT &&
                prior_mode == GAME_MODE_DIALOGUE &&
                prior_dialogue == DIALOGUE_LOOT)) {
        advance_world_tick(game, out);
    }

    return 1;
}

void game_background_step(struct GameState *game, GameEventQueue *out)
{
    advance_world_tick(game, out);
}
