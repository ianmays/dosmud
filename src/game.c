#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "game.h"

#define ENV_FOCUS_NONE 0
#define ENV_FOCUS_RUSTLE 1
#define ENV_FOCUS_CREAK 2
#define ENV_FOCUS_WATER 3
#define ENV_FOCUS_GRIT 4

#define ITEM_NONE 0
#define ITEM_BERRY 1
#define ITEM_STICK 2
#define ITEM_REED 3
#define ITEM_STONE 4
#define ITEM_HERB 5
#define ITEM_FISH 6
#define ITEM_TORCH 7
#define ITEM_SALVE 8
#define ITEM_SPEAR 9

static const char *item_name(item_id)
int item_id;
{
    if (item_id == ITEM_BERRY) return "berry";
    if (item_id == ITEM_STICK) return "stick";
    if (item_id == ITEM_REED) return "reed";
    if (item_id == ITEM_STONE) return "stone";
    if (item_id == ITEM_HERB) return "herb";
    if (item_id == ITEM_FISH) return "fish";
    if (item_id == ITEM_TORCH) return "torch";
    if (item_id == ITEM_SALVE) return "salve";
    if (item_id == ITEM_SPEAR) return "spear";
    return "unknown";
}

static int bag_find_index(game, item_id)
struct GameState *game;
int item_id;
{
    int i;
    for (i = 0; i < game->bag_count; ++i) {
        if (game->bag[i] == item_id) {
            return i;
        }
    }
    return -1;
}

static int bag_add(game, item_id)
struct GameState *game;
int item_id;
{
    if (game->bag_count >= CFG_BAG_MAX) {
        return 0;
    }
    game->bag[game->bag_count] = item_id;
    game->bag_count += 1;
    return 1;
}

static int bag_remove_index(game, index)
struct GameState *game;
int index;
{
    int i;
    if (index < 0 || index >= game->bag_count) {
        return ITEM_NONE;
    }
    for (i = index; i < game->bag_count - 1; ++i) {
        game->bag[i] = game->bag[i + 1];
    }
    game->bag_count -= 1;
    game->bag[game->bag_count] = ITEM_NONE;
    return 1;
}

static int bag_remove_item(game, item_id)
struct GameState *game;
int item_id;
{
    int idx;
    idx = bag_find_index(game, item_id);
    if (idx < 0) {
        return 0;
    }
    return bag_remove_index(game, idx);
}

static int item_is_edible(item_id)
int item_id;
{
    if (item_id == ITEM_BERRY) return 1;
    if (item_id == ITEM_FISH) return 1;
    return 0;
}

static void seed_world_items(game)
struct GameState *game;
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
}

static void maybe_spawn_room_item(game)
struct GameState *game;
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
    printf("A %s catches your eye nearby.\n", item_name(game->room_item[room_id]));
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

static void art_for_room(room_id)
int room_id;
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
}

void game_print_location_art(room_id)
int room_id;
{
    printf("\n");
    art_for_room(room_id);
}

static void wanderer_update_separation(game)
struct GameState *game;
{
    if (game->player.room_id != game->wanderer_room) {
        game->wanderer_need_separation = 0;
    }
}

static void wanderer_step(game)
struct GameState *game;
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

static void wanderer_begin_encounter(game)
struct GameState *game;
{
    if (game->wanderer_need_separation) {
        return;
    }
    game->pond_dialogue = 0;
    printf("\n");
    art_wanderer();
    printf("\nYou nearly bump into a hooded traveler. They straighten with a tired grin.\n");
    printf("\"Easy there—I'm an adventurer too, working odd jobs between towns. ");
    printf("What are you doing out here?\"\n");
    printf("  [1] Looking for trouble worth the trouble.\n");
    printf("  [2] Passing through—keeping my boots honest.\n");
    printf("  [3] That's my business.\n");
    printf("(Answer with 1, 2, 3, or reply <n>.)\n");
    game->wanderer_dialogue = 1;
    game->wanderer_need_separation = 1;
}

static void wanderer_apply_reply(choice)
int choice;
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

static void frog_dialogue_intro(void)
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

static void frog_dialogue_branch(choice)
int choice;
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

