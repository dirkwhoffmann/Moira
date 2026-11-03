#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
typedef uint8_t uae_u8; typedef int8_t uae_s8;
typedef uint16_t uae_u16; typedef int16_t uae_s16;
typedef uint32_t uae_u32; typedef int32_t uae_s32;
typedef uint64_t uae_u64; typedef int64_t uae_s64;
typedef char TCHAR;
#define _T(x) x
#define _tcslen strlen
#define _sntprintf snprintf
#define STATIC_INLINE static inline
#define REGPARAM
#define REGPARAM2
#define REGPARAM3
typedef uintptr_t uaecptr;
