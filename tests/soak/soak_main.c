#include "../unit/greatest.h"
#include "grendr.h"

GREATEST_MAIN_DEFS();

SUITE_EXTERN(soak);

int main(int argc, char **argv)
{
    GREATEST_INIT();
    render_set_suppress(1);
    greatest_set_quiet(0);
    RUN_SUITE(soak);
    GREATEST_PRINT_REPORT();
    (void)argc;
    (void)argv;
    return greatest_all_passed() ? 0 : 1;
}
