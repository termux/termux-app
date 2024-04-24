/*
 * Copyright (c) 2012
 *      MIPS Technologies, Inc., California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the MIPS Technologies, Inc., nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE MIPS TECHNOLOGIES, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE MIPS TECHNOLOGIES, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "pixman-mips-dspr2-asm.h"

/*
 * This routine could be optimized for MIPS64. The current code only
 * uses MIPS32 instructions.
 */

#ifdef EB
#  define LWHI	lwl		/* high part is left in big-endian */
#  define SWHI	swl		/* high part is left in big-endian */
#  define LWLO	lwr		/* low part is right in big-endian */
#  define SWLO	swr		/* low part is right in big-endian */
#else
#  define LWHI	lwr		/* high part is right in little-endian */
#  define SWHI	swr		/* high part is right in little-endian */
#  define LWLO	lwl		/* low part is left in big-endian */
#  define SWLO	swl		/* low part is left in big-endian */
#endif

LEAF_MIPS32R2(pixman_mips_fast_memcpy)

	slti	AT, a2, 8
	bne	AT, zero, $last8
	move	v0, a0	/* memcpy returns the dst pointer */

/* Test if the src and dst are word-aligned, or can be made word-aligned */
	xor	t8, a1, a0
	andi	t8, t8, 0x3		/* t8 is a0/a1 word-displacement */

	bne	t8, zero, $unaligned
	negu	a3, a0

	andi	a3, a3, 0x3	/* we need to copy a3 bytes to make a0/a1 aligned */
	beq	a3, zero, $chk16w	/* when a3=0 then the dst (a0) is word-aligned */
	subu	a2, a2, a3	/* now a2 is the remining bytes count */

	LWHI	t8, 0(a1)
	addu	a1, a1, a3
	SWHI	t8, 0(a0)
	addu	a0, a0, a3

/* Now the dst/src are mutually word-aligned with word-aligned addresses */
$chk16w:	andi	t8, a2, 0x3f	/* any whole 64-byte chunks? */
				/* t8 is the byte count after 64-byte chunks */

	beq	a2, t8, $chk8w	/* if a2==t8, no 64-byte chunks */
				/* There will be at most 1 32-byte chunk after it */
	subu	a3, a2, t8	/* subtract from a2 the reminder */
                                /* Here a3 counts bytes in 16w chunks */
	addu	a3, a0, a3	/* Now a3 is the final dst after 64-byte chunks */

	addu	t0, a0, a2	/* t0 is the "past the end" address */

/*
 * When in the loop we exercise "pref 30, x(a0)", the a0+x should not be past
 * the "t0-32" address
 * This means: for x=128 the last "safe" a0 address is "t0-160"
 * Alternatively, for x=64 the last "safe" a0 address is "t0-96"
 * In the current version we use "pref 30, 128(a0)", so "t0-160" is the limit
 */
	subu	t9, t0, 160	/* t9 is the "last safe pref 30, 128(a0)" address */

	pref    0, 0(a1)		/* bring the first line of src, addr 0 */
	pref    0, 32(a1)	/* bring the second line of src, addr 32 */
	pref    0, 64(a1)	/* bring the third line of src, addr 64 */
	pref	30, 32(a0)	/* safe, as we have at least 64 bytes ahead */
/* In case the a0 > t9 don't use "pref 30" at all */
	sgtu	v1, a0, t9
	bgtz	v1, $loop16w	/* skip "pref 30, 64(a0)" for too short arrays */
	nop
/* otherwise, start with using pref30 */
	pref	30, 64(a0)
$loop16w:
	pref	0, 96(a1)
	lw	t0, 0(a1)
	bgtz	v1, $skip_pref30_96	/* skip "pref 30, 96(a0)" */
	lw	t1, 4(a1)
	pref    30, 96(a0)   /* continue setting up the dest, addr 96 */
$skip_pref30_96:
	lw	t2, 8(a1)
	lw	t3, 12(a1)
	lw	t4, 16(a1)
	lw	t5, 20(a1)
	lw	t6, 24(a1)
	lw	t7, 28(a1)
        pref    0, 128(a1)    /* bring the next lines of src, addr 128 */

	sw	t0, 0(a0)
	sw	t1, 4(a0)
	sw	t2, 8(a0)
	sw	t3, 12(a0)
	sw	t4, 16(a0)
	sw	t5, 20(a0)
	sw	t6, 24(a0)
	sw	t7, 28(a0)

	lw	t0, 32(a1)
	bgtz	v1, $skip_pref30_128	/* skip "pref 30, 128(a0)" */
	lw	t1, 36(a1)
	pref    30, 128(a0)   /* continue setting up the dest, addr 128 */
