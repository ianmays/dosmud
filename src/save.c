#include <stdio.h>
#include <string.h>
#include "config.h"
#include "game.h"
#include "items.h"
#include "save.h"

/*
 * save.c owns explicit binary serialization of the durable simulation state.
 * The format is versioned and field-by-field so compiler padding and TEST_MODE
 * conditionals do not silently corrupt save files.
 */

#define SAVE_MAGIC "DMSV"
#define SAVE_VERSION 1

/*
 * DOS uses a small default stack. Keep the load-validation scratch snapshot in
 * static storage so reading a save does not duplicate GameState on the stack.
 */
static struct GameState g_save_loaded;

static int save_write_bytes(FILE *fp, const void *src, unsigned int size)
{
    return fwrite(src, 1U, size, fp) == size;
}

static int save_read_bytes(FILE *fp, void *dst, unsigned int size)
{
    return fread(dst, 1U, size, fp) == size;
}

static int save_write_u8(FILE *fp, u8 value)
{
    return save_write_bytes(fp, &value, 1U);
}

static int save_read_u8(FILE *fp, u8 *value)
{
    return save_read_bytes(fp, value, 1U);
}

static int save_write_u16(FILE *fp, u16 value)
{
    u8 bytes[2];

    bytes[0] = (u8)(value & 0xFFU);
    bytes[1] = (u8)((value >> 8) & 0xFFU);
    return save_write_bytes(fp, bytes, 2U);
}

static int save_read_u16(FILE *fp, u16 *value)
{
    u8 bytes[2];

    if (!save_read_bytes(fp, bytes, 2U)) {
        return 0;
    }
    *value = (u16)((u16)bytes[0] | ((u16)bytes[1] << 8));
    return 1;
}

static int save_write_s16(FILE *fp, int value)
{
    return save_write_u16(fp, (u16)(short)value);
}

static int save_read_s16(FILE *fp, int *value)
{
    u16 raw;
    short sval;

    if (!save_read_u16(fp, &raw)) {
        return 0;
    }
    sval = (short)raw;
    *value = (int)sval;
    return 1;
}

static int save_write_u32(FILE *fp, u32 value)
{
    u8 bytes[4];

    bytes[0] = (u8)(value & 0xFFUL);
    bytes[1] = (u8)((value >> 8) & 0xFFUL);
    bytes[2] = (u8)((value >> 16) & 0xFFUL);
    bytes[3] = (u8)((value >> 24) & 0xFFUL);
    return save_write_bytes(fp, bytes, 4U);
}

static int save_read_u32(FILE *fp, u32 *value)
{
    u8 bytes[4];

    if (!save_read_bytes(fp, bytes, 4U)) {
        return 0;
    }
    *value = (u32)bytes[0] |
        ((u32)bytes[1] << 8) |
        ((u32)bytes[2] << 16) |
        ((u32)bytes[3] << 24);
    return 1;
}

static int save_write_room(FILE *fp, const struct Room *room)
{
    int i;

    if (!save_write_bytes(fp, room->name, CFG_NAME_MAX)) {
        return 0;
    }
    if (!save_write_bytes(fp, room->desc, CFG_DESC_MAX)) {
        return 0;
    }
    if (!save_write_bytes(fp, room->animal, CFG_NAME_MAX)) {
        return 0;
    }
    if (!save_write_bytes(fp, room->animal_noise, CFG_DESC_MAX)) {
        return 0;
    }
    for (i = 0; i < DIR_NONE; ++i) {
        if (!save_write_s16(fp, room->exits[i])) {
            return 0;
        }
    }
    return 1;
}

static int save_read_room(FILE *fp, struct Room *room)
{
    int i;

    if (!save_read_bytes(fp, room->name, CFG_NAME_MAX)) {
        return 0;
    }
    if (!save_read_bytes(fp, room->desc, CFG_DESC_MAX)) {
        return 0;
    }
    if (!save_read_bytes(fp, room->animal, CFG_NAME_MAX)) {
        return 0;
    }
    if (!save_read_bytes(fp, room->animal_noise, CFG_DESC_MAX)) {
        return 0;
    }
    for (i = 0; i < DIR_NONE; ++i) {
        if (!save_read_s16(fp, &room->exits[i])) {
            return 0;
        }
    }
    return 1;
}

static int save_write_world(FILE *fp, const struct World *world)
{
    int i;

    if (!save_write_s16(fp, world->room_count)) {
        return 0;
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (!save_write_room(fp, &world->rooms[i])) {
            return 0;
        }
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (!save_write_s16(fp, world->map_x[i]) ||
                !save_write_s16(fp, world->map_y[i]) ||
                !save_write_u8(fp, world->map_ready[i])) {
            return 0;
        }
    }
    return 1;
}

