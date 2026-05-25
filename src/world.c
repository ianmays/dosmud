#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "txtres.h"

/*
 * world.c owns the generated room graph and its coordinate projection for the
 * local map display. It keeps the topology deterministic for a given seed.
 */

static void room_set_meta(struct Room *room, const char *name, const char *desc,
    const char *animal, const char *animal_noise)
{
    int i;
    strncpy(room->name, name, CFG_NAME_MAX - 1);
    room->name[CFG_NAME_MAX - 1] = '\0';
    strncpy(room->desc, desc, CFG_DESC_MAX - 1);
    room->desc[CFG_DESC_MAX - 1] = '\0';
    strncpy(room->animal, animal, CFG_NAME_MAX - 1);
    room->animal[CFG_NAME_MAX - 1] = '\0';
    strncpy(room->animal_noise, animal_noise, CFG_DESC_MAX - 1);
    room->animal_noise[CFG_DESC_MAX - 1] = '\0';
    for (i = 0; i < DIR_NONE; ++i) {
        room->exits[i] = -1;
    }
}

static int dir_delta_x(int dir)
{
    if (dir == DIR_EAST) return 1;
    if (dir == DIR_WEST) return -1;
    return 0;
}

static int dir_delta_y(int dir)
{
    if (dir == DIR_NORTH) return -1;
    if (dir == DIR_SOUTH) return 1;
    return 0;
}

/*
 * Assign a room to map coordinates; nudges slightly if another room already
 * occupies the same cell (rare for this generator).
 */
static void world_assign_cell(struct World *world, int rid, int x, int y)
{
    int ox;
    int oy;
    int bump;
    /* The map projection prefers unique cells so the explored view does not overlap. */
    ox = x;
    oy = y;
    bump = 0;
    while (bump < CFG_ROOM_MAX * 8) {
        int k;
        int clash;

        clash = 0;
        for (k = 0; k < CFG_ROOM_MAX; ++k) {
            if (k == rid) {
                continue;
            }
            if (!world->map_ready[k]) {
                continue;
            }
            if (world->map_x[k] == ox && world->map_y[k] == oy) {
                clash = 1;
                break;
            }
        }
        if (!clash) {
            break;
        }
        bump++;
        ox = x + (bump % 5) - 2;
        oy = y + ((bump / 5) % 5) - 2;
    }
    world->map_x[rid] = ox;
    world->map_y[rid] = oy;
    world->map_ready[rid] = 1;
}

static void world_link2(struct World *world, int a, int b, int dir_from_a)
{
    int reverse;
    /* Links are symmetric; map coordinates are propagated from whichever side is already placed. */
    if (dir_from_a == DIR_NORTH) reverse = DIR_SOUTH;
    else if (dir_from_a == DIR_SOUTH) reverse = DIR_NORTH;
    else if (dir_from_a == DIR_EAST) reverse = DIR_WEST;
    else reverse = DIR_EAST;

    world->rooms[a].exits[dir_from_a] = b;
    world->rooms[b].exits[reverse] = a;

    if (world->map_ready[a]) {
        if (!world->map_ready[b]) {
            world_assign_cell(world, b,
                world->map_x[a] + dir_delta_x(dir_from_a),
                world->map_y[a] + dir_delta_y(dir_from_a));
        }
    } else if (world->map_ready[b]) {
        if (!world->map_ready[a]) {
            world_assign_cell(world, a,
                world->map_x[b] + dir_delta_x(reverse),
                world->map_y[b] + dir_delta_y(reverse));
        }
    }
}

static int random_slot(struct Room *room)
{
    int dirs[CFG_DIR_MAX];
    int n;
    int i;
    /* Choose only from currently open exits so generation preserves sparse branching. */
    n = 0;
    for (i = 0; i < DIR_NONE; ++i) {
        if (room->exits[i] < 0) {
            dirs[n] = i;
            ++n;
        }
    }
    if (n <= 0) return -1;
    return dirs[rand() % n];
}

