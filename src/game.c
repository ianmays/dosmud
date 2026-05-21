#include <stdlib.h>
#include "game.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"
#include "gatmos.h"
#include "combat.h"
#include "dialogue.h"
#include "genc.h"
#include "wanderer.h"

void game_set_mode_explore(struct GameState *game)
{
    game->mode = GAME_MODE_EXPLORE;
    game->dialogue = DIALOGUE_NONE;
    game->enemy_handover_pick = 0;
}

void game_set_mode_dialogue(struct GameState *game, enum DialogueKind kind)
{
    game->mode = GAME_MODE_DIALOGUE;
    game->dialogue = kind;
    game->enemy_handover_pick = 0;
}

void game_set_mode_combat(struct GameState *game)
{
    game->mode = GAME_MODE_COMBAT;
    game->dialogue = DIALOGUE_NONE;
    game->enemy_handover_pick = 0;
}

int game_is_busy_dialogue(struct GameState *game)
{
    if (game->mode != GAME_MODE_EXPLORE) {
        return 1;
    }
    return 0;
}

static void do_look(struct GameState *game)
{
    render_room_look(game, npc_in_room(game->player.room_id));
}

static void do_map(struct GameState *game)
{
    render_exploration_map(game);
}

void game_describe_current_room(struct GameState *game)
{
    do_look(game);
}

static void reset_mutable_state(struct GameState *game, int room_id, u32 tick)
{
    int i;

    game_set_mode_explore(game);
    game->player.room_id = room_id;
    game->tick = tick;
    game->wanderer_room = WORLD_ROOM_RUINS;
    game->wanderer_need_separation = 0;
    game->env_focus_active = 0;
    game->env_focus_room = -1;
    game->env_focus_kind = GAME_ENV_NONE;
    game->env_focus_expires_tick = 0;
    game->bag_count = 0;
    game->bag_capacity = CFG_START_BAG_CAPACITY;
    game->level = CFG_START_LEVEL;
    game->xp = CFG_START_XP;
    game->max_hp = CFG_START_MAX_HP;
    game->damage_bonus = CFG_START_DAMAGE_BONUS;
    game->weapon_equipped = ITEM_NONE;
    game->player_hp = CFG_START_MAX_HP;
    game->combat.enemy_hp = 0;
    game->combat.defending = 0;
    game->wanderer_active = 1;
    game->wanderer_return_tick = 0;
#ifdef TEST_MODE
    game_roll_inject_clear(game);
    game->test_quiet_ticks = 0;
#endif
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        game->corpse_present[i] = 0;
        game->corpse_loot[i] = ITEM_NONE;
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
    if (game->roll_inject_active) {
        if (game->roll_queue_i >= game->roll_queue_len) {
            /* Past end: count the draw so fully_consumed fails (over-consumption). */
            game->roll_queue_i++;
            return 0;
        }
        return game->roll_queue[game->roll_queue_i++];
    }
    return rand();
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
    game->roll_inject_active = 0;
    game->roll_queue_len = 0;
    game->roll_queue_i = 0;
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
    (void)game;
    roll = rand();
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

static int game_cmd_allowed_in_mode(struct GameState *game, struct Command *cmd)
{
    if (game->mode == GAME_MODE_COMBAT &&
            cmd->type != CMD_REPLY &&
            cmd->type != CMD_LOOK &&
            cmd->type != CMD_MAP &&
            cmd->type != CMD_BAG &&
            cmd->type != CMD_WIELD &&
            cmd->type != CMD_UNWIELD &&
            cmd->type != CMD_HELP &&
            cmd->type != CMD_QUIT) {
        render_msg_bandit_waiting_reply();
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
            cmd->type != CMD_HELP &&
            cmd->type != CMD_QUIT &&
            !(game->enemy_handover_pick == 1 && cmd->type == CMD_GIVE)) {
        if (game->enemy_handover_pick == 1) {
            render_msg_bandit_waiting_handover_pick();
        } else {
            render_msg_bandit_waiting_reply();
        }
        return 0;
    }

    return 1;
}

static int game_cmd_meta(struct GameState *game, struct Command *cmd)
{
    if (cmd->type == CMD_LOOK) {
        do_look(game);
        return 1;
    }
    if (cmd->type == CMD_MAP) {
        do_map(game);
        return 1;
    }
    if (cmd->type == CMD_HELP) {
        game_print_help(cmd->arg);
        return 1;
    }
    if (cmd->type == CMD_QUIT) {
        game->running = 0;
        return 1;
    }
    if (cmd->type == CMD_WAIT) {
        render_msg_wait();
        return 1;
    }
    return 0;
}

static int game_cmd_move(struct GameState *game, struct Command *cmd)
{
    if (cmd->type != CMD_MOVE) {
        return 0;
    }
    if (!world_can_move(&game->world, game->player.room_id, cmd->dir)) {
        render_msg_cannot_move(world_dir_name(cmd->dir));
        return 0;
    }
    game->player.room_id = world_move(&game->world, game->player.room_id, cmd->dir);
    game->room_explored[game->player.room_id] = 1;
    if (game->mode == GAME_MODE_DIALOGUE) {
        game_set_mode_explore(game);
    }
    render_msg_moved(world_dir_name(cmd->dir));
    do_look(game);
    return 1;
}

static int game_cmd_inventory(struct GameState *game, struct Command *cmd)
{
    if (cmd->type == CMD_LOOT) {
        return game_inv_cmd_loot(game);
    }
    if (cmd->type == CMD_TAKE) {
        return game_inv_cmd_take(game, cmd->arg);
    }
    if (cmd->type == CMD_DROP) {
        return game_inv_cmd_drop(game, cmd->arg);
    }
    if (cmd->type == CMD_BAG) {
        return game_inv_cmd_bag(game);
    }
    if (cmd->type == CMD_WIELD) {
        return game_inv_cmd_wield(game, cmd->arg);
    }
    if (cmd->type == CMD_UNWIELD) {
        return game_inv_cmd_unwield(game);
    }
    if (cmd->type == CMD_EAT) {
        return game_inv_cmd_eat(game, cmd->arg);
    }
    if (cmd->type == CMD_USE) {
        return game_inv_cmd_use(game, cmd->arg);
    }
    if (cmd->type == CMD_CRAFT) {
        return game_inv_cmd_craft(game, cmd->arg);
    }
    return 0;
}

static int game_cmd_reply(struct GameState *game, struct Command *cmd)
{
    if (cmd->type != CMD_REPLY) {
        return 0;
    }
    if (game->mode == GAME_MODE_COMBAT) {
        combat_resolve_reply(game, cmd->arg);
        return 1;
    }
    if (dialogue_cmd_reply(game, cmd->arg)) {
        return 1;
    }
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
        return genc_cmd_reply(game, cmd->arg);
    }
    if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_WANDERER) {
        return wanderer_cmd_reply(game, cmd->arg);
    }
    render_msg_nobody_waiting_reply();
    return 1;
}

