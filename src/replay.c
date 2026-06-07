#include <ctype.h>
#include <stdio.h>
#include "game.h"
#include "gout.h"
#include "replay.h"

/*
 * replay.c owns the optional shell-level event log path. It serializes the
 * existing per-step GameEventQueue without changing engine or renderer state.
 * Text fields use C-style escapes; non-printable bytes become \xHH literals.
 */

static const char *replay_step_name(int step_kind)
{
    switch (step_kind) {
    case REPLAY_STEP_STARTUP:
        return "startup";
    case REPLAY_STEP_INPUT:
        return "input";
    case REPLAY_STEP_IDLE:
        return "idle";
    default:
        return "unknown";
    }
}

static const char *replay_mode_name(int mode)
{
    switch (mode) {
    case GAME_MODE_EXPLORE:
        return "explore";
    case GAME_MODE_DIALOGUE:
        return "dialogue";
    case GAME_MODE_COMBAT:
        return "combat";
    default:
        return "unknown";
    }
}

/* Keep in sync with GameEventKind in gout.h; unknown kinds log as GAME_EVENT_UNKNOWN. */
static const char *replay_event_name(int kind)
{
    switch (kind) {
    case GAME_EVENT_NONE:
        return "GAME_EVENT_NONE";
    case GAME_EVENT_MOVE:
        return "GAME_EVENT_MOVE";
    case GAME_EVENT_ROOM_LOOK:
        return "GAME_EVENT_ROOM_LOOK";
    case GAME_EVENT_MAP:
        return "GAME_EVENT_MAP";
    case GAME_EVENT_HELP:
        return "GAME_EVENT_HELP";
    case GAME_EVENT_WAIT:
        return "GAME_EVENT_WAIT";
    case GAME_EVENT_CANNOT_MOVE:
        return "GAME_EVENT_CANNOT_MOVE";
    case GAME_EVENT_UNKNOWN_COMMAND:
        return "GAME_EVENT_UNKNOWN_COMMAND";
    case GAME_EVENT_ITEM_RESULT:
        return "GAME_EVENT_ITEM_RESULT";
    case GAME_EVENT_BAG_VIEW:
        return "GAME_EVENT_BAG_VIEW";
    case GAME_EVENT_CRAFT_RESULT:
        return "GAME_EVENT_CRAFT_RESULT";
    case GAME_EVENT_EQUIP_RESULT:
        return "GAME_EVENT_EQUIP_RESULT";
    case GAME_EVENT_COMBAT:
        return "GAME_EVENT_COMBAT";
    case GAME_EVENT_XP_GAIN:
        return "GAME_EVENT_XP_GAIN";
    case GAME_EVENT_STAT_CHANGE:
        return "GAME_EVENT_STAT_CHANGE";
    case GAME_EVENT_DIALOGUE:
        return "GAME_EVENT_DIALOGUE";
    case GAME_EVENT_ENCOUNTER:
        return "GAME_EVENT_ENCOUNTER";
    case GAME_EVENT_DIALOGUE_GUARD:
        return "GAME_EVENT_DIALOGUE_GUARD";
    case GAME_EVENT_ENVIRONMENT:
        return "GAME_EVENT_ENVIRONMENT";
    case GAME_EVENT_AMBIENT_NOISE:
        return "GAME_EVENT_AMBIENT_NOISE";
    case GAME_EVENT_ITEM_PRESENCE:
        return "GAME_EVENT_ITEM_PRESENCE";
    case GAME_EVENT_OBSERVATION:
        return "GAME_EVENT_OBSERVATION";
    default:
        return "GAME_EVENT_UNKNOWN";
    }
}

static int replay_write_hex_byte(FILE *fp, unsigned char c)
{
    static const char *digits = "0123456789ABCDEF";

    if (fputc('\\', fp) == EOF || fputc('x', fp) == EOF) {
        return 0;
    }
    if (fputc(digits[(c >> 4) & 0x0F], fp) == EOF) {
        return 0;
    }
    if (fputc(digits[c & 0x0F], fp) == EOF) {
        return 0;
    }
    return 1;
}

static int replay_write_literal(FILE *fp, const char *text)
{
    return fputs(text, fp) != EOF;
}

static char *replay_append_literal(char *dst, const char *src)
{
    while (*src != '\0') {
        *dst = *src;
        ++dst;
        ++src;
    }
    return dst;
}

static char *replay_append_ulong(char *dst, unsigned long value)
{
    char digits[16];
    int i;

    if (value == 0UL) {
        *dst = '0';
        return dst + 1;
    }

    i = 0;
    while (value > 0UL) {
        digits[i] = (char)('0' + (value % 10UL));
        value /= 10UL;
        ++i;
    }
    while (i > 0) {
        --i;
        *dst = digits[i];
        ++dst;
    }
    return dst;
}

static char *replay_append_int(char *dst, int value)
{
    unsigned long abs_value;

    if (value < 0) {
        *dst = '-';
        ++dst;
        abs_value = (unsigned long)(0 - (long)value);
    } else {
        abs_value = (unsigned long)value;
    }
    return replay_append_ulong(dst, abs_value);
}

