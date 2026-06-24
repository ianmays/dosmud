#ifndef COMMAND_H
#define COMMAND_H

#include "config.h"
#include "world.h"

/*
 * Command parsing keeps raw input separate from execution. CMD_* values and
 * help-topic constants are shared with game routing and help text.
 */

/* Passed in Command.arg when type is CMD_HELP. */
#define CMD_HELP_TOPIC_GENERAL 0
#define CMD_HELP_TOPIC_UNKNOWN (-1)
#define CMD_HELP_TOPIC_LOOK 1
#define CMD_HELP_TOPIC_MOVE 2
#define CMD_HELP_TOPIC_WAIT 3
#define CMD_HELP_TOPIC_INSPECT 4
#define CMD_HELP_TOPIC_TAKE 5
#define CMD_HELP_TOPIC_DROP 6
#define CMD_HELP_TOPIC_BAG 7
#define CMD_HELP_TOPIC_EAT 8
#define CMD_HELP_TOPIC_USE 9
#define CMD_HELP_TOPIC_CRAFT 10
#define CMD_HELP_TOPIC_LOOT 11
#define CMD_HELP_TOPIC_TALK 12
#define CMD_HELP_TOPIC_REPLY 13
#define CMD_HELP_TOPIC_QUIT 14
#define CMD_HELP_TOPIC_HELP 15
#define CMD_HELP_TOPIC_GIVE 16
#define CMD_HELP_TOPIC_MAP 17
#define CMD_HELP_TOPIC_WIELD 18
#define CMD_HELP_TOPIC_SAVE 19
#define CMD_HELP_TOPIC_LOAD 20
#define CMD_HELP_TOPIC_VERSION 21

/* Passed in Command.arg when type is CMD_TAKE and the user requested "all". */
#define CMD_TAKE_ALL (-1)
/* Passed in Command.arg when type is CMD_LOOT and the user requested "all". */
#define CMD_LOOT_ALL (-1)

enum CommandType {
    CMD_INVALID = 0,
    CMD_LOOK,
    CMD_MOVE,
    CMD_WAIT,
    CMD_HELP,
    CMD_QUIT,
    CMD_TALK,
    CMD_REPLY,
    CMD_INSPECT,
    CMD_TAKE,
    CMD_DROP,
    CMD_BAG,
    CMD_EAT,
    CMD_USE,
    CMD_CRAFT,
    CMD_LOOT,
    CMD_GIVE,
    CMD_MAP,
    CMD_WIELD,
    CMD_UNWIELD,
    CMD_SAVE,
    CMD_LOAD,
    CMD_VERSION
};

struct Command {
    int type;
    int dir;
    int arg;
};

int command_parse(char *line, struct Command *out_cmd);
int command_advances_time(int type);
const char *command_help_text(void);
const char *command_help_line(int topic);

#endif
