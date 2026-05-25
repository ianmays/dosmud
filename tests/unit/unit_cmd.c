#include <string.h>
#include "greatest.h"
/* strcmp used in help_line inequality check */
#include "command.h"
#include "items.h"

static int parse_line(const char *src, struct Command *cmd)
{
    char buf[CFG_INPUT_MAX];
    strncpy(buf, src, CFG_INPUT_MAX - 1);
    buf[CFG_INPUT_MAX - 1] = '\0';
    return command_parse(buf, cmd);
}

TEST command_parse_core_verbs(void)
{
    struct Command cmd;

    ASSERT_EQ(1, parse_line("look", &cmd));
    ASSERT_EQ(CMD_LOOK, cmd.type);
    ASSERT_EQ(1, parse_line("l", &cmd));
    ASSERT_EQ(CMD_LOOK, cmd.type);
    ASSERT_EQ(1, parse_line("map", &cmd));
    ASSERT_EQ(CMD_MAP, cmd.type);
    ASSERT_EQ(1, parse_line("wait", &cmd));
    ASSERT_EQ(CMD_WAIT, cmd.type);
    ASSERT_EQ(1, parse_line(".", &cmd));
    ASSERT_EQ(CMD_WAIT, cmd.type);
    ASSERT_EQ(1, parse_line("quit", &cmd));
    ASSERT_EQ(CMD_QUIT, cmd.type);
    ASSERT_EQ(1, parse_line("talk", &cmd));
    ASSERT_EQ(CMD_TALK, cmd.type);
    ASSERT_EQ(1, parse_line("loot", &cmd));
    ASSERT_EQ(CMD_LOOT, cmd.type);
    ASSERT_EQ(1, parse_line("unwield", &cmd));
    ASSERT_EQ(CMD_UNWIELD, cmd.type);
    PASS();
}

TEST command_parse_move_and_help(void)
{
    struct Command cmd;

    ASSERT_EQ(1, parse_line("move north", &cmd));
    ASSERT_EQ(CMD_MOVE, cmd.type);
    ASSERT_EQ(DIR_NORTH, cmd.dir);
    ASSERT_EQ(1, parse_line("n", &cmd));
    ASSERT_EQ(CMD_MOVE, cmd.type);
    ASSERT_EQ(DIR_NORTH, cmd.dir);
    ASSERT_EQ(0, parse_line("move", &cmd));
    ASSERT_EQ(1, parse_line("help", &cmd));
    ASSERT_EQ(CMD_HELP, cmd.type);
    ASSERT_EQ(CMD_HELP_TOPIC_GENERAL, cmd.arg);
    ASSERT_EQ(1, parse_line("help take", &cmd));
    ASSERT_EQ(CMD_HELP_TOPIC_TAKE, cmd.arg);
    ASSERT_EQ(1, parse_line("help xyzzy", &cmd));
    ASSERT_EQ(CMD_HELP_TOPIC_UNKNOWN, cmd.arg);
    PASS();
}

TEST command_parse_items_and_reply(void)
{
    struct Command cmd;

    ASSERT_EQ(1, parse_line("take stick", &cmd));
    ASSERT_EQ(CMD_TAKE, cmd.type);
    ASSERT_EQ(ITEM_STICK, cmd.arg);
    ASSERT_EQ(1, parse_line("take all", &cmd));
    ASSERT_EQ(CMD_TAKE, cmd.type);
    ASSERT_EQ(CMD_TAKE_ALL, cmd.arg);
    ASSERT_EQ(1, parse_line("get all", &cmd));
    ASSERT_EQ(CMD_TAKE, cmd.type);
    ASSERT_EQ(CMD_TAKE_ALL, cmd.arg);
    ASSERT_EQ(1, parse_line("pickup all", &cmd));
    ASSERT_EQ(CMD_TAKE, cmd.type);
    ASSERT_EQ(CMD_TAKE_ALL, cmd.arg);
    ASSERT_EQ(0, parse_line("take", &cmd));
    ASSERT_EQ(0, parse_line("take diamond", &cmd));
    ASSERT_EQ(1, parse_line("reply 2", &cmd));
    ASSERT_EQ(CMD_REPLY, cmd.type);
    ASSERT_EQ(2, cmd.arg);
    ASSERT_EQ(1, parse_line("2", &cmd));
    ASSERT_EQ(CMD_REPLY, cmd.type);
    ASSERT_EQ(2, cmd.arg);
    ASSERT_EQ(0, parse_line("reply 4", &cmd));
    PASS();
}

TEST command_parse_inspect(void)
{
    struct Command cmd;

    ASSERT_EQ(1, parse_line("inspect rustle", &cmd));
    ASSERT_EQ(CMD_INSPECT, cmd.type);
    ASSERT_EQ(1, cmd.arg);
    ASSERT_EQ(1, parse_line("inspect reeds", &cmd));
    ASSERT_EQ(1, cmd.arg);
    ASSERT_EQ(1, parse_line("inspect creak", &cmd));
    ASSERT_EQ(2, cmd.arg);
    ASSERT_EQ(1, parse_line("inspect water", &cmd));
    ASSERT_EQ(3, cmd.arg);
    ASSERT_EQ(1, parse_line("inspect grit", &cmd));
    ASSERT_EQ(4, cmd.arg);
    ASSERT_EQ(0, parse_line("inspect moon", &cmd));
    ASSERT_EQ(1, parse_line("inspect", &cmd));
    ASSERT_EQ(0, cmd.arg);
    PASS();
}

TEST command_parse_invalid(void)
{
    struct Command cmd;
    ASSERT_EQ(0, parse_line("", &cmd));
    ASSERT_EQ(0, parse_line("xyzzy", &cmd));
    PASS();
}

TEST command_advances_time_matrix(void)
{
    ASSERT_EQ(1, command_advances_time(CMD_WAIT));
    ASSERT_EQ(1, command_advances_time(CMD_MOVE));
    ASSERT_EQ(1, command_advances_time(CMD_TAKE));
    ASSERT_EQ(0, command_advances_time(CMD_LOOK));
    ASSERT_EQ(0, command_advances_time(CMD_HELP));
    ASSERT_EQ(0, command_advances_time(CMD_REPLY));
    PASS();
}

TEST command_help_line_topics(void)
{
    ASSERT_STR_EQ(command_help_text(), command_help_line(CMD_HELP_TOPIC_GENERAL));
    ASSERT(strcmp(command_help_line(CMD_HELP_TOPIC_LOOK),
            command_help_line(CMD_HELP_TOPIC_UNKNOWN)) != 0);
    ASSERT(strstr(command_help_line(CMD_HELP_TOPIC_TAKE), "take all") != 0);
    PASS();
}

SUITE(command) {
    RUN_TEST(command_parse_core_verbs);
    RUN_TEST(command_parse_move_and_help);
    RUN_TEST(command_parse_items_and_reply);
    RUN_TEST(command_parse_inspect);
    RUN_TEST(command_parse_invalid);
    RUN_TEST(command_advances_time_matrix);
    RUN_TEST(command_help_line_topics);
}
