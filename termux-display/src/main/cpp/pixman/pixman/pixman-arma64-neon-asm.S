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
.arch armv8-a

.altmacro
.p2align 2

#include "pixman-private.h"
#include "pixman-arm-asm.h"
#include "pixman-arma64-neon-asm.h"

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
 * v0,  v1,  v2,  v3  - contain loaded source pixel data
 * v4,  v5,  v6,  v7  - contain loaded destination pixels (if they are needed)
 * v24, v25, v26, v27 - contain loading mask pixel data (if mask is used)
 * v28, v29, v30, v31 - place for storing the result (destination pixels)
 *
 * As can be seen above, four 64-bit NEON registers are used for keeping
 * intermediate pixel data and up to 8 pixels can be processed in one step
 * for 32bpp formats (16 pixels for 16bpp, 32 pixels for 8bpp).
 *
 * This particular function uses the following registers allocation:
 * v0,  v1,  v2,  v3  - contain loaded source pixel data
 * v4,  v5            - contain loaded destination pixels (they are needed)
 * v28, v29           - place for storing the result (destination pixels)
 */

/*
 * Step one. We need to have some code to do some arithmetics on pixel data.
 * This is implemented as a pair of macros: '*_head' and '*_tail'. When used
 * back-to-back, they take pixel data from {v0, v1, v2, v3} and {v4, v5},
 * perform all the needed calculations and write the result to {v28, v29}.
 * The rationale for having two macros and not just one will be explained
 * later. In practice, any single monolitic function which does the work can
 * be split into two parts in any arbitrary way without affecting correctness.
 *
 * There is one special trick here too. Common template macro can optionally
 * make our life a bit easier by doing R, G, B, A color components
 * deinterleaving for 32bpp pixel formats (and this feature is used in
 * 'pixman_composite_over_8888_0565_asm_neon' function). So it means that
 * instead of having 8 packed pixels in {v0, v1, v2, v3} registers, we
 * actually use v0 register for blue channel (a vector of eight 8-bit
 * values), v1 register for green, v2 for red and v3 for alpha. This
 * simple conversion can be also done with a few NEON instructions:
 *
 * Packed to planar conversion: // vuzp8 is a wrapper macro
 *  vuzp8 v0, v1
 *  vuzp8 v2, v3
 *  vuzp8 v1, v3
 *  vuzp8 v0, v2
 *
 * Planar to packed conversion: // vzip8 is a wrapper macro
 *  vzip8 v0, v2
 *  vzip8 v1, v3
 *  vzip8 v2, v3
 *  vzip8 v0, v1
 *
 * But pixel can be loaded directly in planar format using LD4 / b NEON
 * instruction. It is 1 cycle slower than LD1 / s, so this is not always
 * desirable, that's why deinterleaving is optional.
 *
 * But anyway, here is the code:
 */

.macro pixman_composite_over_8888_0565_process_pixblock_head
    /* convert 8 r5g6b5 pixel data from {v4} to planar 8-bit format
       and put data into v6 - red, v7 - green, v30 - blue */
    mov         v4.d[1], v5.d[0]
    shrn        v6.8b, v4.8h, #8
    shrn        v7.8b, v4.8h, #3
    sli         v4.8h, v4.8h, #5
    sri         v6.8b, v6.8b, #5
    mvn         v3.8b, v3.8b      /* invert source alpha */
    sri         v7.8b, v7.8b, #6
    shrn        v30.8b, v4.8h, #2
    /* now do alpha blending, storing results in 8-bit planar format
       into v20 - red, v23 - green, v22 - blue */
    umull       v10.8h, v3.8b, v6.8b
    umull       v11.8h, v3.8b, v7.8b
    umull       v12.8h, v3.8b, v30.8b
    urshr       v17.8h, v10.8h, #8
    urshr       v18.8h, v11.8h, #8
    urshr       v19.8h, v12.8h, #8
    raddhn      v20.8b, v10.8h, v17.8h
    raddhn      v23.8b, v11.8h, v18.8h
    raddhn      v22.8b, v12.8h, v19.8h
.endm

.macro pixman_composite_over_8888_0565_process_pixblock_tail
    /* ... continue alpha blending */
    uqadd       v17.8b, v2.8b, v20.8b
    uqadd       v18.8b, v0.8b, v22.8b
    uqadd       v19.8b, v1.8b, v23.8b
    /* convert the result to r5g6b5 and store it into {v14} */
    ushll       v14.8h, v17.8b, #7
    sli         v14.8h, v14.8h, #1
    ushll       v8.8h, v19.8b, #7
    sli         v8.8h, v8.8h, #1
    ushll       v9.8h, v18.8b, #7
    sli         v9.8h, v9.8h, #1
    sri         v14.8h, v8.8h, #5
    sri         v14.8h, v9.8h, #11
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
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
 *   st1         {v28.4h, v29.4h}, [DST_W], #32
 *   ld1         {v4.4h, v5.4h}, [DST_R], #16
 *   ld4         {v0.2s, v1.2s, v2.2s, v3.2s}, [SRC], #32
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
 * indentation levels and ignoring a few LD/ST instructions would
 * result in exactly the code from '*_head', '*_tail' or 'cache_preload'
 * macro!
 */

#if 1

.macro pixman_composite_over_8888_0565_process_pixblock_tail_head
        uqadd       v17.8b, v2.8b, v20.8b
    ld1         {v4.4h, v5.4h}, [DST_R], #16
    mov         v4.d[1], v5.d[0]
        uqadd       v18.8b, v0.8b, v22.8b
        uqadd       v19.8b, v1.8b, v23.8b
    shrn        v6.8b, v4.8h, #8
    fetch_src_pixblock
    shrn        v7.8b, v4.8h, #3
    sli         v4.8h, v4.8h, #5
        ushll       v14.8h, v17.8b, #7
        sli         v14.8h, v14.8h, #1
                                    PF add PF_X, PF_X, #8
        ushll       v8.8h, v19.8b, #7
        sli         v8.8h, v8.8h,  #1
                                    PF tst PF_CTL, #0xF
    sri         v6.8b, v6.8b, #5
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
10:
    mvn         v3.8b, v3.8b
                                    PF beq 10f
                                    PF sub PF_CTL, PF_CTL, #1
10:
    sri         v7.8b, v7.8b, #6
    shrn        v30.8b, v4.8h, #2
    umull       v10.8h, v3.8b, v6.8b
                                    PF lsl DUMMY, PF_X, #src_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_SRC, DUMMY]
    umull       v11.8h, v3.8b, v7.8b
    umull       v12.8h, v3.8b, v30.8b
                                    PF lsl DUMMY, PF_X, #dst_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_DST, DUMMY]
        sri         v14.8h, v8.8h, #5
                                    PF cmp PF_X, ORIG_W
        ushll       v9.8h, v18.8b, #7
        sli         v9.8h, v9.8h, #1
    urshr       v17.8h, v10.8h, #8
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
10:
    urshr       v19.8h, v11.8h, #8
    urshr       v18.8h, v12.8h, #8
                                    PF ble 10f
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
        sri         v14.8h, v9.8h, #11
        mov         v28.d[0], v14.d[0]
        mov         v29.d[0], v14.d[1]
                                    PF ble 10f
                                    PF lsl DUMMY, SRC_STRIDE, #src_bpp_shift
                                    PF ldrsb DUMMY, [PF_SRC, DUMMY]
                                    PF add PF_SRC, PF_SRC, #1
10:
    raddhn      v20.8b, v10.8h, v17.8h
    raddhn      v23.8b, v11.8h, v19.8h
                                    PF ble 10f
                                    PF lsl DUMMY, DST_STRIDE, #dst_bpp_shift
                                    PF ldrsb DUMMY, [PF_DST, DUMMY]
                                    PF add PF_DST, PF_SRC, #1
10:
    raddhn      v22.8b, v12.8h, v18.8h
        st1         {v14.8h}, [DST_W], #16
.endm

#else

/* If we did not care much about the performance, we would just use this... */
.macro pixman_composite_over_8888_0565_process_pixblock_tail_head
    pixman_composite_over_8888_0565_process_pixblock_tail
    st1         {v14.8h}, [DST_W], #16
    ld1         {v4.4h, v4.5h}, [DST_R], #16
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
    /* convert 8 r5g6b5 pixel data from {v4} to planar 8-bit format
       and put data into v6 - red, v7 - green, v30 - blue */
    mov         v4.d[1], v5.d[0]
    shrn        v6.8b, v4.8h, #8
    shrn        v7.8b, v4.8h, #3
    sli         v4.8h, v4.8h, #5
    sri         v6.8b, v6.8b, #5
    sri         v7.8b, v7.8b, #6
    shrn        v30.8b, v4.8h, #2
    /* now do alpha blending, storing results in 8-bit planar format
       into v20 - red, v23 - green, v22 - blue */
    umull       v10.8h, v3.8b, v6.8b
    umull       v11.8h, v3.8b, v7.8b
    umull       v12.8h, v3.8b, v30.8b
    urshr       v13.8h, v10.8h, #8
    urshr       v14.8h, v11.8h, #8
    urshr       v15.8h, v12.8h, #8
    raddhn      v20.8b, v10.8h, v13.8h
    raddhn      v23.8b, v11.8h, v14.8h
    raddhn      v22.8b, v12.8h, v15.8h
.endm

.macro pixman_composite_over_n_0565_process_pixblock_tail
    /* ... continue alpha blending */
    uqadd       v17.8b, v2.8b, v20.8b
    uqadd       v18.8b, v0.8b, v22.8b
    uqadd       v19.8b, v1.8b, v23.8b
    /* convert the result to r5g6b5 and store it into {v14} */
    ushll       v14.8h, v17.8b, #7
    sli         v14.8h, v14.8h, #1
    ushll       v8.8h, v19.8b, #7
    sli         v8.8h, v8.8h, #1
    ushll       v9.8h, v18.8b, #7
    sli         v9.8h, v9.8h, #1
    sri         v14.8h, v8.8h, #5
    sri         v14.8h, v9.8h, #11
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_n_0565_process_pixblock_tail_head
    pixman_composite_over_n_0565_process_pixblock_tail
    ld1         {v4.4h, v5.4h}, [DST_R], #16
    st1         {v14.8h}, [DST_W], #16
    pixman_composite_over_n_0565_process_pixblock_head
    cache_preload 8, 8
.endm

.macro pixman_composite_over_n_0565_init
    mov         v3.s[0], w4
    dup         v0.8b, v3.b[0]
    dup         v1.8b, v3.b[1]
    dup         v2.8b, v3.b[2]
    dup         v3.8b, v3.b[3]
    mvn         v3.8b, v3.8b      /* invert source alpha */
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
    ushll       v8.8h,  v1.8b,  #7
    sli         v8.8h,  v8.8h,  #1
    ushll       v14.8h, v2.8b,  #7
    sli         v14.8h, v14.8h, #1
    ushll       v9.8h,  v0.8b,  #7
    sli         v9.8h,  v9.8h,  #1
.endm

.macro pixman_composite_src_8888_0565_process_pixblock_tail
    sri         v14.8h, v8.8h, #5
    sri         v14.8h, v9.8h, #11
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
.endm

.macro pixman_composite_src_8888_0565_process_pixblock_tail_head
        sri         v14.8h, v8.8h, #5
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
    fetch_src_pixblock
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
                                    PF sub PF_CTL, PF_CTL, #1
10:
        sri         v14.8h, v9.8h, #11
        mov         v28.d[0], v14.d[0]
        mov         v29.d[0], v14.d[1]
                                    PF cmp PF_X, ORIG_W
                                    PF lsl DUMMY, PF_X, #src_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_SRC, DUMMY]
    ushll       v8.8h, v1.8b, #7
    sli         v8.8h, v8.8h, #1
        st1        {v14.8h}, [DST_W], #16
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    ushll       v14.8h, v2.8b, #7
    sli         v14.8h, v14.8h, #1
                                    PF ble 10f
                                    PF lsl DUMMY, SRC_STRIDE, #src_bpp_shift
                                    PF ldrsb DUMMY, [PF_SRC, DUMMY]
                                    PF add PF_SRC, PF_SRC, #1
10:
    ushll       v9.8h, v0.8b, #7
    sli         v9.8h, v9.8h, #1
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
    mov         v0.d[1], v1.d[0]
    shrn        v30.8b, v0.8h, #8
    shrn        v29.8b, v0.8h, #3
    sli         v0.8h,  v0.8h, #5
    movi        v31.8b, #255
    sri         v30.8b, v30.8b, #5
    sri         v29.8b, v29.8b, #6
    shrn        v28.8b, v0.8h, #2
