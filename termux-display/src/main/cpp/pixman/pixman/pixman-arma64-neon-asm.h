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
 * This file contains a macro ('generate_composite_function') which can
 * construct 2D image processing functions, based on a common template.
 * Any combinations of source, destination and mask images with 8bpp,
 * 16bpp, 24bpp, 32bpp color formats are supported.
 *
 * This macro takes care of:
 *  - handling of leading and trailing unaligned pixels
 *  - doing most of the work related to L2 cache preload
 *  - encourages the use of software pipelining for better instructions
 *    scheduling
 *
 * The user of this macro has to provide some configuration parameters
 * (bit depths for the images, prefetch distance, etc.) and a set of
 * macros, which should implement basic code chunks responsible for
 * pixels processing. See 'pixman-armv8-neon-asm.S' file for the usage
 * examples.
 *
 * TODO:
 *  - try overlapped pixel method (from Ian Rickards) when processing
 *    exactly two blocks of pixels
 *  - maybe add an option to do reverse scanline processing
 */

/*
 * Bit flags for 'generate_composite_function' macro which are used
 * to tune generated functions behavior.
 */
.set FLAG_DST_WRITEONLY,       0
.set FLAG_DST_READWRITE,       1
.set FLAG_DEINTERLEAVE_32BPP,  2

/*
 * Constants for selecting preferable prefetch type.
 */
.set PREFETCH_TYPE_NONE,       0 /* No prefetch at all */
.set PREFETCH_TYPE_SIMPLE,     1 /* A simple, fixed-distance-ahead prefetch */
.set PREFETCH_TYPE_ADVANCED,   2 /* Advanced fine-grained prefetch */

/*
 * prefetch mode
 * available modes are:
 * pldl1keep
 * pldl1strm
 * pldl2keep
 * pldl2strm
 * pldl3keep
 * pldl3strm
 */
#define PREFETCH_MODE pldl1keep

/*
 * Definitions of supplementary pixld/pixst macros (for partial load/store of
 * pixel data).
 */

.macro pixldst1 op, elem_size, reg1, mem_operand, abits
    \op {v\()\reg1\().\()\elem_size}, [\()\mem_operand\()], #8
.endm

.macro pixldst2 op, elem_size, reg1, reg2, mem_operand, abits
    \op {v\()\reg1\().\()\elem_size, v\()\reg2\().\()\elem_size}, [\()\mem_operand\()], #16
.endm

.macro pixldst4 op, elem_size, reg1, reg2, reg3, reg4, mem_operand, abits
    \op {v\()\reg1\().\()\elem_size, v\()\reg2\().\()\elem_size, v\()\reg3\().\()\elem_size, v\()\reg4\().\()\elem_size}, [\()\mem_operand\()], #32
.endm

.macro pixldst0 op, elem_size, reg1, idx, mem_operand, abits, bytes
    \op {v\()\reg1\().\()\elem_size}[\idx], [\()\mem_operand\()], #\()\bytes\()
.endm

.macro pixldst3 op, elem_size, reg1, reg2, reg3, mem_operand
    \op {v\()\reg1\().\()\elem_size, v\()\reg2\().\()\elem_size, v\()\reg3\().\()\elem_size}, [\()\mem_operand\()], #24
.endm

.macro pixldst30 op, elem_size, reg1, reg2, reg3, idx, mem_operand
    \op {v\()\reg1\().\()\elem_size, v\()\reg2\().\()\elem_size, v\()\reg3\().\()\elem_size}[\idx], [\()\mem_operand\()], #3
.endm

.macro pixldst numbytes, op, elem_size, basereg, mem_operand, abits
.if \numbytes == 32
    .if \elem_size==32
        pixldst4 \op, 2s, %(\basereg+4), %(\basereg+5), \
                              %(\basereg+6), %(\basereg+7), \mem_operand, \abits
    .elseif \elem_size==16
        pixldst4 \op, 4h, %(\basereg+4), %(\basereg+5), \
                              %(\basereg+6), %(\basereg+7), \mem_operand, \abits
    .else
        pixldst4 \op, 8b, %(\basereg+4), %(\basereg+5), \
                              %(\basereg+6), %(\basereg+7), \mem_operand, \abits
    .endif
.elseif \numbytes == 16
    .if \elem_size==32
          pixldst2 \op, 2s, %(\basereg+2), %(\basereg+3), \mem_operand, \abits
    .elseif \elem_size==16
          pixldst2 \op, 4h, %(\basereg+2), %(\basereg+3), \mem_operand, \abits
    .else
          pixldst2 \op, 8b, %(\basereg+2), %(\basereg+3), \mem_operand, \abits
    .endif
.elseif \numbytes == 8
    .if \elem_size==32
        pixldst1 \op, 2s, %(\basereg+1), \mem_operand, \abits
    .elseif \elem_size==16
        pixldst1 \op, 4h, %(\basereg+1), \mem_operand, \abits
    .else
        pixldst1 \op, 8b, %(\basereg+1), \mem_operand, \abits
    .endif
.elseif \numbytes == 4
    .if !RESPECT_STRICT_ALIGNMENT || (\elem_size == 32)
        pixldst0 \op, s, %(\basereg+0), 1, \mem_operand, \abits, 4
    .elseif \elem_size == 16
        pixldst0 \op, h, %(\basereg+0), 2, \mem_operand, \abits, 2
        pixldst0 \op, h, %(\basereg+0), 3, \mem_operand, \abits, 2
    .else
        pixldst0 \op, b, %(\basereg+0), 4, \mem_operand, \abits, 1
        pixldst0 \op, b, %(\basereg+0), 5, \mem_operand, \abits, 1
        pixldst0 \op, b, %(\basereg+0), 6, \mem_operand, \abits, 1
        pixldst0 \op, b, %(\basereg+0), 7, \mem_operand, \abits, 1
    .endif
.elseif \numbytes == 2
    .if !RESPECT_STRICT_ALIGNMENT || (\elem_size == 16)
        pixldst0 \op, h, %(\basereg+0), 1, \mem_operand, \abits, 2
    .else
        pixldst0 \op, b, %(\basereg+0), 2, \mem_operand, \abits, 1
        pixldst0 \op, b, %(\basereg+0), 3, \mem_operand, \abits, 1
    .endif
.elseif \numbytes == 1
        pixldst0 \op, b, %(\basereg+0), 1, \mem_operand, \abits, 1
