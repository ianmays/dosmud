#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "base.h"
#include "command.h"
#include "items.h"
#include "txtres.h"

static void lower_inplace(char *s)
{
    while (*s) {
        *s = (char)tolower((u8)*s);
        ++s;
    }
}

static int help_topic_from_word(const char *w)
{
    if (strcmp(w, "look") == 0 || strcmp(w, "l") == 0) {
        return CMD_HELP_TOPIC_LOOK;
    }
    if (strcmp(w, "move") == 0 || strcmp(w, "go") == 0 ||
            strcmp(w, "north") == 0 || strcmp(w, "n") == 0 ||
            strcmp(w, "south") == 0 || strcmp(w, "s") == 0 ||
            strcmp(w, "east") == 0 || strcmp(w, "e") == 0 ||
            strcmp(w, "west") == 0 || strcmp(w, "w") == 0) {
        return CMD_HELP_TOPIC_MOVE;
    }
    if (strcmp(w, "wait") == 0 || strcmp(w, ".") == 0) {
        return CMD_HELP_TOPIC_WAIT;
    }
    if (strcmp(w, "inspect") == 0 || strcmp(w, "examine") == 0 ||
            strcmp(w, "investigate") == 0) {
        return CMD_HELP_TOPIC_INSPECT;
    }
    if (strcmp(w, "take") == 0 || strcmp(w, "get") == 0 ||
            strcmp(w, "pickup") == 0) {
        return CMD_HELP_TOPIC_TAKE;
    }
    if (strcmp(w, "drop") == 0) {
        return CMD_HELP_TOPIC_DROP;
    }
    if (strcmp(w, "bag") == 0 || strcmp(w, "inventory") == 0 ||
            strcmp(w, "inv") == 0) {
        return CMD_HELP_TOPIC_BAG;
    }
    if (strcmp(w, "eat") == 0) {
        return CMD_HELP_TOPIC_EAT;
    }
    if (strcmp(w, "use") == 0) {
        return CMD_HELP_TOPIC_USE;
    }
    if (strcmp(w, "craft") == 0 || strcmp(w, "build") == 0) {
        return CMD_HELP_TOPIC_CRAFT;
    }
    if (strcmp(w, "loot") == 0) {
        return CMD_HELP_TOPIC_LOOT;
    }
    if (strcmp(w, "give") == 0) {
        return CMD_HELP_TOPIC_GIVE;
    }
    if (strcmp(w, "talk") == 0 || strcmp(w, "speak") == 0) {
        return CMD_HELP_TOPIC_TALK;
    }
    if (strcmp(w, "reply") == 0 || strcmp(w, "choices") == 0) {
        return CMD_HELP_TOPIC_REPLY;
    }
    if (strcmp(w, "quit") == 0 || strcmp(w, "exit") == 0) {
        return CMD_HELP_TOPIC_QUIT;
    }
    if (strcmp(w, "map") == 0 || strcmp(w, "m") == 0) {
        return CMD_HELP_TOPIC_MAP;
    }
    if (strcmp(w, "wield") == 0 || strcmp(w, "unwield") == 0) {
        return CMD_HELP_TOPIC_WIELD;
    }
    if (strcmp(w, "help") == 0 || strcmp(w, "?") == 0) {
        return CMD_HELP_TOPIC_HELP;
    }
    return CMD_HELP_TOPIC_UNKNOWN;
}

