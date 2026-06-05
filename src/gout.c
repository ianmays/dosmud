#include "config.h"
#include "gout.h"

/*
 * Event queue implementation for the engine-to-render seam.
 * Core producers append fixed-size records here for headless stepping.
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
    if (out->count >= CFG_GAME_EVENT_MAX) {
        /* Preserve deterministic behavior by flagging overflow and dropping. */
        out->overflowed = 1;
        return 0;
    }
    ev = &out->events[out->count];
    ev->kind = kind;
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
