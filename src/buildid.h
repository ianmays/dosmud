#ifndef BUILDID_H
#define BUILDID_H

/*
 * buildid.c exposes a single shared build identity string for shell and
 * in-game version output. The generated header in build/include/version.h
 * shadows include/version.h on native builds when Git metadata is available.
 */

const char *build_version_line(void);

#endif