$skip_pref30_128:
	lw	t2, 40(a1)
	lw	t3, 44(a1)
	lw	t4, 48(a1)
	lw	t5, 52(a1)
	lw	t6, 56(a1)
	lw	t7, 60(a1)
        pref    0, 160(a1)    /* bring the next lines of src, addr 160 */

	sw	t0, 32(a0)
	sw	t1, 36(a0)
	sw	t2, 40(a0)
	sw	t3, 44(a0)
	sw	t4, 48(a0)
	sw	t5, 52(a0)
	sw	t6, 56(a0)
	sw	t7, 60(a0)

	addiu	a0, a0, 64	/* adding 64 to dest */
	sgtu	v1, a0, t9
	bne	a0, a3, $loop16w
	addiu	a1, a1, 64	/* adding 64 to src */
	move	a2, t8

/* Here we have src and dest word-aligned but less than 64-bytes to go */

$chk8w:
	pref 0, 0x0(a1)
	andi	t8, a2, 0x1f	/* is there a 32-byte chunk? */
				/* the t8 is the reminder count past 32-bytes */
	beq	a2, t8, $chk1w	/* when a2=t8, no 32-byte chunk */
	 nop

	lw	t0, 0(a1)
	lw	t1, 4(a1)
	lw	t2, 8(a1)
	lw	t3, 12(a1)
	lw	t4, 16(a1)
	lw	t5, 20(a1)
	lw	t6, 24(a1)
	lw	t7, 28(a1)
	addiu	a1, a1, 32

	sw	t0, 0(a0)
	sw	t1, 4(a0)
	sw	t2, 8(a0)
	sw	t3, 12(a0)
	sw	t4, 16(a0)
	sw	t5, 20(a0)
	sw	t6, 24(a0)
	sw	t7, 28(a0)
	addiu	a0, a0, 32

$chk1w:
	andi	a2, t8, 0x3	/* now a2 is the reminder past 1w chunks */
	beq	a2, t8, $last8
	subu	a3, t8, a2	/* a3 is count of bytes in 1w chunks */
	addu	a3, a0, a3	/* now a3 is the dst address past the 1w chunks */

/* copying in words (4-byte chunks) */
$wordCopy_loop:
	lw	t3, 0(a1)	/* the first t3 may be equal t0 ... optimize? */
	addiu	a1, a1, 4
	addiu	a0, a0, 4
	bne	a0, a3, $wordCopy_loop
	sw	t3, -4(a0)

/* For the last (<8) bytes */
$last8:
	blez	a2, leave
	addu	a3, a0, a2	/* a3 is the last dst address */
$last8loop:
	lb	v1, 0(a1)
	addiu	a1, a1, 1
	addiu	a0, a0, 1
	bne	a0, a3, $last8loop
	sb	v1, -1(a0)

leave:	j	ra
	nop

/*
 * UNALIGNED case
 */

$unaligned:
	/* got here with a3="negu a0" */
	andi	a3, a3, 0x3	/* test if the a0 is word aligned */
	beqz	a3, $ua_chk16w
	subu	a2, a2, a3	/* bytes left after initial a3 bytes */

	LWHI	v1, 0(a1)
	LWLO	v1, 3(a1)
	addu	a1, a1, a3	/* a3 may be here 1, 2 or 3 */
	SWHI	v1, 0(a0)
	addu	a0, a0, a3	/* below the dst will be word aligned (NOTE1) */

$ua_chk16w:	andi	t8, a2, 0x3f	/* any whole 64-byte chunks? */
				/* t8 is the byte count after 64-byte chunks */
	beq	a2, t8, $ua_chk8w	/* if a2==t8, no 64-byte chunks */
				/* There will be at most 1 32-byte chunk after it */
	subu	a3, a2, t8	/* subtract from a2 the reminder */
                                /* Here a3 counts bytes in 16w chunks */
	addu	a3, a0, a3	/* Now a3 is the final dst after 64-byte chunks */

	addu	t0, a0, a2	/* t0 is the "past the end" address */

	subu	t9, t0, 160	/* t9 is the "last safe pref 30, 128(a0)" address */

	pref    0, 0(a1)		/* bring the first line of src, addr 0 */
	pref    0, 32(a1)	/* bring the second line of src, addr 32 */
	pref    0, 64(a1)	/* bring the third line of src, addr 64 */
	pref	30, 32(a0)	/* safe, as we have at least 64 bytes ahead */
/* In case the a0 > t9 don't use "pref 30" at all */
	sgtu	v1, a0, t9
	bgtz	v1, $ua_loop16w	/* skip "pref 30, 64(a0)" for too short arrays */
	nop
/* otherwise,  start with using pref30 */
	pref	30, 64(a0)
$ua_loop16w:
	pref	0, 96(a1)
	LWHI	t0, 0(a1)
	LWLO	t0, 3(a1)
	LWHI	t1, 4(a1)
	bgtz	v1, $ua_skip_pref30_96
	LWLO	t1, 7(a1)
	pref    30, 96(a0)   /* continue setting up the dest, addr 96 */
