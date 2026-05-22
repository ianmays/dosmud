/* Ambient world feel and room item seeding (FAT 8+3: atmosphere basename too long). */

#ifndef GATMOS_H
#define GATMOS_H

struct GameState;

void seed_world_items(struct GameState *game);
void maybe_emit_animal_noise(struct GameState *game);
void maybe_emit_atmosphere(struct GameState *game);
int gatmos_cmd_inspect(struct GameState *game, int item_arg);

#endif
