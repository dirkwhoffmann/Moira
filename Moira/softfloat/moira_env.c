/*============================================================================

Accessors for the thread-local state of SoftFloat (Moira extension).
See moira_env.h.

=============================================================================*/

#include "platform.h"
#include "softfloat.h"
#include "moira_env.h"

uint_fast8_t moira_sf_getRoundingMode(void) { return moira_softfloat_roundingMode; }
void moira_sf_setRoundingMode(uint_fast8_t mode) { moira_softfloat_roundingMode = mode; }

uint_fast8_t moira_sf_getPrecision(void) { return moira_extF80_roundingPrecision; }
void moira_sf_setPrecision(uint_fast8_t precision) { moira_extF80_roundingPrecision = precision; }

uint_fast8_t moira_sf_getFlags(void) { return moira_softfloat_exceptionFlags; }
void moira_sf_setFlags(uint_fast8_t flags) { moira_softfloat_exceptionFlags = flags; }
