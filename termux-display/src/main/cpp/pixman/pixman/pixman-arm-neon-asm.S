/*
 * Copyright © 2009 Nokia Corporation
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
 */

/*
 * This file contains implementations of NEON optimized pixel processing
 * functions. There is no full and detailed tutorial, but some functions
 * (those which are exposing some new or interesting features) are
 * extensively commented and can be used as examples.
 *
 * You may want to have a look at the comments for following functions:
 *  - pixman_composite_over_8888_0565_asm_neon
 *  - pixman_composite_over_n_8_0565_asm_neon
 */

/* Prevent the stack from becoming executable for no reason... */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

    .text
    .fpu neon
    .arch armv7a
    .object_arch armv4
    .eabi_attribute 10, 0 /* suppress Tag_FP_arch */
    .eabi_attribute 12, 0 /* suppress Tag_Advanced_SIMD_arch */
    .arm
    .altmacro
    .p2align 2

#include "pixman-private.h"
#include "pixman-arm-asm.h"
#include "pixman-arm-neon-asm.h"

/* Global configuration options and preferences */

/*
 * The code can optionally make use of unaligned memory accesses to improve
 * performance of handling leading/trailing pixels for each scanline.
 * Configuration variable RESPECT_STRICT_ALIGNMENT can be set to 0 for
 * example in linux if unaligned memory accesses are not configured to
 * generate.exceptions.
 */
.set RESPECT_STRICT_ALIGNMENT, 1

/*
 * Set default prefetch type. There is a choice between the following options:
 *
 * PREFETCH_TYPE_NONE (may be useful for the ARM cores where PLD is set to work
 * as NOP to workaround some HW bugs or for whatever other reason)
 *
 * PREFETCH_TYPE_SIMPLE (may be useful for simple single-issue ARM cores where
 * advanced prefetch intruduces heavy overhead)
 *
 * PREFETCH_TYPE_ADVANCED (useful for superscalar cores such as ARM Cortex-A8
 * which can run ARM and NEON instructions simultaneously so that extra ARM
 * instructions do not add (many) extra cycles, but improve prefetch efficiency)
 *
 * Note: some types of function can't support advanced prefetch and fallback
 *       to simple one (those which handle 24bpp pixels)
 */
.set PREFETCH_TYPE_DEFAULT, PREFETCH_TYPE_ADVANCED

/* Prefetch distance in pixels for simple prefetch */
.set PREFETCH_DISTANCE_SIMPLE, 64

/*
 * Implementation of pixman_composite_over_8888_0565_asm_neon
 *
 * This function takes a8r8g8b8 source buffer, r5g6b5 destination buffer and
 * performs OVER compositing operation. Function fast_composite_over_8888_0565
 * from pixman-fast-path.c does the same in C and can be used as a reference.
 *
 * First we need to have some NEON assembly code which can do the actual
 * operation on the pixels and provide it to the template macro.
 *
 * Template macro quite conveniently takes care of emitting all the necessary
 * code for memory reading and writing (including quite tricky cases of
 * handling unaligned leading/trailing pixels), so we only need to deal with
 * the data in NEON registers.
 *
 * NEON registers allocation in general is recommented to be the following:
 * d0,  d1,  d2,  d3  - contain loaded source pixel data
 * d4,  d5,  d6,  d7  - contain loaded destination pixels (if they are needed)
 * d24, d25, d26, d27 - contain loading mask pixel data (if mask is used)
 * d28, d29, d30, d31 - place for storing the result (destination pixels)
 *
 * As can be seen above, four 64-bit NEON registers are used for keeping
 * intermediate pixel data and up to 8 pixels can be processed in one step
 * for 32bpp formats (16 pixels for 16bpp, 32 pixels for 8bpp).
 *
 * This particular function uses the following registers allocation:
 * d0,  d1,  d2,  d3  - contain loaded source pixel data
 * d4,  d5            - contain loaded destination pixels (they are needed)
 * d28, d29           - place for storing the result (destination pixels)
 */

/*
 * Step one. We need to have some code to do some arithmetics on pixel data.
 * This is implemented as a pair of macros: '*_head' and '*_tail'. When used
 * back-to-back, they take pixel data from {d0, d1, d2, d3} and {d4, d5},
 * perform all the needed calculations and write the result to {d28, d29}.
 * The rationale for having two macros and not just one will be explained
 * later. In practice, any single monolitic function which does the work can
 * be split into two parts in any arbitrary way without affecting correctness.
 *
 * There is one special trick here too. Common template macro can optionally
 * make our life a bit easier by doing R, G, B, A color components
 * deinterleaving for 32bpp pixel formats (and this feature is used in
 * 'pixman_composite_over_8888_0565_asm_neon' function). So it means that
 * instead of having 8 packed pixels in {d0, d1, d2, d3} registers, we
 * actually use d0 register for blue channel (a vector of eight 8-bit
 * values), d1 register for green, d2 for red and d3 for alpha. This
 * simple conversion can be also done with a few NEON instructions:
 *
 * Packed to planar conversion:
 *  vuzp.8 d0, d1
 *  vuzp.8 d2, d3
 *  vuzp.8 d1, d3
 *  vuzp.8 d0, d2
 *
 * Planar to packed conversion:
 *  vzip.8 d0, d2
 *  vzip.8 d1, d3
 *  vzip.8 d2, d3
 *  vzip.8 d0, d1
 *
 * But pixel can be loaded directly in planar format using VLD4.8 NEON
 * instruction. It is 1 cycle slower than VLD1.32, so this is not always
 * desirable, that's why deinterleaving is optional.
 *
 * But anyway, here is the code:
 */
.macro pixman_composite_over_8888_0565_process_pixblock_head
    /* convert 8 r5g6b5 pixel data from {d4, d5} to planar 8-bit format
       and put data into d6 - red, d7 - green, d30 - blue */
    vshrn.u16   d6, q2, #8
    vshrn.u16   d7, q2, #3
    vsli.u16    q2, q2, #5
    vsri.u8     d6, d6, #5
    vmvn.8      d3, d3      /* invert source alpha */
    vsri.u8     d7, d7, #6
    vshrn.u16   d30, q2, #2
    /* now do alpha blending, storing results in 8-bit planar format
       into d16 - red, d19 - green, d18 - blue */
    vmull.u8    q10, d3, d6
    vmull.u8    q11, d3, d7
    vmull.u8    q12, d3, d30
    vrshr.u16   q13, q10, #8
    vrshr.u16   q3, q11, #8
    vrshr.u16   q15, q12, #8
    vraddhn.u16 d20, q10, q13
    vraddhn.u16 d23, q11, q3
    vraddhn.u16 d22, q12, q15
.endm

.macro pixman_composite_over_8888_0565_process_pixblock_tail
    /* ... continue alpha blending */
    vqadd.u8    d16, d2, d20
    vqadd.u8    q9, q0, q11
    /* convert the result to r5g6b5 and store it into {d28, d29} */
    vshll.u8    q14, d16, #8
    vshll.u8    q8, d19, #8
    vshll.u8    q9, d18, #8
    vsri.u16    q14, q8, #5
    vsri.u16    q14, q9, #11
.endm

/*
 * OK, now we got almost everything that we need. Using the above two
 * macros, the work can be done right. But now we want to optimize
 * it a bit. ARM Cortex-A8 is an in-order core, and benefits really
 * a lot from good code scheduling and software pipelining.
 *
 * Let's construct some code, which will run in the core main loop.
 * Some pseudo-code of the main loop will look like this:
 *   head
 *   while (...) {
 *     tail
 *     head
 *   }
 *   tail
 *
 * It may look a bit weird, but this setup allows to hide instruction
 * latencies better and also utilize dual-issue capability more
 * efficiently (make pairs of load-store and ALU instructions).
 *
 * So what we need now is a '*_tail_head' macro, which will be used
 * in the core main loop. A trivial straightforward implementation
 * of this macro would look like this:
 *
 *   pixman_composite_over_8888_0565_process_pixblock_tail
 *   vst1.16     {d28, d29}, [DST_W, :128]!
 *   vld1.16     {d4, d5}, [DST_R, :128]!
 *   vld4.32     {d0, d1, d2, d3}, [SRC]!
 *   pixman_composite_over_8888_0565_process_pixblock_head
 *   cache_preload 8, 8
 *
 * Now it also got some VLD/VST instructions. We simply can't move from
 * processing one block of pixels to the other one with just arithmetics.
 * The previously processed data needs to be written to memory and new
 * data needs to be fetched. Fortunately, this main loop does not deal
 * with partial leading/trailing pixels and can load/store a full block
 * of pixels in a bulk. Additionally, destination buffer is already
 * 16 bytes aligned here (which is good for performance).
 *
 * New things here are DST_R, DST_W, SRC and MASK identifiers. These
 * are the aliases for ARM registers which are used as pointers for
 * accessing data. We maintain separate pointers for reading and writing
 * destination buffer (DST_R and DST_W).
 *
 * Another new thing is 'cache_preload' macro. It is used for prefetching
 * data into CPU L2 cache and improve performance when dealing with large
 * images which are far larger than cache size. It uses one argument
 * (actually two, but they need to be the same here) - number of pixels
 * in a block. Looking into 'pixman-arm-neon-asm.h' can provide some
 * details about this macro. Moreover, if good performance is needed
 * the code from this macro needs to be copied into '*_tail_head' macro
 * and mixed with the rest of code for optimal instructions scheduling.
 * We are actually doing it below.
 *
 * Now after all the explanations, here is the optimized code.
 * Different instruction streams (originaling from '*_head', '*_tail'
 * and 'cache_preload' macro) use different indentation levels for
 * better readability. Actually taking the code from one of these
 * indentation levels and ignoring a few VLD/VST instructions would
 * result in exactly the code from '*_head', '*_tail' or 'cache_preload'
 * macro!
 */

#if 1

.macro pixman_composite_over_8888_0565_process_pixblock_tail_head
        vqadd.u8    d16, d2, d20
    vld1.16     {d4, d5}, [DST_R, :128]!
        vqadd.u8    q9, q0, q11
    vshrn.u16   d6, q2, #8
    fetch_src_pixblock
    vshrn.u16   d7, q2, #3
    vsli.u16    q2, q2, #5
        vshll.u8    q14, d16, #8
                                    PF add PF_X, PF_X, #8
        vshll.u8    q8, d19, #8
                                    PF tst PF_CTL, #0xF
    vsri.u8     d6, d6, #5
                                    PF addne PF_X, PF_X, #8
    vmvn.8      d3, d3
                                    PF subne PF_CTL, PF_CTL, #1
    vsri.u8     d7, d7, #6
    vshrn.u16   d30, q2, #2
    vmull.u8    q10, d3, d6
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
    vmull.u8    q11, d3, d7
    vmull.u8    q12, d3, d30
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
        vsri.u16    q14, q8, #5
                                    PF cmp PF_X, ORIG_W
        vshll.u8    q9, d18, #8
    vrshr.u16   q13, q10, #8
                                    PF subge PF_X, PF_X, ORIG_W
    vrshr.u16   q3, q11, #8
    vrshr.u16   q15, q12, #8
                                    PF subges PF_CTL, PF_CTL, #0x10
        vsri.u16    q14, q9, #11
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
    vraddhn.u16 d20, q10, q13
    vraddhn.u16 d23, q11, q3
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
    vraddhn.u16 d22, q12, q15
        vst1.16     {d28, d29}, [DST_W, :128]!
.endm

#else

/* If we did not care much about the performance, we would just use this... */
.macro pixman_composite_over_8888_0565_process_pixblock_tail_head
    pixman_composite_over_8888_0565_process_pixblock_tail
    vst1.16     {d28, d29}, [DST_W, :128]!
    vld1.16     {d4, d5}, [DST_R, :128]!
    fetch_src_pixblock
    pixman_composite_over_8888_0565_process_pixblock_head
    cache_preload 8, 8
.endm

#endif

/*
 * And now the final part. We are using 'generate_composite_function' macro
 * to put all the stuff together. We are specifying the name of the function
 * which we want to get, number of bits per pixel for the source, mask and
 * destination (0 if unused, like mask in this case). Next come some bit
 * flags:
 *   FLAG_DST_READWRITE      - tells that the destination buffer is both read
 *                             and written, for write-only buffer we would use
 *                             FLAG_DST_WRITEONLY flag instead
 *   FLAG_DEINTERLEAVE_32BPP - tells that we prefer to work with planar data
 *                             and separate color channels for 32bpp format.
 * The next things are:
 *  - the number of pixels processed per iteration (8 in this case, because
 *    that's the maximum what can fit into four 64-bit NEON registers).
 *  - prefetch distance, measured in pixel blocks. In this case it is 5 times
 *    by 8 pixels. That would be 40 pixels, or up to 160 bytes. Optimal
 *    prefetch distance can be selected by running some benchmarks.
 *
 * After that we specify some macros, these are 'default_init',
 * 'default_cleanup' here which are empty (but it is possible to have custom
 * init/cleanup macros to be able to save/restore some extra NEON registers
 * like d8-d15 or do anything else) followed by
 * 'pixman_composite_over_8888_0565_process_pixblock_head',
 * 'pixman_composite_over_8888_0565_process_pixblock_tail' and
 * 'pixman_composite_over_8888_0565_process_pixblock_tail_head'
 * which we got implemented above.
 *
 * The last part is the NEON registers allocation scheme.
 */
generate_composite_function \
    pixman_composite_over_8888_0565_asm_neon, 32, 0, 16, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_over_8888_0565_process_pixblock_head, \
    pixman_composite_over_8888_0565_process_pixblock_tail, \
    pixman_composite_over_8888_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    24  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_over_n_0565_process_pixblock_head
    /* convert 8 r5g6b5 pixel data from {d4, d5} to planar 8-bit format
       and put data into d6 - red, d7 - green, d30 - blue */
    vshrn.u16   d6, q2, #8
    vshrn.u16   d7, q2, #3
    vsli.u16    q2, q2, #5
    vsri.u8     d6, d6, #5
    vsri.u8     d7, d7, #6
    vshrn.u16   d30, q2, #2
    /* now do alpha blending, storing results in 8-bit planar format
       into d16 - red, d19 - green, d18 - blue */
    vmull.u8    q10, d3, d6
    vmull.u8    q11, d3, d7
    vmull.u8    q12, d3, d30
    vrshr.u16   q13, q10, #8
    vrshr.u16   q3, q11, #8
    vrshr.u16   q15, q12, #8
    vraddhn.u16 d20, q10, q13
    vraddhn.u16 d23, q11, q3
    vraddhn.u16 d22, q12, q15
.endm

