#include "greatest.h"
#include "config.h"
#include "dialogue.h"
#include "world.h"

TEST dialogue_npc_in_room(void)
{
    ASSERT_EQ(0, npc_in_room(WORLD_ROOM_CAMP));
    ASSERT_EQ(1, npc_in_room(WORLD_ROOM_TOWER));
    ASSERT_EQ(2, npc_in_room(WORLD_ROOM_ORCHARD));
    ASSERT_EQ(3, npc_in_room(WORLD_ROOM_CATACOMBS));
    PASS();
}

TEST dialogue_frog_render_paths(void)
{
    frog_dialogue_intro();
    frog_dialogue_branch(1);
    frog_dialogue_branch(2);
    frog_dialogue_branch(3);
    PASS();
}

SUITE(dialogue) {
    RUN_TEST(dialogue_npc_in_room);
    RUN_TEST(dialogue_frog_render_paths);
}
