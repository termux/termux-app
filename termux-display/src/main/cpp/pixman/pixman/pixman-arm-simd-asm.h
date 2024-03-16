/*
 * Copyright © 2012 Raspberry Pi Foundation
 * Copyright © 2012 RISC OS Open Ltd
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author:  Ben Avison (bavison@riscosopen.org)
 *
 */

/*
 * Because the alignment of pixel data to cachelines, and even the number of
 * cachelines per row can vary from row to row, and because of the need to
 * preload each scanline once and only once, this prefetch strategy treats
 * each row of pixels independently. When a pixel row is long enough, there
 * are three distinct phases of prefetch:
 * * an inner loop section, where each time a cacheline of data is
 *    processed, another cacheline is preloaded (the exact distance ahead is
 *    determined empirically using profiling results from lowlevel-blt-bench)
 * * a leading section, where enough cachelines are preloaded to ensure no
 *    cachelines escape being preloaded when the inner loop starts
 * * a trailing section, where a limited number (0 or more) of cachelines
 *    are preloaded to deal with data (if any) that hangs off the end of the
 *    last iteration of the inner loop, plus any trailing bytes that were not
 *    enough to make up one whole iteration of the inner loop
 * 
 * There are (in general) three distinct code paths, selected between
 * depending upon how long the pixel row is. If it is long enough that there
 * is at least one iteration of the inner loop (as described above) then
 * this is described as the "wide" case. If it is shorter than that, but
 * there are still enough bytes output that there is at least one 16-byte-
 * long, 16-byte-aligned write to the destination (the optimum type of
 * write), then this is the "medium" case. If it is not even this long, then
 * this is the "narrow" case, and there is no attempt to align writes to
 * 16-byte boundaries. In the "medium" and "narrow" cases, all the
 * cachelines containing data from the pixel row are prefetched up-front.
 */

/*
 * Determine whether we put the arguments on the stack for debugging.
 */
#undef DEBUG_PARAMS

/*
 * Bit flags for 'generate_composite_function' macro which are used
 * to tune generated functions behavior.
 */
.set FLAG_DST_WRITEONLY,         0
.set FLAG_DST_READWRITE,         1
.set FLAG_COND_EXEC,             0
.set FLAG_BRANCH_OVER,           2
.set FLAG_PROCESS_PRESERVES_PSR, 0
.set FLAG_PROCESS_CORRUPTS_PSR,  4
.set FLAG_PROCESS_DOESNT_STORE,  0
.set FLAG_PROCESS_DOES_STORE,    8 /* usually because it needs to conditionally skip it */
.set FLAG_NO_SPILL_LINE_VARS,        0
.set FLAG_SPILL_LINE_VARS_WIDE,      16
.set FLAG_SPILL_LINE_VARS_NON_WIDE,  32
.set FLAG_SPILL_LINE_VARS,           48
.set FLAG_PROCESS_CORRUPTS_SCRATCH,  0
.set FLAG_PROCESS_PRESERVES_SCRATCH, 64
.set FLAG_PROCESS_PRESERVES_WK0,     0
.set FLAG_PROCESS_CORRUPTS_WK0,      128 /* if possible, use the specified register(s) instead so WK0 can hold number of leading pixels */
.set FLAG_PRELOAD_DST,               0
.set FLAG_NO_PRELOAD_DST,            256

/*
 * Number of bytes by which to adjust preload offset of destination
 * buffer (allows preload instruction to be moved before the load(s))
 */
.set DST_PRELOAD_BIAS, 0

/*
 * Offset into stack where mask and source pointer/stride can be accessed.
 */
#ifdef DEBUG_PARAMS
.set ARGS_STACK_OFFSET,        (9*4+9*4)
#else
.set ARGS_STACK_OFFSET,        (9*4)
#endif

/*
 * Offset into stack where space allocated during init macro can be accessed.
 */
.set LOCALS_STACK_OFFSET,     0

/*
 * Constants for selecting preferable prefetch type.
 */
.set PREFETCH_TYPE_NONE,       0
.set PREFETCH_TYPE_STANDARD,   1

/*
 * Definitions of macros for load/store of pixel data.
 */

.macro pixldst op, cond=al, numbytes, reg0, reg1, reg2, reg3, base, unaligned=0
 .if numbytes == 16
  .if unaligned == 1
        op&r&cond    WK&reg0, [base], #4
        op&r&cond    WK&reg1, [base], #4
        op&r&cond    WK&reg2, [base], #4
        op&r&cond    WK&reg3, [base], #4
  .else
        op&m&cond&ia base!, {WK&reg0,WK&reg1,WK&reg2,WK&reg3}
  .endif
 .elseif numbytes == 8
  .if unaligned == 1
        op&r&cond    WK&reg0, [base], #4
        op&r&cond    WK&reg1, [base], #4
  .else
        op&m&cond&ia base!, {WK&reg0,WK&reg1}
  .endif
 .elseif numbytes == 4
        op&r&cond    WK&reg0, [base], #4
 .elseif numbytes == 2
        op&r&cond&h  WK&reg0, [base], #2
 .elseif numbytes == 1
        op&r&cond&b  WK&reg0, [base], #1
 .else
  .error "unsupported size: numbytes"
 .endif
