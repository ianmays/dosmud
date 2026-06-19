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