static int save_read_world(FILE *fp, struct World *world)
{
    int i;

    if (!save_read_s16(fp, &world->room_count)) {
        return 0;
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (!save_read_room(fp, &world->rooms[i])) {
            return 0;
        }
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (!save_read_s16(fp, &world->map_x[i]) ||
                !save_read_s16(fp, &world->map_y[i]) ||
                !save_read_u8(fp, &world->map_ready[i])) {
            return 0;
        }
    }
    return 1;
}

static int save_write_game_arrays(FILE *fp, const struct GameState *game)
{
    int i;
    int j;

    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        for (j = 0; j < CFG_AREA_ITEM_SLOTS; ++j) {
            if (!save_write_s16(fp, game->room_item[i][j])) {
                return 0;
            }
        }
    }
    for (i = 0; i < CFG_BAG_MAX; ++i) {
        if (!save_write_s16(fp, game->bag[i])) {
            return 0;
        }
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (!save_write_s16(fp, game->corpse_present[i]) ||
                !save_write_s16(fp, game->corpse_loot[i]) ||
                !save_write_u8(fp, game->room_explored[i])) {
            return 0;
        }
    }
#ifdef TEST_MODE
    for (i = 0; i < CFG_ROLL_INJECT_MAX; ++i) {
        if (!save_write_s16(fp, game->roll_queue[i])) {
            return 0;
        }
    }
#endif
    return 1;
}

static int save_read_game_arrays(FILE *fp, struct GameState *game)
{
    int i;
    int j;

    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        for (j = 0; j < CFG_AREA_ITEM_SLOTS; ++j) {
            if (!save_read_s16(fp, &game->room_item[i][j])) {
                return 0;
            }
        }
    }
    for (i = 0; i < CFG_BAG_MAX; ++i) {
        if (!save_read_s16(fp, &game->bag[i])) {
            return 0;
        }
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (!save_read_s16(fp, &game->corpse_present[i]) ||
                !save_read_s16(fp, &game->corpse_loot[i]) ||
                !save_read_u8(fp, &game->room_explored[i])) {
            return 0;
        }
    }
#ifdef TEST_MODE
    for (i = 0; i < CFG_ROLL_INJECT_MAX; ++i) {
        if (!save_read_s16(fp, &game->roll_queue[i])) {
            return 0;
        }
    }
#endif
    return 1;
}

/*
 * rng_draw_count is plat_rand_draw_count at save time; main.c replays it after
 * load so libc rand() continues where the run left off.
 */
static int save_write_game_state(FILE *fp, const struct GameState *game,
                                 u32 rng_draw_count)
{
    if (!save_write_world(fp, &game->world) ||
            !save_write_s16(fp, game->player.room_id) ||
            !save_write_u32(fp, game->tick) ||
            !save_write_u32(fp, game->seed) ||
            !save_write_u32(fp, rng_draw_count) ||
            !save_write_s16(fp, game->running) ||
            !save_write_s16(fp, game->mode) ||
            !save_write_s16(fp, game->dialogue) ||
            !save_write_s16(fp, game->wanderer_room) ||
            !save_write_s16(fp, game->wanderer_need_separation) ||
            !save_write_s16(fp, game->env_focus_active) ||
            !save_write_s16(fp, game->env_focus_room) ||
            !save_write_s16(fp, game->env_focus_kind) ||
            !save_write_u32(fp, game->env_focus_expires_tick) ||
            !save_write_s16(fp, game->bag_count) ||
            !save_write_s16(fp, game->bag_capacity) ||
            !save_write_s16(fp, game->level) ||
            !save_write_s16(fp, game->xp) ||
            !save_write_s16(fp, game->max_hp) ||
            !save_write_s16(fp, game->damage_bonus) ||
            !save_write_s16(fp, game->weapon_equipped) ||
            !save_write_s16(fp, game->player_hp) ||
            !save_write_s16(fp, game->enemy_handover_pick) ||
            !save_write_s16(fp, game->combat.enemy_hp) ||
            !save_write_s16(fp, game->combat.defending) ||
            !save_write_s16(fp, game->wanderer_active) ||
            !save_write_u32(fp, game->wanderer_return_tick)) {
        return 0;
    }
#ifdef TEST_MODE
    /* Persist harness inject state so TEST_MODE saves resume mid-run. */
    if (!save_write_s16(fp, game->roll_inject_active) ||
            !save_write_s16(fp, game->roll_queue_len) ||
            !save_write_s16(fp, game->roll_queue_i) ||
            !save_write_s16(fp, game->test_quiet_ticks)) {
        return 0;
    }
#endif
    return save_write_game_arrays(fp, game);
}