.macro pixman_composite_over_n_0565_process_pixblock_tail
    /* ... continue alpha blending */
    vqadd.u8    d16, d2, d20
    vqadd.u8    q9, q0, q11
    /* convert the result to r5g6b5 and store it into {d28, d29} */
    vshll.u8    q14, d16, #8
    vshll.u8    q8, d19, #8
    vshll.u8    q9, d18, #8
    vsri.u16    q14, q8, #5
    vsri.u16    q14, q9, #11
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_n_0565_process_pixblock_tail_head
    pixman_composite_over_n_0565_process_pixblock_tail
    vld1.16     {d4, d5}, [DST_R, :128]!
    vst1.16     {d28, d29}, [DST_W, :128]!
    pixman_composite_over_n_0565_process_pixblock_head
    cache_preload 8, 8
.endm

.macro pixman_composite_over_n_0565_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d3[0]}, [DUMMY]
    vdup.8      d0, d3[0]
    vdup.8      d1, d3[1]
    vdup.8      d2, d3[2]
    vdup.8      d3, d3[3]
    vmvn.8      d3, d3      /* invert source alpha */
.endm

generate_composite_function \
    pixman_composite_over_n_0565_asm_neon, 0, 0, 16, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_n_0565_init, \
    default_cleanup, \
    pixman_composite_over_n_0565_process_pixblock_head, \
    pixman_composite_over_n_0565_process_pixblock_tail, \
    pixman_composite_over_n_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    24  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_8888_0565_process_pixblock_head
    vshll.u8    q8, d1, #8
    vshll.u8    q14, d2, #8
    vshll.u8    q9, d0, #8
.endm

.macro pixman_composite_src_8888_0565_process_pixblock_tail
    vsri.u16    q14, q8, #5
    vsri.u16    q14, q9, #11
.endm

.macro pixman_composite_src_8888_0565_process_pixblock_tail_head
        vsri.u16    q14, q8, #5
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
    fetch_src_pixblock
                                    PF addne PF_X, PF_X, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vsri.u16    q14, q9, #11
                                    PF cmp PF_X, ORIG_W
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
    vshll.u8    q8, d1, #8
        vst1.16     {d28, d29}, [DST_W, :128]!
                                    PF subge PF_X, PF_X, ORIG_W
                                    PF subges PF_CTL, PF_CTL, #0x10
    vshll.u8    q14, d2, #8
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
    vshll.u8    q9, d0, #8
.endm

generate_composite_function \
    pixman_composite_src_8888_0565_asm_neon, 32, 0, 16, \
    FLAG_DST_WRITEONLY | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_8888_0565_process_pixblock_head, \
    pixman_composite_src_8888_0565_process_pixblock_tail, \
    pixman_composite_src_8888_0565_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_src_0565_8888_process_pixblock_head
    vshrn.u16   d30, q0, #8
    vshrn.u16   d29, q0, #3
    vsli.u16    q0, q0, #5
    vmov.u8     d31, #255
    vsri.u8     d30, d30, #5
    vsri.u8     d29, d29, #6
    vshrn.u16   d28, q0, #2
.endm

.macro pixman_composite_src_0565_8888_process_pixblock_tail
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_src_0565_8888_process_pixblock_tail_head
    pixman_composite_src_0565_8888_process_pixblock_tail
    vst4.8     {d28, d29, d30, d31}, [DST_W, :128]!
    fetch_src_pixblock
    pixman_composite_src_0565_8888_process_pixblock_head
    cache_preload 8, 8
.endm

generate_composite_function \
    pixman_composite_src_0565_8888_asm_neon, 16, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_0565_8888_process_pixblock_head, \
    pixman_composite_src_0565_8888_process_pixblock_tail, \
    pixman_composite_src_0565_8888_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_add_8_8_process_pixblock_head
    vqadd.u8    q14, q0, q2
    vqadd.u8    q15, q1, q3
.endm

.macro pixman_composite_add_8_8_process_pixblock_tail
.endm

.macro pixman_composite_add_8_8_process_pixblock_tail_head
    fetch_src_pixblock
                                    PF add PF_X, PF_X, #32
                                    PF tst PF_CTL, #0xF
    vld1.8      {d4, d5, d6, d7}, [DST_R, :128]!
                                    PF addne PF_X, PF_X, #32
                                    PF subne PF_CTL, PF_CTL, #1
        vst1.8      {d28, d29, d30, d31}, [DST_W, :128]!
                                    PF cmp PF_X, ORIG_W
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
                                    PF subge PF_X, PF_X, ORIG_W
                                    PF subges PF_CTL, PF_CTL, #0x10
    vqadd.u8    q14, q0, q2
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
    vqadd.u8    q15, q1, q3
.endm

generate_composite_function \
    pixman_composite_add_8_8_asm_neon, 8, 0, 8, \
    FLAG_DST_READWRITE, \
    32, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_add_8_8_process_pixblock_head, \
    pixman_composite_add_8_8_process_pixblock_tail, \
    pixman_composite_add_8_8_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_add_8888_8888_process_pixblock_tail_head
    fetch_src_pixblock
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
    vld1.32     {d4, d5, d6, d7}, [DST_R, :128]!
                                    PF addne PF_X, PF_X, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vst1.32     {d28, d29, d30, d31}, [DST_W, :128]!
                                    PF cmp PF_X, ORIG_W
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
                                    PF subge PF_X, PF_X, ORIG_W
                                    PF subges PF_CTL, PF_CTL, #0x10
    vqadd.u8    q14, q0, q2
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
    vqadd.u8    q15, q1, q3
.endm

generate_composite_function \
    pixman_composite_add_8888_8888_asm_neon, 32, 0, 32, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_add_8_8_process_pixblock_head, \
    pixman_composite_add_8_8_process_pixblock_tail, \
    pixman_composite_add_8888_8888_process_pixblock_tail_head

generate_composite_function_single_scanline \
    pixman_composite_scanline_add_asm_neon, 32, 0, 32, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_add_8_8_process_pixblock_head, \
    pixman_composite_add_8_8_process_pixblock_tail, \
    pixman_composite_add_8888_8888_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_out_reverse_8888_8888_process_pixblock_head
    vmvn.8      d24, d3  /* get inverted alpha */
    /* do alpha blending */
    vmull.u8    q8, d24, d4
    vmull.u8    q9, d24, d5
    vmull.u8    q10, d24, d6
    vmull.u8    q11, d24, d7
.endm

.macro pixman_composite_out_reverse_8888_8888_process_pixblock_tail
    vrshr.u16   q14, q8, #8
    vrshr.u16   q15, q9, #8
    vrshr.u16   q12, q10, #8
    vrshr.u16   q13, q11, #8
    vraddhn.u16 d28, q14, q8
    vraddhn.u16 d29, q15, q9
    vraddhn.u16 d30, q12, q10
    vraddhn.u16 d31, q13, q11
.endm

.macro pixman_composite_out_reverse_8888_8888_process_pixblock_tail_head
    vld4.8      {d4, d5, d6, d7}, [DST_R, :128]!
        vrshr.u16   q14, q8, #8
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
        vrshr.u16   q15, q9, #8
        vrshr.u16   q12, q10, #8
        vrshr.u16   q13, q11, #8
                                    PF addne PF_X, PF_X, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vraddhn.u16 d28, q14, q8
        vraddhn.u16 d29, q15, q9
                                    PF cmp PF_X, ORIG_W
        vraddhn.u16 d30, q12, q10
        vraddhn.u16 d31, q13, q11
    fetch_src_pixblock
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
    vmvn.8      d22, d3
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
        vst4.8      {d28, d29, d30, d31}, [DST_W, :128]!
                                    PF subge PF_X, PF_X, ORIG_W
    vmull.u8    q8, d22, d4
                                    PF subges PF_CTL, PF_CTL, #0x10
    vmull.u8    q9, d22, d5
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
    vmull.u8    q10, d22, d6
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
    vmull.u8    q11, d22, d7
.endm

generate_composite_function_single_scanline \
    pixman_composite_scanline_out_reverse_asm_neon, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_out_reverse_8888_8888_process_pixblock_head, \
    pixman_composite_out_reverse_8888_8888_process_pixblock_tail, \
    pixman_composite_out_reverse_8888_8888_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_over_8888_8888_process_pixblock_head
    pixman_composite_out_reverse_8888_8888_process_pixblock_head
.endm

.macro pixman_composite_over_8888_8888_process_pixblock_tail
    pixman_composite_out_reverse_8888_8888_process_pixblock_tail
    vqadd.u8    q14, q0, q14
    vqadd.u8    q15, q1, q15
.endm

.macro pixman_composite_over_8888_8888_process_pixblock_tail_head
    vld4.8      {d4, d5, d6, d7}, [DST_R, :128]!
        vrshr.u16   q14, q8, #8
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
        vrshr.u16   q15, q9, #8
        vrshr.u16   q12, q10, #8
        vrshr.u16   q13, q11, #8
                                    PF addne PF_X, PF_X, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vraddhn.u16 d28, q14, q8
        vraddhn.u16 d29, q15, q9
                                    PF cmp PF_X, ORIG_W
        vraddhn.u16 d30, q12, q10
        vraddhn.u16 d31, q13, q11
        vqadd.u8    q14, q0, q14
        vqadd.u8    q15, q1, q15
    fetch_src_pixblock
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
    vmvn.8      d22, d3
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
        vst4.8      {d28, d29, d30, d31}, [DST_W, :128]!
                                    PF subge PF_X, PF_X, ORIG_W
    vmull.u8    q8, d22, d4
                                    PF subges PF_CTL, PF_CTL, #0x10
    vmull.u8    q9, d22, d5
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
    vmull.u8    q10, d22, d6
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
    vmull.u8    q11, d22, d7
.endm

generate_composite_function \
    pixman_composite_over_8888_8888_asm_neon, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_over_8888_8888_process_pixblock_head, \
    pixman_composite_over_8888_8888_process_pixblock_tail, \
    pixman_composite_over_8888_8888_process_pixblock_tail_head

generate_composite_function_single_scanline \
    pixman_composite_scanline_over_asm_neon, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_over_8888_8888_process_pixblock_head, \
    pixman_composite_over_8888_8888_process_pixblock_tail, \
    pixman_composite_over_8888_8888_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_over_n_8888_process_pixblock_head
    /* deinterleaved source pixels in {d0, d1, d2, d3} */
    /* inverted alpha in {d24} */
    /* destination pixels in {d4, d5, d6, d7} */
    vmull.u8    q8, d24, d4
    vmull.u8    q9, d24, d5
    vmull.u8    q10, d24, d6
    vmull.u8    q11, d24, d7
.endm

.macro pixman_composite_over_n_8888_process_pixblock_tail
    vrshr.u16   q14, q8, #8
    vrshr.u16   q15, q9, #8
    vrshr.u16   q2, q10, #8
    vrshr.u16   q3, q11, #8
    vraddhn.u16 d28, q14, q8
    vraddhn.u16 d29, q15, q9
    vraddhn.u16 d30, q2, q10
    vraddhn.u16 d31, q3, q11
    vqadd.u8    q14, q0, q14
    vqadd.u8    q15, q1, q15
.endm

.macro pixman_composite_over_n_8888_process_pixblock_tail_head
        vrshr.u16   q14, q8, #8
        vrshr.u16   q15, q9, #8
        vrshr.u16   q2, q10, #8
        vrshr.u16   q3, q11, #8
        vraddhn.u16 d28, q14, q8
        vraddhn.u16 d29, q15, q9
        vraddhn.u16 d30, q2, q10
        vraddhn.u16 d31, q3, q11
    vld4.8      {d4, d5, d6, d7}, [DST_R, :128]!
        vqadd.u8    q14, q0, q14
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0x0F
                                    PF addne PF_X, PF_X, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vqadd.u8    q15, q1, q15
                                    PF cmp PF_X, ORIG_W
    vmull.u8    q8, d24, d4
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
    vmull.u8    q9, d24, d5
                                    PF subge PF_X, PF_X, ORIG_W
    vmull.u8    q10, d24, d6
                                    PF subges PF_CTL, PF_CTL, #0x10
    vmull.u8    q11, d24, d7
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
        vst4.8      {d28, d29, d30, d31}, [DST_W, :128]!
.endm

.macro pixman_composite_over_n_8888_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d3[0]}, [DUMMY]
    vdup.8      d0, d3[0]
    vdup.8      d1, d3[1]
    vdup.8      d2, d3[2]
    vdup.8      d3, d3[3]
    vmvn.8      d24, d3  /* get inverted alpha */
.endm

generate_composite_function \
    pixman_composite_over_n_8888_asm_neon, 0, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_n_8888_init, \
    default_cleanup, \
    pixman_composite_over_8888_8888_process_pixblock_head, \
    pixman_composite_over_8888_8888_process_pixblock_tail, \
    pixman_composite_over_n_8888_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_over_reverse_n_8888_process_pixblock_tail_head
        vrshr.u16   q14, q8, #8
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
        vrshr.u16   q15, q9, #8
        vrshr.u16   q12, q10, #8
        vrshr.u16   q13, q11, #8
                                    PF addne PF_X, PF_X, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vraddhn.u16 d28, q14, q8
        vraddhn.u16 d29, q15, q9
                                    PF cmp PF_X, ORIG_W
        vraddhn.u16 d30, q12, q10
        vraddhn.u16 d31, q13, q11
        vqadd.u8    q14, q0, q14
        vqadd.u8    q15, q1, q15
    vld4.8      {d0, d1, d2, d3}, [DST_R, :128]!
    vmvn.8      d22, d3
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
        vst4.8      {d28, d29, d30, d31}, [DST_W, :128]!
                                    PF subge PF_X, PF_X, ORIG_W
    vmull.u8    q8, d22, d4
                                    PF subges PF_CTL, PF_CTL, #0x10
    vmull.u8    q9, d22, d5
    vmull.u8    q10, d22, d6
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
    vmull.u8    q11, d22, d7
.endm

.macro pixman_composite_over_reverse_n_8888_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d7[0]}, [DUMMY]
    vdup.8      d4, d7[0]
    vdup.8      d5, d7[1]
    vdup.8      d6, d7[2]
    vdup.8      d7, d7[3]
.endm