$ua_skip_pref30_96:
	LWHI	t2, 8(a1)
	LWLO	t2, 11(a1)
	LWHI	t3, 12(a1)
	LWLO	t3, 15(a1)
	LWHI	t4, 16(a1)
	LWLO	t4, 19(a1)
	LWHI	t5, 20(a1)
	LWLO	t5, 23(a1)
	LWHI	t6, 24(a1)
	LWLO	t6, 27(a1)
	LWHI	t7, 28(a1)
	LWLO	t7, 31(a1)
        pref    0, 128(a1)    /* bring the next lines of src, addr 128 */

	sw	t0, 0(a0)
	sw	t1, 4(a0)
	sw	t2, 8(a0)
	sw	t3, 12(a0)
	sw	t4, 16(a0)
	sw	t5, 20(a0)
	sw	t6, 24(a0)
	sw	t7, 28(a0)

	LWHI	t0, 32(a1)
	LWLO	t0, 35(a1)
	LWHI	t1, 36(a1)
	bgtz	v1, $ua_skip_pref30_128
	LWLO	t1, 39(a1)
	pref    30, 128(a0)   /* continue setting up the dest, addr 128 */
$ua_skip_pref30_128:
	LWHI	t2, 40(a1)
	LWLO	t2, 43(a1)
	LWHI	t3, 44(a1)
	LWLO	t3, 47(a1)
	LWHI	t4, 48(a1)
	LWLO	t4, 51(a1)
	LWHI	t5, 52(a1)
	LWLO	t5, 55(a1)
	LWHI	t6, 56(a1)
	LWLO	t6, 59(a1)
	LWHI	t7, 60(a1)
	LWLO	t7, 63(a1)
        pref    0, 160(a1)    /* bring the next lines of src, addr 160 */

	sw	t0, 32(a0)
	sw	t1, 36(a0)
	sw	t2, 40(a0)
	sw	t3, 44(a0)
	sw	t4, 48(a0)
	sw	t5, 52(a0)
	sw	t6, 56(a0)
	sw	t7, 60(a0)

	addiu	a0, a0, 64	/* adding 64 to dest */
	sgtu	v1, a0, t9
	bne	a0, a3, $ua_loop16w
	addiu	a1, a1, 64	/* adding 64 to src */
	move	a2, t8

/* Here we have src and dest word-aligned but less than 64-bytes to go */

$ua_chk8w:
	pref 0, 0x0(a1)
	andi	t8, a2, 0x1f	/* is there a 32-byte chunk? */
				/* the t8 is the reminder count */
	beq	a2, t8, $ua_chk1w	/* when a2=t8, no 32-byte chunk */

	LWHI	t0, 0(a1)
	LWLO	t0, 3(a1)
	LWHI	t1, 4(a1)
	LWLO	t1, 7(a1)
	LWHI	t2, 8(a1)
	LWLO	t2, 11(a1)
	LWHI	t3, 12(a1)
	LWLO	t3, 15(a1)
	LWHI	t4, 16(a1)
	LWLO	t4, 19(a1)
	LWHI	t5, 20(a1)
	LWLO	t5, 23(a1)
	LWHI	t6, 24(a1)
	LWLO	t6, 27(a1)
	LWHI	t7, 28(a1)
	LWLO	t7, 31(a1)
	addiu	a1, a1, 32

	sw	t0, 0(a0)
	sw	t1, 4(a0)
	sw	t2, 8(a0)
	sw	t3, 12(a0)
	sw	t4, 16(a0)
	sw	t5, 20(a0)
	sw	t6, 24(a0)
	sw	t7, 28(a0)
	addiu	a0, a0, 32

$ua_chk1w:
	andi	a2, t8, 0x3	/* now a2 is the reminder past 1w chunks */
	beq	a2, t8, $ua_smallCopy
	subu	a3, t8, a2	/* a3 is count of bytes in 1w chunks */
	addu	a3, a0, a3	/* now a3 is the dst address past the 1w chunks */

/* copying in words (4-byte chunks) */
$ua_wordCopy_loop:
	LWHI	v1, 0(a1)
	LWLO	v1, 3(a1)
	addiu	a1, a1, 4
	addiu	a0, a0, 4		/* note: dst=a0 is word aligned here, see NOTE1 */
	bne	a0, a3, $ua_wordCopy_loop
	sw	v1, -4(a0)

/* Now less than 4 bytes (value in a2) left to copy */
$ua_smallCopy:
	beqz	a2, leave
	addu	a3, a0, a2	/* a3 is the last dst address */
$ua_smallCopy_loop:
	lb	v1, 0(a1)
	addiu	a1, a1, 1
	addiu	a0, a0, 1
	bne	a0, a3, $ua_smallCopy_loop
	sb	v1, -1(a0)

	j	ra
	nop

END(pixman_mips_fast_memcpy)
