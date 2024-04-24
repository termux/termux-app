/*
 * Copyright © 2011 SCore Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author:  Siarhei Siamashka (siarhei.siamashka@nokia.com)
 * Author:  Taekyun Kim (tkq.kim@samsung.com)
 */

/*
 * This file contains scaled bilinear scanline functions implemented
 * using older siarhei's bilinear macro template.
 *
 * << General scanline function procedures >>
 *  1. bilinear interpolate source pixels
 *  2. load mask pixels
 *  3. load destination pixels
 *  4. duplicate mask to fill whole register
 *  5. interleave source & destination pixels
 *  6. apply mask to source pixels
 *  7. combine source & destination pixels
 *  8, Deinterleave final result
 *  9. store destination pixels
 *
 * All registers with single number (i.e. src0, tmp0) are 64-bits registers.
 * Registers with double numbers(src01, dst01) are 128-bits registers.
 * All temp registers can be used freely outside the code block.
 * Assume that symbol(register .req) OUT and MASK are defined at caller of these macro blocks.
 *
 * Remarks
 *  There can be lots of pipeline stalls inside code block and between code blocks.
 *  Further optimizations will be done by new macro templates using head/tail_head/tail scheme.
 */

/* Prevent the stack from becoming executable for no reason... */
#if defined(__linux__) && defined (__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

.text
.arch armv8-a
.altmacro
.p2align 2

#include "pixman-private.h"
#include "pixman-arm-asm.h"
#include "pixman-arma64-neon-asm.h"

/*
 * Bilinear macros from pixman-arm-neon-asm.S
 */

/*
 * Bilinear scaling support code which tries to provide pixel fetching, color
 * format conversion, and interpolation as separate macros which can be used
 * as the basic building blocks for constructing bilinear scanline functions.
 */

.macro bilinear_load_8888 reg1, reg2, tmp
    asr       WTMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #2
    ld1       {\()\reg1\().2s}, [TMP1], STRIDE
    ld1       {\()\reg2\().2s}, [TMP1]
.endm

.macro bilinear_load_0565 reg1, reg2, tmp
    asr       WTMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #1
    ld1       {\()\reg2\().s}[0], [TMP1], STRIDE
    ld1       {\()\reg2\().s}[1], [TMP1]
    convert_four_0565_to_x888_packed \reg2, \reg1, \reg2, \tmp
.endm

.macro bilinear_load_and_vertical_interpolate_two_8888 \
                    acc1, acc2, reg1, reg2, reg3, reg4, tmp1, tmp2

    bilinear_load_8888 \reg1, \reg2, \tmp1
    umull     \()\acc1\().8h, \()\reg1\().8b, v28.8b
    umlal     \()\acc1\().8h, \()\reg2\().8b, v29.8b
    bilinear_load_8888 \reg3, \reg4, \tmp2
    umull     \()\acc2\().8h, \()\reg3\().8b, v28.8b
    umlal     \()\acc2\().8h, \()\reg4\().8b, v29.8b
.endm

.macro bilinear_load_and_vertical_interpolate_four_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi, \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi

    bilinear_load_and_vertical_interpolate_two_8888 \
                \xacc1, \xacc2, \xreg1, \xreg2, \xreg3, \xreg4, \xacc2lo, xacc2hi
    bilinear_load_and_vertical_interpolate_two_8888 \
                \yacc1, \yacc2, \yreg1, \yreg2, \yreg3, \yreg4, \yacc2lo, \yacc2hi
.endm

.macro vzip reg1, reg2
    zip1      v24.8b, \reg1, \reg2
    zip2      \reg2,  \reg1, \reg2
    mov       \reg1,  v24.8b
.endm

.macro vuzp reg1, reg2
    uzp1     v24.8b, \reg1, \reg2
    uzp2     \reg2,  \reg1, \reg2
    mov      \reg1,  v24.8b
.endm

.macro bilinear_load_and_vertical_interpolate_two_0565 \
                acc1, acc2, reg1, reg2, reg3, reg4, acc2lo, acc2hi
    asr       WTMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #1
    asr       WTMP2, X, #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, lsl #1
    ld1       {\()\acc2\().s}[0], [TMP1], STRIDE
    ld1       {\()\acc2\().s}[2], [TMP2], STRIDE
    ld1       {\()\acc2\().s}[1], [TMP1]
    ld1       {\()\acc2\().s}[3], [TMP2]
    convert_0565_to_x888 \acc2, \reg3, \reg2, \reg1
    vzip      \()\reg1\().8b, \()\reg3\().8b
    vzip      \()\reg2\().8b, \()\reg4\().8b
    vzip      \()\reg3\().8b, \()\reg4\().8b
    vzip      \()\reg1\().8b, \()\reg2\().8b
    umull     \()\acc1\().8h, \()\reg1\().8b, v28.8b
    umlal     \()\acc1\().8h, \()\reg2\().8b, v29.8b
    umull     \()\acc2\().8h, \()\reg3\().8b, v28.8b
    umlal     \()\acc2\().8h, \()\reg4\().8b, v29.8b
.endm

.macro bilinear_load_and_vertical_interpolate_four_0565 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi, \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi

    asr       WTMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #1
    asr       WTMP2, X, #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, lsl #1
    ld1       {\()\xacc2\().s}[0], [TMP1], STRIDE
    ld1       {\()\xacc2\().s}[2], [TMP2], STRIDE
    ld1       {\()\xacc2\().s}[1], [TMP1]
    ld1       {\()\xacc2\().s}[3], [TMP2]
    convert_0565_to_x888 \xacc2, \xreg3, \xreg2, \xreg1
    asr       WTMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #1
    asr       WTMP2, X, #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, lsl #1
    ld1       {\()\yacc2\().s}[0], [TMP1], STRIDE
    vzip      \()\xreg1\().8b, \()\xreg3\().8b
    ld1       {\()\yacc2\().s}[2], [TMP2], STRIDE
    vzip      \()\xreg2\().8b, \()\xreg4\().8b
    ld1       {\()\yacc2\().s}[1], [TMP1]
    vzip      \()\xreg3\().8b, \()\xreg4\().8b
    ld1       {\()\yacc2\().s}[3], [TMP2]
    vzip      \()\xreg1\().8b, \()\xreg2\().8b
    convert_0565_to_x888 \yacc2, \yreg3, \yreg2, \yreg1
    umull     \()\xacc1\().8h, \()\xreg1\().8b, v28.8b
    vzip      \()\yreg1\().8b, \()\yreg3\().8b
    umlal     \()\xacc1\().8h, \()\xreg2\().8b, v29.8b
    vzip      \()\yreg2\().8b, \()\yreg4\().8b
    umull     \()\xacc2\().8h, \()\xreg3\().8b, v28.8b
    vzip      \()\yreg3\().8b, \()\yreg4\().8b
    umlal     \()\xacc2\().8h, \()\xreg4\().8b, v29.8b
    vzip      \()\yreg1\().8b, \()\yreg2\().8b
    umull     \()\yacc1\().8h, \()\yreg1\().8b, v28.8b
    umlal     \()\yacc1\().8h, \()\yreg2\().8b, v29.8b
    umull     \()\yacc2\().8h, \()\yreg3\().8b, v28.8b
    umlal     \()\yacc2\().8h, \()\yreg4\().8b, v29.8b
