#include <string.h>
#include "greatest.h"
#include "config.h"
#include "items.h"

TEST items_from_word_aliases(void)
{
    char w1[] = "berry";
    char w2[] = "berries";
    char w3[] = "stick";
    char w4[] = "reed";
    char w5[] = "reeds";
    char w6[] = "stone";
    char w7[] = "herb";
    char w8[] = "herbs";
    char w9[] = "fish";
    char w10[] = "torch";
    char w11[] = "salve";
    char w12[] = "spear";
    char w13[] = "marsh-root";
    char w14[] = "marshroot";
    char w15[] = "root";

    ASSERT_EQ(ITEM_BERRY, item_from_word(w1));
    ASSERT_EQ(ITEM_BERRY, item_from_word(w2));
    ASSERT_EQ(ITEM_STICK, item_from_word(w3));
    ASSERT_EQ(ITEM_REED, item_from_word(w4));
    ASSERT_EQ(ITEM_REED, item_from_word(w5));
    ASSERT_EQ(ITEM_STONE, item_from_word(w6));
    ASSERT_EQ(ITEM_HERB, item_from_word(w7));
    ASSERT_EQ(ITEM_HERB, item_from_word(w8));
    ASSERT_EQ(ITEM_FISH, item_from_word(w9));
    ASSERT_EQ(ITEM_TORCH, item_from_word(w10));
    ASSERT_EQ(ITEM_SALVE, item_from_word(w11));
    ASSERT_EQ(ITEM_SPEAR, item_from_word(w12));
    ASSERT_EQ(ITEM_MARSH_ROOT, item_from_word(w13));
    ASSERT_EQ(ITEM_MARSH_ROOT, item_from_word(w14));
    ASSERT_EQ(ITEM_MARSH_ROOT, item_from_word(w15));
    PASS();
}

TEST items_from_word_unknown(void)
{
    char w[] = "diamond";
    ASSERT_EQ(ITEM_NONE, item_from_word(w));
    PASS();
}

TEST items_name_and_flags(void)
{
    ASSERT_STR_EQ("berry", item_name(ITEM_BERRY));
    ASSERT_STR_EQ("stick", item_name(ITEM_STICK));
    ASSERT_STR_EQ("marsh-root", item_name(ITEM_MARSH_ROOT));
    ASSERT_STR_EQ("unknown", item_name(99));
    ASSERT_EQ(1, item_is_edible(ITEM_BERRY));
    ASSERT_EQ(1, item_is_edible(ITEM_FISH));
    ASSERT_EQ(0, item_is_edible(ITEM_STICK));
    ASSERT_EQ(1, item_is_weapon(ITEM_STICK));
    ASSERT_EQ(1, item_is_weapon(ITEM_SPEAR));
    ASSERT_EQ(0, item_is_weapon(ITEM_TORCH));
    ASSERT_EQ(CFG_WEAPON_STICK_DAMAGE_BONUS, item_weapon_damage_bonus(ITEM_STICK));
    ASSERT_EQ(CFG_WEAPON_SPEAR_DAMAGE_BONUS, item_weapon_damage_bonus(ITEM_SPEAR));
    ASSERT_EQ(0, item_weapon_damage_bonus(ITEM_BERRY));
    ASSERT_EQ(CFG_BERRY_HEAL_AMOUNT, item_food_heal_amount(ITEM_BERRY));
    ASSERT_EQ(CFG_FISH_HEAL_AMOUNT, item_food_heal_amount(ITEM_FISH));
    ASSERT_EQ(0, item_food_heal_amount(ITEM_STICK));
    PASS();
}

TEST items_retained_on_defeat_policy(void)
{
    ASSERT_EQ(1, item_is_retained_on_defeat(ITEM_MARSH_ROOT));
    ASSERT_EQ(0, item_is_retained_on_defeat(ITEM_STICK));
    ASSERT_EQ(0, item_is_retained_on_defeat(ITEM_NONE));
    PASS();
}

SUITE(items) {
    RUN_TEST(items_from_word_aliases);
    RUN_TEST(items_from_word_unknown);
    RUN_TEST(items_name_and_flags);
    RUN_TEST(items_retained_on_defeat_policy);
}
