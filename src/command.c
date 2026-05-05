#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "command.h"

static void lower_inplace(char *s)
{
    while (*s) {
        *s = (char)tolower((unsigned char)*s);
        ++s;
    }
}

static int parse_dir(char *word)
{
    if (strcmp(word, "north") == 0 || strcmp(word, "n") == 0) return DIR_NORTH;
    if (strcmp(word, "south") == 0 || strcmp(word, "s") == 0) return DIR_SOUTH;
    if (strcmp(word, "east") == 0 || strcmp(word, "e") == 0) return DIR_EAST;
    if (strcmp(word, "west") == 0 || strcmp(word, "w") == 0) return DIR_WEST;
    return DIR_NONE;
}

static int parse_item_word(char *word)
{
    if (strcmp(word, "berry") == 0 || strcmp(word, "berries") == 0) return 1;
    if (strcmp(word, "stick") == 0) return 2;
    if (strcmp(word, "reed") == 0 || strcmp(word, "reeds") == 0) return 3;
    if (strcmp(word, "stone") == 0) return 4;
    if (strcmp(word, "herb") == 0 || strcmp(word, "herbs") == 0) return 5;
    if (strcmp(word, "fish") == 0) return 6;
    if (strcmp(word, "torch") == 0) return 7;
    if (strcmp(word, "salve") == 0) return 8;
    if (strcmp(word, "spear") == 0) return 9;
    return 0;
}

int command_parse(char *line, struct Command *out_cmd)
{
    char word1[CFG_WORD_MAX];
    char word2[CFG_WORD_MAX];
    int count;
    int dir;

    out_cmd->type = CMD_INVALID;
    out_cmd->dir = DIR_NONE;
    out_cmd->arg = 0;

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
    if (strcmp(word1, "talk") == 0 || strcmp(word1, "speak") == 0) {
        out_cmd->type = CMD_TALK;
        return 1;
    }
    if (strcmp(word1, "inspect") == 0 || strcmp(word1, "examine") == 0 ||
            strcmp(word1, "investigate") == 0) {
        out_cmd->type = CMD_INSPECT;
        if (count < 2) {
            out_cmd->arg = 0;
            return 1;
        }
        if (strcmp(word2, "rustle") == 0 || strcmp(word2, "reeds") == 0) {
            out_cmd->arg = 1;
            return 1;
        }
        if (strcmp(word2, "creak") == 0 || strcmp(word2, "timbers") == 0) {
            out_cmd->arg = 2;
            return 1;
        }
        if (strcmp(word2, "water") == 0 || strcmp(word2, "current") == 0) {
            out_cmd->arg = 3;
            return 1;
        }
        if (strcmp(word2, "grit") == 0 || strcmp(word2, "tracks") == 0) {
            out_cmd->arg = 4;
            return 1;
        }
        return 0;
    }
    if (strcmp(word1, "take") == 0 || strcmp(word1, "get") == 0 ||
            strcmp(word1, "pickup") == 0) {
        out_cmd->type = CMD_TAKE;
        if (count < 2) return 0;
        out_cmd->arg = parse_item_word(word2);
        return out_cmd->arg != 0;
    }
    if (strcmp(word1, "drop") == 0) {
        out_cmd->type = CMD_DROP;
        if (count < 2) return 0;
        out_cmd->arg = parse_item_word(word2);
        return out_cmd->arg != 0;
    }
    if (strcmp(word1, "bag") == 0 || strcmp(word1, "inventory") == 0 ||
            strcmp(word1, "inv") == 0) {
        out_cmd->type = CMD_BAG;
        return 1;
    }
    if (strcmp(word1, "eat") == 0) {
        out_cmd->type = CMD_EAT;
        if (count < 2) return 0;
        out_cmd->arg = parse_item_word(word2);
        return out_cmd->arg != 0;
    }
    if (strcmp(word1, "use") == 0) {
        out_cmd->type = CMD_USE;
        if (count < 2) return 0;
        out_cmd->arg = parse_item_word(word2);
        return out_cmd->arg != 0;
    }
    if (strcmp(word1, "craft") == 0 || strcmp(word1, "build") == 0) {
        out_cmd->type = CMD_CRAFT;
        if (count < 2) return 0;
        out_cmd->arg = parse_item_word(word2);
        return out_cmd->arg != 0;
    }
    if (strcmp(word1, "loot") == 0) {
        out_cmd->type = CMD_LOOT;
        return 1;
    }
    if (strcmp(word1, "reply") == 0) {
        if (count < 2) {
            return 0;
        }
        if (strcmp(word2, "1") == 0) {
            out_cmd->type = CMD_REPLY;
            out_cmd->arg = 1;
            return 1;
        }
        if (strcmp(word2, "2") == 0) {
            out_cmd->type = CMD_REPLY;
            out_cmd->arg = 2;
            return 1;
        }
        if (strcmp(word2, "3") == 0) {
            out_cmd->type = CMD_REPLY;
            out_cmd->arg = 3;
            return 1;
        }
        return 0;
    }
    if (strlen(word1) == 1 && word1[0] >= '1' && word1[0] <= '3') {
        out_cmd->type = CMD_REPLY;
        out_cmd->arg = (int)(word1[0] - '0');
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

int command_advances_time(int type)
{
    if (type == CMD_WAIT) return 1;
    if (type == CMD_MOVE) return 1;
    if (type == CMD_TAKE) return 1;
    if (type == CMD_DROP) return 1;
    if (type == CMD_EAT) return 1;
    if (type == CMD_USE) return 1;
    if (type == CMD_CRAFT) return 1;
    if (type == CMD_LOOT) return 1;
    return 0;
}

const char *command_help_text(void)
{
    return "Commands: look, inspect [rustle|creak|water|grit], take/drop <item>, bag, eat/use <item>, craft <item>, loot, move <dir>, wait, talk, 1/2/3 or reply <1-3>, help, quit";
}
