#include "config.h"
#include "gout.h"

void gout_reset(struct GameOutput *out)
{
    out->count = 0;
    out->overflowed = 0;
}

int gout_push(struct GameOutput *out, int kind, int arg0, int arg1, int arg2,
              int arg3, const char *text)
{
    struct GameOutEvent *ev;
    int i;

    if (out == 0) {
        return 0;
    }
    if (out->count >= CFG_GAME_OUT_MAX) {
        out->overflowed = 1;
        return 0;
    }
    ev = &out->events[out->count];
    ev->kind = kind;
    ev->arg0 = arg0;
    ev->arg1 = arg1;
    ev->arg2 = arg2;
    ev->arg3 = arg3;
    ev->room_id = -1;
    for (i = 0; i < CFG_AREA_ITEM_SLOTS; ++i) {
        ev->room_item[i] = 0;
    }
    ev->text = text;
    out->count += 1;
    return 1;
}
