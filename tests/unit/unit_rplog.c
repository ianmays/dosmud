#include "greatest.h"
#include <stdio.h>
#include <string.h>
#include "command.h"
#include "game.h"
#include "gout.h"
#include "replay.h"

/*
 * Unit tests for replay.c serialization. Exercises replay_log_* directly with
 * synthetic GameEventQueue fixtures rather than the main loop or grendr.
 */

static void read_file_text(FILE *fp, char *buf, size_t size)
{
    size_t n;

    rewind(fp);
    n = fread(buf, 1, size - 1, fp);
    buf[n] = '\0';
}

TEST replay_log_reset_clears_state(void)
{
    ReplayLog log;

    log.fp = (FILE *)1;
    log.path = "replay.log";
    log.next_step = 7;
    replay_log_reset(&log);
    ASSERT(0 == log.fp);
    ASSERT(0 == log.path);
    ASSERT_EQ(0, log.next_step);
    PASS();
}

TEST replay_log_capture_serializes_step_metadata_and_events(void)
{
    struct GameState game;
    GameEventQueue out;
    ReplayLog log;
    GameEvent *ev;
    FILE *fp;
    char text[2048];

    fp = tmpfile();
    ASSERT(fp != 0);

    replay_log_reset(&log);
    log.fp = fp;
    log.path = "tmpfile";
    game_init(&game, 1234U);

    game_event_queue_reset(&out);
    ev = game_event_push(&out, GAME_EVENT_ROOM_LOOK, 0, 0, 0, 0, 0);
    ASSERT(ev != 0);
    ev->room_id = WORLD_ROOM_CAMP;
    ev->room_item[0] = 7;
    ev->room_item[1] = 0;
    ev->room_item[2] = 3;
    ev->room_item[3] = 0;
    ASSERT(0 != game_event_push(&out, GAME_EVENT_MOVE, 0, 0, 0, 0,
        "north \"east\"\\path"));
    out.overflowed = 1;

    ASSERT(replay_log_capture(&log, REPLAY_STEP_INPUT, "move north",
        &game, &out));
    read_file_text(fp, text, sizeof(text));

    ASSERT(0 != strstr(text,
        "step=0 kind=input tick=0 running=1 mode=explore events=2 overflow=1 input=\"move north\""));
    ASSERT(0 != strstr(text,
        "event=0 kind=GAME_EVENT_ROOM_LOOK arg0=0 arg1=0 arg2=0 arg3=0 room=0 room_items=[7,0,3,0] text=null"));
    ASSERT(0 != strstr(text,
        "event=1 kind=GAME_EVENT_MOVE arg0=0 arg1=0 arg2=0 arg3=0 room=-1 room_items=[0,0,0,0] text=\"north \\\"east\\\"\\\\path\""));
    ASSERT_EQ(1, log.next_step);

    fclose(fp);
    PASS();
}

TEST replay_log_open_writes_header_and_capture_increments_steps(void)
{
    struct GameState game;
    GameEventQueue out;
    ReplayLog log;
    FILE *fp;
    char text[2048];
    const char *path = "/tmp/dosmud_replay_unit.log";

    replay_log_reset(&log);
    ASSERT(replay_log_open(&log, path, 99U));
    ASSERT(replay_log_is_enabled(&log));

    game_init(&game, 99U);
    game.tick = 2;
    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_WAIT, 0, 0, 0, 0, 0));
    ASSERT(replay_log_capture(&log, REPLAY_STEP_IDLE, 0, &game, &out));

    game.tick = 3;
    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_HELP, CMD_HELP_TOPIC_MOVE, 0, 0, 0, 0));
    ASSERT(replay_log_capture(&log, REPLAY_STEP_INPUT, "help move", &game, &out));
    replay_log_close(&log);

    fp = fopen(path, "r");
    ASSERT(fp != 0);
    read_file_text(fp, text, sizeof(text));
    fclose(fp);
    remove(path);

    ASSERT(0 != strstr(text, "dosmud-replay-v1 seed=99"));
    ASSERT(0 != strstr(text,
        "step=0 kind=idle tick=2 running=1 mode=explore events=1 overflow=0 input=null"));
    ASSERT(0 != strstr(text,
        "step=1 kind=input tick=3 running=1 mode=explore events=1 overflow=0 input=\"help move\""));
    ASSERT(0 != strstr(text,
        "event=0 kind=GAME_EVENT_HELP arg0=2 arg1=0 arg2=0 arg3=0 room=-1 room_items=[0,0,0,0] text=null"));
    PASS();
}

TEST replay_log_guards_and_disabled_capture(void)
{
    ReplayLog log;
    struct GameState game;
    GameEventQueue out;
    FILE *fp;

    replay_log_reset(0);
    replay_log_close(0);
    ASSERT_EQ(0, replay_log_is_enabled(0));

    replay_log_reset(&log);
    ASSERT_EQ(0, replay_log_is_enabled(&log));
    ASSERT_EQ(0, replay_log_open(0, "/tmp/x", 1U));
    ASSERT_EQ(0, replay_log_open(&log, 0, 1U));
    ASSERT_EQ(0, replay_log_open(&log, "", 1U));

    game_init(&game, 1U);
    game_event_queue_reset(&out);
    ASSERT(replay_log_capture(&log, REPLAY_STEP_INPUT, 0, &game, &out));

    fp = tmpfile();
    ASSERT(fp != 0);
    replay_log_reset(&log);
    log.fp = fp;
    log.path = "tmpfile";
    ASSERT_EQ(0, replay_log_capture(&log, REPLAY_STEP_INPUT, 0, 0, &out));
    ASSERT_EQ(0, replay_log_capture(&log, REPLAY_STEP_INPUT, 0, &game, 0));
    fclose(fp);
    PASS();
}