static int replay_write_text(FILE *fp, const char *text)
{
    const unsigned char *p;

    if (text == 0) {
        return replay_write_literal(fp, "null");
    }

    if (fputc('"', fp) == EOF) {
        return 0;
    }
    p = (const unsigned char *)text;
    while (*p != '\0') {
        switch (*p) {
        case '\\':
            if (fputc('\\', fp) == EOF || fputc('\\', fp) == EOF) {
                return 0;
            }
            break;
        case '"':
            if (fputc('\\', fp) == EOF || fputc('"', fp) == EOF) {
                return 0;
            }
            break;
        case '\n':
            if (fputc('\\', fp) == EOF || fputc('n', fp) == EOF) {
                return 0;
            }
            break;
        case '\r':
            if (fputc('\\', fp) == EOF || fputc('r', fp) == EOF) {
                return 0;
            }
            break;
        case '\t':
            if (fputc('\\', fp) == EOF || fputc('t', fp) == EOF) {
                return 0;
            }
            break;
        default:
            if (isprint((int)*p)) {
                if (fputc((int)*p, fp) == EOF) {
                    return 0;
                }
            } else if (!replay_write_hex_byte(fp, *p)) {
                return 0;
            }
            break;
        }
        ++p;
    }

    return fputc('"', fp) != EOF;
}

static int replay_write_event(FILE *fp, int index, const GameEvent *ev)
{
    char buf[192];
    char *dst;
    int i;

    dst = buf;
    dst = replay_append_literal(dst, "event=");
    dst = replay_append_int(dst, index);
    dst = replay_append_literal(dst, " kind=");
    dst = replay_append_literal(dst, replay_event_name(ev->kind));
    dst = replay_append_literal(dst, " arg0=");
    dst = replay_append_int(dst, ev->arg0);
    dst = replay_append_literal(dst, " arg1=");
    dst = replay_append_int(dst, ev->arg1);
    dst = replay_append_literal(dst, " arg2=");
    dst = replay_append_int(dst, ev->arg2);
    dst = replay_append_literal(dst, " arg3=");
    dst = replay_append_int(dst, ev->arg3);
    dst = replay_append_literal(dst, " room=");
    dst = replay_append_int(dst, ev->room_id);
    dst = replay_append_literal(dst, " room_items=[");
    *dst = '\0';
    if (!replay_write_literal(fp, buf)) {
        return 0;
    }

    for (i = 0; i < CFG_AREA_ITEM_SLOTS; ++i) {
        if (i != 0 && fputc(',', fp) == EOF) {
            return 0;
        }
        dst = buf;
        dst = replay_append_int(dst, ev->room_item[i]);
        *dst = '\0';
        if (!replay_write_literal(fp, buf)) {
            return 0;
        }
    }

    if (!replay_write_literal(fp, "] text=")) {
        return 0;
    }
    if (!replay_write_text(fp, ev->text)) {
        return 0;
    }
    return fputc('\n', fp) != EOF;
}

void replay_log_reset(ReplayLog *log)
{
    if (log == 0) {
        return;
    }
    log->fp = 0;
    log->path = 0;
    log->next_step = 0;
}

int replay_log_is_enabled(const ReplayLog *log)
{
    return log != 0 && log->fp != 0;
}

int replay_log_open(ReplayLog *log, const char *path, u32 seed)
{
    FILE *fp;

    if (log == 0 || path == 0 || path[0] == '\0') {
        return 0;
    }

    replay_log_close(log);
    fp = fopen(path, "w");
    if (fp == 0) {
        return 0;
    }
    {
        char buf[64];
        char *dst;

        dst = buf;
        /* dosmud-replay-v1 is the on-disk format tag; bump when fields change. */
        dst = replay_append_literal(dst, "dosmud-replay-v1 seed=");
        dst = replay_append_ulong(dst, (unsigned long)seed);
        *dst = '\n';
        ++dst;
        *dst = '\0';
        if (!replay_write_literal(fp, buf) || fflush(fp) != 0) {
            fclose(fp);
            return 0;
        }
    }

    log->fp = fp;
    log->path = path;
    log->next_step = 0;
    return 1;
}

void replay_log_close(ReplayLog *log)
{
    if (log == 0) {
        return;
    }
    if (log->fp != 0) {
        fclose(log->fp);
    }
    replay_log_reset(log);
}

int replay_log_capture(ReplayLog *log, int step_kind, const char *input,
                       const struct GameState *game,
                       const struct GameEventQueue *out)
{
    char buf[256];
    char *dst;
    int i;

    if (!replay_log_is_enabled(log)) {
        return 1; /* no-op when --replay-log was not requested */
    }
    if (game == 0 || out == 0) {
        return 0;
    }

    dst = buf;
    dst = replay_append_literal(dst, "step=");
    dst = replay_append_ulong(dst, log->next_step);
    dst = replay_append_literal(dst, " kind=");
    dst = replay_append_literal(dst, replay_step_name(step_kind));
    dst = replay_append_literal(dst, " tick=");
    dst = replay_append_ulong(dst, (unsigned long)game->tick);
    dst = replay_append_literal(dst, " running=");
    dst = replay_append_int(dst, game->running);
    dst = replay_append_literal(dst, " mode=");
    dst = replay_append_literal(dst, replay_mode_name(game->mode));
    dst = replay_append_literal(dst, " events=");
    dst = replay_append_int(dst, out->count);
    dst = replay_append_literal(dst, " overflow=");
    dst = replay_append_int(dst, out->overflowed);
    dst = replay_append_literal(dst, " input=");
    *dst = '\0';
    if (!replay_write_literal(log->fp, buf)) {
        return 0;
    }
    if (!replay_write_text(log->fp, input) || fputc('\n', log->fp) == EOF) {
        return 0;
    }

    for (i = 0; i < out->count; ++i) {
        if (!replay_write_event(log->fp, i, &out->events[i])) {
            return 0;
        }
    }

    log->next_step += 1;
    return fflush(log->fp) == 0;
}
