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
 * pixels processing. See 'pixman-arm-neon-asm.S' file for the usage
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
 * Offset in stack where mask and source pointer/stride can be accessed
 * from 'init' macro. This is useful for doing special handling for solid mask.
 */
.set ARGS_STACK_OFFSET,        40

/*
 * Constants for selecting preferable prefetch type.
 */
.set PREFETCH_TYPE_NONE,       0 /* No prefetch at all */
.set PREFETCH_TYPE_SIMPLE,     1 /* A simple, fixed-distance-ahead prefetch */
.set PREFETCH_TYPE_ADVANCED,   2 /* Advanced fine-grained prefetch */

/*
 * Definitions of supplementary pixld/pixst macros (for partial load/store of
 * pixel data).
 */

.macro pixldst1 op, elem_size, reg1, mem_operand, abits
.if abits > 0
    op&.&elem_size {d&reg1}, [&mem_operand&, :&abits&]!
.else
    op&.&elem_size {d&reg1}, [&mem_operand&]!
.endif
.endm

.macro pixldst2 op, elem_size, reg1, reg2, mem_operand, abits
.if abits > 0
    op&.&elem_size {d&reg1, d&reg2}, [&mem_operand&, :&abits&]!
.else
    op&.&elem_size {d&reg1, d&reg2}, [&mem_operand&]!
.endif
.endm

.macro pixldst4 op, elem_size, reg1, reg2, reg3, reg4, mem_operand, abits
.if abits > 0
    op&.&elem_size {d&reg1, d&reg2, d&reg3, d&reg4}, [&mem_operand&, :&abits&]!
.else
    op&.&elem_size {d&reg1, d&reg2, d&reg3, d&reg4}, [&mem_operand&]!
.endif
.endm

.macro pixldst0 op, elem_size, reg1, idx, mem_operand, abits
    op&.&elem_size {d&reg1[idx]}, [&mem_operand&]!
.endm

.macro pixldst3 op, elem_size, reg1, reg2, reg3, mem_operand
    op&.&elem_size {d&reg1, d&reg2, d&reg3}, [&mem_operand&]!
.endm

.macro pixldst30 op, elem_size, reg1, reg2, reg3, idx, mem_operand
    op&.&elem_size {d&reg1[idx], d&reg2[idx], d&reg3[idx]}, [&mem_operand&]!
.endm

.macro pixldst numbytes, op, elem_size, basereg, mem_operand, abits
.if numbytes == 32
    pixldst4 op, elem_size, %(basereg+4), %(basereg+5), \
                              %(basereg+6), %(basereg+7), mem_operand, abits
.elseif numbytes == 16
    pixldst2 op, elem_size, %(basereg+2), %(basereg+3), mem_operand, abits
.elseif numbytes == 8
    pixldst1 op, elem_size, %(basereg+1), mem_operand, abits
.elseif numbytes == 4
    .if !RESPECT_STRICT_ALIGNMENT || (elem_size == 32)
        pixldst0 op, 32, %(basereg+0), 1, mem_operand, abits
    .elseif elem_size == 16
        pixldst0 op, 16, %(basereg+0), 2, mem_operand, abits
        pixldst0 op, 16, %(basereg+0), 3, mem_operand, abits
    .else
        pixldst0 op, 8, %(basereg+0), 4, mem_operand, abits
        pixldst0 op, 8, %(basereg+0), 5, mem_operand, abits
        pixldst0 op, 8, %(basereg+0), 6, mem_operand, abits
        pixldst0 op, 8, %(basereg+0), 7, mem_operand, abits
    .endif
.elseif numbytes == 2
    .if !RESPECT_STRICT_ALIGNMENT || (elem_size == 16)
        pixldst0 op, 16, %(basereg+0), 1, mem_operand, abits
    .else
        pixldst0 op, 8, %(basereg+0), 2, mem_operand, abits
        pixldst0 op, 8, %(basereg+0), 3, mem_operand, abits
    .endif
.elseif numbytes == 1
    pixldst0 op, 8, %(basereg+0), 1, mem_operand, abits
.else
    .error "unsupported size: numbytes"
.endif
.endm