generate_composite_function \
    pixman_composite_over_reverse_n_8888_asm_neon, 0, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_reverse_n_8888_init, \
    default_cleanup, \
    pixman_composite_over_8888_8888_process_pixblock_head, \
    pixman_composite_over_8888_8888_process_pixblock_tail, \
    pixman_composite_over_reverse_n_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    0,  /* dst_r_basereg */ \
    4,  /* src_basereg   */ \
    24  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_over_8888_8_0565_process_pixblock_head
    vmull.u8    q0,  d24, d8    /* IN for SRC pixels (part1) */
    vmull.u8    q1,  d24, d9
    vmull.u8    q6,  d24, d10
    vmull.u8    q7,  d24, d11
        vshrn.u16   d6,  q2, #8 /* convert DST_R data to 32-bpp (part1) */
        vshrn.u16   d7,  q2, #3
        vsli.u16    q2,  q2, #5
    vrshr.u16   q8,  q0,  #8    /* IN for SRC pixels (part2) */
    vrshr.u16   q9,  q1,  #8
    vrshr.u16   q10, q6,  #8
    vrshr.u16   q11, q7,  #8
    vraddhn.u16 d0,  q0,  q8
    vraddhn.u16 d1,  q1,  q9
    vraddhn.u16 d2,  q6,  q10
    vraddhn.u16 d3,  q7,  q11
        vsri.u8     d6,  d6, #5 /* convert DST_R data to 32-bpp (part2) */
        vsri.u8     d7,  d7, #6
    vmvn.8      d3,  d3
        vshrn.u16   d30, q2, #2
    vmull.u8    q8,  d3, d6     /* now do alpha blending */
    vmull.u8    q9,  d3, d7
    vmull.u8    q10, d3, d30
.endm

.macro pixman_composite_over_8888_8_0565_process_pixblock_tail
    /* 3 cycle bubble (after vmull.u8) */
    vrshr.u16   q13, q8,  #8
    vrshr.u16   q11, q9,  #8
    vrshr.u16   q15, q10, #8
    vraddhn.u16 d16, q8,  q13
    vraddhn.u16 d27, q9,  q11
    vraddhn.u16 d26, q10, q15
    vqadd.u8    d16, d2,  d16
    /* 1 cycle bubble */
    vqadd.u8    q9,  q0,  q13
    vshll.u8    q14, d16, #8    /* convert to 16bpp */
    vshll.u8    q8,  d19, #8
    vshll.u8    q9,  d18, #8
    vsri.u16    q14, q8,  #5
    /* 1 cycle bubble */
    vsri.u16    q14, q9,  #11
.endm

.macro pixman_composite_over_8888_8_0565_process_pixblock_tail_head
    vld1.16     {d4, d5}, [DST_R, :128]!
    vshrn.u16   d6,  q2,  #8
    fetch_mask_pixblock
    vshrn.u16   d7,  q2,  #3
    fetch_src_pixblock
    vmull.u8    q6,  d24, d10
        vrshr.u16   q13, q8,  #8
        vrshr.u16   q11, q9,  #8
        vrshr.u16   q15, q10, #8
        vraddhn.u16 d16, q8,  q13
        vraddhn.u16 d27, q9,  q11
        vraddhn.u16 d26, q10, q15
        vqadd.u8    d16, d2,  d16
    vmull.u8    q1,  d24, d9
        vqadd.u8    q9,  q0,  q13
        vshll.u8    q14, d16, #8
    vmull.u8    q0,  d24, d8
        vshll.u8    q8,  d19, #8
        vshll.u8    q9,  d18, #8
        vsri.u16    q14, q8,  #5
    vmull.u8    q7,  d24, d11
        vsri.u16    q14, q9,  #11

    cache_preload 8, 8

    vsli.u16    q2,  q2,  #5
    vrshr.u16   q8,  q0,  #8
    vrshr.u16   q9,  q1,  #8
    vrshr.u16   q10, q6,  #8
    vrshr.u16   q11, q7,  #8
    vraddhn.u16 d0,  q0,  q8
    vraddhn.u16 d1,  q1,  q9
    vraddhn.u16 d2,  q6,  q10
    vraddhn.u16 d3,  q7,  q11
    vsri.u8     d6,  d6,  #5
    vsri.u8     d7,  d7,  #6
    vmvn.8      d3,  d3
    vshrn.u16   d30, q2,  #2
    vst1.16     {d28, d29}, [DST_W, :128]!
    vmull.u8    q8,  d3,  d6
    vmull.u8    q9,  d3,  d7
    vmull.u8    q10, d3,  d30
.endm

generate_composite_function \
    pixman_composite_over_8888_8_0565_asm_neon, 32, 8, 16, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_over_8888_8_0565_process_pixblock_head, \
    pixman_composite_over_8888_8_0565_process_pixblock_tail, \
    pixman_composite_over_8888_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    24  /* mask_basereg  */

/******************************************************************************/

/*
 * This function needs a special initialization of solid mask.
 * Solid source pixel data is fetched from stack at ARGS_STACK_OFFSET
 * offset, split into color components and replicated in d8-d11
 * registers. Additionally, this function needs all the NEON registers,
 * so it has to save d8-d15 registers which are callee saved according
 * to ABI. These registers are restored from 'cleanup' macro. All the
 * other NEON registers are caller saved, so can be clobbered freely
 * without introducing any problems.
 */
.macro pixman_composite_over_n_8_0565_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vpush       {d8-d15}
    vld1.32     {d11[0]}, [DUMMY]
    vdup.8      d8, d11[0]
    vdup.8      d9, d11[1]
    vdup.8      d10, d11[2]
    vdup.8      d11, d11[3]
.endm

.macro pixman_composite_over_n_8_0565_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_over_n_8_0565_asm_neon, 0, 8, 16, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_n_8_0565_init, \
    pixman_composite_over_n_8_0565_cleanup, \
    pixman_composite_over_8888_8_0565_process_pixblock_head, \
    pixman_composite_over_8888_8_0565_process_pixblock_tail, \
    pixman_composite_over_8888_8_0565_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_over_8888_n_0565_init
    add         DUMMY, sp, #(ARGS_STACK_OFFSET + 8)
    vpush       {d8-d15}
    vld1.32     {d24[0]}, [DUMMY]
    vdup.8      d24, d24[3]
.endm

.macro pixman_composite_over_8888_n_0565_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_over_8888_n_0565_asm_neon, 32, 0, 16, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_8888_n_0565_init, \
    pixman_composite_over_8888_n_0565_cleanup, \
    pixman_composite_over_8888_8_0565_process_pixblock_head, \
    pixman_composite_over_8888_8_0565_process_pixblock_tail, \
    pixman_composite_over_8888_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    24  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_0565_0565_process_pixblock_head
.endm

.macro pixman_composite_src_0565_0565_process_pixblock_tail
.endm

.macro pixman_composite_src_0565_0565_process_pixblock_tail_head
    vst1.16 {d0, d1, d2, d3}, [DST_W, :128]!
    fetch_src_pixblock
    cache_preload 16, 16
.endm

generate_composite_function \
    pixman_composite_src_0565_0565_asm_neon, 16, 0, 16, \
    FLAG_DST_WRITEONLY, \
    16, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_0565_0565_process_pixblock_head, \
    pixman_composite_src_0565_0565_process_pixblock_tail, \
    pixman_composite_src_0565_0565_process_pixblock_tail_head, \
    0, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_n_8_process_pixblock_head
.endm

.macro pixman_composite_src_n_8_process_pixblock_tail
.endm

.macro pixman_composite_src_n_8_process_pixblock_tail_head
    vst1.8  {d0, d1, d2, d3}, [DST_W, :128]!
.endm

.macro pixman_composite_src_n_8_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d0[0]}, [DUMMY]
    vsli.u64    d0, d0, #8
    vsli.u64    d0, d0, #16
    vsli.u64    d0, d0, #32
    vorr        d1, d0, d0
    vorr        q1, q0, q0
.endm

.macro pixman_composite_src_n_8_cleanup
.endm

generate_composite_function \
    pixman_composite_src_n_8_asm_neon, 0, 0, 8, \
    FLAG_DST_WRITEONLY, \
    32, /* number of pixels, processed in a single block */ \
    0,  /* prefetch distance */ \
    pixman_composite_src_n_8_init, \
    pixman_composite_src_n_8_cleanup, \
    pixman_composite_src_n_8_process_pixblock_head, \
    pixman_composite_src_n_8_process_pixblock_tail, \
    pixman_composite_src_n_8_process_pixblock_tail_head, \
    0, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_n_0565_process_pixblock_head
.endm

.macro pixman_composite_src_n_0565_process_pixblock_tail
.endm

.macro pixman_composite_src_n_0565_process_pixblock_tail_head
    vst1.16 {d0, d1, d2, d3}, [DST_W, :128]!
.endm

.macro pixman_composite_src_n_0565_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d0[0]}, [DUMMY]
    vsli.u64    d0, d0, #16
    vsli.u64    d0, d0, #32
    vorr        d1, d0, d0
    vorr        q1, q0, q0
.endm

.macro pixman_composite_src_n_0565_cleanup
.endm

generate_composite_function \
    pixman_composite_src_n_0565_asm_neon, 0, 0, 16, \
    FLAG_DST_WRITEONLY, \
    16, /* number of pixels, processed in a single block */ \
    0,  /* prefetch distance */ \
    pixman_composite_src_n_0565_init, \
    pixman_composite_src_n_0565_cleanup, \
    pixman_composite_src_n_0565_process_pixblock_head, \
    pixman_composite_src_n_0565_process_pixblock_tail, \
    pixman_composite_src_n_0565_process_pixblock_tail_head, \
    0, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_n_8888_process_pixblock_head
.endm

.macro pixman_composite_src_n_8888_process_pixblock_tail
.endm

.macro pixman_composite_src_n_8888_process_pixblock_tail_head
    vst1.32 {d0, d1, d2, d3}, [DST_W, :128]!
.endm

.macro pixman_composite_src_n_8888_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d0[0]}, [DUMMY]
    vsli.u64    d0, d0, #32
    vorr        d1, d0, d0
    vorr        q1, q0, q0
.endm

.macro pixman_composite_src_n_8888_cleanup
.endm

generate_composite_function \
    pixman_composite_src_n_8888_asm_neon, 0, 0, 32, \
    FLAG_DST_WRITEONLY, \
    8, /* number of pixels, processed in a single block */ \
    0, /* prefetch distance */ \
    pixman_composite_src_n_8888_init, \
    pixman_composite_src_n_8888_cleanup, \
    pixman_composite_src_n_8888_process_pixblock_head, \
    pixman_composite_src_n_8888_process_pixblock_tail, \
    pixman_composite_src_n_8888_process_pixblock_tail_head, \
    0, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_8888_8888_process_pixblock_head
.endm

.macro pixman_composite_src_8888_8888_process_pixblock_tail
.endm

.macro pixman_composite_src_8888_8888_process_pixblock_tail_head
    vst1.32 {d0, d1, d2, d3}, [DST_W, :128]!
    fetch_src_pixblock
    cache_preload 8, 8
.endm

generate_composite_function \
    pixman_composite_src_8888_8888_asm_neon, 32, 0, 32, \
    FLAG_DST_WRITEONLY, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_8888_8888_process_pixblock_head, \
    pixman_composite_src_8888_8888_process_pixblock_tail, \
    pixman_composite_src_8888_8888_process_pixblock_tail_head, \
    0, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_x888_8888_process_pixblock_head
    vorr     q0, q0, q2
    vorr     q1, q1, q2
.endm

.macro pixman_composite_src_x888_8888_process_pixblock_tail
.endm

.macro pixman_composite_src_x888_8888_process_pixblock_tail_head
    vst1.32 {d0, d1, d2, d3}, [DST_W, :128]!
    fetch_src_pixblock
    vorr     q0, q0, q2
    vorr     q1, q1, q2
    cache_preload 8, 8
.endm

.macro pixman_composite_src_x888_8888_init
    vmov.u8  q2, #0xFF
    vshl.u32 q2, q2, #24
.endm

generate_composite_function \
    pixman_composite_src_x888_8888_asm_neon, 32, 0, 32, \
    FLAG_DST_WRITEONLY, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    pixman_composite_src_x888_8888_init, \
    default_cleanup, \
    pixman_composite_src_x888_8888_process_pixblock_head, \
    pixman_composite_src_x888_8888_process_pixblock_tail, \
    pixman_composite_src_x888_8888_process_pixblock_tail_head, \
    0, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_n_8_8888_process_pixblock_head
    /* expecting solid source in {d0, d1, d2, d3} */
    /* mask is in d24 (d25, d26, d27 are unused) */

    /* in */
    vmull.u8    q8, d24, d0
    vmull.u8    q9, d24, d1
    vmull.u8    q10, d24, d2
    vmull.u8    q11, d24, d3
    vrsra.u16   q8, q8, #8
    vrsra.u16   q9, q9, #8
    vrsra.u16   q10, q10, #8
    vrsra.u16   q11, q11, #8
.endm

.macro pixman_composite_src_n_8_8888_process_pixblock_tail
    vrshrn.u16  d28, q8, #8
    vrshrn.u16  d29, q9, #8
    vrshrn.u16  d30, q10, #8
    vrshrn.u16  d31, q11, #8
.endm

