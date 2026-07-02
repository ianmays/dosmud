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
    const char *done_desc;
};

#define GWHOK_ROW_COUNT 2

/*
 * C89 static initializers cannot use txtres extern pointers; fill rows once at
 * first apply so adv_id, room_id, and done_desc stay in one table.
 */
static struct GwhokRoomDescRow gwhok_rows[GWHOK_ROW_COUNT];
static int gwhok_rows_ready;

static void gwhok_rows_init(void)
{
    if (gwhok_rows_ready) {
        return;
    }
    gwhok_rows[0].adv_id = WORLD_ADV_ORCHARD_RESTORED;
    gwhok_rows[0].room_id = WORLD_ROOM_ORCHARD;
    gwhok_rows[0].done_desc = TXT_STORY_ORCHARD_DONE_DESC;
    gwhok_rows[1].adv_id = WORLD_ADV_TOWER_MEAL;
    gwhok_rows[1].room_id = WORLD_ROOM_TOWER;
    gwhok_rows[1].done_desc = TXT_STORY_TOWER_FED_DESC;
    gwhok_rows_ready = 1;
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
        desc = row->done_desc;
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

    gwhok_rows_init();
    for (i = 0; i < GWHOK_ROW_COUNT; ++i) {
        gwhok_apply_room_desc(game, &gwhok_rows[i]);
    }
}
