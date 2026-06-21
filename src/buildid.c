/*
 * buildid owns read-only build identity accessors for shell (--version) and
 * gameplay (version command). Values come from version.h and txtres formatting;
 * callers must not mutate game state or RNG when reading these strings.
 */
#include "buildid.h"
#include "txtres.h"
#include "version.h"

const char *build_base_version(void)
{
    return BUILD_BASE_VERSION;
}

const char *build_version_string(void)
{
    return BUILD_VERSION_STRING;
}

const char *build_version_line(void)
{
    return TXT_MAIN_VERSION_FMT;
}