.endm

.macro pixman_composite_src_0565_8888_process_pixblock_tail
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_src_0565_8888_process_pixblock_tail_head
    pixman_composite_src_0565_8888_process_pixblock_tail
    st4         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
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
    uqadd       v28.8b, v0.8b, v4.8b
    uqadd       v29.8b, v1.8b, v5.8b
    uqadd       v30.8b, v2.8b, v6.8b
    uqadd       v31.8b, v3.8b, v7.8b
.endm

.macro pixman_composite_add_8_8_process_pixblock_tail
.endm

.macro pixman_composite_add_8_8_process_pixblock_tail_head
    fetch_src_pixblock
                                    PF add PF_X, PF_X, #32
                                    PF tst PF_CTL, #0xF
    ld1         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
                                    PF beq 10f
                                    PF add PF_X, PF_X, #32
                                    PF sub PF_CTL, PF_CTL, #1
10:
        st1     {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
                                    PF cmp PF_X, ORIG_W
                                    PF lsl DUMMY, PF_X, #src_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_SRC, DUMMY]
                                    PF lsl DUMMY, PF_X, #dst_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_DST, DUMMY]
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    uqadd       v28.8b, v0.8b, v4.8b
                                    PF ble 10f
                                    PF lsl DUMMY, SRC_STRIDE, #src_bpp_shift
                                    PF ldrsb DUMMY, [PF_SRC, DUMMY]
                                    PF add PF_SRC, PF_SRC, #1
                                    PF lsl DUMMY, DST_STRIDE, #dst_bpp_shift
                                    PF ldrsb DUMMY, [PF_DST, DUMMY]
                                    PF add PF_DST, PF_DST, #1
10:
    uqadd       v29.8b, v1.8b, v5.8b
    uqadd       v30.8b, v2.8b, v6.8b
    uqadd       v31.8b, v3.8b, v7.8b
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
    ld1         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
                                    PF sub PF_CTL, PF_CTL, #1
10:
        st1     {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
                                    PF cmp PF_X, ORIG_W
                                    PF lsl DUMMY, PF_X, #src_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_SRC, DUMMY]
                                    PF lsl DUMMY, PF_X, #dst_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_DST, DUMMY]
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    uqadd       v28.8b, v0.8b, v4.8b
                                    PF ble 10f
                                    PF lsl DUMMY, SRC_STRIDE, #src_bpp_shift
                                    PF ldrsb DUMMY, [PF_SRC, DUMMY]
                                    PF add PF_SRC, PF_SRC, #1
                                    PF lsl DUMMY, DST_STRIDE, #dst_bpp_shift
                                    PF ldrsb DUMMY, [PF_DST, DUMMY]
                                    PF add PF_DST, PF_DST, #1
10:
    uqadd       v29.8b, v1.8b, v5.8b
    uqadd       v30.8b, v2.8b, v6.8b
    uqadd       v31.8b, v3.8b, v7.8b
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
    mvn         v24.8b, v3.8b  /* get inverted alpha */
    /* do alpha blending */
    umull       v8.8h, v24.8b, v4.8b
    umull       v9.8h, v24.8b, v5.8b
    umull       v10.8h, v24.8b, v6.8b
    umull       v11.8h, v24.8b, v7.8b
.endm

.macro pixman_composite_out_reverse_8888_8888_process_pixblock_tail
    urshr       v14.8h, v8.8h, #8
    urshr       v15.8h, v9.8h, #8
    urshr       v16.8h, v10.8h, #8
    urshr       v17.8h, v11.8h, #8
    raddhn      v28.8b, v14.8h, v8.8h
    raddhn      v29.8b, v15.8h, v9.8h
    raddhn      v30.8b, v16.8h, v10.8h
    raddhn      v31.8b, v17.8h, v11.8h
.endm

.macro pixman_composite_out_reverse_8888_8888_process_pixblock_tail_head
     ld4        {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
        urshr       v14.8h, v8.8h, #8
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
        urshr       v15.8h, v9.8h, #8
        urshr       v16.8h, v10.8h, #8
        urshr       v17.8h, v11.8h, #8
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
                                    PF sub PF_CTL, PF_CTL, #1
10:
        raddhn      v28.8b, v14.8h, v8.8h
        raddhn      v29.8b, v15.8h, v9.8h
                                    PF cmp PF_X, ORIG_W
        raddhn      v30.8b, v16.8h, v10.8h
        raddhn      v31.8b, v17.8h, v11.8h
    fetch_src_pixblock
                                    PF lsl DUMMY, PF_X, #src_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_SRC, DUMMY]
    mvn         v22.8b, v3.8b
                                    PF lsl DUMMY, PF_X, #dst_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_DST, DUMMY]
         st4        {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
10:
    umull      v8.8h, v22.8b, v4.8b
                                    PF ble 10f
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    umull      v9.8h, v22.8b, v5.8b
                                    PF ble 10f
                                    PF lsl DUMMY, SRC_STRIDE, #src_bpp_shift
                                    PF ldrsb DUMMY, [PF_SRC, DUMMY]
                                    PF add PF_SRC, PF_SRC, #1
10:
    umull      v10.8h, v22.8b, v6.8b
                                    PF ble 10f
                                    PF lsl DUMMY, DST_STRIDE, #dst_bpp_shift
                                    PF ldrsb DUMMY, [PF_DST, DUMMY]
                                    PF add PF_DST, PF_DST, #1
10:
     umull     v11.8h, v22.8b, v7.8b
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
    uqadd       v28.8b, v0.8b, v28.8b
    uqadd       v29.8b, v1.8b, v29.8b
    uqadd       v30.8b, v2.8b, v30.8b
    uqadd       v31.8b, v3.8b, v31.8b
.endm

.macro pixman_composite_over_8888_8888_process_pixblock_tail_head
     ld4        {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
        urshr       v14.8h, v8.8h, #8
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
        urshr       v15.8h, v9.8h, #8
        urshr       v16.8h, v10.8h, #8
        urshr       v17.8h, v11.8h, #8
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
                                    PF sub PF_CTL, PF_CTL, #1
10:
        raddhn      v28.8b, v14.8h, v8.8h
        raddhn      v29.8b, v15.8h, v9.8h
                                    PF cmp PF_X, ORIG_W
        raddhn      v30.8b, v16.8h, v10.8h
        raddhn      v31.8b, v17.8h, v11.8h
        uqadd       v28.8b, v0.8b, v28.8b
        uqadd       v29.8b, v1.8b, v29.8b
        uqadd       v30.8b, v2.8b, v30.8b
        uqadd       v31.8b, v3.8b, v31.8b
    fetch_src_pixblock
                                    PF lsl DUMMY, PF_X, #src_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_SRC, DUMMY]
    mvn        v22.8b, v3.8b
                                    PF lsl DUMMY, PF_X, #dst_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_DST, DUMMY]
         st4        {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
10:
    umull      v8.8h, v22.8b, v4.8b
                                    PF ble 10f
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    umull      v9.8h, v22.8b, v5.8b
                                    PF ble 10f
                                    PF lsl DUMMY, SRC_STRIDE, #src_bpp_shift
                                    PF ldrsb DUMMY, [PF_SRC, DUMMY]
                                    PF add PF_SRC, PF_SRC, #1
10:
    umull      v10.8h, v22.8b, v6.8b
                                    PF ble 10f
                                    PF lsl DUMMY, DST_STRIDE, #dst_bpp_shift
                                    PF ldrsb DUMMY, [PF_DST, DUMMY]
                                    PF add PF_DST, PF_DST, #1
10:
    umull      v11.8h, v22.8b, v7.8b
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
    /* deinterleaved source pixels in {v0, v1, v2, v3} */
    /* inverted alpha in {v24} */
    /* destination pixels in {v4, v5, v6, v7} */
    umull       v8.8h, v24.8b, v4.8b
    umull       v9.8h, v24.8b, v5.8b
    umull       v10.8h, v24.8b, v6.8b
    umull       v11.8h, v24.8b, v7.8b
.endm

.macro pixman_composite_over_n_8888_process_pixblock_tail
    urshr       v14.8h, v8.8h, #8
    urshr       v15.8h, v9.8h, #8
    urshr       v16.8h, v10.8h, #8
    urshr       v17.8h, v11.8h, #8
    raddhn      v28.8b, v14.8h, v8.8h
    raddhn      v29.8b, v15.8h, v9.8h
    raddhn      v30.8b, v16.8h, v10.8h
    raddhn      v31.8b, v17.8h, v11.8h
    uqadd       v28.8b, v0.8b, v28.8b
    uqadd       v29.8b, v1.8b, v29.8b
    uqadd       v30.8b, v2.8b, v30.8b
    uqadd       v31.8b, v3.8b, v31.8b
.endm

.macro pixman_composite_over_n_8888_process_pixblock_tail_head
        urshr       v14.8h, v8.8h, #8
        urshr       v15.8h, v9.8h, #8
        urshr       v16.8h, v10.8h, #8
        urshr       v17.8h, v11.8h, #8
        raddhn      v28.8b, v14.8h, v8.8h
        raddhn      v29.8b, v15.8h, v9.8h
        raddhn      v30.8b, v16.8h, v10.8h
        raddhn      v31.8b, v17.8h, v11.8h
    ld4         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
        uqadd       v28.8b, v0.8b, v28.8b
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0x0F
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
                                    PF sub PF_CTL, PF_CTL, #1
10:
        uqadd       v29.8b, v1.8b, v29.8b
        uqadd       v30.8b, v2.8b, v30.8b
        uqadd       v31.8b, v3.8b, v31.8b
                                    PF cmp PF_X, ORIG_W
    umull       v8.8h, v24.8b, v4.8b
                                    PF lsl DUMMY, PF_X, #dst_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_DST, DUMMY]
    umull       v9.8h, v24.8b, v5.8b
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
10:
    umull       v10.8h, v24.8b, v6.8b
                                    PF subs PF_CTL, PF_CTL, #0x10
    umull       v11.8h, v24.8b, v7.8b
                                    PF ble 10f
                                    PF lsl DUMMY, DST_STRIDE, #dst_bpp_shift
                                    PF ldrsb DUMMY, [PF_DST, DUMMY]
                                    PF add PF_DST, PF_DST, #1
10:
        st4         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
.endm

.macro pixman_composite_over_n_8888_init
    mov         v3.s[0], w4
    dup         v0.8b, v3.b[0]
    dup         v1.8b, v3.b[1]
    dup         v2.8b, v3.b[2]
    dup         v3.8b, v3.b[3]
    mvn         v24.8b, v3.8b  /* get inverted alpha */
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
        urshr       v14.8h, v8.8h, #8
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
        urshr       v15.8h, v9.8h, #8
        urshr       v12.8h, v10.8h, #8
        urshr       v13.8h, v11.8h, #8
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
                                    PF sub PF_CTL, PF_CTL, #1
10:
        raddhn      v28.8b, v14.8h, v8.8h
        raddhn      v29.8b, v15.8h, v9.8h
                                    PF cmp PF_X, ORIG_W
        raddhn      v30.8b, v12.8h, v10.8h
        raddhn      v31.8b, v13.8h, v11.8h
        uqadd       v28.8b, v0.8b, v28.8b
        uqadd       v29.8b, v1.8b, v29.8b
        uqadd       v30.8b, v2.8b, v30.8b
        uqadd       v31.8b, v3.8b, v31.8b
    ld4         {v0.8b, v1.8b, v2.8b, v3.8b}, [DST_R], #32
    mvn         v22.8b, v3.8b
                                    PF lsl DUMMY, PF_X, #dst_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_DST, DUMMY]
        st4         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
                                    PF blt 10f
                                    PF sub PF_X, PF_X, ORIG_W
10:
    umull       v8.8h, v22.8b, v4.8b
                                    PF blt 10f
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    umull       v9.8h, v22.8b, v5.8b
    umull       v10.8h, v22.8b, v6.8b
                                    PF blt 10f
                                    PF lsl DUMMY, DST_STRIDE, #dst_bpp_shift
                                    PF ldrsb DUMMY, [PF_DST, DUMMY]
                                    PF add PF_DST, PF_DST, #1
10:
    umull       v11.8h, v22.8b, v7.8b
.endm

.macro pixman_composite_over_reverse_n_8888_init
    mov         v7.s[0], w4
    dup         v4.8b, v7.b[0]
    dup         v5.8b, v7.b[1]
    dup         v6.8b, v7.b[2]
    dup         v7.8b, v7.b[3]
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
    umull       v0.8h,  v24.8b, v8.8b    /* IN for SRC pixels (part1) */
    umull       v1.8h,  v24.8b, v9.8b
    umull       v2.8h,  v24.8b, v10.8b
    umull       v3.8h,  v24.8b, v11.8b
        mov         v4.d[1], v5.d[0]
        shrn        v25.8b,  v4.8h, #8 /* convert DST_R data to 32-bpp (part1) */
        shrn        v26.8b,  v4.8h, #3
        sli         v4.8h,   v4.8h, #5
    urshr       v17.8h, v0.8h,  #8    /* IN for SRC pixels (part2) */
    urshr       v18.8h, v1.8h,  #8
    urshr       v19.8h, v2.8h,  #8
    urshr       v20.8h, v3.8h,  #8
    raddhn      v0.8b,  v0.8h,  v17.8h
    raddhn      v1.8b,  v1.8h,  v18.8h
    raddhn      v2.8b,  v2.8h,  v19.8h
    raddhn      v3.8b,  v3.8h,  v20.8h
        sri         v25.8b, v25.8b, #5 /* convert DST_R data to 32-bpp (part2) */
        sri         v26.8b, v26.8b, #6
    mvn         v3.8b,  v3.8b
        shrn        v30.8b, v4.8h,  #2
    umull       v18.8h, v3.8b, v25.8b     /* now do alpha blending */
    umull       v19.8h, v3.8b, v26.8b
    umull       v20.8h, v3.8b, v30.8b
.endm

.macro pixman_composite_over_8888_8_0565_process_pixblock_tail
    /* 3 cycle bubble (after vmull.u8) */
    urshr       v5.8h, v18.8h, #8
    urshr       v6.8h, v19.8h, #8
    urshr       v7.8h, v20.8h, #8
    raddhn      v17.8b, v18.8h, v5.8h
    raddhn      v19.8b, v19.8h, v6.8h
    raddhn      v18.8b, v20.8h, v7.8h
    uqadd       v5.8b, v2.8b,  v17.8b
    /* 1 cycle bubble */
    uqadd       v6.8b, v0.8b,  v18.8b
    uqadd       v7.8b, v1.8b,  v19.8b
    ushll       v14.8h, v5.8b, #7    /* convert to 16bpp */
    sli         v14.8h, v14.8h, #1
    ushll       v18.8h, v7.8b, #7
    sli         v18.8h, v18.8h, #1
    ushll       v19.8h, v6.8b, #7
    sli         v19.8h, v19.8h, #1
    sri         v14.8h, v18.8h, #5
    /* 1 cycle bubble */
    sri         v14.8h, v19.8h, #11
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
.endm

.macro pixman_composite_over_8888_8_0565_process_pixblock_tail_head
#if 0
    ld1         {v4.8h}, [DST_R], #16
    shrn        v25.8b,  v4.8h,  #8
    fetch_mask_pixblock
    shrn        v26.8b,  v4.8h,  #3
    fetch_src_pixblock
    umull       v22.8h,  v24.8b, v10.8b
        urshr       v13.8h, v18.8h, #8
        urshr       v11.8h, v19.8h, #8
        urshr       v15.8h, v20.8h, #8
        raddhn      v17.8b, v18.8h, v13.8h
        raddhn      v19.8b, v19.8h, v11.8h
        raddhn      v18.8b, v20.8h, v15.8h
        uqadd       v17.8b, v2.8b, v17.8b
    umull       v21.8h,  v24.8b, v9.8b
        uqadd       v18.8b, v0.8b, v18.8b
        uqadd       v19.8b, v1.8b, v19.8b
        ushll       v14.8h, v17.8b, #7
        sli         v14.8h, v14.8h, #1
    umull       v20.8h,  v24.8b, v8.8b
        ushll       v18.8h,  v18.8b, #7
        sli         v18.8h,  v18.8h, #1
        ushll       v19.8h,  v19.8b, #7
        sli         v19.8h,  v19.8h, #1
        sri         v14.8h,  v18.8h, #5
    umull       v23.8h,  v24.8b, v11.8b
        sri         v14.8h,  v19.8h, #11
        mov         v28.d[0], v14.d[0]
        mov         v29.d[0], v14.d[1]

    cache_preload 8, 8

    sli         v4.8h,  v4.8h,   #5
    urshr       v16.8h, v20.8h,  #8
    urshr       v17.8h, v21.8h,  #8
    urshr       v18.8h, v22.8h,  #8
    urshr       v19.8h, v23.8h,  #8
    raddhn      v0.8b,  v20.8h, v16.8h
    raddhn      v1.8b,  v21.8h, v17.8h
    raddhn      v2.8b,  v22.8h, v18.8h
    raddhn      v3.8b,  v23.8h, v19.8h
    sri         v25.8b,  v25.8b,  #5
    sri         v26.8b,  v26.8b,  #6
    mvn         v3.8b,  v3.8b
    shrn        v30.8b, v4.8h,  #2
    st1         {v14.8h}, [DST_W], #16
    umull       v18.8h, v3.8b, v25.8b
    umull       v19.8h, v3.8b, v26.8b
    umull       v20.8h, v3.8b, v30.8b
#else
    pixman_composite_over_8888_8_0565_process_pixblock_tail
    st1         {v28.4h, v29.4h}, [DST_W], #16
    ld1         {v4.4h, v5.4h}, [DST_R], #16
    fetch_mask_pixblock
    fetch_src_pixblock
    pixman_composite_over_8888_8_0565_process_pixblock_head
#endif
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
    mov         v11.s[0], w4
    dup         v8.8b, v11.b[0]
    dup         v9.8b, v11.b[1]
    dup         v10.8b, v11.b[2]
    dup         v11.8b, v11.b[3]
.endm

.macro pixman_composite_over_n_8_0565_cleanup
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
    pixman_composite_over_8888_8_0565_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    24  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_over_8888_n_0565_init
    mov         v24.s[0], w6
    dup         v24.8b, v24.b[3]
.endm

.macro pixman_composite_over_8888_n_0565_cleanup
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
    st1     {v0.4h, v1.4h, v2.4h, v3.4h}, [DST_W], #32
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
    st1         {v0.8b, v1.8b, v2.8b, v3.8b}, [DST_W], 32
.endm

.macro pixman_composite_src_n_8_init
    mov         v0.s[0], w4
    dup         v3.8b, v0.b[0]
    dup         v2.8b, v0.b[0]
    dup         v1.8b, v0.b[0]
    dup         v0.8b, v0.b[0]
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
    st1     {v0.4h, v1.4h, v2.4h, v3.4h}, [DST_W], #32
.endm

.macro pixman_composite_src_n_0565_init
    mov         v0.s[0], w4
    dup         v3.4h, v0.h[0]
    dup         v2.4h, v0.h[0]
    dup         v1.4h, v0.h[0]
    dup         v0.4h, v0.h[0]
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
    st1         {v0.2s, v1.2s, v2.2s, v3.2s}, [DST_W], #32
.endm

.macro pixman_composite_src_n_8888_init
    mov         v0.s[0], w4
    dup         v3.2s, v0.s[0]
    dup         v2.2s, v0.s[0]
    dup         v1.2s, v0.s[0]
    dup         v0.2s, v0.s[0]
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
    st1  {v0.2s, v1.2s, v2.2s, v3.2s}, [DST_W], #32
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
    orr      v0.8b, v0.8b, v4.8b
    orr      v1.8b, v1.8b, v4.8b
    orr      v2.8b, v2.8b, v4.8b
    orr      v3.8b, v3.8b, v4.8b
.endm

.macro pixman_composite_src_x888_8888_process_pixblock_tail
.endm

.macro pixman_composite_src_x888_8888_process_pixblock_tail_head
    st1      {v0.2s, v1.2s, v2.2s, v3.2s}, [DST_W], #32
    fetch_src_pixblock
    orr      v0.8b, v0.8b, v4.8b
    orr      v1.8b, v1.8b, v4.8b
    orr      v2.8b, v2.8b, v4.8b
    orr      v3.8b, v3.8b, v4.8b
    cache_preload 8, 8
.endm

.macro pixman_composite_src_x888_8888_init
    movi    v4.2s, #0xff, lsl 24
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
    /* expecting solid source in {v0, v1, v2, v3} */
    /* mask is in v24 (v25, v26, v27 are unused) */

    /* in */
    umull       v8.8h,  v24.8b, v0.8b
    umull       v9.8h,  v24.8b, v1.8b
    umull       v10.8h, v24.8b, v2.8b
    umull       v11.8h, v24.8b, v3.8b
    ursra       v8.8h,  v8.8h, #8
    ursra       v9.8h,  v9.8h, #8
    ursra       v10.8h, v10.8h, #8
    ursra       v11.8h, v11.8h, #8
.endm

.macro pixman_composite_src_n_8_8888_process_pixblock_tail
    rshrn       v28.8b, v8.8h, #8
    rshrn       v29.8b, v9.8h, #8
    rshrn       v30.8b, v10.8h, #8
    rshrn       v31.8b, v11.8h, #8
.endm

.macro pixman_composite_src_n_8_8888_process_pixblock_tail_head
    fetch_mask_pixblock
                                    PF add PF_X, PF_X, #8
        rshrn       v28.8b, v8.8h, #8
                                    PF tst PF_CTL, #0x0F
        rshrn       v29.8b, v9.8h, #8
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
10:
        rshrn      v30.8b, v10.8h, #8
                                    PF beq 10f
                                    PF sub PF_CTL, PF_CTL, #1
10:
        rshrn      v31.8b, v11.8h, #8
                                    PF cmp PF_X, ORIG_W
    umull          v8.8h, v24.8b, v0.8b
                                    PF lsl DUMMY, PF_X, #mask_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_MASK, DUMMY]
    umull          v9.8h, v24.8b, v1.8b
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
10:
    umull          v10.8h, v24.8b, v2.8b
                                    PF ble 10f
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    umull          v11.8h, v24.8b, v3.8b
                                    PF ble 10f
                                    PF lsl DUMMY, MASK_STRIDE, #mask_bpp_shift
                                    PF ldrsb DUMMY, [PF_MASK, DUMMY]
                                    PF add PF_MASK, PF_MASK, #1
10:
        st4        {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
    ursra       v8.8h, v8.8h, #8
    ursra       v9.8h, v9.8h, #8
    ursra       v10.8h, v10.8h, #8
    ursra       v11.8h, v11.8h, #8
.endm

.macro pixman_composite_src_n_8_8888_init
    mov         v3.s[0], w4
    dup         v0.8b, v3.b[0]
    dup         v1.8b, v3.b[1]
    dup         v2.8b, v3.b[2]
    dup         v3.8b, v3.b[3]
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
    umull       v0.8h, v24.8b, v16.8b
    umull       v1.8h, v25.8b, v16.8b
    umull       v2.8h, v26.8b, v16.8b
    umull       v3.8h, v27.8b, v16.8b
    ursra       v0.8h, v0.8h,  #8
    ursra       v1.8h, v1.8h,  #8
    ursra       v2.8h, v2.8h,  #8
    ursra       v3.8h, v3.8h,  #8
.endm

.macro pixman_composite_src_n_8_8_process_pixblock_tail
    rshrn       v28.8b, v0.8h, #8
    rshrn       v29.8b, v1.8h, #8
    rshrn       v30.8b, v2.8h, #8
    rshrn       v31.8b, v3.8h, #8
.endm

.macro pixman_composite_src_n_8_8_process_pixblock_tail_head
    fetch_mask_pixblock
                                    PF add PF_X, PF_X, #8
        rshrn       v28.8b, v0.8h, #8
                                    PF tst PF_CTL, #0x0F
        rshrn       v29.8b, v1.8h, #8
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
10:
        rshrn       v30.8b, v2.8h, #8
                                    PF beq 10f
                                    PF sub PF_CTL, PF_CTL, #1
10:
        rshrn       v31.8b, v3.8h, #8
                                    PF cmp PF_X, ORIG_W
    umull       v0.8h,  v24.8b, v16.8b
                                    PF lsl DUMMY, PF_X, mask_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_MASK, DUMMY]
    umull       v1.8h,  v25.8b, v16.8b
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
10:
    umull       v2.8h,  v26.8b, v16.8b
                                    PF ble 10f
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    umull       v3.8h,  v27.8b, v16.8b
                                    PF ble 10f
                                    PF lsl DUMMY, MASK_STRIDE, #mask_bpp_shift
                                    PF ldrsb DUMMY, [PF_MASK, DUMMY]
                                    PF add PF_MASK, PF_MASK, #1
10:
        st1         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
    ursra       v0.8h, v0.8h,  #8
    ursra       v1.8h, v1.8h,  #8
    ursra       v2.8h, v2.8h,  #8
    ursra       v3.8h, v3.8h,  #8
.endm

.macro pixman_composite_src_n_8_8_init
    mov         v16.s[0], w4
    dup         v16.8b, v16.b[3]
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
    /* expecting deinterleaved source data in {v8, v9, v10, v11} */
    /* v8 - blue, v9 - green, v10 - red, v11 - alpha */
    /* and destination data in {v4, v5, v6, v7} */
    /* mask is in v24 (v25, v26, v27 are unused) */

    /* in */
    umull       v12.8h, v24.8b, v8.8b
    umull       v13.8h, v24.8b, v9.8b
    umull       v14.8h, v24.8b, v10.8b
    umull       v15.8h, v24.8b, v11.8b
    urshr       v16.8h, v12.8h, #8
    urshr       v17.8h, v13.8h, #8
    urshr       v18.8h, v14.8h, #8
    urshr       v19.8h, v15.8h, #8
    raddhn      v0.8b, v12.8h, v16.8h
    raddhn      v1.8b, v13.8h, v17.8h
    raddhn      v2.8b, v14.8h, v18.8h
    raddhn      v3.8b, v15.8h, v19.8h
    mvn         v25.8b, v3.8b  /* get inverted alpha */
    /* source:      v0 - blue, v1 - green, v2 - red, v3 - alpha */
    /* destination: v4 - blue, v5 - green, v6 - red, v7 - alpha */
    /* now do alpha blending */
    umull       v12.8h, v25.8b, v4.8b
    umull       v13.8h, v25.8b, v5.8b
    umull       v14.8h, v25.8b, v6.8b
    umull       v15.8h, v25.8b, v7.8b
.endm

.macro pixman_composite_over_n_8_8888_process_pixblock_tail
    urshr       v16.8h, v12.8h, #8
    urshr       v17.8h, v13.8h, #8
    urshr       v18.8h, v14.8h, #8
    urshr       v19.8h, v15.8h, #8
    raddhn      v28.8b, v16.8h, v12.8h
    raddhn      v29.8b, v17.8h, v13.8h
    raddhn      v30.8b, v18.8h, v14.8h
    raddhn      v31.8b, v19.8h, v15.8h
    uqadd       v28.8b, v0.8b, v28.8b
    uqadd       v29.8b, v1.8b, v29.8b
    uqadd       v30.8b, v2.8b, v30.8b
    uqadd       v31.8b, v3.8b, v31.8b
.endm

.macro pixman_composite_over_n_8_8888_process_pixblock_tail_head
        urshr       v16.8h, v12.8h, #8
     ld4        {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
        urshr       v17.8h, v13.8h, #8
    fetch_mask_pixblock
        urshr       v18.8h, v14.8h, #8
                                    PF add PF_X, PF_X, #8
        urshr       v19.8h, v15.8h, #8
                                    PF tst PF_CTL, #0x0F
        raddhn      v28.8b, v16.8h, v12.8h
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
10:
        raddhn      v29.8b, v17.8h, v13.8h
                                    PF beq 10f
                                    PF sub PF_CTL, PF_CTL, #1
10:
        raddhn      v30.8b, v18.8h, v14.8h
                                    PF cmp PF_X, ORIG_W
        raddhn      v31.8b, v19.8h, v15.8h
                                    PF lsl DUMMY, PF_X, #dst_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_DST, DUMMY]
    umull       v16.8h, v24.8b, v8.8b
                                    PF lsl DUMMY, PF_X, #mask_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_MASK, DUMMY]
    umull       v17.8h, v24.8b, v9.8b
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
10:
    umull       v18.8h, v24.8b, v10.8b
                                    PF ble 10f
                                    PF subs PF_CTL, PF_CTL, #0x10
10:
    umull       v19.8h, v24.8b, v11.8b
                                    PF ble 10f
                                    PF lsl DUMMY, DST_STRIDE, #dst_bpp_shift
                                    PF ldrsb DUMMY, [PF_DST, DUMMY]
                                    PF add PF_DST, PF_DST, #1
10:
        uqadd       v28.8b, v0.8b, v28.8b
                                    PF ble 10f
                                    PF lsl DUMMY, MASK_STRIDE, #mask_bpp_shift
                                    PF ldrsb DUMMY, [PF_MASK, DUMMY]
                                    PF add PF_MASK, PF_MASK, #1
10:
        uqadd        v29.8b, v1.8b, v29.8b
        uqadd        v30.8b, v2.8b, v30.8b
        uqadd        v31.8b, v3.8b, v31.8b
    urshr       v12.8h, v16.8h, #8
    urshr       v13.8h, v17.8h, #8
    urshr       v14.8h, v18.8h, #8
    urshr       v15.8h, v19.8h, #8
    raddhn      v0.8b, v16.8h, v12.8h
    raddhn      v1.8b, v17.8h, v13.8h
    raddhn      v2.8b, v18.8h, v14.8h
    raddhn      v3.8b, v19.8h, v15.8h
        st4          {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
    mvn         v25.8b, v3.8b
    umull       v12.8h, v25.8b, v4.8b
    umull       v13.8h, v25.8b, v5.8b
    umull       v14.8h, v25.8b, v6.8b
    umull       v15.8h, v25.8b, v7.8b
.endm

.macro pixman_composite_over_n_8_8888_init
    mov         v11.s[0], w4
    dup         v8.8b, v11.b[0]
    dup         v9.8b, v11.b[1]
    dup         v10.8b, v11.b[2]
    dup         v11.8b, v11.b[3]
.endm

.macro pixman_composite_over_n_8_8888_cleanup
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
    umull       v0.8h,  v24.8b, v8.8b
    umull       v1.8h,  v25.8b, v8.8b
    umull       v2.8h,  v26.8b, v8.8b
    umull       v3.8h,  v27.8b, v8.8b
    urshr       v10.8h, v0.8h,  #8
    urshr       v11.8h, v1.8h,  #8
    urshr       v12.8h, v2.8h,  #8
    urshr       v13.8h, v3.8h,  #8
    raddhn      v0.8b,  v0.8h,  v10.8h
    raddhn      v1.8b,  v1.8h,  v11.8h
    raddhn      v2.8b,  v2.8h,  v12.8h
    raddhn      v3.8b,  v3.8h,  v13.8h
    mvn         v24.8b, v0.8b
    mvn         v25.8b, v1.8b
    mvn         v26.8b, v2.8b
    mvn         v27.8b, v3.8b
    umull       v10.8h, v24.8b, v4.8b
    umull       v11.8h, v25.8b, v5.8b
    umull       v12.8h, v26.8b, v6.8b
    umull       v13.8h, v27.8b, v7.8b
.endm

.macro pixman_composite_over_n_8_8_process_pixblock_tail
    urshr       v14.8h, v10.8h,  #8
    urshr       v15.8h, v11.8h,  #8
    urshr       v16.8h, v12.8h, #8
    urshr       v17.8h, v13.8h, #8
    raddhn      v28.8b, v14.8h, v10.8h
    raddhn      v29.8b, v15.8h, v11.8h
    raddhn      v30.8b, v16.8h, v12.8h
    raddhn      v31.8b, v17.8h, v13.8h
    uqadd       v28.8b, v0.8b,  v28.8b
    uqadd       v29.8b, v1.8b,  v29.8b
    uqadd       v30.8b, v2.8b,  v30.8b
    uqadd       v31.8b, v3.8b,  v31.8b
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_n_8_8_process_pixblock_tail_head
    ld1         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    pixman_composite_over_n_8_8_process_pixblock_tail
    fetch_mask_pixblock
    cache_preload 32, 32
    st1         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
    pixman_composite_over_n_8_8_process_pixblock_head
.endm

.macro pixman_composite_over_n_8_8_init
    mov         v8.s[0], w4
    dup         v8.8b, v8.b[3]
.endm

.macro pixman_composite_over_n_8_8_cleanup
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
     * input:  solid src (n) in {v8,  v9,  v10, v11}
     *         dest in          {v4,  v5,  v6,  v7 }
     *         mask in          {v24, v25, v26, v27}
     * output: updated src in   {v0,  v1,  v2,  v3 }
     *         updated mask in  {v24, v25, v26, v3 }
     */
    umull       v0.8h,  v24.8b, v8.8b
    umull       v1.8h,  v25.8b, v9.8b
    umull       v2.8h,  v26.8b, v10.8b
    umull       v3.8h,  v27.8b, v11.8b
    umull       v12.8h, v11.8b, v25.8b
    umull       v13.8h, v11.8b, v24.8b
    umull       v14.8h, v11.8b, v26.8b
    urshr       v15.8h, v0.8h,  #8
    urshr       v16.8h, v1.8h,  #8
    urshr       v17.8h, v2.8h,  #8
    raddhn      v0.8b,  v0.8h,  v15.8h
    raddhn      v1.8b,  v1.8h,  v16.8h
    raddhn      v2.8b,  v2.8h,  v17.8h
    urshr       v15.8h, v13.8h, #8
    urshr       v16.8h, v12.8h, #8
    urshr       v17.8h, v14.8h, #8
    urshr       v18.8h, v3.8h,  #8
    raddhn      v24.8b, v13.8h, v15.8h
    raddhn      v25.8b, v12.8h, v16.8h
    raddhn      v26.8b, v14.8h, v17.8h
    raddhn      v3.8b,  v3.8h,  v18.8h
    /*
     * 'combine_over_ca' replacement
     *
     * output: updated dest in {v28, v29, v30, v31}
     */
    mvn         v24.8b, v24.8b
    mvn         v25.8b, v25.8b
    mvn         v26.8b, v26.8b
    mvn         v27.8b, v3.8b
    umull       v12.8h, v24.8b, v4.8b
    umull       v13.8h, v25.8b, v5.8b
    umull       v14.8h, v26.8b, v6.8b
    umull       v15.8h, v27.8b, v7.8b
.endm

.macro pixman_composite_over_n_8888_8888_ca_process_pixblock_tail
    /* ... continue 'combine_over_ca' replacement */
    urshr       v16.8h, v12.8h, #8
    urshr       v17.8h, v13.8h, #8
    urshr       v18.8h, v14.8h, #8
    urshr       v19.8h, v15.8h, #8
    raddhn      v28.8b, v16.8h, v12.8h
    raddhn      v29.8b, v17.8h, v13.8h
    raddhn      v30.8b, v18.8h, v14.8h
    raddhn      v31.8b, v19.8h, v15.8h
    uqadd       v28.8b, v0.8b,  v28.8b
    uqadd       v29.8b, v1.8b,  v29.8b
    uqadd       v30.8b, v2.8b,  v30.8b
    uqadd       v31.8b, v3.8b,  v31.8b
.endm

.macro pixman_composite_over_n_8888_8888_ca_process_pixblock_tail_head
        urshr       v16.8h, v12.8h, #8
        urshr       v17.8h, v13.8h, #8
    ld4         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
        urshr       v18.8h, v14.8h, #8
        urshr       v19.8h, v15.8h, #8
        raddhn      v28.8b, v16.8h, v12.8h
        raddhn      v29.8b, v17.8h, v13.8h
        raddhn      v30.8b, v18.8h, v14.8h
        raddhn      v31.8b, v19.8h, v15.8h
    fetch_mask_pixblock
        uqadd       v28.8b, v0.8b, v28.8b
        uqadd       v29.8b, v1.8b, v29.8b
        uqadd       v30.8b, v2.8b, v30.8b
        uqadd       v31.8b, v3.8b, v31.8b
    cache_preload 8, 8
    pixman_composite_over_n_8888_8888_ca_process_pixblock_head
    st4         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
.endm

.macro pixman_composite_over_n_8888_8888_ca_init
    mov         v13.s[0], w4
    dup         v8.8b, v13.b[0]
    dup         v9.8b, v13.b[1]
    dup         v10.8b, v13.b[2]
    dup         v11.8b, v13.b[3]
.endm

.macro pixman_composite_over_n_8888_8888_ca_cleanup
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
     * input:  solid src (n) in {v8,  v9,  v10, v11}  [B, G, R, A]
     *         mask in          {v24, v25, v26}       [B, G, R]
     * output: updated src in   {v0,  v1,  v2 }       [B, G, R]
     *         updated mask in  {v24, v25, v26}       [B, G, R]
     */
    umull       v0.8h,  v24.8b, v8.8b
    umull       v1.8h,  v25.8b, v9.8b
    umull       v2.8h,  v26.8b, v10.8b
    umull       v12.8h, v11.8b, v24.8b
    umull       v13.8h, v11.8b, v25.8b
    umull       v14.8h, v11.8b, v26.8b
    urshr       v15.8h, v0.8h,  #8
    urshr       v16.8h, v1.8h,  #8
    urshr       v17.8h, v2.8h,  #8
    raddhn      v0.8b,  v0.8h,  v15.8h
    raddhn      v1.8b,  v1.8h,  v16.8h
    raddhn      v2.8b,  v2.8h,  v17.8h
    urshr       v19.8h, v12.8h, #8
    urshr       v20.8h, v13.8h, #8
    urshr       v21.8h, v14.8h, #8
    raddhn      v24.8b, v12.8h, v19.8h
    raddhn      v25.8b, v13.8h, v20.8h
    /*
     * convert 8 r5g6b5 pixel data from {v4} to planar 8-bit format
     * and put data into v16 - blue, v17 - green, v18 - red
     */
       mov         v4.d[1], v5.d[0]
       shrn        v17.8b, v4.8h,  #3
       shrn        v18.8b, v4.8h,  #8
    raddhn      v26.8b, v14.8h, v21.8h
       sli         v4.8h,  v4.8h,  #5
       sri         v18.8b, v18.8b, #5
       sri         v17.8b, v17.8b, #6
    /*
     * 'combine_over_ca' replacement
     *
     * output: updated dest in v16 - blue, v17 - green, v18 - red
     */
    mvn         v24.8b, v24.8b
    mvn         v25.8b, v25.8b
       shrn       v16.8b, v4.8h,  #2
    mvn         v26.8b, v26.8b
    umull       v5.8h, v16.8b, v24.8b
    umull       v6.8h, v17.8b, v25.8b
    umull       v7.8h, v18.8b, v26.8b
.endm

.macro pixman_composite_over_n_8888_0565_ca_process_pixblock_tail
    /* ... continue 'combine_over_ca' replacement */
    urshr       v13.8h, v5.8h, #8
    urshr       v14.8h, v6.8h, #8
    urshr       v15.8h, v7.8h, #8
    raddhn      v16.8b, v13.8h, v5.8h
    raddhn      v17.8b, v14.8h, v6.8h
    raddhn      v18.8b, v15.8h, v7.8h
    uqadd       v16.8b, v0.8b, v16.8b
    uqadd       v17.8b, v1.8b, v17.8b
    uqadd       v18.8b, v2.8b, v18.8b
    /*
     * convert the results in v16, v17, v18 to r5g6b5 and store
     * them into {v14}
     */
    ushll       v14.8h, v18.8b, #7
    sli         v14.8h, v14.8h, #1
    ushll       v12.8h, v17.8b, #7
    sli         v12.8h, v12.8h, #1
    ushll       v13.8h, v16.8b, #7
    sli         v13.8h, v13.8h, #1
    sri         v14.8h, v12.8h, #5
    sri         v14.8h, v13.8h, #11
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
.endm

.macro pixman_composite_over_n_8888_0565_ca_process_pixblock_tail_head
    fetch_mask_pixblock
        urshr       v13.8h, v5.8h, #8
        urshr       v14.8h, v6.8h, #8
    ld1         {v4.8h}, [DST_R], #16
        urshr       v15.8h, v7.8h, #8
        raddhn      v16.8b, v13.8h, v5.8h
        raddhn      v17.8b, v14.8h, v6.8h
        raddhn      v18.8b, v15.8h, v7.8h
    mov         v5.d[0], v4.d[1]
            /* process_pixblock_head */
            /*
             * 'combine_mask_ca' replacement
             *
             * input:  solid src (n) in {v8,  v9,  v10, v11}  [B, G, R, A]
             *         mask in          {v24, v25, v26}       [B, G, R]
             * output: updated src in   {v0,  v1,  v2 }       [B, G, R]
             *         updated mask in  {v24, v25, v26}       [B, G, R]
             */
        uqadd       v16.8b, v0.8b, v16.8b
        uqadd       v17.8b, v1.8b, v17.8b
        uqadd       v18.8b, v2.8b, v18.8b
            umull       v0.8h,  v24.8b, v8.8b
            umull       v1.8h,  v25.8b, v9.8b
            umull       v2.8h,  v26.8b, v10.8b
        /*
         * convert the result in v16, v17, v18 to r5g6b5 and store
         * it into {v14}
         */
        ushll       v14.8h, v18.8b, #7
        sli         v14.8h, v14.8h, #1
        ushll       v18.8h, v16.8b, #7
        sli         v18.8h, v18.8h, #1
        ushll       v19.8h, v17.8b, #7
        sli         v19.8h, v19.8h, #1
            umull       v12.8h, v11.8b, v24.8b
        sri         v14.8h, v19.8h, #5
            umull       v13.8h, v11.8b, v25.8b
            umull       v15.8h, v11.8b, v26.8b
        sri         v14.8h, v18.8h, #11
        mov         v28.d[0], v14.d[0]
        mov         v29.d[0], v14.d[1]
    cache_preload 8, 8
            urshr       v16.8h, v0.8h,  #8
            urshr       v17.8h, v1.8h,  #8
            urshr       v18.8h, v2.8h,  #8
            raddhn      v0.8b,  v0.8h,  v16.8h
            raddhn      v1.8b,  v1.8h,  v17.8h
            raddhn      v2.8b,  v2.8h,  v18.8h
            urshr       v19.8h, v12.8h, #8
            urshr       v20.8h, v13.8h, #8
            urshr       v21.8h, v15.8h, #8
            raddhn      v24.8b, v12.8h, v19.8h
            raddhn      v25.8b, v13.8h, v20.8h
                /*
                 * convert 8 r5g6b5 pixel data from {v4, v5} to planar
             * 8-bit format and put data into v16 - blue, v17 - green,
             * v18 - red
                 */
		mov         v4.d[1], v5.d[0]
                shrn        v17.8b, v4.8h,  #3
                shrn        v18.8b, v4.8h,  #8
            raddhn      v26.8b, v15.8h, v21.8h
                sli         v4.8h,  v4.8h,  #5
                sri         v17.8b, v17.8b, #6
                sri         v18.8b, v18.8b, #5
            /*
             * 'combine_over_ca' replacement
             *
             * output: updated dest in v16 - blue, v17 - green, v18 - red
             */
            mvn         v24.8b, v24.8b
            mvn         v25.8b, v25.8b
                shrn        v16.8b, v4.8h,  #2
            mvn         v26.8b, v26.8b
            umull       v5.8h, v16.8b, v24.8b
            umull       v6.8h, v17.8b, v25.8b
            umull       v7.8h, v18.8b, v26.8b
    st1         {v14.8h}, [DST_W], #16
.endm

.macro pixman_composite_over_n_8888_0565_ca_init
    mov         v13.s[0], w4
    dup         v8.8b, v13.b[0]
    dup         v9.8b, v13.b[1]
    dup         v10.8b, v13.b[2]
    dup         v11.8b, v13.b[3]
.endm

.macro pixman_composite_over_n_8888_0565_ca_cleanup
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
    /* expecting source data in {v0, v1, v2, v3} */
    /* and destination data in {v4, v5, v6, v7} */
    umull       v8.8h,  v4.8b,  v3.8b
    umull       v9.8h,  v5.8b,  v3.8b
    umull       v10.8h, v6.8b,  v3.8b
    umull       v11.8h, v7.8b,  v3.8b
.endm

.macro pixman_composite_in_n_8_process_pixblock_tail
    urshr       v14.8h, v8.8h,  #8
    urshr       v15.8h, v9.8h,  #8
    urshr       v12.8h, v10.8h, #8
    urshr       v13.8h, v11.8h, #8
    raddhn      v28.8b, v8.8h,  v14.8h
    raddhn      v29.8b, v9.8h,  v15.8h
    raddhn      v30.8b, v10.8h, v12.8h
    raddhn      v31.8b, v11.8h, v13.8h
.endm

.macro pixman_composite_in_n_8_process_pixblock_tail_head
    pixman_composite_in_n_8_process_pixblock_tail
    ld1         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    cache_preload 32, 32
    pixman_composite_in_n_8_process_pixblock_head
    st1         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
.endm

.macro pixman_composite_in_n_8_init
    mov         v3.s[0], w4
    dup         v3.8b, v3.b[3]
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
    /* expecting source data in {v8, v9, v10, v11} */
    /* v8 - blue, v9 - green, v10 - red, v11 - alpha */
    /* and destination data in {v4, v5, v6, v7} */
    /* mask is in v24, v25, v26, v27 */
    umull       v0.8h, v24.8b, v11.8b
    umull       v1.8h, v25.8b, v11.8b
    umull       v2.8h, v26.8b, v11.8b
    umull       v3.8h, v27.8b, v11.8b
    urshr       v12.8h, v0.8h, #8
    urshr       v13.8h, v1.8h, #8
    urshr       v14.8h, v2.8h, #8
    urshr       v15.8h, v3.8h, #8
    raddhn      v0.8b, v0.8h, v12.8h
    raddhn      v1.8b, v1.8h, v13.8h
    raddhn      v2.8b, v2.8h, v14.8h
    raddhn      v3.8b, v3.8h, v15.8h
    uqadd       v28.8b, v0.8b, v4.8b
    uqadd       v29.8b, v1.8b, v5.8b
    uqadd       v30.8b, v2.8b, v6.8b
    uqadd       v31.8b, v3.8b, v7.8b
.endm

.macro pixman_composite_add_n_8_8_process_pixblock_tail
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_add_n_8_8_process_pixblock_tail_head
    pixman_composite_add_n_8_8_process_pixblock_tail
    st1         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
    ld1         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    fetch_mask_pixblock
    cache_preload 32, 32
    pixman_composite_add_n_8_8_process_pixblock_head
.endm

.macro pixman_composite_add_n_8_8_init
    mov         v11.s[0], w4
    dup         v11.8b, v11.b[3]
.endm

.macro pixman_composite_add_n_8_8_cleanup
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
    /* expecting source data in {v0, v1, v2, v3} */
    /* destination data in {v4, v5, v6, v7} */
    /* mask in {v24, v25, v26, v27} */
    umull       v8.8h, v24.8b, v0.8b
    umull       v9.8h, v25.8b, v1.8b
    umull       v10.8h, v26.8b, v2.8b
    umull       v11.8h, v27.8b, v3.8b
    urshr       v0.8h, v8.8h, #8
    urshr       v1.8h, v9.8h, #8
    urshr       v12.8h, v10.8h, #8
    urshr       v13.8h, v11.8h, #8
    raddhn      v0.8b, v0.8h, v8.8h
    raddhn      v1.8b, v1.8h, v9.8h
    raddhn      v2.8b, v12.8h, v10.8h
    raddhn      v3.8b, v13.8h, v11.8h
    uqadd       v28.8b, v0.8b, v4.8b
    uqadd       v29.8b, v1.8b, v5.8b
    uqadd       v30.8b, v2.8b, v6.8b
    uqadd       v31.8b, v3.8b, v7.8b
.endm

.macro pixman_composite_add_8_8_8_process_pixblock_tail
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_add_8_8_8_process_pixblock_tail_head
    pixman_composite_add_8_8_8_process_pixblock_tail
    st1         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
    ld1         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
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
    /* expecting source data in {v0, v1, v2, v3} */
    /* destination data in {v4, v5, v6, v7} */
    /* mask in {v24, v25, v26, v27} */
    umull       v8.8h,  v27.8b, v0.8b
    umull       v9.8h,  v27.8b, v1.8b
    umull       v10.8h, v27.8b, v2.8b
    umull       v11.8h, v27.8b, v3.8b
    /* 1 cycle bubble */
    ursra       v8.8h,  v8.8h,  #8
    ursra       v9.8h,  v9.8h,  #8
    ursra       v10.8h, v10.8h, #8
    ursra       v11.8h, v11.8h, #8
.endm

.macro pixman_composite_add_8888_8888_8888_process_pixblock_tail
    /* 2 cycle bubble */
    rshrn       v28.8b, v8.8h,  #8
    rshrn       v29.8b, v9.8h,  #8
    rshrn       v30.8b, v10.8h, #8
    rshrn       v31.8b, v11.8h, #8
    uqadd       v28.8b, v4.8b,  v28.8b
    uqadd       v29.8b, v5.8b,  v29.8b
    uqadd       v30.8b, v6.8b,  v30.8b
    uqadd       v31.8b, v7.8b,  v31.8b
.endm

.macro pixman_composite_add_8888_8888_8888_process_pixblock_tail_head
    fetch_src_pixblock
        rshrn       v28.8b, v8.8h,  #8
    fetch_mask_pixblock
        rshrn       v29.8b, v9.8h,  #8
    umull       v8.8h,  v27.8b, v0.8b
        rshrn       v30.8b, v10.8h, #8
    umull       v9.8h,  v27.8b, v1.8b
        rshrn       v31.8b, v11.8h, #8
    umull       v10.8h, v27.8b, v2.8b
    umull       v11.8h, v27.8b, v3.8b
        uqadd       v28.8b, v4.8b,  v28.8b
        uqadd       v29.8b, v5.8b,  v29.8b
        uqadd       v30.8b, v6.8b,  v30.8b
        uqadd       v31.8b, v7.8b,  v31.8b
    ursra       v8.8h,  v8.8h,  #8
    ld4         {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    ursra       v9.8h,  v9.8h,  #8
        st4         {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
    ursra       v10.8h, v10.8h, #8

    cache_preload 8, 8

    ursra       v11.8h, v11.8h, #8
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
    pixman_composite_add_8888_8888_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    24  /* mask_basereg  */

generate_composite_function_single_scanline \
    pixman_composite_scanline_add_mask_asm_neon, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_DEINTERLEAVE_32BPP, \
    8, /* number of pixels, processed in a single block */ \
    default_init, \
    default_cleanup, \
    pixman_composite_add_8888_8888_8888_process_pixblock_head, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail, \
    pixman_composite_add_8888_8888_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    24  /* mask_basereg  */

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
    mov         v3.s[0], w4
    dup         v0.8b, v3.b[0]
    dup         v1.8b, v3.b[1]
    dup         v2.8b, v3.b[2]
    dup         v3.8b, v3.b[3]
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
    mov         v27.s[0], w6
    dup         v27.8b, v27.b[3]
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
    /* expecting source data in {v0, v1, v2, v3} */
    /* destination data in {v4, v5, v6, v7} */
    /* solid mask is in v15 */

    /* 'in' */
    umull       v11.8h, v15.8b, v3.8b
    umull       v10.8h, v15.8b, v2.8b
    umull       v9.8h,  v15.8b, v1.8b
    umull       v8.8h,  v15.8b, v0.8b
    urshr       v16.8h, v11.8h, #8
    urshr       v14.8h, v10.8h, #8
    urshr       v13.8h,  v9.8h, #8
    urshr       v12.8h,  v8.8h, #8
    raddhn      v3.8b, v11.8h, v16.8h
    raddhn      v2.8b, v10.8h, v14.8h
    raddhn      v1.8b,  v9.8h, v13.8h
    raddhn      v0.8b,  v8.8h, v12.8h
    mvn         v24.8b, v3.8b  /* get inverted alpha */
    /* now do alpha blending */
    umull       v8.8h, v24.8b, v4.8b
    umull       v9.8h, v24.8b, v5.8b
    umull       v10.8h, v24.8b, v6.8b
    umull       v11.8h, v24.8b, v7.8b
.endm

.macro pixman_composite_out_reverse_8888_n_8888_process_pixblock_tail
    urshr       v16.8h, v8.8h, #8
    urshr       v17.8h, v9.8h, #8
    urshr       v18.8h, v10.8h, #8
    urshr       v19.8h, v11.8h, #8
    raddhn      v28.8b, v16.8h, v8.8h
    raddhn      v29.8b, v17.8h, v9.8h
    raddhn      v30.8b, v18.8h, v10.8h
    raddhn      v31.8b, v19.8h, v11.8h
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_out_reverse_8888_8888_8888_process_pixblock_tail_head
    ld4        {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    pixman_composite_out_reverse_8888_n_8888_process_pixblock_tail
    fetch_src_pixblock
    cache_preload 8, 8
    fetch_mask_pixblock
    pixman_composite_out_reverse_8888_n_8888_process_pixblock_head
    st4        {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
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
    uqadd       v28.8b, v0.8b, v28.8b
    uqadd       v29.8b, v1.8b, v29.8b
    uqadd       v30.8b, v2.8b, v30.8b
    uqadd       v31.8b, v3.8b, v31.8b
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_8888_n_8888_process_pixblock_tail_head
    ld4        {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    pixman_composite_over_8888_n_8888_process_pixblock_tail
    fetch_src_pixblock
    cache_preload 8, 8
    pixman_composite_over_8888_n_8888_process_pixblock_head
    st4        {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
.endm

.macro pixman_composite_over_8888_n_8888_init
    mov         v15.s[0], w6
    dup         v15.8b, v15.b[3]
.endm

.macro pixman_composite_over_8888_n_8888_cleanup
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
    pixman_composite_over_8888_n_8888_process_pixblock_tail_head, \
    28, /* dst_w_basereg */ \
    4,  /* dst_r_basereg */ \
    0,  /* src_basereg   */ \
    12  /* mask_basereg  */

/******************************************************************************/

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_8888_8888_8888_process_pixblock_tail_head
    ld4        {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    pixman_composite_over_8888_n_8888_process_pixblock_tail
    fetch_src_pixblock
    cache_preload 8, 8
    fetch_mask_pixblock
    pixman_composite_over_8888_n_8888_process_pixblock_head
    st4        {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
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
    ld4        {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    pixman_composite_over_8888_n_8888_process_pixblock_tail
    fetch_src_pixblock
    cache_preload 8, 8
    fetch_mask_pixblock
    pixman_composite_over_8888_n_8888_process_pixblock_head
    st4        {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
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
    st3     {v0.8b, v1.8b, v2.8b}, [DST_W], #24
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
    mov    v31.8b, v2.8b
    mov    v2.8b, v0.8b
    mov    v0.8b, v31.8b
.endm

.macro pixman_composite_src_0888_8888_rev_process_pixblock_tail
.endm

.macro pixman_composite_src_0888_8888_rev_process_pixblock_tail_head
    st4    {v0.8b, v1.8b, v2.8b, v3.8b}, [DST_W], #32
    fetch_src_pixblock
    mov    v31.8b, v2.8b
    mov    v2.8b, v0.8b
    mov    v0.8b, v31.8b
    cache_preload 8, 8
.endm

.macro pixman_composite_src_0888_8888_rev_init
    eor    v3.8b, v3.8b, v3.8b
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
    ushll       v8.8h, v1.8b, #7
    sli         v8.8h, v8.8h, #1
    ushll       v9.8h, v2.8b, #7
    sli         v9.8h, v9.8h, #1
.endm

.macro pixman_composite_src_0888_0565_rev_process_pixblock_tail
    ushll       v14.8h, v0.8b, #7
    sli         v14.8h, v14.8h, #1
    sri         v14.8h, v8.8h, #5
    sri         v14.8h, v9.8h, #11
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
.endm

.macro pixman_composite_src_0888_0565_rev_process_pixblock_tail_head
        ushll       v14.8h, v0.8b, #7
        sli         v14.8h, v14.8h, #1
    fetch_src_pixblock
        sri         v14.8h, v8.8h, #5
        sri         v14.8h, v9.8h, #11
        mov         v28.d[0], v14.d[0]
        mov         v29.d[0], v14.d[1]
    ushll       v8.8h, v1.8b, #7
    sli         v8.8h, v8.8h, #1
        st1     {v14.8h}, [DST_W], #16
    ushll       v9.8h, v2.8b, #7
    sli         v9.8h, v9.8h, #1
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
    umull       v8.8h, v3.8b, v0.8b
    umull       v9.8h, v3.8b, v1.8b
    umull       v10.8h, v3.8b, v2.8b
.endm

.macro pixman_composite_src_pixbuf_8888_process_pixblock_tail
    urshr       v11.8h, v8.8h, #8
    mov         v30.8b, v31.8b
    mov         v31.8b, v3.8b
    mov         v3.8b, v30.8b
    urshr       v12.8h, v9.8h, #8
    urshr       v13.8h, v10.8h, #8
    raddhn      v30.8b, v11.8h, v8.8h
    raddhn      v29.8b, v12.8h, v9.8h
    raddhn      v28.8b, v13.8h, v10.8h
.endm

.macro pixman_composite_src_pixbuf_8888_process_pixblock_tail_head
        urshr       v11.8h, v8.8h, #8
        mov         v30.8b, v31.8b
        mov         v31.8b, v3.8b
        mov         v3.8b, v31.8b
        urshr       v12.8h, v9.8h, #8
        urshr       v13.8h, v10.8h, #8
    fetch_src_pixblock
        raddhn      v30.8b, v11.8h, v8.8h
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
                                    PF sub PF_CTL, PF_CTL, #1
10:
        raddhn      v29.8b, v12.8h, v9.8h
        raddhn      v28.8b, v13.8h, v10.8h
    umull       v8.8h, v3.8b, v0.8b
    umull       v9.8h, v3.8b, v1.8b
    umull       v10.8h, v3.8b, v2.8b
         st4    {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
                                    PF cmp PF_X, ORIG_W
                                    PF lsl DUMMY, PF_X, src_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_SRC, DUMMY]
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
                                    PF subs PF_CTL, PF_CTL, #0x10
                                    PF ble 10f
                                    PF lsl DUMMY, SRC_STRIDE, #src_bpp_shift
                                    PF ldrsb DUMMY, [PF_SRC, DUMMY]
                                    PF add PF_SRC, PF_SRC, #1
10:
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
    umull       v8.8h, v3.8b, v0.8b
    umull       v9.8h, v3.8b, v1.8b
    umull       v10.8h, v3.8b, v2.8b
.endm

.macro pixman_composite_src_rpixbuf_8888_process_pixblock_tail
    urshr       v11.8h, v8.8h, #8
    mov         v30.8b, v31.8b
    mov         v31.8b, v3.8b
    mov         v3.8b, v30.8b
    urshr       v12.8h, v9.8h, #8
    urshr       v13.8h, v10.8h, #8
    raddhn      v28.8b, v11.8h, v8.8h
    raddhn      v29.8b, v12.8h, v9.8h
    raddhn      v30.8b, v13.8h, v10.8h
.endm

.macro pixman_composite_src_rpixbuf_8888_process_pixblock_tail_head
        urshr       v11.8h, v8.8h, #8
        mov         v30.8b, v31.8b
        mov         v31.8b, v3.8b
        mov         v3.8b, v30.8b
        urshr       v12.8h, v9.8h, #8
        urshr       v13.8h, v10.8h, #8
    fetch_src_pixblock
        raddhn      v28.8b, v11.8h, v8.8h
                                    PF add PF_X, PF_X, #8
                                    PF tst PF_CTL, #0xF
                                    PF beq 10f
                                    PF add PF_X, PF_X, #8
                                    PF sub PF_CTL, PF_CTL, #1
10:
        raddhn      v29.8b, v12.8h, v9.8h
        raddhn      v30.8b, v13.8h, v10.8h
    umull       v8.8h, v3.8b, v0.8b
    umull       v9.8h, v3.8b, v1.8b
    umull       v10.8h, v3.8b, v2.8b
         st4    {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
                                    PF cmp PF_X, ORIG_W
                                    PF lsl DUMMY, PF_X, src_bpp_shift
                                    PF prfm PREFETCH_MODE, [PF_SRC, DUMMY]
                                    PF ble 10f
                                    PF sub PF_X, PF_X, ORIG_W
                                    PF subs PF_CTL, PF_CTL, #0x10
                                    PF ble 10f
                                    PF lsl DUMMY, SRC_STRIDE, #src_bpp_shift
                                    PF ldrsb DUMMY, [PF_SRC, DUMMY]
                                    PF add PF_SRC, PF_SRC, #1
10:
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
    /* mask is in v15 */
    mov         v4.d[0], v8.d[0]
    mov         v4.d[1], v9.d[0]
    mov         v13.d[0], v10.d[0]
    mov         v13.d[1], v11.d[0]
    convert_0565_to_x888 v4, v2, v1, v0
    convert_0565_to_x888 v13, v6, v5, v4
    /* source pixel data is in      {v0, v1, v2, XX} */
    /* destination pixel data is in {v4, v5, v6, XX} */
    mvn         v7.8b,  v15.8b
    umull       v10.8h, v15.8b, v2.8b
    umull       v9.8h,  v15.8b, v1.8b
    umull       v8.8h,  v15.8b, v0.8b
    umull       v11.8h, v7.8b,  v4.8b
    umull       v12.8h, v7.8b,  v5.8b
    umull       v13.8h, v7.8b,  v6.8b
    urshr       v19.8h, v10.8h, #8
    urshr       v18.8h, v9.8h,  #8
    urshr       v17.8h, v8.8h,  #8
    raddhn      v2.8b,  v10.8h, v19.8h
    raddhn      v1.8b,  v9.8h,  v18.8h
    raddhn      v0.8b,  v8.8h,  v17.8h
.endm

.macro pixman_composite_over_0565_8_0565_process_pixblock_tail
    urshr       v17.8h, v11.8h,  #8
    urshr       v18.8h, v12.8h,  #8
    urshr       v19.8h, v13.8h,  #8
    raddhn      v28.8b, v17.8h, v11.8h
    raddhn      v29.8b, v18.8h, v12.8h
    raddhn      v30.8b, v19.8h, v13.8h
    uqadd       v0.8b,  v0.8b,  v28.8b
    uqadd       v1.8b,  v1.8b,  v29.8b
    uqadd       v2.8b,  v2.8b,  v30.8b
    /* 32bpp result is in {v0, v1, v2, XX} */
    convert_8888_to_0565 v2, v1, v0, v14, v30, v13
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_over_0565_8_0565_process_pixblock_tail_head
    fetch_mask_pixblock
    pixman_composite_over_0565_8_0565_process_pixblock_tail
    fetch_src_pixblock
    ld1        {v10.4h, v11.4h}, [DST_R], #16
    cache_preload 8, 8
    pixman_composite_over_0565_8_0565_process_pixblock_head
    st1        {v14.8h}, [DST_W], #16
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
    mov         v15.s[0], w6
    dup         v15.8b, v15.b[3]
.endm

.macro pixman_composite_over_0565_n_0565_cleanup
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
    10,  /* dst_r_basereg */ \
    8,  /* src_basereg   */ \
    15  /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_add_0565_8_0565_process_pixblock_head
    /* mask is in v15 */
    mov         v4.d[0], v8.d[0]
    mov         v4.d[1], v9.d[0]
    mov         v13.d[0], v10.d[0]
    mov         v13.d[1], v11.d[0]
    convert_0565_to_x888 v4,  v2, v1, v0
    convert_0565_to_x888 v13, v6, v5, v4
    /* source pixel data is in      {v0, v1, v2, XX} */
    /* destination pixel data is in {v4, v5, v6, XX} */
    umull       v9.8h,  v15.8b, v2.8b
    umull       v8.8h,  v15.8b, v1.8b
    umull       v7.8h,  v15.8b, v0.8b
    urshr       v12.8h, v9.8h,  #8
    urshr       v11.8h, v8.8h,  #8
    urshr       v10.8h, v7.8h,  #8
    raddhn      v2.8b,  v9.8h,  v12.8h
    raddhn      v1.8b,  v8.8h,  v11.8h
    raddhn      v0.8b,  v7.8h,  v10.8h
.endm

.macro pixman_composite_add_0565_8_0565_process_pixblock_tail
    uqadd       v0.8b,  v0.8b,  v4.8b
    uqadd       v1.8b,  v1.8b,  v5.8b
    uqadd       v2.8b,  v2.8b,  v6.8b
    /* 32bpp result is in {v0, v1, v2, XX} */
    convert_8888_to_0565 v2, v1, v0, v14, v30, v13
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_add_0565_8_0565_process_pixblock_tail_head
    fetch_mask_pixblock
    pixman_composite_add_0565_8_0565_process_pixblock_tail
    fetch_src_pixblock
    ld1        {v10.4h, v11.4h}, [DST_R], #16
    cache_preload 8, 8
    pixman_composite_add_0565_8_0565_process_pixblock_head
    st1        {v14.8h}, [DST_W], #16
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
    /* mask is in v15 */
    mov         v12.d[0], v10.d[0]
    mov         v12.d[1], v11.d[0]
    convert_0565_to_x888 v12, v6, v5, v4
    /* destination pixel data is in {v4, v5, v6, xx} */
    mvn         v24.8b, v15.8b /* get inverted alpha */
    /* now do alpha blending */
    umull       v8.8h,  v24.8b, v4.8b
    umull       v9.8h,  v24.8b, v5.8b
    umull       v10.8h, v24.8b, v6.8b
.endm

.macro pixman_composite_out_reverse_8_0565_process_pixblock_tail
    urshr       v11.8h, v8.8h, #8
    urshr       v12.8h, v9.8h, #8
    urshr       v13.8h, v10.8h, #8
    raddhn      v0.8b, v11.8h, v8.8h
    raddhn      v1.8b, v12.8h, v9.8h
    raddhn      v2.8b, v13.8h, v10.8h
    /* 32bpp result is in {v0, v1, v2, XX} */
    convert_8888_to_0565 v2, v1, v0, v14, v12, v3
    mov         v28.d[0], v14.d[0]
    mov         v29.d[0], v14.d[1]
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_out_reverse_8_0565_process_pixblock_tail_head
    fetch_src_pixblock
    pixman_composite_out_reverse_8_0565_process_pixblock_tail
    ld1        {v10.4h, v11.4h}, [DST_R], #16
    cache_preload 8, 8
    pixman_composite_out_reverse_8_0565_process_pixblock_head
    st1        {v14.8h}, [DST_W], #16
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
    10,  /* dst_r_basereg */ \
    15, /* src_basereg   */ \
    0   /* mask_basereg  */

/******************************************************************************/

.macro pixman_composite_out_reverse_8_8888_process_pixblock_head
    /* src is in v0 */
    /* destination pixel data is in {v4, v5, v6, v7} */
    mvn         v1.8b, v0.8b /* get inverted alpha */
    /* now do alpha blending */
    umull       v8.8h, v1.8b, v4.8b
    umull       v9.8h, v1.8b, v5.8b
    umull       v10.8h, v1.8b, v6.8b
    umull       v11.8h, v1.8b, v7.8b
.endm

.macro pixman_composite_out_reverse_8_8888_process_pixblock_tail
    urshr       v14.8h, v8.8h, #8
    urshr       v15.8h, v9.8h, #8
    urshr       v12.8h, v10.8h, #8
    urshr       v13.8h, v11.8h, #8
    raddhn      v28.8b, v14.8h, v8.8h
    raddhn      v29.8b, v15.8h, v9.8h
    raddhn      v30.8b, v12.8h, v10.8h
    raddhn      v31.8b, v13.8h, v11.8h
    /* 32bpp result is in {v28, v29, v30, v31} */
.endm

/* TODO: expand macros and do better instructions scheduling */
.macro pixman_composite_out_reverse_8_8888_process_pixblock_tail_head
    fetch_src_pixblock
    pixman_composite_out_reverse_8_8888_process_pixblock_tail
    ld4       {v4.8b, v5.8b, v6.8b, v7.8b}, [DST_R], #32
    cache_preload 8, 8
    pixman_composite_out_reverse_8_8888_process_pixblock_head
    st4       {v28.8b, v29.8b, v30.8b, v31.8b}, [DST_W], #32
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
    asr       TMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #2
    ld1       {&reg1&.2s}, [TMP1], STRIDE
    ld1       {&reg2&.2s}, [TMP1]
.endm

.macro bilinear_load_0565 reg1, reg2, tmp
    asr       TMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #1
    ld1       {&reg2&.s}[0], [TMP1], STRIDE
    ld1       {&reg2&.s}[1], [TMP1]
    convert_four_0565_to_x888_packed reg2, reg1, reg2, tmp
.endm

.macro bilinear_load_and_vertical_interpolate_two_8888 \
                    acc1, acc2, reg1, reg2, reg3, reg4, tmp1, tmp2

    bilinear_load_8888 reg1, reg2, tmp1
    umull     &acc1&.8h, &reg1&.8b, v28.8b
    umlal     &acc1&.8h, &reg2&.8b, v29.8b
    bilinear_load_8888 reg3, reg4, tmp2
    umull     &acc2&.8h, &reg3&.8b, v28.8b
    umlal     &acc2&.8h, &reg4&.8b, v29.8b
.endm

.macro bilinear_load_and_vertical_interpolate_four_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi

    bilinear_load_and_vertical_interpolate_two_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi
    bilinear_load_and_vertical_interpolate_two_8888 \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi
.endm

.macro vzip reg1, reg2
    umov      TMP4, v31.d[0]
    zip1      v31.8b, reg1, reg2
    zip2      reg2,   reg1, reg2
    mov       reg1,   v31.8b
    mov       v31.d[0], TMP4
.endm

.macro vuzp reg1, reg2
    umov      TMP4, v31.d[0]
    uzp1      v31.8b, reg1, reg2
    uzp2      reg2,   reg1, reg2
    mov       reg1,   v31.8b
    mov       v31.d[0], TMP4
.endm

.macro bilinear_load_and_vertical_interpolate_two_0565 \
                acc1, acc2, reg1, reg2, reg3, reg4, acc2lo, acc2hi
    asr       TMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #1
    asr       TMP2, X, #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, lsl #1
    ld1       {&acc2&.s}[0], [TMP1], STRIDE
    ld1       {&acc2&.s}[2], [TMP2], STRIDE
    ld1       {&acc2&.s}[1], [TMP1]
    ld1       {&acc2&.s}[3], [TMP2]
    convert_0565_to_x888 acc2, reg3, reg2, reg1
    vzip      &reg1&.8b, &reg3&.8b
    vzip      &reg2&.8b, &reg4&.8b
    vzip      &reg3&.8b, &reg4&.8b
    vzip      &reg1&.8b, &reg2&.8b
    umull     &acc1&.8h, &reg1&.8b, v28.8b
    umlal     &acc1&.8h, &reg2&.8b, v29.8b
    umull     &acc2&.8h, &reg3&.8b, v28.8b
    umlal     &acc2&.8h, &reg4&.8b, v29.8b
.endm

.macro bilinear_load_and_vertical_interpolate_four_0565 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi
    asr       TMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #1
    asr       TMP2, X, #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, lsl #1
    ld1       {&xacc2&.s}[0], [TMP1], STRIDE
    ld1       {&xacc2&.s}[2], [TMP2], STRIDE
    ld1       {&xacc2&.s}[1], [TMP1]
    ld1       {&xacc2&.s}[3], [TMP2]
    convert_0565_to_x888 xacc2, xreg3, xreg2, xreg1
    asr       TMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #1
    asr       TMP2, X, #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, lsl #1
    ld1       {&yacc2&.s}[0], [TMP1], STRIDE
    vzip      &xreg1&.8b, &xreg3&.8b
    ld1       {&yacc2&.s}[2], [TMP2], STRIDE
    vzip      &xreg2&.8b, &xreg4&.8b
    ld1       {&yacc2&.s}[1], [TMP1]
    vzip      &xreg3&.8b, &xreg4&.8b
    ld1       {&yacc2&.s}[3], [TMP2]
    vzip      &xreg1&.8b, &xreg2&.8b
    convert_0565_to_x888 yacc2, yreg3, yreg2, yreg1
    umull     &xacc1&.8h, &xreg1&.8b, v28.8b
    vzip      &yreg1&.8b, &yreg3&.8b
    umlal     &xacc1&.8h, &xreg2&.8b, v29.8b
    vzip      &yreg2&.8b, &yreg4&.8b
    umull     &xacc2&.8h, &xreg3&.8b, v28.8b
    vzip      &yreg3&.8b, &yreg4&.8b
    umlal     &xacc2&.8h, &xreg4&.8b, v29.8b
    vzip      &yreg1&.8b, &yreg2&.8b
    umull     &yacc1&.8h, &yreg1&.8b, v28.8b
    umlal     &yacc1&.8h, &yreg2&.8b, v29.8b
    umull     &yacc2&.8h, &yreg3&.8b, v28.8b
    umlal     &yacc2&.8h, &yreg4&.8b, v29.8b
.endm

.macro bilinear_store_8888 numpix, tmp1, tmp2
.if numpix == 4
    st1       {v0.2s, v1.2s}, [OUT], #16
.elseif numpix == 2
    st1       {v0.2s}, [OUT], #8
.elseif numpix == 1
    st1       {v0.s}[0], [OUT], #4
.else
    .error bilinear_store_8888 numpix is unsupported
.endif
.endm

.macro bilinear_store_0565 numpix, tmp1, tmp2
    vuzp      v0.8b, v1.8b
    vuzp      v2.8b, v3.8b
    vuzp      v1.8b, v3.8b
    vuzp      v0.8b, v2.8b
    convert_8888_to_0565 v2, v1, v0, v1, tmp1, tmp2
.if numpix == 4
    st1       {v1.4h}, [OUT], #8
.elseif numpix == 2
    st1       {v1.s}[0], [OUT], #4
.elseif numpix == 1
    st1       {v1.h}[0], [OUT], #2
.else
    .error bilinear_store_0565 numpix is unsupported
.endif
.endm

.macro bilinear_interpolate_last_pixel src_fmt, dst_fmt
    bilinear_load_&src_fmt v0, v1, v2
    umull     v2.8h, v0.8b, v28.8b
    umlal     v2.8h, v1.8b, v29.8b
    /* 5 cycles bubble */
    ushll     v0.4s, v2.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v0.4s, v2.4h, v15.h[0]
    umlal2    v0.4s, v2.8h, v15.h[0]
    /* 5 cycles bubble */
    shrn      v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    /* 3 cycles bubble */
    xtn       v0.8b, v0.8h
    /* 1 cycle bubble */
    bilinear_store_&dst_fmt 1, v3, v4
.endm

.macro bilinear_interpolate_two_pixels src_fmt, dst_fmt
    bilinear_load_and_vertical_interpolate_two_&src_fmt \
                v1, v11, v2, v3, v20, v21, v22, v23
    ushll     v0.4s, v1.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v0.4s, v1.4h, v15.h[0]
    umlal2    v0.4s, v1.8h, v15.h[0]
    ushll     v10.4s, v11.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v10.4s, v11.4h, v15.h[4]
    umlal2    v10.4s, v11.8h, v15.h[4]
    shrn      v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn2     v0.8h, v10.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    add       v12.8h, v12.8h, v13.8h
    xtn       v0.8b, v0.8h
    bilinear_store_&dst_fmt 2, v3, v4
.endm

.macro bilinear_interpolate_four_pixels src_fmt, dst_fmt
    bilinear_load_and_vertical_interpolate_four_&src_fmt \
                v1, v11, v14, v20, v16, v17, v22, v23 \
                v3, v9,  v24, v25, v26, v27, v18, v19
    prfm      PREFETCH_MODE, [TMP1, PF_OFFS]
    sub       TMP1, TMP1, STRIDE
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
    prfm      PREFETCH_MODE, [TMP2, PF_OFFS]
    umlsl     v8.4s, v9.4h, v15.h[4]
    umlal2    v8.4s, v9.8h, v15.h[4]
    add       v12.8h, v12.8h, v13.8h
    shrn      v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn2     v0.8h, v10.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn      v2.4h, v2.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn2     v2.8h, v8.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    xtn       v0.8b, v0.8h
    xtn       v1.8b, v2.8h
    add       v12.8h, v12.8h, v13.8h
    bilinear_store_&dst_fmt 4, v3, v4
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
    OUT       .req      x0
    TOP       .req      x1
    BOTTOM    .req      x2
    WT        .req      x3
    WB        .req      x4
    X         .req      x5
    UX        .req      x6
    WIDTH     .req      x7
    TMP1      .req      x8
    TMP2      .req      x9
    PF_OFFS   .req      x10
    TMP3      .req      x11
    TMP4      .req      x12
    STRIDE    .req      x13

    sxtw      x3, w3
    sxtw      x4, w4
    sxtw      x5, w5
    sxtw      x6, w6
    sxtw      x7, w7

    stp       x29, x30, [sp, -16]!
    mov       x29, sp
    sub       sp,  sp, 112  /* push all registers */
    sub       x29, x29, 64
    st1       {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], #32
    st1       {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], #32
    stp        x8,  x9, [x29, -80]
    stp       x10, x11, [x29, -96]
    stp       x12, x13, [x29, -112]

    mov       PF_OFFS, #prefetch_distance
    mul       PF_OFFS, PF_OFFS, UX

    subs      STRIDE, BOTTOM, TOP
    .unreq    BOTTOM

    cmp       WIDTH, #0
    ble       300f

    dup       v12.8h, w5
    dup       v13.8h, w6
    dup       v28.8b, w3
    dup       v29.8b, w4
    mov       v25.d[0], v12.d[1]
    mov       v26.d[0], v13.d[0]
    add       v25.4h, v25.4h, v26.4h
    mov       v12.d[1], v25.d[0]

    /* ensure good destination alignment  */
    cmp       WIDTH, #1
    blt       100f
    tst       OUT, #(1 << dst_bpp_shift)
    beq       100f
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    add       v12.8h, v12.8h, v13.8h
    bilinear_interpolate_last_pixel src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #1
100:
    add       v13.8h, v13.8h, v13.8h
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    add       v12.8h, v12.8h, v13.8h

    cmp       WIDTH, #2
    blt       100f
    tst       OUT, #(1 << (dst_bpp_shift + 1))
    beq       100f
    bilinear_interpolate_two_pixels src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #2
100:
.if ((flags) & BILINEAR_FLAG_UNROLL_8) != 0
/*********** 8 pixels per iteration *****************/
    cmp       WIDTH, #4
    blt       100f
    tst       OUT, #(1 << (dst_bpp_shift + 2))
    beq       100f
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #4
100:
    subs      WIDTH, WIDTH, #8
    blt       100f
    asr       PF_OFFS, PF_OFFS, #(16 - src_bpp_shift)
    bilinear_interpolate_eight_pixels_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #8
    blt       500f
1000:
    bilinear_interpolate_eight_pixels_tail_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #8
    bge       1000b
500:
    bilinear_interpolate_eight_pixels_tail src_fmt, dst_fmt
100:
    tst       WIDTH, #4
    beq       200f
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
200:
.else
/*********** 4 pixels per iteration *****************/
    subs      WIDTH, WIDTH, #4
    blt       100f
    asr       PF_OFFS, PF_OFFS, #(16 - src_bpp_shift)
    bilinear_interpolate_four_pixels_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #4
    blt       500f
1000:
    bilinear_interpolate_four_pixels_tail_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #4
    bge       1000b
500:
    bilinear_interpolate_four_pixels_tail src_fmt, dst_fmt
100:
/****************************************************/
.endif
    /* handle the remaining trailing pixels */
    tst       WIDTH, #2
    beq       200f
    bilinear_interpolate_two_pixels src_fmt, dst_fmt
200:
    tst       WIDTH, #1
    beq       300f
    bilinear_interpolate_last_pixel src_fmt, dst_fmt
300:
    sub       x29, x29, 64
    ld1       {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], #32
    ld1       {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], #32
    ldp        x8,  x9, [x29, -80]
    ldp       x10, x11, [x29, -96]
    ldp       x12, x13, [x29, -104]
    mov       sp, x29
    ldp       x29, x30, [sp], 16
    ret

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
    asr       TMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #2
    asr       TMP2, X, #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, lsl #2

    ld1       {v22.2s}, [TMP1], STRIDE
    ld1       {v23.2s}, [TMP1]
    asr       TMP3, X, #16
    add       X, X, UX
    add       TMP3, TOP, TMP3, lsl #2
    umull     v8.8h, v22.8b, v28.8b
    umlal     v8.8h, v23.8b, v29.8b

    ld1       {v22.2s}, [TMP2], STRIDE
    ld1       {v23.2s}, [TMP2]
    asr       TMP4, X, #16
    add       X, X, UX
    add       TMP4, TOP, TMP4, lsl #2
    umull     v9.8h, v22.8b, v28.8b
    umlal     v9.8h, v23.8b, v29.8b

    ld1       {v22.2s}, [TMP3], STRIDE
    ld1       {v23.2s}, [TMP3]
    umull     v10.8h, v22.8b, v28.8b
    umlal     v10.8h, v23.8b, v29.8b

    ushll     v0.4s, v8.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v0.4s, v8.4h, v15.h[0]
    umlal2    v0.4s, v8.8h, v15.h[0]

    prfm      PREFETCH_MODE, [TMP4, PF_OFFS]
    ld1       {v16.2s}, [TMP4], STRIDE
    ld1       {v17.2s}, [TMP4]
    prfm      PREFETCH_MODE, [TMP4, PF_OFFS]
    umull     v11.8h, v16.8b, v28.8b
    umlal     v11.8h, v17.8b, v29.8b

    ushll     v1.4s, v9.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v1.4s, v9.4h, v15.h[4]
.endm

.macro bilinear_interpolate_four_pixels_8888_8888_tail
    umlal2    v1.4s, v9.8h, v15.h[4]
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    ushll     v2.4s, v10.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v2.4s, v10.4h, v15.h[0]
    umlal2    v2.4s, v10.8h, v15.h[0]
    ushll     v3.4s, v11.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v3.4s, v11.4h, v15.h[4]
    umlal2    v3.4s, v11.8h, v15.h[4]
    add       v12.8h, v12.8h, v13.8h
    shrn      v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn2     v0.8h, v1.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    shrn      v2.4h, v2.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    shrn2     v2.8h, v3.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    xtn       v6.8b, v0.8h
    xtn       v7.8b, v2.8h
    add       v12.8h, v12.8h, v13.8h
    st1       {v6.2s, v7.2s}, [OUT], #16
.endm

.macro bilinear_interpolate_four_pixels_8888_8888_tail_head
    asr       TMP1, X, #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, lsl #2
    asr       TMP2, X, #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, lsl #2
        umlal2    v1.4s, v9.8h, v15.h[4]
        ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
        ushll     v2.4s, v10.4h, #BILINEAR_INTERPOLATION_BITS
        umlsl     v2.4s, v10.4h, v15.h[0]
        umlal2    v2.4s, v10.8h, v15.h[0]
        ushll     v3.4s, v11.4h, #BILINEAR_INTERPOLATION_BITS
    ld1       {v20.2s}, [TMP1], STRIDE
        umlsl     v3.4s, v11.4h, v15.h[4]
        umlal2    v3.4s, v11.8h, v15.h[4]
    ld1       {v21.2s}, [TMP1]
    umull     v8.8h, v20.8b, v28.8b
    umlal     v8.8h, v21.8b, v29.8b
        shrn      v0.4h, v0.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
        shrn2     v0.8h, v1.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
        shrn      v4.4h, v2.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
    ld1       {v22.2s}, [TMP2], STRIDE
        shrn2     v4.8h, v3.4s, #(2 * BILINEAR_INTERPOLATION_BITS)
        add       v12.8h, v12.8h, v13.8h
    ld1       {v23.2s}, [TMP2]
    umull     v9.8h, v22.8b, v28.8b
    asr       TMP3, X, #16
    add       X, X, UX
    add       TMP3, TOP, TMP3, lsl #2
    asr       TMP4, X, #16
    add       X, X, UX
    add       TMP4, TOP, TMP4, lsl #2
    umlal     v9.8h, v23.8b, v29.8b
    ld1       {v22.2s}, [TMP3], STRIDE
        ushr      v15.8h, v12.8h, #(16 - BILINEAR_INTERPOLATION_BITS)
    ld1       {v23.2s}, [TMP3]
    umull     v10.8h, v22.8b, v28.8b
    umlal     v10.8h, v23.8b, v29.8b
        xtn       v6.8b, v0.8h
    ushll     v0.4s, v8.4h, #BILINEAR_INTERPOLATION_BITS
        xtn       v7.8b, v4.8h
    umlsl     v0.4s, v8.4h, v15.h[0]
    umlal2    v0.4s, v8.8h, v15.h[0]
    prfm      PREFETCH_MODE, [TMP4, PF_OFFS]
    ld1       {v16.2s}, [TMP4], STRIDE
        add       v12.8h, v12.8h, v13.8h
    ld1       {v17.2s}, [TMP4]
    prfm      PREFETCH_MODE, [TMP4, PF_OFFS]
    umull     v11.8h, v16.8b, v28.8b
    umlal     v11.8h, v17.8b, v29.8b
        st1       {v6.2s, v7.2s}, [OUT], #16
    ushll     v1.4s, v9.4h, #BILINEAR_INTERPOLATION_BITS
    umlsl     v1.4s, v9.4h, v15.h[4]
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
