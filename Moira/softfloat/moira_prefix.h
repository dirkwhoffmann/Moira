/*============================================================================

Symbol prefix for the Moira copy of SoftFloat.

SoftFloat is plain C and keeps global state (rounding mode, precision, exception
flags). To link several Moira libraries into one program, compile each of them
with a different -DMOIRA_SOFTFLOAT_PREFIX=<prefix> (e.g., moiraUAE_). All
externally visible symbols are then renamed to <prefix><name>. If the macro is
not defined, the default "moira_" names are used.

=============================================================================*/

#ifndef moira_softfloat_prefix_h
#define moira_softfloat_prefix_h 1

#ifdef MOIRA_SOFTFLOAT_PREFIX

#define MOIRA_SF_CAT_(a, b) a##b
#define MOIRA_SF_CAT(a, b) MOIRA_SF_CAT_(a, b)

#define moira_extF80_add MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_add)
#define moira_extF80_div MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_div)
#define moira_extF80_eq MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_eq)
#define moira_extF80_isSignalingNaN MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_isSignalingNaN)
#define moira_extF80_le MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_le)
#define moira_extF80_lt MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_lt)
#define moira_extF80_mul MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_mul)
#define moira_extF80_rem MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_rem)
#define moira_extF80_roundToInt MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_roundToInt)
#define moira_extF80_roundingPrecision MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_roundingPrecision)
#define moira_extF80_sqrt MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_sqrt)
#define moira_extF80_sub MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_sub)
#define moira_extF80_to_f32 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_to_f32)
#define moira_extF80_to_f64 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_to_f64)
#define moira_extF80_to_i32 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_to_i32)
#define moira_extF80_to_i64 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, extF80_to_i64)
#define moira_f32_to_extF80 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, f32_to_extF80)
#define moira_f64_to_extF80 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, f64_to_extF80)
#define moira_i32_to_extF80 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, i32_to_extF80)
#define moira_i64_to_extF80 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, i64_to_extF80)
#define moira_sf_getFlags MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, sf_getFlags)
#define moira_sf_getPrecision MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, sf_getPrecision)
#define moira_sf_getRoundingMode MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, sf_getRoundingMode)
#define moira_sf_setFlags MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, sf_setFlags)
#define moira_sf_setPrecision MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, sf_setPrecision)
#define moira_sf_setRoundingMode MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, sf_setRoundingMode)
#define moira_softfloat_add128 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_add128)
#define moira_softfloat_addMagsExtF80 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_addMagsExtF80)
#define moira_softfloat_approxRecip32_1 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_approxRecip32_1)
#define moira_softfloat_approxRecipSqrt32_1 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_approxRecipSqrt32_1)
#define moira_softfloat_approxRecipSqrt_1k0s MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_approxRecipSqrt_1k0s)
#define moira_softfloat_approxRecipSqrt_1k1s MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_approxRecipSqrt_1k1s)
#define moira_softfloat_approxRecip_1k0s MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_approxRecip_1k0s)
#define moira_softfloat_approxRecip_1k1s MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_approxRecip_1k1s)
#define moira_softfloat_commonNaNToExtF80UI MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_commonNaNToExtF80UI)
#define moira_softfloat_commonNaNToF32UI MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_commonNaNToF32UI)
#define moira_softfloat_commonNaNToF64UI MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_commonNaNToF64UI)
#define moira_softfloat_countLeadingZeros8 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_countLeadingZeros8)
#define moira_softfloat_detectTininess MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_detectTininess)
#define moira_softfloat_exceptionFlags MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_exceptionFlags)
#define moira_softfloat_extF80UIToCommonNaN MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_extF80UIToCommonNaN)
#define moira_softfloat_f32UIToCommonNaN MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_f32UIToCommonNaN)
#define moira_softfloat_f64UIToCommonNaN MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_f64UIToCommonNaN)
#define moira_softfloat_le128 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_le128)
#define moira_softfloat_lt128 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_lt128)
#define moira_softfloat_normRoundPackToExtF80 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_normRoundPackToExtF80)
#define moira_softfloat_normSubnormalExtF80Sig MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_normSubnormalExtF80Sig)
#define moira_softfloat_normSubnormalF32Sig MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_normSubnormalF32Sig)
#define moira_softfloat_normSubnormalF64Sig MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_normSubnormalF64Sig)
#define moira_softfloat_propagateNaNExtF80UI MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_propagateNaNExtF80UI)
#define moira_softfloat_raiseFlags MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_raiseFlags)
#define moira_softfloat_roundPackToExtF80 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_roundPackToExtF80)
#define moira_softfloat_roundPackToF32 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_roundPackToF32)
#define moira_softfloat_roundPackToF64 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_roundPackToF64)
#define moira_softfloat_roundToI32 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_roundToI32)
#define moira_softfloat_roundToI64 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_roundToI64)
#define moira_softfloat_roundingMode MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_roundingMode)
#define moira_softfloat_shiftRightJam128 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_shiftRightJam128)
#define moira_softfloat_shiftRightJam32 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_shiftRightJam32)
#define moira_softfloat_shiftRightJam64 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_shiftRightJam64)
#define moira_softfloat_shiftRightJam64Extra MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_shiftRightJam64Extra)
#define moira_softfloat_shortShiftLeft128 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_shortShiftLeft128)
#define moira_softfloat_shortShiftRightJam64 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_shortShiftRightJam64)
#define moira_softfloat_shortShiftRightJam64Extra MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_shortShiftRightJam64Extra)
#define moira_softfloat_sub128 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_sub128)
#define moira_softfloat_subMagsExtF80 MOIRA_SF_CAT(MOIRA_SOFTFLOAT_PREFIX, softfloat_subMagsExtF80)

#endif
#endif
