/*============================================================================

Platform configuration of Berkeley SoftFloat Release 3e for Moira.

This file replaces the platform-specific "platform.h" and the compiler options
that SoftFloat's build scripts normally pass on the command line (see
build/Linux-x86_64-GCC in the SoftFloat distribution). Feature macros are
defined here so that C and C++ translation units agree on them.

=============================================================================*/

#ifndef moira_softfloat_platform_h
#define moira_softfloat_platform_h 1

#include "moira_prefix.h"

// Use the 64-bit integer implementation (required for the extF80_t functions)
#ifndef SOFTFLOAT_FAST_INT64
#define SOFTFLOAT_FAST_INT64
#endif
#define SOFTFLOAT_ROUND_ODD
#define SOFTFLOAT_FAST_DIV32TO16
#define SOFTFLOAT_FAST_DIV64TO32
#define INLINE_LEVEL 5

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
// big endian: LITTLEENDIAN stays undefined
#else
#define LITTLEENDIAN 1
#endif

#define INLINE static inline

#if defined(__GNUC__) || defined(__clang__)

#define SOFTFLOAT_BUILTIN_CLZ 1
#ifdef __SIZEOF_INT128__
#define SOFTFLOAT_INTRINSIC_INT128 1
#endif
#include "opts-GCC.h"

#endif

#endif
