/*============================================================================

Accessors for the thread-local state of SoftFloat (Moira extension).

SoftFloat keeps its rounding mode, exception flags and other state in
thread-local variables that are defined in C. C++ code must not declare and
use such variables directly, because the C++ ABI accesses 'extern thread_local'
variables through compiler-generated wrapper functions that don't exist for
variables defined in C. These functions provide a safe interface.

=============================================================================*/

#ifndef moira_env_h
#define moira_env_h 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint_fast8_t moira_sf_getRoundingMode(void);
void moira_sf_setRoundingMode(uint_fast8_t mode);

uint_fast8_t moira_sf_getPrecision(void);
void moira_sf_setPrecision(uint_fast8_t precision);

uint_fast8_t moira_sf_getFlags(void);
void moira_sf_setFlags(uint_fast8_t flags);

#ifdef __cplusplus
}
#endif

#endif