.else
    .error "unsupported size: \numbytes"
.endif
.endm

.macro pixld numpix, bpp, basereg, mem_operand, abits=0
.if \bpp > 0
.if (\bpp == 32) && (\numpix == 8) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    pixldst4 ld4, 8b, %(\basereg+4), %(\basereg+5), \
                      %(\basereg+6), %(\basereg+7), \mem_operand, \abits
.elseif (\bpp == 24) && (\numpix == 8)
    pixldst3 ld3, 8b, %(\basereg+3), %(\basereg+4), %(\basereg+5), \mem_operand
.elseif (\bpp == 24) && (\numpix == 4)
    pixldst30 ld3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 4, \mem_operand
    pixldst30 ld3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 5, \mem_operand
    pixldst30 ld3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 6, \mem_operand
    pixldst30 ld3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 7, \mem_operand
.elseif (\bpp == 24) && (\numpix == 2)
    pixldst30 ld3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 2, \mem_operand
    pixldst30 ld3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 3, \mem_operand
.elseif (\bpp == 24) && (\numpix == 1)
    pixldst30 ld3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 1, \mem_operand
.else
    pixldst %(\numpix * \bpp / 8), ld1, %(\bpp), \basereg, \mem_operand, \abits
.endif
.endif
.endm

.macro pixst numpix, bpp, basereg, mem_operand, abits=0
.if \bpp > 0
.if (\bpp == 32) && (\numpix == 8) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    pixldst4 st4, 8b, %(\basereg+4), %(\basereg+5), \
                      %(\basereg+6), %(\basereg+7), \mem_operand, \abits
.elseif (\bpp == 24) && (\numpix == 8)
    pixldst3 st3, 8b, %(\basereg+3), %(\basereg+4), %(\basereg+5), \mem_operand
.elseif (\bpp == 24) && (\numpix == 4)
    pixldst30 st3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 4, \mem_operand
    pixldst30 st3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 5, \mem_operand
    pixldst30 st3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 6, \mem_operand
    pixldst30 st3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 7, \mem_operand
.elseif (\bpp == 24) && (\numpix == 2)
    pixldst30 st3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 2, \mem_operand
    pixldst30 st3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 3, \mem_operand
.elseif (\bpp == 24) && (\numpix == 1)
    pixldst30 st3, b, %(\basereg+0), %(\basereg+1), %(\basereg+2), 1, \mem_operand
.elseif \numpix * \bpp == 32 && \abits == 32
    pixldst 4, st1, 32, \basereg, \mem_operand, \abits
.elseif \numpix * \bpp == 16 && \abits == 16
    pixldst 2, st1, 16, \basereg, \mem_operand, \abits
.else
    pixldst %(\numpix * \bpp / 8), st1, %(\bpp), \basereg, \mem_operand, \abits
.endif
.endif
.endm

.macro pixld_a numpix, bpp, basereg, mem_operand
.if (\bpp * \numpix) <= 128
    pixld \numpix, \bpp, \basereg, \mem_operand, %(\bpp * \numpix)
.else
    pixld \numpix, \bpp, \basereg, \mem_operand, 128
.endif
.endm

.macro pixst_a numpix, bpp, basereg, mem_operand
.if (\bpp * \numpix) <= 128
    pixst \numpix, \bpp, \basereg, \mem_operand, %(\bpp * \numpix)
.else
    pixst \numpix, \bpp, \basereg, \mem_operand, 128
.endif
.endm

/*
 * Pixel fetcher for nearest scaling (needs TMP1, TMP2, VX, UNIT_X register
 * aliases to be defined)
 */
.macro pixld1_s elem_size, reg1, mem_operand
.if \elem_size == 16
    asr     TMP1, VX, #16
    adds    VX, VX, UNIT_X
    bmi     55f
5:  subs    VX, VX, SRC_WIDTH_FIXED
    bpl     5b
55:
    add     TMP1, \mem_operand, TMP1, lsl #1
    asr     TMP2, VX, #16
    adds    VX, VX, UNIT_X
    bmi     55f
5:  subs    VX, VX, SRC_WIDTH_FIXED
    bpl     5b
55:
    add     TMP2, \mem_operand, TMP2, lsl #1
    ld1     {v\()\reg1\().h}[0], [TMP1]
    asr     TMP1, VX, #16
    adds    VX, VX, UNIT_X
    bmi     55f
5:  subs    VX, VX, SRC_WIDTH_FIXED
    bpl     5b
55:
    add     TMP1, \mem_operand, TMP1, lsl #1
    ld1     {v\()\reg1\().h}[1], [TMP2]
    asr     TMP2, VX, #16
    adds    VX, VX, UNIT_X
    bmi     55f
5:  subs    VX, VX, SRC_WIDTH_FIXED
    bpl     5b
55:
    add     TMP2, \mem_operand, TMP2, lsl #1
    ld1     {v\()\reg1\().h}[2], [TMP1]
    ld1     {v\()\reg1\().h}[3], [TMP2]
.elseif \elem_size == 32
    asr     TMP1, VX, #16
    adds    VX, VX, UNIT_X
    bmi     55f
5:  subs    VX, VX, SRC_WIDTH_FIXED
    bpl     5b
55:
    add     TMP1, \mem_operand, TMP1, lsl #2
    asr     TMP2, VX, #16
    adds    VX, VX, UNIT_X
    bmi     55f
5:  subs    VX, VX, SRC_WIDTH_FIXED
    bpl     5b
55:
    add     TMP2, \mem_operand, TMP2, lsl #2
    ld1     {v\()\reg1\().s}[0], [TMP1]
    ld1     {v\()\reg1\().s}[1], [TMP2]
.else
    .error "unsupported"
.endif
.endm

.macro pixld2_s elem_size, reg1, reg2, mem_operand
.if 0 /* \elem_size == 32 */
    mov     TMP1, VX, asr #16
    add     VX, VX, UNIT_X, asl #1
    add     TMP1, \mem_operand, TMP1, asl #2
    mov     TMP2, VX, asr #16
    sub     VX, VX, UNIT_X
    add     TMP2, \mem_operand, TMP2, asl #2
    ld1     {v\()\reg1\().s}[0], [TMP1]
    mov     TMP1, VX, asr #16
    add     VX, VX, UNIT_X, asl #1
    add     TMP1, \mem_operand, TMP1, asl #2
    ld1     {v\()\reg2\().s}[0], [TMP2, :32]
    mov     TMP2, VX, asr #16
    add     VX, VX, UNIT_X
    add     TMP2, \mem_operand, TMP2, asl #2
    ld1     {v\()\reg1\().s}[1], [TMP1]
    ld1     {v\()\reg2\().s}[1], [TMP2]
