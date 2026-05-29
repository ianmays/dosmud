#ifndef DIALOGUE_H
#define DIALOGUE_H

struct GameState;
struct GameOutput;

/*
 * Dialogue helpers dispatch fixed NPC branches and one-off room hints.
 */

/* NPC id for look HUD in the current room; 0 if none. */
int npc_in_room(int room_id);

void frog_dialogue_intro(struct GameOutput *out);
void frog_dialogue_branch(int choice, struct GameOutput *out);
int dialogue_cmd_talk(struct GameState *game, struct GameOutput *out);
int dialogue_cmd_reply(struct GameState *game, int choice, struct GameOutput *out);

#endif
