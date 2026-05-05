#include <stdio.h>
#include "grendr.h"
#include "game.h"
#include "items.h"
#include "command.h"
#include "world.h"

static int hud_xp_to_next(int level)
{
    return 20 + ((level - 1) * 15);
}

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
    printf("                    (campfire at first watch)\n");
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
    printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
    printf("  .     .      .     .      .     .      .     .\n");
    printf("    .      .      .      .      .      .      .\n");
    printf("      .      .      .      .      .      .\n");
    printf("                  (wagon ruts hold rain)\n");
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
    printf("                (reeds whisper across water)\n");
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
    printf("              (needles dampen every footfall)\n");
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
    printf("              (stones split the current)\n");
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
    printf("                (columns eroded by years)\n");
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
    printf("~~~~~~~~~~~~~~~~~~ wind-scoured ledge ~~~~~~~~~~~~~~~~~~\n");
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
    printf("           (mud pulls at your boots)\n");
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
    printf("                 (an old growth ring)\n");
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
    printf("                 (air tastes of chalk)\n");
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
    printf("            (seed heads bend in the wind)\n");
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
    printf("              (watchfire long gone cold)\n");
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
    printf("             (fallen fruit scents the air)\n");
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
    printf("              (drips mark patient time)\n");
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
    printf("              (cloak wet with road mist)\n");
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
    printf("                (His Majesty, Pond Operations)\n");
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

    room = &game->world.rooms[game->player.room_id];
    game_print_location_art(game->player.room_id);
    printf("\n%s\n", room->name);
    printf("%s\n", room->desc);
    printf("Exits:");
    for (dir = 0; dir < DIR_NONE; ++dir) {
        if (room->exits[dir] >= 0) {
            printf(" %s", world_dir_name(dir));
        }
    }
    printf("\n");
    if (game->room_item[game->player.room_id] != ITEM_NONE) {
        printf("On the ground: %s. (take %s)\n",
            item_name(game->room_item[game->player.room_id]),
            item_name(game->room_item[game->player.room_id]));
    }
    if (game->corpse_present[game->player.room_id]) {
        printf("A bandit corpse lies here. (loot)\n");
    }
    if (npc_in_room_hint != 0) {
        printf("Someone nearby might talk. (talk)\n");
    }
    if (game->env_focus_active &&
            game->env_focus_room == game->player.room_id &&
            game->tick < game->env_focus_expires_tick) {
        if (game->env_focus_kind == GAME_ENV_RUSTLE) {
            printf("Something is rustling nearby. (inspect rustle)\n");
        } else if (game->env_focus_kind == GAME_ENV_CREAK) {
            printf("You can track the source of the creaking. (inspect creak)\n");
        } else if (game->env_focus_kind == GAME_ENV_WATER) {
            printf("You can follow the moving water sound. (inspect water)\n");
        } else if (game->env_focus_kind == GAME_ENV_GRIT) {
            printf("Fresh grit skids nearby. (inspect grit)\n");
        }
    }
}

void game_render(const struct GameState *game)
{
    const struct Room *room;
    int needed;

    room = &game->world.rooms[game->player.room_id];
    needed = hud_xp_to_next(game->level);
    printf("\n[T:%lu] %s [HP:%d/%d] [Lv:%d XP:%d/%d]\n",
        game->tick, room->name, game->player_hp, game->max_hp,
        game->level, game->xp, needed);
}

void game_print_help(void)
{
    printf("%s\n", command_help_text());
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
    printf("\nA road bandit steps from cover with a hand on a rusted blade.\n");
    printf("\"Easy now. We can do this three ways.\"\n");
    printf("  [1] Refuse and fight.\n");
    printf("  [2] Hand over one item from your bag.\n");
    printf("  [3] Talk it down and part ways.\n");
    printf("(Answer with 1, 2, 3, or reply <n>.)\n");
}

void render_combat_start(int player_hp, int enemy_hp)
{
    printf("Combat starts. You HP: %d, Bandit HP: %d.\n", player_hp, enemy_hp);
    printf("Choose: [1] Attack  [2] Defend  [3] Use salve\n");
}

void render_combat_enemy_strike(int dmg)
{
    printf("The bandit strikes for %d damage.\n", dmg);
}

void render_combat_player_fallen(void)
{
    printf("You collapse. The road takes everything.\n");
}

void render_combat_status_line(int player_hp, int enemy_hp)
{
    printf("You HP: %d, Bandit HP: %d.\n", player_hp, enemy_hp);
}

void render_combat_player_hit(int dmg)
{
    printf("You hit the bandit for %d damage.\n", dmg);
}

