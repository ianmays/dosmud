#ifndef COMMAND_H
#define COMMAND_H

#include "config.h"
#include "world.h"

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
    CMD_LOOT
};

struct Command {
    int type;
    int dir;
    int arg;
};

int command_parse(char *line, struct Command *out_cmd);
int command_advances_time(int type);
const char *command_help_text(void);

#endif
