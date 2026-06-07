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

SUITE(rplog)
{
    RUN_TEST(replay_log_reset_clears_state);
    RUN_TEST(replay_log_capture_serializes_step_metadata_and_events);
    RUN_TEST(replay_log_open_writes_header_and_capture_increments_steps);
}