.macro pixld numpix, bpp, basereg, mem_operand, abits=0
.if bpp > 0
.if (bpp == 32) && (numpix == 8) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    pixldst4 vld4, 8, %(basereg+4), %(basereg+5), \
                      %(basereg+6), %(basereg+7), mem_operand, abits
.elseif (bpp == 24) && (numpix == 8)
    pixldst3 vld3, 8, %(basereg+3), %(basereg+4), %(basereg+5), mem_operand
.elseif (bpp == 24) && (numpix == 4)
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 4, mem_operand
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 5, mem_operand
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 6, mem_operand
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 7, mem_operand
.elseif (bpp == 24) && (numpix == 2)
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 2, mem_operand
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 3, mem_operand
.elseif (bpp == 24) && (numpix == 1)
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 1, mem_operand
.else
    pixldst %(numpix * bpp / 8), vld1, %(bpp), basereg, mem_operand, abits
.endif
.endif
.endm

.macro pixst numpix, bpp, basereg, mem_operand, abits=0
.if bpp > 0
.if (bpp == 32) && (numpix == 8) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    pixldst4 vst4, 8, %(basereg+4), %(basereg+5), \
                      %(basereg+6), %(basereg+7), mem_operand, abits
.elseif (bpp == 24) && (numpix == 8)
    pixldst3 vst3, 8, %(basereg+3), %(basereg+4), %(basereg+5), mem_operand
.elseif (bpp == 24) && (numpix == 4)
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 4, mem_operand
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 5, mem_operand
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 6, mem_operand
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 7, mem_operand
.elseif (bpp == 24) && (numpix == 2)
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 2, mem_operand
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 3, mem_operand
.elseif (bpp == 24) && (numpix == 1)
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 1, mem_operand
.else
    pixldst %(numpix * bpp / 8), vst1, %(bpp), basereg, mem_operand, abits
.endif
.endif
.endm

.macro pixld_a numpix, bpp, basereg, mem_operand
.if (bpp * numpix) <= 128
    pixld numpix, bpp, basereg, mem_operand, %(bpp * numpix)
.else
    pixld numpix, bpp, basereg, mem_operand, 128
.endif
.endm

.macro pixst_a numpix, bpp, basereg, mem_operand
.if (bpp * numpix) <= 128
    pixst numpix, bpp, basereg, mem_operand, %(bpp * numpix)
.else
    pixst numpix, bpp, basereg, mem_operand, 128
.endif
.endm

/*
 * Pixel fetcher for nearest scaling (needs TMP1, TMP2, VX, UNIT_X register
 * aliases to be defined)
 */
.macro pixld1_s elem_size, reg1, mem_operand
.if elem_size == 16
    mov     TMP1, VX, asr #16
    adds    VX, VX, UNIT_X
5:  subpls  VX, VX, SRC_WIDTH_FIXED
    bpl     5b
    add     TMP1, mem_operand, TMP1, asl #1
    mov     TMP2, VX, asr #16
    adds    VX, VX, UNIT_X
5:  subpls  VX, VX, SRC_WIDTH_FIXED
    bpl     5b
    add     TMP2, mem_operand, TMP2, asl #1
    vld1.16 {d&reg1&[0]}, [TMP1, :16]
    mov     TMP1, VX, asr #16
    adds    VX, VX, UNIT_X
5:  subpls  VX, VX, SRC_WIDTH_FIXED
    bpl     5b
    add     TMP1, mem_operand, TMP1, asl #1
    vld1.16 {d&reg1&[1]}, [TMP2, :16]
    mov     TMP2, VX, asr #16
    adds    VX, VX, UNIT_X
5:  subpls  VX, VX, SRC_WIDTH_FIXED
    bpl     5b
    add     TMP2, mem_operand, TMP2, asl #1
    vld1.16 {d&reg1&[2]}, [TMP1, :16]
    vld1.16 {d&reg1&[3]}, [TMP2, :16]
.elseif elem_size == 32
    mov     TMP1, VX, asr #16
    adds    VX, VX, UNIT_X
5:  subpls  VX, VX, SRC_WIDTH_FIXED
    bpl     5b
    add     TMP1, mem_operand, TMP1, asl #2
    mov     TMP2, VX, asr #16
    adds    VX, VX, UNIT_X