static void do_look(game)
struct GameState *game;
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
    if (game->env_focus_active &&
            game->env_focus_room == game->player.room_id &&
            game->tick < game->env_focus_expires_tick) {
        if (game->env_focus_kind == ENV_FOCUS_RUSTLE) {
            printf("Something is rustling nearby. (inspect rustle)\n");
        } else if (game->env_focus_kind == ENV_FOCUS_CREAK) {
            printf("You can track the source of the creaking. (inspect creak)\n");
        } else if (game->env_focus_kind == ENV_FOCUS_WATER) {
            printf("You can follow the moving water sound. (inspect water)\n");
        } else if (game->env_focus_kind == ENV_FOCUS_GRIT) {
            printf("Fresh grit skids nearby. (inspect grit)\n");
        }
    }
}

void game_describe_current_room(game)
struct GameState *game;
{
    do_look(game);
}

void game_init(game)
struct GameState *game;
{
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
    game->env_focus_kind = ENV_FOCUS_NONE;
    game->env_focus_expires_tick = 0;
    game->bag_count = 0;
    seed_world_items(game);
    srand((unsigned int)time(NULL));
}

void game_render(game)
const struct GameState *game;
{
    const struct Room *room;

    room = &game->world.rooms[game->player.room_id];
    printf("\n[T:%lu] %s\n", game->tick, room->name);
}

void game_print_help(void)
{
    printf("%s\n", command_help_text());
}

