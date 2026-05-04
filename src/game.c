#include <stdio.h>
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
}

void game_print_location_art(room_id)
int room_id;
{
    printf("\n");
    art_for_room(room_id);
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
        printf("You move %s.\n", world_dir_name(cmd->dir));
        do_look(game);
        return 1;
    }
    if (cmd->type == CMD_TALK) {
        if (game->player.room_id != WORLD_ROOM_POND) {
            printf("Nobody here wants to talk.\n");
            return 1;
        }
        frog_dialogue_intro();
        game->pond_dialogue = 1;
        return 1;
    }
    if (cmd->type == CMD_REPLY) {
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
        game->tick += 1;
        world_step(&game->world, game->tick);
    }

    return 1;
}