5:  subpls  VX, VX, SRC_WIDTH_FIXED
    bpl     5b
    add     TMP2, mem_operand, TMP2, asl #2
    vld1.32 {d&reg1&[0]}, [TMP1, :32]
    vld1.32 {d&reg1&[1]}, [TMP2, :32]
.else
    .error "unsupported"
.endif
.endm

.macro pixld2_s elem_size, reg1, reg2, mem_operand
.if 0 /* elem_size == 32 */
    mov     TMP1, VX, asr #16
    add     VX, VX, UNIT_X, asl #1
    add     TMP1, mem_operand, TMP1, asl #2
    mov     TMP2, VX, asr #16
    sub     VX, VX, UNIT_X
    add     TMP2, mem_operand, TMP2, asl #2
    vld1.32 {d&reg1&[0]}, [TMP1, :32]
    mov     TMP1, VX, asr #16
    add     VX, VX, UNIT_X, asl #1
    add     TMP1, mem_operand, TMP1, asl #2
    vld1.32 {d&reg2&[0]}, [TMP2, :32]
    mov     TMP2, VX, asr #16
    add     VX, VX, UNIT_X
    add     TMP2, mem_operand, TMP2, asl #2
    vld1.32 {d&reg1&[1]}, [TMP1, :32]
    vld1.32 {d&reg2&[1]}, [TMP2, :32]
.else
    pixld1_s elem_size, reg1, mem_operand
    pixld1_s elem_size, reg2, mem_operand
.endif
.endm

.macro pixld0_s elem_size, reg1, idx, mem_operand
.if elem_size == 16
    mov     TMP1, VX, asr #16
    adds    VX, VX, UNIT_X
5:  subpls  VX, VX, SRC_WIDTH_FIXED
    bpl     5b
    add     TMP1, mem_operand, TMP1, asl #1
    vld1.16 {d&reg1&[idx]}, [TMP1, :16]
.elseif elem_size == 32
    mov     TMP1, VX, asr #16
    adds    VX, VX, UNIT_X
5:  subpls  VX, VX, SRC_WIDTH_FIXED
    bpl     5b
    add     TMP1, mem_operand, TMP1, asl #2
    vld1.32 {d&reg1&[idx]}, [TMP1, :32]
.endif
.endm

.macro pixld_s_internal numbytes, elem_size, basereg, mem_operand
.if numbytes == 32
    pixld2_s elem_size, %(basereg+4), %(basereg+5), mem_operand
    pixld2_s elem_size, %(basereg+6), %(basereg+7), mem_operand
    pixdeinterleave elem_size, %(basereg+4)
.elseif numbytes == 16
    pixld2_s elem_size, %(basereg+2), %(basereg+3), mem_operand
.elseif numbytes == 8
    pixld1_s elem_size, %(basereg+1), mem_operand
.elseif numbytes == 4
    .if elem_size == 32
        pixld0_s elem_size, %(basereg+0), 1, mem_operand
    .elseif elem_size == 16
        pixld0_s elem_size, %(basereg+0), 2, mem_operand
        pixld0_s elem_size, %(basereg+0), 3, mem_operand
    .else
        pixld0_s elem_size, %(basereg+0), 4, mem_operand
        pixld0_s elem_size, %(basereg+0), 5, mem_operand
        pixld0_s elem_size, %(basereg+0), 6, mem_operand
        pixld0_s elem_size, %(basereg+0), 7, mem_operand
    .endif
.elseif numbytes == 2
    .if elem_size == 16
        pixld0_s elem_size, %(basereg+0), 1, mem_operand
    .else
        pixld0_s elem_size, %(basereg+0), 2, mem_operand
        pixld0_s elem_size, %(basereg+0), 3, mem_operand
    .endif
.elseif numbytes == 1
    pixld0_s elem_size, %(basereg+0), 1, mem_operand
.else
    .error "unsupported size: numbytes"
.endif
.endm

.macro pixld_s numpix, bpp, basereg, mem_operand
.if bpp > 0
    pixld_s_internal %(numpix * bpp / 8), %(bpp), basereg, mem_operand
.endif
.endm

.macro vuzp8 reg1, reg2
    vuzp.8 d&reg1, d&reg2