void render_combat_braced(void)
{
    printf("You brace for the incoming strike.\n");
}

void render_combat_no_salve_bag(void)
{
    printf("You fumble for a salve, but you have none.\n");
}

void render_combat_salve_in_combat(int hp)
{
    printf("You apply salve and recover. HP now %d.\n", hp);
}

void render_combat_invalid_choice(void)
{
    printf("Pick 1, 2, or 3.\n");
}

void render_combat_bandit_defeated(void)
{
    printf("The bandit falls. The body slumps into the dust.\n");
}

void render_combat_menu(void)
{
    printf("Choose: [1] Attack  [2] Defend  [3] Use salve\n");
}

void render_xp_gained(int amount)
{
    printf("You gain %d XP.\n", amount);
}

void render_level_up(int level, int max_hp, int damage_bonus, int bag_capacity)
{
    printf("Level up! You are now level %d.\n", level);
    printf("Max HP %d, Damage bonus +%d, Bag capacity %d.\n",
        max_hp, damage_bonus, bag_capacity);
}

void render_nearby_item_notice(const char *item_name)
{
    printf("A %s catches your eye nearby.\n", item_name);
}

void render_animal_noise_line(const char *line)
{
    printf("\n%s\n", line);
}

void render_atmosphere_gust(void)
{
    printf("\nA cool gust threads through the area and fades.\n");
}

void render_atmosphere_rustle(void)
{
    printf("\nSomething small rustles just out of sight.\n");
}

void render_atmosphere_berry_drop(void)
{
    printf("A berry drops from the brush.\n");
}

void render_atmosphere_creak(void)
{
    printf("\nA distant creak rolls across the landscape.\n");
}

void render_atmosphere_water(void)
{
    printf("\nYou hear water moving somewhere beyond the path.\n");
}

void render_atmosphere_reed_drop(void)
{
    printf("A loose reed drifts to your feet.\n");
}

void render_atmosphere_grit(void)
{
    printf("\nLoose grit skips over stone under an uncertain breeze.\n");
}

void render_wanderer_scene(void)
{
    printf("\n");
    art_wanderer();
    printf("\nYou nearly bump into a hooded traveler. They straighten with a tired grin.\n");
    printf("\"Easy there—I'm an adventurer too, working odd jobs between towns. ");
    printf("What are you doing out here?\"\n");
    printf("  [1] Looking for trouble worth the trouble.\n");
    printf("  [2] Passing through—keeping my boots honest.\n");
    printf("  [3] That's my business.\n");
    printf("(Answer with 1, 2, 3, or reply <n>.)\n");
}

void render_wanderer_reply(int choice)
{
    if (choice == 1) {
        printf("\nThey nod, amused. \"Bold. Don't trip over your own story.\"\n");
        return;
    }
    if (choice == 2) {
        printf("\nThey relax a fraction. \"Good. Miles keep liars honest.\"\n");
        return;
    }
    printf("\nThey raise both hands. \"Fair. The road spies on everyone anyway.\"\n");
}

void render_frog_dialogue_intro(void)
{
    printf("\n");
    art_frog_portrait();
    printf("\nA damp frog wearing an imaginary crown clears his throat.\n");
    printf("\"Official pond hours are whenever I say they are. Pick a vibe:\"\n");
    printf("  [1] Bow and wish him a nice pond.\n");
    printf("  [2] Insult his lily pad.\n");
    printf("  [3] Ask if he is a wizard, a snack, or both.\n");
    printf("(Answer with 1, 2, 3, or reply <n>.)\n");
}

void render_frog_dialogue_branch(int choice)
{
    if (choice == 1) {
        printf("\nYou bow. The frog salutes with a webbed hand.\n");
        printf("\"Finally—someone whose parents finished the tutorial. ");
        printf("Wisdom of the pond: the water is wet, the mud is judgy, ");
        printf("and I am technically management. You're welcome. Ribbit.\"\n");
        return;
    }
    if (choice == 2) {
        printf("\nYou call his lily pad 'discount turf.' ");
        printf("The frog clutches his chest like you stabbed Shakespeare.\n");
        printf("\"Rude! Delicious! That's how you get warts—not magic, ");
        printf("just bad networking. Also you're banned from handsomeness.\"\n");
        return;
    }
    printf("\nYou lean in and whisper that the moon is 'basically a lid.'\n");
    printf("The frog nods with the gravity of a tiny judge.\n");
    printf("\"The moon knows what it did. I'm not allowed to say which phase. ");
    printf("If anyone asks, you hallucinated this conversation. For tax reasons.\"\n");
}

