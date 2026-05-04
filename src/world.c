#include <string.h>
#include "world.h"

static void room_set(room, name, desc, animal, animal_noise, n, s, e, w)
struct Room *room;
char *name;
char *desc;
char *animal;
char *animal_noise;
int n;
int s;
int e;
int w;
{
    strncpy(room->name, name, CFG_NAME_MAX - 1);
    room->name[CFG_NAME_MAX - 1] = '\0';
    strncpy(room->desc, desc, CFG_DESC_MAX - 1);
    room->desc[CFG_DESC_MAX - 1] = '\0';
    strncpy(room->animal, animal, CFG_NAME_MAX - 1);
    room->animal[CFG_NAME_MAX - 1] = '\0';
    strncpy(room->animal_noise, animal_noise, CFG_DESC_MAX - 1);
    room->animal_noise[CFG_DESC_MAX - 1] = '\0';

    room->exits[DIR_NORTH] = n;
    room->exits[DIR_SOUTH] = s;
    room->exits[DIR_EAST] = e;
    room->exits[DIR_WEST] = w;
}

void world_init(world)
struct World *world;
{
    world->room_count = 12;
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
        "A small campfire burns quietly.", "cricket",
        "A cricket chirps from beside the coals.", 1, 11, 2, 3);
    room_set(&world->rooms[WORLD_ROOM_ROAD], "Road",
        "A dusty road stretches toward pale hills.", "crow",
        "A crow caws from a leaning signpost.", 6, 0, -1, -1);
    room_set(&world->rooms[WORLD_ROOM_POND], "Pond",
        "A still pond reflects the dim sky. A frog lounges on a lily pad like he owns the place.",
        "frog", "A frog lets out a smug, wet ribbit.", -1, -1, 7, 0);
    room_set(&world->rooms[WORLD_ROOM_FOREST], "Forest",
        "Needles soften your steps; distant branches knit the light.", "owl",
        "An owl hoots once, then holds the dark still.", -1, 5, 0, 8);
    room_set(&world->rooms[WORLD_ROOM_STREAM], "Stream",
        "Cold water chatters over stones. Moss holds every splinter of sound.", "otter",
        "An otter splashes and chatters downstream.", 3, 4, 9, -1);
    room_set(&world->rooms[WORLD_ROOM_RUINS], "Ruins",
        "Broken columns argue with the sky; carved letters sleep under dust.", "lizard",
        "A wall lizard clicks between cracked stones.", 5, 10, -1, -1);
    room_set(&world->rooms[WORLD_ROOM_CLIFF], "Cliff",
        "Wind gnaws at the cliff edge above a dry ravine.", "hawk",
        "A hawk screams high overhead.", -1, 1, -1, -1);
    room_set(&world->rooms[WORLD_ROOM_MARSH], "Marsh",
        "Reeds lean over black water and soft, uncertain ground.", "heron",
        "A heron rattles through the reeds.", -1, -1, -1, 2);
    room_set(&world->rooms[WORLD_ROOM_GROVE], "Grove",
        "Old trees ring a quiet clearing striped with moss.", "fox",
        "A fox yips from behind the roots.", -1, -1, 3, -1);
    room_set(&world->rooms[WORLD_ROOM_BRIDGE], "Bridge",
        "A narrow timber bridge bows over the quick stream.", "kingfisher",
        "A kingfisher snaps its beak and dives.", -1, -1, -1, 5);
    room_set(&world->rooms[WORLD_ROOM_CATACOMBS], "Catacombs",
        "Cold tunnels run under the ruins, ribbed with old masonry.", "rat",
        "Rats skitter and squeak through the dark.", 4, -1, -1, -1);
    room_set(&world->rooms[WORLD_ROOM_MEADOW], "Meadow",
        "Tall grass moves in waves around scattered standing stones.", "hare",
        "A hare thumps through the grass nearby.", 0, -1, -1, -1);
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

const char *world_room_animal(world, room_id)
struct World *world;
int room_id;
{
    if (room_id < 0 || room_id >= world->room_count) {
        return "something";
    }
    return world->rooms[room_id].animal;
}

const char *world_room_animal_noise(world, room_id)
struct World *world;
int room_id;
{
    if (room_id < 0 || room_id >= world->room_count) {
        return "You hear a distant animal noise.";
    }
    return world->rooms[room_id].animal_noise;
}
