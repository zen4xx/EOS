#ifndef TYPES_H
#define TYPES_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;

typedef unsigned long long uintptr_t;
typedef unsigned long long size_t;

#define low_16(x)          ((u16)((u64)(x) & 0xFFFF))
#define mid_16(x)          ((u16)(((u64)(x) >> 16) & 0xFFFF))
#define high_32(x)         ((u32)(((u64)(x) >> 32) & 0xFFFFFFFF))
#define low_32(x)          ((u32)((u64)(x) & 0xFFFFFFFF))

#define BIT(n)             (1ULL << (n))

#endif