static int apply_command(struct GameState *game, struct Command *cmd)
{
    if (!game_cmd_allowed_in_mode(game, cmd)) {
        return 0;
    }
    if (game_cmd_meta(game, cmd)) {
        return 1;
    }
    if (game_cmd_move(game, cmd)) {
        return 1;
    }
    if (game_cmd_inventory(game, cmd)) {
        return 1;
    }
    if (cmd->type == CMD_INSPECT) {
        return gatmos_cmd_inspect(game, cmd->arg);
    }
    if (cmd->type == CMD_TALK) {
        return dialogue_cmd_talk(game);
    }
    if (game_cmd_reply(game, cmd)) {
        return 1;
    }
    if (cmd->type == CMD_GIVE) {
        return genc_cmd_give(game, cmd->arg);
    }
    return 0;
}

static void advance_world_tick(struct GameState *game, int wanderer_moves_first)
{
    int old_wanderer_room;

    game->tick += 1;
    wanderer_update_separation(game);
#ifdef TEST_MODE
    if (game->test_quiet_ticks) {
        world_step(&game->world, game->tick);
        return;
    }
#endif
    if (!game->wanderer_active && game->tick >= game->wanderer_return_tick) {
        game->wanderer_active = 1;
        game->wanderer_room = rand() % game->world.room_count;
    }

    old_wanderer_room = game->wanderer_room;
    if (game->wanderer_active) {
        if (wanderer_moves_first) {
            wanderer_step(game);
        }
        if (game->player.room_id == game->wanderer_room) {
            wanderer_begin_encounter(game);
        } else if (!wanderer_moves_first) {
            wanderer_step(game);
            if (game->player.room_id == game->wanderer_room) {
                wanderer_begin_encounter(game);
            }
        } else if (old_wanderer_room != game->wanderer_room &&
                game->player.room_id == game->wanderer_room) {
            wanderer_begin_encounter(game);
        }
    }

    world_step(&game->world, game->tick);
    maybe_emit_animal_noise(game);
    maybe_emit_atmosphere(game);
    if (!game_is_busy_dialogue(game) &&
            (rand() % CFG_ROLL_PERCENT_RANGE) < CFG_BANDIT_ENCOUNTER_CHANCE_BELOW) {
        enemy_begin_encounter(game);
    }
}

int game_process_input(struct GameState *game, char *line)
{
    struct Command cmd;
    int parsed;
    int applied;

    parsed = command_parse(line, &cmd);
    if (!parsed) {
        render_msg_unknown_command();
        return 0;
    }

    applied = apply_command(game, &cmd);
    if (!applied) {
        return 0;
    }

    if (command_advances_time(cmd.type)) {
        if (cmd.type == CMD_MOVE) {
            advance_world_tick(game, 0);
        } else {
            advance_world_tick(game, 1);
        }
    }

    return 1;
}

void game_background_step(struct GameState *game)
{
    advance_world_tick(game, 1);
}
