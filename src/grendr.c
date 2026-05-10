#include <stdio.h>
#include "grendr.h"
#include "game.h"
#include "items.h"
#include "command.h"
#include "world.h"
#include "txtres.h"

static void art_room_camp(void)
{
    printf("                *          .            *\n");
    printf("         .            .           .           .\n");
    printf("                        /\\\n");
    printf("                       /  \\        /\\\n");
    printf("                      /____\\      /  \\\n");
    printf("                 /\\   |    |     /____\\\n");
    printf("                /  \\  |[]  |  /\\ | [] |\n");
    printf("               /____\\ |    | /  \\|    |\n");
    printf("               |    |/______\\____/______\\\n");
    printf("               | [] /  /\\      /\\      / \\\n");
    printf("               |   /  /  \\____/  \\____/   \\\n");
    printf("              /___/_________________________\\\n");
    printf("                    (  )   (  )   (  )\n");
    printf("                     )(___)(___(__(\n");
    printf("                    (____(____)____)\n");
    printf("                     \\   \\|/   / \n");
    printf("                      \\   |   /\n");
    printf("                       '. | .'\n");
    printf("                         \\|/\n");
    printf("                         / \\\n");
    printf("                    %s\n", g_room_art_captions[WORLD_ROOM_CAMP]);
}

static void art_room_road(void)
{
    printf("           /\\                        /\\\n");
    printf("          /  \\        _.._          /  \\\n");
    printf("         /    \\    .-'    '-.      /    \\\n");
    printf("        /      \\__/  _  _    \\____/      \\\n");
    printf("       /  /\\                        /\\    \\\n");
    printf("      /  /  \\                      /  \\    \\\n");
    printf("     /__/____\\____________________/____\\____\\\n");
    printf("                  ||        ||\n");
    printf("                  ||        ||\n");
    printf("                  ||        ||\n");
    printf("__________________||________||________________________\n");
    printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
    printf("  .     .      .     .      .     .      .     .\n");
    printf("    .      .      .      .      .      .      .\n");
    printf("      .      .      .      .      .      .\n");
    printf("                  %s\n", g_room_art_captions[WORLD_ROOM_ROAD]);
}