void world_init(struct World *world)
{
    int i;
    int wilds[CFG_WORLD_WILDS_COUNT];
    int ruins[CFG_WORLD_RUINS_COUNT];
    int base_path[CFG_ROOM_MAX];
    int path_len;
    int dir;

    /* Seeded world generation is order-sensitive, so the shuffle and link pass stay explicit. */
    world->room_count = CFG_ROOM_MAX;

    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        room_set_meta(&world->rooms[i],
            g_room_names[i],
            g_room_descs[i],
            g_room_animals[i],
            g_room_noises[i]);
        world->map_x[i] = 0;
        world->map_y[i] = 0;
        world->map_ready[i] = 0;
    }

    wilds[0] = WORLD_ROOM_FOREST;
    wilds[1] = WORLD_ROOM_MEADOW;
    wilds[2] = WORLD_ROOM_GROVE;
    wilds[3] = WORLD_ROOM_MARSH;
    wilds[4] = WORLD_ROOM_POND;
    wilds[5] = WORLD_ROOM_ORCHARD;
    wilds[6] = WORLD_ROOM_STREAM;

    ruins[0] = WORLD_ROOM_RUINS;
    ruins[1] = WORLD_ROOM_CATACOMBS;
    ruins[2] = WORLD_ROOM_CLIFF;
    ruins[3] = WORLD_ROOM_CANYON;
    ruins[4] = WORLD_ROOM_CAVE;

    for (i = 0; i < CFG_WORLD_WILDS_COUNT; ++i) {
        int j;
        j = rand() % CFG_WORLD_WILDS_COUNT;
        {
            int t = wilds[i];
            wilds[i] = wilds[j];
            wilds[j] = t;
        }
    }
    for (i = 0; i < CFG_WORLD_RUINS_COUNT; ++i) {
        int j;
        j = rand() % CFG_WORLD_RUINS_COUNT;
        {
            int t = ruins[i];
            ruins[i] = ruins[j];
            ruins[j] = t;
        }
    }

    path_len = 0;
    base_path[path_len++] = wilds[0];
    base_path[path_len++] = wilds[1];
    base_path[path_len++] = WORLD_ROOM_CAMP;
    base_path[path_len++] = WORLD_ROOM_ROAD;
    base_path[path_len++] = WORLD_ROOM_TOWER;
    base_path[path_len++] = WORLD_ROOM_BRIDGE;
    base_path[path_len++] = ruins[0];
    base_path[path_len++] = ruins[1];
    base_path[path_len++] = ruins[2];

    world_assign_cell(world, base_path[0], 0, 0);

    for (i = 0; i < path_len - 1; ++i) {
        dir = random_slot(&world->rooms[base_path[i]]);
        if (dir < 0) dir = DIR_NORTH;
        world_link2(world, base_path[i], base_path[i + 1], dir);
    }

    for (i = CFG_WORLD_WILD_BRANCH_START_INDEX; i < CFG_WORLD_WILDS_COUNT; ++i) {
        int anchor;
        anchor = base_path[rand() % path_len];
        dir = random_slot(&world->rooms[anchor]);
        if (dir >= 0) {
            world_link2(world, anchor, wilds[i], dir);
        }
    }

    for (i = CFG_WORLD_RUIN_BRANCH_START_INDEX; i < CFG_WORLD_RUINS_COUNT; ++i) {
        int anchor;
        anchor = ruins[rand() % CFG_WORLD_RUIN_ANCHOR_POOL];
        dir = random_slot(&world->rooms[anchor]);
        if (dir >= 0) {
            world_link2(world, anchor, ruins[i], dir);
        }
    }

    /* Ensure pond and orchard remain soft-biome neighbors when possible. */
    if (world->rooms[WORLD_ROOM_POND].exits[DIR_EAST] < 0 &&
            world->rooms[WORLD_ROOM_ORCHARD].exits[DIR_WEST] < 0) {
        world_link2(world, WORLD_ROOM_POND, WORLD_ROOM_ORCHARD, DIR_EAST);
    }

    for (i = 0; i < world->room_count; ++i) {
        int connected;
        int d;
        connected = 0;
        for (d = 0; d < DIR_NONE; ++d) {
            if (world->rooms[i].exits[d] >= 0) {
                connected = 1;
                break;
            }
        }
        if (!connected && i != WORLD_ROOM_CAMP) {
            dir = random_slot(&world->rooms[WORLD_ROOM_CAMP]);
            if (dir >= 0) {
                world_link2(world, WORLD_ROOM_CAMP, i, dir);
                continue;
            }
            dir = random_slot(&world->rooms[WORLD_ROOM_ROAD]);
            if (dir >= 0) {
                world_link2(world, WORLD_ROOM_ROAD, i, dir);
            }
        }
    }
}

#ifdef TEST_MODE
/* Apply a harness-supplied graph without re-running seeded generation. */

static void world_reset_graph(struct World *world)
{
    int i;
    int d;

    world->room_count = CFG_ROOM_MAX;
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        room_set_meta(&world->rooms[i],
            g_room_names[i],
            g_room_descs[i],
            g_room_animals[i],
            g_room_noises[i]);
        world->map_x[i] = 0;
        world->map_y[i] = 0;
        world->map_ready[i] = 0;
        for (d = 0; d < DIR_NONE; ++d) {
            world->rooms[i].exits[d] = -1;
        }
    }
}

void world_apply_graph(struct World *world,
    const int exits[CFG_ROOM_MAX][CFG_DIR_MAX],
    const int map_x[CFG_ROOM_MAX],
    const int map_y[CFG_ROOM_MAX])
{
    int i;
    int d;

    world_reset_graph(world);
    for (i = 0; i < CFG_ROOM_MAX; ++i) {
        for (d = 0; d < DIR_NONE; ++d) {
            world->rooms[i].exits[d] = exits[i][d];
        }
        world->map_x[i] = map_x[i];
        world->map_y[i] = map_y[i];
        world->map_ready[i] = 1;
    }
}
#endif /* TEST_MODE */

void world_step(struct World *world, u32 tick)
{
    /* World stepping is a hook today; keeping it explicit preserves the call boundary. */
    (void)world;
    (void)tick;
}

int world_can_move(struct World *world, int room_id, int dir)
{
    if (room_id < 0 || room_id >= world->room_count) {
        return 0;
    }
    if (dir < 0 || dir >= DIR_NONE) {
        return 0;
    }
    return world->rooms[room_id].exits[dir] >= 0;
}

int world_move(struct World *world, int room_id, int dir)
{
    if (!world_can_move(world, room_id, dir)) {
        return room_id;
    }
    return world->rooms[room_id].exits[dir];
}

const char *world_dir_name(int dir)
{
    return txtres_dir_name(dir);
}

const char *world_room_animal(struct World *world, int room_id)
{
    if (room_id < 0 || room_id >= world->room_count) {
        return TXT_ROOM_ANIMAL_FALLBACK;
    }
    return world->rooms[room_id].animal;
}

const char *world_room_animal_noise(struct World *world, int room_id)
{
    if (room_id < 0 || room_id >= world->room_count) {
        return TXT_ROOM_NOISE_FALLBACK;
    }
    return world->rooms[room_id].animal_noise;
}
