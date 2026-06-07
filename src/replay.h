#ifndef REPLAY_H
#define REPLAY_H

#include <stdio.h>
#include "base.h"

/*
 * Optional shell-level replay log. Serializes per-step GameEventQueue snapshots
 * without mutating gameplay or render state. main.c opens the file and calls
 * replay_log_capture after each step and before the next queue reset.
 * replay_log_capture returns 1 when logging is disabled or on success, 0 on I/O
 * failure.
 */

struct GameState;
struct GameEventQueue;

enum ReplayStepKind {
    REPLAY_STEP_STARTUP = 0,
    REPLAY_STEP_INPUT,
    REPLAY_STEP_IDLE
};

struct ReplayLog {
    FILE *fp;
    const char *path;
    unsigned long next_step;
};

typedef struct ReplayLog ReplayLog;

void replay_log_reset(ReplayLog *log);
int replay_log_is_enabled(const ReplayLog *log);
int replay_log_open(ReplayLog *log, const char *path, u32 seed);
void replay_log_close(ReplayLog *log);
int replay_log_capture(ReplayLog *log, int step_kind, const char *input,
                       const struct GameState *game,
                       const struct GameEventQueue *out);

#endif
