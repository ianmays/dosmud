#include <stdlib.h>
#include <time.h>
#include "game.h"
#include "grendr.h"
#include "invent.h"
#include "items.h"

int game_xp_to_next_level(int level)
{
    return CFG_XP_LEVEL_BASE + ((level - 1) * CFG_XP_LEVEL_PER_LEVEL);
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
        game->max_hp += CFG_LEVELUP_MAX_HP_DELTA;
        game->damage_bonus += CFG_LEVELUP_DAMAGE_BONUS_DELTA;
        if (game->bag_capacity < CFG_BAG_MAX) {
            game->bag_capacity += CFG_LEVELUP_BAG_CAPACITY_DELTA;
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
    game->enemy_handover_pick = 0;
}

static void combat_start(struct GameState *game)
{
    game->enemy_dialogue = 0;
    game->enemy_handover_pick = 0;
    game->combat_active = 1;
    game->enemy_hp = CFG_COMBAT_ENEMY_HP_BASE + (rand() % CFG_COMBAT_ENEMY_HP_SPREAD);
    game->combat_defending = 0;
    render_combat_start(game->player_hp, game->enemy_hp);
}

static void combat_enemy_turn(struct GameState *game)
{
    int dmg;
    dmg = CFG_COMBAT_ENEMY_DMG_BASE + (rand() % CFG_COMBAT_ENEMY_DMG_SPREAD);
    if (game->combat_defending) {
        dmg -= CFG_COMBAT_DEFEND_DAMAGE_REDUCTION;
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
        dmg = CFG_COMBAT_PLAYER_HIT_BASE + (rand() % CFG_COMBAT_PLAYER_HIT_SPREAD) +
            game->damage_bonus;
        if (game->weapon_equipped != ITEM_NONE) {
            dmg += item_weapon_damage_bonus(game->weapon_equipped);
        }
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
            game->player_hp += CFG_SALVE_HEAL_AMOUNT;
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
        {
            int roll;
            int loot_item;

            roll = rand() % CFG_ROLL_PERCENT_RANGE;
            if (roll < CFG_COMBAT_CORPSE_LOOT_SPEAR_BELOW) {
                loot_item = ITEM_SPEAR;
            } else if (roll < CFG_COMBAT_CORPSE_LOOT_STICK_BELOW) {
                loot_item = ITEM_STICK;
            } else if (roll < CFG_COMBAT_CORPSE_LOOT_BERRY_BELOW) {
                loot_item = ITEM_BERRY;
            } else if (roll < CFG_COMBAT_CORPSE_LOOT_HERB_BELOW) {
                loot_item = ITEM_HERB;
            } else {
                loot_item = ITEM_FISH;
            }
            game->corpse_loot[game->player.room_id] = loot_item;
        }
        gain_xp(game, CFG_COMBAT_KILL_XP_BASE + (rand() % CFG_COMBAT_KILL_XP_SPREAD));
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
    int s;
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

static void maybe_spawn_room_item(struct GameState *game)
{
    int room_id;
    int roll;
    int spawned;

    room_id = game->player.room_id;
    if (room_id < 0 || room_id >= game->world.room_count) {
        return;
    }
    if (!game_room_ground_has_space(game, room_id)) {
        return;
    }
    if ((rand() % CFG_ROLL_PERCENT_RANGE) >= CFG_ROOM_ITEM_SPAWN_GATE) {
        return;
    }
    roll = rand() % CFG_ROLL_PERCENT_RANGE;
    if (roll < CFG_ROOM_SPAWN_ROLL_BERRY_BELOW) spawned = ITEM_BERRY;
    else if (roll < CFG_ROOM_SPAWN_ROLL_STICK_BELOW) spawned = ITEM_STICK;
    else if (roll < CFG_ROOM_SPAWN_ROLL_REED_BELOW) spawned = ITEM_REED;
    else if (roll < CFG_ROOM_SPAWN_ROLL_STONE_BELOW) spawned = ITEM_STONE;
    else if (roll < CFG_ROOM_SPAWN_ROLL_HERB_BELOW) spawned = ITEM_HERB;
    else spawned = ITEM_FISH;
    if (!game_room_ground_try_add(game, room_id, spawned)) {
        return;
    }
    render_nearby_item_notice(item_name(spawned));
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
    game->pond_dialogue = 0;
    game->wanderer_room = WORLD_ROOM_RUINS;
    game->wanderer_dialogue = 0;
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
    game->enemy_dialogue = 0;
    game->enemy_handover_pick = 0;
    game->combat_active = 0;
    game->enemy_hp = 0;
    game->combat_defending = 0;
    game->npc_dialogue = 0;
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

static int apply_command(struct GameState *game, struct Command *cmd)
{
    if (game->combat_active == 1 &&
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

    if (game->enemy_dialogue == 1 &&
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
    if (cmd->type == CMD_GIVE) {
        if (game->enemy_dialogue == 1 && game->enemy_handover_pick == 1) {
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
            game->enemy_dialogue = 0;
            game->enemy_handover_pick = 0;
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
            game->wanderer_return_tick = game->tick + CFG_WANDERER_RETURN_DELAY_BASE +
                (rand() % CFG_WANDERER_RETURN_DELAY_SPREAD);
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
    if ((game->tick % (unsigned long)CFG_ANIMAL_NOISE_TICK_PERIOD) != 0UL) {
        return;
    }
    if ((rand() % CFG_ROLL_PERCENT_RANGE) >= CFG_ANIMAL_NOISE_SKIP_ROLL_GE) {
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

    roll = rand() % CFG_ROLL_PERCENT_RANGE;
    if (roll < CFG_ATMOSPHERE_ROLL_GUST_BELOW) {
        render_atmosphere_gust();
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_RUSTLE_BELOW) {
        render_atmosphere_rustle();
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_RUSTLE;
        game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
        if (game_room_ground_has_space(game, game->player.room_id) &&
                (rand() % CFG_ROLL_PERCENT_RANGE) < CFG_ATMOSPHERE_FOCUS_EXTRA_ITEM_BELOW) {
            if (game_room_ground_try_add(game, game->player.room_id, ITEM_BERRY)) {
                render_atmosphere_berry_drop();
            }
        }
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_CREAK_BELOW) {
        render_atmosphere_creak();
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_CREAK;
        game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_WATER_BELOW) {
        render_atmosphere_water();
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_WATER;
        game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
        if (game_room_ground_has_space(game, game->player.room_id) &&
                (rand() % CFG_ROLL_PERCENT_RANGE) < CFG_ATMOSPHERE_FOCUS_EXTRA_ITEM_BELOW) {
            if (game_room_ground_try_add(game, game->player.room_id, ITEM_REED)) {
                render_atmosphere_reed_drop();
            }
        }
        return;
    }
    if (roll < CFG_ATMOSPHERE_ROLL_GRIT_BELOW) {
        render_atmosphere_grit();
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = GAME_ENV_GRIT;
        game->env_focus_expires_tick = game->tick + CFG_ENV_FOCUS_DURATION_TICKS;
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
