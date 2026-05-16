#ifndef BASE_H
#define BASE_H

/*
 * Portable unsigned aliases for GCC and Open Watcom (DOS + Linux test builds).
 *
 * u8  -> unsigned char   (8-bit byte storage)
 * u16 -> unsigned short  (16-bit; required sizeof == 2 on supported targets)
 * u32 -> unsigned long   (at least 32 bits; 32-bit on DOS/Open Watcom, may be
 *                          wider on LP64 Linux test builds)
 *
 * C89 does not guarantee char bit-width > 8 everywhere; we target platforms
 * where CHAR_BIT == 8, sizeof(unsigned short) == 2, and sizeof(unsigned long) >= 4.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

/* Compile-time guards for supported toolchains (negative size = error). */
typedef char base_h_u8_is_1[(sizeof(u8) == 1U) ? 1 : -1];
typedef char base_h_u16_is_2[(sizeof(u16) == 2U) ? 1 : -1];
typedef char base_h_u32_at_least_4[(sizeof(u32) >= 4U) ? 1 : -1];

#endif
