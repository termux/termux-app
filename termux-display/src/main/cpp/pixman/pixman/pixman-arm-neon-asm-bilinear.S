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
.fpu neon
.arch armv7a
.object_arch armv4
.eabi_attribute 10, 0
.eabi_attribute 12, 0
.arm
.altmacro
.p2align 2

#include "pixman-private.h"
#include "pixman-arm-asm.h"
#include "pixman-arm-neon-asm.h"

/*
 * Bilinear macros from pixman-arm-neon-asm.S
 */

/*
 * Bilinear scaling support code which tries to provide pixel fetching, color
 * format conversion, and interpolation as separate macros which can be used
 * as the basic building blocks for constructing bilinear scanline functions.
 */

.macro bilinear_load_8888 reg1, reg2, tmp
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    vld1.32   {reg1}, [TMP1], STRIDE
    vld1.32   {reg2}, [TMP1]
.endm

.macro bilinear_load_0565 reg1, reg2, tmp
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #1
    vld1.32   {reg2[0]}, [TMP1], STRIDE
    vld1.32   {reg2[1]}, [TMP1]
    convert_four_0565_to_x888_packed reg2, reg1, reg2, tmp
.endm

.macro bilinear_load_and_vertical_interpolate_two_8888 \
                    acc1, acc2, reg1, reg2, reg3, reg4, tmp1, tmp2

    bilinear_load_8888 reg1, reg2, tmp1
    vmull.u8  acc1, reg1, d28
    vmlal.u8  acc1, reg2, d29
    bilinear_load_8888 reg3, reg4, tmp2
    vmull.u8  acc2, reg3, d28
    vmlal.u8  acc2, reg4, d29
.endm

.macro bilinear_load_and_vertical_interpolate_four_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi

    bilinear_load_and_vertical_interpolate_two_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi
    bilinear_load_and_vertical_interpolate_two_8888 \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi
.endm

.macro bilinear_load_and_vertical_interpolate_two_0565 \
                acc1, acc2, reg1, reg2, reg3, reg4, acc2lo, acc2hi

    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #1
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #1
    vld1.32   {acc2lo[0]}, [TMP1], STRIDE
    vld1.32   {acc2hi[0]}, [TMP2], STRIDE
    vld1.32   {acc2lo[1]}, [TMP1]
    vld1.32   {acc2hi[1]}, [TMP2]
    convert_0565_to_x888 acc2, reg3, reg2, reg1
    vzip.u8   reg1, reg3
    vzip.u8   reg2, reg4
    vzip.u8   reg3, reg4
    vzip.u8   reg1, reg2
    vmull.u8  acc1, reg1, d28
    vmlal.u8  acc1, reg2, d29
    vmull.u8  acc2, reg3, d28
    vmlal.u8  acc2, reg4, d29
.endm

.macro bilinear_load_and_vertical_interpolate_four_0565 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi

    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #1
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #1
    vld1.32   {xacc2lo[0]}, [TMP1], STRIDE
    vld1.32   {xacc2hi[0]}, [TMP2], STRIDE
    vld1.32   {xacc2lo[1]}, [TMP1]
    vld1.32   {xacc2hi[1]}, [TMP2]
    convert_0565_to_x888 xacc2, xreg3, xreg2, xreg1
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #1
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #1
    vld1.32   {yacc2lo[0]}, [TMP1], STRIDE
    vzip.u8   xreg1, xreg3
    vld1.32   {yacc2hi[0]}, [TMP2], STRIDE
    vzip.u8   xreg2, xreg4
    vld1.32   {yacc2lo[1]}, [TMP1]
    vzip.u8   xreg3, xreg4
    vld1.32   {yacc2hi[1]}, [TMP2]
    vzip.u8   xreg1, xreg2
    convert_0565_to_x888 yacc2, yreg3, yreg2, yreg1
    vmull.u8  xacc1, xreg1, d28
    vzip.u8   yreg1, yreg3
    vmlal.u8  xacc1, xreg2, d29
    vzip.u8   yreg2, yreg4
    vmull.u8  xacc2, xreg3, d28
    vzip.u8   yreg3, yreg4
    vmlal.u8  xacc2, xreg4, d29
    vzip.u8   yreg1, yreg2
    vmull.u8  yacc1, yreg1, d28
    vmlal.u8  yacc1, yreg2, d29
    vmull.u8  yacc2, yreg3, d28
    vmlal.u8  yacc2, yreg4, d29
.endm

.macro bilinear_store_8888 numpix, tmp1, tmp2
.if numpix == 4
    vst1.32   {d0, d1}, [OUT]!
.elseif numpix == 2
    vst1.32   {d0}, [OUT]!
