#include <stdlib.h>
#include <string.h>
#include "world.h"

static void room_set_meta(room, name, desc, animal, animal_noise)
struct Room *room;
char *name;
char *desc;
char *animal;
char *animal_noise;
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

static void world_link2(world, a, b, dir_from_a)
struct World *world;
int a;
int b;
int dir_from_a;
{
    int reverse;
    if (dir_from_a == DIR_NORTH) reverse = DIR_SOUTH;
    else if (dir_from_a == DIR_SOUTH) reverse = DIR_NORTH;
    else if (dir_from_a == DIR_EAST) reverse = DIR_WEST;
    else reverse = DIR_EAST;

    world->rooms[a].exits[dir_from_a] = b;
    world->rooms[b].exits[reverse] = a;
}

static int random_slot(room)
struct Room *room;
{
    int dirs[CFG_DIR_MAX];
    int n;
    int i;
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

void world_init(world)
struct World *world;
{
    int i;
    int wilds[7];
    int ruins[5];
    int base_path[16];
    int path_len;
    int dir;

    world->room_count = 16;

    room_set_meta(&world->rooms[WORLD_ROOM_CAMP], "Camp",
        "A small campfire burns quietly.", "cricket",
        "A cricket chirps from beside the coals.");
    room_set_meta(&world->rooms[WORLD_ROOM_ROAD], "Road",
        "A dusty road stretches toward pale hills.", "crow",
        "A crow caws from a leaning signpost.");
    room_set_meta(&world->rooms[WORLD_ROOM_POND], "Pond",
        "A still pond reflects the dim sky.", "frog",
        "A frog lets out a smug, wet ribbit.");
    room_set_meta(&world->rooms[WORLD_ROOM_FOREST], "Forest",
        "Needles soften your steps; distant branches knit the light.", "owl",
        "An owl hoots once, then holds the dark still.");
    room_set_meta(&world->rooms[WORLD_ROOM_RUINS], "Ruins",
        "Broken columns argue with the sky.", "lizard",
        "A wall lizard clicks between cracked stones.");
    room_set_meta(&world->rooms[WORLD_ROOM_STREAM], "Stream",
        "Cold water chatters over stones.", "otter",
        "An otter splashes and chatters downstream.");
    room_set_meta(&world->rooms[WORLD_ROOM_CLIFF], "Cliff",
        "Wind gnaws at the cliff edge.", "hawk",
        "A hawk screams high overhead.");
    room_set_meta(&world->rooms[WORLD_ROOM_MARSH], "Marsh",
        "Reeds lean over black water.", "heron",
        "A heron rattles through the reeds.");
    room_set_meta(&world->rooms[WORLD_ROOM_GROVE], "Grove",
        "Old trees ring a quiet clearing.", "fox",
        "A fox yips from behind the roots.");
    room_set_meta(&world->rooms[WORLD_ROOM_BRIDGE], "Bridge",
        "A narrow timber bridge bows over the stream.", "kingfisher",
        "A kingfisher snaps its beak and dives.");
    room_set_meta(&world->rooms[WORLD_ROOM_CATACOMBS], "Catacombs",
        "Cold tunnels run under the ruins.", "rat",
        "Rats skitter and squeak through the dark.");
    room_set_meta(&world->rooms[WORLD_ROOM_MEADOW], "Meadow",
        "Tall grass moves in waves around standing stones.", "hare",
        "A hare thumps through the grass nearby.");
    room_set_meta(&world->rooms[WORLD_ROOM_CANYON], "Canyon",
        "Red stone walls hold old heat and thin echoes.", "vulture",
        "A vulture croaks from the upper rim.");
    room_set_meta(&world->rooms[WORLD_ROOM_TOWER], "Tower",
        "A watchtower leans above weathered stairs.", "raven",
        "A raven taps at cracked slate.");
    room_set_meta(&world->rooms[WORLD_ROOM_ORCHARD], "Orchard",
        "Crooked fruit trees crowd a forgotten lane.", "thrush",
        "A thrush trills from the orchard rows.");
    room_set_meta(&world->rooms[WORLD_ROOM_CAVE], "Cave",
        "A limestone cave breathes cold air from below.", "bat",
        "Bats chirr overhead in brief bursts.");

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

    for (i = 0; i < 7; ++i) {
        int j;
        j = rand() % 7;
        {
            int t = wilds[i];
            wilds[i] = wilds[j];
            wilds[j] = t;
        }
    }
    for (i = 0; i < 5; ++i) {
        int j;
        j = rand() % 5;
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

    for (i = 0; i < path_len - 1; ++i) {
        dir = random_slot(&world->rooms[base_path[i]]);
        if (dir < 0) dir = DIR_NORTH;
        world_link2(world, base_path[i], base_path[i + 1], dir);
    }

    for (i = 2; i < 7; ++i) {
        int anchor;
        anchor = base_path[rand() % path_len];
        dir = random_slot(&world->rooms[anchor]);
        if (dir >= 0) {
            world_link2(world, anchor, wilds[i], dir);
        }
    }

    for (i = 3; i < 5; ++i) {
        int anchor;
        anchor = ruins[rand() % 3];
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

void world_step(world, tick)
struct World *world;
unsigned long tick;
{
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
