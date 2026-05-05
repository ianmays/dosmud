#include <stdlib.h>
#include <time.h>
#include "game.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"

int game_xp_to_next_level(int level)
{
    return 20 + ((level - 1) * 15);
}

static void gain_xp(struct GameState *game, int amount)
{
    int needed;
    game->xp += amount;
    render_xp_gained(amount);
    needed = game_xp_to_next_level(game->level);
    while (game->xp >= needed) {
        game->xp -= needed;
        game->level += 1;
        game->max_hp += 4;
        game->damage_bonus += 1;
        if (game->bag_capacity < CFG_BAG_MAX) {
            game->bag_capacity += 1;
        }
        game->player_hp = game->max_hp;
        render_level_up(game->level, game->max_hp, game->damage_bonus,
            game->bag_capacity);
        needed = game_xp_to_next_level(game->level);
    }
}

static int game_is_busy_dialogue(struct GameState *game)
{
    if (game->wanderer_dialogue == 1) return 1;
    if (game->pond_dialogue == 1) return 1;
    if (game->npc_dialogue != 0) return 1;
    if (game->enemy_dialogue == 1) return 1;
    if (game->combat_active == 1) return 1;
    return 0;
}

static int npc_in_room(int room_id)
{
    if (room_id == WORLD_ROOM_TOWER) return 1;  /* watchman */
    if (room_id == WORLD_ROOM_ORCHARD) return 2;/* herbalist */
    if (room_id == WORLD_ROOM_CATACOMBS) return 3; /* archivist */
    return 0;
}

static void enemy_begin_encounter(struct GameState *game)
{
    if (game_is_busy_dialogue(game)) {
        return;
    }
    render_bandit_encounter_open();
    game->enemy_dialogue = 1;
}

static void combat_start(struct GameState *game)
{
    game->enemy_dialogue = 0;
    game->combat_active = 1;
    game->enemy_hp = 8 + (rand() % 5);
    game->combat_defending = 0;
    render_combat_start(game->player_hp, game->enemy_hp);
}

static void combat_enemy_turn(struct GameState *game)
{
    int dmg;
    dmg = 1 + (rand() % 4);
    if (game->combat_defending) {
        dmg -= 2;
        if (dmg < 0) dmg = 0;
    }
    if (dmg > 0) {
        game->player_hp -= dmg;
    }
    render_combat_enemy_strike(dmg);
    if (game->player_hp <= 0) {
        game->player_hp = 0;
        render_combat_player_fallen();
        game->running = 0;
        return;
    }
    render_combat_status_line(game->player_hp, game->enemy_hp);
}

static void combat_resolve_reply(struct GameState *game, int choice)
{
    int dmg;
    if (choice == 1) {
        dmg = 2 + (rand() % 4) + game->damage_bonus;
        game->enemy_hp -= dmg;
        render_combat_player_hit(dmg);
    } else if (choice == 2) {
        game->combat_defending = 1;
        render_combat_braced();
    } else if (choice == 3) {
        if (game_inv_bag_find_index(game, ITEM_SALVE) < 0) {
            render_combat_no_salve_bag();
        } else {
            game_inv_bag_remove_item(game, ITEM_SALVE);
            game->player_hp += 5;
            if (game->player_hp > game->max_hp) game->player_hp = game->max_hp;
            render_combat_salve_in_combat(game->player_hp);
        }
    } else {
        render_combat_invalid_choice();
        return;
    }

    if (game->enemy_hp <= 0) {
        game->enemy_hp = 0;
        game->combat_active = 0;
        game->combat_defending = 0;
        render_combat_bandit_defeated();
        game->corpse_present[game->player.room_id] = 1;
        game->corpse_loot[game->player.room_id] = (rand() % 2) ? ITEM_STONE : ITEM_HERB;
        gain_xp(game, 12 + (rand() % 5));
        return;
    }

    combat_enemy_turn(game);
    game->combat_defending = 0;
    if (game->running && game->combat_active) {
        render_combat_menu();
    }
}

static void seed_world_items(struct GameState *game)
{
    int i;
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        game->room_item[i] = ITEM_NONE;
    }
    game->room_item[WORLD_ROOM_CAMP] = ITEM_STICK;
    game->room_item[WORLD_ROOM_ROAD] = ITEM_STONE;
    game->room_item[WORLD_ROOM_POND] = ITEM_FISH;
    game->room_item[WORLD_ROOM_FOREST] = ITEM_HERB;
    game->room_item[WORLD_ROOM_RUINS] = ITEM_STONE;
    game->room_item[WORLD_ROOM_STREAM] = ITEM_REED;
    game->room_item[WORLD_ROOM_MARSH] = ITEM_REED;
    game->room_item[WORLD_ROOM_MEADOW] = ITEM_BERRY;
    game->room_item[WORLD_ROOM_ORCHARD] = ITEM_BERRY;
    game->room_item[WORLD_ROOM_CANYON] = ITEM_STONE;
    game->room_item[WORLD_ROOM_CAVE] = ITEM_HERB;
}

