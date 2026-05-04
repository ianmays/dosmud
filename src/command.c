#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "command.h"

static void lower_inplace(s)
char *s;
{
    while (*s) {
        *s = (char)tolower((unsigned char)*s);
        ++s;
    }
}

static int parse_dir(word)
char *word;
{
    if (strcmp(word, "north") == 0 || strcmp(word, "n") == 0) return DIR_NORTH;
    if (strcmp(word, "south") == 0 || strcmp(word, "s") == 0) return DIR_SOUTH;
    if (strcmp(word, "east") == 0 || strcmp(word, "e") == 0) return DIR_EAST;
    if (strcmp(word, "west") == 0 || strcmp(word, "w") == 0) return DIR_WEST;
    return DIR_NONE;
}

int command_parse(line, out_cmd)
char *line;
struct Command *out_cmd;
{
    char word1[CFG_WORD_MAX];
    char word2[CFG_WORD_MAX];
    int count;
    int dir;

    out_cmd->type = CMD_INVALID;
    out_cmd->dir = DIR_NONE;

    count = sscanf(line, "%15s %15s", word1, word2);
    if (count <= 0) {
        return 0;
    }

    lower_inplace(word1);
    if (count > 1) {
        lower_inplace(word2);
    }

    if (strcmp(word1, "look") == 0 || strcmp(word1, "l") == 0) {
        out_cmd->type = CMD_LOOK;
        return 1;
    }
    if (strcmp(word1, "wait") == 0 || strcmp(word1, ".") == 0) {
        out_cmd->type = CMD_WAIT;
        return 1;
    }
    if (strcmp(word1, "help") == 0 || strcmp(word1, "?") == 0) {
        out_cmd->type = CMD_HELP;
        return 1;
    }
    if (strcmp(word1, "quit") == 0 || strcmp(word1, "exit") == 0) {
        out_cmd->type = CMD_QUIT;
        return 1;
    }

    if (strcmp(word1, "move") == 0 || strcmp(word1, "go") == 0) {
        if (count < 2) {
            return 0;
        }
        dir = parse_dir(word2);
        if (dir == DIR_NONE) {
            return 0;
        }
        out_cmd->type = CMD_MOVE;
        out_cmd->dir = dir;
        return 1;
    }

    /* Direction shorthand as direct move command. */
    dir = parse_dir(word1);
    if (dir != DIR_NONE) {
        out_cmd->type = CMD_MOVE;
        out_cmd->dir = dir;
        return 1;
    }

    return 0;
}

int command_advances_time(type)
int type;
{
    if (type == CMD_WAIT) return 1;
    if (type == CMD_MOVE) return 1;
    return 0;
}

const char *command_help_text(void)
{
    return "Commands: look, move <north|south|east|west>, wait, help, quit";
}
