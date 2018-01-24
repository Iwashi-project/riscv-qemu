/*
 * RISC-V FPU Emulation Helpers for QEMU.
 *
 * Author: Sagar Karandikar, sagark@eecs.berkeley.edu
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include <stdlib.h>
#include "cpu.h"
#include "qemu/host-utils.h"
#include "exec/helper-proto.h"

#ifndef CONFIG_USER_ONLY
#define require_fp if (!(env->mstatus & MSTATUS_FS)) { \
    helper_raise_exception(env, RISCV_EXCP_ILLEGAL_INST); \
}
#else
#define require_fp /* nop */
#endif

/* convert RISC-V rounding mode to IEEE library numbers */
static const unsigned int ieee_rm[] = {
    float_round_nearest_even,
    float_round_to_zero,
    float_round_down,
    float_round_up,
    float_round_ties_away
};

static inline void set_fp_round_mode(CPURISCVState *env, uint64_t rm)
{
    if (rm == 7) {
        rm = env->frm;
    } else if (rm > 4) {
        helper_raise_exception(env, RISCV_EXCP_ILLEGAL_INST);
        g_assert_not_reached();
    }
    set_float_rounding_mode(ieee_rm[rm], &env->fp_status);
}

/* convert softfloat library flag numbers to RISC-V */
static unsigned int softfloat_flags_to_riscv(unsigned int flags)
{
    int rv_flags = 0;
    rv_flags |= (flags & float_flag_inexact) ? FPEXC_NX : 0;
    rv_flags |= (flags & float_flag_underflow) ? FPEXC_UF : 0;
    rv_flags |= (flags & float_flag_overflow) ? FPEXC_OF : 0;
    rv_flags |= (flags & float_flag_divbyzero) ? FPEXC_DZ : 0;
    rv_flags |= (flags & float_flag_invalid) ? FPEXC_NV : 0;
    return rv_flags;
}

static inline void set_fp_exceptions(CPURISCVState *env)
{
    int flags = get_float_exception_flags(&env->fp_status);
    if (flags) {
        set_float_exception_flags(0, &env->fp_status);
        env->fflags |= softfloat_flags_to_riscv(flags);
    }
}