static int save_read_game_state(FILE *fp, struct GameState *game,
                                u32 *out_rng_draw_count)
{
    if (!save_read_world(fp, &game->world) ||
            !save_read_s16(fp, &game->player.room_id) ||
            !save_read_u32(fp, &game->tick) ||
            !save_read_u32(fp, &game->seed) ||
            !save_read_u32(fp, out_rng_draw_count) ||
            !save_read_s16(fp, &game->running) ||
            !save_read_s16(fp, &game->mode) ||
            !save_read_s16(fp, &game->dialogue) ||
            !save_read_s16(fp, &game->wanderer_room) ||
            !save_read_s16(fp, &game->wanderer_need_separation) ||
            !save_read_s16(fp, &game->env_focus_active) ||
            !save_read_s16(fp, &game->env_focus_room) ||
            !save_read_s16(fp, &game->env_focus_kind) ||
            !save_read_u32(fp, &game->env_focus_expires_tick) ||
            !save_read_s16(fp, &game->bag_count) ||
            !save_read_s16(fp, &game->bag_capacity) ||
            !save_read_s16(fp, &game->level) ||
            !save_read_s16(fp, &game->xp) ||
            !save_read_s16(fp, &game->max_hp) ||
            !save_read_s16(fp, &game->damage_bonus) ||
            !save_read_s16(fp, &game->weapon_equipped) ||
            !save_read_s16(fp, &game->player_hp) ||
            !save_read_s16(fp, &game->enemy_handover_pick) ||
            !save_read_s16(fp, &game->combat.enemy_hp) ||
            !save_read_s16(fp, &game->combat.defending) ||
            !save_read_s16(fp, &game->wanderer_active) ||
            !save_read_u32(fp, &game->wanderer_return_tick)) {
        return 0;
    }
#ifdef TEST_MODE
    if (!save_read_s16(fp, &game->roll_inject_active) ||
            !save_read_s16(fp, &game->roll_queue_len) ||
            !save_read_s16(fp, &game->roll_queue_i) ||
            !save_read_s16(fp, &game->test_quiet_ticks)) {
        return 0;
    }
#endif
    return save_read_game_arrays(fp, game);
}

static int save_string_has_nul(const char *text, unsigned int size)
{
    unsigned int i;

    for (i = 0; i < size; ++i) {
        if (text[i] == '\0') {
            return 1;
        }
    }
    return 0;
}

static int save_valid_item(int item_id)
{
    return item_id >= ITEM_NONE && item_id <= ITEM_SPEAR;
}

static int save_valid_room_index(int room_id, int room_count)
{
    return room_id >= 0 && room_id < room_count;
}

static int save_valid_room_or_none(int room_id, int room_count)
{
    return room_id == -1 || save_valid_room_index(room_id, room_count);
}

static int save_valid_boolish(int value)
{
    return value == 0 || value == 1;
}

static int save_valid_rng_draw_count(u32 rng_draw_count)
{
    return rng_draw_count <= (u32)CFG_SAVE_RNG_DRAW_MAX;
}

static int save_validate_world(const struct World *world)
{
    int i;
    int d;

    if (world->room_count != CFG_ROOM_MAX) {
        return 0;
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (!save_string_has_nul(world->rooms[i].name, CFG_NAME_MAX) ||
                !save_string_has_nul(world->rooms[i].desc, CFG_DESC_MAX) ||
                !save_string_has_nul(world->rooms[i].animal, CFG_NAME_MAX) ||
                !save_string_has_nul(world->rooms[i].animal_noise, CFG_DESC_MAX) ||
                !save_valid_boolish((int)world->map_ready[i])) {
            return 0;
        }
        for (d = 0; d < DIR_NONE; ++d) {
            if (world->rooms[i].exits[d] < -1 ||
                    world->rooms[i].exits[d] >= world->room_count) {
                return 0;
            }
        }
    }
    return 1;
}

