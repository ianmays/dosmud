/*
 * Fetch-quest progress helpers proven by the Herbalist slice (#76, #49).
 * Short basename keeps the DOS tree within FAT 8+3 limits.
 */

#ifndef GSTORY_H
#define GSTORY_H

struct GameState;

/* Persisted fetch-quest progress (numeric values align with HerbalistStoryState). */
enum StoryProgress {
    STORY_PROGRESS_NONE = 0,
    STORY_PROGRESS_ACTIVE,
    STORY_PROGRESS_DONE
};

/* Derived scene for talk/reply copy selection. */
enum StoryFetchScene {
    STORY_FETCH_NOT_STARTED = 0,
    STORY_FETCH_ACTIVE,
    STORY_FETCH_READY,
    STORY_FETCH_DONE
};

/* Pure scene derivation; callers cast to actor-specific dialogue enums when aligned. */
enum StoryFetchScene story_fetch_scene(int progress, int has_required_item);
/* Read-only: player bag or any room slot. */
int story_world_has_item(const struct GameState *game, int item_id);
/* Place item in room_id when not already in play; spawned_out marks reachability. */
int story_seed_recoverable_item(struct GameState *game, int room_id,
    int item_id, int *spawned_out);

#endif