.elseif numpix == 1
    vst1.32   {d0[0]}, [OUT, :32]!
.else
    .error bilinear_store_8888 numpix is unsupported
.endif
.endm

.macro bilinear_store_0565 numpix, tmp1, tmp2
    vuzp.u8 d0, d1
    vuzp.u8 d2, d3
    vuzp.u8 d1, d3
    vuzp.u8 d0, d2
    convert_8888_to_0565 d2, d1, d0, q1, tmp1, tmp2
.if numpix == 4
    vst1.16   {d2}, [OUT]!
.elseif numpix == 2
    vst1.32   {d2[0]}, [OUT]!
.elseif numpix == 1
    vst1.16   {d2[0]}, [OUT]!
.else
    .error bilinear_store_0565 numpix is unsupported
.endif
.endm


/*
 * Macros for loading mask pixels into register 'mask'.
 * vdup must be done in somewhere else.
 */
.macro bilinear_load_mask_x numpix, mask
.endm

.macro bilinear_load_mask_8 numpix, mask
.if numpix == 4
    vld1.32     {mask[0]}, [MASK]!
.elseif numpix == 2
    vld1.16     {mask[0]}, [MASK]!
.elseif numpix == 1
    vld1.8      {mask[0]}, [MASK]!
.else
    .error bilinear_load_mask_8 numpix is unsupported
