#include <string.h>
#include "world.h"

static void room_set(room, name, desc, n, s, e, w)
struct Room *room;
char *name;
char *desc;
int n;
int s;
int e;
int w;
{
    strncpy(room->name, name, CFG_NAME_MAX - 1);
    room->name[CFG_NAME_MAX - 1] = '\0';
    strncpy(room->desc, desc, CFG_DESC_MAX - 1);
    room->desc[CFG_DESC_MAX - 1] = '\0';

    room->exits[DIR_NORTH] = n;
    room->exits[DIR_SOUTH] = s;
    room->exits[DIR_EAST] = e;
    room->exits[DIR_WEST] = w;
}

void world_init(world)
struct World *world;
{
    world->room_count = 6;
    /*
     *        [Road]
     *          |
     * [Forest]-[Camp]-[Pond]
     *          |
     *       [Stream]
     *          |
     *       [Ruins]
     */
    room_set(&world->rooms[WORLD_ROOM_CAMP], "Camp",
        "A small campfire burns quietly.", 1, -1, 2, 3);
    room_set(&world->rooms[WORLD_ROOM_ROAD], "Road",
        "A dusty road stretches toward pale hills.", -1, 0, -1, -1);
    room_set(&world->rooms[WORLD_ROOM_POND], "Pond",
        "A still pond reflects the dim sky. A frog lounges on a lily pad like he owns the place.",
        -1, -1, -1, 0);
    room_set(&world->rooms[WORLD_ROOM_FOREST], "Forest",
        "Needles soften your steps; distant branches knit the light.", -1, 5, 0, -1);
    room_set(&world->rooms[WORLD_ROOM_STREAM], "Stream",
        "Cold water chatters over stones. Moss holds every splinter of sound.", 3, 4, -1, -1);
    room_set(&world->rooms[WORLD_ROOM_RUINS], "Ruins",
        "Broken columns argue with the sky; carved letters sleep under dust.", 5, -1, -1, -1);
}

void world_step(world, tick)
struct World *world;
unsigned long tick;
{
    /* Placeholder for NPC/environment turns. */
    (void)world;
    (void)tick;
}

int world_can_move(world, room_id, dir)
struct World *world;
int room_id;
int dir;
{
    if (room_id < 0 || room_id >= world->room_count) {
        return 0;
    }
    if (dir < 0 || dir >= DIR_NONE) {
        return 0;
    }
    return world->rooms[room_id].exits[dir] >= 0;
}

int world_move(world, room_id, dir)
struct World *world;
int room_id;
int dir;
{
    if (!world_can_move(world, room_id, dir)) {
        return room_id;
    }
    return world->rooms[room_id].exits[dir];
}

const char *world_dir_name(dir)
int dir;
{
    if (dir == DIR_NORTH) return "north";
    if (dir == DIR_SOUTH) return "south";
    if (dir == DIR_EAST) return "east";
    if (dir == DIR_WEST) return "west";
    return "unknown";
}