static void art_room_pond(void)
{
    printf("                    _..-~~~~-.._\n");
    printf("               _.-~`  ~  ~   ~  `~-._\n");
    printf("            .-`  ~   _..----.._   ~  `-.\n");
    printf("          .'  ~   .-'  .--.   '-.   ~   '.\n");
    printf("         / ~   .-'   .(____).    '-.  ~   \\\n");
    printf("        /_____/________________________\\____\\\n");
    printf("        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("      ~~~   ~~~~~   ~~~~~   ~~~~~   ~~~~~   ~~~\n");
    printf("    ~~~   ~~~   ~~~   ~~~   ~~~   ~~~   ~~~   ~~~\n");
    printf("         .-._                               _.-.\n");
    printf("       .'    '.         .-''''-.          .'    '.\n");
    printf("      /  o  o  \\       /  .--.  \\        /  o  o  \\\n");
    printf("      |   --   |      |  (____)  |       |   --   |\n");
    printf("      \\  '__' /        \\  '--'  /        \\  '__' /\n");
    printf("       '.___.'          '.___.'           '.___.'\n");
    printf("                %s\n", g_room_art_captions[WORLD_ROOM_POND]);
}

static void art_room_forest(void)
{
    printf("               &&& &&  & &&\n");
    printf("          && &\\/&\\|& ()|/ @, &&\n");
    printf("          &\\/(/&/&||/& /_/)_&/_&\n");
    printf("       &() &\\/&|()|/&\\/ '%%\" & ()\n");
    printf("      &_\\_&&_\\ |& |&&/&__%%_/_& &&\n");
    printf("    &&   && & &| &| /& & %% ()& /&&\n");
    printf("      ()&_---()&\\&\\|&&-&&--%%---()~\n");
    printf("          &&     \\||| \n");
    printf("                  |||\n");
    printf("                  |||\n");
    printf("                  |||\n");
    printf("            ______|||______\n");
    printf("           /_____/////_____\\\n");
    printf("              %s\n", g_room_art_captions[WORLD_ROOM_FOREST]);
}

static void art_room_stream(void)
{
    printf("                .      .      .      .\n");
    printf("            .      .      .      .      .\n");
    printf("      _..-~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-.._\n");
    printf("   .-~   ~~~~    ~~~~    ~~~~    ~~~~    ~   ~-.\n");
    printf("  /   ~~~   ~~~~   ~~~~   ~~~~   ~~~~   ~~~    \\\n");
    printf(" | ~~~~   ~~~~   ~~~~   ~~~~   ~~~~   ~~~~  ~  |\n");
    printf(" |_______________________________________________|\n");
    printf("      o   o     o    o     o    o     o    o\n");
    printf("    _/|\\_/|\\___/|\\__/|\\___/|\\__/|\\___/|\\__/|\\_\n");
    printf("   /____________________________________________\\\n");
    printf("              %s\n", g_room_art_captions[WORLD_ROOM_STREAM]);
}

static void art_room_ruins(void)
{
    printf("                 |\\                       /|\n");
    printf("                 | \\        ____         / |\n");
    printf("                 |  \\    .-' __ '-.     /  |\n");
    printf("                 |   |  /   /  \\   \\   |   |\n");
    printf("             ____|___|_|___|____|___|__|___|____\n");
    printf("            /   _    _    _    _    _    _    \\\n");
    printf("           /___|_|__|_|__|_|__|_|__|_|__|_|____\\\n");
    printf("           |  _   _   _   _   _   _   _   _   |\n");
    printf("           | |_| |_| |_| |_| |_| |_| |_| |_|  |\n");
    printf("           |  _   _   _   _   _   _   _   _   |\n");
    printf("           |___________________________________|\n");
    printf("          / / / / / / / / / / / / / / / / / / /\n");
    printf("         /_/ /_/ /_/ /_/ /_/ /_/ /_/ /_/ /_/ /_\n");
    printf("                %s\n", g_room_art_captions[WORLD_ROOM_RUINS]);
}

static void art_room_cliff(void)
{
    printf("                           /\\\n");
    printf("                          /  \\\n");
    printf("                 /\\      / /\\ \\      /\\\n");
    printf("                /  \\    / /  \\ \\    /  \\\n");
    printf("               / /\\ \\__/ /____\\ \\__/ /\\ \\\n");
    printf("              / /  \\____/      \\____/  \\ \\\n");
    printf("             /_/                        \\_\\\n");
    printf("             ||   ____            ____   ||\n");
    printf("             ||  / __ \\__________/ __ \\  ||\n");
    printf("             ||_/ /  \\____________/  \\_\\_||\n");
    printf("~~~~~~~~~~~~~~~~~~ %s ~~~~~~~~~~~~~~~~~~\n", g_room_art_captions[WORLD_ROOM_CLIFF]);
}

static void art_room_marsh(void)
{
    printf("       ||      ||      ||      ||\n");
    printf("       ||      ||      ||      ||\n");
    printf("   _.-~||~--._ ||  _.-~||~--._ ||\n");
    printf(".-~  ~ || ~  ~'||-~  ~ || ~  ~'||-.\n");
    printf("|  ~   ||   ~  ||  ~   ||   ~  || |\n");
    printf("|  _..-||-.._  ||  _..-||-.._  || |\n");
    printf("|.'    ||    '.||.'    ||    '.|| |\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("           %s\n", g_room_art_captions[WORLD_ROOM_MARSH]);
}

static void art_room_grove(void)
{
    printf("                .-~~~~~~~~~~~~-.\n");
    printf("            .-~   .-~~~~~~-.    ~-.\n");
    printf("         .-~    .'  .--.    '.     ~-.\n");
    printf("        /     .'   /    \\     '.      \\\n");
    printf("       /     /    / /\\   \\      \\      \\\n");
    printf("      |     |    | |  |   |      |      |\n");
    printf("      |     |    | |  |   |      |      |\n");
    printf("      |      \\    \\ \\/   /      /       |\n");
    printf("       \\      '.   '--' .'      /       /\n");
    printf("        '.      '-.__.-'      .'      .'\n");
    printf("          '-._              _.-'  _.-'\n");
    printf("               '----------'   .-'\n");
    printf("                 %s\n", g_room_art_captions[WORLD_ROOM_GROVE]);
}

static void art_room_bridge(void)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  ~~~~~~~~~~~~~~~~\n");
    printf("      ||\\                                   /||\n");
    printf("      || \\_________________________________/ ||\n");
    printf("      ||  |  |  |  |  |  |  |  |  |  |  |   ||\n");
    printf("======||==|==|==|==|==|==|==|==|==|==|==|===||======\n");
    printf("      ||  |  |  |  |  |  |  |  |  |  |  |   ||\n");
    printf("      ||_/_________________________________\\_||\n");
    printf("      ||/                                   \\||\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  ~~~~~~~~~~~~~~~~\n");
}

static void art_room_catacombs(void)
{
    printf("              __________________________\n");
    printf("           .-'__________________________'-.\n");
    printf("         .'  ___   ___   ___   ___   ___  '.\n");
    printf("        /   /___\\ /___\\ /___\\ /___\\ /___\\  \\\n");
    printf("       |    |   | |   | |   | |   | |   |   |\n");
    printf("       |    |___| |___| |___| |___| |___|   |\n");
    printf("       |                                     |\n");
    printf("       |    |___| |___| |___| |___| |___|   |\n");
    printf("       |    |   | |   | |   | |   | |   |   |\n");
    printf("        \\___________________________________/\n");
    printf("                 %s\n", g_room_art_captions[WORLD_ROOM_CATACOMBS]);
}

static void art_room_meadow(void)
{
    printf("             \\  |  /            \\  |  /\n");
    printf("          '.   \\|/    .'      '.   \\|/    .'\n");
    printf("        --  *   *   *  --   --  *   *   *  --\n");
    printf("          .'   /|\\    '.      .'   /|\\    .'\n");
    printf("             /  |  \\            /  |  \\\n");
    printf("     .             .      .              .\n");
    printf("  .     .      .      .      .      .      .\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("            %s\n", g_room_art_captions[WORLD_ROOM_MEADOW]);
}

static void art_room_canyon(void)
{
    printf("            /\\                         /\\\n");
    printf("           /  \\      __      __       /  \\\n");
    printf("          / /\\ \\____/  \\____/  \\_____/ /\\ \\\n");
    printf("         / /  \\________________________/  \\ \\\n");
    printf("        /_/    /  /  /  /  /  /  /  /    \\_\\\n");
    printf("        ||    /__/__/__/__/__/__/__/      ||\n");
    printf("        ||                                   ||\n");
    printf("~~~~~~~~||~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~||~~~~~~~~\n");
}

static void art_room_tower(void)
{
    printf("                       /\\\n");
    printf("                      /  \\\n");
    printf("                     /____\\\n");
    printf("                   __| [] |__\n");
    printf("                  /  |    |  \\\n");
    printf("                 /___|____|___\\\n");
    printf("                    / || \\\n");
    printf("                   /  ||  \\\n");
    printf("                  /___||___\\\n");
    printf("                 /____||____\\\n");
    printf("              %s\n", g_room_art_captions[WORLD_ROOM_TOWER]);
}

static void art_room_orchard(void)
{
    printf("          &&&        &&&        &&&\n");
    printf("       &&&&&&&    &&&&&&&    &&&&&&&\n");
    printf("      &&&&&&&&&  &&&&&&&&&  &&&&&&&&&\n");
    printf("         ||||        ||||        ||||\n");
    printf("         ||||        ||||        ||||\n");
    printf("      .-.||||.-.  .-.||||.-.  .-.||||.-.\n");
    printf("     (o o)  (o o)(o o)  (o o)(o o)  (o o)\n");
    printf("      '-'    '-'  '-'    '-'  '-'    '-'\n");
    printf("             %s\n", g_room_art_captions[WORLD_ROOM_ORCHARD]);
}

static void art_room_cave(void)
{
    printf("                 ______________________\n");
    printf("            _.-'                      '-._\n");
    printf("         .-'    .--.            .--.      '-.\n");
    printf("        /      /    \\          /    \\        \\\n");
    printf("       /______/______\\________/______\\________\\\n");
    printf("       |   .-.    .-.    .-.    .-.    .-.   |\n");
    printf("       |  (   )  (   )  (   )  (   )  (   )  |\n");
    printf("       |   '-'    '-'    '-'    '-'    '-'   |\n");
    printf("       |______________________________________|\n");
    printf("              %s\n", g_room_art_captions[WORLD_ROOM_CAVE]);
}

static void art_wanderer(void)
{
    printf("                        .-''''-.\n");
    printf("                      .'  _  _  '.\n");
    printf("                     /   (o)(o)   \\\n");
    printf("                    |      /\\      |\n");
    printf("                    |     /  \\     |\n");
    printf("                    |   .-====-.   |\n");
    printf("                    |  /  ____  \\  |\n");
    printf("                    | /  / __ \\  \\ |\n");
    printf("                    | | | /  \\ | | |\n");
    printf("                    | | | \\__/ | | |\n");
    printf("                    |  \\ \\____/ /  |\n");
    printf("                    |   '------'   |\n");
    printf("                    |   /|    |\\   |\n");
    printf("                    |  /_|____|_\\  |\n");
    printf("                    | /__|____|__\\ |\n");
    printf("                    |/___/    \\___\\|\n");
    printf("                   /____/      \\____\\\n");
    printf("              %s\n", TXT_WANDERER_ART_CAPTION);
}

static void art_frog_portrait(void)
{
    printf("                            _..---.._\n");
    printf("                        .-''  .-.  ''-.\n");
    printf("                      .'     (   )     '.\n");
    printf("                     /   .-.  '-'  .-.   \\\n");
    printf("                    /   /   \\     /   \\   \\\n");
    printf("                    |  | (o) |   | (o) |  |\n");
    printf("                    |  |     /___\\     |  |\n");
    printf("                    |  |   .-`___`-.   |  |\n");
    printf("                    |   \\  \\_____/  /  /   |\n");
    printf("                    |    '._\\___/_.-'   |\n");
    printf("                    |       /   \\       |\n");
    printf("                  __|__   _/_____|\\_   __|__\n");
    printf("               .-'____ '-/  /\\ /\\  \\-' ____'-.\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("                %s\n", TXT_FROG_ART_CAPTION);
}

static void art_for_room(int room_id)
{
    if (room_id == WORLD_ROOM_CAMP) {
        art_room_camp();
        return;
    }
    if (room_id == WORLD_ROOM_ROAD) {
        art_room_road();
        return;
    }
    if (room_id == WORLD_ROOM_POND) {
        art_room_pond();
        return;
    }
    if (room_id == WORLD_ROOM_FOREST) {
        art_room_forest();
        return;
    }
    if (room_id == WORLD_ROOM_STREAM) {
        art_room_stream();
        return;
    }
    if (room_id == WORLD_ROOM_RUINS) {
        art_room_ruins();
        return;
    }
    if (room_id == WORLD_ROOM_CLIFF) {
        art_room_cliff();
        return;
    }
    if (room_id == WORLD_ROOM_MARSH) {
        art_room_marsh();
        return;
    }
    if (room_id == WORLD_ROOM_GROVE) {
        art_room_grove();
        return;
    }
    if (room_id == WORLD_ROOM_BRIDGE) {
        art_room_bridge();
        return;
    }
    if (room_id == WORLD_ROOM_CATACOMBS) {
        art_room_catacombs();
        return;
    }
    if (room_id == WORLD_ROOM_MEADOW) {
        art_room_meadow();
        return;
    }
    if (room_id == WORLD_ROOM_CANYON) {
        art_room_canyon();
        return;
    }
    if (room_id == WORLD_ROOM_TOWER) {
        art_room_tower();
        return;
    }
    if (room_id == WORLD_ROOM_ORCHARD) {
        art_room_orchard();
        return;
    }
    if (room_id == WORLD_ROOM_CAVE) {
        art_room_cave();
        return;
    }
}

void game_print_location_art(int room_id)
{
    printf("\n");
    art_for_room(room_id);
}

void render_room_look(struct GameState *game, int npc_in_room_hint)
{
    struct Room *room;
    int dir;
    int rid;
    int s;
    int ground_count;
    int first_ground;

    room = &game->world.rooms[game->player.room_id];
    rid = game->player.room_id;
    game_print_location_art(game->player.room_id);
    printf("\n%s\n", room->name);
    printf("%s\n", room->desc);
    printf("%s", TXT_UI_EXITS_LABEL);
    for (dir = 0; dir < DIR_NONE; ++dir) {
        if (room->exits[dir] >= 0) {
            printf(" %s", world_dir_name(dir));
        }
    }
    printf("\n");
    ground_count = 0;
    first_ground = ITEM_NONE;
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        if (game->room_item[rid][s] != ITEM_NONE) {
            if (ground_count == 0) {
                first_ground = game->room_item[rid][s];
            }
            ground_count += 1;
        }
    }
    if (ground_count == 1) {
        printf(TXT_UI_GROUND_ITEM_FMT,
            item_name(first_ground),
            item_name(first_ground));
    } else if (ground_count > 1) {
        printf("%s", TXT_UI_GROUND_ITEMS_HEADER);
        for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
            if (game->room_item[rid][s] != ITEM_NONE) {
                printf(TXT_UI_GROUND_ITEM_LINE_FMT,
                    item_name(game->room_item[rid][s]),
                    item_name(game->room_item[rid][s]));
            }
        }
    }
    if (game->corpse_present[game->player.room_id]) {
        printf("%s", TXT_UI_BANDIT_CORPSE);
    }
    if (npc_in_room_hint != 0) {
        printf("%s", TXT_UI_NPC_HINT);
    }
    if (game->env_focus_active &&
            game->env_focus_room == game->player.room_id &&
            game->tick < game->env_focus_expires_tick) {
        if (game->env_focus_kind == GAME_ENV_RUSTLE) {
            printf("%s", TXT_UI_FOCUS_RUSTLE);
        } else if (game->env_focus_kind == GAME_ENV_CREAK) {
            printf("%s", TXT_UI_FOCUS_CREAK);
        } else if (game->env_focus_kind == GAME_ENV_WATER) {
            printf("%s", TXT_UI_FOCUS_WATER);
        } else if (game->env_focus_kind == GAME_ENV_GRIT) {
            printf("%s", TXT_UI_FOCUS_GRIT);
        }
    }
}

void game_render(const struct GameState *game)
{
    const struct Room *room;
    int needed;

    room = &game->world.rooms[game->player.room_id];
    needed = game_xp_to_next_level(game->level);
    printf(TXT_HUD_FMT,
        game->tick, room->name, game->player_hp, game->max_hp,
        game->level, game->xp, needed);
}

void game_print_help(int topic)
{
    printf("%s\n", command_help_line(topic));
}

void render_bandit_encounter_open(void)
{
    printf("\n");
    printf("                  .--.__________________.--.\n");
    printf("                .'   /  _   _   _   _  \\   '.\n");
    printf("               /    |  (o) (o) (o) (o) |    \\\n");
    printf("              |     |        /\\         |     |\n");
    printf("              |     |   .-==========-.  |     |\n");
    printf("              |     |  /  /  /  /  / |  |     |\n");
    printf("              |     | /__/__/__/__/  |  |     |\n");
    printf("              |     | \\  rusty blade  /  |     |\n");
    printf("               \\    |  '------------'   |    /\n");
    printf("                '.  \\__________________/  .'\n");
    printf("                  '--._______________ .--'\n");
    printf("%s", TXT_BANDIT_OPEN_INTRO);
    printf("%s", TXT_BANDIT_OPEN_QUOTE);
    printf("%s", TXT_BANDIT_OPEN_OPT1);
    printf("%s", TXT_BANDIT_OPEN_OPT2);
    printf("%s", TXT_BANDIT_OPEN_OPT3);
    printf("%s", TXT_REPLY_PROMPT);
}

void render_combat_start(int player_hp, int enemy_hp)
{
    printf(TXT_COMBAT_START_FMT, player_hp, enemy_hp);
    printf("%s", TXT_COMBAT_MENU);
}

void render_combat_enemy_strike(int dmg)
{
    printf(TXT_COMBAT_ENEMY_STRIKE_FMT, dmg);
}

void render_combat_player_fallen(void)
{
    printf("%s", TXT_COMBAT_PLAYER_FALLEN);
}

void render_combat_status_line(int player_hp, int enemy_hp)
{
    printf(TXT_COMBAT_STATUS_FMT, player_hp, enemy_hp);
}

void render_combat_player_hit(int dmg)
{
    printf(TXT_COMBAT_PLAYER_HIT_FMT, dmg);
}

void render_combat_braced(void)
{
    printf("%s", TXT_COMBAT_BRACED);
}

void render_combat_no_salve_bag(void)
{
    printf("%s", TXT_COMBAT_NO_SALVE);
}

void render_combat_salve_in_combat(int hp)
{
    printf(TXT_COMBAT_SALVE_FMT, hp);
}

void render_combat_invalid_choice(void)
{
    printf("%s", TXT_PICK_123);
}

void render_combat_bandit_defeated(void)
{
    printf("%s", TXT_COMBAT_BANDIT_DEFEATED);
}

void render_combat_menu(void)
{
    printf("%s", TXT_COMBAT_MENU);
}

void render_xp_gained(int amount)
{
    printf(TXT_XP_GAIN_FMT, amount);
}

void render_level_up(int level, int max_hp, int damage_bonus, int bag_capacity)
{
    printf(TXT_LEVEL_UP_FMT, level);
    printf(TXT_LEVEL_STATS_FMT,
        max_hp, damage_bonus, bag_capacity);
}

void render_nearby_item_notice(const char *item_name)
{
    printf(TXT_NEARBY_ITEM_FMT, item_name);
}

void render_animal_noise_line(const char *line)
{
    printf("\n%s\n", line);
}

void render_atmosphere_gust(void)
{
    printf("%s", TXT_ATMO_GUST);
}

void render_atmosphere_rustle(void)
{
    printf("%s", TXT_ATMO_RUSTLE);
}

void render_atmosphere_berry_drop(void)
{
    printf("%s", TXT_ATMO_BERRY_DROP);
}

void render_atmosphere_creak(void)
{
    printf("%s", TXT_ATMO_CREAK);
}

void render_atmosphere_water(void)
{
    printf("%s", TXT_ATMO_WATER);
}

void render_atmosphere_reed_drop(void)
{
    printf("%s", TXT_ATMO_REED_DROP);
}

void render_atmosphere_grit(void)
{
    printf("%s", TXT_ATMO_GRIT);
}

void render_wanderer_scene(void)
{
    printf("\n");
    art_wanderer();
    printf("%s", TXT_WANDERER_INTRO);
    printf("%s", TXT_WANDERER_QUOTE_A);
    printf("%s", TXT_WANDERER_QUOTE_B);
    printf("%s", TXT_WANDERER_OPT1);
    printf("%s", TXT_WANDERER_OPT2);
    printf("%s", TXT_WANDERER_OPT3);
    printf("%s", TXT_REPLY_PROMPT);
}

void render_wanderer_reply(int choice)
{
    printf("%s", txtres_wanderer_reply(choice));
}

void render_frog_dialogue_intro(void)
{
    printf("\n");
    art_frog_portrait();
    printf("%s", TXT_FROG_INTRO);
    printf("%s", TXT_FROG_QUOTE);
    printf("%s", TXT_FROG_OPT1);
    printf("%s", TXT_FROG_OPT2);
    printf("%s", TXT_FROG_OPT3);
    printf("%s", TXT_REPLY_PROMPT);
}

void render_frog_dialogue_branch(int choice)
{
    if (choice == 1) {
        printf("%s", TXT_FROG_REPLY_A1);
        printf("%s", TXT_FROG_REPLY_A2);
        printf("%s", TXT_FROG_REPLY_A3);
        return;
    }
    if (choice == 2) {
        printf("%s", TXT_FROG_REPLY_B1);
        printf("%s", TXT_FROG_REPLY_B2);
        printf("%s", TXT_FROG_REPLY_B3);
        return;
    }
    printf("%s", TXT_FROG_REPLY_C1);
    printf("%s", TXT_FROG_REPLY_C2);
    printf("%s", TXT_FROG_REPLY_C3);
}

void render_msg_bandit_waiting_reply(void)
{
    printf("%s", TXT_MSG_BANDIT_WAITING);
}

void render_msg_bandit_waiting_handover_pick(void)
{
    printf("%s", TXT_MSG_BANDIT_WAITING_HANDOVER);
}

void render_bandit_handover_pick_prompt(void)
{
    printf("%s", TXT_BANDIT_HANDOVER_PICK_PROMPT);
}

void render_msg_bandit_give_not_carrying(void)
{
    printf("%s", TXT_MSG_BANDIT_GIVE_NOT_CARRYING);
}

void render_msg_give_wrong_context(void)
{
    printf("%s", TXT_MSG_GIVE_WRONG_CONTEXT);
}

void render_msg_unknown_command(void)
{
    printf("%s", TXT_MSG_UNKNOWN_COMMAND);
}

void render_msg_wait(void)
{
    printf("%s", TXT_MSG_WAIT);
}

void render_msg_cannot_move(const char *dir_name)
{
    printf(TXT_MSG_CANNOT_MOVE_FMT, dir_name);
}

void render_msg_moved(const char *dir_name)
{
    printf(TXT_MSG_MOVED_FMT, dir_name);
}

void render_msg_inspect_nothing(void)
{
    printf("%s", TXT_MSG_INSPECT_NOTHING);
}

void render_msg_inspect_wrong_focus(void)
{
    printf("%s", TXT_MSG_INSPECT_WRONG_FOCUS);
}

void render_msg_inspect_rustle(void)
{
    printf("%s", TXT_MSG_INSPECT_RUSTLE);
}

void render_msg_inspect_creak(void)
{
    printf("%s", TXT_MSG_INSPECT_CREAK);
}

void render_msg_inspect_water(void)
{
    printf("%s", TXT_MSG_INSPECT_WATER);
}

void render_msg_inspect_grit(void)
{
    printf("%s", TXT_MSG_INSPECT_GRIT);
}

void render_msg_bandit_blocks_talk(void)
{
    printf("%s", TXT_MSG_BANDIT_BLOCK_TALK);
}

void render_msg_traveler_waiting(void)
{
    printf("%s", TXT_MSG_TRAVELER_WAITING);
}

void render_msg_watchman_talk(void)
{
    printf("%s", TXT_MSG_WATCHMAN_TALK_LINE1);
    printf("%s", TXT_MSG_WATCHMAN_TALK_LINE2);
    printf("%s", TXT_MSG_WATCHMAN_TALK_LINE3);
    printf("%s", TXT_MSG_WATCHMAN_TALK_LINE4);
    printf("%s", TXT_REPLY_PROMPT);
}

void render_msg_herbalist_talk(void)
{
    printf("%s", TXT_MSG_HERBALIST_TALK_LINE1);
    printf("%s", TXT_MSG_HERBALIST_TALK_LINE2);
    printf("%s", TXT_MSG_HERBALIST_TALK_LINE3);
    printf("%s", TXT_MSG_HERBALIST_TALK_LINE4);
    printf("%s", TXT_REPLY_PROMPT);
}

void render_msg_archivist_talk(void)
{
    printf("%s", TXT_MSG_ARCHIVIST_TALK_LINE1);
    printf("%s", TXT_MSG_ARCHIVIST_TALK_LINE2);
    printf("%s", TXT_MSG_ARCHIVIST_TALK_LINE3);
    printf("%s", TXT_MSG_ARCHIVIST_TALK_LINE4);
    printf("%s", TXT_REPLY_PROMPT);
}

void render_msg_nobody_talk(void)
{
    printf("%s", TXT_MSG_NOBODY_TALK);
}

void render_msg_watchman_reply(int arg)
{
    printf("%s", txtres_msg_watchman_reply(arg));
}

void render_msg_herbalist_reply(int arg)
{
    printf("%s", txtres_msg_herbalist_reply(arg));
}

void render_msg_archivist_reply(int arg)
{
    printf("%s", txtres_msg_archivist_reply(arg));
}

void render_msg_hand_over_item(const char *item_name)
{
    printf(TXT_MSG_HAND_OVER_ITEM_FMT, item_name);
}

void render_msg_bag_empty_bandit(void)
{
    printf("%s", TXT_MSG_BAG_EMPTY_BANDIT);
}

void render_msg_intimidate_success(void)
{
    printf("%s", TXT_MSG_INTIMIDATE_SUCCESS);
}

void render_msg_intimidate_fail(void)
{
    printf("%s", TXT_MSG_INTIMIDATE_FAIL);
}

void render_msg_pick_123(void)
{
    printf("%s", TXT_PICK_123);
}

void render_msg_nobody_waiting_reply(void)
{
    printf("%s", TXT_MSG_NOBODY_WAITING);
}