.endm

.macro vzip8 reg1, reg2
    vzip.8 d&reg1, d&reg2
.endm

/* deinterleave B, G, R, A channels for eight 32bpp pixels in 4 registers */
.macro pixdeinterleave bpp, basereg
.if (bpp == 32) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    vuzp8 %(basereg+0), %(basereg+1)
    vuzp8 %(basereg+2), %(basereg+3)
    vuzp8 %(basereg+1), %(basereg+3)
    vuzp8 %(basereg+0), %(basereg+2)
.endif
.endm

/* interleave B, G, R, A channels for eight 32bpp pixels in 4 registers */
.macro pixinterleave bpp, basereg
.if (bpp == 32) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    vzip8 %(basereg+0), %(basereg+2)
    vzip8 %(basereg+1), %(basereg+3)
    vzip8 %(basereg+2), %(basereg+3)
    vzip8 %(basereg+0), %(basereg+1)
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
    a x
.endif
.endm

.macro cache_preload std_increment, boost_increment
.if (src_bpp_shift >= 0) || (dst_r_bpp != 0) || (mask_bpp_shift >= 0)
.if regs_shortage
    PF ldr ORIG_W, [sp] /* If we are short on regs, ORIG_W is kept on stack */
.endif
.if std_increment != 0
    PF add PF_X, PF_X, #std_increment
.endif
    PF tst PF_CTL, #0xF
    PF addne PF_X, PF_X, #boost_increment
    PF subne PF_CTL, PF_CTL, #1
    PF cmp PF_X, ORIG_W
.if src_bpp_shift >= 0
    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
.endif
.if dst_r_bpp != 0
    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
