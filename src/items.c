#include <string.h>
#include "config.h"
#include "items.h"

/*
 * Item metadata lives in one place so parsing, rendering, and combat bonuses
 * share the same names and values.
 */

int item_from_word(char *word)
{
    /* The parser only needs a narrow vocabulary, so item names stay centralized here. */
    if (strcmp(word, "berry") == 0 || strcmp(word, "berries") == 0) return ITEM_BERRY;
    if (strcmp(word, "stick") == 0) return ITEM_STICK;
    if (strcmp(word, "reed") == 0 || strcmp(word, "reeds") == 0) return ITEM_REED;
    if (strcmp(word, "stone") == 0) return ITEM_STONE;
    if (strcmp(word, "herb") == 0 || strcmp(word, "herbs") == 0) return ITEM_HERB;
    if (strcmp(word, "fish") == 0) return ITEM_FISH;
    if (strcmp(word, "torch") == 0) return ITEM_TORCH;
    if (strcmp(word, "salve") == 0) return ITEM_SALVE;
    if (strcmp(word, "spear") == 0) return ITEM_SPEAR;
    return ITEM_NONE;
}

const char *item_name(int item_id)
{
    if (item_id == ITEM_BERRY) return "berry";
    if (item_id == ITEM_STICK) return "stick";
    if (item_id == ITEM_REED) return "reed";
    if (item_id == ITEM_STONE) return "stone";
    if (item_id == ITEM_HERB) return "herb";
    if (item_id == ITEM_FISH) return "fish";
    if (item_id == ITEM_TORCH) return "torch";
    if (item_id == ITEM_SALVE) return "salve";
    if (item_id == ITEM_SPEAR) return "spear";
    return "unknown";
}

int item_is_edible(int item_id)
{
    if (item_id == ITEM_BERRY) return 1;
    if (item_id == ITEM_FISH) return 1;
    return 0;
}

int item_food_heal_amount(int item_id)
{
    if (item_id == ITEM_BERRY) return CFG_BERRY_HEAL_AMOUNT;
    if (item_id == ITEM_FISH) return CFG_FISH_HEAL_AMOUNT;
    return 0;
}

int item_is_weapon(int item_id)
{
    if (item_id == ITEM_STICK) return 1;
    if (item_id == ITEM_SPEAR) return 1;
    return 0;
}

int item_weapon_damage_bonus(int item_id)
{
    /* Weapon bonuses are kept with item metadata instead of being spread across combat code. */
    if (item_id == ITEM_STICK) return CFG_WEAPON_STICK_DAMAGE_BONUS;
    if (item_id == ITEM_SPEAR) return CFG_WEAPON_SPEAR_DAMAGE_BONUS;
    return 0;
}

int item_value(int item_id)
{
    if (item_id == ITEM_BERRY) return CFG_ITEM_VALUE_BERRY;
    if (item_id == ITEM_STICK) return CFG_ITEM_VALUE_STICK;
    if (item_id == ITEM_REED) return CFG_ITEM_VALUE_REED;
    if (item_id == ITEM_STONE) return CFG_ITEM_VALUE_STONE;
    if (item_id == ITEM_HERB) return CFG_ITEM_VALUE_HERB;
    if (item_id == ITEM_FISH) return CFG_ITEM_VALUE_FISH;
    if (item_id == ITEM_TORCH) return CFG_ITEM_VALUE_TORCH;
    if (item_id == ITEM_SALVE) return CFG_ITEM_VALUE_SALVE;
    if (item_id == ITEM_SPEAR) return CFG_ITEM_VALUE_SPEAR;
    return 0;
}