.macro pixman_composite_src_n_8_8888_process_pixblock_tail_head
    fetch_mask_pixblock
                                    PF add PF_X, PF_X, #8
        vrshrn.u16  d28, q8, #8
                                    PF tst PF_CTL, #0x0F
        vrshrn.u16  d29, q9, #8
                                    PF addne PF_X, PF_X, #8
        vrshrn.u16  d30, q10, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vrshrn.u16  d31, q11, #8
                                    PF cmp PF_X, ORIG_W
    vmull.u8    q8, d24, d0
                                    PF pld, [PF_MASK, PF_X, lsl #mask_bpp_shift]
    vmull.u8    q9, d24, d1
                                    PF subge PF_X, PF_X, ORIG_W
    vmull.u8    q10, d24, d2
                                    PF subges PF_CTL, PF_CTL, #0x10
    vmull.u8    q11, d24, d3
                                    PF ldrgeb DUMMY, [PF_MASK, MASK_STRIDE, lsl #mask_bpp_shift]!
        vst4.8      {d28, d29, d30, d31}, [DST_W, :128]!
    vrsra.u16   q8, q8, #8
    vrsra.u16   q9, q9, #8
    vrsra.u16   q10, q10, #8
    vrsra.u16   q11, q11, #8
.endm

.macro pixman_composite_src_n_8_8888_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d3[0]}, [DUMMY]
    vdup.8      d0, d3[0]
    vdup.8      d1, d3[1]
    vdup.8      d2, d3[2]
    vdup.8      d3, d3[3]
.endm

.macro pixman_composite_src_n_8_8888_cleanup
.endm

generate_composite_function \
    pixman_composite_src_n_8_8888_asm_neon, 0, 8, 32, \
    FLAG_DST_WRITEONLY | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_src_n_8_8888_init, \
    pixman_composite_src_n_8_8888_cleanup, \
    pixman_composite_src_n_8_8888_process_pixblock_head, \
    pixman_composite_src_n_8_8888_process_pixblock_tail, \
    pixman_composite_src_n_8_8888_process_pixblock_tail_head, \

/******************************************************************************/

.macro pixman_composite_src_n_8_8_process_pixblock_head
    vmull.u8    q0, d24, d16
    vmull.u8    q1, d25, d16
    vmull.u8    q2, d26, d16
    vmull.u8    q3, d27, d16
    vrsra.u16   q0, q0,  #8
    vrsra.u16   q1, q1,  #8
    vrsra.u16   q2, q2,  #8
    vrsra.u16   q3, q3,  #8
.endm

.macro pixman_composite_src_n_8_8_process_pixblock_tail
    vrshrn.u16  d28, q0, #8
    vrshrn.u16  d29, q1, #8
    vrshrn.u16  d30, q2, #8
    vrshrn.u16  d31, q3, #8
.endm

.macro pixman_composite_src_n_8_8_process_pixblock_tail_head
    fetch_mask_pixblock
                                    PF add PF_X, PF_X, #8
        vrshrn.u16  d28, q0, #8
                                    PF tst PF_CTL, #0x0F
        vrshrn.u16  d29, q1, #8
                                    PF addne PF_X, PF_X, #8
        vrshrn.u16  d30, q2, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vrshrn.u16  d31, q3, #8
                                    PF cmp PF_X, ORIG_W
    vmull.u8    q0,  d24, d16
                                    PF pld, [PF_MASK, PF_X, lsl #mask_bpp_shift]
    vmull.u8    q1,  d25, d16
                                    PF subge PF_X, PF_X, ORIG_W
    vmull.u8    q2,  d26, d16
                                    PF subges PF_CTL, PF_CTL, #0x10
    vmull.u8    q3,  d27, d16
                                    PF ldrgeb DUMMY, [PF_MASK, MASK_STRIDE, lsl #mask_bpp_shift]!
        vst1.8      {d28, d29, d30, d31}, [DST_W, :128]!
    vrsra.u16   q0, q0,  #8
    vrsra.u16   q1, q1,  #8
    vrsra.u16   q2, q2,  #8
    vrsra.u16   q3, q3,  #8
.endm

.macro pixman_composite_src_n_8_8_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d16[0]}, [DUMMY]
    vdup.8      d16, d16[3]
.endm

.macro pixman_composite_src_n_8_8_cleanup
.endm

generate_composite_function \
    pixman_composite_src_n_8_8_asm_neon, 0, 8, 8, \
    FLAG_DST_WRITEONLY, \
    32, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_src_n_8_8_init, \
    pixman_composite_src_n_8_8_cleanup, \
    pixman_composite_src_n_8_8_process_pixblock_head, \
    pixman_composite_src_n_8_8_process_pixblock_tail, \
    pixman_composite_src_n_8_8_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_over_n_8_8888_process_pixblock_head
    /* expecting deinterleaved source data in {d8, d9, d10, d11} */
    /* d8 - blue, d9 - green, d10 - red, d11 - alpha */
    /* and destination data in {d4, d5, d6, d7} */
    /* mask is in d24 (d25, d26, d27 are unused) */

    /* in */
    vmull.u8    q6, d24, d8
    vmull.u8    q7, d24, d9
    vmull.u8    q8, d24, d10
    vmull.u8    q9, d24, d11
    vrshr.u16   q10, q6, #8
    vrshr.u16   q11, q7, #8
    vrshr.u16   q12, q8, #8
    vrshr.u16   q13, q9, #8
    vraddhn.u16 d0, q6, q10
    vraddhn.u16 d1, q7, q11
    vraddhn.u16 d2, q8, q12
    vraddhn.u16 d3, q9, q13
    vmvn.8      d25, d3  /* get inverted alpha */
    /* source:      d0 - blue, d1 - green, d2 - red, d3 - alpha */
    /* destination: d4 - blue, d5 - green, d6 - red, d7 - alpha */
    /* now do alpha blending */
    vmull.u8    q8, d25, d4
    vmull.u8    q9, d25, d5
    vmull.u8    q10, d25, d6
    vmull.u8    q11, d25, d7
.endm

.macro pixman_composite_over_n_8_8888_process_pixblock_tail
    vrshr.u16   q14, q8, #8
    vrshr.u16   q15, q9, #8
    vrshr.u16   q6, q10, #8
    vrshr.u16   q7, q11, #8
    vraddhn.u16 d28, q14, q8
    vraddhn.u16 d29, q15, q9
    vraddhn.u16 d30, q6, q10
    vraddhn.u16 d31, q7, q11
    vqadd.u8    q14, q0, q14
    vqadd.u8    q15, q1, q15
.endm

.macro pixman_composite_over_n_8_8888_process_pixblock_tail_head
        vrshr.u16   q14, q8, #8
    vld4.8      {d4, d5, d6, d7}, [DST_R, :128]!
        vrshr.u16   q15, q9, #8
    fetch_mask_pixblock
        vrshr.u16   q6, q10, #8
                                    PF add PF_X, PF_X, #8
        vrshr.u16   q7, q11, #8
                                    PF tst PF_CTL, #0x0F
        vraddhn.u16 d28, q14, q8
                                    PF addne PF_X, PF_X, #8
        vraddhn.u16 d29, q15, q9
                                    PF subne PF_CTL, PF_CTL, #1
        vraddhn.u16 d30, q6, q10
                                    PF cmp PF_X, ORIG_W
        vraddhn.u16 d31, q7, q11
                                    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
    vmull.u8    q6, d24, d8
                                    PF pld, [PF_MASK, PF_X, lsl #mask_bpp_shift]
    vmull.u8    q7, d24, d9
                                    PF subge PF_X, PF_X, ORIG_W
    vmull.u8    q8, d24, d10
                                    PF subges PF_CTL, PF_CTL, #0x10
    vmull.u8    q9, d24, d11
                                    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
        vqadd.u8    q14, q0, q14
                                    PF ldrgeb DUMMY, [PF_MASK, MASK_STRIDE, lsl #mask_bpp_shift]!
        vqadd.u8    q15, q1, q15
    vrshr.u16   q10, q6, #8
    vrshr.u16   q11, q7, #8
    vrshr.u16   q12, q8, #8
    vrshr.u16   q13, q9, #8
    vraddhn.u16 d0, q6, q10
    vraddhn.u16 d1, q7, q11
    vraddhn.u16 d2, q8, q12
    vraddhn.u16 d3, q9, q13
        vst4.8      {d28, d29, d30, d31}, [DST_W, :128]!
    vmvn.8      d25, d3
    vmull.u8    q8, d25, d4
    vmull.u8    q9, d25, d5
    vmull.u8    q10, d25, d6
    vmull.u8    q11, d25, d7
.endm

.macro pixman_composite_over_n_8_8888_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vpush       {d8-d15}
    vld1.32     {d11[0]}, [DUMMY]
    vdup.8      d8, d11[0]
    vdup.8      d9, d11[1]
    vdup.8      d10, d11[2]
    vdup.8      d11, d11[3]
.endm

.macro pixman_composite_over_n_8_8888_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_over_n_8_8888_asm_neon, 0, 8, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_n_8_8888_init, \
    pixman_composite_over_n_8_8888_cleanup, \
    pixman_composite_over_n_8_8888_process_pixblock_head, \
    pixman_composite_over_n_8_8888_process_pixblock_tail, \
    pixman_composite_over_n_8_8888_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_over_n_8_8_process_pixblock_head
    vmull.u8    q0,  d24, d8
    vmull.u8    q1,  d25, d8
    vmull.u8    q6,  d26, d8
    vmull.u8    q7,  d27, d8
    vrshr.u16   q10, q0,  #8
    vrshr.u16   q11, q1,  #8
    vrshr.u16   q12, q6,  #8
    vrshr.u16   q13, q7,  #8
    vraddhn.u16 d0,  q0,  q10
    vraddhn.u16 d1,  q1,  q11
    vraddhn.u16 d2,  q6,  q12
    vraddhn.u16 d3,  q7,  q13
    vmvn.8      q12, q0
    vmvn.8      q13, q1
    vmull.u8    q8,  d24, d4
    vmull.u8    q9,  d25, d5
    vmull.u8    q10, d26, d6
    vmull.u8    q11, d27, d7
.endm

.macro pixman_composite_over_n_8_8_process_pixblock_tail
    vrshr.u16   q14, q8,  #8
    vrshr.u16   q15, q9,  #8
    vrshr.u16   q12, q10, #8
    vrshr.u16   q13, q11, #8
    vraddhn.u16 d28, q14, q8
    vraddhn.u16 d29, q15, q9
    vraddhn.u16 d30, q12, q10
    vraddhn.u16 d31, q13, q11
    vqadd.u8    q14, q0,  q14
    vqadd.u8    q15, q1,  q15
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_n_8_8_process_pixblock_tail_head
    vld1.8      {d4, d5, d6, d7}, [DST_R, :128]!
    pixman_composite_over_n_8_8_process_pixblock_tail
    fetch_mask_pixblock
    cache_preload 32, 32
    vst1.8      {d28, d29, d30, d31}, [DST_W, :128]!
    pixman_composite_over_n_8_8_process_pixblock_head
.endm

.macro pixman_composite_over_n_8_8_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vpush       {d8-d15}
    vld1.32     {d8[0]}, [DUMMY]
    vdup.8      d8, d8[3]
.endm

.macro pixman_composite_over_n_8_8_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_over_n_8_8_asm_neon, 0, 8, 8, \
    FLAG_DST_READWRITE, \
    32, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_n_8_8_init, \
    pixman_composite_over_n_8_8_cleanup, \
    pixman_composite_over_n_8_8_process_pixblock_head, \
    pixman_composite_over_n_8_8_process_pixblock_tail, \
    pixman_composite_over_n_8_8_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_over_n_8888_8888_ca_process_pixblock_head
    /*
     * 'combine_mask_ca' replacement
     *
     * input:  solid src (n) in {d8,  d9,  d10, d11}
     *         dest in          {d4,  d5,  d6,  d7 }
     *         mask in          {d24, d25, d26, d27}
     * output: updated src in   {d0,  d1,  d2,  d3 }
     *         updated mask in  {d24, d25, d26, d3 }
     */
    vmull.u8    q0,  d24, d8
    vmull.u8    q1,  d25, d9
    vmull.u8    q6,  d26, d10
    vmull.u8    q7,  d27, d11
    vmull.u8    q9,  d11, d25
    vmull.u8    q12, d11, d24
    vmull.u8    q13, d11, d26
    vrshr.u16   q8,  q0,  #8
    vrshr.u16   q10, q1,  #8
    vrshr.u16   q11, q6,  #8
    vraddhn.u16 d0,  q0,  q8
    vraddhn.u16 d1,  q1,  q10
    vraddhn.u16 d2,  q6,  q11
    vrshr.u16   q11, q12, #8
    vrshr.u16   q8,  q9,  #8
    vrshr.u16   q6,  q13, #8
    vrshr.u16   q10, q7,  #8
    vraddhn.u16 d24, q12, q11
    vraddhn.u16 d25, q9,  q8
    vraddhn.u16 d26, q13, q6
    vraddhn.u16 d3,  q7,  q10
    /*
     * 'combine_over_ca' replacement
     *
     * output: updated dest in {d28, d29, d30, d31}
     */
    vmvn.8      q12, q12
    vmvn.8      d26, d26
    vmull.u8    q8,  d24, d4
    vmull.u8    q9,  d25, d5
    vmvn.8      d27, d3
    vmull.u8    q10, d26, d6
    vmull.u8    q11, d27, d7
.endm

.macro pixman_composite_over_n_8888_8888_ca_process_pixblock_tail
    /* ... continue 'combine_over_ca' replacement */
    vrshr.u16   q14, q8,  #8
    vrshr.u16   q15, q9,  #8
    vrshr.u16   q6,  q10, #8
    vrshr.u16   q7,  q11, #8
    vraddhn.u16 d28, q14, q8
    vraddhn.u16 d29, q15, q9
    vraddhn.u16 d30, q6,  q10
    vraddhn.u16 d31, q7,  q11
    vqadd.u8    q14, q0,  q14
    vqadd.u8    q15, q1,  q15
.endm

.macro pixman_composite_over_n_8888_8888_ca_process_pixblock_tail_head
        vrshr.u16   q14, q8, #8
        vrshr.u16   q15, q9, #8
    vld4.8      {d4, d5, d6, d7}, [DST_R, :128]!
        vrshr.u16   q6, q10, #8
        vrshr.u16   q7, q11, #8
        vraddhn.u16 d28, q14, q8
        vraddhn.u16 d29, q15, q9
        vraddhn.u16 d30, q6, q10
        vraddhn.u16 d31, q7, q11
    fetch_mask_pixblock
        vqadd.u8    q14, q0, q14
        vqadd.u8    q15, q1, q15
    cache_preload 8, 8
    pixman_composite_over_n_8888_8888_ca_process_pixblock_head
    vst4.8      {d28, d29, d30, d31}, [DST_W, :128]!
.endm

.macro pixman_composite_over_n_8888_8888_ca_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vpush       {d8-d15}
    vld1.32     {d11[0]}, [DUMMY]
    vdup.8      d8, d11[0]
    vdup.8      d9, d11[1]
    vdup.8      d10, d11[2]
    vdup.8      d11, d11[3]
.endm

.macro pixman_composite_over_n_8888_8888_ca_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_over_n_8888_8888_ca_asm_neon, 0, 32, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_n_8888_8888_ca_init, \
    pixman_composite_over_n_8888_8888_ca_cleanup, \
    pixman_composite_over_n_8888_8888_ca_process_pixblock_head, \
    pixman_composite_over_n_8888_8888_ca_process_pixblock_tail, \
    pixman_composite_over_n_8888_8888_ca_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_over_n_8888_0565_ca_process_pixblock_head
    /*
     * 'combine_mask_ca' replacement
     *
     * input:  solid src (n) in {d8,  d9,  d10, d11}  [B, G, R, A]
     *         mask in          {d24, d25, d26}       [B, G, R]
     * output: updated src in   {d0,  d1,  d2 }       [B, G, R]
     *         updated mask in  {d24, d25, d26}       [B, G, R]
     */
    vmull.u8    q0,  d24, d8
    vmull.u8    q1,  d25, d9
    vmull.u8    q6,  d26, d10
    vmull.u8    q9,  d11, d25
    vmull.u8    q12, d11, d24
    vmull.u8    q13, d11, d26
    vrshr.u16   q8,  q0,  #8
    vrshr.u16   q10, q1,  #8
    vrshr.u16   q11, q6,  #8
    vraddhn.u16 d0,  q0,  q8
    vraddhn.u16 d1,  q1,  q10
    vraddhn.u16 d2,  q6,  q11
    vrshr.u16   q11, q12, #8
    vrshr.u16   q8,  q9,  #8
    vrshr.u16   q6,  q13, #8
    vraddhn.u16 d24, q12, q11
    vraddhn.u16 d25, q9,  q8
    /*
     * convert 8 r5g6b5 pixel data from {d4, d5} to planar 8-bit format
     * and put data into d16 - blue, d17 - green, d18 - red
     */
       vshrn.u16   d17, q2,  #3
       vshrn.u16   d18, q2,  #8
    vraddhn.u16 d26, q13, q6
       vsli.u16    q2,  q2,  #5
       vsri.u8     d18, d18, #5
       vsri.u8     d17, d17, #6
    /*
     * 'combine_over_ca' replacement
     *
     * output: updated dest in d16 - blue, d17 - green, d18 - red
     */
    vmvn.8      q12, q12
       vshrn.u16   d16, q2,  #2
    vmvn.8      d26, d26
    vmull.u8    q6,  d16, d24
    vmull.u8    q7,  d17, d25
    vmull.u8    q11, d18, d26
.endm

.macro pixman_composite_over_n_8888_0565_ca_process_pixblock_tail
    /* ... continue 'combine_over_ca' replacement */
    vrshr.u16   q10, q6,  #8
    vrshr.u16   q14, q7,  #8
    vrshr.u16   q15, q11, #8
    vraddhn.u16 d16, q10, q6
    vraddhn.u16 d17, q14, q7
    vraddhn.u16 d18, q15, q11
    vqadd.u8    q8,  q0,  q8
    vqadd.u8    d18, d2,  d18
    /*
     * convert the results in d16, d17, d18 to r5g6b5 and store
     * them into {d28, d29}
     */
    vshll.u8    q14, d18, #8
    vshll.u8    q10, d17, #8
    vshll.u8    q15, d16, #8
    vsri.u16    q14, q10, #5
    vsri.u16    q14, q15, #11
.endm

.macro pixman_composite_over_n_8888_0565_ca_process_pixblock_tail_head
    fetch_mask_pixblock
        vrshr.u16   q10, q6, #8
        vrshr.u16   q14, q7, #8
    vld1.16     {d4, d5}, [DST_R, :128]!
        vrshr.u16   q15, q11, #8
        vraddhn.u16 d16, q10, q6
        vraddhn.u16 d17, q14, q7
        vraddhn.u16 d22, q15, q11
            /* process_pixblock_head */
            /*
             * 'combine_mask_ca' replacement
             *
             * input:  solid src (n) in {d8,  d9,  d10, d11}  [B, G, R, A]
             *         mask in          {d24, d25, d26}       [B, G, R]
             * output: updated src in   {d0,  d1,  d2 }       [B, G, R]
             *         updated mask in  {d24, d25, d26}       [B, G, R]
             */
            vmull.u8    q6,  d26, d10
        vqadd.u8    q8,  q0, q8
            vmull.u8    q0,  d24, d8
        vqadd.u8    d22, d2, d22
            vmull.u8    q1,  d25, d9
        /*
         * convert the result in d16, d17, d22 to r5g6b5 and store
         * it into {d28, d29}
         */
        vshll.u8    q14, d22, #8
        vshll.u8    q10, d17, #8
        vshll.u8    q15, d16, #8
            vmull.u8    q9,  d11, d25
        vsri.u16    q14, q10, #5
            vmull.u8    q12, d11, d24
            vmull.u8    q13, d11, d26
        vsri.u16    q14, q15, #11
    cache_preload 8, 8
            vrshr.u16   q8,  q0,  #8
            vrshr.u16   q10, q1,  #8
            vrshr.u16   q11, q6,  #8
            vraddhn.u16 d0,  q0,  q8
            vraddhn.u16 d1,  q1,  q10
            vraddhn.u16 d2,  q6,  q11
            vrshr.u16   q11, q12, #8
            vrshr.u16   q8,  q9,  #8
            vrshr.u16   q6,  q13, #8
            vraddhn.u16 d24, q12, q11
            vraddhn.u16 d25, q9,  q8
                /*
                 * convert 8 r5g6b5 pixel data from {d4, d5} to planar
	         * 8-bit format and put data into d16 - blue, d17 - green,
	         * d18 - red
                 */
                vshrn.u16   d17, q2,  #3
                vshrn.u16   d18, q2,  #8
            vraddhn.u16 d26, q13, q6
                vsli.u16    q2,  q2,  #5
                vsri.u8     d17, d17, #6
                vsri.u8     d18, d18, #5
            /*
             * 'combine_over_ca' replacement
             *
             * output: updated dest in d16 - blue, d17 - green, d18 - red
             */
            vmvn.8      q12, q12
                vshrn.u16   d16, q2,  #2
            vmvn.8      d26, d26
            vmull.u8    q7,  d17, d25
            vmull.u8    q6,  d16, d24
            vmull.u8    q11, d18, d26
    vst1.16     {d28, d29}, [DST_W, :128]!
.endm

.macro pixman_composite_over_n_8888_0565_ca_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vpush       {d8-d15}
    vld1.32     {d11[0]}, [DUMMY]
    vdup.8      d8, d11[0]
    vdup.8      d9, d11[1]
    vdup.8      d10, d11[2]
    vdup.8      d11, d11[3]
.endm

.macro pixman_composite_over_n_8888_0565_ca_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_over_n_8888_0565_ca_asm_neon, 0, 32, 16, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_n_8888_0565_ca_init, \
    pixman_composite_over_n_8888_0565_ca_cleanup, \
    pixman_composite_over_n_8888_0565_ca_process_pixblock_head, \
    pixman_composite_over_n_8888_0565_ca_process_pixblock_tail, \
    pixman_composite_over_n_8888_0565_ca_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_in_n_8_process_pixblock_head
    /* expecting source data in {d0, d1, d2, d3} */
    /* and destination data in {d4, d5, d6, d7} */
    vmull.u8    q8,  d4,  d3
    vmull.u8    q9,  d5,  d3
    vmull.u8    q10, d6,  d3
    vmull.u8    q11, d7,  d3
.endm

.macro pixman_composite_in_n_8_process_pixblock_tail
    vrshr.u16   q14, q8,  #8
    vrshr.u16   q15, q9,  #8
    vrshr.u16   q12, q10, #8
    vrshr.u16   q13, q11, #8
    vraddhn.u16 d28, q8,  q14
    vraddhn.u16 d29, q9,  q15
    vraddhn.u16 d30, q10, q12
    vraddhn.u16 d31, q11, q13
.endm

.macro pixman_composite_in_n_8_process_pixblock_tail_head
    pixman_composite_in_n_8_process_pixblock_tail
    vld1.8      {d4, d5, d6, d7}, [DST_R, :128]!
    cache_preload 32, 32
    pixman_composite_in_n_8_process_pixblock_head
    vst1.8      {d28, d29, d30, d31}, [DST_W, :128]!
.endm

.macro pixman_composite_in_n_8_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d3[0]}, [DUMMY]
    vdup.8      d3, d3[3]
.endm

.macro pixman_composite_in_n_8_cleanup
.endm

generate_composite_function \
    pixman_composite_in_n_8_asm_neon, 0, 0, 8, \
    FLAG_DST_READWRITE, \
    32, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_in_n_8_init, \
    pixman_composite_in_n_8_cleanup, \
    pixman_composite_in_n_8_process_pixblock_head, \
    pixman_composite_in_n_8_process_pixblock_tail, \
    pixman_composite_in_n_8_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    24  /* mask_basereg  */

.macro pixman_composite_add_n_8_8_process_pixblock_head
    /* expecting source data in {d8, d9, d10, d11} */
    /* d8 - blue, d9 - green, d10 - red, d11 - alpha */
    /* and destination data in {d4, d5, d6, d7} */
    /* mask is in d24, d25, d26, d27 */
    vmull.u8    q0, d24, d11
    vmull.u8    q1, d25, d11
    vmull.u8    q6, d26, d11
    vmull.u8    q7, d27, d11
    vrshr.u16   q10, q0, #8
    vrshr.u16   q11, q1, #8
    vrshr.u16   q12, q6, #8
    vrshr.u16   q13, q7, #8
    vraddhn.u16 d0, q0, q10
    vraddhn.u16 d1, q1, q11
    vraddhn.u16 d2, q6, q12
    vraddhn.u16 d3, q7, q13
    vqadd.u8    q14, q0, q2
    vqadd.u8    q15, q1, q3
.endm

.macro pixman_composite_add_n_8_8_process_pixblock_tail
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_add_n_8_8_process_pixblock_tail_head
    pixman_composite_add_n_8_8_process_pixblock_tail
    vst1.8      {d28, d29, d30, d31}, [DST_W, :128]!
    vld1.8      {d4, d5, d6, d7}, [DST_R, :128]!
    fetch_mask_pixblock
    cache_preload 32, 32
    pixman_composite_add_n_8_8_process_pixblock_head
.endm

.macro pixman_composite_add_n_8_8_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vpush       {d8-d15}
    vld1.32     {d11[0]}, [DUMMY]
    vdup.8      d11, d11[3]
.endm

.macro pixman_composite_add_n_8_8_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_add_n_8_8_asm_neon, 0, 8, 8, \
    FLAG_DST_READWRITE, \
    32, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_add_n_8_8_init, \
    pixman_composite_add_n_8_8_cleanup, \
    pixman_composite_add_n_8_8_process_pixblock_head, \
    pixman_composite_add_n_8_8_process_pixblock_tail, \
    pixman_composite_add_n_8_8_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_add_8_8_8_process_pixblock_head
    /* expecting source data in {d0, d1, d2, d3} */
    /* destination data in {d4, d5, d6, d7} */
    /* mask in {d24, d25, d26, d27} */
    vmull.u8    q8, d24, d0
    vmull.u8    q9, d25, d1
    vmull.u8    q10, d26, d2
    vmull.u8    q11, d27, d3
    vrshr.u16   q0, q8, #8
    vrshr.u16   q1, q9, #8
    vrshr.u16   q12, q10, #8
    vrshr.u16   q13, q11, #8
    vraddhn.u16 d0, q0, q8
    vraddhn.u16 d1, q1, q9
    vraddhn.u16 d2, q12, q10
    vraddhn.u16 d3, q13, q11
    vqadd.u8    q14, q0, q2
    vqadd.u8    q15, q1, q3
.endm

.macro pixman_composite_add_8_8_8_process_pixblock_tail
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_add_8_8_8_process_pixblock_tail_head
    pixman_composite_add_8_8_8_process_pixblock_tail
    vst1.8      {d28, d29, d30, d31}, [DST_W, :128]!
    vld1.8      {d4, d5, d6, d7}, [DST_R, :128]!
    fetch_mask_pixblock
    fetch_src_pixblock
    cache_preload 32, 32
    pixman_composite_add_8_8_8_process_pixblock_head
.endm

.macro pixman_composite_add_8_8_8_init
.endm

.macro pixman_composite_add_8_8_8_cleanup
.endm

generate_composite_function \
    pixman_composite_add_8_8_8_asm_neon, 8, 8, 8, \
    FLAG_DST_READWRITE, \
    32, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_add_8_8_8_init, \
    pixman_composite_add_8_8_8_cleanup, \
    pixman_composite_add_8_8_8_process_pixblock_head, \
    pixman_composite_add_8_8_8_process_pixblock_tail, \
    pixman_composite_add_8_8_8_process_pixblock_tail_head

/******************************************************************************/

.macro pixman_composite_add_8888_8888_8888_process_pixblock_head
    /* expecting source data in {d0, d1, d2, d3} */
    /* destination data in {d4, d5, d6, d7} */
    /* mask in {d24, d25, d26, d27} */
    vmull.u8    q8,  d27, d0
    vmull.u8    q9,  d27, d1
    vmull.u8    q10, d27, d2
    vmull.u8    q11, d27, d3
    /* 1 cycle bubble */
    vrsra.u16   q8,  q8,  #8
    vrsra.u16   q9,  q9,  #8
    vrsra.u16   q10, q10, #8
    vrsra.u16   q11, q11, #8
.endm

.macro pixman_composite_add_8888_8888_8888_process_pixblock_tail
    /* 2 cycle bubble */
    vrshrn.u16  d28, q8,  #8
    vrshrn.u16  d29, q9,  #8
    vrshrn.u16  d30, q10, #8
    vrshrn.u16  d31, q11, #8
    vqadd.u8    q14, q2,  q14
    /* 1 cycle bubble */
    vqadd.u8    q15, q3,  q15
.endm

.macro pixman_composite_add_8888_8888_8888_process_pixblock_tail_head
    fetch_src_pixblock
        vrshrn.u16  d28, q8,  #8
    fetch_mask_pixblock
        vrshrn.u16  d29, q9,  #8
    vmull.u8    q8,  d27, d0
        vrshrn.u16  d30, q10, #8
    vmull.u8    q9,  d27, d1
        vrshrn.u16  d31, q11, #8
    vmull.u8    q10, d27, d2
        vqadd.u8    q14, q2,  q14
    vmull.u8    q11, d27, d3
        vqadd.u8    q15, q3,  q15
    vrsra.u16   q8,  q8,  #8
    vld4.8      {d4, d5, d6, d7}, [DST_R, :128]!
    vrsra.u16   q9,  q9,  #8
        vst4.8      {d28, d29, d30, d31}, [DST_W, :128]!
    vrsra.u16   q10, q10, #8

    cache_preload 8, 8

    vrsra.u16   q11, q11, #8
.endm

generate_composite_function \
    pixman_composite_add_8888_8888_8888_asm_neon, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_add_8888_8888_8888_process_pixblock_head, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail_head

generate_composite_function_single_scanline \
    pixman_composite_scanline_add_mask_asm_neon, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_add_8888_8888_8888_process_pixblock_head, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail_head

/******************************************************************************/

generate_composite_function \
    pixman_composite_add_8888_8_8888_asm_neon, 32, 8, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_add_8888_8888_8888_process_pixblock_head, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    27  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_add_n_8_8888_init
    add         DUMMY, sp, #ARGS_STACK_OFFSET
    vld1.32     {d3[0]}, [DUMMY]
    vdup.8      d0, d3[0]
    vdup.8      d1, d3[1]
    vdup.8      d2, d3[2]
    vdup.8      d3, d3[3]
.endm

.macro pixman_composite_add_n_8_8888_cleanup
.endm

generate_composite_function \
    pixman_composite_add_n_8_8888_asm_neon, 0, 8, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_add_n_8_8888_init, \
    pixman_composite_add_n_8_8888_cleanup, \
    pixman_composite_add_8888_8888_8888_process_pixblock_head, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    27  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_add_8888_n_8888_init
    add         DUMMY, sp, #(ARGS_STACK_OFFSET + 8)
    vld1.32     {d27[0]}, [DUMMY]
    vdup.8      d27, d27[3]
.endm

.macro pixman_composite_add_8888_n_8888_cleanup
.endm

generate_composite_function \
    pixman_composite_add_8888_n_8888_asm_neon, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_add_8888_n_8888_init, \
    pixman_composite_add_8888_n_8888_cleanup, \
    pixman_composite_add_8888_8888_8888_process_pixblock_head, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    27  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_out_reverse_8888_n_8888_process_pixblock_head
    /* expecting source data in {d0, d1, d2, d3} */
    /* destination data in {d4, d5, d6, d7} */
    /* solid mask is in d15 */

    /* 'in' */
    vmull.u8    q8, d15, d3
    vmull.u8    q6, d15, d2
    vmull.u8    q5, d15, d1
    vmull.u8    q4, d15, d0
    vrshr.u16   q13, q8, #8
    vrshr.u16   q12, q6, #8
    vrshr.u16   q11, q5, #8
    vrshr.u16   q10, q4, #8
    vraddhn.u16 d3, q8, q13
    vraddhn.u16 d2, q6, q12
    vraddhn.u16 d1, q5, q11
    vraddhn.u16 d0, q4, q10
    vmvn.8      d24, d3  /* get inverted alpha */
    /* now do alpha blending */
    vmull.u8    q8, d24, d4
    vmull.u8    q9, d24, d5
    vmull.u8    q10, d24, d6
    vmull.u8    q11, d24, d7
.endm

.macro pixman_composite_out_reverse_8888_n_8888_process_pixblock_tail
    vrshr.u16   q14, q8, #8
    vrshr.u16   q15, q9, #8
    vrshr.u16   q12, q10, #8
    vrshr.u16   q13, q11, #8
    vraddhn.u16 d28, q14, q8
    vraddhn.u16 d29, q15, q9
    vraddhn.u16 d30, q12, q10
    vraddhn.u16 d31, q13, q11
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_out_reverse_8888_8888_8888_process_pixblock_tail_head
    vld4.8     {d4, d5, d6, d7}, [DST_R, :128]!
    pixman_composite_out_reverse_8888_n_8888_process_pixblock_tail
    fetch_src_pixblock
    cache_preload 8, 8
    fetch_mask_pixblock
    pixman_composite_out_reverse_8888_n_8888_process_pixblock_head
    vst4.8     {d28, d29, d30, d31}, [DST_W, :128]!
.endm

generate_composite_function_single_scanline \
    pixman_composite_scanline_out_reverse_mask_asm_neon, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_out_reverse_8888_n_8888_process_pixblock_head, \
    pixman_composite_out_reverse_8888_n_8888_process_pixblock_tail, \
    pixman_composite_out_reverse_8888_8888_8888_process_pixblock_tail_head \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    12  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_over_8888_n_8888_process_pixblock_head
    pixman_composite_out_reverse_8888_n_8888_process_pixblock_head
.endm

.macro pixman_composite_over_8888_n_8888_process_pixblock_tail
    pixman_composite_out_reverse_8888_n_8888_process_pixblock_tail
    vqadd.u8    q14, q0, q14
    vqadd.u8    q15, q1, q15
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_8888_n_8888_process_pixblock_tail_head
    vld4.8     {d4, d5, d6, d7}, [DST_R, :128]!
    pixman_composite_over_8888_n_8888_process_pixblock_tail
    fetch_src_pixblock
    cache_preload 8, 8
    pixman_composite_over_8888_n_8888_process_pixblock_head
    vst4.8     {d28, d29, d30, d31}, [DST_W, :128]!
.endm

.macro pixman_composite_over_8888_n_8888_init
    add         DUMMY, sp, #48
    vpush       {d8-d15}
    vld1.32     {d15[0]}, [DUMMY]
    vdup.8      d15, d15[3]
.endm

.macro pixman_composite_over_8888_n_8888_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_over_8888_n_8888_asm_neon, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_8888_n_8888_init, \
    pixman_composite_over_8888_n_8888_cleanup, \
    pixman_composite_over_8888_n_8888_process_pixblock_head, \
    pixman_composite_over_8888_n_8888_process_pixblock_tail, \
    pixman_composite_over_8888_n_8888_process_pixblock_tail_head

/******************************************************************************/

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_8888_8888_8888_process_pixblock_tail_head
    vld4.8     {d4, d5, d6, d7}, [DST_R, :128]!
    pixman_composite_over_8888_n_8888_process_pixblock_tail
    fetch_src_pixblock
    cache_preload 8, 8
    fetch_mask_pixblock
    pixman_composite_over_8888_n_8888_process_pixblock_head
    vst4.8     {d28, d29, d30, d31}, [DST_W, :128]!
.endm

generate_composite_function \
    pixman_composite_over_8888_8888_8888_asm_neon, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_over_8888_n_8888_process_pixblock_head, \
    pixman_composite_over_8888_n_8888_process_pixblock_tail, \
    pixman_composite_over_8888_8888_8888_process_pixblock_tail_head \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    12  /* mask_basereg  */

generate_composite_function_single_scanline \
    pixman_composite_scanline_over_mask_asm_neon, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_over_8888_n_8888_process_pixblock_head, \
    pixman_composite_over_8888_n_8888_process_pixblock_tail, \
    pixman_composite_over_8888_8888_8888_process_pixblock_tail_head \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    12  /* mask_basereg  */

/******************************************************************************/

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_8888_8_8888_process_pixblock_tail_head
    vld4.8     {d4, d5, d6, d7}, [DST_R, :128]!
    pixman_composite_over_8888_n_8888_process_pixblock_tail
    fetch_src_pixblock
    cache_preload 8, 8
    fetch_mask_pixblock
    pixman_composite_over_8888_n_8888_process_pixblock_head
    vst4.8     {d28, d29, d30, d31}, [DST_W, :128]!
.endm

generate_composite_function \
    pixman_composite_over_8888_8_8888_asm_neon, 32, 8, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_over_8888_n_8888_process_pixblock_head, \
    pixman_composite_over_8888_n_8888_process_pixblock_tail, \
    pixman_composite_over_8888_8_8888_process_pixblock_tail_head \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    15  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_0888_0888_process_pixblock_head
.endm

.macro pixman_composite_src_0888_0888_process_pixblock_tail
.endm

.macro pixman_composite_src_0888_0888_process_pixblock_tail_head
    vst3.8 {d0, d1, d2}, [DST_W]!
    fetch_src_pixblock
    cache_preload 8, 8
.endm

generate_composite_function \
    pixman_composite_src_0888_0888_asm_neon, 24, 0, 24, \
    FLAG_DST_WRITEONLY, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_0888_0888_process_pixblock_head, \
    pixman_composite_src_0888_0888_process_pixblock_tail, \
    pixman_composite_src_0888_0888_process_pixblock_tail_head, \
    0, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_0888_8888_rev_process_pixblock_head
    vswp   d0, d2
.endm

.macro pixman_composite_src_0888_8888_rev_process_pixblock_tail
.endm

.macro pixman_composite_src_0888_8888_rev_process_pixblock_tail_head
    vst4.8 {d0, d1, d2, d3}, [DST_W]!
    fetch_src_pixblock
    vswp   d0, d2
    cache_preload 8, 8
.endm

.macro pixman_composite_src_0888_8888_rev_init
    veor   d3, d3, d3
.endm

generate_composite_function \
    pixman_composite_src_0888_8888_rev_asm_neon, 24, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    pixman_composite_src_0888_8888_rev_init, \
    default_cleanup, \
    pixman_composite_src_0888_8888_rev_process_pixblock_head, \
    pixman_composite_src_0888_8888_rev_process_pixblock_tail, \
    pixman_composite_src_0888_8888_rev_process_pixblock_tail_head, \
    0, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_0888_0565_rev_process_pixblock_head
    vshll.u8    q8, d1, #8
    vshll.u8    q9, d2, #8
.endm

.macro pixman_composite_src_0888_0565_rev_process_pixblock_tail
    vshll.u8    q14, d0, #8
    vsri.u16    q14, q8, #5
    vsri.u16    q14, q9, #11
.endm

.macro pixman_composite_src_0888_0565_rev_process_pixblock_tail_head
        vshll.u8    q14, d0, #8
    fetch_src_pixblock
        vsri.u16    q14, q8, #5
        vsri.u16    q14, q9, #11
    vshll.u8    q8, d1, #8
        vst1.16 {d28, d29}, [DST_W, :128]!
    vshll.u8    q9, d2, #8
.endm

generate_composite_function \
    pixman_composite_src_0888_0565_rev_asm_neon, 24, 0, 16, \
    FLAG_DST_WRITEONLY, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_0888_0565_rev_process_pixblock_head, \
    pixman_composite_src_0888_0565_rev_process_pixblock_tail, \
    pixman_composite_src_0888_0565_rev_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_pixbuf_8888_process_pixblock_head
    vmull.u8    q8, d3, d0
    vmull.u8    q9, d3, d1
    vmull.u8    q10, d3, d2
.endm

.macro pixman_composite_src_pixbuf_8888_process_pixblock_tail
    vrshr.u16   q11, q8, #8
    vswp        d3, d31
    vrshr.u16   q12, q9, #8
    vrshr.u16   q13, q10, #8
    vraddhn.u16 d30, q11, q8
    vraddhn.u16 d29, q12, q9
    vraddhn.u16 d28, q13, q10
.endm

.macro pixman_composite_src_pixbuf_8888_process_pixblock_tail_head
        vrshr.u16   q11, q8, #8
        vswp        d3, d31
        vrshr.u16   q12, q9, #8
        vrshr.u16   q13, q10, #8
    fetch_src_pixblock
        vraddhn.u16 d30, q11, q8
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
                                    PF addne PF_X, PF_X, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vraddhn.u16 d29, q12, q9
        vraddhn.u16 d28, q13, q10
    vmull.u8    q8, d3, d0
    vmull.u8    q9, d3, d1
    vmull.u8    q10, d3, d2
        vst4.8 {d28, d29, d30, d31}, [DST_W, :128]!
                                    PF cmp PF_X, ORIG_W
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
                                    PF subge PF_X, PF_X, ORIG_W
                                    PF subges PF_CTL, PF_CTL, #0x10
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
.endm

generate_composite_function \
    pixman_composite_src_pixbuf_8888_asm_neon, 32, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_pixbuf_8888_process_pixblock_head, \
    pixman_composite_src_pixbuf_8888_process_pixblock_tail, \
    pixman_composite_src_pixbuf_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_src_rpixbuf_8888_process_pixblock_head
    vmull.u8    q8, d3, d0
    vmull.u8    q9, d3, d1
    vmull.u8    q10, d3, d2
.endm

.macro pixman_composite_src_rpixbuf_8888_process_pixblock_tail
    vrshr.u16   q11, q8, #8
    vswp        d3, d31
    vrshr.u16   q12, q9, #8
    vrshr.u16   q13, q10, #8
    vraddhn.u16 d28, q11, q8
    vraddhn.u16 d29, q12, q9
    vraddhn.u16 d30, q13, q10
.endm

.macro pixman_composite_src_rpixbuf_8888_process_pixblock_tail_head
        vrshr.u16   q11, q8, #8
        vswp        d3, d31
        vrshr.u16   q12, q9, #8
        vrshr.u16   q13, q10, #8
    fetch_src_pixblock
        vraddhn.u16 d28, q11, q8
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
                                    PF addne PF_X, PF_X, #8
                                    PF subne PF_CTL, PF_CTL, #1
        vraddhn.u16 d29, q12, q9
        vraddhn.u16 d30, q13, q10
    vmull.u8    q8, d3, d0
    vmull.u8    q9, d3, d1
    vmull.u8    q10, d3, d2
        vst4.8 {d28, d29, d30, d31}, [DST_W, :128]!
                                    PF cmp PF_X, ORIG_W
                                    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
                                    PF subge PF_X, PF_X, ORIG_W
                                    PF subges PF_CTL, PF_CTL, #0x10
                                    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
.endm

generate_composite_function \
    pixman_composite_src_rpixbuf_8888_asm_neon, 32, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    10, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_rpixbuf_8888_process_pixblock_head, \
    pixman_composite_src_rpixbuf_8888_process_pixblock_tail, \
    pixman_composite_src_rpixbuf_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    0, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_over_0565_8_0565_process_pixblock_head
    /* mask is in d15 */
    convert_0565_to_x888 q4, d2, d1, d0
    convert_0565_to_x888 q5, d6, d5, d4
    /* source pixel data is in      {d0, d1, d2, XX} */
    /* destination pixel data is in {d4, d5, d6, XX} */
    vmvn.8      d7,  d15
    vmull.u8    q6,  d15, d2
    vmull.u8    q5,  d15, d1
    vmull.u8    q4,  d15, d0
    vmull.u8    q8,  d7,  d4
    vmull.u8    q9,  d7,  d5
    vmull.u8    q13, d7,  d6
    vrshr.u16   q12, q6,  #8
    vrshr.u16   q11, q5,  #8
    vrshr.u16   q10, q4,  #8
    vraddhn.u16 d2,  q6,  q12
    vraddhn.u16 d1,  q5,  q11
    vraddhn.u16 d0,  q4,  q10
.endm

.macro pixman_composite_over_0565_8_0565_process_pixblock_tail
    vrshr.u16   q14, q8,  #8
    vrshr.u16   q15, q9,  #8
    vrshr.u16   q12, q13, #8
    vraddhn.u16 d28, q14, q8
    vraddhn.u16 d29, q15, q9
    vraddhn.u16 d30, q12, q13
    vqadd.u8    q0,  q0,  q14
    vqadd.u8    q1,  q1,  q15
    /* 32bpp result is in {d0, d1, d2, XX} */
    convert_8888_to_0565 d2, d1, d0, q14, q15, q3
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_0565_8_0565_process_pixblock_tail_head
    fetch_mask_pixblock
    pixman_composite_over_0565_8_0565_process_pixblock_tail
    fetch_src_pixblock
    vld1.16    {d10, d11}, [DST_R, :128]!
    cache_preload 8, 8
    pixman_composite_over_0565_8_0565_process_pixblock_head
    vst1.16    {d28, d29}, [DST_W, :128]!
.endm

generate_composite_function \
    pixman_composite_over_0565_8_0565_asm_neon, 16, 8, 16, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_over_0565_8_0565_process_pixblock_head, \
    pixman_composite_over_0565_8_0565_process_pixblock_tail, \
    pixman_composite_over_0565_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    10,  /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    15  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_over_0565_n_0565_init
    add         DUMMY, sp, #(ARGS_STACK_OFFSET + 8)
    vpush       {d8-d15}
    vld1.32     {d15[0]}, [DUMMY]
    vdup.8      d15, d15[3]
.endm

.macro pixman_composite_over_0565_n_0565_cleanup
    vpop        {d8-d15}
.endm

generate_composite_function \
    pixman_composite_over_0565_n_0565_asm_neon, 16, 0, 16, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    pixman_composite_over_0565_n_0565_init, \
    pixman_composite_over_0565_n_0565_cleanup, \
    pixman_composite_over_0565_8_0565_process_pixblock_head, \
    pixman_composite_over_0565_8_0565_process_pixblock_tail, \
    pixman_composite_over_0565_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    10, /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    15  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_add_0565_8_0565_process_pixblock_head
    /* mask is in d15 */
    convert_0565_to_x888 q4, d2, d1, d0
    convert_0565_to_x888 q5, d6, d5, d4
    /* source pixel data is in      {d0, d1, d2, XX} */
    /* destination pixel data is in {d4, d5, d6, XX} */
    vmull.u8    q6,  d15, d2
    vmull.u8    q5,  d15, d1
    vmull.u8    q4,  d15, d0
    vrshr.u16   q12, q6,  #8
    vrshr.u16   q11, q5,  #8
    vrshr.u16   q10, q4,  #8
    vraddhn.u16 d2,  q6,  q12
    vraddhn.u16 d1,  q5,  q11
    vraddhn.u16 d0,  q4,  q10
.endm

.macro pixman_composite_add_0565_8_0565_process_pixblock_tail
    vqadd.u8    q0,  q0,  q2
    vqadd.u8    q1,  q1,  q3
    /* 32bpp result is in {d0, d1, d2, XX} */
    convert_8888_to_0565 d2, d1, d0, q14, q15, q3
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_add_0565_8_0565_process_pixblock_tail_head
    fetch_mask_pixblock
    pixman_composite_add_0565_8_0565_process_pixblock_tail
    fetch_src_pixblock
    vld1.16    {d10, d11}, [DST_R, :128]!
    cache_preload 8, 8
    pixman_composite_add_0565_8_0565_process_pixblock_head
    vst1.16    {d28, d29}, [DST_W, :128]!
.endm

generate_composite_function \
    pixman_composite_add_0565_8_0565_asm_neon, 16, 8, 16, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_add_0565_8_0565_process_pixblock_head, \
    pixman_composite_add_0565_8_0565_process_pixblock_tail, \
    pixman_composite_add_0565_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    10, /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    15  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_out_reverse_8_0565_process_pixblock_head
    /* mask is in d15 */
    convert_0565_to_x888 q5, d6, d5, d4
    /* destination pixel data is in {d4, d5, d6, xx} */
    vmvn.8      d24, d15 /* get inverted alpha */
    /* now do alpha blending */
    vmull.u8    q8, d24, d4
    vmull.u8    q9, d24, d5
    vmull.u8    q10, d24, d6
.endm

.macro pixman_composite_out_reverse_8_0565_process_pixblock_tail
    vrshr.u16   q14, q8, #8
    vrshr.u16   q15, q9, #8
    vrshr.u16   q12, q10, #8
    vraddhn.u16 d0, q14, q8
    vraddhn.u16 d1, q15, q9
    vraddhn.u16 d2, q12, q10
    /* 32bpp result is in {d0, d1, d2, XX} */
    convert_8888_to_0565 d2, d1, d0, q14, q15, q3
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_out_reverse_8_0565_process_pixblock_tail_head
    fetch_src_pixblock
    pixman_composite_out_reverse_8_0565_process_pixblock_tail
    vld1.16    {d10, d11}, [DST_R, :128]!
    cache_preload 8, 8
    pixman_composite_out_reverse_8_0565_process_pixblock_head
    vst1.16    {d28, d29}, [DST_W, :128]!
.endm

generate_composite_function \
    pixman_composite_out_reverse_8_0565_asm_neon, 8, 0, 16, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_out_reverse_8_0565_process_pixblock_head, \
    pixman_composite_out_reverse_8_0565_process_pixblock_tail, \
    pixman_composite_out_reverse_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    10, /* dst_r_basereg */ \
    15, /* src_basereg   */ \
    0   /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_out_reverse_8_8888_process_pixblock_head
    /* src is in d0 */
    /* destination pixel data is in {d4, d5, d6, d7} */
    vmvn.8      d1, d0 /* get inverted alpha */
    /* now do alpha blending */
    vmull.u8    q8, d1, d4
    vmull.u8    q9, d1, d5
    vmull.u8    q10, d1, d6
    vmull.u8    q11, d1, d7
.endm

.macro pixman_composite_out_reverse_8_8888_process_pixblock_tail
    vrshr.u16   q14, q8, #8
    vrshr.u16   q15, q9, #8
    vrshr.u16   q12, q10, #8
    vrshr.u16   q13, q11, #8
    vraddhn.u16 d28, q14, q8
    vraddhn.u16 d29, q15, q9
    vraddhn.u16 d30, q12, q10
    vraddhn.u16 d31, q13, q11
    /* 32bpp result is in {d28, d29, d30, d31} */
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_out_reverse_8_8888_process_pixblock_tail_head
    fetch_src_pixblock
    pixman_composite_out_reverse_8_8888_process_pixblock_tail
    vld4.8    {d4, d5, d6, d7}, [DST_R, :128]!
    cache_preload 8, 8
    pixman_composite_out_reverse_8_8888_process_pixblock_head
    vst4.8    {d28, d29, d30, d31}, [DST_W, :128]!
.endm

generate_composite_function \
    pixman_composite_out_reverse_8_8888_asm_neon, 8, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    5, /* prefetch distance */ \
    default_init, \
    default_cleanup, \
    pixman_composite_out_reverse_8_8888_process_pixblock_head, \
    pixman_composite_out_reverse_8_8888_process_pixblock_tail, \
    pixman_composite_out_reverse_8_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4, /* dst_r_basereg */ \
    0, /* src_basereg   */ \
    0   /* mask_basereg  */

/******************************************************************************/

generate_composite_function_nearest_scanline \
    pixman_scaled_nearest_scanline_8888_8888_OVER_asm_neon, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_over_8888_8888_process_pixblock_head, \
    pixman_composite_over_8888_8888_process_pixblock_tail, \
    pixman_composite_over_8888_8888_process_pixblock_tail_head

generate_composite_function_nearest_scanline \
    pixman_scaled_nearest_scanline_8888_0565_OVER_asm_neon, 32, 0, 16, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_over_8888_0565_process_pixblock_head, \
    pixman_composite_over_8888_0565_process_pixblock_tail, \
    pixman_composite_over_8888_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    24  /* mask_basereg  */

generate_composite_function_nearest_scanline \
    pixman_scaled_nearest_scanline_8888_0565_SRC_asm_neon, 32, 0, 16, \
    FLAG_DST_WRITEONLY | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_8888_0565_process_pixblock_head, \
    pixman_composite_src_8888_0565_process_pixblock_tail, \
    pixman_composite_src_8888_0565_process_pixblock_tail_head

generate_composite_function_nearest_scanline \
    pixman_scaled_nearest_scanline_0565_8888_SRC_asm_neon, 16, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_src_0565_8888_process_pixblock_head, \
    pixman_composite_src_0565_8888_process_pixblock_tail, \
    pixman_composite_src_0565_8888_process_pixblock_tail_head

generate_composite_function_nearest_scanline \
    pixman_scaled_nearest_scanline_8888_8_0565_OVER_asm_neon, 32, 8, 16, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_over_8888_8_0565_process_pixblock_head, \
    pixman_composite_over_8888_8_0565_process_pixblock_tail, \
    pixman_composite_over_8888_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    24  /* mask_basereg  */

generate_composite_function_nearest_scanline \
    pixman_scaled_nearest_scanline_0565_8_0565_OVER_asm_neon, 16, 8, 16, \
    FLAG_DST_READWRITE, \
    8, /* number of pixels, processed in a single block */ \
    default_init_need_all_regs, \
    default_cleanup_need_all_regs, \
    pixman_composite_over_0565_8_0565_process_pixblock_head, \
    pixman_composite_over_0565_8_0565_process_pixblock_tail, \
    pixman_composite_over_0565_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    10,  /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    15  /* mask_basereg  */

/******************************************************************************/

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
    vst1.32   {d0, d1}, [OUT, :128]!
.elseif numpix == 2
    vst1.32   {d0}, [OUT, :64]!
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
    vst1.16   {d2}, [OUT, :64]!
.elseif numpix == 2
    vst1.32   {d2[0]}, [OUT, :32]!
.elseif numpix == 1
    vst1.16   {d2[0]}, [OUT, :16]!
.else
    .error bilinear_store_0565 numpix is unsupported
.endif
.endm

.macro bilinear_interpolate_last_pixel src_fmt, dst_fmt
    bilinear_load_&src_fmt d0, d1, d2
    vmull.u8  q1, d0, d28
    vmlal.u8  q1, d1, d29
    /* 5 cycles bubble */
    vshll.u16 q0, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d2, d30
    vmlal.u16 q0, d3, d30
    /* 5 cycles bubble */
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    /* 3 cycles bubble */
    vmovn.u16 d0, q0
    /* 1 cycle bubble */
    bilinear_store_&dst_fmt 1, q2, q3
.endm

.macro bilinear_interpolate_two_pixels src_fmt, dst_fmt
    bilinear_load_and_vertical_interpolate_two_&src_fmt \
                q1, q11, d0, d1, d20, d21, d22, d23
    vshll.u16 q0, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d2, d30
    vmlal.u16 q0, d3, d30
    vshll.u16 q10, d22, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q10, d22, d31
    vmlal.u16 q10, d23, d31
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d1, q10, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16  q12, q12, q13
    vmovn.u16 d0, q0
    bilinear_store_&dst_fmt 2, q2, q3
.endm

.macro bilinear_interpolate_four_pixels src_fmt, dst_fmt
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
    pld       [TMP2, PF_OFFS]
    vmlsl.u16 q8, d18, d31
    vmlal.u16 q8, d19, d31
    vadd.u16  q12, q12, q13
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d1, q10, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d5, q8, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vmovn.u16 d0, q0
    vmovn.u16 d1, q2
    vadd.u16  q12, q12, q13
    bilinear_store_&dst_fmt 4, q2, q3
.endm

.macro bilinear_interpolate_four_pixels_head src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt&_head
.else
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
.endif
.endm

.macro bilinear_interpolate_four_pixels_tail src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt&_tail
.endif
.endm

.macro bilinear_interpolate_four_pixels_tail_head src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt&_tail_head
.else
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
.endif
.endm

.macro bilinear_interpolate_eight_pixels_head src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_eight_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_eight_pixels_&src_fmt&_&dst_fmt&_head
.else
    bilinear_interpolate_four_pixels_head src_fmt, dst_fmt
    bilinear_interpolate_four_pixels_tail_head src_fmt, dst_fmt
.endif
.endm

.macro bilinear_interpolate_eight_pixels_tail src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_eight_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_eight_pixels_&src_fmt&_&dst_fmt&_tail
.else
    bilinear_interpolate_four_pixels_tail src_fmt, dst_fmt
.endif
.endm

.macro bilinear_interpolate_eight_pixels_tail_head src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_eight_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_eight_pixels_&src_fmt&_&dst_fmt&_tail_head
.else
    bilinear_interpolate_four_pixels_tail_head src_fmt, dst_fmt
    bilinear_interpolate_four_pixels_tail_head src_fmt, dst_fmt
.endif
.endm

.set BILINEAR_FLAG_UNROLL_4,          0
.set BILINEAR_FLAG_UNROLL_8,          1
.set BILINEAR_FLAG_USE_ALL_NEON_REGS, 2

/*
 * Main template macro for generating NEON optimized bilinear scanline
 * functions.
 *
 * Bilinear scanline scaler macro template uses the following arguments:
 *  fname             - name of the function to generate
 *  src_fmt           - source color format (8888 or 0565)
 *  dst_fmt           - destination color format (8888 or 0565)
 *  bpp_shift         - (1 << bpp_shift) is the size of source pixel in bytes
 *  prefetch_distance - prefetch in the source image by that many
 *                      pixels ahead
 */

.macro generate_bilinear_scanline_func fname, src_fmt, dst_fmt, \
                                       src_bpp_shift, dst_bpp_shift, \
                                       prefetch_distance, flags

pixman_asm_function fname
    OUT       .req      r0
    TOP       .req      r1
    BOTTOM    .req      r2
    WT        .req      r3
    WB        .req      r4
    X         .req      r5
    UX        .req      r6
    WIDTH     .req      ip
    TMP1      .req      r3
    TMP2      .req      r4
    PF_OFFS   .req      r7
    TMP3      .req      r8
    TMP4      .req      r9
    STRIDE    .req      r2

    mov       ip, sp
    push      {r4, r5, r6, r7, r8, r9}
    mov       PF_OFFS, #prefetch_distance
    ldmia     ip, {WB, X, UX, WIDTH}
    mul       PF_OFFS, PF_OFFS, UX

.if ((flags) & BILINEAR_FLAG_USE_ALL_NEON_REGS) != 0
    vpush     {d8-d15}
.endif

    sub       STRIDE, BOTTOM, TOP
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
    bilinear_interpolate_last_pixel src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #1
0:
    vadd.u16  q13, q13, q13
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16  q12, q12, q13

    cmp       WIDTH, #2
    blt       0f
    tst       OUT, #(1 << (dst_bpp_shift + 1))
    beq       0f
    bilinear_interpolate_two_pixels src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #2
0:
.if ((flags) & BILINEAR_FLAG_UNROLL_8) != 0
/*********** 8 pixels per iteration *****************/
    cmp       WIDTH, #4
    blt       0f
    tst       OUT, #(1 << (dst_bpp_shift + 2))
    beq       0f
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #4
0:
    subs      WIDTH, WIDTH, #8
    blt       1f
    mov       PF_OFFS, PF_OFFS, asr #(16 - src_bpp_shift)
    bilinear_interpolate_eight_pixels_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #8
    blt       5f
0:
    bilinear_interpolate_eight_pixels_tail_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #8
    bge       0b
5:
    bilinear_interpolate_eight_pixels_tail src_fmt, dst_fmt
1:
    tst       WIDTH, #4
    beq       2f
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
2:
.else
/*********** 4 pixels per iteration *****************/
    subs      WIDTH, WIDTH, #4
    blt       1f
    mov       PF_OFFS, PF_OFFS, asr #(16 - src_bpp_shift)
    bilinear_interpolate_four_pixels_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #4
    blt       5f
0:
    bilinear_interpolate_four_pixels_tail_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #4
    bge       0b
5:
    bilinear_interpolate_four_pixels_tail src_fmt, dst_fmt
1:
/****************************************************/
.endif
    /* handle the remaining trailing pixels */
    tst       WIDTH, #2
    beq       2f
    bilinear_interpolate_two_pixels src_fmt, dst_fmt
2:
    tst       WIDTH, #1
    beq       3f
    bilinear_interpolate_last_pixel src_fmt, dst_fmt
3:
.if ((flags) & BILINEAR_FLAG_USE_ALL_NEON_REGS) != 0
    vpop      {d8-d15}
.endif
    pop       {r4, r5, r6, r7, r8, r9}
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
.endfunc

.endm

/*****************************************************************************/

.set have_bilinear_interpolate_four_pixels_8888_8888, 1

.macro bilinear_interpolate_four_pixels_8888_8888_head
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #2

    vld1.32   {d22}, [TMP1], STRIDE
    vld1.32   {d23}, [TMP1]
    mov       TMP3, X, asr #16
    add       X, X, UX
    add       TMP3, TOP, TMP3, asl #2
    vmull.u8  q8, d22, d28
    vmlal.u8  q8, d23, d29

    vld1.32   {d22}, [TMP2], STRIDE
    vld1.32   {d23}, [TMP2]
    mov       TMP4, X, asr #16
    add       X, X, UX
    add       TMP4, TOP, TMP4, asl #2
    vmull.u8  q9, d22, d28
    vmlal.u8  q9, d23, d29

    vld1.32   {d22}, [TMP3], STRIDE
    vld1.32   {d23}, [TMP3]
    vmull.u8  q10, d22, d28
    vmlal.u8  q10, d23, d29

    vshll.u16 q0, d16, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d16, d30
    vmlal.u16 q0, d17, d30

    pld       [TMP4, PF_OFFS]
    vld1.32   {d16}, [TMP4], STRIDE
    vld1.32   {d17}, [TMP4]
    pld       [TMP4, PF_OFFS]
    vmull.u8  q11, d16, d28
    vmlal.u8  q11, d17, d29

    vshll.u16 q1, d18, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q1, d18, d31
.endm

.macro bilinear_interpolate_four_pixels_8888_8888_tail
    vmlal.u16 q1, d19, d31
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vshll.u16 q2, d20, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q2, d20, d30
    vmlal.u16 q2, d21, d30
    vshll.u16 q3, d22, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q3, d22, d31
    vmlal.u16 q3, d23, d31
    vadd.u16  q12, q12, q13
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d5, q3, #(2 * BILINEAR_INTERPOLATION_BITS)
    vmovn.u16 d6, q0
    vmovn.u16 d7, q2
    vadd.u16  q12, q12, q13
    vst1.32   {d6, d7}, [OUT, :128]!
.endm

.macro bilinear_interpolate_four_pixels_8888_8888_tail_head
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #2
        vmlal.u16 q1, d19, d31
        vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
        vshll.u16 q2, d20, #BILINEAR_INTERPOLATION_BITS
        vmlsl.u16 q2, d20, d30
        vmlal.u16 q2, d21, d30
        vshll.u16 q3, d22, #BILINEAR_INTERPOLATION_BITS
    vld1.32   {d20}, [TMP1], STRIDE
        vmlsl.u16 q3, d22, d31
        vmlal.u16 q3, d23, d31
    vld1.32   {d21}, [TMP1]
    vmull.u8  q8, d20, d28
    vmlal.u8  q8, d21, d29
        vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
        vshrn.u32 d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
        vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vld1.32   {d22}, [TMP2], STRIDE
        vshrn.u32 d5, q3, #(2 * BILINEAR_INTERPOLATION_BITS)
        vadd.u16  q12, q12, q13
    vld1.32   {d23}, [TMP2]
    vmull.u8  q9, d22, d28
    mov       TMP3, X, asr #16
    add       X, X, UX
    add       TMP3, TOP, TMP3, asl #2
    mov       TMP4, X, asr #16
    add       X, X, UX
    add       TMP4, TOP, TMP4, asl #2
    vmlal.u8  q9, d23, d29
    vld1.32   {d22}, [TMP3], STRIDE
        vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vld1.32   {d23}, [TMP3]
    vmull.u8  q10, d22, d28
    vmlal.u8  q10, d23, d29
        vmovn.u16 d6, q0
    vshll.u16 q0, d16, #BILINEAR_INTERPOLATION_BITS
        vmovn.u16 d7, q2
    vmlsl.u16 q0, d16, d30
    vmlal.u16 q0, d17, d30
    pld       [TMP4, PF_OFFS]
    vld1.32   {d16}, [TMP4], STRIDE
        vadd.u16  q12, q12, q13
    vld1.32   {d17}, [TMP4]
    pld       [TMP4, PF_OFFS]
    vmull.u8  q11, d16, d28
    vmlal.u8  q11, d17, d29
        vst1.32   {d6, d7}, [OUT, :128]!
    vshll.u16 q1, d18, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q1, d18, d31
.endm

/*****************************************************************************/

.set have_bilinear_interpolate_eight_pixels_8888_0565, 1

.macro bilinear_interpolate_eight_pixels_8888_0565_head
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #2
    vld1.32   {d20}, [TMP1], STRIDE
    vld1.32   {d21}, [TMP1]
    vmull.u8  q8, d20, d28
    vmlal.u8  q8, d21, d29
    vld1.32   {d22}, [TMP2], STRIDE
    vld1.32   {d23}, [TMP2]
    vmull.u8  q9, d22, d28
    mov       TMP3, X, asr #16
    add       X, X, UX
    add       TMP3, TOP, TMP3, asl #2
    mov       TMP4, X, asr #16
    add       X, X, UX
    add       TMP4, TOP, TMP4, asl #2
    vmlal.u8  q9, d23, d29
    vld1.32   {d22}, [TMP3], STRIDE
    vld1.32   {d23}, [TMP3]
    vmull.u8  q10, d22, d28
    vmlal.u8  q10, d23, d29
    vshll.u16 q0, d16, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d16, d30
    vmlal.u16 q0, d17, d30
    pld       [TMP4, PF_OFFS]
    vld1.32   {d16}, [TMP4], STRIDE
    vld1.32   {d17}, [TMP4]
    pld       [TMP4, PF_OFFS]
    vmull.u8  q11, d16, d28
    vmlal.u8  q11, d17, d29
    vshll.u16 q1, d18, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q1, d18, d31

    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #2
        vmlal.u16 q1, d19, d31
        vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
        vshll.u16 q2, d20, #BILINEAR_INTERPOLATION_BITS
        vmlsl.u16 q2, d20, d30
        vmlal.u16 q2, d21, d30
        vshll.u16 q3, d22, #BILINEAR_INTERPOLATION_BITS
    vld1.32   {d20}, [TMP1], STRIDE
        vmlsl.u16 q3, d22, d31
        vmlal.u16 q3, d23, d31
    vld1.32   {d21}, [TMP1]
    vmull.u8  q8, d20, d28
    vmlal.u8  q8, d21, d29
        vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
        vshrn.u32 d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
        vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vld1.32   {d22}, [TMP2], STRIDE
        vshrn.u32 d5, q3, #(2 * BILINEAR_INTERPOLATION_BITS)
        vadd.u16  q12, q12, q13
    vld1.32   {d23}, [TMP2]
    vmull.u8  q9, d22, d28
    mov       TMP3, X, asr #16
    add       X, X, UX
    add       TMP3, TOP, TMP3, asl #2
    mov       TMP4, X, asr #16
    add       X, X, UX
    add       TMP4, TOP, TMP4, asl #2
    vmlal.u8  q9, d23, d29
    vld1.32   {d22}, [TMP3], STRIDE
        vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vld1.32   {d23}, [TMP3]
    vmull.u8  q10, d22, d28
    vmlal.u8  q10, d23, d29
        vmovn.u16 d8, q0
    vshll.u16 q0, d16, #BILINEAR_INTERPOLATION_BITS
        vmovn.u16 d9, q2
    vmlsl.u16 q0, d16, d30
    vmlal.u16 q0, d17, d30
    pld       [TMP4, PF_OFFS]
    vld1.32   {d16}, [TMP4], STRIDE
        vadd.u16  q12, q12, q13
    vld1.32   {d17}, [TMP4]
    pld       [TMP4, PF_OFFS]
    vmull.u8  q11, d16, d28
    vmlal.u8  q11, d17, d29
    vshll.u16 q1, d18, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q1, d18, d31
.endm

.macro bilinear_interpolate_eight_pixels_8888_0565_tail
    vmlal.u16 q1, d19, d31
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vshll.u16 q2, d20, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q2, d20, d30
    vmlal.u16 q2, d21, d30
    vshll.u16 q3, d22, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q3, d22, d31
    vmlal.u16 q3, d23, d31
    vadd.u16  q12, q12, q13
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d5, q3, #(2 * BILINEAR_INTERPOLATION_BITS)
    vmovn.u16 d10, q0
    vmovn.u16 d11, q2
    vadd.u16  q12, q12, q13

    vuzp.u8   d8, d9
    vuzp.u8   d10, d11
    vuzp.u8   d9, d11
    vuzp.u8   d8, d10
    vshll.u8  q6, d9, #8
    vshll.u8  q5, d10, #8
    vshll.u8  q7, d8, #8
    vsri.u16  q5, q6, #5
    vsri.u16  q5, q7, #11
    vst1.32   {d10, d11}, [OUT, :128]!
.endm

.macro bilinear_interpolate_eight_pixels_8888_0565_tail_head
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #2
        vmlal.u16 q1, d19, d31
        vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
            vuzp.u8 d8, d9
        vshll.u16 q2, d20, #BILINEAR_INTERPOLATION_BITS
        vmlsl.u16 q2, d20, d30
        vmlal.u16 q2, d21, d30
        vshll.u16 q3, d22, #BILINEAR_INTERPOLATION_BITS
    vld1.32   {d20}, [TMP1], STRIDE
        vmlsl.u16 q3, d22, d31
        vmlal.u16 q3, d23, d31
    vld1.32   {d21}, [TMP1]
    vmull.u8  q8, d20, d28
    vmlal.u8  q8, d21, d29
        vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
        vshrn.u32 d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
        vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vld1.32   {d22}, [TMP2], STRIDE
        vshrn.u32 d5, q3, #(2 * BILINEAR_INTERPOLATION_BITS)
        vadd.u16  q12, q12, q13
    vld1.32   {d23}, [TMP2]
    vmull.u8  q9, d22, d28
    mov       TMP3, X, asr #16
    add       X, X, UX
    add       TMP3, TOP, TMP3, asl #2
    mov       TMP4, X, asr #16
    add       X, X, UX
    add       TMP4, TOP, TMP4, asl #2
    vmlal.u8  q9, d23, d29
    vld1.32   {d22}, [TMP3], STRIDE
        vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vld1.32   {d23}, [TMP3]
    vmull.u8  q10, d22, d28
    vmlal.u8  q10, d23, d29
        vmovn.u16 d10, q0
    vshll.u16 q0, d16, #BILINEAR_INTERPOLATION_BITS
        vmovn.u16 d11, q2
    vmlsl.u16 q0, d16, d30
    vmlal.u16 q0, d17, d30
    pld       [TMP4, PF_OFFS]
    vld1.32   {d16}, [TMP4], STRIDE
        vadd.u16  q12, q12, q13
    vld1.32   {d17}, [TMP4]
    pld       [TMP4, PF_OFFS]
    vmull.u8  q11, d16, d28
    vmlal.u8  q11, d17, d29
            vuzp.u8 d10, d11
    vshll.u16 q1, d18, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q1, d18, d31

    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #2
        vmlal.u16 q1, d19, d31
            vuzp.u8 d9, d11
        vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
        vshll.u16 q2, d20, #BILINEAR_INTERPOLATION_BITS
            vuzp.u8 d8, d10
        vmlsl.u16 q2, d20, d30
        vmlal.u16 q2, d21, d30
        vshll.u16 q3, d22, #BILINEAR_INTERPOLATION_BITS
    vld1.32   {d20}, [TMP1], STRIDE
        vmlsl.u16 q3, d22, d31
        vmlal.u16 q3, d23, d31
    vld1.32   {d21}, [TMP1]
    vmull.u8  q8, d20, d28
    vmlal.u8  q8, d21, d29
            vshll.u8  q6, d9, #8
            vshll.u8  q5, d10, #8
            vshll.u8  q7, d8, #8
        vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
            vsri.u16  q5, q6, #5
        vshrn.u32 d1, q1, #(2 * BILINEAR_INTERPOLATION_BITS)
            vsri.u16  q5, q7, #11
        vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vld1.32   {d22}, [TMP2], STRIDE
        vshrn.u32 d5, q3, #(2 * BILINEAR_INTERPOLATION_BITS)
        vadd.u16  q12, q12, q13
    vld1.32   {d23}, [TMP2]
    vmull.u8  q9, d22, d28
    mov       TMP3, X, asr #16
    add       X, X, UX
    add       TMP3, TOP, TMP3, asl #2
    mov       TMP4, X, asr #16
    add       X, X, UX
    add       TMP4, TOP, TMP4, asl #2
    vmlal.u8  q9, d23, d29
    vld1.32   {d22}, [TMP3], STRIDE
        vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vld1.32   {d23}, [TMP3]
    vmull.u8  q10, d22, d28
    vmlal.u8  q10, d23, d29
        vmovn.u16 d8, q0
    vshll.u16 q0, d16, #BILINEAR_INTERPOLATION_BITS
        vmovn.u16 d9, q2
    vmlsl.u16 q0, d16, d30
    vmlal.u16 q0, d17, d30
    pld       [TMP4, PF_OFFS]
    vld1.32   {d16}, [TMP4], STRIDE
        vadd.u16  q12, q12, q13
    vld1.32   {d17}, [TMP4]
    pld       [TMP4, PF_OFFS]
    vmull.u8  q11, d16, d28
    vmlal.u8  q11, d17, d29
    vshll.u16 q1, d18, #BILINEAR_INTERPOLATION_BITS
            vst1.32   {d10, d11}, [OUT, :128]!
    vmlsl.u16 q1, d18, d31
.endm
/*****************************************************************************/

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_neon, 8888, 8888, \
    2, 2, 28, BILINEAR_FLAG_UNROLL_4

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_0565_SRC_asm_neon, 8888, 0565, \
    2, 1, 28, BILINEAR_FLAG_UNROLL_8 | BILINEAR_FLAG_USE_ALL_NEON_REGS

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_0565_x888_SRC_asm_neon, 0565, 8888, \
    1, 2, 28, BILINEAR_FLAG_UNROLL_4

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_0565_0565_SRC_asm_neon, 0565, 0565, \
    1, 1, 28, BILINEAR_FLAG_UNROLL_4