void render_msg_bandit_waiting_reply(void)
{
    printf("The bandit is waiting on your move (reply 1/2/3).\n");
}

void render_msg_unknown_command(void)
{
    printf("Unknown command. Type 'help'.\n");
}

void render_msg_wait(void)
{
    printf("You wait.\n");
}

void render_msg_cannot_move(const char *dir_name)
{
    printf("You cannot move %s from here.\n", dir_name);
}

void render_msg_moved(const char *dir_name)
{
    printf("You move %s.\n", dir_name);
}

void render_msg_inspect_nothing(void)
{
    printf("Nothing here stands out right now.\n");
}

void render_msg_inspect_wrong_focus(void)
{
    printf("That is not what is drawing your attention.\n");
}

void render_msg_inspect_rustle(void)
{
    printf("You part the brush and startle a hare into a low sprint.\n");
}

void render_msg_inspect_creak(void)
{
    printf("An old branch rocks against another, groaning like timber.\n");
}

void render_msg_inspect_water(void)
{
    printf("You find a thin runnel cutting fresh lines through mud.\n");
}

void render_msg_inspect_grit(void)
{
    printf("New tracks cross the grit: light, quick, and already fading.\n");
}

void render_msg_bandit_blocks_talk(void)
{
    printf("The bandit has your full attention right now.\n");
}

void render_msg_traveler_waiting(void)
{
    printf("The traveler is waiting for an answer (1/2/3).\n");
}

void render_msg_watchman_talk(void)
{
    printf("A one-eyed watchman leans on the parapet.\n");
    printf("\"Storms come from the canyon. You carry a torch?\"\n");
    printf("  [1] Ask for warning signs.\n");
    printf("  [2] Offer to share a meal.\n");
    printf("  [3] Say nothing and move on.\n");
    printf("(Answer with 1, 2, 3, or reply <n>.)\n");
}

void render_msg_herbalist_talk(void)
{
    printf("An herbalist kneels among fallen fruit.\n");
    printf("\"Need a field remedy or just company?\"\n");
    printf("  [1] Ask for medicine advice.\n");
    printf("  [2] Trade gossip from the road.\n");
    printf("  [3] Leave politely.\n");
    printf("(Answer with 1, 2, 3, or reply <n>.)\n");
}

void render_msg_archivist_talk(void)
{
    printf("A dust-caked archivist lights a stub candle.\n");
    printf("\"Speak quickly. Stone remembers everything.\"\n");
    printf("  [1] Ask about the ruins.\n");
    printf("  [2] Ask about safer routes.\n");
    printf("  [3] Thank them and leave.\n");
    printf("(Answer with 1, 2, 3, or reply <n>.)\n");
}

void render_msg_nobody_talk(void)
{
    printf("Nobody here wants to talk.\n");
}

void render_msg_watchman_reply(int arg)
{
    if (arg == 1) {
        printf("He points west. \"If crows go quiet, squall in ten minutes.\"\n");
    } else if (arg == 2) {
        printf("He accepts, then hands you dried herbs. \"Stay upright.\"\n");
    } else {
        printf("He nods once and returns to the horizon.\n");
    }
}

void render_msg_herbalist_reply(int arg)
{
    if (arg == 1) {
        printf("She mutters ratios: \"Two berries, one herb, crush fine.\"\n");
    } else if (arg == 2) {
        printf("She laughs. \"Road stories always cost extra.\"\n");
    } else {
        printf("She waves without looking up.\n");
    }
}

void render_msg_archivist_reply(int arg)
{
    if (arg == 1) {
        printf("Archivist: \"The top stones cracked first. The foundations were already wrong.\"\n");
    } else if (arg == 2) {
        printf("Archivist: \"Follow running water; dead tunnels lie to travelers.\"\n");
    } else {
        printf("Archivist: \"Go, then. Before the candle quits.\"\n");
    }
}

void render_msg_hand_over_item(const char *item_name)
{
    printf("You hand over your %s. The bandit backs off and leaves.\n", item_name);
}

void render_msg_bag_empty_bandit(void)
{
    printf("Your bag is empty. The bandit laughs and attacks.\n");
}

void render_msg_intimidate_success(void)
{
    printf("You keep your voice steady. The bandit grunts and withdraws.\n");
}

void render_msg_intimidate_fail(void)
{
    printf("Your pitch fails. The bandit lunges.\n");
}

void render_msg_pick_123(void)
{
    printf("Pick 1, 2, or 3.\n");
}

void render_msg_nobody_waiting_reply(void)
{
    printf("Nobody is waiting for an answer.\n");
}