TEST replay_log_capture_step_modes_kinds_and_escapes(void)
{
    struct GameState game;
    GameEventQueue out;
    ReplayLog log;
    GameEvent *ev;
    FILE *fp;
    char text[8192];
    static const int kinds[] = {
        GAME_EVENT_NONE,
        GAME_EVENT_MOVE,
        GAME_EVENT_ROOM_LOOK,
        GAME_EVENT_MAP,
        GAME_EVENT_HELP,
        GAME_EVENT_VERSION,
        GAME_EVENT_WAIT,
        GAME_EVENT_CANNOT_MOVE,
        GAME_EVENT_UNKNOWN_COMMAND,
        GAME_EVENT_ITEM_RESULT,
        GAME_EVENT_CORPSE_VIEW,
        GAME_EVENT_BAG_VIEW,
        GAME_EVENT_CRAFT_RESULT,
        GAME_EVENT_EQUIP_RESULT,
        GAME_EVENT_COMBAT,
        GAME_EVENT_XP_GAIN,
        GAME_EVENT_STAT_CHANGE,
        GAME_EVENT_DIALOGUE,
        GAME_EVENT_ENCOUNTER,
        GAME_EVENT_DIALOGUE_GUARD,
        GAME_EVENT_ENVIRONMENT,
        GAME_EVENT_AMBIENT_NOISE,
        GAME_EVENT_ITEM_PRESENCE,
        GAME_EVENT_OBSERVATION
    };
    static const char escaped[] = {'a', '\n', 'b', '\r', 'c', '\t', 'd', '\x01', 'e', '\0'};
    int i;

    fp = tmpfile();
    ASSERT(fp != 0);

    replay_log_reset(&log);
    log.fp = fp;
    log.path = "tmpfile";
    game_init(&game, 42U);

    game_event_queue_reset(&out);
    for (i = 0; i < (int)(sizeof(kinds) / sizeof(kinds[0])); ++i) {
        ev = game_event_push(&out, kinds[i], -1, -2, -3, -4, 0);
        ASSERT(ev != 0);
    }
    ev = game_event_push(&out, GAME_EVENT_WAIT, 0, 0, 0, 0, escaped);
    ASSERT(ev != 0);
    ev = game_event_push(&out, GAME_EVENT_WAIT, 0, 0, 0, 0, 0);
    ASSERT(ev != 0);
    ev->kind = 999;

    ASSERT(replay_log_capture(&log, REPLAY_STEP_STARTUP, 0, &game, &out));

    game.mode = GAME_MODE_DIALOGUE;
    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_DIALOGUE, 0, 0, 0, 0, 0));
    ASSERT(replay_log_capture(&log, REPLAY_STEP_INPUT, "talk", &game, &out));

    game.mode = GAME_MODE_COMBAT;
    game_event_queue_reset(&out);
    ASSERT(0 != game_event_push(&out, GAME_EVENT_COMBAT, 0, 0, 0, 0, 0));
    ASSERT(replay_log_capture(&log, 99, "fight", &game, &out));

    read_file_text(fp, text, sizeof(text));

    ASSERT(0 != strstr(text, "step=0 kind=startup"));
    ASSERT(0 != strstr(text, "step=1 kind=input tick=0 running=1 mode=dialogue"));
    ASSERT(0 != strstr(text, "step=2 kind=unknown tick=0 running=1 mode=combat"));
    ASSERT(0 != strstr(text, "kind=GAME_EVENT_MAP"));
    ASSERT(0 != strstr(text, "kind=GAME_EVENT_VERSION"));
    ASSERT(0 != strstr(text, "kind=GAME_EVENT_COMBAT"));
    ASSERT(0 != strstr(text, "kind=GAME_EVENT_OBSERVATION"));
    ASSERT(0 != strstr(text, "kind=GAME_EVENT_CORPSE_VIEW"));
    ASSERT(0 != strstr(text, "kind=GAME_EVENT_UNKNOWN"));
    ASSERT(0 != strstr(text, "a\\nb\\rc\\t"));
    ASSERT(0 != strstr(text, "\\x01"));
    ASSERT_EQ(3, log.next_step);

    fclose(fp);
    PASS();
}

SUITE(rplog)
{
    RUN_TEST(replay_log_reset_clears_state);
    RUN_TEST(replay_log_capture_serializes_step_metadata_and_events);
    RUN_TEST(replay_log_open_writes_header_and_capture_increments_steps);
    RUN_TEST(replay_log_guards_and_disabled_capture);
    RUN_TEST(replay_log_capture_step_modes_kinds_and_escapes);
}
