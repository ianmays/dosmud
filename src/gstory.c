#include "gstory.h"
#include "config.h"
#include "game.h"
#include "invent.h"
#include "items.h"

/*
 * gstory.c owns reusable fetch-quest progress derivation and recoverable item
 * seeding. Authored NPC slices (npc.c) keep exchange, world hooks, and dialogue.
 * Read-only world queries and room_item placement only; no gout or render seam.
 */

static int story_room_has_item(const struct GameState *game, int room_id,
                               int item_id)
{
    int slot;

    if (room_id < 0 || room_id >= CFG_ROOM_MAX) {
        return 0;
    }
    for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
        if (game->room_item[room_id][slot] == item_id) {
            return 1;
        }
    }
    return 0;
}

/* True when item_id is in player bag or any room slot (invent + room_item scan). */
int story_world_has_item(const struct GameState *game, int item_id)
{
    int room_id;

    if (game_inv_player_has_item((struct GameState *)game, item_id)) {
        return 1;
    }
    for (room_id = 0; room_id < CFG_ROOM_MAX; ++room_id) {
        if (story_room_has_item(game, room_id, item_id)) {
            return 1;
        }
    }
    return 0;
}

enum StoryFetchScene story_fetch_scene(int progress, int has_required_item)
{
    if (progress == STORY_PROGRESS_DONE) {
        return STORY_FETCH_DONE;
    }
    if (progress == STORY_PROGRESS_ACTIVE) {
        if (has_required_item) {
            return STORY_FETCH_READY;
        }
        return STORY_FETCH_ACTIVE;
    }
    /* Values outside STORY_PROGRESS_* fall back to NOT_STARTED. */
    return STORY_FETCH_NOT_STARTED;
}

/*
 * Idempotent recoverable seed: when the item is already in play, spawned_out=1
 * with no room mutation. Otherwise place in the lowest empty slot in room_id;
 * spawned=0 when the room has no free slots.
 */
int story_seed_recoverable_item(struct GameState *game, int room_id,
                                int item_id, int *spawned_out)
{
    int slot;
    int spawned;

    if (story_world_has_item(game, item_id)) {
        spawned = 1;
        if (spawned_out != 0) {
            *spawned_out = spawned;
        }
        return spawned;
    }
    spawned = 0;
    if (room_id >= 0 && room_id < CFG_ROOM_MAX) {
        for (slot = 0; slot < CFG_AREA_ITEM_SLOTS; ++slot) {
            if (game->room_item[room_id][slot] == ITEM_NONE) {
                game->room_item[room_id][slot] = item_id;
                spawned = 1;
                break;
            }
        }
    }
    if (spawned_out != 0) {
        *spawned_out = spawned;
    }
    return spawned;
}