.endm

.macro pixst_baseupdated cond, numbytes, reg0, reg1, reg2, reg3, base
 .if numbytes == 16
        stm&cond&db base, {WK&reg0,WK&reg1,WK&reg2,WK&reg3}
 .elseif numbytes == 8
        stm&cond&db base, {WK&reg0,WK&reg1}
 .elseif numbytes == 4
        str&cond    WK&reg0, [base, #-4]
 .elseif numbytes == 2
        str&cond&h  WK&reg0, [base, #-2]
 .elseif numbytes == 1
        str&cond&b  WK&reg0, [base, #-1]
 .else
  .error "unsupported size: numbytes"
 .endif
.endm

.macro pixld cond, numbytes, firstreg, base, unaligned
        pixldst ld, cond, numbytes, %(firstreg+0), %(firstreg+1), %(firstreg+2), %(firstreg+3), base, unaligned
.endm

.macro pixst cond, numbytes, firstreg, base
 .if (flags) & FLAG_DST_READWRITE
        pixst_baseupdated cond, numbytes, %(firstreg+0), %(firstreg+1), %(firstreg+2), %(firstreg+3), base
 .else
        pixldst st, cond, numbytes, %(firstreg+0), %(firstreg+1), %(firstreg+2), %(firstreg+3), base
 .endif
.endm

.macro PF a, x:vararg
 .if (PREFETCH_TYPE_CURRENT == PREFETCH_TYPE_STANDARD)
        a x
 .endif
.endm


.macro preload_leading_step1  bpp, ptr, base
/* If the destination is already 16-byte aligned, then we need to preload
 * between 0 and prefetch_distance (inclusive) cache lines ahead so there
 * are no gaps when the inner loop starts.
 */
 .if bpp > 0
        PF  bic,    ptr, base, #31
  .set OFFSET, 0
  .rept prefetch_distance+1
        PF  pld,    [ptr, #OFFSET]
   .set OFFSET, OFFSET+32
  .endr
 .endif
.endm

.macro preload_leading_step2  bpp, bpp_shift, ptr, base
/* However, if the destination is not 16-byte aligned, we may need to
 * preload more cache lines than that. The question we need to ask is:
 * are the bytes corresponding to the leading pixels more than the amount
 * by which the source pointer will be rounded down for preloading, and if
 * so, by how many cache lines? Effectively, we want to calculate
 *     leading_bytes = ((-dst)&15)*src_bpp/dst_bpp
 *     inner_loop_offset = (src+leading_bytes)&31
 *     extra_needed = leading_bytes - inner_loop_offset
 * and test if extra_needed is <= 0, <= 32, or > 32 (where > 32 is only
 * possible when there are 4 src bytes for every 1 dst byte).
 */
 .if bpp > 0
  .ifc base,DST
        /* The test can be simplified further when preloading the destination */
        PF  tst,    base, #16
        PF  beq,    61f
  .else
   .if bpp/dst_w_bpp == 4
        PF  add,    SCRATCH, base, WK0, lsl #bpp_shift-dst_bpp_shift
        PF  and,    SCRATCH, SCRATCH, #31
        PF  rsb,    SCRATCH, SCRATCH, WK0, lsl #bpp_shift-dst_bpp_shift
        PF  sub,    SCRATCH, SCRATCH, #1        /* so now ranges are -16..-1 / 0..31 / 32..63 */
        PF  movs,   SCRATCH, SCRATCH, lsl #32-6 /* so this sets         NC   /  nc   /   Nc   */
        PF  bcs,    61f
        PF  bpl,    60f
        PF  pld,    [ptr, #32*(prefetch_distance+2)]
   .else
        PF  mov,    SCRATCH, base, lsl #32-5
        PF  add,    SCRATCH, SCRATCH, WK0, lsl #32-5+bpp_shift-dst_bpp_shift
        PF  rsbs,   SCRATCH, SCRATCH, WK0, lsl #32-5+bpp_shift-dst_bpp_shift
        PF  bls,    61f
   .endif
  .endif
60:     PF  pld,    [ptr, #32*(prefetch_distance+1)]
61:
 .endif
.endm

#define IS_END_OF_GROUP(INDEX,SIZE) ((SIZE) < 2 || ((INDEX) & ~((INDEX)+1)) & ((SIZE)/2))
.macro preload_middle   bpp, base, scratch_holds_offset
 .if bpp > 0
        /* prefetch distance = 256/bpp, stm distance = 128/dst_w_bpp */
  .if IS_END_OF_GROUP(SUBBLOCK,256/128*dst_w_bpp/bpp)
   .if scratch_holds_offset
        PF  pld,    [base, SCRATCH]
   .else
        PF  bic,    SCRATCH, base, #31
        PF  pld,    [SCRATCH, #32*prefetch_distance]
   .endif
  .endif
 .endif
.endm

.macro preload_trailing  bpp, bpp_shift, base
 .if bpp > 0
  .if bpp*pix_per_block > 256
        /* Calculations are more complex if more than one fetch per block */
        PF  and,    WK1, base, #31
        PF  add,    WK1, WK1, WK0, lsl #bpp_shift
        PF  add,    WK1, WK1, #32*(bpp*pix_per_block/256-1)*(prefetch_distance+1)
        PF  bic,    SCRATCH, base, #31
80:     PF  pld,    [SCRATCH, #32*(prefetch_distance+1)]
        PF  add,    SCRATCH, SCRATCH, #32
        PF  subs,   WK1, WK1, #32
        PF  bhi,    80b
  .else
        /* If exactly one fetch per block, then we need either 0, 1 or 2 extra preloads */
        PF  mov,    SCRATCH, base, lsl #32-5
        PF  adds,   SCRATCH, SCRATCH, X, lsl #32-5+bpp_shift
        PF  adceqs, SCRATCH, SCRATCH, #0
        /* The instruction above has two effects: ensures Z is only
         * set if C was clear (so Z indicates that both shifted quantities
         * were 0), and clears C if Z was set (so C indicates that the sum
         * of the shifted quantities was greater and not equal to 32) */
        PF  beq,    82f
        PF  bic,    SCRATCH, base, #31
        PF  bcc,    81f
        PF  pld,    [SCRATCH, #32*(prefetch_distance+2)]
81:     PF  pld,    [SCRATCH, #32*(prefetch_distance+1)]
82:
  .endif
 .endif
.endm


.macro preload_line    narrow_case, bpp, bpp_shift, base
/* "narrow_case" - just means that the macro was invoked from the "narrow"
 *    code path rather than the "medium" one - because in the narrow case,
 *    the row of pixels is known to output no more than 30 bytes, then
 *    (assuming the source pixels are no wider than the the destination
 *    pixels) they cannot possibly straddle more than 2 32-byte cachelines,
 *    meaning there's no need for a loop.
 * "bpp" - number of bits per pixel in the channel (source, mask or
 *    destination) that's being preloaded, or 0 if this channel is not used
 *    for reading
 * "bpp_shift" - log2 of ("bpp"/8) (except if "bpp"=0 of course)
 * "base" - base address register of channel to preload (SRC, MASK or DST)
 */
 .if bpp > 0
  .if narrow_case && (bpp <= dst_w_bpp)
        /* In these cases, each line for each channel is in either 1 or 2 cache lines */
        PF  bic,    WK0, base, #31
        PF  pld,    [WK0]
        PF  add,    WK1, base, X, LSL #bpp_shift
        PF  sub,    WK1, WK1, #1
        PF  bic,    WK1, WK1, #31
        PF  cmp,    WK1, WK0
        PF  beq,    90f
        PF  pld,    [WK1]
90:
  .else
        PF  bic,    WK0, base, #31
        PF  pld,    [WK0]
        PF  add,    WK1, base, X, lsl #bpp_shift
        PF  sub,    WK1, WK1, #1
        PF  bic,    WK1, WK1, #31
        PF  cmp,    WK1, WK0
        PF  beq,    92f
91:     PF  add,    WK0, WK0, #32
        PF  cmp,    WK0, WK1
        PF  pld,    [WK0]
        PF  bne,    91b
92:
  .endif
 .endif
.endm


.macro conditional_process1_helper  cond, process_head, process_tail, numbytes, firstreg, unaligned_src, unaligned_mask, decrementx
        process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, 0
 .if decrementx
        sub&cond X, X, #8*numbytes/dst_w_bpp
 .endif
        process_tail  cond, numbytes, firstreg
 .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   cond, numbytes, firstreg, DST
 .endif
.endm

.macro conditional_process1  cond, process_head, process_tail, numbytes, firstreg, unaligned_src, unaligned_mask, decrementx
 .if (flags) & FLAG_BRANCH_OVER
  .ifc cond,mi
        bpl     100f
  .endif
  .ifc cond,cs
        bcc     100f
  .endif
  .ifc cond,ne
        beq     100f
  .endif
        conditional_process1_helper  , process_head, process_tail, numbytes, firstreg, unaligned_src, unaligned_mask, decrementx
100:
 .else
        conditional_process1_helper  cond, process_head, process_tail, numbytes, firstreg, unaligned_src, unaligned_mask, decrementx
 .endif
.endm

.macro conditional_process2  test, cond1, cond2, process_head, process_tail, numbytes1, numbytes2, firstreg1, firstreg2, unaligned_src, unaligned_mask, decrementx
 .if (flags) & (FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE)
        /* Can't interleave reads and writes */
        test
        conditional_process1  cond1, process_head, process_tail, numbytes1, firstreg1, unaligned_src, unaligned_mask, decrementx
  .if (flags) & FLAG_PROCESS_CORRUPTS_PSR
        test
  .endif
        conditional_process1  cond2, process_head, process_tail, numbytes2, firstreg2, unaligned_src, unaligned_mask, decrementx
 .else
        /* Can interleave reads and writes for better scheduling */
        test
        process_head  cond1, numbytes1, firstreg1, unaligned_src, unaligned_mask, 0
        process_head  cond2, numbytes2, firstreg2, unaligned_src, unaligned_mask, 0
  .if decrementx
        sub&cond1 X, X, #8*numbytes1/dst_w_bpp
        sub&cond2 X, X, #8*numbytes2/dst_w_bpp
  .endif
        process_tail  cond1, numbytes1, firstreg1
        process_tail  cond2, numbytes2, firstreg2
        pixst   cond1, numbytes1, firstreg1, DST
        pixst   cond2, numbytes2, firstreg2, DST
 .endif
.endm


.macro test_bits_1_0_ptr
 .if (flags) & FLAG_PROCESS_CORRUPTS_WK0
        movs    SCRATCH, X, lsl #32-1  /* C,N = bits 1,0 of DST */
 .else
        movs    SCRATCH, WK0, lsl #32-1  /* C,N = bits 1,0 of DST */
 .endif
.endm

.macro test_bits_3_2_ptr
 .if (flags) & FLAG_PROCESS_CORRUPTS_WK0
        movs    SCRATCH, X, lsl #32-3  /* C,N = bits 3, 2 of DST */
 .else
        movs    SCRATCH, WK0, lsl #32-3  /* C,N = bits 3, 2 of DST */
 .endif
.endm

.macro leading_15bytes  process_head, process_tail
        /* On entry, WK0 bits 0-3 = number of bytes until destination is 16-byte aligned */
 .set DECREMENT_X, 1
 .if (flags) & FLAG_PROCESS_CORRUPTS_WK0
  .set DECREMENT_X, 0
        sub     X, X, WK0, lsr #dst_bpp_shift
        str     X, [sp, #LINE_SAVED_REG_COUNT*4]
        mov     X, WK0
 .endif
        /* Use unaligned loads in all cases for simplicity */
 .if dst_w_bpp == 8
        conditional_process2  test_bits_1_0_ptr, mi, cs, process_head, process_tail, 1, 2, 1, 2, 1, 1, DECREMENT_X
 .elseif dst_w_bpp == 16
        test_bits_1_0_ptr
        conditional_process1  cs, process_head, process_tail, 2, 2, 1, 1, DECREMENT_X
 .endif
        conditional_process2  test_bits_3_2_ptr, mi, cs, process_head, process_tail, 4, 8, 1, 2, 1, 1, DECREMENT_X
 .if (flags) & FLAG_PROCESS_CORRUPTS_WK0
        ldr     X, [sp, #LINE_SAVED_REG_COUNT*4]
 .endif
.endm

.macro test_bits_3_2_pix
        movs    SCRATCH, X, lsl #dst_bpp_shift+32-3
.endm

.macro test_bits_1_0_pix
 .if dst_w_bpp == 8
        movs    SCRATCH, X, lsl #dst_bpp_shift+32-1
 .else
        movs    SCRATCH, X, lsr #1
 .endif
.endm

.macro trailing_15bytes  process_head, process_tail, unaligned_src, unaligned_mask
        conditional_process2  test_bits_3_2_pix, cs, mi, process_head, process_tail, 8, 4, 0, 2, unaligned_src, unaligned_mask, 0
 .if dst_w_bpp == 16
        test_bits_1_0_pix
        conditional_process1  cs, process_head, process_tail, 2, 0, unaligned_src, unaligned_mask, 0
 .elseif dst_w_bpp == 8
        conditional_process2  test_bits_1_0_pix, cs, mi, process_head, process_tail, 2, 1, 0, 1, unaligned_src, unaligned_mask, 0
 .endif
.endm


.macro wide_case_inner_loop  process_head, process_tail, unaligned_src, unaligned_mask, dst_alignment
110:
 .set SUBBLOCK, 0 /* this is a count of STMs; there can be up to 8 STMs per block */
 .rept pix_per_block*dst_w_bpp/128
        process_head  , 16, 0, unaligned_src, unaligned_mask, 1
  .if (src_bpp > 0) && (mask_bpp == 0) && ((flags) & FLAG_PROCESS_PRESERVES_SCRATCH)
        preload_middle  src_bpp, SRC, 1
  .elseif (src_bpp == 0) && (mask_bpp > 0) && ((flags) & FLAG_PROCESS_PRESERVES_SCRATCH)
        preload_middle  mask_bpp, MASK, 1
  .else
        preload_middle  src_bpp, SRC, 0
        preload_middle  mask_bpp, MASK, 0
  .endif
  .if (dst_r_bpp > 0) && ((SUBBLOCK % 2) == 0) && (((flags) & FLAG_NO_PRELOAD_DST) == 0)
        /* Because we know that writes are 16-byte aligned, it's relatively easy to ensure that
         * destination prefetches are 32-byte aligned. It's also the easiest channel to offset
         * preloads for, to achieve staggered prefetches for multiple channels, because there are
         * always two STMs per prefetch, so there is always an opposite STM on which to put the
         * preload. Note, no need to BIC the base register here */
        PF  pld,    [DST, #32*prefetch_distance - dst_alignment]
  .endif
        process_tail  , 16, 0
  .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   , 16, 0, DST
  .endif
  .set SUBBLOCK, SUBBLOCK+1
 .endr
        subs    X, X, #pix_per_block
        bhs     110b
.endm

.macro wide_case_inner_loop_and_trailing_pixels  process_head, process_tail, process_inner_loop, exit_label, unaligned_src, unaligned_mask
        /* Destination now 16-byte aligned; we have at least one block before we have to stop preloading */
 .if dst_r_bpp > 0
        tst     DST, #16
        bne     111f
        process_inner_loop  process_head, process_tail, unaligned_src, unaligned_mask, 16 + DST_PRELOAD_BIAS
        b       112f
111:
 .endif
        process_inner_loop  process_head, process_tail, unaligned_src, unaligned_mask, 0 + DST_PRELOAD_BIAS
112:
        /* Just before the final (prefetch_distance+1) 32-byte blocks, deal with final preloads */
 .if (src_bpp*pix_per_block > 256) || (mask_bpp*pix_per_block > 256) || (dst_r_bpp*pix_per_block > 256)
        PF  and,    WK0, X, #pix_per_block-1
 .endif
        preload_trailing  src_bpp, src_bpp_shift, SRC
        preload_trailing  mask_bpp, mask_bpp_shift, MASK
 .if ((flags) & FLAG_NO_PRELOAD_DST) == 0
        preload_trailing  dst_r_bpp, dst_bpp_shift, DST
 .endif
        add     X, X, #(prefetch_distance+2)*pix_per_block - 128/dst_w_bpp
        /* The remainder of the line is handled identically to the medium case */
        medium_case_inner_loop_and_trailing_pixels  process_head, process_tail,, exit_label, unaligned_src, unaligned_mask
.endm

.macro medium_case_inner_loop_and_trailing_pixels  process_head, process_tail, unused, exit_label, unaligned_src, unaligned_mask
120:
        process_head  , 16, 0, unaligned_src, unaligned_mask, 0
        process_tail  , 16, 0
 .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   , 16, 0, DST
 .endif
        subs    X, X, #128/dst_w_bpp
        bhs     120b
        /* Trailing pixels */
        tst     X, #128/dst_w_bpp - 1
        beq     exit_label
        trailing_15bytes  process_head, process_tail, unaligned_src, unaligned_mask
.endm

.macro narrow_case_inner_loop_and_trailing_pixels  process_head, process_tail, unused, exit_label, unaligned_src, unaligned_mask
        tst     X, #16*8/dst_w_bpp
        conditional_process1  ne, process_head, process_tail, 16, 0, unaligned_src, unaligned_mask, 0
        /* Trailing pixels */
        /* In narrow case, it's relatively unlikely to be aligned, so let's do without a branch here */
        trailing_15bytes  process_head, process_tail, unaligned_src, unaligned_mask
.endm

.macro switch_on_alignment  action, process_head, process_tail, process_inner_loop, exit_label
 /* Note that if we're reading the destination, it's already guaranteed to be aligned at this point */
 .if mask_bpp == 8 || mask_bpp == 16
        tst     MASK, #3
        bne     141f
 .endif
  .if src_bpp == 8 || src_bpp == 16
        tst     SRC, #3
        bne     140f
  .endif
        action  process_head, process_tail, process_inner_loop, exit_label, 0, 0
  .if src_bpp == 8 || src_bpp == 16
        b       exit_label
140:
        action  process_head, process_tail, process_inner_loop, exit_label, 1, 0
  .endif
 .if mask_bpp == 8 || mask_bpp == 16
        b       exit_label
141:
  .if src_bpp == 8 || src_bpp == 16
        tst     SRC, #3
        bne     142f
  .endif
        action  process_head, process_tail, process_inner_loop, exit_label, 0, 1
  .if src_bpp == 8 || src_bpp == 16
        b       exit_label
142:
        action  process_head, process_tail, process_inner_loop, exit_label, 1, 1
  .endif
 .endif
.endm


.macro end_of_line      restore_x, vars_spilled, loop_label, last_one
 .if vars_spilled
        /* Sadly, GAS doesn't seem have an equivalent of the DCI directive? */
        /* This is ldmia sp,{} */
        .word   0xE89D0000 | LINE_SAVED_REGS
 .endif
        subs    Y, Y, #1
 .if vars_spilled
  .if (LINE_SAVED_REGS) & (1<<1)
        str     Y, [sp]
  .endif
 .endif
        add     DST, DST, STRIDE_D
 .if src_bpp > 0
        add     SRC, SRC, STRIDE_S
 .endif
 .if mask_bpp > 0
        add     MASK, MASK, STRIDE_M
 .endif
 .if restore_x
        mov     X, ORIG_W
 .endif
        bhs     loop_label
 .ifc "last_one",""
  .if vars_spilled
        b       197f
  .else
        b       198f
  .endif
 .else
  .if (!vars_spilled) && ((flags) & FLAG_SPILL_LINE_VARS)
        b       198f
  .endif
 .endif
.endm


.macro generate_composite_function fname, \
                                   src_bpp_, \
                                   mask_bpp_, \
                                   dst_w_bpp_, \
                                   flags_, \
                                   prefetch_distance_, \
                                   init, \
                                   newline, \
                                   cleanup, \
                                   process_head, \
                                   process_tail, \
                                   process_inner_loop

    pixman_asm_function fname

/*
 * Make some macro arguments globally visible and accessible
 * from other macros
 */
 .set src_bpp, src_bpp_
 .set mask_bpp, mask_bpp_
 .set dst_w_bpp, dst_w_bpp_
 .set flags, flags_
 .set prefetch_distance, prefetch_distance_

/*
 * Select prefetch type for this function.
 */
 .if prefetch_distance == 0
  .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_NONE
 .else
  .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_STANDARD
 .endif

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

 .set pix_per_block, 16*8/dst_w_bpp
 .if src_bpp != 0
  .if 32*8/src_bpp > pix_per_block
   .set pix_per_block, 32*8/src_bpp
  .endif
 .endif
 .if mask_bpp != 0
  .if 32*8/mask_bpp > pix_per_block
   .set pix_per_block, 32*8/mask_bpp
  .endif
 .endif
 .if dst_r_bpp != 0
  .if 32*8/dst_r_bpp > pix_per_block
   .set pix_per_block, 32*8/dst_r_bpp
  .endif
 .endif

/* The standard entry conditions set up by pixman-arm-common.h are:
 * r0 = width (pixels)
 * r1 = height (rows)
 * r2 = pointer to top-left pixel of destination
 * r3 = destination stride (pixels)
 * [sp] = source pixel value, or pointer to top-left pixel of source
 * [sp,#4] = 0 or source stride (pixels)
 * The following arguments are unused for non-mask operations
 * [sp,#8] = mask pixel value, or pointer to top-left pixel of mask
 * [sp,#12] = 0 or mask stride (pixels)
 */

/*
 * Assign symbolic names to registers
 */
    X           .req    r0  /* pixels to go on this line */
    Y           .req    r1  /* lines to go */
    DST         .req    r2  /* destination pixel pointer */
    STRIDE_D    .req    r3  /* destination stride (bytes, minus width) */
    SRC         .req    r4  /* source pixel pointer */
    STRIDE_S    .req    r5  /* source stride (bytes, minus width) */
    MASK        .req    r6  /* mask pixel pointer (if applicable) */
    STRIDE_M    .req    r7  /* mask stride (bytes, minus width) */
    WK0         .req    r8  /* pixel data registers */
    WK1         .req    r9
    WK2         .req    r10
    WK3         .req    r11
    SCRATCH     .req    r12
    ORIG_W      .req    r14 /* width (pixels) */

        push    {r4-r11, lr}        /* save all registers */

        subs    Y, Y, #1
        blo     199f

#ifdef DEBUG_PARAMS
        sub     sp, sp, #9*4
#endif

 .if src_bpp > 0
        ldr     SRC, [sp, #ARGS_STACK_OFFSET]
        ldr     STRIDE_S, [sp, #ARGS_STACK_OFFSET+4]
 .endif
 .if mask_bpp > 0
        ldr     MASK, [sp, #ARGS_STACK_OFFSET+8]
        ldr     STRIDE_M, [sp, #ARGS_STACK_OFFSET+12]
 .endif
        
#ifdef DEBUG_PARAMS
        add     Y, Y, #1
        stmia   sp, {r0-r7,pc}
        sub     Y, Y, #1
#endif

        init

 .if (flags) & FLAG_PROCESS_CORRUPTS_WK0
        /* Reserve a word in which to store X during leading pixels */
        sub     sp, sp, #4
  .set ARGS_STACK_OFFSET, ARGS_STACK_OFFSET+4
  .set LOCALS_STACK_OFFSET, LOCALS_STACK_OFFSET+4
 .endif
        
        lsl     STRIDE_D, #dst_bpp_shift /* stride in bytes */
        sub     STRIDE_D, STRIDE_D, X, lsl #dst_bpp_shift
 .if src_bpp > 0
        lsl     STRIDE_S, #src_bpp_shift
        sub     STRIDE_S, STRIDE_S, X, lsl #src_bpp_shift
 .endif
 .if mask_bpp > 0
        lsl     STRIDE_M, #mask_bpp_shift
        sub     STRIDE_M, STRIDE_M, X, lsl #mask_bpp_shift
 .endif
 
        /* Are we not even wide enough to have one 16-byte aligned 16-byte block write? */
        cmp     X, #2*16*8/dst_w_bpp - 1
        blo     170f
 .if src_bpp || mask_bpp || dst_r_bpp /* Wide and medium cases are the same for fill */
        /* To preload ahead on the current line, we need at least (prefetch_distance+2) 32-byte blocks on all prefetch channels */
        cmp     X, #(prefetch_distance+3)*pix_per_block - 1
        blo     160f

        /* Wide case */
        /* Adjust X so that the decrement instruction can also test for
         * inner loop termination. We want it to stop when there are
         * (prefetch_distance+1) complete blocks to go. */
        sub     X, X, #(prefetch_distance+2)*pix_per_block
        mov     ORIG_W, X
  .if (flags) & FLAG_SPILL_LINE_VARS_WIDE
        /* This is stmdb sp!,{} */
        .word   0xE92D0000 | LINE_SAVED_REGS
   .set ARGS_STACK_OFFSET, ARGS_STACK_OFFSET + LINE_SAVED_REG_COUNT*4
   .set LOCALS_STACK_OFFSET, LOCALS_STACK_OFFSET + LINE_SAVED_REG_COUNT*4
  .endif
151:    /* New line */
        newline
        preload_leading_step1  src_bpp, WK1, SRC
        preload_leading_step1  mask_bpp, WK2, MASK
  .if ((flags) & FLAG_NO_PRELOAD_DST) == 0
        preload_leading_step1  dst_r_bpp, WK3, DST
  .endif
        
        ands    WK0, DST, #15
        beq     154f
        rsb     WK0, WK0, #16 /* number of leading bytes until destination aligned */

        preload_leading_step2  src_bpp, src_bpp_shift, WK1, SRC
        preload_leading_step2  mask_bpp, mask_bpp_shift, WK2, MASK
  .if ((flags) & FLAG_NO_PRELOAD_DST) == 0
        preload_leading_step2  dst_r_bpp, dst_bpp_shift, WK3, DST
  .endif

        leading_15bytes  process_head, process_tail
        
154:    /* Destination now 16-byte aligned; we have at least one prefetch on each channel as well as at least one 16-byte output block */
  .if (src_bpp > 0) && (mask_bpp == 0) && ((flags) & FLAG_PROCESS_PRESERVES_SCRATCH)
        and     SCRATCH, SRC, #31
        rsb     SCRATCH, SCRATCH, #32*prefetch_distance
  .elseif (src_bpp == 0) && (mask_bpp > 0) && ((flags) & FLAG_PROCESS_PRESERVES_SCRATCH)
        and     SCRATCH, MASK, #31
        rsb     SCRATCH, SCRATCH, #32*prefetch_distance
  .endif
  .ifc "process_inner_loop",""
        switch_on_alignment  wide_case_inner_loop_and_trailing_pixels, process_head, process_tail, wide_case_inner_loop, 157f
  .else
        switch_on_alignment  wide_case_inner_loop_and_trailing_pixels, process_head, process_tail, process_inner_loop, 157f
  .endif

157:    /* Check for another line */
        end_of_line 1, %((flags) & FLAG_SPILL_LINE_VARS_WIDE), 151b
  .if (flags) & FLAG_SPILL_LINE_VARS_WIDE
   .set ARGS_STACK_OFFSET, ARGS_STACK_OFFSET - LINE_SAVED_REG_COUNT*4
   .set LOCALS_STACK_OFFSET, LOCALS_STACK_OFFSET - LINE_SAVED_REG_COUNT*4
  .endif
 .endif

 .ltorg

160:    /* Medium case */
        mov     ORIG_W, X
 .if (flags) & FLAG_SPILL_LINE_VARS_NON_WIDE
        /* This is stmdb sp!,{} */
        .word   0xE92D0000 | LINE_SAVED_REGS
  .set ARGS_STACK_OFFSET, ARGS_STACK_OFFSET + LINE_SAVED_REG_COUNT*4
  .set LOCALS_STACK_OFFSET, LOCALS_STACK_OFFSET + LINE_SAVED_REG_COUNT*4
 .endif
161:    /* New line */
        newline
        preload_line 0, src_bpp, src_bpp_shift, SRC  /* in: X, corrupts: WK0-WK1 */
        preload_line 0, mask_bpp, mask_bpp_shift, MASK
 .if ((flags) & FLAG_NO_PRELOAD_DST) == 0
        preload_line 0, dst_r_bpp, dst_bpp_shift, DST
 .endif
        
        sub     X, X, #128/dst_w_bpp     /* simplifies inner loop termination */
        ands    WK0, DST, #15
        beq     164f
        rsb     WK0, WK0, #16 /* number of leading bytes until destination aligned */
        
        leading_15bytes  process_head, process_tail
        
164:    /* Destination now 16-byte aligned; we have at least one 16-byte output block */
        switch_on_alignment  medium_case_inner_loop_and_trailing_pixels, process_head, process_tail,, 167f
        
167:    /* Check for another line */
        end_of_line 1, %((flags) & FLAG_SPILL_LINE_VARS_NON_WIDE), 161b

 .ltorg

170:    /* Narrow case, less than 31 bytes, so no guarantee of at least one 16-byte block */
 .if dst_w_bpp < 32
        mov     ORIG_W, X
 .endif
 .if (flags) & FLAG_SPILL_LINE_VARS_NON_WIDE
        /* This is stmdb sp!,{} */
        .word   0xE92D0000 | LINE_SAVED_REGS
 .endif
171:    /* New line */
        newline
        preload_line 1, src_bpp, src_bpp_shift, SRC  /* in: X, corrupts: WK0-WK1 */
        preload_line 1, mask_bpp, mask_bpp_shift, MASK
 .if ((flags) & FLAG_NO_PRELOAD_DST) == 0
        preload_line 1, dst_r_bpp, dst_bpp_shift, DST
 .endif
        
 .if dst_w_bpp == 8
        tst     DST, #3
        beq     174f
172:    subs    X, X, #1
        blo     177f
        process_head  , 1, 0, 1, 1, 0
        process_tail  , 1, 0
  .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   , 1, 0, DST
  .endif
        tst     DST, #3
        bne     172b
 .elseif dst_w_bpp == 16
        tst     DST, #2
        beq     174f
        subs    X, X, #1
        blo     177f
        process_head  , 2, 0, 1, 1, 0
        process_tail  , 2, 0
  .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   , 2, 0, DST
  .endif
 .endif

174:    /* Destination now 4-byte aligned; we have 0 or more output bytes to go */
        switch_on_alignment  narrow_case_inner_loop_and_trailing_pixels, process_head, process_tail,, 177f

177:    /* Check for another line */
        end_of_line %(dst_w_bpp < 32), %((flags) & FLAG_SPILL_LINE_VARS_NON_WIDE), 171b, last_one
 .if (flags) & FLAG_SPILL_LINE_VARS_NON_WIDE
  .set ARGS_STACK_OFFSET, ARGS_STACK_OFFSET - LINE_SAVED_REG_COUNT*4
  .set LOCALS_STACK_OFFSET, LOCALS_STACK_OFFSET - LINE_SAVED_REG_COUNT*4
 .endif

197:
 .if (flags) & FLAG_SPILL_LINE_VARS
        add     sp, sp, #LINE_SAVED_REG_COUNT*4
 .endif
198:
 .if (flags) & FLAG_PROCESS_CORRUPTS_WK0
  .set ARGS_STACK_OFFSET, ARGS_STACK_OFFSET-4
  .set LOCALS_STACK_OFFSET, LOCALS_STACK_OFFSET-4
        add     sp, sp, #4
 .endif

        cleanup

#ifdef DEBUG_PARAMS
        add     sp, sp, #9*4 /* junk the debug copy of arguments */
#endif
199:
        pop     {r4-r11, pc}  /* exit */

 .ltorg

    .unreq  X
    .unreq  Y
    .unreq  DST
    .unreq  STRIDE_D
    .unreq  SRC
    .unreq  STRIDE_S
    .unreq  MASK
    .unreq  STRIDE_M
    .unreq  WK0
    .unreq  WK1
    .unreq  WK2
    .unreq  WK3
    .unreq  SCRATCH
    .unreq  ORIG_W
    .endfunc
.endm

.macro line_saved_regs  x:vararg
 .set LINE_SAVED_REGS, 0
 .set LINE_SAVED_REG_COUNT, 0
 .irp SAVED_REG,x
  .ifc "SAVED_REG","Y"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<1)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
  .ifc "SAVED_REG","STRIDE_D"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<3)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
  .ifc "SAVED_REG","STRIDE_S"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<5)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
  .ifc "SAVED_REG","STRIDE_M"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<7)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
  .ifc "SAVED_REG","ORIG_W"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<14)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
 .endr
.endm

.macro nop_macro x:vararg
.endm