static int parse_dir(char *word)
{
    if (strcmp(word, "north") == 0 || strcmp(word, "n") == 0) return DIR_NORTH;
    if (strcmp(word, "south") == 0 || strcmp(word, "s") == 0) return DIR_SOUTH;
    if (strcmp(word, "east") == 0 || strcmp(word, "e") == 0) return DIR_EAST;
    if (strcmp(word, "west") == 0 || strcmp(word, "w") == 0) return DIR_WEST;
    return DIR_NONE;
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
    if (strcmp(word1, "map") == 0 || strcmp(word1, "m") == 0) {
        out_cmd->type = CMD_MAP;
        return 1;
    }
    if (strcmp(word1, "wait") == 0 || strcmp(word1, ".") == 0) {
        out_cmd->type = CMD_WAIT;
        return 1;
    }
    if (strcmp(word1, "help") == 0 || strcmp(word1, "?") == 0) {
        out_cmd->type = CMD_HELP;
        if (count < 2) {
            out_cmd->arg = CMD_HELP_TOPIC_GENERAL;
        } else {
            out_cmd->arg = help_topic_from_word(word2);
        }
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
        if (strcmp(word2, "all") == 0) {
            out_cmd->arg = ITEM_ALL;
            return 1;
        }
        out_cmd->arg = item_from_word(word2);
        return out_cmd->arg != ITEM_NONE;
    }
    if (strcmp(word1, "drop") == 0) {
        out_cmd->type = CMD_DROP;
        if (count < 2) return 0;
        out_cmd->arg = item_from_word(word2);
        return out_cmd->arg != ITEM_NONE;
    }
    if (strcmp(word1, "give") == 0) {
        out_cmd->type = CMD_GIVE;
        if (count < 2) return 0;
        out_cmd->arg = item_from_word(word2);
        return out_cmd->arg != ITEM_NONE;
    }
    if (strcmp(word1, "bag") == 0 || strcmp(word1, "inventory") == 0 ||
            strcmp(word1, "inv") == 0) {
        out_cmd->type = CMD_BAG;
        return 1;
    }
    if (strcmp(word1, "eat") == 0) {
        out_cmd->type = CMD_EAT;
        if (count < 2) return 0;
        out_cmd->arg = item_from_word(word2);
        return out_cmd->arg != ITEM_NONE;
    }
    if (strcmp(word1, "use") == 0) {
        out_cmd->type = CMD_USE;
        if (count < 2) return 0;
        out_cmd->arg = item_from_word(word2);
        return out_cmd->arg != ITEM_NONE;
    }
    if (strcmp(word1, "craft") == 0 || strcmp(word1, "build") == 0) {
        out_cmd->type = CMD_CRAFT;
        if (count < 2) return 0;
        out_cmd->arg = item_from_word(word2);
        return out_cmd->arg != ITEM_NONE;
    }
    if (strcmp(word1, "loot") == 0) {
        out_cmd->type = CMD_LOOT;
        return 1;
    }
    if (strcmp(word1, "unwield") == 0) {
        out_cmd->type = CMD_UNWIELD;
        return 1;
    }
    if (strcmp(word1, "wield") == 0) {
        out_cmd->type = CMD_WIELD;
        if (count < 2) return 0;
        out_cmd->arg = item_from_word(word2);
        return out_cmd->arg != ITEM_NONE;
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
    return TXT_COMMAND_HELP;
}

const char *command_help_line(int topic)
{
    switch (topic) {
    case CMD_HELP_TOPIC_GENERAL:
        return TXT_COMMAND_HELP;
    case CMD_HELP_TOPIC_UNKNOWN:
        return TXT_HELP_TOPIC_UNKNOWN;
    case CMD_HELP_TOPIC_LOOK:
        return TXT_HELP_LOOK;
    case CMD_HELP_TOPIC_MOVE:
        return TXT_HELP_MOVE;
    case CMD_HELP_TOPIC_WAIT:
        return TXT_HELP_WAIT;
    case CMD_HELP_TOPIC_INSPECT:
        return TXT_HELP_INSPECT;
    case CMD_HELP_TOPIC_TAKE:
        return TXT_HELP_TAKE;
    case CMD_HELP_TOPIC_DROP:
        return TXT_HELP_DROP;
    case CMD_HELP_TOPIC_BAG:
        return TXT_HELP_BAG;
    case CMD_HELP_TOPIC_EAT:
        return TXT_HELP_EAT;
    case CMD_HELP_TOPIC_USE:
        return TXT_HELP_USE;
    case CMD_HELP_TOPIC_CRAFT:
        return TXT_HELP_CRAFT;
    case CMD_HELP_TOPIC_LOOT:
        return TXT_HELP_LOOT;
    case CMD_HELP_TOPIC_TALK:
        return TXT_HELP_TALK;
    case CMD_HELP_TOPIC_REPLY:
        return TXT_HELP_REPLY;
    case CMD_HELP_TOPIC_GIVE:
        return TXT_HELP_GIVE;
    case CMD_HELP_TOPIC_QUIT:
        return TXT_HELP_QUIT;
    case CMD_HELP_TOPIC_HELP:
        return TXT_HELP_HELP;
    case CMD_HELP_TOPIC_MAP:
        return TXT_HELP_MAP;
    case CMD_HELP_TOPIC_WIELD:
        return TXT_HELP_WIELD;
    default:
        return TXT_HELP_TOPIC_UNKNOWN;
    }
}
