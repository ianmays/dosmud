#ifndef FMT_H
#define FMT_H

struct GameState;

/* Write aggregated bag item list to buf (e.g. " berry [2], stick").
 * bag_count==0 writes "" and returns 0.
 * Returns bytes written excluding NUL, or -1 if bufsize too small. */
int fmt_inv_bag_items(const struct GameState *game, char *buf, int bufsize);

#endif
