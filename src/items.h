#ifndef ITEMS_H
#define ITEMS_H

#define ITEM_NONE 0
#define ITEM_BERRY 1
#define ITEM_STICK 2
#define ITEM_REED 3
#define ITEM_STONE 4
#define ITEM_HERB 5
#define ITEM_FISH 6
#define ITEM_TORCH 7
#define ITEM_SALVE 8
#define ITEM_SPEAR 9

int item_from_word(char *word);
const char *item_name(int item_id);
int item_is_edible(int item_id);
int item_is_weapon(int item_id);
int item_weapon_damage_bonus(int item_id);

#endif
