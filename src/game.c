#include <stdio.h>
#include "game.h"

static void do_look(game)
struct GameState *game;
{
    struct Room *room;
    int dir;

    room = &game->world.rooms[game->player.room_id];
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

void game_init(game)
struct GameState *game;
{
    world_init(&game->world);
    game->player.room_id = 0;
    game->tick = 0;
    game->seed = 1;
    game->running = 1;
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
        printf("You move %s.\n", world_dir_name(cmd->dir));
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
