#ifndef DIALOGUE_H
#define DIALOGUE_H

struct GameState;

/* Dialogue helpers dispatch fixed NPC branches and one-off room hints. */

/* NPC id for look HUD in the current room; 0 if none. */
int npc_in_room(int room_id);

void frog_dialogue_intro(void);
void frog_dialogue_branch(int choice);
int dialogue_cmd_talk(struct GameState *game);
int dialogue_cmd_reply(struct GameState *game, int choice);

#endif