static void maybe_spawn_room_item(struct GameState *game)
{
    int room_id;
    int roll;
    room_id = game->player.room_id;
    if (room_id < 0 || room_id >= game->world.room_count) {
        return;
    }
    if (game->room_item[room_id] != ITEM_NONE) {
        return;
    }
    if ((rand() % 100) >= 20) {
        return;
    }
    roll = rand() % 100;
    if (roll < 25) game->room_item[room_id] = ITEM_BERRY;
    else if (roll < 45) game->room_item[room_id] = ITEM_STICK;
    else if (roll < 65) game->room_item[room_id] = ITEM_REED;
    else if (roll < 80) game->room_item[room_id] = ITEM_STONE;
    else if (roll < 92) game->room_item[room_id] = ITEM_HERB;
    else game->room_item[room_id] = ITEM_FISH;
    render_nearby_item_notice(item_name(game->room_item[room_id]));
}

static void wanderer_update_separation(struct GameState *game)
{
    if (game->player.room_id != game->wanderer_room) {
        game->wanderer_need_separation = 0;
    }
}

static void wanderer_step(struct GameState *game)
{
    struct Room *r;
    int dirs[CFG_DIR_MAX];
    int n;
    int i;
    int pick;

    if (game->world.room_count <= 0) {
        return;
    }
    if (game->wanderer_room < 0 || game->wanderer_room >= game->world.room_count) {
        return;
    }
    r = &game->world.rooms[game->wanderer_room];
    n = 0;
    for (i = 0; i < DIR_NONE; ++i) {
        if (r->exits[i] >= 0) {
            dirs[n] = i;
            ++n;
        }
    }
    if (n <= 0) {
        return;
    }
    pick = rand() % n;
    game->wanderer_room = r->exits[dirs[pick]];
}

static void wanderer_begin_encounter(struct GameState *game)
{
    if (game_is_busy_dialogue(game)) {
        return;
    }
    if (game->wanderer_need_separation) {
        return;
    }
    game->pond_dialogue = 0;
    render_wanderer_scene();
    game->wanderer_dialogue = 1;
    game->wanderer_need_separation = 1;
}

static void wanderer_apply_reply(int choice)
{
    render_wanderer_reply(choice);
}

static void frog_dialogue_intro(void)
{
    render_frog_dialogue_intro();
}

static void frog_dialogue_branch(int choice)
{
    render_frog_dialogue_branch(choice);
}

static void do_look(struct GameState *game)
{
    render_room_look(game, npc_in_room(game->player.room_id));
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
    game->seed = 1;
    game->running = 1;
    game->pond_dialogue = 0;
    game->wanderer_room = WORLD_ROOM_RUINS;
    game->wanderer_dialogue = 0;
    game->wanderer_need_separation = 0;
    game->env_focus_active = 0;
    game->env_focus_room = -1;
    game->env_focus_kind = GAME_ENV_NONE;
    game->env_focus_expires_tick = 0;
    game->bag_count = 0;
    game->bag_capacity = 5;
    game->level = 1;
    game->xp = 0;
    game->max_hp = 20;
    game->damage_bonus = 0;
    game->player_hp = 20;
    game->enemy_dialogue = 0;
    game->combat_active = 0;
    game->enemy_hp = 0;
    game->combat_defending = 0;
    game->npc_dialogue = 0;
    game->wanderer_active = 1;
    game->wanderer_return_tick = 0;
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        game->corpse_present[i] = 0;
        game->corpse_loot[i] = ITEM_NONE;
    }
    seed_world_items(game);
}

