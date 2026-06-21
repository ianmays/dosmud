/*
 * buildid owns read-only build identity accessors for shell (--version) and
 * gameplay (version command). Values come from version.h and txtres formatting;
 * callers must not mutate game state or RNG when reading these strings.
 */
#include "buildid.h"
#include "txtres.h"
#include "version.h"

const char *build_version_line(void)
{
    return TXT_MAIN_VERSION_FMT;
}
