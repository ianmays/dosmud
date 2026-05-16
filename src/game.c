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

void game_set_mode_dialogue(struct GameState *game, int kind)
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

void game_init(struct GameState *game)
{
    int i;
    world_init(&game->world);
    game->player.room_id = 0;
    game->tick = 0;
    game->seed = CFG_GAME_INIT_SEED;
    game->running = 1;
    game_set_mode_explore(game);
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
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        game->corpse_present[i] = 0;
        game->corpse_loot[i] = ITEM_NONE;
        game->room_explored[i] = 0;
    }
    game->room_explored[game->player.room_id] = 1;
    seed_world_items(game);
}

static int apply_room_npc_reply(struct GameState *game, struct Command *cmd)
{
    if (game->mode != GAME_MODE_DIALOGUE) {
        return 0;
    }
    if (game->dialogue == DIALOGUE_NPC_FROG) {
        if (cmd->arg < 1 || cmd->arg > 3) {
            render_msg_pick_123();
            return 1;
        }
        frog_dialogue_branch(cmd->arg);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_WATCHMAN) {
        render_msg_watchman_reply(cmd->arg);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_HERBALIST) {
        render_msg_herbalist_reply(cmd->arg);
        game_set_mode_explore(game);
        return 1;
    }
    if (game->dialogue == DIALOGUE_NPC_ARCHIVIST) {
        render_msg_archivist_reply(cmd->arg);
        game_set_mode_explore(game);
        return 1;
    }
    return 0;
}

static int apply_command(struct GameState *game, struct Command *cmd)
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
    if (cmd->type == CMD_MOVE) {
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
    if (cmd->type == CMD_LOOT) {
        return game_inv_cmd_loot(game);
    }
    if (cmd->type == CMD_TAKE) {
        return game_inv_cmd_take(game, cmd->arg);
    }
    if (cmd->type == CMD_DROP) {
        return game_inv_cmd_drop(game, cmd->arg);
    }
    if (cmd->type == CMD_GIVE) {
        if (game->mode == GAME_MODE_DIALOGUE &&
                game->dialogue == DIALOGUE_ENEMY &&
                game->enemy_handover_pick == 1) {
            if (!game_inv_player_has_item(game, cmd->arg)) {
                render_msg_bandit_give_not_carrying();
                return 1;
            }
            render_msg_hand_over_item(item_name(cmd->arg));
            if (game->weapon_equipped == cmd->arg) {
                game->weapon_equipped = ITEM_NONE;
            } else {
                game_inv_bag_remove_item(game, cmd->arg);
            }
            game_set_mode_explore(game);
            return 1;
        }
        render_msg_give_wrong_context();
        return 1;
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
    if (cmd->type == CMD_INSPECT) {
        if (!game->env_focus_active ||
                game->env_focus_room != game->player.room_id ||
                game->tick >= game->env_focus_expires_tick) {
            render_msg_inspect_nothing();
            game->env_focus_active = 0;
            game->env_focus_room = -1;
            game->env_focus_kind = GAME_ENV_NONE;
            game->env_focus_expires_tick = 0;
            return 1;
        }
        if (cmd->arg != 0 && cmd->arg != game->env_focus_kind) {
            render_msg_inspect_wrong_focus();
            return 1;
        }
        if (game->env_focus_kind == GAME_ENV_RUSTLE) {
            render_msg_inspect_rustle();
        } else if (game->env_focus_kind == GAME_ENV_CREAK) {
            render_msg_inspect_creak();
        } else if (game->env_focus_kind == GAME_ENV_WATER) {
            render_msg_inspect_water();
        } else if (game->env_focus_kind == GAME_ENV_GRIT) {
            render_msg_inspect_grit();
        }
        game->env_focus_active = 0;
        game->env_focus_room = -1;
        game->env_focus_kind = GAME_ENV_NONE;
        game->env_focus_expires_tick = 0;
        return 1;
    }
    if (cmd->type == CMD_TALK) {
        if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
            render_msg_bandit_blocks_talk();
            return 1;
        }
        if (game->mode == GAME_MODE_COMBAT) {
            render_msg_bandit_blocks_talk();
            return 1;
        }
        if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_WANDERER) {
            render_msg_traveler_waiting();
            return 1;
        }
        if (game->player.room_id == WORLD_ROOM_TOWER) {
            render_msg_watchman_talk();
            game_set_mode_dialogue(game, DIALOGUE_NPC_WATCHMAN);
            return 1;
        }
        if (game->player.room_id == WORLD_ROOM_ORCHARD) {
            render_msg_herbalist_talk();
            game_set_mode_dialogue(game, DIALOGUE_NPC_HERBALIST);
            return 1;
        }
        if (game->player.room_id == WORLD_ROOM_CATACOMBS) {
            render_msg_archivist_talk();
            game_set_mode_dialogue(game, DIALOGUE_NPC_ARCHIVIST);
            return 1;
        }
        if (game->player.room_id != WORLD_ROOM_POND) {
            render_msg_nobody_talk();
            return 1;
        }
        frog_dialogue_intro();
        game_set_mode_dialogue(game, DIALOGUE_NPC_FROG);
        return 1;
    }
    if (cmd->type == CMD_REPLY) {
        if (game->mode == GAME_MODE_COMBAT) {
            combat_resolve_reply(game, cmd->arg);
            return 1;
        }
        if (apply_room_npc_reply(game, cmd)) {
            return 1;
        }
        if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_ENEMY) {
            if (cmd->arg == 1) {
                combat_start(game);
                return 1;
            }
            if (cmd->arg == 2) {
                if (game->bag_count <= 0 && game->weapon_equipped == ITEM_NONE) {
                    render_msg_bag_empty_bandit();
                    combat_start(game);
                    return 1;
                }
                game->enemy_handover_pick = 1;
                render_bandit_handover_pick_prompt();
                return 1;
            }
            if (cmd->arg == 3) {
                game->enemy_handover_pick = 0;
                if ((rand() % CFG_ROLL_PERCENT_RANGE) < CFG_BANDIT_INTIMIDATE_SUCCESS_BELOW) {
                    render_msg_intimidate_success();
                    game_set_mode_explore(game);
                } else {
                    render_msg_intimidate_fail();
                    combat_start(game);
                }
                return 1;
            }
            render_msg_pick_123();
            return 1;
        }
        if (game->mode == GAME_MODE_DIALOGUE && game->dialogue == DIALOGUE_WANDERER) {
            if (cmd->arg < 1 || cmd->arg > 3) {
                render_msg_pick_123();
                return 1;
            }
            wanderer_apply_reply(cmd->arg);
            game_set_mode_explore(game);
            game->wanderer_active = 0;
            game->wanderer_room = -1;
            game->wanderer_return_tick = game->tick + CFG_WANDERER_RETURN_DELAY_BASE +
                (rand() % CFG_WANDERER_RETURN_DELAY_SPREAD);
            return 1;
        }
        render_msg_nobody_waiting_reply();
        return 1;
    }

    return 0;
}

static void advance_world_tick(struct GameState *game, int wanderer_moves_first)
{
    int old_wanderer_room;

    game->tick += 1;
    wanderer_update_separation(game);
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