uint64_t helper_fmadd_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                        uint64_t frs3, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_muladd(frs1, frs2, frs3, 0, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmadd_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                        uint64_t frs3, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_muladd(frs1, frs2, frs3, 0, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmsub_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                        uint64_t frs3, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_muladd(frs1, frs2, frs3 ^ (uint32_t)INT32_MIN, 0,
                          &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmsub_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                        uint64_t frs3, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_muladd(frs1, frs2, frs3 ^ (uint64_t)INT64_MIN, 0,
                          &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fnmsub_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                         uint64_t frs3, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_muladd(frs1 ^ (uint32_t)INT32_MIN, frs2, frs3, 0,
                          &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fnmsub_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                         uint64_t frs3, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_muladd(frs1 ^ (uint64_t)INT64_MIN, frs2, frs3, 0,
                          &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fnmadd_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                         uint64_t frs3, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_muladd(frs1 ^ (uint32_t)INT32_MIN, frs2,
                          frs3 ^ (uint32_t)INT32_MIN, 0, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fnmadd_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                         uint64_t frs3, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_muladd(frs1 ^ (uint64_t)INT64_MIN, frs2,
                          frs3 ^ (uint64_t)INT64_MIN, 0, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fadd_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                       uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_add(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fsub_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                       uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_sub(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmul_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                       uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_mul(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fdiv_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                       uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_div(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmin_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float32_minnum(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmax_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float32_maxnum(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fsqrt_s(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_sqrt(frs1, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_fle_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float32_le(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_flt_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float32_lt(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_feq_s(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float32_eq_quiet(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_fcvt_w_s(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_to_int32(frs1, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_fcvt_wu_s(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = (int32_t)float32_to_uint32(frs1, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

#if defined(TARGET_RISCV64)
uint64_t helper_fcvt_l_s(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_to_int64(frs1, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fcvt_lu_s(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float32_to_uint64(frs1, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}
#endif

uint64_t helper_fcvt_s_w(CPURISCVState *env, target_ulong rs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    rs1 = int32_to_float32((int32_t)rs1, &env->fp_status);
    set_fp_exceptions(env);
    return rs1;
}

uint64_t helper_fcvt_s_wu(CPURISCVState *env, target_ulong rs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    rs1 = uint32_to_float32((uint32_t)rs1, &env->fp_status);
    set_fp_exceptions(env);
    return rs1;
}

#if defined(TARGET_RISCV64)
uint64_t helper_fcvt_s_l(CPURISCVState *env, uint64_t rs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    rs1 = int64_to_float32(rs1, &env->fp_status);
    set_fp_exceptions(env);
    return rs1;
}

uint64_t helper_fcvt_s_lu(CPURISCVState *env, uint64_t rs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    rs1 = uint64_to_float32(rs1, &env->fp_status);
    set_fp_exceptions(env);
    return rs1;
}
#endif

/* adapted from spike */
#define isNaNF32UI(ui) (0xFF000000 < (uint32_t)((uint32_t)ui << 1))
#define signF32UI(a) ((bool)((uint32_t)a >> 31))
#define expF32UI(a) ((int16_t)(a >> 23) & 0xFF)
#define fracF32UI(a) (a & 0x007FFFFF)

static target_ulong float32_classify(uint32_t uiA, float_status *status)
{
    bool infOrNaN = expF32UI(uiA) == 0xFF;
    bool subnormalOrZero = expF32UI(uiA) == 0;
    bool sign = signF32UI(uiA);
    bool fracZero = fracF32UI(uiA) == 0;
    bool isNaN = isNaNF32UI(uiA);
    bool isSNaN = float32_is_signaling_nan(uiA, status);

    return
        (sign && infOrNaN && fracZero)           << 0 |
        (sign && !infOrNaN && !subnormalOrZero)  << 1 |
        (sign && subnormalOrZero && !fracZero)   << 2 |
        (sign && subnormalOrZero && fracZero)    << 3 |
        (!sign && infOrNaN && fracZero)          << 7 |
        (!sign && !infOrNaN && !subnormalOrZero) << 6 |
        (!sign && subnormalOrZero && !fracZero)  << 5 |
        (!sign && subnormalOrZero && fracZero)   << 4 |
        (isNaN && isSNaN)                        << 8 |
        (isNaN && !isSNaN)                       << 9;
}

target_ulong helper_fclass_s(CPURISCVState *env, uint64_t frs1)
{
    require_fp;
    frs1 = float32_classify(frs1, &env->fp_status);
    return frs1;
}

uint64_t helper_fadd_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                       uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_add(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fsub_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                       uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_sub(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmul_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                       uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_mul(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fdiv_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2,
                       uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_div(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmin_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float64_minnum(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fmax_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float64_maxnum(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fcvt_s_d(CPURISCVState *env, uint64_t rs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    rs1 = float64_to_float32(rs1, &env->fp_status);
    set_fp_exceptions(env);
    return float32_maybe_silence_nan(rs1, &env->fp_status);
}

uint64_t helper_fcvt_d_s(CPURISCVState *env, uint64_t rs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    rs1 = float32_to_float64(rs1, &env->fp_status);
    set_fp_exceptions(env);
    return float64_maybe_silence_nan(rs1, &env->fp_status);
}

uint64_t helper_fsqrt_d(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_sqrt(frs1, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_fle_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float64_le(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_flt_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float64_lt(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_feq_d(CPURISCVState *env, uint64_t frs1, uint64_t frs2)
{
    require_fp;
    frs1 = float64_eq_quiet(frs1, frs2, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_fcvt_w_d(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = (int64_t)((int32_t)float64_to_int32(frs1, &env->fp_status));
    set_fp_exceptions(env);
    return frs1;
}

target_ulong helper_fcvt_wu_d(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = (int64_t)((int32_t)float64_to_uint32(frs1, &env->fp_status));
    set_fp_exceptions(env);
    return frs1;
}

#if defined(TARGET_RISCV64)
uint64_t helper_fcvt_l_d(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_to_int64(frs1, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}

uint64_t helper_fcvt_lu_d(CPURISCVState *env, uint64_t frs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    frs1 = float64_to_uint64(frs1, &env->fp_status);
    set_fp_exceptions(env);
    return frs1;
}
#endif

uint64_t helper_fcvt_d_w(CPURISCVState *env, target_ulong rs1, uint64_t rm)
{
    require_fp;
    uint64_t res;
    set_fp_round_mode(env, rm);
    res = int32_to_float64((int32_t)rs1, &env->fp_status);
    set_fp_exceptions(env);
    return res;
}

uint64_t helper_fcvt_d_wu(CPURISCVState *env, target_ulong rs1, uint64_t rm)
{
    require_fp;
    uint64_t res;
    set_fp_round_mode(env, rm);
    res = uint32_to_float64((uint32_t)rs1, &env->fp_status);
    set_fp_exceptions(env);
    return res;
}

#if defined(TARGET_RISCV64)
uint64_t helper_fcvt_d_l(CPURISCVState *env, uint64_t rs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    rs1 = int64_to_float64(rs1, &env->fp_status);
    set_fp_exceptions(env);
    return rs1;
}

uint64_t helper_fcvt_d_lu(CPURISCVState *env, uint64_t rs1, uint64_t rm)
{
    require_fp;
    set_fp_round_mode(env, rm);
    rs1 = uint64_to_float64(rs1, &env->fp_status);
    set_fp_exceptions(env);
    return rs1;
}
#endif

/* adapted from spike */
#define isNaNF64UI(ui) (UINT64_C(0xFFE0000000000000) \
                       < (uint64_t)((uint64_t)ui << 1))
#define signF64UI(a) ((bool)((uint64_t) a >> 63))
#define expF64UI(a) ((int16_t)(a >> 52) & 0x7FF)
#define fracF64UI(a) (a & UINT64_C(0x000FFFFFFFFFFFFF))

static target_ulong float64_classify(uint64_t uiA, float_status *status)
{
    bool infOrNaN = expF64UI(uiA) == 0x7FF;
    bool subnormalOrZero = expF64UI(uiA) == 0;
    bool sign = signF64UI(uiA);
    bool fracZero = fracF32UI(uiA) == 0;
    bool isNaN = isNaNF64UI(uiA);
    bool isSNaN = float64_is_signaling_nan(uiA, status);

    return
        (sign && infOrNaN && fracZero)           << 0 |
        (sign && !infOrNaN && !subnormalOrZero)  << 1 |
        (sign && subnormalOrZero && !fracZero)   << 2 |
        (sign && subnormalOrZero && fracZero)    << 3 |
        (!sign && infOrNaN && fracZero)          << 7 |
        (!sign && !infOrNaN && !subnormalOrZero) << 6 |
        (!sign && subnormalOrZero && !fracZero)  << 5 |
        (!sign && subnormalOrZero && fracZero)   << 4 |
        (isNaN && isSNaN)                        << 8 |
        (isNaN && !isSNaN)                       << 9;
}

target_ulong helper_fclass_d(CPURISCVState *env, uint64_t frs1)
{
    require_fp;
    frs1 = float64_classify(frs1, &env->fp_status);
    return frs1;
}