.else
    pixld1_s \elem_size, \reg1, \mem_operand
    pixld1_s \elem_size, \reg2, \mem_operand
.endif
.endm

.macro pixld0_s elem_size, reg1, idx, mem_operand
.if \elem_size == 16
    asr     TMP1, VX, #16
    adds    VX, VX, UNIT_X
    bmi     55f
5:  subs    VX, VX, SRC_WIDTH_FIXED
    bpl     5b
55:
    add     TMP1, \mem_operand, TMP1, lsl #1
    ld1     {v\()\reg1\().h}[\idx], [TMP1]
.elseif \elem_size == 32
    asr     DUMMY, VX, #16
    mov     TMP1, DUMMY
    adds    VX, VX, UNIT_X
    bmi     55f
5:  subs    VX, VX, SRC_WIDTH_FIXED
    bpl     5b
55:
    add     TMP1, \mem_operand, TMP1, lsl #2
    ld1     {v\()\reg1\().s}[\idx], [TMP1]
.endif
.endm

.macro pixld_s_internal numbytes, elem_size, basereg, mem_operand
.if \numbytes == 32
    pixld2_s \elem_size, %(\basereg+4), %(\basereg+5), \mem_operand
    pixld2_s \elem_size, %(\basereg+6), %(\basereg+7), \mem_operand
    pixdeinterleave \elem_size, %(\basereg+4)
.elseif \numbytes == 16
    pixld2_s \elem_size, %(\basereg+2), %(\basereg+3), \mem_operand
.elseif \numbytes == 8
    pixld1_s \elem_size, %(\basereg+1), \mem_operand
.elseif \numbytes == 4
    .if \elem_size == 32
        pixld0_s \elem_size, %(\basereg+0), 1, \mem_operand
    .elseif \elem_size == 16
        pixld0_s \elem_size, %(\basereg+0), 2, \mem_operand
        pixld0_s \elem_size, %(\basereg+0), 3, \mem_operand
    .else
        pixld0_s \elem_size, %(\basereg+0), 4, \mem_operand
        pixld0_s \elem_size, %(\basereg+0), 5, \mem_operand
        pixld0_s \elem_size, %(\basereg+0), 6, \mem_operand
        pixld0_s \elem_size, %(\basereg+0), 7, \mem_operand
    .endif
.elseif \numbytes == 2
    .if \elem_size == 16
        pixld0_s \elem_size, %(\basereg+0), 1, \mem_operand
    .else
        pixld0_s \elem_size, %(\basereg+0), 2, \mem_operand
        pixld0_s \elem_size, %(\basereg+0), 3, \mem_operand
    .endif
.elseif \numbytes == 1
    pixld0_s \elem_size, %(\basereg+0), 1, \mem_operand
.else
    .error "unsupported size: \numbytes"
.endif
.endm

.macro pixld_s numpix, bpp, basereg, mem_operand
.if \bpp > 0
    pixld_s_internal %(\numpix * \bpp / 8), %(\bpp), \basereg, \mem_operand
.endif
.endm

.macro vuzp8 reg1, reg2
    umov DUMMY, v16.d[0]
    uzp1 v16.8b,          v\()\reg1\().8b, v\()\reg2\().8b
    uzp2 v\()\reg2\().8b, v\()\reg1\().8b, v\()\reg2\().8b
    mov  v\()\reg1\().8b, v16.8b
    mov  v16.d[0], DUMMY
.endm

.macro vzip8 reg1, reg2
    umov DUMMY, v16.d[0]
    zip1 v16.8b,          v\()\reg1\().8b, v\()\reg2\().8b
    zip2 v\()\reg2\().8b, v\()\reg1\().8b, v\()\reg2\().8b
    mov  v\()\reg1\().8b, v16.8b
    mov  v16.d[0], DUMMY
.endm

/* deinterleave B, G, R, A channels for eight 32bpp pixels in 4 registers */
.macro pixdeinterleave bpp, basereg
.if (\bpp == 32) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    vuzp8 %(\basereg+0), %(\basereg+1)
    vuzp8 %(\basereg+2), %(\basereg+3)
    vuzp8 %(\basereg+1), %(\basereg+3)
    vuzp8 %(\basereg+0), %(\basereg+2)
.endif
.endm

/* interleave B, G, R, A channels for eight 32bpp pixels in 4 registers */
.macro pixinterleave bpp, basereg
.if (\bpp == 32) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    vzip8 %(\basereg+0), %(\basereg+2)
    vzip8 %(\basereg+1), %(\basereg+3)
    vzip8 %(\basereg+2), %(\basereg+3)
    vzip8 %(\basereg+0), %(\basereg+1)
.endif
.endm

/*
 * This is a macro for implementing cache preload. The main idea is that
 * cache preload logic is mostly independent from the rest of pixels
 * processing code. It starts at the top left pixel and moves forward
 * across pixels and can jump across scanlines. Prefetch distance is
 * handled in an 'incremental' way: it starts from 0 and advances to the
 * optimal distance over time. After reaching optimal prefetch distance,
 * it is kept constant. There are some checks which prevent prefetching
 * unneeded pixel lines below the image (but it still can prefetch a bit
 * more data on the right side of the image - not a big issue and may
 * be actually helpful when rendering text glyphs). Additional trick is
 * the use of LDR instruction for prefetch instead of PLD when moving to
 * the next line, the point is that we have a high chance of getting TLB
 * miss in this case, and PLD would be useless.
 *
 * This sounds like it may introduce a noticeable overhead (when working with
 * fully cached data). But in reality, due to having a separate pipeline and
 * instruction queue for NEON unit in ARM Cortex-A8, normal ARM code can
 * execute simultaneously with NEON and be completely shadowed by it. Thus
 * we get no performance overhead at all (*). This looks like a very nice
 * feature of Cortex-A8, if used wisely. We don't have a hardware prefetcher,
 * but still can implement some rather advanced prefetch logic in software
 * for almost zero cost!
 *
 * (*) The overhead of the prefetcher is visible when running some trivial
 * pixels processing like simple copy. Anyway, having prefetch is a must
 * when working with the graphics data.
 */
