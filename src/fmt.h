#ifndef FMT_H
#define FMT_H

struct GameState;

/*
 * fmt functions build UI text into caller-owned buffers so grendr can print
 * deterministic strings without mixing gameplay logic into terminal I/O.
 */

/* Write aggregated bag item list to buf (e.g. " berry [2], stick").
 * bag_count==0 writes "" and returns 0.
 * Returns bytes written excluding NUL, or -1 if bufsize too small. */
int fmt_inv_bag_items(const struct GameState *game, char *buf, int bufsize);

/* Room ground items for look output from a caller-supplied item-slot snapshot.
 * No items writes "" and returns 0.
 * Returns bytes written excluding NUL, or -1 if bufsize too small. */
int fmt_room_ground_items(const int *room_items, char *buf, int bufsize);

/* Exploration map text (header, grid or none-explored message, legend,
 * then open exits for player's current room; label/dir order match look).
 * Returns bytes written excluding NUL, or -1 if bufsize too small. */
int fmt_exploration_map(const struct GameState *game, char *buf, int bufsize);

/* Open exits for player's current room; label/dir order match look and map footer.
 * Returns bytes written excluding NUL, or -1 if bufsize too small. */
int fmt_player_room_exits(const struct GameState *game, char *buf, int bufsize);

#endif