static int apply_command(game, cmd)
struct GameState *game;
struct Command *cmd;
{
    int room_id;
    int ground_item;
    int i;

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
        printf("You wait.\n");
        return 1;
    }
    if (cmd->type == CMD_MOVE) {
        if (!world_can_move(&game->world, game->player.room_id, cmd->dir)) {
            printf("You cannot move %s from here.\n", world_dir_name(cmd->dir));
            return 0;
        }
        game->player.room_id = world_move(&game->world, game->player.room_id, cmd->dir);
        game->pond_dialogue = 0;
        game->wanderer_dialogue = 0;
        printf("You move %s.\n", world_dir_name(cmd->dir));
        do_look(game);
        return 1;
    }
    if (cmd->type == CMD_TAKE) {
        room_id = game->player.room_id;
        ground_item = game->room_item[room_id];
        if (ground_item == ITEM_NONE) {
            printf("There is nothing here to take.\n");
            return 1;
        }
        if (cmd->arg != ground_item) {
            printf("You cannot take that from here.\n");
            return 1;
        }
        if (!bag_add(game, ground_item)) {
            printf("Your bag is full (5 items max).\n");
            return 1;
        }
        game->room_item[room_id] = ITEM_NONE;
        printf("You pick up the %s.\n", item_name(ground_item));
        return 1;
    }
    if (cmd->type == CMD_DROP) {
        room_id = game->player.room_id;
        if (bag_find_index(game, cmd->arg) < 0) {
            printf("You are not carrying a %s.\n", item_name(cmd->arg));
            return 1;
        }
        if (game->room_item[room_id] != ITEM_NONE) {
            printf("The ground here is already occupied by a %s.\n",
                item_name(game->room_item[room_id]));
            return 1;
        }
        bag_remove_item(game, cmd->arg);
        game->room_item[room_id] = cmd->arg;
        printf("You drop the %s.\n", item_name(cmd->arg));
        return 1;
    }
    if (cmd->type == CMD_BAG) {
        printf("Bag (%d/%d):", game->bag_count, CFG_BAG_MAX);
        if (game->bag_count <= 0) {
            printf(" empty\n");
            return 1;
        }
        for (i = 0; i < game->bag_count; ++i) {
            printf(" %s", item_name(game->bag[i]));
            if (i < game->bag_count - 1) {
                printf(",");
            }
        }
        printf("\n");
        return 1;
    }
    if (cmd->type == CMD_EAT) {
        if (bag_find_index(game, cmd->arg) < 0) {
            printf("You are not carrying a %s.\n", item_name(cmd->arg));
            return 1;
        }
        if (!item_is_edible(cmd->arg)) {
            printf("You cannot eat the %s.\n", item_name(cmd->arg));
            return 1;
        }
        bag_remove_item(game, cmd->arg);
        if (cmd->arg == ITEM_BERRY) {
            printf("You eat the berry. Tart, but fresh.\n");
        } else {
            printf("You eat the fish. Not ideal raw, but nourishing.\n");
        }
        return 1;
    }
    if (cmd->type == CMD_USE) {
        if (bag_find_index(game, cmd->arg) < 0) {
            printf("You are not carrying a %s.\n", item_name(cmd->arg));
            return 1;
        }
        if (cmd->arg == ITEM_TORCH) {
            printf("You raise the torch; nearby details sharpen in warm light.\n");
            return 1;
        }
        if (cmd->arg == ITEM_SALVE) {
            printf("You apply the salve. The sting fades.\n");
            bag_remove_item(game, cmd->arg);
            return 1;
        }
        if (cmd->arg == ITEM_SPEAR) {
            printf("You test the spear's weight. Balanced enough.\n");
            return 1;
        }
        printf("You cannot find a practical use for the %s right now.\n",
            item_name(cmd->arg));
        return 1;
    }
    if (cmd->type == CMD_CRAFT) {
        if (cmd->arg == ITEM_TORCH) {
            if (bag_find_index(game, ITEM_STICK) < 0 ||
                    bag_find_index(game, ITEM_REED) < 0) {
                printf("Craft torch needs: stick + reed.\n");
                return 1;
            }
            bag_remove_item(game, ITEM_STICK);
            bag_remove_item(game, ITEM_REED);
            bag_add(game, ITEM_TORCH);
            printf("You bind a makeshift torch.\n");
            return 1;
        }
        if (cmd->arg == ITEM_SALVE) {
            if (bag_find_index(game, ITEM_HERB) < 0 ||
                    bag_find_index(game, ITEM_BERRY) < 0) {
                printf("Craft salve needs: herb + berry.\n");
                return 1;
            }
            bag_remove_item(game, ITEM_HERB);
            bag_remove_item(game, ITEM_BERRY);
            bag_add(game, ITEM_SALVE);
            printf("You mash a basic healing salve.\n");
            return 1;
        }
        if (cmd->arg == ITEM_SPEAR) {
            if (bag_find_index(game, ITEM_STICK) < 0 ||
                    bag_find_index(game, ITEM_STONE) < 0) {
                printf("Craft spear needs: stick + stone.\n");
                return 1;
            }
            bag_remove_item(game, ITEM_STICK);
            bag_remove_item(game, ITEM_STONE);
            bag_add(game, ITEM_SPEAR);
            printf("You lash a stone point to the stick and craft a spear.\n");
            return 1;
        }
        printf("You do not know how to craft that.\n");
        return 1;
    }
    if (cmd->type == CMD_INSPECT) {
        if (!game->env_focus_active ||
                game->env_focus_room != game->player.room_id ||
                game->tick >= game->env_focus_expires_tick) {
            printf("Nothing here stands out right now.\n");
            game->env_focus_active = 0;
            game->env_focus_room = -1;
            game->env_focus_kind = ENV_FOCUS_NONE;
            game->env_focus_expires_tick = 0;
            return 1;
        }
        if (cmd->arg != 0 && cmd->arg != game->env_focus_kind) {
            printf("That is not what is drawing your attention.\n");
            return 1;
        }
        if (game->env_focus_kind == ENV_FOCUS_RUSTLE) {
            printf("You part the brush and startle a hare into a low sprint.\n");
        } else if (game->env_focus_kind == ENV_FOCUS_CREAK) {
            printf("An old branch rocks against another, groaning like timber.\n");
        } else if (game->env_focus_kind == ENV_FOCUS_WATER) {
            printf("You find a thin runnel cutting fresh lines through mud.\n");
        } else if (game->env_focus_kind == ENV_FOCUS_GRIT) {
            printf("New tracks cross the grit: light, quick, and already fading.\n");
        }
        game->env_focus_active = 0;
        game->env_focus_room = -1;
        game->env_focus_kind = ENV_FOCUS_NONE;
        game->env_focus_expires_tick = 0;
        return 1;
    }
    if (cmd->type == CMD_TALK) {
        if (game->wanderer_dialogue == 1) {
            printf("The traveler is waiting for an answer (1/2/3).\n");
            return 1;
        }
        if (game->player.room_id != WORLD_ROOM_POND) {
            printf("Nobody here wants to talk.\n");
            return 1;
        }
        frog_dialogue_intro();
        game->pond_dialogue = 1;
        return 1;
    }
    if (cmd->type == CMD_REPLY) {
        if (game->wanderer_dialogue == 1) {
            if (cmd->arg < 1 || cmd->arg > 3) {
                printf("Pick 1, 2, or 3.\n");
                return 1;
            }
            wanderer_apply_reply(cmd->arg);
            game->wanderer_dialogue = 0;
            return 1;
        }
        if (game->pond_dialogue != 1) {
            printf("Nobody is waiting for an answer.\n");
            return 1;
        }
        if (cmd->arg < 1 || cmd->arg > 3) {
            printf("Pick 1, 2, or 3.\n");
            return 1;
        }
        frog_dialogue_branch(cmd->arg);
        game->pond_dialogue = 0;
        return 1;
    }

    return 0;
}