.endm

.macro bilinear_store_8888 numpix, tmp1, tmp2
.if \numpix == 4
    st1       {v0.2s, v1.2s}, [OUT], #16
.elseif \numpix == 2
    st1       {v0.2s}, [OUT], #8
.elseif \numpix == 1
    st1       {v0.s}[0], [OUT], #4
.else
    .error bilinear_store_8888 \numpix is unsupported
.endif
.endm

.macro bilinear_store_0565 numpix, tmp1, tmp2
    vuzp    v0.8b, v1.8b
    vuzp    v2.8b, v3.8b
    vuzp    v1.8b, v3.8b
    vuzp    v0.8b, v2.8b
    convert_8888_to_0565 v2, v1, v0, v1, \tmp1, \tmp2
.if \numpix == 4
    st1       {v1.4h}, [OUT], #8
.elseif \numpix == 2
    st1       {v1.s}[0], [OUT], #4
.elseif \numpix == 1
    st1       {v1.h}[0], [OUT], #2
.else
    .error bilinear_store_0565 \numpix is unsupported
.endif
.endm


/*
 * Macros for loading mask pixels into register 'mask'.
 * dup must be done in somewhere else.
 */
.macro bilinear_load_mask_x numpix, mask
.endm

.macro bilinear_load_mask_8 numpix, mask
.if \numpix == 4
    ld1         {\()\mask\().s}[0], [MASK], #4
.elseif \numpix == 2
    ld1         {\()\mask\().h}[0], [MASK], #2
.elseif \numpix == 1
    ld1         {\()\mask\().b}[0], [MASK], #1
.else
    .error bilinear_load_mask_8 \numpix is unsupported
