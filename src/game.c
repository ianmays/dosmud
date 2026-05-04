#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "game.h"

static void art_room_camp(void)
{
    printf("                    .       *        .        *\n");
    printf("         *    .         .        .        .         *\n");
    printf("                           _..---.._\n");
    printf("                        .-'  _   _  '-.\n");
    printf("                       /    (o)_(o)    \\\n");
    printf("                      ;       ___       ;\n");
    printf("                      |     .'___'.     |\n");
    printf("                      |    / /   \\ \\    |\n");
    printf("            /\\        ;    | |   | |    ;        /\\\n");
    printf("           /  \\       /\\   |_|___|_|   /\\       /  \\\n");
    printf("          /____\\     /  '-.  ____  .-'  \\     /____\\\n");
    printf("         /|    |\\___/_____/\\/____\\/\\_____\\___/|    |\\\n");
    printf("        /_|____|_\\_____________________________/_|____|_\\\n");
    printf("          /_/\\_\\     ~   ~   ~  ~  ~   ~      /_/\\_\\\n");
    printf("         /_/  \\_\\  ~  ~  ~   ~  ~   ~  ~  ~  /_/  \\_\\\n");
    printf("                    (fire pops cheerfully)\n");
}

static void art_room_road(void)
{
    printf("                    .-^^-.                   .-^^-.\n");
    printf("                 .-'_    '-.             .-'    _'-.\n");
    printf("                /  ( )      \\           /      ( )  \\\n");
    printf("               /______________\\         /______________\\\n");
    printf("                         \\                    /\n");
    printf("                          \\                  /\n");
    printf("                           \\                /\n");
    printf("                            \\              /\n");
    printf("                             \\            /\n");
    printf("                              \\          /\n");
    printf("_______________________________\\________/_______________________________\n");
    printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
    printf("  .   .  .   .   .   .   .   .   .   .   .   .   .   .\n");
    printf("   .   .   .   .   .   .   .   .   .   .   .   .   .\n");
    printf("    .   .   .   .   .   .   .   .   .   .   .   .\n");
    printf("     .   .   .   .   .   .   .   .   .   .   .\n");
    printf("      .   .   .   .   .   .   .   .   .   .\n");
    printf("       '-----------------------------------'\n");
}