static void maybe_emit_animal_noise(game)
struct GameState *game;
{
    if ((game->tick % 2UL) != 0UL) {
        return;
    }
    if ((rand() % 100) >= 75) {
        return;
    }
    printf("\n%s\n", world_room_animal_noise(&game->world, game->player.room_id));
}

static void maybe_emit_atmosphere(game)
struct GameState *game;
{
    int roll;

    if (game->env_focus_active && game->tick >= game->env_focus_expires_tick) {
        game->env_focus_active = 0;
        game->env_focus_room = -1;
        game->env_focus_kind = ENV_FOCUS_NONE;
        game->env_focus_expires_tick = 0;
    }

    roll = rand() % 100;
    if (roll < 35) {
        printf("\nA cool gust threads through the area and fades.\n");
        return;
    }
    if (roll < 55) {
        printf("\nSomething small rustles just out of sight.\n");
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = ENV_FOCUS_RUSTLE;
        game->env_focus_expires_tick = game->tick + 3;
        if (game->room_item[game->player.room_id] == ITEM_NONE && (rand() % 100) < 50) {
            game->room_item[game->player.room_id] = ITEM_BERRY;
            printf("A berry drops from the brush.\n");
        }
        return;
    }
    if (roll < 70) {
        printf("\nA distant creak rolls across the landscape.\n");
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = ENV_FOCUS_CREAK;
        game->env_focus_expires_tick = game->tick + 3;
        return;
    }
    if (roll < 82) {
        printf("\nYou hear water moving somewhere beyond the path.\n");
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = ENV_FOCUS_WATER;
        game->env_focus_expires_tick = game->tick + 3;
        if (game->room_item[game->player.room_id] == ITEM_NONE && (rand() % 100) < 50) {
            game->room_item[game->player.room_id] = ITEM_REED;
            printf("A loose reed drifts to your feet.\n");
        }
        return;
    }
    if (roll < 92) {
        printf("\nLoose grit skips over stone under an uncertain breeze.\n");
        game->env_focus_active = 1;
        game->env_focus_room = game->player.room_id;
        game->env_focus_kind = ENV_FOCUS_GRIT;
        game->env_focus_expires_tick = game->tick + 3;
        return;
    }
    maybe_spawn_room_item(game);
}

static void advance_world_tick(game, wanderer_moves_first)
struct GameState *game;
int wanderer_moves_first;
{
    int old_wanderer_room;

    game->tick += 1;
    wanderer_update_separation(game);
    old_wanderer_room = game->wanderer_room;

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

    world_step(&game->world, game->tick);
    maybe_emit_animal_noise(game);
    maybe_emit_atmosphere(game);
}

int game_process_input(game, line)
struct GameState *game;
char *line;
{
    struct Command cmd;
    int parsed;
    int applied;

    parsed = command_parse(line, &cmd);
    if (!parsed) {
        printf("Unknown command. Type 'help'.\n");
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

void game_background_step(game)
struct GameState *game;
{
    advance_world_tick(game, 1);
}