.endif
.if mask_bpp_shift >= 0
    PF pld, [PF_MASK, PF_X, lsl #mask_bpp_shift]
.endif
    PF subge PF_X, PF_X, ORIG_W
    PF subges PF_CTL, PF_CTL, #0x10
.if src_bpp_shift >= 0
    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
.endif
.if dst_r_bpp != 0
    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
.endif
.if mask_bpp_shift >= 0
    PF ldrgeb DUMMY, [PF_MASK, MASK_STRIDE, lsl #mask_bpp_shift]!
.endif
.endif
.endm

.macro cache_preload_simple
.if (PREFETCH_TYPE_CURRENT == PREFETCH_TYPE_SIMPLE)
.if src_bpp > 0
    pld [SRC, #(PREFETCH_DISTANCE_SIMPLE * src_bpp / 8)]
.endif
.if dst_r_bpp > 0
    pld [DST_R, #(PREFETCH_DISTANCE_SIMPLE * dst_r_bpp / 8)]
.endif
.if mask_bpp > 0
    pld [MASK, #(PREFETCH_DISTANCE_SIMPLE * mask_bpp / 8)]
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
    beq         2f

.irp lowbit, 1, 2, 4, 8, 16
local skip1
.if (dst_w_bpp <= (lowbit * 8)) && ((lowbit * 8) < (pixblock_size * dst_w_bpp))
.if lowbit < 16 /* we don't need more than 16-byte alignment */
    tst         DST_R, #lowbit
    beq         1f
.endif
    pixld_src   (lowbit * 8 / dst_w_bpp), src_bpp, src_basereg, SRC
    pixld       (lowbit * 8 / dst_w_bpp), mask_bpp, mask_basereg, MASK
.if dst_r_bpp > 0
    pixld_a     (lowbit * 8 / dst_r_bpp), dst_r_bpp, dst_r_basereg, DST_R
.else
    add         DST_R, DST_R, #lowbit
.endif
    PF add      PF_X, PF_X, #(lowbit * 8 / dst_w_bpp)
    sub         W, W, #(lowbit * 8 / dst_w_bpp)
1:
.endif
.endr
    pixdeinterleave src_bpp, src_basereg
    pixdeinterleave mask_bpp, mask_basereg
    pixdeinterleave dst_r_bpp, dst_r_basereg

    process_pixblock_head
    cache_preload 0, pixblock_size
    cache_preload_simple
    process_pixblock_tail

    pixinterleave dst_w_bpp, dst_w_basereg
.irp lowbit, 1, 2, 4, 8, 16
.if (dst_w_bpp <= (lowbit * 8)) && ((lowbit * 8) < (pixblock_size * dst_w_bpp))
.if lowbit < 16 /* we don't need more than 16-byte alignment */
    tst         DST_W, #lowbit
    beq         1f
.endif
    pixst_a     (lowbit * 8 / dst_w_bpp), dst_w_bpp, dst_w_basereg, DST_W
1:
.endif
.endr
.endif
2:
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
    beq         2f
.irp chunk_size, 16, 8, 4, 2, 1
.if pixblock_size > chunk_size
    tst         W, #chunk_size
    beq         1f
    pixld_src   chunk_size, src_bpp, src_basereg, SRC
    pixld       chunk_size, mask_bpp, mask_basereg, MASK
.if dst_aligned_flag != 0
    pixld_a     chunk_size, dst_r_bpp, dst_r_basereg, DST_R
.else
    pixld       chunk_size, dst_r_bpp, dst_r_basereg, DST_R
.endif
.if cache_preload_flag != 0
    PF add      PF_X, PF_X, #chunk_size
.endif
1:
.endif
.endr
    pixdeinterleave src_bpp, src_basereg
    pixdeinterleave mask_bpp, mask_basereg
    pixdeinterleave dst_r_bpp, dst_r_basereg

    process_pixblock_head
.if cache_preload_flag != 0
    cache_preload 0, pixblock_size
    cache_preload_simple
.endif
    process_pixblock_tail
    pixinterleave dst_w_bpp, dst_w_basereg
.irp chunk_size, 16, 8, 4, 2, 1
.if pixblock_size > chunk_size
    tst         W, #chunk_size
    beq         1f
.if dst_aligned_flag != 0
    pixst_a     chunk_size, dst_w_bpp, dst_w_basereg, DST_W
.else
    pixst       chunk_size, dst_w_bpp, dst_w_basereg, DST_W
.endif
1:
.endif
.endr
2:
.endm

/*
 * Macro, which performs all the needed operations to switch to the next
 * scanline and start the next loop iteration unless all the scanlines
 * are already processed.
 */
.macro advance_to_next_scanline start_of_loop_label
.if regs_shortage
    ldrd        W, [sp] /* load W and H (width and height) from stack */
.else
    mov         W, ORIG_W
.endif
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
.if regs_shortage
    str         H, [sp, #4] /* save updated height to stack */
.endif
    bge         start_of_loop_label
.endm

/*
 * Registers are allocated in the following way by default:
 * d0, d1, d2, d3     - reserved for loading source pixel data
 * d4, d5, d6, d7     - reserved for loading destination pixel data
 * d24, d25, d26, d27 - reserved for loading mask pixel data
 * d28, d29, d30, d31 - final destination pixel data for writeback to memory
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

    pixman_asm_function fname

    push        {r4-r12, lr}        /* save all registers */

/*
 * Select prefetch type for this function. If prefetch distance is
 * set to 0 or one of the color formats is 24bpp, SIMPLE prefetch
 * has to be used instead of ADVANCED.
 */
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_DEFAULT
.if prefetch_distance == 0
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_NONE
.elseif (PREFETCH_TYPE_CURRENT > PREFETCH_TYPE_SIMPLE) && \
        ((src_bpp_ == 24) || (mask_bpp_ == 24) || (dst_w_bpp_ == 24))
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_SIMPLE
.endif

/*
 * Make some macro arguments globally visible and accessible
 * from other macros
 */
    .set src_bpp, src_bpp_
    .set mask_bpp, mask_bpp_
    .set dst_w_bpp, dst_w_bpp_
    .set pixblock_size, pixblock_size_
    .set dst_w_basereg, dst_w_basereg_
    .set dst_r_basereg, dst_r_basereg_
    .set src_basereg, src_basereg_
    .set mask_basereg, mask_basereg_

    .macro pixld_src x:vararg
        pixld x
    .endm
    .macro fetch_src_pixblock
        pixld_src   pixblock_size, src_bpp, \
                    (src_basereg - pixblock_size * src_bpp / 64), SRC
    .endm
/*
 * Assign symbolic names to registers
 */
    W           .req        r0      /* width (is updated during processing) */
    H           .req        r1      /* height (is updated during processing) */
    DST_W       .req        r2      /* destination buffer pointer for writes */
    DST_STRIDE  .req        r3      /* destination image stride */
    SRC         .req        r4      /* source buffer pointer */
    SRC_STRIDE  .req        r5      /* source image stride */
    DST_R       .req        r6      /* destination buffer pointer for reads */

    MASK        .req        r7      /* mask pointer */
    MASK_STRIDE .req        r8      /* mask stride */

    PF_CTL      .req        r9      /* combined lines counter and prefetch */
                                    /* distance increment counter */
    PF_X        .req        r10     /* pixel index in a scanline for current */
                                    /* pretetch position */
    PF_SRC      .req        r11     /* pointer to source scanline start */
                                    /* for prefetch purposes */
    PF_DST      .req        r12     /* pointer to destination scanline start */
                                    /* for prefetch purposes */
    PF_MASK     .req        r14     /* pointer to mask scanline start */
                                    /* for prefetch purposes */
/*
 * Check whether we have enough registers for all the local variables.
 * If we don't have enough registers, original width and height are
 * kept on top of stack (and 'regs_shortage' variable is set to indicate
 * this for the rest of code). Even if there are enough registers, the
 * allocation scheme may be a bit different depending on whether source
 * or mask is not used.
 */
.if (PREFETCH_TYPE_CURRENT < PREFETCH_TYPE_ADVANCED)
    ORIG_W      .req        r10     /* saved original width */
    DUMMY       .req        r12     /* temporary register */
    .set        regs_shortage, 0
.elseif mask_bpp == 0
    ORIG_W      .req        r7      /* saved original width */
    DUMMY       .req        r8      /* temporary register */
    .set        regs_shortage, 0
.elseif src_bpp == 0
    ORIG_W      .req        r4      /* saved original width */
    DUMMY       .req        r5      /* temporary register */
    .set        regs_shortage, 0
.else
    ORIG_W      .req        r1      /* saved original width */
    DUMMY       .req        r1      /* temporary register */
    .set        regs_shortage, 1
.endif

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

.if (((flags) & FLAG_DST_READWRITE) != 0)
    .set dst_r_bpp, dst_w_bpp
.else
    .set dst_r_bpp, 0
.endif
.if (((flags) & FLAG_DEINTERLEAVE_32BPP) != 0)
    .set DEINTERLEAVE_32BPP_ENABLED, 1
.else
    .set DEINTERLEAVE_32BPP_ENABLED, 0
.endif

.if prefetch_distance < 0 || prefetch_distance > 15
    .error "invalid prefetch distance (prefetch_distance)"
.endif

.if src_bpp > 0
    ldr         SRC, [sp, #40]
.endif
.if mask_bpp > 0
    ldr         MASK, [sp, #48]
.endif
    PF mov      PF_X, #0
.if src_bpp > 0
    ldr         SRC_STRIDE, [sp, #44]
.endif
.if mask_bpp > 0
    ldr         MASK_STRIDE, [sp, #52]
.endif
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
    PF mov      PF_SRC, SRC
    PF mov      PF_DST, DST_R
    PF mov      PF_MASK, MASK
    /* PF_CTL = prefetch_distance | ((h - 1) << 4) */
    PF mov      PF_CTL, H, lsl #4
    PF add      PF_CTL, #(prefetch_distance - 0x10)

    init
.if regs_shortage
    push        {r0, r1}
.endif
    subs        H, H, #1
.if regs_shortage
    str         H, [sp, #4] /* save updated height to stack */
.else
    mov         ORIG_W, W
.endif
    blt         9f
    cmp         W, #(pixblock_size * 2)
    blt         8f
/*
 * This is the start of the pipelined loop, which if optimized for
 * long scanlines
 */
0:
    ensure_destination_ptr_alignment process_pixblock_head, \
                                     process_pixblock_tail, \
                                     process_pixblock_tail_head

    /* Implement "head (tail_head) ... (tail_head) tail" loop pattern */
    pixld_a     pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    fetch_src_pixblock
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    PF add      PF_X, PF_X, #pixblock_size
    process_pixblock_head
    cache_preload 0, pixblock_size
    cache_preload_simple
    subs        W, W, #(pixblock_size * 2)
    blt         2f
1:
    process_pixblock_tail_head
    cache_preload_simple
    subs        W, W, #pixblock_size
    bge         1b
2:
    process_pixblock_tail
    pixst_a     pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W

    /* Process the remaining trailing pixels in the scanline */
    process_trailing_pixels 1, 1, \
                            process_pixblock_head, \
                            process_pixblock_tail, \
                            process_pixblock_tail_head
    advance_to_next_scanline 0b

.if regs_shortage
    pop         {r0, r1}
.endif
    cleanup
    pop         {r4-r12, pc}  /* exit */
/*
 * This is the start of the loop, designed to process images with small width
 * (less than pixblock_size * 2 pixels). In this case neither pipelining
 * nor prefetch are used.
 */
8:
    /* Process exactly pixblock_size pixels if needed */
    tst         W, #pixblock_size
    beq         1f
    pixld       pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    fetch_src_pixblock
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    process_pixblock_head
    process_pixblock_tail
    pixst       pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W
1:
    /* Process the remaining trailing pixels in the scanline */
    process_trailing_pixels 0, 0, \
                            process_pixblock_head, \
                            process_pixblock_tail, \
                            process_pixblock_tail_head
    advance_to_next_scanline 8b
9:
.if regs_shortage
    pop         {r0, r1}
.endif
    cleanup
    pop         {r4-r12, pc}  /* exit */

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
    .endfunc
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

    pixman_asm_function fname

    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_NONE
/*
 * Make some macro arguments globally visible and accessible
 * from other macros
 */
    .set src_bpp, src_bpp_
    .set mask_bpp, mask_bpp_
    .set dst_w_bpp, dst_w_bpp_
    .set pixblock_size, pixblock_size_
    .set dst_w_basereg, dst_w_basereg_
    .set dst_r_basereg, dst_r_basereg_
    .set src_basereg, src_basereg_
    .set mask_basereg, mask_basereg_

.if use_nearest_scaling != 0
    /*
     * Assign symbolic names to registers for nearest scaling
     */
    W           .req        r0
    DST_W       .req        r1
    SRC         .req        r2
    VX          .req        r3
    UNIT_X      .req        ip
    MASK        .req        lr
    TMP1        .req        r4
    TMP2        .req        r5
    DST_R       .req        r6
    SRC_WIDTH_FIXED .req        r7

    .macro pixld_src x:vararg
        pixld_s x
    .endm

    ldr         UNIT_X, [sp]
    push        {r4-r8, lr}
    ldr         SRC_WIDTH_FIXED, [sp, #(24 + 4)]
    .if mask_bpp != 0
    ldr         MASK, [sp, #(24 + 8)]
    .endif
.else
    /*
     * Assign symbolic names to registers
     */
    W           .req        r0      /* width (is updated during processing) */
    DST_W       .req        r1      /* destination buffer pointer for writes */
    SRC         .req        r2      /* source buffer pointer */
    DST_R       .req        ip      /* destination buffer pointer for reads */
    MASK        .req        r3      /* mask pointer */

    .macro pixld_src x:vararg
        pixld x
    .endm
.endif

.if (((flags) & FLAG_DST_READWRITE) != 0)
    .set dst_r_bpp, dst_w_bpp
.else
    .set dst_r_bpp, 0
.endif
.if (((flags) & FLAG_DEINTERLEAVE_32BPP) != 0)
    .set DEINTERLEAVE_32BPP_ENABLED, 1
.else
    .set DEINTERLEAVE_32BPP_ENABLED, 0
.endif

    .macro fetch_src_pixblock
        pixld_src   pixblock_size, src_bpp, \
                    (src_basereg - pixblock_size * src_bpp / 64), SRC
    .endm

    init
    mov         DST_R, DST_W

    cmp         W, #pixblock_size
    blt         8f

    ensure_destination_ptr_alignment process_pixblock_head, \
                                     process_pixblock_tail, \
                                     process_pixblock_tail_head

    subs        W, W, #pixblock_size
    blt         7f

    /* Implement "head (tail_head) ... (tail_head) tail" loop pattern */
    pixld_a     pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    fetch_src_pixblock
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    process_pixblock_head
    subs        W, W, #pixblock_size
    blt         2f
1:
    process_pixblock_tail_head
    subs        W, W, #pixblock_size
    bge         1b
2:
    process_pixblock_tail
    pixst_a     pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W
7:
    /* Process the remaining trailing pixels in the scanline (dst aligned) */
    process_trailing_pixels 0, 1, \
                            process_pixblock_head, \
                            process_pixblock_tail, \
                            process_pixblock_tail_head

    cleanup
.if use_nearest_scaling != 0
    pop         {r4-r8, pc}  /* exit */
.else
    bx          lr  /* exit */
.endif
8:
    /* Process the remaining trailing pixels in the scanline (dst unaligned) */
    process_trailing_pixels 0, 0, \
                            process_pixblock_head, \
                            process_pixblock_tail, \
                            process_pixblock_tail_head

    cleanup

.if use_nearest_scaling != 0
    pop         {r4-r8, pc}  /* exit */

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
    bx          lr  /* exit */

    .unreq      SRC
    .unreq      MASK
    .unreq      DST_R
    .unreq      DST_W
    .unreq      W
.endif

    .purgem     fetch_src_pixblock
    .purgem     pixld_src

    .endfunc
.endm

.macro generate_composite_function_single_scanline x:vararg
    generate_composite_function_scanline 0, x
.endm

.macro generate_composite_function_nearest_scanline x:vararg
    generate_composite_function_scanline 1, x
.endm

/* Default prologue/epilogue, nothing special needs to be done */

.macro default_init
.endm

.macro default_cleanup
.endm

/*
 * Prologue/epilogue variant which additionally saves/restores d8-d15
 * registers (they need to be saved/restored by callee according to ABI).
 * This is required if the code needs to use all the NEON registers.
 */

.macro default_init_need_all_regs
    vpush       {d8-d15}
.endm

.macro default_cleanup_need_all_regs
    vpop        {d8-d15}
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
    vshrn.u16   out_r, in,    #8
    vshrn.u16   out_g, in,    #3
    vsli.u16    in,    in,    #5
    vmov.u8     out_a, #255
    vsri.u8     out_r, out_r, #5
    vsri.u8     out_g, out_g, #6
    vshrn.u16   out_b, in,    #2
.endm

.macro convert_0565_to_x888 in, out_r, out_g, out_b
    vshrn.u16   out_r, in,    #8
    vshrn.u16   out_g, in,    #3
    vsli.u16    in,    in,    #5
    vsri.u8     out_r, out_r, #5
    vsri.u8     out_g, out_g, #6
    vshrn.u16   out_b, in,    #2
.endm

/*
 * Conversion from planar a8r8g8b8 format (with a, r, g, b color components
 * in 64-bit registers in_a, in_r, in_g, in_b respectively) into 8 r5g6b6
 * pixels packed in 128-bit register (out). Requires two temporary 128-bit
 * registers (tmp1, tmp2)
 */
.macro convert_8888_to_0565 in_r, in_g, in_b, out, tmp1, tmp2
    vshll.u8    tmp1, in_g, #8
    vshll.u8    out, in_r, #8
    vshll.u8    tmp2, in_b, #8
    vsri.u16    out, tmp1, #5
    vsri.u16    out, tmp2, #11
.endm

/*
 * Conversion of four r5g6b5 pixels (in) to four x8r8g8b8 pixels
 * returned in (out0, out1) registers pair. Requires one temporary
 * 64-bit register (tmp). 'out1' and 'in' may overlap, the original
 * value from 'in' is lost
 */
.macro convert_four_0565_to_x888_packed in, out0, out1, tmp
    vshl.u16    out0, in,   #5  /* G top 6 bits */
    vshl.u16    tmp,  in,   #11 /* B top 5 bits */
    vsri.u16    in,   in,   #5  /* R is ready in top bits */
    vsri.u16    out0, out0, #6  /* G is ready in top bits */
    vsri.u16    tmp,  tmp,  #5  /* B is ready in top bits */
    vshr.u16    out1, in,   #8  /* R is in place */
    vsri.u16    out0, tmp,  #8  /* G & B is in place */
    vzip.u16    out0, out1      /* everything is in place */
.endm