.macro PF a, x:vararg
.if (PREFETCH_TYPE_CURRENT == PREFETCH_TYPE_ADVANCED)
    \a \x
.endif
.endm

.macro cache_preload std_increment, boost_increment
.if (src_bpp_shift >= 0) || (dst_r_bpp != 0) || (mask_bpp_shift >= 0)
.if \std_increment != 0
    PF add, PF_X, PF_X, #\std_increment
.endif
    PF tst, PF_CTL, #0xF
    PF beq, 71f
    PF add, PF_X, PF_X, #\boost_increment
    PF sub, PF_CTL, PF_CTL, #1
71:
    PF cmp, PF_X, ORIG_W
.if src_bpp_shift >= 0
    PF lsl, DUMMY, PF_X, #src_bpp_shift
    PF prfm, PREFETCH_MODE, [PF_SRC, DUMMY]
.endif
.if dst_r_bpp != 0
    PF lsl, DUMMY, PF_X, #dst_bpp_shift
    PF prfm, PREFETCH_MODE, [PF_DST, DUMMY]
.endif
.if mask_bpp_shift >= 0
    PF lsl, DUMMY, PF_X, #mask_bpp_shift
    PF prfm, PREFETCH_MODE, [PF_MASK, DUMMY]
.endif
    PF ble, 71f
    PF sub, PF_X, PF_X, ORIG_W
    PF subs, PF_CTL, PF_CTL, #0x10
71:
    PF ble, 72f
.if src_bpp_shift >= 0
    PF lsl, DUMMY, SRC_STRIDE, #src_bpp_shift
    PF ldrsb, DUMMY, [PF_SRC, DUMMY]
    PF add, PF_SRC, PF_SRC, #1
.endif
.if dst_r_bpp != 0
    PF lsl, DUMMY, DST_STRIDE, #dst_bpp_shift
    PF ldrsb, DUMMY, [PF_DST, DUMMY]
    PF add, PF_DST, PF_DST, #1
.endif
.if mask_bpp_shift >= 0
    PF lsl, DUMMY, MASK_STRIDE, #mask_bpp_shift
    PF ldrsb, DUMMY, [PF_MASK, DUMMY]
    PF add, PF_MASK, PF_MASK, #1
.endif
72:
.endif
.endm