static int apply_command(struct GameState *game, struct Command *cmd)
{
    if ((game->enemy_dialogue == 1 || game->combat_active == 1) &&
            cmd->type != CMD_REPLY &&
            cmd->type != CMD_LOOK &&
            cmd->type != CMD_BAG &&
            cmd->type != CMD_HELP &&
            cmd->type != CMD_QUIT) {
        render_msg_bandit_waiting_reply();
        return 0;
    }

    if (cmd->type == CMD_LOOK) {
        do_look(game);
        return 1;
    }
    if (cmd->type == CMD_HELP) {
        game_print_help();
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
        game->pond_dialogue = 0;
        game->wanderer_dialogue = 0;
        game->npc_dialogue = 0;
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
    if (cmd->type == CMD_BAG) {
        return game_inv_cmd_bag(game);
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
        if (game->enemy_dialogue == 1 || game->combat_active == 1) {
            render_msg_bandit_blocks_talk();
            return 1;
        }
        if (game->wanderer_dialogue == 1) {
            render_msg_traveler_waiting();
            return 1;
        }
        if (game->player.room_id == WORLD_ROOM_TOWER) {
            render_msg_watchman_talk();
            game->npc_dialogue = 2;
            return 1;
        }
        if (game->player.room_id == WORLD_ROOM_ORCHARD) {
            render_msg_herbalist_talk();
            game->npc_dialogue = 3;
            return 1;
        }
        if (game->player.room_id == WORLD_ROOM_CATACOMBS) {
            render_msg_archivist_talk();
            game->npc_dialogue = 4;
            return 1;
        }
        if (game->player.room_id != WORLD_ROOM_POND) {
            render_msg_nobody_talk();
            return 1;
        }
        frog_dialogue_intro();
        game->pond_dialogue = 1;
        return 1;
    }
    if (cmd->type == CMD_REPLY) {
        if (game->combat_active == 1) {
            combat_resolve_reply(game, cmd->arg);
            return 1;
        }
        if (game->npc_dialogue == 2) {
            render_msg_watchman_reply(cmd->arg);
            game->npc_dialogue = 0;
            return 1;
        }
        if (game->npc_dialogue == 3) {
            render_msg_herbalist_reply(cmd->arg);
            game->npc_dialogue = 0;
            return 1;
        }
        if (game->npc_dialogue == 4) {
            render_msg_archivist_reply(cmd->arg);
            game->npc_dialogue = 0;
            return 1;
        }
        if (game->enemy_dialogue == 1) {
            if (cmd->arg == 1) {
                combat_start(game);
                return 1;
            }
            if (cmd->arg == 2) {
                if (game->bag_count <= 0) {
                    render_msg_bag_empty_bandit();
                    combat_start(game);
                    return 1;
                }
                render_msg_hand_over_item(item_name(game->bag[0]));
                game_inv_bag_remove_index(game, 0);
                game->enemy_dialogue = 0;
                return 1;
            }
            if (cmd->arg == 3) {
                if ((rand() % 100) < 60) {
                    render_msg_intimidate_success();
                    game->enemy_dialogue = 0;
                } else {
                    render_msg_intimidate_fail();
                    combat_start(game);
                }
                return 1;
            }
            render_msg_pick_123();
            return 1;
        }
        if (game->wanderer_dialogue == 1) {
            if (cmd->arg < 1 || cmd->arg > 3) {
                render_msg_pick_123();
                return 1;
            }
            wanderer_apply_reply(cmd->arg);
            game->wanderer_dialogue = 0;
            game->wanderer_active = 0;
            game->wanderer_room = -1;
            game->wanderer_return_tick = game->tick + 8 + (rand() % 16);
            return 1;
        }
        if (game->pond_dialogue != 1) {
            render_msg_nobody_waiting_reply();
            return 1;
        }
        if (cmd->arg < 1 || cmd->arg > 3) {
            render_msg_pick_123();
            return 1;
        }
        frog_dialogue_branch(cmd->arg);
        game->pond_dialogue = 0;
        return 1;
    }

    return 0;
}

static void maybe_emit_animal_noise(struct GameState *game)
{
    if ((game->tick % 2UL) != 0UL) {
        return;
    }
    if ((rand() % 100) >= 75) {
        return;
    }
    render_animal_noise_line(
        world_room_animal_noise(&game->world, game->player.room_id));
}

static void maybe_emit_atmosphere(struct GameState *game)
{
    int roll;

    if (game->env_focus_active && game->tick >= game->env_focus_expires_tick) {
        game->env_focus_active = 0;
        game->env_focus_room = -1;
        game->env_focus_kind = GAME_ENV_NONE;
        game->env_focus_expires_tick = 0;
    }

    roll = rand() % 100;
    if (roll < 35) {
        render_atmosphere_gust();
        return;
    }
    if (roll < 55) {
        render_atmosphere_rustle();
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_RUSTLE;
        game->env_focus_expires_tick = game->tick + 3;
        if (game->room_item[game->player.room_id] == ITEM_NONE && (rand() % 100) < 50) {
            game->room_item[game->player.room_id] = ITEM_BERRY;
            render_atmosphere_berry_drop();
        }
        return;
    }
    if (roll < 70) {
        render_atmosphere_creak();
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_CREAK;
        game->env_focus_expires_tick = game->tick + 3;
        return;
    }
    if (roll < 82) {
        render_atmosphere_water();
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_WATER;
        game->env_focus_expires_tick = game->tick + 3;
        if (game->room_item[game->player.room_id] == ITEM_NONE && (rand() % 100) < 50) {
            game->room_item[game->player.room_id] = ITEM_REED;
            render_atmosphere_reed_drop();
        }
        return;
    }
    if (roll < 92) {
        render_atmosphere_grit();
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_GRIT;
        game->env_focus_expires_tick = game->tick + 3;
        return;
    }
    maybe_spawn_room_item(game);
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
    if (!game_is_busy_dialogue(game) && (rand() % 100) < 14) {
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