static void art_room_pond(void)
{
    printf("                    _.-~~~~~~~~~~-._\n");
    printf("                .-~'  ~  ~  ~  ~   '~-.\n");
    printf("             .-'  ~   ~  ~  ~   ~  ~  '-.\n");
    printf("           .' ~  ~   ~   ~   ~   ~   ~   '.\n");
    printf("          /  ~   ~  ~  ~  ~  ~  ~  ~  ~    \\\n");
    printf("         /____________________________________\\\n");
    printf("         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("      ~~~  ~~~   ~~~   ~~~   ~~~   ~~~   ~~~  ~~~\n");
    printf("    ~~     ~      ~     ~     ~     ~      ~    ~~\n");
    printf("      ~      .-''''-.             .-''''-.     ~\n");
    printf("         ~  /  .--.  \\  .---.    /  .--.  \\  ~\n");
    printf("       ~   |  (o  o)  || o o |  |  (o  o)  |   ~\n");
    printf("      ~     \\   --   /  | ^ |    \\   --   /    ~\n");
    printf("        ~    '-.__.-'   '---'     '-.__.-'   ~\n");
    printf("          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("             (the frog's lily pad throne)\n");
}

static void art_room_forest(void)
{
    printf("                        /\\  /\\  /\\\n");
    printf("                       /  \\/  \\/  \\\n");
    printf("                      /    \\  /    \\\n");
    printf("                     /______\\/______\\\n");
    printf("                    /~ ~ ~ ~ ~ ~ ~ ~ ~\\\n");
    printf("                   /~  ~  ~  ~  ~  ~  ~\\\n");
    printf("                  /~   ~   ~   ~   ~   ~\\\n");
    printf("         /\\      /~  ~  ~  ~  ~  ~  ~  ~\\\n");
    printf("        /  \\    /________________________\\\n");
    printf("       /    \\  /| | | | | | | | | | | | | \\\n");
    printf("      /______\\/ | | | | | | | | | | | | | | \\\n");
    printf("     /|  ||  |\\  | | | | | | | | | | | | |  \\\n");
    printf("    / |  ||  | \\___________________________\\\n");
    printf("   /  |__||__|  \\  (shadows hold their breath)  \\\n");
    printf("  /________________\\__________________________\\\n");
}

static void art_room_stream(void)
{
    printf("            .     .     .     .     .     .\n");
    printf("         .     .     .     .     .     .\n");
    printf("      ~~~~~~~~   ~~~~   ~~~~   ~~~~   ~~~~~~~~\n");
    printf("   ~~~~   ~~~~~   ~~~~~   ~~~~~   ~~~~~   ~~~~\n");
    printf("  ~~~~~  ~~~~~  ~~~~~  ~~~~~  ~~~~~  ~~~~~  ~~~~~\n");
    printf(" ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~\n");
    printf("  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~\n");
    printf("   ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~\n");
    printf("     ~~~~  ~~~~  ~~~~  ~~~~  ~~~~  ~~~~\n");
    printf("   @==@==@==@==@==@==@==@==@==@==@==@==@==@==@==@\n");
    printf("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf(" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("   (the current is patient; the stones are not)\n");
}

static void art_room_ruins(void)
{
    printf("                    |     |     |\n");
    printf("                    |     |     |\n");
    printf("                 ___|_____|_____|___\n");
    printf("                /   |     |     |   \\\n");
    printf("               /    |  |  |  |  |    \\\n");
    printf("              /_____|__|__|__|__|_____\\\n");
    printf("             /|  |  |  |  |  |  |  |  |\\\n");
    printf("            / |  |  |  |  |  |  |  |  | \\\n");
    printf("           /  |__|__|__|__|__|__|__|__|  | \\\n");
    printf("          /   |  |  |  |  |  |  |  |  |   | \\\n");
    printf("         /____|__|__|__|__|__|__|__|__|____| \\\n");
    printf("        / |    |    |    |    |    |    |    | \\\n");
    printf("       /__|____|____|____|____|____|____|____|__\\\n");
    printf("      /___________________________________________\\\n");
    printf("                    (time chipped the capitals)\n");
}

static void art_room_cliff(void)
{
    printf("                       /^\\\n");
    printf("                      /   \\\n");
    printf("                     /  ^  \\\n");
    printf("                    /  / \\  \\\n");
    printf("                   /__/___\\__\\\n");
    printf("                   |  _   _  |\n");
    printf("                   | | | | | |\n");
    printf("                   | |_| |_| |\n");
    printf("                   |  _   _  |\n");
    printf("~~~~~~~~~~~~~~~~~~~|_| |_| |_|~~~~~~~~~~~~~~~~~~~\n");
}

static void art_room_marsh(void)
{
    printf("      ~~~   ~~~   ~~~   ~~~   ~~~\n");
    printf("   ~~~   ~~~   ~~~   ~~~   ~~~   ~~~\n");
    printf("      ||    ||    ||    ||    ||\n");
    printf("      ||    ||    ||    ||    ||\n");
    printf("    __||____||____||____||____||__\n");
    printf("   /~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\\\n");
    printf("   \\________________________________/\n");
}

static void art_room_grove(void)
{
    printf("          &&& &&  & &&\n");
    printf("      && &\\/&\\|& ()|/ @, &&\n");
    printf("      &\\/(/&/&||/& /_/)_&/_&\n");
    printf("   &() &\\/&|()|/&\\/ '%%\" & ()\n");
    printf("  &_\\_&&_\\ |& |&&/&__%%_/_& &&\n");
    printf("&&   && & &| &| /& & %% ()& /&&\n");
    printf(" ()&_---()&\\&\\|&&-&&--%%---()~\n");
}

static void art_room_bridge(void)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("      ||             ||\n");
    printf("======||=============||======\n");
    printf("      ||             ||\n");
    printf("      ||             ||\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

static void art_room_catacombs(void)
{
    printf("   ______________________________________\n");
    printf("  /_____________________________________/|\n");
    printf("  |  []   []   []   []   []   []   []  | |\n");
    printf("  |                                      | |\n");
    printf("  |  []   []   []   []   []   []   []  | |\n");
    printf("  |______________________________________|/\n");
}

static void art_room_meadow(void)
{
    printf("       \\ | /            \\ | /\n");
    printf("     '.  *  .'        '.  *  .'\n");
    printf("  --  *  *  *  --  --  *  *  *  --\n");
    printf("     .'  *  '.        .'  *  .'\n");
    printf("       / | \\            / | \\\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

static void art_wanderer(void)
{
    printf("                        __\n");
    printf("                    _.-'  '-._\n");
    printf("                 .-'  |##|  '-.\n");
    printf("                / (|  /  \\  |) \\\n");
    printf("               |   |  o  o  |   |\n");
    printf("               |   |   >>   |   |\n");
    printf("               |   |  /  \\  |   |\n");
    printf("               |    `------'    |\n");
    printf("               |   /|      |\\   |\n");
    printf("               |  / |      | \\  |\n");
    printf("               '._/ |______| \\_.'\n");
    printf("                  /___||___\\\n");
    printf("                 /____||____\\\n");
    printf("                /_____/  \\_____\\\n");
    printf("               /________________\\\n");
    printf("              ( a fellow tourist of misfortune )\n");
}

static void art_frog_portrait(void)
{
    printf("                                _.--._\n");
    printf("                             .-'  ||  '-.\n");
    printf("                            /   .-''-.   \\\n");
    printf("                           /   /_    _\\   \\\n");
    printf("                           |  |  o  o  |  |\n");
    printf("                           |  |   __   |  |\n");
    printf("                           |  |  (__)  |  |\n");
    printf("                           |   \\  --  /   |\n");
    printf("                           |    '----'    |\n");
    printf("                          /|   .-====-.   |\\\n");
    printf("                         /_|__/  /\\  \\ \\__|_\\\n");
    printf("                      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("                   ~~~~~   ~~~~~   ~~~~~   ~~~~~   ~~~~\n");
    printf("                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("                     (His Majesty, CEO of Pond Ops)\n");
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
    (void)game;

    roll = rand() % 100;
    if (roll < 35) {
        printf("\nA cool gust threads through the area and fades.\n");
        return;
    }
    if (roll < 55) {
        printf("\nSomething small rustles just out of sight.\n");
        return;
    }
    if (roll < 70) {
        printf("\nA distant creak rolls across the landscape.\n");
        return;
    }
    if (roll < 82) {
        printf("\nYou hear water moving somewhere beyond the path.\n");
        return;
    }
    if (roll < 92) {
        printf("\nLoose grit skips over stone under an uncertain breeze.\n");
        return;
    }
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