.macro cache_preload_simple
.if (PREFETCH_TYPE_CURRENT == PREFETCH_TYPE_SIMPLE)
.if src_bpp > 0
    prfm PREFETCH_MODE, [SRC, #(PREFETCH_DISTANCE_SIMPLE * src_bpp / 8)]
.endif
.if dst_r_bpp > 0
    prfm PREFETCH_MODE, [DST_R, #(PREFETCH_DISTANCE_SIMPLE * dst_r_bpp / 8)]
.endif
.if mask_bpp > 0
    prfm PREFETCH_MODE, [MASK, #(PREFETCH_DISTANCE_SIMPLE * mask_bpp / 8)]
.endif
.endif
.endm

.macro fetch_mask_pixblock
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
.endm

/*
 * Macro which is used to process leading pixels until destination
 * pointer is properly aligned (at 16 bytes boundary). When destination
 * buffer uses 16bpp format, this is unnecessary, or even pointless.
 */
.macro ensure_destination_ptr_alignment process_pixblock_head, \
                                        process_pixblock_tail, \
                                        process_pixblock_tail_head
.if dst_w_bpp != 24
    tst         DST_R, #0xF
    beq         52f

.if src_bpp > 0 || mask_bpp > 0 || dst_r_bpp > 0
.irp lowbit, 1, 2, 4, 8, 16

.if (dst_w_bpp <= (\lowbit * 8)) && ((\lowbit * 8) < (pixblock_size * dst_w_bpp))
.if \lowbit < 16 /* we don't need more than 16-byte alignment */
    tst         DST_R, #\lowbit
    beq         51f
.endif
    pixld_src   (\lowbit * 8 / dst_w_bpp), src_bpp, src_basereg, SRC
    pixld       (\lowbit * 8 / dst_w_bpp), mask_bpp, mask_basereg, MASK
.if dst_r_bpp > 0
    pixld_a     (\lowbit * 8 / dst_r_bpp), dst_r_bpp, dst_r_basereg, DST_R
.else
    add         DST_R, DST_R, #\lowbit
.endif
    PF add,     PF_X, PF_X, #(\lowbit * 8 / dst_w_bpp)
    sub         W, W, #(\lowbit * 8 / dst_w_bpp)
51:
.endif
.endr
.endif
    pixdeinterleave src_bpp, src_basereg
    pixdeinterleave mask_bpp, mask_basereg
    pixdeinterleave dst_r_bpp, dst_r_basereg

    \process_pixblock_head
    cache_preload 0, pixblock_size
    cache_preload_simple
    \process_pixblock_tail

    pixinterleave dst_w_bpp, dst_w_basereg

.irp lowbit, 1, 2, 4, 8, 16
.if (dst_w_bpp <= (\lowbit * 8)) && ((\lowbit * 8) < (pixblock_size * dst_w_bpp))
.if \lowbit < 16 /* we don't need more than 16-byte alignment */
    tst         DST_W, #\lowbit
    beq         51f
.endif
.if src_bpp == 0 && mask_bpp == 0 && dst_r_bpp == 0
    sub         W, W, #(\lowbit * 8 / dst_w_bpp)
.endif
    pixst_a     (\lowbit * 8 / dst_w_bpp), dst_w_bpp, dst_w_basereg, DST_W
51:
.endif
.endr
.endif
52:
.endm

/*
 * Special code for processing up to (pixblock_size - 1) remaining
 * trailing pixels. As SIMD processing performs operation on
 * pixblock_size pixels, anything smaller than this has to be loaded
 * and stored in a special way. Loading and storing of pixel data is
 * performed in such a way that we fill some 'slots' in the NEON
 * registers (some slots naturally are unused), then perform compositing
 * operation as usual. In the end, the data is taken from these 'slots'
 * and saved to memory.
 *
 * cache_preload_flag - allows to suppress prefetch if
 *                      set to 0
 * dst_aligned_flag   - selects whether destination buffer
 *                      is aligned
 */
.macro process_trailing_pixels cache_preload_flag, \
                               dst_aligned_flag, \
                               process_pixblock_head, \
                               process_pixblock_tail, \
                               process_pixblock_tail_head
    tst         W, #(pixblock_size - 1)
    beq         52f
.if src_bpp > 0 || mask_bpp > 0 || dst_r_bpp > 0
.irp chunk_size, 16, 8, 4, 2, 1
.if pixblock_size > \chunk_size
    tst         W, #\chunk_size
    beq         51f
    pixld_src   \chunk_size, src_bpp, src_basereg, SRC
    pixld       \chunk_size, mask_bpp, mask_basereg, MASK
.if \dst_aligned_flag != 0
    pixld_a     \chunk_size, dst_r_bpp, dst_r_basereg, DST_R
.else
    pixld       \chunk_size, dst_r_bpp, dst_r_basereg, DST_R
.endif
.if \cache_preload_flag != 0
    PF add,     PF_X, PF_X, #\chunk_size
.endif
51:
.endif
.endr
.endif
    pixdeinterleave src_bpp, src_basereg
    pixdeinterleave mask_bpp, mask_basereg
    pixdeinterleave dst_r_bpp, dst_r_basereg

    \process_pixblock_head
.if \cache_preload_flag != 0
    cache_preload 0, pixblock_size
    cache_preload_simple
.endif
    \process_pixblock_tail
    pixinterleave dst_w_bpp, dst_w_basereg
.irp chunk_size, 16, 8, 4, 2, 1
.if pixblock_size > \chunk_size
    tst         W, #\chunk_size
    beq         51f
.if \dst_aligned_flag != 0
    pixst_a     \chunk_size, dst_w_bpp, dst_w_basereg, DST_W
.else
    pixst       \chunk_size, dst_w_bpp, dst_w_basereg, DST_W
.endif
51:
.endif
.endr
52:
.endm

/*
 * Macro, which performs all the needed operations to switch to the next
 * scanline and start the next loop iteration unless all the scanlines
 * are already processed.
 */
.macro advance_to_next_scanline start_of_loop_label
    mov         W, ORIG_W
    add         DST_W, DST_W, DST_STRIDE, lsl #dst_bpp_shift
.if src_bpp != 0
    add         SRC, SRC, SRC_STRIDE, lsl #src_bpp_shift
.endif
.if mask_bpp != 0
    add         MASK, MASK, MASK_STRIDE, lsl #mask_bpp_shift
.endif
.if (dst_w_bpp != 24)
    sub         DST_W, DST_W, W, lsl #dst_bpp_shift
.endif
.if (src_bpp != 24) && (src_bpp != 0)
    sub         SRC, SRC, W, lsl #src_bpp_shift
.endif
.if (mask_bpp != 24) && (mask_bpp != 0)
    sub         MASK, MASK, W, lsl #mask_bpp_shift
.endif
    subs        H, H, #1
    mov         DST_R, DST_W
    bge         \start_of_loop_label
.endm

/*
 * Registers are allocated in the following way by default:
 * v0, v1, v2, v3     - reserved for loading source pixel data
 * v4, v5, v6, v7     - reserved for loading destination pixel data
 * v24, v25, v26, v27 - reserved for loading mask pixel data
 * v28, v29, v30, v31 - final destination pixel data for writeback to memory
 */
.macro generate_composite_function fname, \
                                   src_bpp_, \
                                   mask_bpp_, \
                                   dst_w_bpp_, \
                                   flags, \
                                   pixblock_size_, \
                                   prefetch_distance, \
                                   init, \
                                   cleanup, \
                                   process_pixblock_head, \
                                   process_pixblock_tail, \
                                   process_pixblock_tail_head, \
                                   dst_w_basereg_ = 28, \
                                   dst_r_basereg_ = 4, \
                                   src_basereg_   = 0, \
                                   mask_basereg_  = 24

    pixman_asm_function \fname
    stp         x29, x30, [sp, -16]!
    mov         x29, sp
    sub         sp,   sp, 232  /* push all registers */
    sub         x29, x29, 64
    st1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], #32
    st1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], #32
    stp          x8,   x9, [x29, -80]
    stp         x10,  x11, [x29, -96]
    stp         x12,  x13, [x29, -112]
    stp         x14,  x15, [x29, -128]
    stp         x16,  x17, [x29, -144]
    stp         x18,  x19, [x29, -160]
    stp         x20,  x21, [x29, -176]
    stp         x22,  x23, [x29, -192]
    stp         x24,  x25, [x29, -208]
    stp         x26,  x27, [x29, -224]
    str         x28, [x29, -232]

/*
 * Select prefetch type for this function. If prefetch distance is
 * set to 0 or one of the color formats is 24bpp, SIMPLE prefetch
 * has to be used instead of ADVANCED.
 */
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_DEFAULT
.if \prefetch_distance == 0
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_NONE
.elseif (PREFETCH_TYPE_CURRENT > PREFETCH_TYPE_SIMPLE) && \
        ((\src_bpp_ == 24) || (\mask_bpp_ == 24) || (\dst_w_bpp_ == 24))
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_SIMPLE
.endif

/*
 * Make some macro arguments globally visible and accessible
 * from other macros
 */
    .set src_bpp, \src_bpp_
    .set mask_bpp, \mask_bpp_
    .set dst_w_bpp, \dst_w_bpp_
    .set pixblock_size, \pixblock_size_
    .set dst_w_basereg, \dst_w_basereg_
    .set dst_r_basereg, \dst_r_basereg_
    .set src_basereg, \src_basereg_
    .set mask_basereg, \mask_basereg_

    .macro pixld_src x:vararg
        pixld \x
    .endm
    .macro fetch_src_pixblock
        pixld_src   pixblock_size, src_bpp, \
                    (src_basereg - pixblock_size * src_bpp / 64), SRC
    .endm
/*
 * Assign symbolic names to registers
 */
    W           .req       x0      /* width (is updated during processing) */
    H           .req       x1      /* height (is updated during processing) */
    DST_W       .req       x2      /* destination buffer pointer for writes */
    DST_STRIDE  .req       x3      /* destination image stride */
    SRC         .req       x4      /* source buffer pointer */
    SRC_STRIDE  .req       x5      /* source image stride */
    MASK        .req       x6      /* mask pointer */
    MASK_STRIDE .req       x7      /* mask stride */

    DST_R       .req       x8      /* destination buffer pointer for reads */

    PF_CTL      .req       x9      /* combined lines counter and prefetch */
                                    /* distance increment counter */
    PF_X        .req       x10     /* pixel index in a scanline for current */
                                    /* pretetch position */
    PF_SRC      .req       x11     /* pointer to source scanline start */
                                    /* for prefetch purposes */
    PF_DST      .req       x12     /* pointer to destination scanline start */
                                    /* for prefetch purposes */
    PF_MASK     .req       x13     /* pointer to mask scanline start */
                                    /* for prefetch purposes */

    ORIG_W      .req       x14     /* saved original width */
    DUMMY       .req       x15     /* temporary register */

    sxtw        x0, w0
    sxtw        x1, w1
    sxtw        x3, w3
    sxtw        x5, w5
    sxtw        x7, w7

    .set mask_bpp_shift, -1
.if src_bpp == 32
    .set src_bpp_shift, 2
.elseif src_bpp == 24
    .set src_bpp_shift, 0
.elseif src_bpp == 16
    .set src_bpp_shift, 1
.elseif src_bpp == 8
    .set src_bpp_shift, 0
.elseif src_bpp == 0
    .set src_bpp_shift, -1
.else
    .error "requested src bpp (src_bpp) is not supported"
.endif
.if mask_bpp == 32
    .set mask_bpp_shift, 2
.elseif mask_bpp == 24
    .set mask_bpp_shift, 0
.elseif mask_bpp == 8
    .set mask_bpp_shift, 0
.elseif mask_bpp == 0
    .set mask_bpp_shift, -1
.else
    .error "requested mask bpp (mask_bpp) is not supported"
.endif
.if dst_w_bpp == 32
    .set dst_bpp_shift, 2
.elseif dst_w_bpp == 24
    .set dst_bpp_shift, 0
.elseif dst_w_bpp == 16
    .set dst_bpp_shift, 1
.elseif dst_w_bpp == 8
    .set dst_bpp_shift, 0
.else
    .error "requested dst bpp (dst_w_bpp) is not supported"
.endif

.if (((\flags) & FLAG_DST_READWRITE) != 0)
    .set dst_r_bpp, dst_w_bpp
.else
    .set dst_r_bpp, 0
.endif
.if (((\flags) & FLAG_DEINTERLEAVE_32BPP) != 0)
    .set DEINTERLEAVE_32BPP_ENABLED, 1
.else
    .set DEINTERLEAVE_32BPP_ENABLED, 0
.endif

.if \prefetch_distance < 0 || \prefetch_distance > 15
    .error "invalid prefetch distance (\prefetch_distance)"
.endif

    PF mov,     PF_X, #0
    mov         DST_R, DST_W

.if src_bpp == 24
    sub         SRC_STRIDE, SRC_STRIDE, W
    sub         SRC_STRIDE, SRC_STRIDE, W, lsl #1
.endif
.if mask_bpp == 24
    sub         MASK_STRIDE, MASK_STRIDE, W
    sub         MASK_STRIDE, MASK_STRIDE, W, lsl #1
.endif
.if dst_w_bpp == 24
    sub         DST_STRIDE, DST_STRIDE, W
    sub         DST_STRIDE, DST_STRIDE, W, lsl #1
.endif

/*
 * Setup advanced prefetcher initial state
 */
    PF mov,     PF_SRC, SRC
    PF mov,     PF_DST, DST_R
    PF mov,     PF_MASK, MASK
    /* PF_CTL = \prefetch_distance | ((h - 1) << 4) */
    PF lsl,     DUMMY, H, #4
    PF mov,     PF_CTL, DUMMY
    PF add,     PF_CTL, PF_CTL, #(\prefetch_distance - 0x10)

    \init
    subs        H, H, #1
    mov         ORIG_W, W
    blt         9f
    cmp         W, #(pixblock_size * 2)
    blt         800f
/*
 * This is the start of the pipelined loop, which if optimized for
 * long scanlines
 */
0:
    ensure_destination_ptr_alignment \process_pixblock_head, \
                                     \process_pixblock_tail, \
                                     \process_pixblock_tail_head

    /* Implement "head (tail_head) ... (tail_head) tail" loop pattern */
    pixld_a     pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    fetch_src_pixblock
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    PF add,     PF_X, PF_X, #pixblock_size
    \process_pixblock_head
    cache_preload 0, pixblock_size
    cache_preload_simple
    subs        W, W, #(pixblock_size * 2)
    blt         200f

100:
    \process_pixblock_tail_head
    cache_preload_simple
    subs        W, W, #pixblock_size
    bge         100b

200:
    \process_pixblock_tail
    pixst_a     pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W

    /* Process the remaining trailing pixels in the scanline */
    process_trailing_pixels 1, 1, \
                            \process_pixblock_head, \
                            \process_pixblock_tail, \
                            \process_pixblock_tail_head
    advance_to_next_scanline 0b

    \cleanup
1000:
    /* pop all registers */
    sub         x29, x29, 64
    ld1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    ld1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    ldp          x8,   x9, [x29, -80]
    ldp         x10,  x11, [x29, -96]
    ldp         x12,  x13, [x29, -112]
    ldp         x14,  x15, [x29, -128]
    ldp         x16,  x17, [x29, -144]
    ldp         x18,  x19, [x29, -160]
    ldp         x20,  x21, [x29, -176]
    ldp         x22,  x23, [x29, -192]
    ldp         x24,  x25, [x29, -208]
    ldp         x26,  x27, [x29, -224]
    ldr         x28, [x29, -232]
    mov         sp, x29
    ldp         x29, x30, [sp], 16
    ret  /* exit */
/*
 * This is the start of the loop, designed to process images with small width
 * (less than pixblock_size * 2 pixels). In this case neither pipelining
 * nor prefetch are used.
 */
800:
.if src_bpp_shift >= 0
    PF lsl, DUMMY, SRC_STRIDE, #src_bpp_shift
    PF prfm, PREFETCH_MODE, [SRC, DUMMY]
.endif
.if dst_r_bpp != 0
    PF lsl,  DUMMY, DST_STRIDE, #dst_bpp_shift
    PF prfm, PREFETCH_MODE, [DST_R, DUMMY]
.endif
.if mask_bpp_shift >= 0
    PF lsl,  DUMMY, MASK_STRIDE, #mask_bpp_shift
    PF prfm, PREFETCH_MODE, [MASK, DUMMY]
.endif
    /* Process exactly pixblock_size pixels if needed */
    tst         W, #pixblock_size
    beq         100f
    pixld       pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    fetch_src_pixblock
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    \process_pixblock_head
    \process_pixblock_tail
    pixst       pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W
100:
    /* Process the remaining trailing pixels in the scanline */
    process_trailing_pixels 0, 0, \
                            \process_pixblock_head, \
                            \process_pixblock_tail, \
                            \process_pixblock_tail_head
    advance_to_next_scanline 800b
9:
    \cleanup
    /* pop all registers */
    sub         x29, x29, 64
    ld1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    ld1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    ldp          x8,   x9, [x29, -80]
    ldp         x10,  x11, [x29, -96]
    ldp         x12,  x13, [x29, -112]
    ldp         x14,  x15, [x29, -128]
    ldp         x16,  x17, [x29, -144]
    ldp         x18,  x19, [x29, -160]
    ldp         x20,  x21, [x29, -176]
    ldp         x22,  x23, [x29, -192]
    ldp         x24,  x25, [x29, -208]
    ldp         x26,  x27, [x29, -224]
    ldr         x28, [x29, -232]
    mov         sp, x29
    ldp         x29, x30, [sp], 16
    ret  /* exit */

    .purgem     fetch_src_pixblock
    .purgem     pixld_src

    .unreq      SRC
    .unreq      MASK
    .unreq      DST_R
    .unreq      DST_W
    .unreq      ORIG_W
    .unreq      W
    .unreq      H
    .unreq      SRC_STRIDE
    .unreq      DST_STRIDE
    .unreq      MASK_STRIDE
    .unreq      PF_CTL
    .unreq      PF_X
    .unreq      PF_SRC
    .unreq      PF_DST
    .unreq      PF_MASK
    .unreq      DUMMY
    pixman_end_asm_function
.endm

/*
 * A simplified variant of function generation template for a single
 * scanline processing (for implementing pixman combine functions)
 */
.macro generate_composite_function_scanline        use_nearest_scaling, \
                                                   fname, \
                                                   src_bpp_, \
                                                   mask_bpp_, \
                                                   dst_w_bpp_, \
                                                   flags, \
                                                   pixblock_size_, \
                                                   init, \
                                                   cleanup, \
                                                   process_pixblock_head, \
                                                   process_pixblock_tail, \
                                                   process_pixblock_tail_head, \
                                                   dst_w_basereg_ = 28, \
                                                   dst_r_basereg_ = 4, \
                                                   src_basereg_   = 0, \
                                                   mask_basereg_  = 24

    pixman_asm_function \fname
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_NONE

/*
 * Make some macro arguments globally visible and accessible
 * from other macros
 */
    .set src_bpp, \src_bpp_
    .set mask_bpp, \mask_bpp_
    .set dst_w_bpp, \dst_w_bpp_
    .set pixblock_size, \pixblock_size_
    .set dst_w_basereg, \dst_w_basereg_
    .set dst_r_basereg, \dst_r_basereg_
    .set src_basereg, \src_basereg_
    .set mask_basereg, \mask_basereg_

.if \use_nearest_scaling != 0
    /*
     * Assign symbolic names to registers for nearest scaling
     */
    W           .req        x0
    DST_W       .req        x1
    SRC         .req        x2
    VX          .req        x3
    UNIT_X      .req        x4
    SRC_WIDTH_FIXED .req    x5
    MASK        .req        x6
    TMP1        .req        x8
    TMP2        .req        x9
    DST_R       .req        x10
    DUMMY       .req        x30

    .macro pixld_src x:vararg
        pixld_s \x
    .endm

    sxtw        x0, w0
    sxtw        x3, w3
    sxtw        x4, w4
    sxtw        x5, w5

    stp         x29, x30, [sp, -16]!
    mov         x29, sp
    sub         sp, sp, 88
    sub         x29, x29, 64
    st1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    st1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    stp         x8, x9, [x29, -80]
    str         x10, [x29, -88]
.else
    /*
     * Assign symbolic names to registers
     */
    W           .req        x0      /* width (is updated during processing) */
    DST_W       .req        x1      /* destination buffer pointer for writes */
    SRC         .req        x2      /* source buffer pointer */
    MASK        .req        x3      /* mask pointer */
    DST_R       .req        x4      /* destination buffer pointer for reads */
    DUMMY       .req        x30

    .macro pixld_src x:vararg
        pixld \x
    .endm

    sxtw        x0, w0

    stp         x29, x30, [sp, -16]!
    mov         x29, sp
    sub         sp, sp, 64
    sub         x29, x29, 64
    st1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    st1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
.endif

.if (((\flags) & FLAG_DST_READWRITE) != 0)
    .set dst_r_bpp, dst_w_bpp
.else
    .set dst_r_bpp, 0
.endif
.if (((\flags) & FLAG_DEINTERLEAVE_32BPP) != 0)
    .set DEINTERLEAVE_32BPP_ENABLED, 1
.else
    .set DEINTERLEAVE_32BPP_ENABLED, 0
.endif

    .macro fetch_src_pixblock
        pixld_src   pixblock_size, src_bpp, \
                    (src_basereg - pixblock_size * src_bpp / 64), SRC
    .endm

    \init
    mov         DST_R, DST_W

    cmp         W, #pixblock_size
    blt         800f

    ensure_destination_ptr_alignment \process_pixblock_head, \
                                     \process_pixblock_tail, \
                                     \process_pixblock_tail_head

    subs        W, W, #pixblock_size
    blt         700f

    /* Implement "head (tail_head) ... (tail_head) tail" loop pattern */
    pixld_a     pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    fetch_src_pixblock
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    \process_pixblock_head
    subs        W, W, #pixblock_size
    blt         200f
100:
    \process_pixblock_tail_head
    subs        W, W, #pixblock_size
    bge         100b
200:
    \process_pixblock_tail
    pixst_a     pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W
700:
    /* Process the remaining trailing pixels in the scanline (dst aligned) */
    process_trailing_pixels 0, 1, \
                            \process_pixblock_head, \
                            \process_pixblock_tail, \
                            \process_pixblock_tail_head

    \cleanup
.if \use_nearest_scaling != 0
    sub         x29, x29, 64
    ld1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    ld1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    ldp         x8, x9, [x29, -80]
    ldr         x10, [x29, -96]
    mov         sp, x29
    ldp         x29, x30, [sp], 16
    ret  /* exit */
.else
    sub         x29, x29, 64
    ld1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    ld1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    mov         sp, x29
    ldp         x29, x30, [sp], 16
    ret  /* exit */
.endif
800:
    /* Process the remaining trailing pixels in the scanline (dst unaligned) */
    process_trailing_pixels 0, 0, \
                            \process_pixblock_head, \
                            \process_pixblock_tail, \
                            \process_pixblock_tail_head

    \cleanup
.if \use_nearest_scaling != 0
    sub         x29, x29, 64
    ld1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    ld1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    ldp         x8, x9, [x29, -80]
    ldr         x10, [x29, -88]
    mov         sp, x29
    ldp         x29, x30, [sp], 16
    ret  /* exit */

    .unreq      DUMMY
    .unreq      DST_R
    .unreq      SRC
    .unreq      W
    .unreq      VX
    .unreq      UNIT_X
    .unreq      TMP1
    .unreq      TMP2
    .unreq      DST_W
    .unreq      MASK
    .unreq      SRC_WIDTH_FIXED

.else
    sub         x29, x29, 64
    ld1         {v8.8b, v9.8b, v10.8b, v11.8b}, [x29], 32
    ld1         {v12.8b, v13.8b, v14.8b, v15.8b}, [x29], 32
    mov          sp, x29
    ldp          x29, x30, [sp], 16
    ret  /* exit */

    .unreq      DUMMY
    .unreq      SRC
    .unreq      MASK
    .unreq      DST_R
    .unreq      DST_W
    .unreq      W
.endif

    .purgem     fetch_src_pixblock
    .purgem     pixld_src

    pixman_end_asm_function
.endm

.macro generate_composite_function_single_scanline x:vararg
    generate_composite_function_scanline 0, \x
.endm

.macro generate_composite_function_nearest_scanline x:vararg
    generate_composite_function_scanline 1, \x
.endm

/* Default prologue/epilogue, nothing special needs to be done */

.macro default_init
.endm

.macro default_cleanup
.endm

/*
 * Prologue/epilogue variant which additionally saves/restores v8-v15
 * registers (they need to be saved/restored by callee according to ABI).
 * This is required if the code needs to use all the NEON registers.
 */

.macro default_init_need_all_regs
.endm

.macro default_cleanup_need_all_regs
.endm

/******************************************************************************/

/*
 * Conversion of 8 r5g6b6 pixels packed in 128-bit register (in)
 * into a planar a8r8g8b8 format (with a, r, g, b color components
 * stored into 64-bit registers out_a, out_r, out_g, out_b respectively).
 *
 * Warning: the conversion is destructive and the original
 *          value (in) is lost.
 */
.macro convert_0565_to_8888 in, out_a, out_r, out_g, out_b
    shrn        \()\out_r\().8b, \()\in\().8h,    #8
    shrn        \()\out_g\().8b, \()\in\().8h,    #3
    sli         \()\in\().8h,    \()\in\().8h,    #5
    movi        \()\out_a\().8b, #255
    sri         \()\out_r\().8b, \()\out_r\().8b, #5
    sri         \()\out_g\().8b, \()\out_g\().8b, #6
    shrn        \()\out_b\().8b, \()\in\().8h,    #2
.endm

.macro convert_0565_to_x888 in, out_r, out_g, out_b
    shrn        \()\out_r\().8b, \()\in\().8h,    #8
    shrn        \()\out_g\().8b, \()\in\().8h,    #3
    sli         \()\in\().8h,    \()\in\().8h,    #5
    sri         \()\out_r\().8b, \()\out_r\().8b, #5
    sri         \()\out_g\().8b, \()\out_g\().8b, #6
    shrn        \()\out_b\().8b, \()\in\().8h,    #2
.endm

/*
 * Conversion from planar a8r8g8b8 format (with a, r, g, b color components
 * in 64-bit registers in_a, in_r, in_g, in_b respectively) into 8 r5g6b6
 * pixels packed in 128-bit register (out). Requires two temporary 128-bit
 * registers (tmp1, tmp2)
 */
.macro convert_8888_to_0565 in_r, in_g, in_b, out, tmp1, tmp2
    ushll       \()\tmp1\().8h, \()\in_g\().8b, #7
    shl         \()\tmp1\().8h, \()\tmp1\().8h, #1
    ushll       \()\out\().8h,  \()\in_r\().8b, #7
    shl         \()\out\().8h,  \()\out\().8h,  #1
    ushll       \()\tmp2\().8h, \()\in_b\().8b, #7
    shl         \()\tmp2\().8h, \()\tmp2\().8h, #1
    sri         \()\out\().8h, \()\tmp1\().8h, #5
    sri         \()\out\().8h, \()\tmp2\().8h, #11
.endm

/*
 * Conversion of four r5g6b5 pixels (in) to four x8r8g8b8 pixels
 * returned in (out0, out1) registers pair. Requires one temporary
 * 64-bit register (tmp). 'out1' and 'in' may overlap, the original
 * value from 'in' is lost
 */
.macro convert_four_0565_to_x888_packed in, out0, out1, tmp
    shl         \()\out0\().4h, \()\in\().4h,   #5  /* G top 6 bits */
    shl         \()\tmp\().4h,  \()\in\().4h,   #11 /* B top 5 bits */
    sri         \()\in\().4h,   \()\in\().4h,   #5  /* R is ready \in top bits */
    sri         \()\out0\().4h, \()\out0\().4h, #6  /* G is ready \in top bits */
    sri         \()\tmp\().4h,  \()\tmp\().4h,  #5  /* B is ready \in top bits */
    ushr        \()\out1\().4h, \()\in\().4h,   #8  /* R is \in place */
    sri         \()\out0\().4h, \()\tmp\().4h,  #8  /* G \() B is \in place */
    zip1        \()\tmp\().4h,  \()\out0\().4h, \()\out1\().4h  /* everything is \in place */
    zip2        \()\out1\().4h, \()\out0\().4h, \()\out1\().4h
    mov         \()\out0\().d[0], \()\tmp\().d[0]
.endm
