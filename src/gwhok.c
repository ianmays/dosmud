#include <string.h>
#include "gwhok.h"
#include "config.h"
#include "game.h"
#include "txtres.h"
#include "world.h"

/*
 * gwhok.c applies persisted authored world advancement consequences from a
 * small static table. Triggers live in npc.c; this module stays free of gout
 * and render seams.
 */

struct GwhokRoomDescRow {
    int adv_id;
    int room_id;
};

/* Each row: flag set -> txtres done desc; cleared -> g_room_descs baseline. */
static const struct GwhokRoomDescRow GWHOK_ROOM_DESC_ROWS[] = {
    { WORLD_ADV_ORCHARD_RESTORED, WORLD_ROOM_ORCHARD },
    { WORLD_ADV_TOWER_MEAL, WORLD_ROOM_TOWER }
};

static const char *gwhok_done_desc(int adv_id)
{
    if (adv_id == WORLD_ADV_ORCHARD_RESTORED) {
        return TXT_STORY_ORCHARD_DONE_DESC;
    }
    if (adv_id == WORLD_ADV_TOWER_MEAL) {
        return TXT_STORY_TOWER_FED_DESC;
    }
    return 0;
}

/* Mutates world.rooms[].desc only; npc.c triggers flags, save stores bits. */
static void gwhok_apply_room_desc(struct GameState *game,
                                  const struct GwhokRoomDescRow *row)
{
    const char *desc;
    int room_id;

    room_id = row->room_id;
    if (room_id < 0 || room_id >= CFG_ROOM_MAX) {
        return;
    }
    if ((game->world_adv_flags & row->adv_id) != 0) {
        desc = gwhok_done_desc(row->adv_id);
    } else {
        desc = g_room_descs[room_id];
    }
    if (desc == 0) {
        return;
    }
    strncpy(game->world.rooms[room_id].desc, desc, CFG_DESC_MAX - 1);
    game->world.rooms[room_id].desc[CFG_DESC_MAX - 1] = '\0';
}

int gwhok_has(const struct GameState *game, int adv_id)
{
    if (adv_id <= 0) {
        return 0;
    }
    return (game->world_adv_flags & adv_id) != 0;
}

int gwhok_set(struct GameState *game, int adv_id)
{
    if (adv_id <= 0) {
        return 0;
    }
    if ((game->world_adv_flags & adv_id) != 0) {
        return 0;
    }
    game->world_adv_flags |= adv_id;
    gwhok_apply_all(game);
    return 1;
}

void gwhok_apply_all(struct GameState *game)
{
    int i;
    int count;

    count = (int)(sizeof(GWHOK_ROOM_DESC_ROWS) /
        sizeof(GWHOK_ROOM_DESC_ROWS[0]));
    for (i = 0; i < count; ++i) {
        gwhok_apply_room_desc(game, &GWHOK_ROOM_DESC_ROWS[i]);
    }
}
