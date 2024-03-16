/*
 * Copyright © 2008 Mozilla Corporation
 * Copyright © 2010 Nokia Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Mozilla Corporation not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Mozilla Corporation makes no
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
 * Author:  Jeff Muizelaar (jeff@infidigm.net)
 *
 */

/* Prevent the stack from becoming executable */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

	.text
	.arch armv6
	.object_arch armv4
	.arm
	.altmacro
	.p2align 2

#include "pixman-arm-asm.h"

/*
 * Note: This code is only using armv5te instructions (not even armv6),
 *       but is scheduled for ARM Cortex-A8 pipeline. So it might need to
 *       be split into a few variants, tuned for each microarchitecture.
 *
 * TODO: In order to get good performance on ARM9/ARM11 cores (which don't
 * have efficient write combining), it needs to be changed to use 16-byte
 * aligned writes using STM instruction.
 *
 * Nearest scanline scaler macro template uses the following arguments:
 *  fname                     - name of the function to generate
 *  bpp_shift                 - (1 << bpp_shift) is the size of pixel in bytes
 *  t                         - type suffix for LDR/STR instructions
 *  prefetch_distance         - prefetch in the source image by that many
 *                              pixels ahead
 *  prefetch_braking_distance - stop prefetching when that many pixels are
 *                              remaining before the end of scanline
 */

.macro generate_nearest_scanline_func fname, bpp_shift, t,      \
                                      prefetch_distance,        \
                                      prefetch_braking_distance

pixman_asm_function fname
	W		.req	r0
	DST		.req	r1
	SRC		.req	r2
	VX		.req	r3
	UNIT_X		.req	ip
	TMP1		.req	r4
	TMP2		.req	r5
	VXMASK		.req	r6
	PF_OFFS		.req	r7
	SRC_WIDTH_FIXED	.req	r8

	ldr	UNIT_X, [sp]
	push	{r4, r5, r6, r7, r8, r10}
	mvn	VXMASK, #((1 << bpp_shift) - 1)
	ldr	SRC_WIDTH_FIXED, [sp, #28]

	/* define helper macro */
	.macro	scale_2_pixels
		ldr&t	TMP1, [SRC, TMP1]
		and	TMP2, VXMASK, VX, asr #(16 - bpp_shift)
		adds	VX, VX, UNIT_X
		str&t	TMP1, [DST], #(1 << bpp_shift)
9:		subpls	VX, VX, SRC_WIDTH_FIXED
		bpl	9b

		ldr&t	TMP2, [SRC, TMP2]
		and	TMP1, VXMASK, VX, asr #(16 - bpp_shift)
		adds	VX, VX, UNIT_X
		str&t	TMP2, [DST], #(1 << bpp_shift)
9:		subpls	VX, VX, SRC_WIDTH_FIXED
		bpl	9b
	.endm

	/* now do the scaling */
	and	TMP1, VXMASK, VX, asr #(16 - bpp_shift)
	adds	VX, VX, UNIT_X
9:	subpls	VX, VX, SRC_WIDTH_FIXED
	bpl	9b
	subs	W, W, #(8 + prefetch_braking_distance)
	blt	2f
	/* calculate prefetch offset */
	mov	PF_OFFS, #prefetch_distance
	mla	PF_OFFS, UNIT_X, PF_OFFS, VX
1:	/* main loop, process 8 pixels per iteration with prefetch */
	pld	[SRC, PF_OFFS, asr #(16 - bpp_shift)]
	add	PF_OFFS, UNIT_X, lsl #3
	scale_2_pixels
	scale_2_pixels
	scale_2_pixels
	scale_2_pixels
	subs	W, W, #8
	bge	1b
2:
	subs	W, W, #(4 - 8 - prefetch_braking_distance)
	blt	2f
1:	/* process the remaining pixels */
	scale_2_pixels
	scale_2_pixels
	subs	W, W, #4
	bge	1b
2:
	tst	W, #2
	beq	2f
	scale_2_pixels
2:
	tst	W, #1
	ldrne&t	TMP1, [SRC, TMP1]
	strne&t	TMP1, [DST]
	/* cleanup helper macro */
	.purgem	scale_2_pixels
	.unreq	DST
	.unreq	SRC
	.unreq	W
	.unreq	VX
	.unreq	UNIT_X
	.unreq	TMP1
	.unreq	TMP2
	.unreq	VXMASK
	.unreq	PF_OFFS
	.unreq  SRC_WIDTH_FIXED
	/* return */
	pop	{r4, r5, r6, r7, r8, r10}
	bx	lr
.endfunc
.endm

generate_nearest_scanline_func \
    pixman_scaled_nearest_scanline_0565_0565_SRC_asm_armv6, 1, h, 80, 32

generate_nearest_scanline_func \
    pixman_scaled_nearest_scanline_8888_8888_SRC_asm_armv6, 2,  , 48, 32