static int save_validate_game(const struct GameState *game)
{
    int i;
    int j;
    int room_count;

    if (!save_validate_world(&game->world)) {
        return 0;
    }
    room_count = game->world.room_count;
    if (!save_valid_room_index(game->player.room_id, room_count) ||
            !save_valid_boolish(game->running) ||
            game->mode < GAME_MODE_EXPLORE ||
            game->mode > GAME_MODE_COMBAT ||
            game->dialogue < DIALOGUE_NONE ||
            game->dialogue > DIALOGUE_ENEMY ||
            !save_valid_room_or_none(game->wanderer_room, room_count) ||
            !save_valid_boolish(game->wanderer_need_separation) ||
            !save_valid_boolish(game->env_focus_active) ||
            !save_valid_room_or_none(game->env_focus_room, room_count) ||
            game->env_focus_kind < GAME_ENV_NONE ||
            game->env_focus_kind > GAME_ENV_GRIT ||
            game->bag_count < 0 ||
            game->bag_count > CFG_BAG_MAX ||
            game->bag_capacity < 0 ||
            game->bag_capacity > CFG_BAG_MAX ||
            game->level < CFG_START_LEVEL ||
            game->xp < 0 ||
            game->max_hp < 1 ||
            game->damage_bonus < 0 ||
            !save_valid_item(game->weapon_equipped) ||
            game->player_hp < 0 ||
            game->player_hp > game->max_hp ||
            !save_valid_boolish(game->enemy_handover_pick) ||
            game->combat.enemy_hp < 0 ||
            !save_valid_boolish(game->combat.defending) ||
            !save_valid_boolish(game->wanderer_active)) {
        return 0;
    }
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        if (!save_valid_boolish((int)game->room_explored[i]) ||
                !save_valid_boolish(game->corpse_present[i]) ||
                !save_valid_item(game->corpse_loot[i])) {
            return 0;
        }
        for (j = 0; j < CFG_AREA_ITEM_SLOTS; ++j) {
            if (!save_valid_item(game->room_item[i][j])) {
                return 0;
            }
        }
    }
    for (i = 0; i < CFG_BAG_MAX; ++i) {
        if (!save_valid_item(game->bag[i])) {
            return 0;
        }
    }
#ifdef TEST_MODE
    if (!save_valid_boolish(game->roll_inject_active) ||
            game->roll_queue_len < 0 ||
            game->roll_queue_len > CFG_ROLL_INJECT_MAX ||
            game->roll_queue_i < 0 ||
            game->roll_queue_i > CFG_ROLL_INJECT_MAX ||
            !save_valid_boolish(game->test_quiet_ticks)) {
        return 0;
    }
#endif
    return 1;
}

int save_write_game(const char *path, const struct GameState *game,
                    u32 rng_draw_count)
{
    FILE *fp;
    int ok;
    int close_rc;

    if (path == 0 || path[0] == '\0' || game == 0) {
        return SAVE_RESULT_IO;
    }
    if (!save_valid_rng_draw_count(rng_draw_count)) {
        return SAVE_RESULT_RANGE;
    }
    fp = fopen(path, "wb");
    if (fp == 0) {
        return SAVE_RESULT_IO;
    }

    ok = save_write_bytes(fp, SAVE_MAGIC, 4U) &&
        save_write_u16(fp, (u16)SAVE_VERSION) &&
        save_write_game_state(fp, game, rng_draw_count);
    if (ok && fflush(fp) != 0) {
        ok = 0;
    }
    close_rc = fclose(fp);
    if (!ok || close_rc != 0) {
        return SAVE_RESULT_IO;
    } 
    return SAVE_RESULT_OK;
}

int save_read_game(const char *path, struct GameState *out_game,
                   u32 *out_rng_draw_count)
{
    FILE *fp;
    char magic[4];
    u16 version;
    u32 loaded_rng_draw_count;
    int rc;
    int trailing;

    if (path == 0 || path[0] == '\0' || out_game == 0 ||
            out_rng_draw_count == 0) {
        return SAVE_RESULT_IO;
    }
    fp = fopen(path, "rb");
    if (fp == 0) {
        return SAVE_RESULT_IO;
    }

    rc = SAVE_RESULT_FORMAT;
    if (!save_read_bytes(fp, magic, 4U)) {
        rc = SAVE_RESULT_IO;
        goto done;
    }
    if (memcmp(magic, SAVE_MAGIC, 4U) != 0) {
        goto done;
    }
    if (!save_read_u16(fp, &version)) {
        rc = SAVE_RESULT_IO;
        goto done;
    }
    if (version != (u16)SAVE_VERSION) {
        goto done;
    }
    if (!save_read_game_state(fp, &g_save_loaded, &loaded_rng_draw_count)) {
        goto done;
    }
    /* Reject padded or concatenated files; payload must end at EOF. */
    trailing = fgetc(fp);
    if (trailing != EOF) {
        goto done;
    }
    /* Validate into g_save_loaded so a bad file does not clobber out_game. */
    if (!save_valid_rng_draw_count(loaded_rng_draw_count) ||
            !save_validate_game(&g_save_loaded)) {
        rc = SAVE_RESULT_RANGE;
        goto done;
    }
    *out_game = g_save_loaded;
    *out_rng_draw_count = loaded_rng_draw_count;
    rc = SAVE_RESULT_OK;

done:
    if (fclose(fp) != 0 && rc == SAVE_RESULT_OK) {
        return SAVE_RESULT_IO;
    }
    return rc;
}
