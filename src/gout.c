#include "config.h"
#include "gout.h"

/*
 * Event queue implementation for the engine-to-render seam.
 * Core producers append fixed-size records here; legacy helpers stay as a
 * temporary bridge while #47 follow-up migration slices land.
 */

void game_event_queue_reset(GameEventQueue *out)
{
    if (out == 0) {
        return;
    }
    out->count = 0;
    out->overflowed = 0;
}

GameEvent *game_event_push(GameEventQueue *out, int kind, int arg0, int arg1,
                           int arg2, int arg3, const char *text)
{
    GameEvent *ev;
    int i;

    if (out == 0) {
        return 0;
    }
    if (out->count >= CFG_GAME_OUT_MAX) {
        /* Preserve deterministic behavior by flagging overflow and dropping. */
        out->overflowed = 1;
        return 0;
    }
    ev = &out->events[out->count];
    ev->kind = kind;
    ev->legacy_kind = GAME_OUT_NONE;
    ev->arg0 = arg0;
    ev->arg1 = arg1;
    ev->arg2 = arg2;
    ev->arg3 = arg3;
    /* Room snapshot fields are filled by the producer when needed (e.g. look). */
    ev->room_id = -1;
    for (i = 0; i < CFG_AREA_ITEM_SLOTS; ++i) {
        ev->room_item[i] = 0;
    }
    ev->text = text;
    out->count += 1;
    return ev;
}

GameEvent *game_event_push_legacy(GameEventQueue *out, int legacy_kind,
                                  int arg0, int arg1, int arg2, int arg3,
                                  const char *text)
{
    GameEvent *ev;

    /* Legacy producers are wrapped in a generic event until follow-ups migrate. */
    ev = game_event_push(out, GAME_EVENT_LEGACY, arg0, arg1, arg2, arg3, text);
    if (ev == 0) {
        return 0;
    }
    ev->legacy_kind = legacy_kind;
    return ev;
}

void gout_reset(struct GameOutput *out)
{
    /* Transitional API retained for unchanged GAME_OUT_* producers. */
    game_event_queue_reset(out);
}

int gout_push(struct GameOutput *out, int kind, int arg0, int arg1, int arg2,
              int arg3, const char *text)
{
    if (game_event_push_legacy(out, kind, arg0, arg1, arg2, arg3, text) == 0) {
        return 0;
    }
    return 1;
}