.endif
    prfum       PREFETCH_MODE, [MASK, #(prefetch_offset)]
.endm

.macro bilinear_load_mask mask_fmt, numpix, mask
    bilinear_load_mask_\mask_fmt \numpix, \mask
.endm


/*
 * Macros for loading destination pixels into register 'dst0' and 'dst1'.
 * Interleave should be done somewhere else.
 */
.macro bilinear_load_dst_0565_src numpix, dst0, dst1, dst01
.endm

.macro bilinear_load_dst_8888_src numpix, dst0, dst1, dst01
.endm

.macro bilinear_load_dst_8888 numpix, dst0, dst1, dst01
.if \numpix == 4
    ld1         {\()\dst0\().2s, \()\dst1\().2s}, [OUT]
.elseif \numpix == 2
    ld1         {\()\dst0\().2s}, [OUT]
.elseif \numpix == 1
    ld1         {\()\dst0\().s}[0], [OUT]
.else
    .error bilinear_load_dst_8888 \numpix is unsupported
.endif
    mov         \()\dst01\().d[0], \()\dst0\().d[0]
    mov         \()\dst01\().d[1], \()\dst1\().d[0]
    prfm        PREFETCH_MODE, [OUT, #(prefetch_offset * 4)]
.endm

.macro bilinear_load_dst_8888_over numpix, dst0, dst1, dst01
    bilinear_load_dst_8888 \numpix, \dst0, \dst1, \dst01
.endm

.macro bilinear_load_dst_8888_add numpix, dst0, dst1, dst01
    bilinear_load_dst_8888 \numpix, \dst0, \dst1, \dst01
.endm

.macro bilinear_load_dst dst_fmt, op, numpix, dst0, dst1, dst01
    bilinear_load_dst_\()\dst_fmt\()_\()\op \numpix, \dst0, \dst1, \dst01
.endm

/*
 * Macros for duplicating partially loaded mask to fill entire register.
 * We will apply mask to interleaved source pixels, that is
 *  (r0, r1, r2, r3, g0, g1, g2, g3) x (m0, m1, m2, m3, m0, m1, m2, m3)
 *  (b0, b1, b2, b3, a0, a1, a2, a3) x (m0, m1, m2, m3, m0, m1, m2, m3)
 * So, we need to duplicate loaded mask into whole register.
 *
 * For two pixel case
 *  (r0, r1, x, x, g0, g1, x, x) x (m0, m1, m0, m1, m0, m1, m0, m1)
 *  (b0, b1, x, x, a0, a1, x, x) x (m0, m1, m0, m1, m0, m1, m0, m1)
 * We can do some optimizations for this including last pixel cases.
 */
.macro bilinear_duplicate_mask_x numpix, mask
.endm

.macro bilinear_duplicate_mask_8 numpix, mask
.if \numpix == 4
    dup         \()\mask\().2s, \()\mask\().s[0]
.elseif \numpix == 2
    dup         \()\mask\().4h, \()\mask\().h[0]
.elseif \numpix == 1
    dup         \()\mask\().8b, \()\mask\().b[0]
.else
    .error bilinear_duplicate_\mask_8 is unsupported
.endif
.endm

.macro bilinear_duplicate_mask mask_fmt, numpix, mask
    bilinear_duplicate_mask_\()\mask_fmt \numpix, \mask
.endm

/*
 * Macros for interleaving src and dst pixels to rrrr gggg bbbb aaaa form.
 * Interleave should be done when maks is enabled or operator is 'over'.
 */
.macro bilinear_interleave src0, src1, src01, dst0, dst1, dst01
    vuzp       \()\src0\().8b, \()\src1\().8b
    vuzp       \()\dst0\().8b, \()\dst1\().8b
    vuzp       \()\src0\().8b, \()\src1\().8b
    vuzp       \()\dst0\().8b, \()\dst1\().8b
    mov        \()\src01\().d[1], \()\src1\().d[0]
    mov        \()\src01\().d[0], \()\src0\().d[0]
    mov        \()\dst01\().d[1], \()\dst1\().d[0]
    mov        \()\dst01\().d[0], \()\dst0\().d[0]
.endm

.macro bilinear_interleave_src_dst_x_src \
                numpix, src0, src1, src01, dst0, dst1, dst01
.endm

.macro bilinear_interleave_src_dst_x_over \
                numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave \src0, \src1, \src01, \dst0, \dst1, \dst01
.endm

.macro bilinear_interleave_src_dst_x_add \
                numpix, src0, src1, src01, dst0, dst1, dst01
                
    bilinear_interleave \src0, \src1, \src01, \dst0, \dst1, \dst01
.endm

.macro bilinear_interleave_src_dst_8_src \
                numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave \src0, \src1, \src01, \dst0, \dst1, \dst01
.endm

.macro bilinear_interleave_src_dst_8_over \
                numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave \src0, \src1, \src01, \dst0, \dst1, \dst01
.endm

.macro bilinear_interleave_src_dst_8_add \
                numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave \src0, \src1, \src01, \dst0, \dst1, \dst01
.endm

.macro bilinear_interleave_src_dst \
                mask_fmt, op, numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave_src_dst_\()\mask_fmt\()_\()\op \
                \numpix, \src0, \src1, \src01, \dst0, \dst1, \dst01
.endm


/*
 * Macros for applying masks to src pixels. (see combine_mask_u() function)
 * src, dst should be in interleaved form.
 * mask register should be in form (m0, m1, m2, m3).
 */
.macro bilinear_apply_mask_to_src_x \
                numpix, src0, src1, src01, mask, \
                tmp01, tmp23, tmp45, tmp67
.endm

.macro bilinear_apply_mask_to_src_8 \
                numpix, src0, src1, src01, mask, \
                tmp01, tmp23, tmp45, tmp67

    umull           \()\tmp01\().8h, \()\src0\().8b, \()\mask\().8b
    umull           \()\tmp23\().8h, \()\src1\().8b, \()\mask\().8b
    /* bubbles */
    urshr           \()\tmp45\().8h, \()\tmp01\().8h, #8
    urshr           \()\tmp67\().8h, \()\tmp23\().8h, #8
    /* bubbles */
    raddhn          \()\src0\().8b, \()\tmp45\().8h, \()\tmp01\().8h
    raddhn          \()\src1\().8b, \()\tmp67\().8h, \()\tmp23\().8h
    mov             \()\src01\().d[0], \()\src0\().d[0]
    mov             \()\src01\().d[1], \()\src1\().d[0]
.endm

.macro bilinear_apply_mask_to_src \
                mask_fmt, numpix, src0, src1, src01, mask, \
                tmp01, tmp23, tmp45, tmp67

    bilinear_apply_mask_to_src_\()\mask_fmt \
                \numpix, \src0, \src1, \src01, \mask, \
                \tmp01, \tmp23, \tmp45, \tmp67
.endm


/*
 * Macros for combining src and destination pixels.
 * Interleave or not is depending on operator 'op'.
 */
.macro bilinear_combine_src \
                numpix, src0, src1, src01, dst0, dst1, dst01, \
                tmp01, tmp23, tmp45, tmp67, tmp8
.endm

.macro bilinear_combine_over \
                numpix, src0, src1, src01, dst0, dst1, dst01, \
                tmp01, tmp23, tmp45, tmp67, tmp8

    dup         \()\tmp8\().2s, \()\src1\().s[1]
    /* bubbles */
    mvn         \()\tmp8\().8b, \()\tmp8\().8b
    /* bubbles */
    umull       \()\tmp01\().8h, \()\dst0\().8b, \()\tmp8\().8b
    /* bubbles */
    umull       \()\tmp23\().8h, \()\dst1\().8b, \()\tmp8\().8b
    /* bubbles */
    urshr       \()\tmp45\().8h, \()\tmp01\().8h, #8
    urshr       \()\tmp67\().8h, \()\tmp23\().8h, #8
    /* bubbles */
    raddhn      \()\dst0\().8b, \()\tmp45\().8h, \()\tmp01\().8h
    raddhn      \()\dst1\().8b, \()\tmp67\().8h, \()\tmp23\().8h
    mov         \()\dst01\().d[0], \()\dst0\().d[0]
    mov         \()\dst01\().d[1], \()\dst1\().d[0]
    /* bubbles */
    uqadd       \()\src0\().8b, \()\dst0\().8b, \()\src0\().8b
    uqadd       \()\src1\().8b, \()\dst1\().8b, \()\src1\().8b
    mov         \()\src01\().d[0], \()\src0\().d[0]
    mov         \()\src01\().d[1], \()\src1\().d[0]
.endm

.macro bilinear_combine_add \
                numpix, src0, src1, src01, dst0, dst1, dst01, \
                tmp01, tmp23, tmp45, tmp67, tmp8

    uqadd       \()\src0\().8b, \()\dst0\().8b, \()\src0\().8b
    uqadd       \()\src1\().8b, \()\dst1\().8b, \()\src1\().8b
    mov         \()\src01\().d[0], \()\src0\().d[0]
    mov         \()\src01\().d[1], \()\src1\().d[0]
.endm

.macro bilinear_combine \
                op, numpix, src0, src1, src01, dst0, dst1, dst01, \
                tmp01, tmp23, tmp45, tmp67, tmp8

    bilinear_combine_\()\op \
                \numpix, \src0, \src1, \src01, \dst0, \dst1, \dst01, \
                \tmp01, \tmp23, \tmp45, \tmp67, \tmp8
.endm

/*
 * Macros for final deinterleaving of destination pixels if needed.
 */
.macro bilinear_deinterleave numpix, dst0, dst1, dst01
    vuzp       \()\dst0\().8b, \()\dst1\().8b
    /* bubbles */
    vuzp       \()\dst0\().8b, \()\dst1\().8b
    mov        \()\dst01\().d[0], \()\dst0\().d[0]
    mov        \()\dst01\().d[1], \()\dst1\().d[0]
.endm

.macro bilinear_deinterleave_dst_x_src numpix, dst0, dst1, dst01
.endm

.macro bilinear_deinterleave_dst_x_over numpix, dst0, dst1, dst01
    bilinear_deinterleave \numpix, \dst0, \dst1, \dst01
.endm

.macro bilinear_deinterleave_dst_x_add numpix, dst0, dst1, dst01
    bilinear_deinterleave \numpix, \dst0, \dst1, \dst01
.endm

.macro bilinear_deinterleave_dst_8_src numpix, dst0, dst1, dst01
    bilinear_deinterleave \numpix, \dst0, \dst1, \dst01
.endm

.macro bilinear_deinterleave_dst_8_over numpix, dst0, dst1, dst01
    bilinear_deinterleave \numpix, \dst0, \dst1, \dst01
.endm

.macro bilinear_deinterleave_dst_8_add numpix, dst0, dst1, dst01
    bilinear_deinterleave \numpix, \dst0, \dst1, \dst01
.endm

.macro bilinear_deinterleave_dst mask_fmt, op, numpix, dst0, dst1, dst01
    bilinear_deinterleave_dst_\()\mask_fmt\()_\()\op \numpix, \dst0, \dst1, \dst01
.endm


.macro bilinear_interpolate_last_pixel src_fmt, mask_fmt, dst_fmt, op
    bilinear_load_\()\src_fmt v0, v1, v2
    bilinear_load_mask \mask_fmt, 1, v4
    bilinear_load_dst \dst_fmt, \op, 1, v18, v19, v9
    umull     v2.8h, v0.8b, v28.8b
    umlal     v2.8h, v1.8b, v29.8b
    /* 5 cycles bubble */
    ushll     v0.4s, v2.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v0.4s, v2.4h, v15.h[0]
    umlal2    v0.4s, v2.8h, v15.h[0]
    /* 5 cycles bubble */
    bilinear_duplicate_mask \mask_fmt, 1, v4
    shrn      v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    /* 3 cycles bubble */
    xtn       v0.8b, v0.8h
    /* 1 cycle bubble */
    bilinear_interleave_src_dst \
                \mask_fmt, \op, 1, v0, v1, v0, v18, v19, v9
    bilinear_apply_mask_to_src \
                \mask_fmt, 1, v0, v1, v0, v4, \
                v3, v8, v10, v11
    bilinear_combine \
                \op, 1, v0, v1, v0, v18, v19, v9, \
                v3, v8, v10, v11, v5
    bilinear_deinterleave_dst \mask_fmt, \op, 1, v0, v1, v0
    bilinear_store_\()\dst_fmt 1, v17, v18
.endm

.macro bilinear_interpolate_two_pixels src_fmt, mask_fmt, dst_fmt, op
    bilinear_load_and_vertical_interpolate_two_\()\src_fmt \
                v1, v11, v18, v19, v20, v21, v22, v23
    bilinear_load_mask \mask_fmt, 2, v4
    bilinear_load_dst \dst_fmt, \op, 2, v18, v19, v9
    ushll     v0.4s, v1.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v0.4s, v1.4h, v15.h[0]
    umlal2    v0.4s, v1.8h, v15.h[0]
    ushll     v10.4s, v11.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v10.4s, v11.4h, v15.h[4]
    umlal2    v10.4s, v11.8h, v15.h[4]
    shrn      v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn2     v0.8h, v10.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    bilinear_duplicate_mask \mask_fmt, 2, v4
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    add       v12.8h, v12.8h, v13.8h
    xtn       v0.8b, v0.8h
    bilinear_interleave_src_dst \
                \mask_fmt, \op, 2, v0, v1, v0, v18, v19, v9
    bilinear_apply_mask_to_src \
                \mask_fmt, 2, v0, v1, v0, v4, \
                v3, v8, v10, v11
    bilinear_combine \
                \op, 2, v0, v1, v0, v18, v19, v9, \
                v3, v8, v10, v11, v5
    bilinear_deinterleave_dst \mask_fmt, \op, 2, v0, v1, v0
    bilinear_store_\()\dst_fmt 2, v16, v17
.endm

.macro bilinear_interpolate_four_pixels src_fmt, mask_fmt, dst_fmt, op
    bilinear_load_and_vertical_interpolate_four_\()\src_fmt \
                v1, v11, v4,  v5,  v6,  v7,  v22, v23, \
                v3, v9,  v16, v17, v20, v21, v18, v19
    prfm      PREFETCH_MODE, [TMP1, PF_OFFS]
    sub       TMP1, TMP1, STRIDE
    prfm      PREFETCH_MODE, [TMP1, PF_OFFS]
    ushll     v0.4s, v1.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v0.4s, v1.4h, v15.h[0]
    umlal2    v0.4s, v1.8h, v15.h[0]
    ushll     v10.4s, v11.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v10.4s, v11.4h, v15.h[4]
    umlal2    v10.4s, v11.8h, v15.h[4]
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    ushll     v2.4s, v3.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v2.4s, v3.4h, v15.h[0]
    umlal2    v2.4s, v3.8h, v15.h[0]
    ushll     v8.4s, v9.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v8.4s, v9.4h, v15.h[4]
    umlal2    v8.4s, v9.8h, v15.h[4]
    add       v12.8h, v12.8h, v13.8h
    shrn      v0.4h,  v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn2     v0.8h, v10.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn      v2.4h,  v2.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn2     v2.8h,  v8.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    bilinear_load_mask \mask_fmt, 4, v4
    bilinear_duplicate_mask \mask_fmt, 4, v4
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    xtn       v0.8b, v0.8h
    xtn       v1.8b, v2.8h
    add       v12.8h, v12.8h, v13.8h
    bilinear_load_dst \dst_fmt, \op, 4, v2, v3, v21
    bilinear_interleave_src_dst \
                \mask_fmt, \op, 4, v0, v1, v0, v2, v3, v11
    bilinear_apply_mask_to_src \
                \mask_fmt, 4, v0, v1, v0, v4, \
                v6, v8, v9, v10
    bilinear_combine \
                \op, 4, v0, v1, v0, v2, v3, v1, \
                v6, v8, v9, v10, v23
    bilinear_deinterleave_dst \mask_fmt, \op, 4, v0, v1, v0
    bilinear_store_\()\dst_fmt 4, v6, v7
.endm

.set BILINEAR_FLAG_USE_MASK,        1
.set BILINEAR_FLAG_USE_ALL_NEON_REGS,    2

/*
 * Main template macro for generating NEON optimized bilinear scanline functions.
 *
 * Bilinear scanline generator macro take folling arguments:
 *  fname            - name of the function to generate
 *  src_fmt            - source color format (8888 or 0565)
 *  dst_fmt            - destination color format (8888 or 0565)
 *  src/dst_bpp_shift        - (1 << bpp_shift) is the size of src/dst pixel in bytes
 *  process_last_pixel        - code block that interpolate one pixel and does not
 *                  update horizontal weight
 *  process_two_pixels        - code block that interpolate two pixels and update
 *                  horizontal weight
 *  process_four_pixels        - code block that interpolate four pixels and update
 *                  horizontal weight
 *  process_pixblock_head    - head part of middle loop
 *  process_pixblock_tail    - tail part of middle loop
 *  process_pixblock_tail_head    - tail_head of middle loop
 *  pixblock_size        - number of pixels processed in a single middle loop
 *  prefetch_distance        - prefetch in the source image by that many pixels ahead
 */

.macro generate_bilinear_scanline_func \
    fname, \
    src_fmt, dst_fmt, src_bpp_shift, dst_bpp_shift, \
    bilinear_process_last_pixel, \
    bilinear_process_two_pixels, \
    bilinear_process_four_pixels, \
    bilinear_process_pixblock_head, \
    bilinear_process_pixblock_tail, \
    bilinear_process_pixblock_tail_head, \
    pixblock_size, \
    prefetch_distance, \
    flags

pixman_asm_function \fname
.if \pixblock_size == 8
.elseif \pixblock_size == 4
.else
    .error unsupported pixblock size
.endif

.if ((\flags) & BILINEAR_FLAG_USE_MASK) == 0
    OUT       .req    x0
    TOP       .req    x1
    BOTTOM    .req    x2
    WT        .req    x3
    WWT       .req    w3
    WB        .req    x4
    WWB       .req    w4
    X         .req    w5
    UX        .req    w6
    WIDTH     .req    x7
    TMP1      .req    x10
    WTMP1     .req    w10
    TMP2      .req    x11
    WTMP2     .req    w11
    PF_OFFS   .req    x12
    TMP3      .req    x13
    WTMP3     .req    w13
    TMP4      .req    x14
    WTMP4     .req    w14
    STRIDE    .req    x15
    DUMMY     .req    x30

    stp       x29, x30, [sp, -16]!
    mov       x29, sp
    sub       sp, sp, 112
    sub       x29, x29, 64
    st1       {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    st1       {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    stp       x10, x11, [x29, -80]
    stp       x12, x13, [x29, -96]
    stp       x14, x15, [x29, -112]
.else
    OUT       .req      x0
    MASK      .req      x1
    TOP       .req      x2
    BOTTOM    .req      x3
    WT        .req      x4
    WWT       .req      w4
    WB        .req      x5
    WWB       .req      w5
    X         .req      w6
    UX        .req      w7
    WIDTH     .req      x8
    TMP1      .req      x10
    WTMP1     .req      w10
    TMP2      .req      x11
    WTMP2     .req      w11
    PF_OFFS   .req      x12
    TMP3      .req      x13
    WTMP3     .req      w13
    TMP4      .req      x14
    WTMP4     .req      w14
    STRIDE    .req      x15
    DUMMY     .req      x30

    .set prefetch_offset, \prefetch_distance

    stp      x29, x30, [sp, -16]!
    mov      x29, sp
    sub      x29, x29, 64
    st1      {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    st1      {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    stp      x10, x11, [x29, -80]
    stp      x12, x13, [x29, -96]
    stp      x14, x15, [x29, -112]
    str      x8, [x29, -120]
    ldr      w8, [x29, 16]
    sub      sp, sp, 120
.endif

    mov      WTMP1, #\prefetch_distance
    umull    PF_OFFS, WTMP1, UX

    sub      STRIDE, BOTTOM, TOP
    .unreq   BOTTOM

    cmp      WIDTH, #0
    ble      300f

    dup      v12.8h, X
    dup      v13.8h, UX
    dup      v28.8b, WWT
    dup      v29.8b, WWB
    mov      v25.d[0], v12.d[1]
    mov      v26.d[0], v13.d[0]
    add      v25.4h, v25.4h, v26.4h
    mov      v12.d[1], v25.d[0]

    /* ensure good destination alignment  */
    cmp       WIDTH, #1
    blt       100f
    tst       OUT, #(1 << \dst_bpp_shift)
    beq       100f
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    add       v12.8h, v12.8h, v13.8h
    \bilinear_process_last_pixel
    sub       WIDTH, WIDTH, #1
100:
    add       v13.8h, v13.8h, v13.8h
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    add       v12.8h, v12.8h, v13.8h

    cmp       WIDTH, #2
    blt       100f
    tst       OUT, #(1 << (\dst_bpp_shift + 1))
    beq       100f
    \bilinear_process_two_pixels
    sub       WIDTH, WIDTH, #2
100:
.if \pixblock_size == 8
    cmp       WIDTH, #4
    blt       100f
    tst       OUT, #(1 << (\dst_bpp_shift + 2))
    beq       100f
    \bilinear_process_four_pixels
    sub       WIDTH, WIDTH, #4
100:
.endif
    subs      WIDTH, WIDTH, #\pixblock_size
    blt       100f
    asr       PF_OFFS, PF_OFFS, #(16 - \src_bpp_shift)
    \bilinear_process_pixblock_head
    subs      WIDTH, WIDTH, #\pixblock_size
    blt       500f
0:
    \bilinear_process_pixblock_tail_head
    subs      WIDTH, WIDTH, #\pixblock_size
    bge       0b
500:
    \bilinear_process_pixblock_tail
100:
.if \pixblock_size == 8
    tst       WIDTH, #4
    beq       200f
    \bilinear_process_four_pixels
200:
.endif
    /* handle the remaining trailing pixels */
    tst       WIDTH, #2
    beq       200f
    \bilinear_process_two_pixels
200:
    tst       WIDTH, #1
    beq       300f
    \bilinear_process_last_pixel
300:

.if ((\flags) & BILINEAR_FLAG_USE_MASK) == 0
    sub       x29, x29, 64
    ld1       {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    ld1       {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    ldp       x10, x11, [x29, -80]
    ldp       x12, x13, [x29, -96]
    ldp       x14, x15, [x29, -112]
    mov       sp, x29
    ldp       x29, x30, [sp], 16
.else
    sub       x29, x29, 64
    ld1       {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    ld1       {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    ldp       x10, x11, [x29, -80]
    ldp       x12, x13, [x29, -96]
    ldp       x14, x15, [x29, -112]
    ldr       x8, [x29, -120]
    mov       sp, x29
    ldp       x29, x30, [sp], 16
.endif
    ret

    .unreq    OUT
    .unreq    TOP
    .unreq    WT
    .unreq    WWT
    .unreq    WB
    .unreq    WWB
    .unreq    X
    .unreq    UX
    .unreq    WIDTH
    .unreq    TMP1
    .unreq    WTMP1
    .unreq    TMP2
    .unreq    PF_OFFS
    .unreq    TMP3
    .unreq    TMP4
    .unreq    STRIDE
.if ((\flags) & BILINEAR_FLAG_USE_MASK) != 0
    .unreq    MASK
.endif

pixman_end_asm_function

.endm

/* src_8888_8_8888 */
.macro bilinear_src_8888_8_8888_process_last_pixel
    bilinear_interpolate_last_pixel 8888, 8, 8888, src
.endm

.macro bilinear_src_8888_8_8888_process_two_pixels
    bilinear_interpolate_two_pixels 8888, 8, 8888, src
.endm

.macro bilinear_src_8888_8_8888_process_four_pixels
    bilinear_interpolate_four_pixels 8888, 8, 8888, src
.endm

.macro bilinear_src_8888_8_8888_process_pixblock_head
    bilinear_src_8888_8_8888_process_four_pixels
.endm

.macro bilinear_src_8888_8_8888_process_pixblock_tail
.endm

.macro bilinear_src_8888_8_8888_process_pixblock_tail_head
    bilinear_src_8888_8_8888_process_pixblock_tail
    bilinear_src_8888_8_8888_process_pixblock_head
.endm

/* src_8888_8_0565 */
.macro bilinear_src_8888_8_0565_process_last_pixel
    bilinear_interpolate_last_pixel 8888, 8, 0565, src
.endm

.macro bilinear_src_8888_8_0565_process_two_pixels
    bilinear_interpolate_two_pixels 8888, 8, 0565, src
.endm

.macro bilinear_src_8888_8_0565_process_four_pixels
    bilinear_interpolate_four_pixels 8888, 8, 0565, src
.endm

.macro bilinear_src_8888_8_0565_process_pixblock_head
    bilinear_src_8888_8_0565_process_four_pixels
.endm

.macro bilinear_src_8888_8_0565_process_pixblock_tail
.endm

.macro bilinear_src_8888_8_0565_process_pixblock_tail_head
    bilinear_src_8888_8_0565_process_pixblock_tail
    bilinear_src_8888_8_0565_process_pixblock_head
.endm

/* src_0565_8_x888 */
.macro bilinear_src_0565_8_x888_process_last_pixel
    bilinear_interpolate_last_pixel 0565, 8, 8888, src
.endm

.macro bilinear_src_0565_8_x888_process_two_pixels
    bilinear_interpolate_two_pixels 0565, 8, 8888, src
.endm

.macro bilinear_src_0565_8_x888_process_four_pixels
    bilinear_interpolate_four_pixels 0565, 8, 8888, src
.endm

.macro bilinear_src_0565_8_x888_process_pixblock_head
    bilinear_src_0565_8_x888_process_four_pixels
.endm

.macro bilinear_src_0565_8_x888_process_pixblock_tail
.endm

.macro bilinear_src_0565_8_x888_process_pixblock_tail_head
    bilinear_src_0565_8_x888_process_pixblock_tail
    bilinear_src_0565_8_x888_process_pixblock_head
.endm

/* src_0565_8_0565 */
.macro bilinear_src_0565_8_0565_process_last_pixel
    bilinear_interpolate_last_pixel 0565, 8, 0565, src
.endm

.macro bilinear_src_0565_8_0565_process_two_pixels
    bilinear_interpolate_two_pixels 0565, 8, 0565, src
.endm

.macro bilinear_src_0565_8_0565_process_four_pixels
    bilinear_interpolate_four_pixels 0565, 8, 0565, src
.endm

.macro bilinear_src_0565_8_0565_process_pixblock_head
    bilinear_src_0565_8_0565_process_four_pixels
.endm

.macro bilinear_src_0565_8_0565_process_pixblock_tail
.endm

.macro bilinear_src_0565_8_0565_process_pixblock_tail_head
    bilinear_src_0565_8_0565_process_pixblock_tail
    bilinear_src_0565_8_0565_process_pixblock_head
.endm

/* over_8888_8888 */
.macro bilinear_over_8888_8888_process_last_pixel
    bilinear_interpolate_last_pixel 8888, x, 8888, over
.endm

.macro bilinear_over_8888_8888_process_two_pixels
    bilinear_interpolate_two_pixels 8888, x, 8888, over
.endm

.macro bilinear_over_8888_8888_process_four_pixels
    bilinear_interpolate_four_pixels 8888, x, 8888, over
.endm

.macro bilinear_over_8888_8888_process_pixblock_head
    asr         WTMP1, X, #16
    add         X, X, UX
    add         TMP1, TOP, TMP1, lsl #2
    asr         WTMP2, X, #16
    add         X, X, UX
    add         TMP2, TOP, TMP2, lsl #2

    ld1         {v22.2s}, [TMP1], STRIDE
    ld1         {v23.2s}, [TMP1]
    asr         WTMP3, X, #16
    add         X, X, UX
    add         TMP3, TOP, TMP3, lsl #2
    umull       v8.8h, v22.8b, v28.8b
    umlal       v8.8h, v23.8b, v29.8b

    ld1         {v22.2s}, [TMP2], STRIDE
    ld1         {v23.2s}, [TMP2]
    asr         WTMP4, X, #16
    add         X, X, UX
    add         TMP4, TOP, TMP4, lsl #2
    umull       v9.8h, v22.8b, v28.8b
    umlal       v9.8h, v23.8b, v29.8b

    ld1         {v22.2s}, [TMP3], STRIDE
    ld1         {v23.2s}, [TMP3]
    umull       v10.8h, v22.8b, v28.8b
    umlal       v10.8h, v23.8b, v29.8b

    ushll       v0.4s, v8.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl       v0.4s, v8.4h, v15.h[0]
    umlal2      v0.4s, v8.8h, v15.h[0]

    prfm        PREFETCH_MODE, [TMP4, PF_OFFS]
    ld1         {v16.2s}, [TMP4], STRIDE
    ld1         {v17.2s}, [TMP4]
    prfm        PREFETCH_MODE, [TMP4, PF_OFFS]
    umull       v11.8h, v16.8b, v28.8b
    umlal       v11.8h, v17.8b, v29.8b

    ushll       v1.4s, v9.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl       v1.4s, v9.4h, v15.h[4]
    umlal2      v1.4s, v9.8h, v15.h[4]
    ushr        v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    add         v12.8h, v12.8h, v13.8h
.endm

.macro bilinear_over_8888_8888_process_pixblock_tail
    ushll       v2.4s, v10.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl       v2.4s, v10.4h, v15.h[0]
    umlal2      v2.4s, v10.8h, v15.h[0]
    ushll       v3.4s, v11.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl       v3.4s, v11.4h, v15.h[4]
    umlal2      v3.4s, v11.8h, v15.h[4]
    shrn        v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn2       v0.8h, v1.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn        v2.4h, v2.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    ushr        v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    shrn2       v2.8h, v3.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    xtn         v6.8b, v0.8h
    xtn         v7.8b, v2.8h
    ld1         {v2.2s, v3.2s}, [OUT]
    prfm        PREFETCH_MODE, [OUT, #(prefetch_offset * 4)]
    vuzp        v6.8b, v7.8b
    vuzp        v2.8b, v3.8b
    vuzp        v6.8b, v7.8b
    vuzp        v2.8b, v3.8b
    dup         v4.2s, v7.s[1]
    mvn         v4.8b, v4.8b
    umull       v11.8h, v2.8b, v4.8b
    umull       v2.8h,  v3.8b, v4.8b
    urshr       v1.8h, v11.8h, #8
    urshr       v10.8h, v2.8h, #8
    raddhn      v3.8b, v10.8h, v2.8h
    raddhn      v2.8b, v1.8h, v11.8h
    uqadd       v6.8b, v2.8b,  v6.8b
    uqadd       v7.8b, v3.8b,  v7.8b
    vuzp        v6.8b, v7.8b
    vuzp        v6.8b, v7.8b
    add         v12.8h, v12.8h, v13.8h
    st1         {v6.2s, v7.2s}, [OUT], #16
.endm

.macro bilinear_over_8888_8888_process_pixblock_tail_head
                                            ushll       v2.4s, v10.4h, #BILINEAR_INTERPOLATION_BITS
    asr         WTMP1, X, #16
    add         X, X, UX
    add         TMP1, TOP, TMP1, lsl #2
                                            umlsl       v2.4s, v10.4h, v15.h[0]
    asr         WTMP2, X, #16
    add         X, X, UX
    add         TMP2, TOP, TMP2, lsl #2
                                            umlal2      v2.4s, v10.8h, v15.h[0]
                                            ushll       v3.4s, v11.4h, #BILINEAR_INTERPOLATION_BITS
    ld1         {v20.2s}, [TMP1], STRIDE
                                            umlsl       v3.4s, v11.4h, v15.h[4]
                                            umlal2      v3.4s, v11.8h, v15.h[4]
    ld1         {v21.2s}, [TMP1]
    umull       v8.8h, v20.8b, v28.8b
    umlal       v8.8h, v21.8b, v29.8b
                                            shrn        v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            shrn2       v0.8h, v1.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            shrn        v2.4h, v2.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            ushr        v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    ld1         {v22.2s}, [TMP2], STRIDE
                                            shrn2       v2.8h, v3.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            xtn         v6.8b, v0.8h
    ld1         {v23.2s}, [TMP2]
    umull       v9.8h, v22.8b, v28.8b
    asr         WTMP3, X, #16
    add         X, X, UX
    add         TMP3, TOP, TMP3, lsl #2
    asr         WTMP4, X, #16
    add         X, X, UX
    add         TMP4, TOP, TMP4, lsl #2
    umlal       v9.8h, v23.8b, v29.8b
                                            xtn         v7.8b, v2.8h
                                            ld1         {v2.2s, v3.2s}, [OUT]
                                            prfm        PREFETCH_MODE, [OUT, PF_OFFS]
    ld1         {v22.2s}, [TMP3], STRIDE
                                            vuzp        v6.8b, v7.8b
                                            vuzp        v2.8b, v3.8b
                                            vuzp        v6.8b, v7.8b
                                            vuzp        v2.8b, v3.8b
                                            dup         v4.2s, v7.s[1]
    ld1         {v23.2s}, [TMP3]
                                            mvn         v4.8b, v4.8b
    umull       v10.8h, v22.8b, v28.8b
    umlal       v10.8h, v23.8b, v29.8b
                                            umull       v11.8h, v2.8b, v4.8b
                                            umull        v2.8h, v3.8b, v4.8b
    ushll       v0.4s, v8.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl       v0.4s, v8.4h, v15.h[0]
                                            urshr       v1.8h, v11.8h, #8
    umlal2      v0.4s, v8.8h, v15.h[0]
                                            urshr       v8.8h, v2.8h, #8
                                            raddhn      v3.8b, v8.8h, v2.8h
                                            raddhn      v2.8b, v1.8h, v11.8h
    prfm        PREFETCH_MODE, [TMP4, PF_OFFS]
    ld1         {v16.2s}, [TMP4], STRIDE
                                            uqadd       v6.8b, v2.8b, v6.8b
                                            uqadd       v7.8b, v3.8b, v7.8b
    ld1         {v17.2s}, [TMP4]
    prfm        PREFETCH_MODE, [TMP4, PF_OFFS]
    umull       v11.8h, v16.8b, v28.8b
    umlal       v11.8h, v17.8b, v29.8b
                                            vuzp        v6.8b, v7.8b
    ushll       v1.4s, v9.4h, #BILINEAR_INTERPOLATION_BITS
                                            vuzp        v6.8b, v7.8b
    umlsl       v1.4s, v9.4h, v15.h[4]
                                            add         v12.8h, v12.8h, v13.8h
    umlal2      v1.4s, v9.8h, v15.h[4]
    ushr        v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    add         v12.8h, v12.8h, v13.8h
                                            st1         {v6.2s, v7.2s}, [OUT], #16
.endm

/* over_8888_8_8888 */
.macro bilinear_over_8888_8_8888_process_last_pixel
    bilinear_interpolate_last_pixel 8888, 8, 8888, over
.endm

.macro bilinear_over_8888_8_8888_process_two_pixels
    bilinear_interpolate_two_pixels 8888, 8, 8888, over
.endm

.macro bilinear_over_8888_8_8888_process_four_pixels
    bilinear_interpolate_two_pixels 8888, 8, 8888, over
    bilinear_interpolate_two_pixels 8888, 8, 8888, over
.endm

.macro bilinear_over_8888_8_8888_process_pixblock_head
    bilinear_over_8888_8_8888_process_four_pixels
.endm

.macro bilinear_over_8888_8_8888_process_pixblock_tail
.endm

.macro bilinear_over_8888_8_8888_process_pixblock_tail_head
     bilinear_over_8888_8_8888_process_pixblock_tail
     bilinear_over_8888_8_8888_process_pixblock_head
.endm

/* add_8888_8888 */
.macro bilinear_add_8888_8888_process_last_pixel
    bilinear_interpolate_last_pixel 8888, x, 8888, add
.endm

.macro bilinear_add_8888_8888_process_two_pixels
    bilinear_interpolate_two_pixels 8888, x, 8888, add
.endm

.macro bilinear_add_8888_8888_process_four_pixels
    bilinear_interpolate_two_pixels 8888, x, 8888, add
    bilinear_interpolate_two_pixels 8888, x, 8888, add
.endm

.macro bilinear_add_8888_8888_process_pixblock_head
    bilinear_add_8888_8888_process_four_pixels
.endm

.macro bilinear_add_8888_8888_process_pixblock_tail
.endm

.macro bilinear_add_8888_8888_process_pixblock_tail_head
    bilinear_add_8888_8888_process_pixblock_tail
    bilinear_add_8888_8888_process_pixblock_head
.endm

/* add_8888_8_8888 */
.macro bilinear_add_8888_8_8888_process_last_pixel
    bilinear_interpolate_last_pixel 8888, 8, 8888, add
.endm

.macro bilinear_add_8888_8_8888_process_two_pixels
    bilinear_interpolate_two_pixels 8888, 8, 8888, add
.endm

.macro bilinear_add_8888_8_8888_process_four_pixels
    bilinear_interpolate_four_pixels 8888, 8, 8888, add
.endm

.macro bilinear_add_8888_8_8888_process_pixblock_head
    bilinear_add_8888_8_8888_process_four_pixels
.endm

.macro bilinear_add_8888_8_8888_process_pixblock_tail
.endm

.macro bilinear_add_8888_8_8888_process_pixblock_tail_head
    bilinear_add_8888_8_8888_process_pixblock_tail
    bilinear_add_8888_8_8888_process_pixblock_head
.endm


/* Bilinear scanline functions */
generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_8_8888_SRC_asm_neon, \
    8888, 8888, 2, 2, \
    bilinear_src_8888_8_8888_process_last_pixel, \
    bilinear_src_8888_8_8888_process_two_pixels, \
    bilinear_src_8888_8_8888_process_four_pixels, \
    bilinear_src_8888_8_8888_process_pixblock_head, \
    bilinear_src_8888_8_8888_process_pixblock_tail, \
    bilinear_src_8888_8_8888_process_pixblock_tail_head, \
    4, 28, BILINEAR_FLAG_USE_MASK

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_8_0565_SRC_asm_neon, \
    8888, 0565, 2, 1, \
    bilinear_src_8888_8_0565_process_last_pixel, \
    bilinear_src_8888_8_0565_process_two_pixels, \
    bilinear_src_8888_8_0565_process_four_pixels, \
    bilinear_src_8888_8_0565_process_pixblock_head, \
    bilinear_src_8888_8_0565_process_pixblock_tail, \
    bilinear_src_8888_8_0565_process_pixblock_tail_head, \
    4, 28, BILINEAR_FLAG_USE_MASK

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_0565_8_x888_SRC_asm_neon, \
    0565, 8888, 1, 2, \
    bilinear_src_0565_8_x888_process_last_pixel, \
    bilinear_src_0565_8_x888_process_two_pixels, \
    bilinear_src_0565_8_x888_process_four_pixels, \
    bilinear_src_0565_8_x888_process_pixblock_head, \
    bilinear_src_0565_8_x888_process_pixblock_tail, \
    bilinear_src_0565_8_x888_process_pixblock_tail_head, \
    4, 28, BILINEAR_FLAG_USE_MASK

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_0565_8_0565_SRC_asm_neon, \
    0565, 0565, 1, 1, \
    bilinear_src_0565_8_0565_process_last_pixel, \
    bilinear_src_0565_8_0565_process_two_pixels, \
    bilinear_src_0565_8_0565_process_four_pixels, \
    bilinear_src_0565_8_0565_process_pixblock_head, \
    bilinear_src_0565_8_0565_process_pixblock_tail, \
    bilinear_src_0565_8_0565_process_pixblock_tail_head, \
    4, 28, BILINEAR_FLAG_USE_MASK

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_8888_OVER_asm_neon, \
    8888, 8888, 2, 2, \
    bilinear_over_8888_8888_process_last_pixel, \
    bilinear_over_8888_8888_process_two_pixels, \
    bilinear_over_8888_8888_process_four_pixels, \
    bilinear_over_8888_8888_process_pixblock_head, \
    bilinear_over_8888_8888_process_pixblock_tail, \
    bilinear_over_8888_8888_process_pixblock_tail_head, \
    4, 28, 0

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_8_8888_OVER_asm_neon, \
    8888, 8888, 2, 2, \
    bilinear_over_8888_8_8888_process_last_pixel, \
    bilinear_over_8888_8_8888_process_two_pixels, \
    bilinear_over_8888_8_8888_process_four_pixels, \
    bilinear_over_8888_8_8888_process_pixblock_head, \
    bilinear_over_8888_8_8888_process_pixblock_tail, \
    bilinear_over_8888_8_8888_process_pixblock_tail_head, \
    4, 28, BILINEAR_FLAG_USE_MASK

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_8888_ADD_asm_neon, \
    8888, 8888, 2, 2, \
    bilinear_add_8888_8888_process_last_pixel, \
    bilinear_add_8888_8888_process_two_pixels, \
    bilinear_add_8888_8888_process_four_pixels, \
    bilinear_add_8888_8888_process_pixblock_head, \
    bilinear_add_8888_8888_process_pixblock_tail, \
    bilinear_add_8888_8888_process_pixblock_tail_head, \
    4, 28, 0

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_8_8888_ADD_asm_neon, \
    8888, 8888, 2, 2, \
    bilinear_add_8888_8_8888_process_last_pixel, \
    bilinear_add_8888_8_8888_process_two_pixels, \
    bilinear_add_8888_8_8888_process_four_pixels, \
    bilinear_add_8888_8_8888_process_pixblock_head, \
    bilinear_add_8888_8_8888_process_pixblock_tail, \
    bilinear_add_8888_8_8888_process_pixblock_tail_head, \
    4, 28, BILINEAR_FLAG_USE_MASK