.endif
    pld         [MASK, #prefetch_offset]
.endm

.macro bilinear_load_mask mask_fmt, numpix, mask
    bilinear_load_mask_&mask_fmt numpix, mask
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
.if numpix == 4
    vld1.32     {dst0, dst1}, [OUT]
.elseif numpix == 2
    vld1.32     {dst0}, [OUT]
.elseif numpix == 1
    vld1.32     {dst0[0]}, [OUT]
.else
    .error bilinear_load_dst_8888 numpix is unsupported
.endif
    pld         [OUT, #(prefetch_offset * 4)]
.endm

.macro bilinear_load_dst_8888_over numpix, dst0, dst1, dst01
    bilinear_load_dst_8888 numpix, dst0, dst1, dst01
.endm

.macro bilinear_load_dst_8888_add numpix, dst0, dst1, dst01
    bilinear_load_dst_8888 numpix, dst0, dst1, dst01
.endm

.macro bilinear_load_dst dst_fmt, op, numpix, dst0, dst1, dst01
    bilinear_load_dst_&dst_fmt&_&op numpix, dst0, dst1, dst01
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
.if numpix == 4
    vdup.32     mask, mask[0]
.elseif numpix == 2
    vdup.16     mask, mask[0]
.elseif numpix == 1
    vdup.8      mask, mask[0]
.else
    .error bilinear_duplicate_mask_8 is unsupported
.endif
.endm

.macro bilinear_duplicate_mask mask_fmt, numpix, mask
    bilinear_duplicate_mask_&mask_fmt numpix, mask
.endm

/*
 * Macros for interleaving src and dst pixels to rrrr gggg bbbb aaaa form.
 * Interleave should be done when maks is enabled or operator is 'over'.
 */
.macro bilinear_interleave src0, src1, dst0, dst1
    vuzp.8      src0, src1
    vuzp.8      dst0, dst1
    vuzp.8      src0, src1
    vuzp.8      dst0, dst1
.endm

.macro bilinear_interleave_src_dst_x_src \
                numpix, src0, src1, src01, dst0, dst1, dst01
.endm

.macro bilinear_interleave_src_dst_x_over \
                numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave src0, src1, dst0, dst1
.endm

.macro bilinear_interleave_src_dst_x_add \
                numpix, src0, src1, src01, dst0, dst1, dst01
.endm

.macro bilinear_interleave_src_dst_8_src \
                numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave src0, src1, dst0, dst1
.endm

.macro bilinear_interleave_src_dst_8_over \
                numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave src0, src1, dst0, dst1
.endm

.macro bilinear_interleave_src_dst_8_add \
                numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave src0, src1, dst0, dst1
.endm

.macro bilinear_interleave_src_dst \
                mask_fmt, op, numpix, src0, src1, src01, dst0, dst1, dst01

    bilinear_interleave_src_dst_&mask_fmt&_&op \
                numpix, src0, src1, src01, dst0, dst1, dst01
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

    vmull.u8        tmp01, src0, mask
    vmull.u8        tmp23, src1, mask
    /* bubbles */
    vrshr.u16       tmp45, tmp01, #8
    vrshr.u16       tmp67, tmp23, #8
    /* bubbles */
    vraddhn.u16     src0, tmp45, tmp01
    vraddhn.u16     src1, tmp67, tmp23
.endm

.macro bilinear_apply_mask_to_src \
                mask_fmt, numpix, src0, src1, src01, mask, \
                tmp01, tmp23, tmp45, tmp67

    bilinear_apply_mask_to_src_&mask_fmt \
                numpix, src0, src1, src01, mask, \
                tmp01, tmp23, tmp45, tmp67
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

    vdup.32     tmp8, src1[1]
    /* bubbles */
    vmvn.8      tmp8, tmp8
    /* bubbles */
    vmull.u8    tmp01, dst0, tmp8
    /* bubbles */
    vmull.u8    tmp23, dst1, tmp8
    /* bubbles */
    vrshr.u16   tmp45, tmp01, #8
    vrshr.u16   tmp67, tmp23, #8
    /* bubbles */
    vraddhn.u16 dst0, tmp45, tmp01
    vraddhn.u16 dst1, tmp67, tmp23
    /* bubbles */
    vqadd.u8    src01, dst01, src01
.endm

.macro bilinear_combine_add \
                numpix, src0, src1, src01, dst0, dst1, dst01, \
                tmp01, tmp23, tmp45, tmp67, tmp8

    vqadd.u8    src01, dst01, src01
.endm

.macro bilinear_combine \
                op, numpix, src0, src1, src01, dst0, dst1, dst01, \
                tmp01, tmp23, tmp45, tmp67, tmp8

    bilinear_combine_&op \
                numpix, src0, src1, src01, dst0, dst1, dst01, \
                tmp01, tmp23, tmp45, tmp67, tmp8
.endm

/*
 * Macros for final deinterleaving of destination pixels if needed.
 */
.macro bilinear_deinterleave numpix, dst0, dst1, dst01
    vuzp.8      dst0, dst1
    /* bubbles */
    vuzp.8      dst0, dst1
.endm

.macro bilinear_deinterleave_dst_x_src numpix, dst0, dst1, dst01
.endm

.macro bilinear_deinterleave_dst_x_over numpix, dst0, dst1, dst01
    bilinear_deinterleave numpix, dst0, dst1, dst01
.endm

.macro bilinear_deinterleave_dst_x_add numpix, dst0, dst1, dst01
.endm

.macro bilinear_deinterleave_dst_8_src numpix, dst0, dst1, dst01
    bilinear_deinterleave numpix, dst0, dst1, dst01
.endm

.macro bilinear_deinterleave_dst_8_over numpix, dst0, dst1, dst01
    bilinear_deinterleave numpix, dst0, dst1, dst01
.endm

.macro bilinear_deinterleave_dst_8_add numpix, dst0, dst1, dst01
    bilinear_deinterleave numpix, dst0, dst1, dst01
.endm

.macro bilinear_deinterleave_dst mask_fmt, op, numpix, dst0, dst1, dst01
    bilinear_deinterleave_dst_&mask_fmt&_&op numpix, dst0, dst1, dst01
.endm


.macro bilinear_interpolate_last_pixel src_fmt, mask_fmt, dst_fmt, op
    bilinear_load_&src_fmt d0, d1, d2
    bilinear_load_mask mask_fmt, 1, d4
    bilinear_load_dst dst_fmt, op, 1, d18, d19, q9
    vmull.u8  q1, d0, d28
    vmlal.u8  q1, d1, d29
    /* 5 cycles bubble */
    vshll.u16 q0, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d2, d30
    vmlal.u16 q0, d3, d30
    /* 5 cycles bubble */
    bilinear_duplicate_mask mask_fmt, 1, d4
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    /* 3 cycles bubble */
    vmovn.u16 d0, q0
    /* 1 cycle bubble */
    bilinear_interleave_src_dst \
                mask_fmt, op, 1, d0, d1, q0, d18, d19, q9
    bilinear_apply_mask_to_src \
                mask_fmt, 1, d0, d1, q0, d4, \
                q3, q8, q10, q11
    bilinear_combine \
                op, 1, d0, d1, q0, d18, d19, q9, \
                q3, q8, q10, q11, d5
    bilinear_deinterleave_dst mask_fmt, op, 1, d0, d1, q0
    bilinear_store_&dst_fmt 1, q2, q3
.endm

.macro bilinear_interpolate_two_pixels src_fmt, mask_fmt, dst_fmt, op
    bilinear_load_and_vertical_interpolate_two_&src_fmt \
                q1, q11, d0, d1, d20, d21, d22, d23
    bilinear_load_mask mask_fmt, 2, d4
    bilinear_load_dst dst_fmt, op, 2, d18, d19, q9
    vshll.u16 q0, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d2, d30
    vmlal.u16 q0, d3, d30
    vshll.u16 q10, d22, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q10, d22, d31
    vmlal.u16 q10, d23, d31
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d1, q10, #(2 * BILINEAR_INTERPOLATION_BITS)
    bilinear_duplicate_mask mask_fmt, 2, d4
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16  q12, q12, q13
    vmovn.u16 d0, q0
    bilinear_interleave_src_dst \
                mask_fmt, op, 2, d0, d1, q0, d18, d19, q9
    bilinear_apply_mask_to_src \
                mask_fmt, 2, d0, d1, q0, d4, \
                q3, q8, q10, q11
    bilinear_combine \
                op, 2, d0, d1, q0, d18, d19, q9, \
                q3, q8, q10, q11, d5
    bilinear_deinterleave_dst mask_fmt, op, 2, d0, d1, q0
    bilinear_store_&dst_fmt 2, q2, q3
.endm

.macro bilinear_interpolate_four_pixels src_fmt, mask_fmt, dst_fmt, op
    bilinear_load_and_vertical_interpolate_four_&src_fmt \
                q1, q11, d0, d1, d20, d21, d22, d23 \
                q3, q9,  d4, d5, d16, d17, d18, d19
    pld       [TMP1, PF_OFFS]
    sub       TMP1, TMP1, STRIDE
    vshll.u16 q0, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d2, d30
    vmlal.u16 q0, d3, d30
    vshll.u16 q10, d22, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q10, d22, d31
    vmlal.u16 q10, d23, d31
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vshll.u16 q2, d6, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q2, d6, d30
    vmlal.u16 q2, d7, d30
    vshll.u16 q8, d18, #BILINEAR_INTERPOLATION_BITS
    bilinear_load_mask mask_fmt, 4, d22
    bilinear_load_dst dst_fmt, op, 4, d2, d3, q1
    pld       [TMP1, PF_OFFS]
    vmlsl.u16 q8, d18, d31
    vmlal.u16 q8, d19, d31
    vadd.u16  q12, q12, q13
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d1, q10, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d5, q8, #(2 * BILINEAR_INTERPOLATION_BITS)
    bilinear_duplicate_mask mask_fmt, 4, d22
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vmovn.u16 d0, q0
    vmovn.u16 d1, q2
    vadd.u16  q12, q12, q13
    bilinear_interleave_src_dst \
                mask_fmt, op, 4, d0, d1, q0, d2, d3, q1
    bilinear_apply_mask_to_src \
                mask_fmt, 4, d0, d1, q0, d22, \
                q3, q8, q9, q10
    bilinear_combine \
                op, 4, d0, d1, q0, d2, d3, q1, \
                q3, q8, q9, q10, d23
    bilinear_deinterleave_dst mask_fmt, op, 4, d0, d1, q0
    bilinear_store_&dst_fmt 4, q2, q3
.endm

.set BILINEAR_FLAG_USE_MASK,		1
.set BILINEAR_FLAG_USE_ALL_NEON_REGS,	2

/*
 * Main template macro for generating NEON optimized bilinear scanline functions.
 *
 * Bilinear scanline generator macro take folling arguments:
 *  fname			- name of the function to generate
 *  src_fmt			- source color format (8888 or 0565)
 *  dst_fmt			- destination color format (8888 or 0565)
 *  src/dst_bpp_shift		- (1 << bpp_shift) is the size of src/dst pixel in bytes
 *  process_last_pixel		- code block that interpolate one pixel and does not
 *				  update horizontal weight
 *  process_two_pixels		- code block that interpolate two pixels and update
 *				  horizontal weight
 *  process_four_pixels		- code block that interpolate four pixels and update
 *				  horizontal weight
 *  process_pixblock_head	- head part of middle loop
 *  process_pixblock_tail	- tail part of middle loop
 *  process_pixblock_tail_head	- tail_head of middle loop
 *  pixblock_size		- number of pixels processed in a single middle loop
 *  prefetch_distance		- prefetch in the source image by that many pixels ahead
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

pixman_asm_function fname
.if pixblock_size == 8
.elseif pixblock_size == 4
.else
    .error unsupported pixblock size
.endif

.if ((flags) & BILINEAR_FLAG_USE_MASK) == 0
    OUT       .req    r0
    TOP       .req    r1
    BOTTOM    .req    r2
    WT        .req    r3
    WB        .req    r4
    X         .req    r5
    UX        .req    r6
    WIDTH     .req    ip
    TMP1      .req    r3
    TMP2      .req    r4
    PF_OFFS   .req    r7
    TMP3      .req    r8
    TMP4      .req    r9
    STRIDE    .req    r2

    mov		ip, sp
    push	{r4, r5, r6, r7, r8, r9}
    mov		PF_OFFS, #prefetch_distance
    ldmia	ip, {WB, X, UX, WIDTH}
.else
    OUT       .req      r0
    MASK      .req      r1
    TOP       .req      r2
    BOTTOM    .req      r3
    WT        .req      r4
    WB        .req      r5
    X         .req      r6
    UX        .req      r7
    WIDTH     .req      ip
    TMP1      .req      r4
    TMP2      .req      r5
    PF_OFFS   .req      r8
    TMP3      .req      r9
    TMP4      .req      r10
    STRIDE    .req      r3

    .set prefetch_offset, prefetch_distance

    mov       ip, sp
    push      {r4, r5, r6, r7, r8, r9, r10, ip}
    mov       PF_OFFS, #prefetch_distance
    ldmia     ip, {WT, WB, X, UX, WIDTH}
.endif

    mul       PF_OFFS, PF_OFFS, UX

.if ((flags) & BILINEAR_FLAG_USE_ALL_NEON_REGS) != 0
    vpush     {d8-d15}
.endif

    sub	      STRIDE, BOTTOM, TOP
    .unreq    BOTTOM

    cmp       WIDTH, #0
    ble       3f

    vdup.u16  q12, X
    vdup.u16  q13, UX
    vdup.u8   d28, WT
    vdup.u8   d29, WB
    vadd.u16  d25, d25, d26

    /* ensure good destination alignment  */
    cmp       WIDTH, #1
    blt       0f
    tst       OUT, #(1 << dst_bpp_shift)
    beq       0f
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16  q12, q12, q13
    bilinear_process_last_pixel
    sub       WIDTH, WIDTH, #1
0:
    vadd.u16  q13, q13, q13
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16  q12, q12, q13

    cmp       WIDTH, #2
    blt       0f
    tst       OUT, #(1 << (dst_bpp_shift + 1))
    beq       0f
    bilinear_process_two_pixels
    sub       WIDTH, WIDTH, #2
0:
.if pixblock_size == 8
    cmp       WIDTH, #4
    blt       0f
    tst       OUT, #(1 << (dst_bpp_shift + 2))
    beq       0f
    bilinear_process_four_pixels
    sub       WIDTH, WIDTH, #4
0:
.endif
    subs      WIDTH, WIDTH, #pixblock_size
    blt       1f
    mov       PF_OFFS, PF_OFFS, asr #(16 - src_bpp_shift)
    bilinear_process_pixblock_head
    subs      WIDTH, WIDTH, #pixblock_size
    blt       5f
0:
    bilinear_process_pixblock_tail_head
    subs      WIDTH, WIDTH, #pixblock_size
    bge       0b
5:
    bilinear_process_pixblock_tail
1:
.if pixblock_size == 8
    tst       WIDTH, #4
    beq       2f
    bilinear_process_four_pixels
2:
.endif
    /* handle the remaining trailing pixels */
    tst       WIDTH, #2
    beq       2f
    bilinear_process_two_pixels
2:
    tst       WIDTH, #1
    beq       3f
    bilinear_process_last_pixel
3:
.if ((flags) & BILINEAR_FLAG_USE_ALL_NEON_REGS) != 0
    vpop      {d8-d15}
.endif

.if ((flags) & BILINEAR_FLAG_USE_MASK) == 0
    pop       {r4, r5, r6, r7, r8, r9}
.else
    pop       {r4, r5, r6, r7, r8, r9, r10, ip}
.endif
    bx        lr

    .unreq    OUT
    .unreq    TOP
    .unreq    WT
    .unreq    WB
    .unreq    X
    .unreq    UX
    .unreq    WIDTH
    .unreq    TMP1
    .unreq    TMP2
    .unreq    PF_OFFS
    .unreq    TMP3
    .unreq    TMP4
    .unreq    STRIDE
.if ((flags) & BILINEAR_FLAG_USE_MASK) != 0
    .unreq    MASK
.endif

.endfunc

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
    mov         TMP1, X, asr #16
    add         X, X, UX
    add         TMP1, TOP, TMP1, asl #2
    mov         TMP2, X, asr #16
    add         X, X, UX
    add         TMP2, TOP, TMP2, asl #2

    vld1.32     {d22}, [TMP1], STRIDE
    vld1.32     {d23}, [TMP1]
    mov         TMP3, X, asr #16
    add         X, X, UX
    add         TMP3, TOP, TMP3, asl #2
    vmull.u8    q8, d22, d28
    vmlal.u8    q8, d23, d29

    vld1.32     {d22}, [TMP2], STRIDE
    vld1.32     {d23}, [TMP2]
    mov         TMP4, X, asr #16
    add         X, X, UX
    add         TMP4, TOP, TMP4, asl #2
    vmull.u8    q9, d22, d28
    vmlal.u8    q9, d23, d29

    vld1.32     {d22}, [TMP3], STRIDE
    vld1.32     {d23}, [TMP3]
    vmull.u8    q10, d22, d28
    vmlal.u8    q10, d23, d29

    vshll.u16   q0, d16, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16   q0, d16, d30
    vmlal.u16   q0, d17, d30

    pld         [TMP4, PF_OFFS]
    vld1.32     {d16}, [TMP4], STRIDE
    vld1.32     {d17}, [TMP4]
    pld         [TMP4, PF_OFFS]
    vmull.u8    q11, d16, d28
    vmlal.u8    q11, d17, d29

    vshll.u16   q1, d18, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16   q1, d18, d31
    vmlal.u16   q1, d19, d31
    vshr.u16    q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16    q12, q12, q13
.endm

.macro bilinear_over_8888_8888_process_pixblock_tail
    vshll.u16   q2, d20, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16   q2, d20, d30
    vmlal.u16   q2, d21, d30
    vshll.u16   q3, d22, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16   q3, d22, d31
    vmlal.u16   q3, d23, d31
    vshrn.u32   d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32   d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
    vld1.32     {d2, d3}, [OUT, :128]
    pld         [OUT, #(prefetch_offset * 4)]
    vshrn.u32   d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshr.u16    q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vshrn.u32   d5, q3, #(2 * BILINEAR_INTERPOLATION_BITS)
    vmovn.u16   d6, q0
    vmovn.u16   d7, q2
    vuzp.8      d6, d7
    vuzp.8      d2, d3
    vuzp.8      d6, d7
    vuzp.8      d2, d3
    vdup.32     d4, d7[1]
    vmvn.8      d4, d4
    vmull.u8    q11, d2, d4
    vmull.u8    q2, d3, d4
    vrshr.u16   q1, q11, #8
    vrshr.u16   q10, q2, #8
    vraddhn.u16 d2, q1, q11
    vraddhn.u16 d3, q10, q2
    vqadd.u8    q3, q1, q3
    vuzp.8      d6, d7
    vuzp.8      d6, d7
    vadd.u16    q12, q12, q13
    vst1.32     {d6, d7}, [OUT, :128]!
.endm

.macro bilinear_over_8888_8888_process_pixblock_tail_head
                                            vshll.u16   q2, d20, #BILINEAR_INTERPOLATION_BITS
    mov         TMP1, X, asr #16
    add         X, X, UX
    add         TMP1, TOP, TMP1, asl #2
                                            vmlsl.u16   q2, d20, d30
    mov         TMP2, X, asr #16
    add         X, X, UX
    add         TMP2, TOP, TMP2, asl #2
                                            vmlal.u16   q2, d21, d30
                                            vshll.u16   q3, d22, #BILINEAR_INTERPOLATION_BITS
    vld1.32     {d20}, [TMP1], STRIDE
                                            vmlsl.u16   q3, d22, d31
                                            vmlal.u16   q3, d23, d31
    vld1.32     {d21}, [TMP1]
    vmull.u8    q8, d20, d28
    vmlal.u8    q8, d21, d29
                                            vshrn.u32   d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            vshrn.u32   d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            vld1.32     {d2, d3}, [OUT, :128]
                                            pld         [OUT, PF_OFFS]
                                            vshrn.u32   d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            vshr.u16    q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vld1.32     {d22}, [TMP2], STRIDE
                                            vshrn.u32   d5, q3, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            vmovn.u16   d6, q0
    vld1.32     {d23}, [TMP2]
    vmull.u8    q9, d22, d28
    mov         TMP3, X, asr #16
    add         X, X, UX
    add         TMP3, TOP, TMP3, asl #2
    mov         TMP4, X, asr #16
    add         X, X, UX
    add         TMP4, TOP, TMP4, asl #2
    vmlal.u8    q9, d23, d29
                                            vmovn.u16   d7, q2
    vld1.32     {d22}, [TMP3], STRIDE
                                            vuzp.8      d6, d7
                                            vuzp.8      d2, d3
                                            vuzp.8      d6, d7
                                            vuzp.8      d2, d3
                                            vdup.32     d4, d7[1]
    vld1.32     {d23}, [TMP3]
                                            vmvn.8      d4, d4
    vmull.u8    q10, d22, d28
    vmlal.u8    q10, d23, d29
                                            vmull.u8    q11, d2, d4
                                            vmull.u8    q2, d3, d4
    vshll.u16   q0, d16, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16   q0, d16, d30
                                            vrshr.u16   q1, q11, #8
    vmlal.u16   q0, d17, d30
                                            vrshr.u16   q8, q2, #8
                                            vraddhn.u16 d2, q1, q11
                                            vraddhn.u16 d3, q8, q2
    pld         [TMP4, PF_OFFS]
    vld1.32     {d16}, [TMP4], STRIDE
                                            vqadd.u8    q3, q1, q3
    vld1.32     {d17}, [TMP4]
    pld         [TMP4, PF_OFFS]
    vmull.u8    q11, d16, d28
    vmlal.u8    q11, d17, d29
                                            vuzp.8      d6, d7
    vshll.u16   q1, d18, #BILINEAR_INTERPOLATION_BITS
                                            vuzp.8      d6, d7
    vmlsl.u16   q1, d18, d31
                                            vadd.u16    q12, q12, q13
    vmlal.u16   q1, d19, d31
    vshr.u16    q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16    q12, q12, q13
                                            vst1.32     {d6, d7}, [OUT, :128]!
.endm

/* over_8888_8_8888 */
.macro bilinear_over_8888_8_8888_process_last_pixel
    bilinear_interpolate_last_pixel 8888, 8, 8888, over
.endm

.macro bilinear_over_8888_8_8888_process_two_pixels
    bilinear_interpolate_two_pixels 8888, 8, 8888, over
.endm

.macro bilinear_over_8888_8_8888_process_four_pixels
    bilinear_interpolate_four_pixels 8888, 8, 8888, over
.endm

.macro bilinear_over_8888_8_8888_process_pixblock_head
    mov         TMP1, X, asr #16
    add         X, X, UX
    add         TMP1, TOP, TMP1, asl #2
    vld1.32     {d0}, [TMP1], STRIDE
    mov         TMP2, X, asr #16
    add         X, X, UX
    add         TMP2, TOP, TMP2, asl #2
    vld1.32     {d1}, [TMP1]
    mov         TMP3, X, asr #16
    add         X, X, UX
    add         TMP3, TOP, TMP3, asl #2
    vld1.32     {d2}, [TMP2], STRIDE
    mov         TMP4, X, asr #16
    add         X, X, UX
    add         TMP4, TOP, TMP4, asl #2
    vld1.32     {d3}, [TMP2]
    vmull.u8    q2, d0, d28
    vmull.u8    q3, d2, d28
    vmlal.u8    q2, d1, d29
    vmlal.u8    q3, d3, d29
    vshll.u16   q0, d4, #BILINEAR_INTERPOLATION_BITS
    vshll.u16   q1, d6, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16   q0, d4, d30
    vmlsl.u16   q1, d6, d31
    vmlal.u16   q0, d5, d30
    vmlal.u16   q1, d7, d31
    vshrn.u32   d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32   d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
    vld1.32     {d2}, [TMP3], STRIDE
    vld1.32     {d3}, [TMP3]
    pld         [TMP4, PF_OFFS]
    vld1.32     {d4}, [TMP4], STRIDE
    vld1.32     {d5}, [TMP4]
    pld         [TMP4, PF_OFFS]
    vmull.u8    q3, d2, d28
    vmlal.u8    q3, d3, d29
    vmull.u8    q1, d4, d28
    vmlal.u8    q1, d5, d29
    vshr.u16    q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vld1.32     {d22[0]}, [MASK]!
    pld         [MASK, #prefetch_offset]
    vadd.u16    q12, q12, q13
    vmovn.u16   d16, q0
.endm

.macro bilinear_over_8888_8_8888_process_pixblock_tail
    vshll.u16   q9, d6, #BILINEAR_INTERPOLATION_BITS
    vshll.u16   q10, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16   q9, d6, d30
    vmlsl.u16   q10, d2, d31
    vmlal.u16   q9, d7, d30
    vmlal.u16   q10, d3, d31
    vshr.u16    q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16    q12, q12, q13
    vdup.32     d22, d22[0]
    vshrn.u32   d18, q9, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32   d19, q10, #(2 * BILINEAR_INTERPOLATION_BITS)
    vmovn.u16   d17, q9
    vld1.32     {d18, d19}, [OUT, :128]
    pld         [OUT, PF_OFFS]
    vuzp.8      d16, d17
    vuzp.8      d18, d19
    vuzp.8      d16, d17
    vuzp.8      d18, d19
    vmull.u8    q10, d16, d22
    vmull.u8    q11, d17, d22
    vrsra.u16   q10, q10, #8
    vrsra.u16   q11, q11, #8
    vrshrn.u16  d16, q10, #8
    vrshrn.u16  d17, q11, #8
    vdup.32     d22, d17[1]
    vmvn.8      d22, d22
    vmull.u8    q10, d18, d22
    vmull.u8    q11, d19, d22
    vrshr.u16   q9, q10, #8
    vrshr.u16   q0, q11, #8
    vraddhn.u16 d18, q9, q10
    vraddhn.u16 d19, q0, q11
    vqadd.u8    q9, q8, q9
    vuzp.8      d18, d19
    vuzp.8      d18, d19
    vst1.32     {d18, d19}, [OUT, :128]!
.endm

.macro bilinear_over_8888_8_8888_process_pixblock_tail_head
                                            vshll.u16   q9, d6, #BILINEAR_INTERPOLATION_BITS
    mov         TMP1, X, asr #16
    add         X, X, UX
    add         TMP1, TOP, TMP1, asl #2
                                            vshll.u16   q10, d2, #BILINEAR_INTERPOLATION_BITS
    vld1.32     {d0}, [TMP1], STRIDE
    mov         TMP2, X, asr #16
    add         X, X, UX
    add         TMP2, TOP, TMP2, asl #2
                                            vmlsl.u16   q9, d6, d30
                                            vmlsl.u16   q10, d2, d31
    vld1.32     {d1}, [TMP1]
    mov         TMP3, X, asr #16
    add         X, X, UX
    add         TMP3, TOP, TMP3, asl #2
                                            vmlal.u16   q9, d7, d30
                                            vmlal.u16   q10, d3, d31
    vld1.32     {d2}, [TMP2], STRIDE
    mov         TMP4, X, asr #16
    add         X, X, UX
    add         TMP4, TOP, TMP4, asl #2
                                            vshr.u16    q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
                                            vadd.u16    q12, q12, q13
    vld1.32     {d3}, [TMP2]
                                            vdup.32     d22, d22[0]
                                            vshrn.u32   d18, q9, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            vshrn.u32   d19, q10, #(2 * BILINEAR_INTERPOLATION_BITS)
    vmull.u8    q2, d0, d28
    vmull.u8    q3, d2, d28
                                            vmovn.u16   d17, q9
                                            vld1.32     {d18, d19}, [OUT, :128]
                                            pld         [OUT, #(prefetch_offset * 4)]
    vmlal.u8    q2, d1, d29
    vmlal.u8    q3, d3, d29
                                            vuzp.8      d16, d17
                                            vuzp.8      d18, d19
    vshll.u16   q0, d4, #BILINEAR_INTERPOLATION_BITS
    vshll.u16   q1, d6, #BILINEAR_INTERPOLATION_BITS
                                            vuzp.8      d16, d17
                                            vuzp.8      d18, d19
    vmlsl.u16   q0, d4, d30
    vmlsl.u16   q1, d6, d31
                                            vmull.u8    q10, d16, d22
                                            vmull.u8    q11, d17, d22
    vmlal.u16   q0, d5, d30
    vmlal.u16   q1, d7, d31
                                            vrsra.u16   q10, q10, #8
                                            vrsra.u16   q11, q11, #8
    vshrn.u32   d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32   d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
                                            vrshrn.u16  d16, q10, #8
                                            vrshrn.u16  d17, q11, #8
    vld1.32     {d2}, [TMP3], STRIDE
                                            vdup.32     d22, d17[1]
    vld1.32     {d3}, [TMP3]
                                            vmvn.8      d22, d22
    pld         [TMP4, PF_OFFS]
    vld1.32     {d4}, [TMP4], STRIDE
                                            vmull.u8    q10, d18, d22
                                            vmull.u8    q11, d19, d22
    vld1.32     {d5}, [TMP4]
    pld         [TMP4, PF_OFFS]
    vmull.u8    q3, d2, d28
                                            vrshr.u16   q9, q10, #8
                                            vrshr.u16   q15, q11, #8
    vmlal.u8    q3, d3, d29
    vmull.u8    q1, d4, d28
                                            vraddhn.u16 d18, q9, q10
                                            vraddhn.u16 d19, q15, q11
    vmlal.u8    q1, d5, d29
    vshr.u16    q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
                                            vqadd.u8    q9, q8, q9
    vld1.32     {d22[0]}, [MASK]!
                                            vuzp.8      d18, d19
    vadd.u16    q12, q12, q13
                                            vuzp.8      d18, d19
    vmovn.u16   d16, q0
                                            vst1.32     {d18, d19}, [OUT, :128]!
.endm

/* add_8888_8888 */
.macro bilinear_add_8888_8888_process_last_pixel
    bilinear_interpolate_last_pixel 8888, x, 8888, add
.endm

.macro bilinear_add_8888_8888_process_two_pixels
    bilinear_interpolate_two_pixels 8888, x, 8888, add
.endm

.macro bilinear_add_8888_8888_process_four_pixels
    bilinear_interpolate_four_pixels 8888, x, 8888, add
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
