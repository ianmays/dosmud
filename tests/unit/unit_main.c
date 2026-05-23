#include "greatest.h"
#include "grendr.h"
#include <stdio.h>
#include <string.h>

GREATEST_MAIN_DEFS();

SUITE_EXTERN(items);
SUITE_EXTERN(command);
SUITE_EXTERN(harness);
SUITE_EXTERN(invent);
SUITE_EXTERN(combat);
SUITE_EXTERN(gprog);
SUITE_EXTERN(genc);
SUITE_EXTERN(dialogue);
SUITE_EXTERN(wanderer);
SUITE_EXTERN(gatmos);
SUITE_EXTERN(fmt);
SUITE_EXTERN(world);
SUITE_EXTERN(game);
SUITE_EXTERN(testharn);

#define UNIT_VERBOSE_GREATEST 1
#define UNIT_VERBOSE_GAMEPLAY 2

static void unit_test_usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "  (default)            final summary only; gameplay render suppressed\n"
        "  --verbose, -v        greatest suite/test progress (render still off)\n"
        "  --verbose-gameplay   greatest progress and gameplay render output\n"
        "  -h, --help           print this help\n",
        prog);
}

static void unit_test_apply_verbosity(int level)
{
    render_set_suppress(1);
    if (level >= UNIT_VERBOSE_GREATEST) {
        greatest_set_quiet(0);
        greatest_set_verbosity(1);
    }
    if (level >= UNIT_VERBOSE_GAMEPLAY) {
        render_set_suppress(0);
    }
}

static int unit_test_parse_args(int argc, char **argv, int *level_out)
{
    int i;
    int level = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            if (level < UNIT_VERBOSE_GREATEST) {
                level = UNIT_VERBOSE_GREATEST;
            }
        } else if (strcmp(argv[i], "--verbose-gameplay") == 0) {
            level = UNIT_VERBOSE_GAMEPLAY;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            unit_test_usage(argv[0]);
            return 1;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            unit_test_usage(argv[0]);
            return -1;
        }
    }
    *level_out = level;
    return 0;
}

int main(int argc, char **argv)
{
    int arg_res;
    int level;

    GREATEST_INIT();
    arg_res = unit_test_parse_args(argc, argv, &level);
    if (arg_res != 0) {
        return arg_res < 0 ? 1 : 0;
    }
    unit_test_apply_verbosity(level);

    RUN_SUITE(items);
    RUN_SUITE(command);
    RUN_SUITE(harness);
    RUN_SUITE(invent);
    RUN_SUITE(combat);
    RUN_SUITE(gprog);
    RUN_SUITE(genc);
    RUN_SUITE(dialogue);
    RUN_SUITE(wanderer);
    RUN_SUITE(gatmos);
    RUN_SUITE(fmt);
    RUN_SUITE(world);
    RUN_SUITE(game);
    RUN_SUITE(testharn);

    GREATEST_PRINT_REPORT();
    return greatest_all_passed() ? 0 : 1;
}
