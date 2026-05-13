#include "dialogue.h"
#include "grendr.h"
#include "world.h"

int npc_in_room(int room_id)
{
    if (room_id == WORLD_ROOM_TOWER) return 1;  /* watchman */
    if (room_id == WORLD_ROOM_ORCHARD) return 2;/* herbalist */
    if (room_id == WORLD_ROOM_CATACOMBS) return 3; /* archivist */
    return 0;
}

void frog_dialogue_intro(void)
{
    render_frog_dialogue_intro();
}

void frog_dialogue_branch(int choice)
{
    render_frog_dialogue_branch(choice);
}
