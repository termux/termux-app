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
 *
 * Author:  Nemanja Lukic (nemanja.lukic@rt-rk.com)
 */

#include "pixman-private.h"
#include "pixman-mips-dspr2-asm.h"

LEAF_MIPS_DSPR2(pixman_fill_buff16_mips)
/*
 * a0 - *dest
 * a1 - count (bytes)
 * a2 - value to fill buffer with
 */

    beqz     a1, 3f
     andi    t1, a0, 0x0002
    beqz     t1, 0f          /* check if address is 4-byte aligned */
     nop
    sh       a2, 0(a0)
    addiu    a0, a0, 2
    addiu    a1, a1, -2
0:
    srl      t1, a1, 5       /* t1 how many multiples of 32 bytes */
    replv.ph a2, a2          /* replicate fill value (16bit) in a2 */
    beqz     t1, 2f
     nop
1:
    addiu    t1, t1, -1
    beqz     t1, 11f
     addiu   a1, a1, -32
    pref     30, 32(a0)
    sw       a2, 0(a0)
    sw       a2, 4(a0)
    sw       a2, 8(a0)
    sw       a2, 12(a0)
    sw       a2, 16(a0)
    sw       a2, 20(a0)
    sw       a2, 24(a0)
    sw       a2, 28(a0)
    b        1b
     addiu   a0, a0, 32
11:
    sw       a2, 0(a0)
    sw       a2, 4(a0)
    sw       a2, 8(a0)
    sw       a2, 12(a0)
    sw       a2, 16(a0)
    sw       a2, 20(a0)
    sw       a2, 24(a0)
    sw       a2, 28(a0)
    addiu    a0, a0, 32
2:
    blez     a1, 3f
     addiu   a1, a1, -2
    sh       a2, 0(a0)
    b        2b
     addiu   a0, a0, 2
3:
    jr       ra
     nop

END(pixman_fill_buff16_mips)

LEAF_MIPS32R2(pixman_fill_buff32_mips)
/*
 * a0 - *dest
 * a1 - count (bytes)
 * a2 - value to fill buffer with
 */

    beqz     a1, 3f
     nop
    srl      t1, a1, 5 /* t1 how many multiples of 32 bytes */
    beqz     t1, 2f
     nop
1:
    addiu    t1, t1, -1
    beqz     t1, 11f
     addiu   a1, a1, -32
    pref     30, 32(a0)
    sw       a2, 0(a0)
    sw       a2, 4(a0)
    sw       a2, 8(a0)
    sw       a2, 12(a0)
    sw       a2, 16(a0)
    sw       a2, 20(a0)
    sw       a2, 24(a0)
    sw       a2, 28(a0)
    b        1b
     addiu   a0, a0, 32
11:
    sw       a2, 0(a0)
    sw       a2, 4(a0)
    sw       a2, 8(a0)
    sw       a2, 12(a0)
    sw       a2, 16(a0)
    sw       a2, 20(a0)
    sw       a2, 24(a0)
    sw       a2, 28(a0)
    addiu    a0, a0, 32
2:
    blez     a1, 3f
     addiu   a1, a1, -4
    sw       a2, 0(a0)
    b        2b
     addiu   a0, a0, 4
3:
    jr       ra
     nop

END(pixman_fill_buff32_mips)

LEAF_MIPS_DSPR2(pixman_composite_src_8888_0565_asm_mips)
/*
 * a0 - dst (r5g6b5)
 * a1 - src (a8r8g8b8)
 * a2 - w
 */

    beqz     a2, 3f
     nop
    addiu    t1, a2, -1
    beqz     t1, 2f
     nop
    li       t4, 0xf800f800
    li       t5, 0x07e007e0
    li       t6, 0x001f001f
1:
    lw       t0, 0(a1)
    lw       t1, 4(a1)
    addiu    a1, a1, 8
    addiu    a2, a2, -2

    CONVERT_2x8888_TO_2x0565 t0, t1, t2, t3, t4, t5, t6, t7, t8

    sh       t2, 0(a0)
    sh       t3, 2(a0)

    addiu    t2, a2, -1
    bgtz     t2, 1b
     addiu   a0, a0, 4
2:
    beqz     a2, 3f
     nop
    lw       t0, 0(a1)

    CONVERT_1x8888_TO_1x0565 t0, t1, t2, t3

    sh       t1, 0(a0)
3:
    j        ra
     nop

END(pixman_composite_src_8888_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_src_0565_8888_asm_mips)
/*
 * a0 - dst (a8r8g8b8)
 * a1 - src (r5g6b5)
 * a2 - w
 */

    beqz     a2, 3f
     nop
    addiu    t1, a2, -1
    beqz     t1, 2f
     nop
    li       t4, 0x07e007e0
    li       t5, 0x001F001F
1:
    lhu      t0, 0(a1)
    lhu      t1, 2(a1)
    addiu    a1, a1, 4
    addiu    a2, a2, -2

    CONVERT_2x0565_TO_2x8888 t0, t1, t2, t3, t4, t5, t6, t7, t8, t9

    sw       t2, 0(a0)
    sw       t3, 4(a0)

    addiu    t2, a2, -1
    bgtz     t2, 1b
     addiu   a0, a0, 8
2:
    beqz     a2, 3f
     nop
    lhu      t0, 0(a1)

    CONVERT_1x0565_TO_1x8888 t0, t1, t2, t3

    sw       t1, 0(a0)
3:
    j        ra
     nop

END(pixman_composite_src_0565_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_src_x888_8888_asm_mips)
/*
 * a0 - dst (a8r8g8b8)
 * a1 - src (x8r8g8b8)
 * a2 - w
 */

    beqz     a2, 4f
     nop
    li       t9, 0xff000000
    srl      t8, a2, 3    /* t1 = how many multiples of 8 src pixels */
    beqz     t8, 3f       /* branch if less than 8 src pixels */
     nop
1:
    addiu    t8, t8, -1
    beqz     t8, 2f
     addiu   a2, a2, -8
    pref     0, 32(a1)
    lw       t0, 0(a1)
    lw       t1, 4(a1)
    lw       t2, 8(a1)
    lw       t3, 12(a1)
    lw       t4, 16(a1)
    lw       t5, 20(a1)
    lw       t6, 24(a1)
    lw       t7, 28(a1)
    addiu    a1, a1, 32
    or       t0, t0, t9
    or       t1, t1, t9
    or       t2, t2, t9
    or       t3, t3, t9
    or       t4, t4, t9
    or       t5, t5, t9
    or       t6, t6, t9
    or       t7, t7, t9
    pref     30, 32(a0)
    sw       t0, 0(a0)
    sw       t1, 4(a0)
    sw       t2, 8(a0)
    sw       t3, 12(a0)
    sw       t4, 16(a0)
    sw       t5, 20(a0)
    sw       t6, 24(a0)
    sw       t7, 28(a0)
    b        1b
     addiu   a0, a0, 32
2:
    lw       t0, 0(a1)
    lw       t1, 4(a1)
    lw       t2, 8(a1)
    lw       t3, 12(a1)
    lw       t4, 16(a1)
    lw       t5, 20(a1)
    lw       t6, 24(a1)
    lw       t7, 28(a1)
    addiu    a1, a1, 32
    or       t0, t0, t9
    or       t1, t1, t9
    or       t2, t2, t9
    or       t3, t3, t9
    or       t4, t4, t9
    or       t5, t5, t9
    or       t6, t6, t9
    or       t7, t7, t9
    sw       t0, 0(a0)
    sw       t1, 4(a0)
    sw       t2, 8(a0)
    sw       t3, 12(a0)
    sw       t4, 16(a0)
    sw       t5, 20(a0)
    sw       t6, 24(a0)
    sw       t7, 28(a0)
    beqz     a2, 4f
     addiu   a0, a0, 32
3:
    lw       t0, 0(a1)
    addiu    a1, a1, 4
    addiu    a2, a2, -1
    or       t1, t0, t9
    sw       t1, 0(a0)
    bnez     a2, 3b
     addiu   a0, a0, 4
4:
    jr       ra
     nop

END(pixman_composite_src_x888_8888_asm_mips)

#if defined(__MIPSEL__) || defined(__MIPSEL) || defined(_MIPSEL) || defined(MIPSEL)
LEAF_MIPS_DSPR2(pixman_composite_src_0888_8888_rev_asm_mips)
/*
 * a0 - dst (a8r8g8b8)
 * a1 - src (b8g8r8)
 * a2 - w
 */

    beqz              a2, 6f
     nop

    lui               t8, 0xff00;
    srl               t9, a2, 2   /* t9 = how many multiples of 4 src pixels */
    beqz              t9, 4f      /* branch if less than 4 src pixels */
     nop

    li                t0, 0x1
    li                t1, 0x2
    li                t2, 0x3
    andi              t3, a1, 0x3
    beq               t3, t0, 1f
     nop
    beq               t3, t1, 2f
     nop
    beq               t3, t2, 3f
     nop

0:
    beqz              t9, 4f
     addiu            t9, t9, -1
    lw                t0, 0(a1)            /* t0 = R2 | B1 | G1 | R1 */
    lw                t1, 4(a1)            /* t1 = G3 | R3 | B2 | G2 */
    lw                t2, 8(a1)            /* t2 = B4 | G4 | R4 | B3 */

    addiu             a1, a1, 12
    addiu             a2, a2, -4

    wsbh              t0, t0               /* t0 = B1 | R2 | R1 | G1 */
    wsbh              t1, t1               /* t1 = R3 | G3 | G2 | B2 */
    wsbh              t2, t2               /* t2 = G4 | B4 | B3 | R4 */

    packrl.ph         t3, t1, t0           /* t3 = G2 | B2 | B1 | R2 */
    packrl.ph         t4, t0, t0           /* t4 = R1 | G1 | B1 | R2 */
    rotr              t3, t3, 16           /* t3 = B1 | R2 | G2 | B2 */
    or                t3, t3, t8           /* t3 = FF | R2 | G2 | B2 */
    srl               t4, t4, 8            /* t4 =  0 | R1 | G1 | B1 */
    or                t4, t4, t8           /* t4 = FF | R1 | G1 | B1 */
    packrl.ph         t5, t2, t1           /* t5 = B3 | R4 | R3 | G3 */
    rotr              t5, t5, 24           /* t5 = R4 | R3 | G3 | B3 */
    or                t5, t5, t8           /* t5 = FF | R3 | G3 | B3 */
    rotr              t2, t2, 16           /* t2 = B3 | R4 | G4 | B4 */
    or                t2, t2, t8           /* t5 = FF | R3 | G3 | B3 */

    sw                t4, 0(a0)
    sw                t3, 4(a0)
    sw                t5, 8(a0)
    sw                t2, 12(a0)
    b                 0b
     addiu            a0, a0, 16

1:
    lbu               t6, 0(a1)            /* t6 =  0 |  0 |  0 | R1 */
    lhu               t7, 1(a1)            /* t7 =  0 |  0 | B1 | G1 */
    sll               t6, t6, 16           /* t6 =  0 | R1 |  0 | 0  */
    wsbh              t7, t7               /* t7 =  0 |  0 | G1 | B1 */
    or                t7, t6, t7           /* t7 =  0 | R1 | G1 | B1 */
11:
    beqz              t9, 4f
     addiu            t9, t9, -1
    lw                t0, 3(a1)            /* t0 = R3 | B2 | G2 | R2 */
    lw                t1, 7(a1)            /* t1 = G4 | R4 | B3 | G3 */
    lw                t2, 11(a1)           /* t2 = B5 | G5 | R5 | B4 */

    addiu             a1, a1, 12
    addiu             a2, a2, -4

    wsbh              t0, t0               /* t0 = B2 | R3 | R2 | G2 */
    wsbh              t1, t1               /* t1 = R4 | G4 | G3 | B3 */
    wsbh              t2, t2               /* t2 = G5 | B5 | B4 | R5 */

    packrl.ph         t3, t1, t0           /* t3 = G3 | B3 | B2 | R3 */
    packrl.ph         t4, t2, t1           /* t4 = B4 | R5 | R4 | G4 */
    rotr              t0, t0, 24           /* t0 = R3 | R2 | G2 | B2 */
    rotr              t3, t3, 16           /* t3 = B2 | R3 | G3 | B3 */
    rotr              t4, t4, 24           /* t4 = R5 | R4 | G4 | B4 */
    or                t7, t7, t8           /* t7 = FF | R1 | G1 | B1 */
    or                t0, t0, t8           /* t0 = FF | R2 | G2 | B2 */
    or                t3, t3, t8           /* t1 = FF | R3 | G3 | B3 */
    or                t4, t4, t8           /* t3 = FF | R4 | G4 | B4 */

    sw                t7, 0(a0)
    sw                t0, 4(a0)
    sw                t3, 8(a0)
    sw                t4, 12(a0)
    rotr              t7, t2, 16           /* t7 = xx | R5 | G5 | B5 */
    b                 11b
     addiu            a0, a0, 16

2:
    lhu               t7, 0(a1)            /* t7 =  0 |  0 | G1 | R1 */
    wsbh              t7, t7               /* t7 =  0 |  0 | R1 | G1 */
21:
    beqz              t9, 4f
     addiu            t9, t9, -1
    lw                t0, 2(a1)            /* t0 = B2 | G2 | R2 | B1 */
    lw                t1, 6(a1)            /* t1 = R4 | B3 | G3 | R3 */
    lw                t2, 10(a1)           /* t2 = G5 | R5 | B4 | G4 */

    addiu             a1, a1, 12
    addiu             a2, a2, -4

    wsbh              t0, t0               /* t0 = G2 | B2 | B1 | R2 */
    wsbh              t1, t1               /* t1 = B3 | R4 | R3 | G3 */
    wsbh              t2, t2               /* t2 = R5 | G5 | G4 | B4 */

    precr_sra.ph.w    t7, t0, 0            /* t7 = R1 | G1 | B1 | R2 */
    rotr              t0, t0, 16           /* t0 = B1 | R2 | G2 | B2 */
    packrl.ph         t3, t2, t1           /* t3 = G4 | B4 | B3 | R4 */
    rotr              t1, t1, 24           /* t1 = R4 | R3 | G3 | B3 */
    srl               t7, t7, 8            /* t7 =  0 | R1 | G1 | B1 */
    rotr              t3, t3, 16           /* t3 = B3 | R4 | G4 | B4 */
    or                t7, t7, t8           /* t7 = FF | R1 | G1 | B1 */
    or                t0, t0, t8           /* t0 = FF | R2 | G2 | B2 */
    or                t1, t1, t8           /* t1 = FF | R3 | G3 | B3 */
    or                t3, t3, t8           /* t3 = FF | R4 | G4 | B4 */

    sw                t7, 0(a0)
    sw                t0, 4(a0)
    sw                t1, 8(a0)
    sw                t3, 12(a0)
    srl               t7, t2, 16           /* t7 =  0 |  0 | R5 | G5 */
    b                 21b
     addiu            a0, a0, 16

3:
    lbu               t7, 0(a1)            /* t7 =  0 |  0 |  0 | R1 */
31:
    beqz              t9, 4f
     addiu            t9, t9, -1
    lw                t0, 1(a1)            /* t0 = G2 | R2 | B1 | G1 */
    lw                t1, 5(a1)            /* t1 = B3 | G3 | R3 | B2 */
    lw                t2, 9(a1)            /* t2 = R5 | B4 | G4 | R4 */

    addiu             a1, a1, 12
    addiu             a2, a2, -4

    wsbh              t0, t0               /* t0 = R2 | G2 | G1 | B1 */
    wsbh              t1, t1               /* t1 = G3 | B3 | B2 | R3 */
    wsbh              t2, t2               /* t2 = B4 | R5 | R4 | G4 */

    precr_sra.ph.w    t7, t0, 0            /* t7 = xx | R1 | G1 | B1 */
    packrl.ph         t3, t1, t0           /* t3 = B2 | R3 | R2 | G2 */
    rotr              t1, t1, 16           /* t1 = B2 | R3 | G3 | B3 */
    rotr              t4, t2, 24           /* t4 = R5 | R4 | G4 | B4 */
    rotr              t3, t3, 24           /* t3 = R3 | R2 | G2 | B2 */
    or                t7, t7, t8           /* t7 = FF | R1 | G1 | B1 */
    or                t3, t3, t8           /* t3 = FF | R2 | G2 | B2 */
    or                t1, t1, t8           /* t1 = FF | R3 | G3 | B3 */
    or                t4, t4, t8           /* t4 = FF | R4 | G4 | B4 */

    sw                t7, 0(a0)
    sw                t3, 4(a0)
    sw                t1, 8(a0)
    sw                t4, 12(a0)
    srl               t7, t2, 16           /* t7 =  0 |  0 | xx | R5 */
    b                 31b
     addiu            a0, a0, 16

4:
    beqz              a2, 6f
     nop
5:
    lbu               t0, 0(a1)            /* t0 =  0 | 0 | 0 | R */
    lbu               t1, 1(a1)            /* t1 =  0 | 0 | 0 | G */
    lbu               t2, 2(a1)            /* t2 =  0 | 0 | 0 | B */
    addiu             a1, a1, 3

    sll               t0, t0, 16           /* t2 =  0 | R | 0 | 0 */
    sll               t1, t1, 8            /* t1 =  0 | 0 | G | 0 */

    or                t2, t2, t1           /* t2 =  0 | 0 | G | B */
    or                t2, t2, t0           /* t2 =  0 | R | G | B */
    or                t2, t2, t8           /* t2 = FF | R | G | B */

    sw                t2, 0(a0)
    addiu             a2, a2, -1
    bnez              a2, 5b
     addiu            a0, a0, 4
6:
    j                 ra
     nop

END(pixman_composite_src_0888_8888_rev_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_src_0888_0565_rev_asm_mips)
/*
 * a0 - dst (r5g6b5)
 * a1 - src (b8g8r8)
 * a2 - w
 */

    SAVE_REGS_ON_STACK 0, v0, v1
    beqz              a2, 6f
     nop

    li                t6, 0xf800f800
    li                t7, 0x07e007e0
    li                t8, 0x001F001F
    srl               t9, a2, 2   /* t9 = how many multiples of 4 src pixels */
    beqz              t9, 4f      /* branch if less than 4 src pixels */
     nop

    li                t0, 0x1
    li                t1, 0x2
    li                t2, 0x3
    andi              t3, a1, 0x3
    beq               t3, t0, 1f
     nop
    beq               t3, t1, 2f
     nop
    beq               t3, t2, 3f
     nop

0:
    beqz              t9, 4f
     addiu            t9, t9, -1
    lw                t0, 0(a1)            /* t0 = R2 | B1 | G1 | R1 */
    lw                t1, 4(a1)            /* t1 = G3 | R3 | B2 | G2 */
    lw                t2, 8(a1)            /* t2 = B4 | G4 | R4 | B3 */

    addiu             a1, a1, 12
    addiu             a2, a2, -4

    wsbh              t0, t0               /* t0 = B1 | R2 | R1 | G1 */
    wsbh              t1, t1               /* t1 = R3 | G3 | G2 | B2 */
    wsbh              t2, t2               /* t2 = G4 | B4 | B3 | R4 */

    packrl.ph         t3, t1, t0           /* t3 = G2 | B2 | B1 | R2 */
    packrl.ph         t4, t0, t0           /* t4 = R1 | G1 | B1 | R2 */
    rotr              t3, t3, 16           /* t3 = B1 | R2 | G2 | B2 */
    srl               t4, t4, 8            /* t4 =  0 | R1 | G1 | B1 */
    packrl.ph         t5, t2, t1           /* t5 = B3 | R4 | R3 | G3 */
    rotr              t5, t5, 24           /* t5 = R4 | R3 | G3 | B3 */
    rotr              t2, t2, 16           /* t2 = B3 | R4 | G4 | B4 */

    CONVERT_2x8888_TO_2x0565 t4, t3, t4, t3, t6, t7, t8, v0, v1
    CONVERT_2x8888_TO_2x0565 t5, t2, t5, t2, t6, t7, t8, v0, v1

    sh                t4, 0(a0)
    sh                t3, 2(a0)
    sh                t5, 4(a0)
    sh                t2, 6(a0)
    b                 0b
     addiu            a0, a0, 8

1:
    lbu               t4, 0(a1)            /* t4 =  0 |  0 |  0 | R1 */
    lhu               t5, 1(a1)            /* t5 =  0 |  0 | B1 | G1 */
    sll               t4, t4, 16           /* t4 =  0 | R1 |  0 | 0  */
    wsbh              t5, t5               /* t5 =  0 |  0 | G1 | B1 */
    or                t5, t4, t5           /* t5 =  0 | R1 | G1 | B1 */
11:
    beqz              t9, 4f
     addiu            t9, t9, -1
    lw                t0, 3(a1)            /* t0 = R3 | B2 | G2 | R2 */
    lw                t1, 7(a1)            /* t1 = G4 | R4 | B3 | G3 */
    lw                t2, 11(a1)           /* t2 = B5 | G5 | R5 | B4 */

    addiu             a1, a1, 12
    addiu             a2, a2, -4

    wsbh              t0, t0               /* t0 = B2 | R3 | R2 | G2 */
    wsbh              t1, t1               /* t1 = R4 | G4 | G3 | B3 */
    wsbh              t2, t2               /* t2 = G5 | B5 | B4 | R5 */

    packrl.ph         t3, t1, t0           /* t3 = G3 | B3 | B2 | R3 */
    packrl.ph         t4, t2, t1           /* t4 = B4 | R5 | R4 | G4 */
    rotr              t0, t0, 24           /* t0 = R3 | R2 | G2 | B2 */
    rotr              t3, t3, 16           /* t3 = B2 | R3 | G3 | B3 */
    rotr              t4, t4, 24           /* t4 = R5 | R4 | G4 | B4 */

    CONVERT_2x8888_TO_2x0565 t5, t0, t5, t0, t6, t7, t8, v0, v1
    CONVERT_2x8888_TO_2x0565 t3, t4, t3, t4, t6, t7, t8, v0, v1

    sh                t5, 0(a0)
    sh                t0, 2(a0)
    sh                t3, 4(a0)
    sh                t4, 6(a0)
    rotr              t5, t2, 16           /* t5 = xx | R5 | G5 | B5 */
    b                 11b
     addiu            a0, a0, 8

2:
    lhu               t5, 0(a1)            /* t5 =  0 |  0 | G1 | R1 */
    wsbh              t5, t5               /* t5 =  0 |  0 | R1 | G1 */
21:
    beqz              t9, 4f
     addiu            t9, t9, -1
    lw                t0, 2(a1)            /* t0 = B2 | G2 | R2 | B1 */
    lw                t1, 6(a1)            /* t1 = R4 | B3 | G3 | R3 */
    lw                t2, 10(a1)           /* t2 = G5 | R5 | B4 | G4 */

    addiu             a1, a1, 12
    addiu             a2, a2, -4

    wsbh              t0, t0               /* t0 = G2 | B2 | B1 | R2 */
    wsbh              t1, t1               /* t1 = B3 | R4 | R3 | G3 */
    wsbh              t2, t2               /* t2 = R5 | G5 | G4 | B4 */

    precr_sra.ph.w    t5, t0, 0            /* t5 = R1 | G1 | B1 | R2 */
    rotr              t0, t0, 16           /* t0 = B1 | R2 | G2 | B2 */
    packrl.ph         t3, t2, t1           /* t3 = G4 | B4 | B3 | R4 */
    rotr              t1, t1, 24           /* t1 = R4 | R3 | G3 | B3 */
    srl               t5, t5, 8            /* t5 =  0 | R1 | G1 | B1 */
    rotr              t3, t3, 16           /* t3 = B3 | R4 | G4 | B4 */

    CONVERT_2x8888_TO_2x0565 t5, t0, t5, t0, t6, t7, t8, v0, v1
    CONVERT_2x8888_TO_2x0565 t1, t3, t1, t3, t6, t7, t8, v0, v1

    sh                t5, 0(a0)
    sh                t0, 2(a0)
    sh                t1, 4(a0)
    sh                t3, 6(a0)
    srl               t5, t2, 16           /* t5 =  0 |  0 | R5 | G5 */
    b                 21b
     addiu            a0, a0, 8

3:
    lbu               t5, 0(a1)            /* t5 =  0 |  0 |  0 | R1 */
31:
    beqz              t9, 4f
     addiu            t9, t9, -1
    lw                t0, 1(a1)            /* t0 = G2 | R2 | B1 | G1 */
    lw                t1, 5(a1)            /* t1 = B3 | G3 | R3 | B2 */
    lw                t2, 9(a1)            /* t2 = R5 | B4 | G4 | R4 */

    addiu             a1, a1, 12
    addiu             a2, a2, -4

    wsbh              t0, t0               /* t0 = R2 | G2 | G1 | B1 */
    wsbh              t1, t1               /* t1 = G3 | B3 | B2 | R3 */
    wsbh              t2, t2               /* t2 = B4 | R5 | R4 | G4 */

    precr_sra.ph.w    t5, t0, 0            /* t5 = xx | R1 | G1 | B1 */
    packrl.ph         t3, t1, t0           /* t3 = B2 | R3 | R2 | G2 */
    rotr              t1, t1, 16           /* t1 = B2 | R3 | G3 | B3 */
    rotr              t4, t2, 24           /* t4 = R5 | R4 | G4 | B4 */
    rotr              t3, t3, 24           /* t3 = R3 | R2 | G2 | B2 */

    CONVERT_2x8888_TO_2x0565 t5, t3, t5, t3, t6, t7, t8, v0, v1
    CONVERT_2x8888_TO_2x0565 t1, t4, t1, t4, t6, t7, t8, v0, v1

    sh                t5, 0(a0)
    sh                t3, 2(a0)
    sh                t1, 4(a0)
    sh                t4, 6(a0)
    srl               t5, t2, 16           /* t5 =  0 |  0 | xx | R5 */
    b                 31b
     addiu            a0, a0, 8

4:
    beqz              a2, 6f
     nop
5:
    lbu               t0, 0(a1)            /* t0 =  0 | 0 | 0 | R */
    lbu               t1, 1(a1)            /* t1 =  0 | 0 | 0 | G */
    lbu               t2, 2(a1)            /* t2 =  0 | 0 | 0 | B */
    addiu             a1, a1, 3

    sll               t0, t0, 16           /* t2 =  0 | R | 0 | 0 */
    sll               t1, t1, 8            /* t1 =  0 | 0 | G | 0 */

    or                t2, t2, t1           /* t2 =  0 | 0 | G | B */
    or                t2, t2, t0           /* t2 =  0 | R | G | B */

    CONVERT_1x8888_TO_1x0565 t2, t3, t4, t5

    sh                t3, 0(a0)
    addiu             a2, a2, -1
    bnez              a2, 5b
     addiu            a0, a0, 2
6:
    RESTORE_REGS_FROM_STACK 0, v0, v1
    j                 ra
     nop

END(pixman_composite_src_0888_0565_rev_asm_mips)
#endif

LEAF_MIPS_DSPR2(pixman_composite_src_pixbuf_8888_asm_mips)
/*
 * a0 - dst  (a8b8g8r8)
 * a1 - src  (a8r8g8b8)
 * a2 - w
 */

    SAVE_REGS_ON_STACK 0, v0
    li       v0, 0x00ff00ff

    beqz     a2, 3f
     nop
    addiu    t1, a2, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1)
    lw       t1, 4(a1)
    addiu    a1, a1, 8
    addiu    a2, a2, -2
    srl      t2, t0, 24
    srl      t3, t1, 24

    MIPS_2xUN8x4_MUL_2xUN8 t0, t1, t2, t3, t0, t1, v0, t4, t5, t6, t7, t8, t9

    sll      t0, t0, 8
    sll      t1, t1, 8
    andi     t2, t2, 0xff
    andi     t3, t3, 0xff
    or       t0, t0, t2
    or       t1, t1, t3
    wsbh     t0, t0
    wsbh     t1, t1
    rotr     t0, t0, 16
    rotr     t1, t1, 16
    sw       t0, 0(a0)
    sw       t1, 4(a0)

    addiu    t2, a2, -1
    bgtz     t2, 1b
     addiu   a0, a0, 8
2:
    beqz     a2, 3f
     nop
    lw       t0, 0(a1)
    srl      t1, t0, 24

    MIPS_UN8x4_MUL_UN8 t0, t1, t0, v0, t3, t4, t5

    sll      t0, t0, 8
    andi     t1, t1, 0xff
    or       t0, t0, t1
    wsbh     t0, t0
    rotr     t0, t0, 16
    sw       t0, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, v0
    j        ra
     nop

END(pixman_composite_src_pixbuf_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_src_rpixbuf_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8r8g8b8)
 * a2 - w
 */

    SAVE_REGS_ON_STACK 0, v0
    li       v0, 0x00ff00ff

    beqz     a2, 3f
     nop
    addiu    t1, a2, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1)
    lw       t1, 4(a1)
    addiu    a1, a1, 8
    addiu    a2, a2, -2
    srl      t2, t0, 24
    srl      t3, t1, 24

    MIPS_2xUN8x4_MUL_2xUN8 t0, t1, t2, t3, t0, t1, v0, t4, t5, t6, t7, t8, t9

    sll      t0, t0, 8
    sll      t1, t1, 8
    andi     t2, t2, 0xff
    andi     t3, t3, 0xff
    or       t0, t0, t2
    or       t1, t1, t3
    rotr     t0, t0, 8
    rotr     t1, t1, 8
    sw       t0, 0(a0)
    sw       t1, 4(a0)

    addiu    t2, a2, -1
    bgtz     t2, 1b
     addiu   a0, a0, 8
2:
    beqz     a2, 3f
     nop
    lw       t0, 0(a1)
    srl      t1, t0, 24

    MIPS_UN8x4_MUL_UN8 t0, t1, t0, v0, t3, t4, t5

    sll      t0, t0, 8
    andi     t1, t1, 0xff
    or       t0, t0, t1
    rotr     t0, t0, 8
    sw       t0, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, v0
    j        ra
     nop

END(pixman_composite_src_rpixbuf_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_src_n_8_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (32bit constant)
 * a2 - mask (a8)
 * a3 - w
 */


    SAVE_REGS_ON_STACK 0, v0
    li       v0, 0x00ff00ff

    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop

1:
                       /* a1 = source      (32bit constant) */
    lbu      t0, 0(a2) /* t2 = mask        (a8) */
    lbu      t1, 1(a2) /* t3 = mask        (a8) */
    addiu    a2, a2, 2

    MIPS_2xUN8x4_MUL_2xUN8 a1, a1, t0, t1, t2, t3, v0, t4, t5, t6, t7, t8, t9

    sw       t2, 0(a0)
    sw       t3, 4(a0)
    addiu    a3, a3, -2
    addiu    t2, a3, -1
    bgtz     t2, 1b
     addiu   a0, a0, 8

    beqz     a3, 3f
     nop

2:
    lbu      t0, 0(a2)
    addiu    a2, a2, 1

    MIPS_UN8x4_MUL_UN8 a1, t0, t1, v0, t3, t4, t5

    sw       t1, 0(a0)
    addiu    a3, a3, -1
    addiu    a0, a0, 4

3:
    RESTORE_REGS_FROM_STACK 0, v0
    j        ra
     nop

END(pixman_composite_src_n_8_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_src_n_8_8_asm_mips)
/*
 * a0 - dst  (a8)
 * a1 - src  (32bit constant)
 * a2 - mask (a8)
 * a3 - w
 */

    li                t9, 0x00ff00ff
    beqz              a3, 3f
     nop
    srl               t7, a3, 2   /* t7 = how many multiples of 4 dst pixels */
    beqz              t7, 1f      /* branch if less than 4 src pixels */
     nop

    srl               t8, a1, 24
    replv.ph          t8, t8

0:
    beqz              t7, 1f
     addiu            t7, t7, -1
    lbu               t0, 0(a2)
    lbu               t1, 1(a2)
    lbu               t2, 2(a2)
    lbu               t3, 3(a2)

    addiu             a2, a2, 4

    precr_sra.ph.w    t1, t0, 0
    precr_sra.ph.w    t3, t2, 0
    precr.qb.ph       t0, t3, t1

    muleu_s.ph.qbl    t2, t0, t8
    muleu_s.ph.qbr    t3, t0, t8
    shra_r.ph         t4, t2, 8
    shra_r.ph         t5, t3, 8
    and               t4, t4, t9
    and               t5, t5, t9
    addq.ph           t2, t2, t4
    addq.ph           t3, t3, t5
    shra_r.ph         t2, t2, 8
    shra_r.ph         t3, t3, 8
    precr.qb.ph       t2, t2, t3

    sb                t2, 0(a0)
    srl               t2, t2, 8
    sb                t2, 1(a0)
    srl               t2, t2, 8
    sb                t2, 2(a0)
    srl               t2, t2, 8
    sb                t2, 3(a0)
    addiu             a3, a3, -4
    b                 0b
     addiu            a0, a0, 4

1:
    beqz              a3, 3f
     nop
    srl               t8, a1, 24
2:
    lbu               t0, 0(a2)
    addiu             a2, a2, 1

    mul               t2, t0, t8
    shra_r.ph         t3, t2, 8
    andi              t3, t3, 0x00ff
    addq.ph           t2, t2, t3
    shra_r.ph         t2, t2, 8

    sb                t2, 0(a0)
    addiu             a3, a3, -1
    bnez              a3, 2b
     addiu            a0, a0, 1

3:
    j                 ra
     nop

END(pixman_composite_src_n_8_8_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_n_8888_8888_ca_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (32bit constant)
 * a2 - mask (a8r8g8b8)
 * a3 - w
 */

    beqz         a3, 8f
     nop
    SAVE_REGS_ON_STACK 8, s0, s1, s2, s3, s4, s5

    li           t6, 0xff
    addiu        t7, zero, -1 /* t7 = 0xffffffff */
    srl          t8, a1, 24   /* t8 = srca */
    li           t9, 0x00ff00ff

    addiu        t1, a3, -1
    beqz         t1, 4f       /* last pixel */
     nop

0:
    lw           t0, 0(a2)    /* t0 = mask */
    lw           t1, 4(a2)    /* t1 = mask */
    addiu        a3, a3, -2   /* w = w - 2 */
    or           t2, t0, t1
    beqz         t2, 3f      /* if (t0 == 0) && (t1 == 0) */
     addiu       a2, a2, 8
    and          t2, t0, t1
    beq          t2, t7, 1f  /* if (t0 == 0xffffffff) && (t1 == 0xffffffff) */
     nop

//if(ma)
    lw           t2, 0(a0)    /* t2 = dst */
    lw           t3, 4(a0)    /* t3 = dst */
    MIPS_2xUN8x4_MUL_2xUN8x4 a1, a1, t0, t1, t4, t5, t9, s0, s1, s2, s3, s4, s5
    MIPS_2xUN8x4_MUL_2xUN8   t0, t1, t8, t8, t0, t1, t9, s0, s1, s2, s3, s4, s5
    not          t0, t0
    not          t1, t1
    MIPS_2xUN8x4_MUL_2xUN8x4 t2, t3, t0, t1, t2, t3, t9, s0, s1, s2, s3, s4, s5
    addu_s.qb    t2, t4, t2
    addu_s.qb    t3, t5, t3
    sw           t2, 0(a0)
    sw           t3, 4(a0)
    addiu        t1, a3, -1
    bgtz         t1, 0b
     addiu       a0, a0, 8
    b            4f
     nop
1:
//if (t0 == 0xffffffff) && (t1 == 0xffffffff):
    beq          t8, t6, 2f   /* if (srca == 0xff) */
     nop
    lw           t2, 0(a0)    /* t2 = dst */
    lw           t3, 4(a0)    /* t3 = dst */
    not          t0, a1
    not          t1, a1
    srl          t0, t0, 24
    srl          t1, t1, 24
    MIPS_2xUN8x4_MUL_2xUN8 t2, t3, t0, t1, t2, t3, t9, s0, s1, s2, s3, s4, s5
    addu_s.qb    t2, a1, t2
    addu_s.qb    t3, a1, t3
    sw           t2, 0(a0)
    sw           t3, 4(a0)
    addiu        t1, a3, -1
    bgtz         t1, 0b
     addiu       a0, a0, 8
    b            4f
     nop
2:
    sw           a1, 0(a0)
    sw           a1, 4(a0)
3:
    addiu        t1, a3, -1
    bgtz         t1, 0b
     addiu       a0, a0, 8

4:
    beqz         a3, 7f
     nop
                              /* a1 = src */
    lw           t0, 0(a2)    /* t0 = mask */
    beqz         t0, 7f       /* if (t0 == 0) */
     nop
    beq          t0, t7, 5f  /* if (t0 == 0xffffffff) */
     nop
//if(ma)
    lw           t1, 0(a0)    /* t1 = dst */
    MIPS_UN8x4_MUL_UN8x4  a1, t0, t2, t9, t3, t4, t5, s0
    MIPS_UN8x4_MUL_UN8    t0, t8, t0, t9, t3, t4, t5
    not          t0, t0
    MIPS_UN8x4_MUL_UN8x4  t1, t0, t1, t9, t3, t4, t5, s0
    addu_s.qb    t1, t2, t1
    sw           t1, 0(a0)
    RESTORE_REGS_FROM_STACK 8, s0, s1, s2, s3, s4, s5
    j            ra
     nop
5:
//if (t0 == 0xffffffff)
    beq          t8, t6, 6f   /* if (srca == 0xff) */
     nop
    lw           t1, 0(a0)    /* t1 = dst */
    not          t0, a1
    srl          t0, t0, 24
    MIPS_UN8x4_MUL_UN8 t1, t0, t1, t9, t2, t3, t4
    addu_s.qb    t1, a1, t1
    sw           t1, 0(a0)
    RESTORE_REGS_FROM_STACK 8, s0, s1, s2, s3, s4, s5
    j            ra
     nop
6:
    sw           a1, 0(a0)
7:
    RESTORE_REGS_FROM_STACK 8, s0, s1, s2, s3, s4, s5
8:
    j            ra
     nop

END(pixman_composite_over_n_8888_8888_ca_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_n_8888_0565_ca_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (32bit constant)
 * a2 - mask (a8r8g8b8)
 * a3 - w
 */

    beqz         a3, 8f
     nop
    SAVE_REGS_ON_STACK 20, s0, s1, s2, s3, s4, s5, s6, s7, s8

    li           t6, 0xff
    addiu        t7, zero, -1 /* t7 = 0xffffffff */
    srl          t8, a1, 24   /* t8 = srca */
    li           t9, 0x00ff00ff
    li           s6, 0xf800f800
    li           s7, 0x07e007e0
    li           s8, 0x001F001F

    addiu        t1, a3, -1
    beqz         t1, 4f       /* last pixel */
     nop

0:
    lw           t0, 0(a2)    /* t0 = mask */
    lw           t1, 4(a2)    /* t1 = mask */
    addiu        a3, a3, -2   /* w = w - 2 */
    or           t2, t0, t1
    beqz         t2, 3f      /* if (t0 == 0) && (t1 == 0) */
     addiu       a2, a2, 8
    and          t2, t0, t1
    beq          t2, t7, 1f  /* if (t0 == 0xffffffff) && (t1 == 0xffffffff) */
     nop

//if(ma)
    lhu          t2, 0(a0)    /* t2 = dst */
    lhu          t3, 2(a0)    /* t3 = dst */
    MIPS_2xUN8x4_MUL_2xUN8x4 a1, a1, t0, t1, t4, t5, t9, s0, s1, s2, s3, s4, s5
    MIPS_2xUN8x4_MUL_2xUN8   t0, t1, t8, t8, t0, t1, t9, s0, s1, s2, s3, s4, s5
    not          t0, t0
    not          t1, t1
    CONVERT_2x0565_TO_2x8888 t2, t3, t2, t3, s7, s8, s0, s1, s2, s3
    MIPS_2xUN8x4_MUL_2xUN8x4 t2, t3, t0, t1, t2, t3, t9, s0, s1, s2, s3, s4, s5
    addu_s.qb    t2, t4, t2
    addu_s.qb    t3, t5, t3
    CONVERT_2x8888_TO_2x0565 t2, t3, t2, t3, s6, s7, s8, s0, s1
    sh           t2, 0(a0)
    sh           t3, 2(a0)
    addiu        t1, a3, -1
    bgtz         t1, 0b
     addiu       a0, a0, 4
    b            4f
     nop
1:
//if (t0 == 0xffffffff) && (t1 == 0xffffffff):
    beq          t8, t6, 2f   /* if (srca == 0xff) */
     nop
    lhu          t2, 0(a0)    /* t2 = dst */
    lhu          t3, 2(a0)    /* t3 = dst */
    not          t0, a1
    not          t1, a1
    srl          t0, t0, 24
    srl          t1, t1, 24
    CONVERT_2x0565_TO_2x8888 t2, t3, t2, t3, s7, s8, s0, s1, s2, s3
    MIPS_2xUN8x4_MUL_2xUN8   t2, t3, t0, t1, t2, t3, t9, s0, s1, s2, s3, s4, s5
    addu_s.qb    t2, a1, t2
    addu_s.qb    t3, a1, t3
    CONVERT_2x8888_TO_2x0565 t2, t3, t2, t3, s6, s7, s8, s0, s1
    sh           t2, 0(a0)
    sh           t3, 2(a0)
    addiu        t1, a3, -1
    bgtz         t1, 0b
     addiu       a0, a0, 4
    b            4f
     nop
2:
    CONVERT_1x8888_TO_1x0565 a1, t2, s0, s1
    sh           t2, 0(a0)
    sh           t2, 2(a0)
3:
    addiu        t1, a3, -1
    bgtz         t1, 0b
     addiu       a0, a0, 4

4:
    beqz         a3, 7f
     nop
                              /* a1 = src */
    lw           t0, 0(a2)    /* t0 = mask */
    beqz         t0, 7f       /* if (t0 == 0) */
     nop
    beq          t0, t7, 5f  /* if (t0 == 0xffffffff) */
     nop
//if(ma)
    lhu          t1, 0(a0)    /* t1 = dst */
    MIPS_UN8x4_MUL_UN8x4     a1, t0, t2, t9, t3, t4, t5, s0
    MIPS_UN8x4_MUL_UN8       t0, t8, t0, t9, t3, t4, t5
    not          t0, t0
    CONVERT_1x0565_TO_1x8888 t1, s1, s2, s3
    MIPS_UN8x4_MUL_UN8x4     s1, t0, s1, t9, t3, t4, t5, s0
    addu_s.qb    s1, t2, s1
    CONVERT_1x8888_TO_1x0565 s1, t1, s0, s2
    sh           t1, 0(a0)
    RESTORE_REGS_FROM_STACK 20, s0, s1, s2, s3, s4, s5, s6, s7, s8
    j            ra
     nop
5:
//if (t0 == 0xffffffff)
    beq          t8, t6, 6f   /* if (srca == 0xff) */
     nop
    lhu          t1, 0(a0)    /* t1 = dst */
    not          t0, a1
    srl          t0, t0, 24
    CONVERT_1x0565_TO_1x8888 t1, s1, s2, s3
    MIPS_UN8x4_MUL_UN8       s1, t0, s1, t9, t2, t3, t4
    addu_s.qb    s1, a1, s1
    CONVERT_1x8888_TO_1x0565 s1, t1, s0, s2
    sh           t1, 0(a0)
    RESTORE_REGS_FROM_STACK 20, s0, s1, s2, s3, s4, s5, s6, s7, s8
    j            ra
     nop
6:
    CONVERT_1x8888_TO_1x0565 a1, t1, s0, s2
    sh           t1, 0(a0)
7:
    RESTORE_REGS_FROM_STACK 20, s0, s1, s2, s3, s4, s5, s6, s7, s8
8:
    j            ra
     nop

END(pixman_composite_over_n_8888_0565_ca_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_n_8_8_asm_mips)
/*
 * a0 - dst  (a8)
 * a1 - src  (32bit constant)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, v0
    li                t9, 0x00ff00ff
    beqz              a3, 3f
     nop
    srl               v0, a3, 2   /* v0 = how many multiples of 4 dst pixels */
    beqz              v0, 1f      /* branch if less than 4 src pixels */
     nop

    srl               t8, a1, 24
    replv.ph          t8, t8

0:
    beqz              v0, 1f
     addiu            v0, v0, -1
    lbu               t0, 0(a2)
    lbu               t1, 1(a2)
    lbu               t2, 2(a2)
    lbu               t3, 3(a2)
    lbu               t4, 0(a0)
    lbu               t5, 1(a0)
    lbu               t6, 2(a0)
    lbu               t7, 3(a0)

    addiu             a2, a2, 4

    precr_sra.ph.w    t1, t0, 0
    precr_sra.ph.w    t3, t2, 0
    precr_sra.ph.w    t5, t4, 0
    precr_sra.ph.w    t7, t6, 0

    precr.qb.ph       t0, t3, t1
    precr.qb.ph       t1, t7, t5

    muleu_s.ph.qbl    t2, t0, t8
    muleu_s.ph.qbr    t3, t0, t8
    shra_r.ph         t4, t2, 8
    shra_r.ph         t5, t3, 8
    and               t4, t4, t9
    and               t5, t5, t9
    addq.ph           t2, t2, t4
    addq.ph           t3, t3, t5
    shra_r.ph         t2, t2, 8
    shra_r.ph         t3, t3, 8
    precr.qb.ph       t0, t2, t3
    not               t6, t0

    preceu.ph.qbl     t7, t6
    preceu.ph.qbr     t6, t6

    muleu_s.ph.qbl    t2, t1, t7
    muleu_s.ph.qbr    t3, t1, t6
    shra_r.ph         t4, t2, 8
    shra_r.ph         t5, t3, 8
    and               t4, t4, t9
    and               t5, t5, t9
    addq.ph           t2, t2, t4
    addq.ph           t3, t3, t5
    shra_r.ph         t2, t2, 8
    shra_r.ph         t3, t3, 8
    precr.qb.ph       t1, t2, t3

    addu_s.qb         t2, t0, t1

    sb                t2, 0(a0)
    srl               t2, t2, 8
    sb                t2, 1(a0)
    srl               t2, t2, 8
    sb                t2, 2(a0)
    srl               t2, t2, 8
    sb                t2, 3(a0)
    addiu             a3, a3, -4
    b                 0b
     addiu            a0, a0, 4

1:
    beqz              a3, 3f
     nop
    srl               t8, a1, 24
2:
    lbu               t0, 0(a2)
    lbu               t1, 0(a0)
    addiu             a2, a2, 1

    mul               t2, t0, t8
    shra_r.ph         t3, t2, 8
    andi              t3, t3, 0x00ff
    addq.ph           t2, t2, t3
    shra_r.ph         t2, t2, 8
    not               t3, t2
    andi              t3, t3, 0x00ff


    mul               t4, t1, t3
    shra_r.ph         t5, t4, 8
    andi              t5, t5, 0x00ff
    addq.ph           t4, t4, t5
    shra_r.ph         t4, t4, 8
    andi              t4, t4, 0x00ff

    addu_s.qb         t2, t2, t4
    sb                t2, 0(a0)
    addiu             a3, a3, -1
    bnez              a3, 2b
     addiu            a0, a0, 1

3:
    RESTORE_REGS_FROM_STACK 0, v0
    j                 ra
     nop

END(pixman_composite_over_n_8_8_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_n_8_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (32bit constant)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 4, s0, s1, s2, s3, s4
    beqz      a3, 4f
     nop
    li        t4, 0x00ff00ff
    li        t5, 0xff
    addiu     t0, a3, -1
    beqz      t0, 3f         /* last pixel */
     srl      t6, a1, 24     /* t6 = srca */
    not       s4, a1
    beq       t5, t6, 2f     /* if (srca == 0xff) */
     srl      s4, s4, 24
1:
                             /* a1 = src */
    lbu       t0, 0(a2)      /* t0 = mask */
    lbu       t1, 1(a2)      /* t1 = mask */
    or        t2, t0, t1
    beqz      t2, 111f       /* if (t0 == 0) && (t1 == 0) */
     addiu    a2, a2, 2
    and       t3, t0, t1

    lw        t2, 0(a0)      /* t2 = dst */
    beq       t3, t5, 11f    /* if (t0 == 0xff) && (t1 == 0xff) */
     lw       t3, 4(a0)      /* t3 = dst */

    MIPS_2xUN8x4_MUL_2xUN8 a1, a1, t0, t1, s0, s1, t4, t6, t7, t8, t9, s2, s3
    not       s2, s0
    not       s3, s1
    srl       s2, s2, 24
    srl       s3, s3, 24
    MIPS_2xUN8x4_MUL_2xUN8 t2, t3, s2, s3, t2, t3, t4, t0, t1, t6, t7, t8, t9
    addu_s.qb s2, t2, s0
    addu_s.qb s3, t3, s1
    sw        s2, 0(a0)
    b         111f
     sw       s3, 4(a0)
11:
    MIPS_2xUN8x4_MUL_2xUN8 t2, t3, s4, s4, t2, t3, t4, t0, t1, t6, t7, t8, t9
    addu_s.qb s2, t2, a1
    addu_s.qb s3, t3, a1
    sw        s2, 0(a0)
    sw        s3, 4(a0)

111:
    addiu     a3, a3, -2
    addiu     t0, a3, -1
    bgtz      t0, 1b
     addiu    a0, a0, 8
    b         3f
     nop
2:
                             /* a1 = src */
    lbu       t0, 0(a2)      /* t0 = mask */
    lbu       t1, 1(a2)      /* t1 = mask */
    or        t2, t0, t1
    beqz      t2, 222f       /* if (t0 == 0) && (t1 == 0) */
     addiu    a2, a2, 2
    and       t3, t0, t1
    beq       t3, t5, 22f    /* if (t0 == 0xff) && (t1 == 0xff) */
     nop
    lw        t2, 0(a0)      /* t2 = dst */
    lw        t3, 4(a0)      /* t3 = dst */

    OVER_2x8888_2x8_2x8888 a1, a1, t0, t1, t2, t3, \
                           t6, t7, t4, t8, t9, s0, s1, s2, s3
    sw        t6, 0(a0)
    b         222f
     sw        t7, 4(a0)
22:
    sw        a1, 0(a0)
    sw        a1, 4(a0)
222:
    addiu     a3, a3, -2
    addiu     t0, a3, -1
    bgtz      t0, 2b
     addiu    a0, a0, 8
3:
    blez      a3, 4f
     nop
                             /* a1 = src */
    lbu       t0, 0(a2)      /* t0 = mask */
    beqz      t0, 4f         /* if (t0 == 0) */
     addiu    a2, a2, 1
    move      t3, a1
    beq       t0, t5, 31f    /* if (t0 == 0xff) */
     lw       t1, 0(a0)      /* t1 = dst */

    MIPS_UN8x4_MUL_UN8 a1, t0, t3, t4, t6, t7, t8
31:
    not       t2, t3
    srl       t2, t2, 24
    MIPS_UN8x4_MUL_UN8 t1, t2, t1, t4, t6, t7, t8
    addu_s.qb t2, t1, t3
    sw        t2, 0(a0)
4:
    RESTORE_REGS_FROM_STACK 4, s0, s1, s2, s3, s4
    j         ra
     nop

END(pixman_composite_over_n_8_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_n_8_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (32bit constant)
 * a2 - mask (a8)
 * a3 - w
 */
    SAVE_REGS_ON_STACK 24, v0, s0, s1, s2, s3, s4, s5, s6, s7, s8
    beqz     a3, 4f
     nop
    li       t4, 0x00ff00ff
    li       t5, 0xff
    li       t6, 0xf800f800
    li       t7, 0x07e007e0
    li       t8, 0x001F001F
    addiu    t1, a3, -1
    beqz     t1, 3f         /* last pixel */
     srl     t0, a1, 24     /* t0 = srca */
    not      v0, a1
    beq      t0, t5, 2f     /* if (srca == 0xff) */
     srl     v0, v0, 24
1:
                            /* a1 = src */
    lbu      t0, 0(a2)      /* t0 = mask */
    lbu      t1, 1(a2)      /* t1 = mask */
    or       t2, t0, t1
    beqz     t2, 111f       /* if (t0 == 0) && (t1 == 0) */
     addiu   a2, a2, 2
    lhu      t2, 0(a0)      /* t2 = dst */
    lhu      t3, 2(a0)      /* t3 = dst */
    CONVERT_2x0565_TO_2x8888 t2, t3, s0, s1, t7, t8, t9, s2, s3, s4
    and      t9, t0, t1
    beq      t9, t5, 11f    /* if (t0 == 0xff) && (t1 == 0xff) */
     nop

    MIPS_2xUN8x4_MUL_2xUN8   a1, a1, t0, t1, s2, s3, t4, t9, s4, s5, s6, s7, s8
    not      s4, s2
    not      s5, s3
    srl      s4, s4, 24
    srl      s5, s5, 24
    MIPS_2xUN8x4_MUL_2xUN8   s0, s1, s4, s5, s0, s1, t4, t9, t0, t1, s6, s7, s8
    addu_s.qb                s4, s2, s0
    addu_s.qb                s5, s3, s1
    CONVERT_2x8888_TO_2x0565 s4, s5, t2, t3, t6, t7, t8, s0, s1
    sh       t2, 0(a0)
    b        111f
     sh      t3, 2(a0)
11:
    MIPS_2xUN8x4_MUL_2xUN8   s0, s1, v0, v0, s0, s1, t4, t9, t0, t1, s6, s7, s8
    addu_s.qb                s4, a1, s0
    addu_s.qb                s5, a1, s1
    CONVERT_2x8888_TO_2x0565 s4, s5, t2, t3, t6, t7, t8, s0, s1
    sh       t2, 0(a0)
    sh       t3, 2(a0)
111:
    addiu    a3, a3, -2
    addiu    t0, a3, -1
    bgtz     t0, 1b
     addiu   a0, a0, 4
    b        3f
     nop
2:
    CONVERT_1x8888_TO_1x0565 a1, s0, s1, s2
21:
                            /* a1 = src */
    lbu      t0, 0(a2)      /* t0 = mask */
    lbu      t1, 1(a2)      /* t1 = mask */
    or       t2, t0, t1
    beqz     t2, 222f       /* if (t0 == 0) && (t1 == 0) */
     addiu   a2, a2, 2
    and      t9, t0, t1
    move     s2, s0
    beq      t9, t5, 22f    /* if (t0 == 0xff) && (t2 == 0xff) */
     move    s3, s0
    lhu      t2, 0(a0)      /* t2 = dst */
    lhu      t3, 2(a0)      /* t3 = dst */

    CONVERT_2x0565_TO_2x8888 t2, t3, s2, s3, t7, t8, s4, s5, s6, s7
    OVER_2x8888_2x8_2x8888   a1, a1, t0, t1, s2, s3, \
                             t2, t3, t4, t9, s4, s5, s6, s7, s8
    CONVERT_2x8888_TO_2x0565 t2, t3, s2, s3, t6, t7, t8, s4, s5
22:
    sh       s2, 0(a0)
    sh       s3, 2(a0)
222:
    addiu    a3, a3, -2
    addiu    t0, a3, -1
    bgtz     t0, 21b
     addiu   a0, a0, 4
3:
    blez      a3, 4f
     nop
                            /* a1 = src */
    lbu      t0, 0(a2)      /* t0 = mask */
    beqz     t0, 4f         /* if (t0 == 0) */
     nop
    lhu      t1, 0(a0)      /* t1 = dst */
    CONVERT_1x0565_TO_1x8888 t1, t2, t3, t7
    beq      t0, t5, 31f    /* if (t0 == 0xff) */
     move    t3, a1

    MIPS_UN8x4_MUL_UN8       a1, t0, t3, t4, t7, t8, t9
31:
    not      t6, t3
    srl      t6, t6, 24
    MIPS_UN8x4_MUL_UN8       t2, t6, t2, t4, t7, t8, t9
    addu_s.qb                t1, t2, t3
    CONVERT_1x8888_TO_1x0565 t1, t2, t3, t7
    sh       t2, 0(a0)
4:
    RESTORE_REGS_FROM_STACK  24, v0, s0, s1, s2, s3, s4, s5, s6, s7, s8
    j        ra
     nop

END(pixman_composite_over_n_8_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_8888_n_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8r8g8b8)
 * a2 - mask (32bit constant)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, s0
    li       t4, 0x00ff00ff
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    srl      a2, a2, 24
    beqz     t1, 2f
     nop

1:
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 4(a1) /* t1 = source      (a8r8g8b8) */
                       /* a2 = mask        (32bit constant) */
    lw       t2, 0(a0) /* t2 = destination (a8r8g8b8) */
    lw       t3, 4(a0) /* t3 = destination (a8r8g8b8) */
    addiu    a1, a1, 8

    OVER_2x8888_2x8_2x8888 t0, t1, a2, a2, t2, t3, \
                           t5, t6, t4, t7, t8, t9, t0, t1, s0

    sw       t5, 0(a0)
    sw       t6, 4(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a3, 3f
     nop
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
                       /* a2 = mask        (32bit constant) */
    lw       t1, 0(a0) /* t1 = destination (a8r8g8b8) */

    OVER_8888_8_8888 t0, a2, t1, t3, t4, t5, t6, t7, t8

    sw       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0
    j        ra
     nop

END(pixman_composite_over_8888_n_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_8888_n_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (a8r8g8b8)
 * a2 - mask (32bit constant)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2, s3
    li       t6, 0x00ff00ff
    li       t7, 0xf800f800
    li       t8, 0x07e007e0
    li       t9, 0x001F001F
    beqz     a3, 3f
     nop
    srl      a2, a2, 24
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 4(a1) /* t1 = source      (a8r8g8b8) */
                       /* a2 = mask        (32bit constant) */
    lhu      t2, 0(a0) /* t2 = destination (r5g6b5) */
    lhu      t3, 2(a0) /* t2 = destination (r5g6b5) */
    addiu    a1, a1, 8

    CONVERT_2x0565_TO_2x8888 t2, t3, t4, t5, t8, t9, s0, s1, t2, t3
    OVER_2x8888_2x8_2x8888   t0, t1, a2, a2, t4, t5, \
                             t2, t3, t6, t0, t1, s0, s1, s2, s3
    CONVERT_2x8888_TO_2x0565 t2, t3, t4, t5, t7, t8, t9, s0, s1

    sh       t4, 0(a0)
    sh       t5, 2(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a3, 3f
     nop
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
                       /* a2 = mask        (32bit constant) */
    lhu      t1, 0(a0) /* t1 = destination (r5g6b5) */

    CONVERT_1x0565_TO_1x8888 t1, t2, t4, t5
    OVER_8888_8_8888         t0, a2, t2, t1, t6, t3, t4, t5, t7
    CONVERT_1x8888_TO_1x0565 t1, t3, t4, t5

    sh       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2, s3
    j                 ra
     nop

END(pixman_composite_over_8888_n_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_0565_n_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (r5g6b5)
 * a2 - mask (32bit constant)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 20, s0, s1, s2, s3, s4, s5
    li       t6, 0x00ff00ff
    li       t7, 0xf800f800
    li       t8, 0x07e007e0
    li       t9, 0x001F001F
    beqz     a3, 3f
     nop
    srl      a2, a2, 24
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lhu      t0, 0(a1) /* t0 = source      (r5g6b5) */
    lhu      t1, 2(a1) /* t1 = source      (r5g6b5) */
                       /* a2 = mask        (32bit constant) */
    lhu      t2, 0(a0) /* t2 = destination (r5g6b5) */
    lhu      t3, 2(a0) /* t3 = destination (r5g6b5) */
    addiu    a1, a1, 4

    CONVERT_2x0565_TO_2x8888 t0, t1, t4, t5, t8, t9, s0, s1, s2, s3
    CONVERT_2x0565_TO_2x8888 t2, t3, s0, s1, t8, t9, s2, s3, s4, s5
    OVER_2x8888_2x8_2x8888   t4, t5, a2, a2, s0, s1, \
                             t0, t1, t6, s2, s3, s4, s5, t4, t5
    CONVERT_2x8888_TO_2x0565 t0, t1, s0, s1, t7, t8, t9, s2, s3

    sh       s0, 0(a0)
    sh       s1, 2(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a3, 3f
     nop
    lhu      t0, 0(a1) /* t0 = source      (r5g6b5) */
                       /* a2 = mask        (32bit constant) */
    lhu      t1, 0(a0) /* t1 = destination (r5g6b5) */

    CONVERT_1x0565_TO_1x8888 t0, t2, t4, t5
    CONVERT_1x0565_TO_1x8888 t1, t3, t4, t5
    OVER_8888_8_8888         t2, a2, t3, t0, t6, t1, t4, t5, t7
    CONVERT_1x8888_TO_1x0565 t0, t3, t4, t5

    sh       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 20, s0, s1, s2, s3, s4, s5
    j        ra
     nop

END(pixman_composite_over_0565_n_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_8888_8_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8r8g8b8)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, s0, s1
    li       t4, 0x00ff00ff
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 4(a1) /* t1 = source      (a8r8g8b8) */
    lbu      t2, 0(a2) /* t2 = mask        (a8) */
    lbu      t3, 1(a2) /* t3 = mask        (a8) */
    lw       t5, 0(a0) /* t5 = destination (a8r8g8b8) */
    lw       t6, 4(a0) /* t6 = destination (a8r8g8b8) */
    addiu    a1, a1, 8
    addiu    a2, a2, 2

    OVER_2x8888_2x8_2x8888 t0, t1, t2, t3, t5, t6, \
                           t7, t8, t4, t9, s0, s1, t0, t1, t2

    sw       t7, 0(a0)
    sw       t8, 4(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a3, 3f
     nop
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lbu      t1, 0(a2) /* t1 = mask        (a8) */
    lw       t2, 0(a0) /* t2 = destination (a8r8g8b8) */

    OVER_8888_8_8888 t0, t1, t2, t3, t4, t5, t6, t7, t8

    sw       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1
    j        ra
     nop

END(pixman_composite_over_8888_8_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_8888_8_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (a8r8g8b8)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 20, s0, s1, s2, s3, s4, s5
    li       t6, 0x00ff00ff
    li       t7, 0xf800f800
    li       t8, 0x07e007e0
    li       t9, 0x001F001F
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 4(a1) /* t1 = source      (a8r8g8b8) */
    lbu      t2, 0(a2) /* t2 = mask        (a8) */
    lbu      t3, 1(a2) /* t3 = mask        (a8) */
    lhu      t4, 0(a0) /* t4 = destination (r5g6b5) */
    lhu      t5, 2(a0) /* t5 = destination (r5g6b5) */
    addiu    a1, a1, 8
    addiu    a2, a2, 2

    CONVERT_2x0565_TO_2x8888 t4, t5, s0, s1, t8, t9, s2, s3, s4, s5
    OVER_2x8888_2x8_2x8888   t0, t1, t2, t3, s0, s1, \
                             t4, t5, t6, s2, s3, s4, s5, t0, t1
    CONVERT_2x8888_TO_2x0565 t4, t5, s0, s1, t7, t8, t9, s2, s3

    sh       s0, 0(a0)
    sh       s1, 2(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a3, 3f
     nop
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lbu      t1, 0(a2) /* t1 = mask        (a8) */
    lhu      t2, 0(a0) /* t2 = destination (r5g6b5) */

    CONVERT_1x0565_TO_1x8888 t2, t3, t4, t5
    OVER_8888_8_8888         t0, t1, t3, t2, t6, t4, t5, t7, t8
    CONVERT_1x8888_TO_1x0565 t2, t3, t4, t5

    sh       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 20, s0, s1, s2, s3, s4, s5
    j        ra
     nop

END(pixman_composite_over_8888_8_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_0565_8_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (r5g6b5)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 20, s0, s1, s2, s3, s4, s5
    li       t4, 0xf800f800
    li       t5, 0x07e007e0
    li       t6, 0x001F001F
    li       t7, 0x00ff00ff
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lhu      t0, 0(a1) /* t0 = source      (r5g6b5) */
    lhu      t1, 2(a1) /* t1 = source      (r5g6b5) */
    lbu      t2, 0(a2) /* t2 = mask        (a8) */
    lbu      t3, 1(a2) /* t3 = mask        (a8) */
    lhu      t8, 0(a0) /* t8 = destination (r5g6b5) */
    lhu      t9, 2(a0) /* t9 = destination (r5g6b5) */
    addiu    a1, a1, 4
    addiu    a2, a2, 2

    CONVERT_2x0565_TO_2x8888 t0, t1, s0, s1, t5, t6, s2, s3, s4, s5
    CONVERT_2x0565_TO_2x8888 t8, t9, s2, s3, t5, t6, s4, s5, t0, t1
    OVER_2x8888_2x8_2x8888   s0, s1, t2, t3, s2, s3, \
                             t0, t1, t7, s4, s5, t8, t9, s0, s1
    CONVERT_2x8888_TO_2x0565 t0, t1, s0, s1, t4, t5, t6, s2, s3

    sh       s0, 0(a0)
    sh       s1, 2(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a3, 3f
     nop
    lhu      t0, 0(a1) /* t0 = source      (r5g6b5) */
    lbu      t1, 0(a2) /* t1 = mask        (a8) */
    lhu      t2, 0(a0) /* t2 = destination (r5g6b5) */

    CONVERT_1x0565_TO_1x8888 t0, t3, t4, t5
    CONVERT_1x0565_TO_1x8888 t2, t4, t5, t6
    OVER_8888_8_8888         t3, t1, t4, t0, t7, t2, t5, t6, t8
    CONVERT_1x8888_TO_1x0565 t0, t3, t4, t5

    sh       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 20, s0, s1, s2, s3, s4, s5
    j        ra
     nop

END(pixman_composite_over_0565_8_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_8888_8888_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8r8g8b8)
 * a2 - mask (a8r8g8b8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2
    li       t4, 0x00ff00ff
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 4(a1) /* t1 = source      (a8r8g8b8) */
    lw       t2, 0(a2) /* t2 = mask        (a8r8g8b8) */
    lw       t3, 4(a2) /* t3 = mask        (a8r8g8b8) */
    lw       t5, 0(a0) /* t5 = destination (a8r8g8b8) */
    lw       t6, 4(a0) /* t6 = destination (a8r8g8b8) */
    addiu    a1, a1, 8
    addiu    a2, a2, 8
    srl      t2, t2, 24
    srl      t3, t3, 24

    OVER_2x8888_2x8_2x8888 t0, t1, t2, t3, t5, t6, t7, t8, t4, t9, s0, s1, s2, t0, t1

    sw       t7, 0(a0)
    sw       t8, 4(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a3, 3f
     nop
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 0(a2) /* t1 = mask        (a8r8g8b8) */
    lw       t2, 0(a0) /* t2 = destination (a8r8g8b8) */
    srl      t1, t1, 24

    OVER_8888_8_8888 t0, t1, t2, t3, t4, t5, t6, t7, t8

    sw       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2
    j        ra
     nop

END(pixman_composite_over_8888_8888_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_8888_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8r8g8b8)
 * a2 - w
 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2
    li           t4, 0x00ff00ff
    beqz         a2, 3f
     nop
    addiu        t1, a2, -1
    beqz         t1, 2f
     nop
1:
    lw           t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw           t1, 4(a1) /* t1 = source      (a8r8g8b8) */
    lw           t2, 0(a0) /* t2 = destination (a8r8g8b8) */
    lw           t3, 4(a0) /* t3 = destination (a8r8g8b8) */
    addiu        a1, a1, 8

    not          t5, t0
    srl          t5, t5, 24
    not          t6, t1
    srl          t6, t6, 24

    or           t7, t5, t6
    beqz         t7, 11f
     or          t8, t0, t1
    beqz         t8, 12f

    MIPS_2xUN8x4_MUL_2xUN8 t2, t3, t5, t6, t7, t8, t4, t9, s0, s1, s2, t2, t3

    addu_s.qb    t0, t7, t0
    addu_s.qb    t1, t8, t1
11:
    sw           t0, 0(a0)
    sw           t1, 4(a0)
12:
    addiu        a2, a2, -2
    addiu        t1, a2, -1
    bgtz         t1, 1b
     addiu       a0, a0, 8
2:
    beqz         a2, 3f
     nop

    lw           t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw           t1, 0(a0) /* t1 = destination (a8r8g8b8) */
    addiu        a1, a1, 4

    not          t2, t0
    srl          t2, t2, 24

    beqz         t2, 21f
     nop
    beqz         t0, 3f

    MIPS_UN8x4_MUL_UN8 t1, t2, t3, t4, t5, t6, t7

    addu_s.qb    t0, t3, t0
21:
    sw           t0, 0(a0)

3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2
    j            ra
     nop

END(pixman_composite_over_8888_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_8888_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (a8r8g8b8)
 * a2 - w
 */

    SAVE_REGS_ON_STACK 8, s0, s1, s2, s3, s4, s5
    li           t4, 0x00ff00ff
    li           s3, 0xf800f800
    li           s4, 0x07e007e0
    li           s5, 0x001F001F
    beqz         a2, 3f
     nop
    addiu        t1, a2, -1
    beqz         t1, 2f
     nop
1:
    lw           t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw           t1, 4(a1) /* t1 = source      (a8r8g8b8) */
    lhu          t2, 0(a0) /* t2 = destination (r5g6b5) */
    lhu          t3, 2(a0) /* t3 = destination (r5g6b5) */
    addiu        a1, a1, 8

    not          t5, t0
    srl          t5, t5, 24
    not          t6, t1
    srl          t6, t6, 24

    or           t7, t5, t6
    beqz         t7, 11f
     or          t8, t0, t1
    beqz         t8, 12f

    CONVERT_2x0565_TO_2x8888 t2, t3, s0, s1, s4, s5, t7, t8, t9, s2
    MIPS_2xUN8x4_MUL_2xUN8   s0, s1, t5, t6, t7, t8, t4, t9, t2, t3, s2, s0, s1

    addu_s.qb    t0, t7, t0
    addu_s.qb    t1, t8, t1
11:
    CONVERT_2x8888_TO_2x0565 t0, t1, t7, t8, s3, s4, s5, t2, t3
    sh           t7, 0(a0)
    sh           t8, 2(a0)
12:
    addiu        a2, a2, -2
    addiu        t1, a2, -1
    bgtz         t1, 1b
     addiu       a0, a0, 4
2:
    beqz         a2, 3f
     nop

    lw           t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lhu          t1, 0(a0) /* t1 = destination (r5g6b5) */
    addiu        a1, a1, 4

    not          t2, t0
    srl          t2, t2, 24

    beqz         t2, 21f
     nop
    beqz         t0, 3f

    CONVERT_1x0565_TO_1x8888 t1, s0, t8, t9
    MIPS_UN8x4_MUL_UN8       s0, t2, t3, t4, t5, t6, t7

    addu_s.qb    t0, t3, t0
21:
    CONVERT_1x8888_TO_1x0565 t0, s0, t8, t9
    sh           s0, 0(a0)

3:
    RESTORE_REGS_FROM_STACK 8, s0, s1, s2, s3, s4, s5
    j            ra
     nop

END(pixman_composite_over_8888_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_n_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (32bit constant)
 * a2 - w
 */

    beqz         a2, 5f
     nop

    not          t0, a1
    srl          t0, t0, 24
    bgtz         t0, 1f
     nop
    CONVERT_1x8888_TO_1x0565 a1, t1, t2, t3
0:
    sh           t1, 0(a0)
    addiu        a2, a2, -1
    bgtz         a2, 0b
     addiu       a0, a0, 2
    j            ra
     nop

1:
    SAVE_REGS_ON_STACK 0, s0, s1, s2
    li           t4, 0x00ff00ff
    li           t5, 0xf800f800
    li           t6, 0x07e007e0
    li           t7, 0x001F001F
    addiu        t1, a2, -1
    beqz         t1, 3f
     nop
2:
    lhu          t1, 0(a0) /* t1 = destination (r5g6b5) */
    lhu          t2, 2(a0) /* t2 = destination (r5g6b5) */

    CONVERT_2x0565_TO_2x8888 t1, t2, t3, t8, t6, t7, t9, s0, s1, s2
    MIPS_2xUN8x4_MUL_2xUN8   t3, t8, t0, t0, t1, t2, t4, t9, s0, s1, s2, t3, t8
    addu_s.qb                t1, t1, a1
    addu_s.qb                t2, t2, a1
    CONVERT_2x8888_TO_2x0565 t1, t2, t3, t8, t5, t6, t7, s0, s1

    sh           t3, 0(a0)
    sh           t8, 2(a0)

    addiu        a2, a2, -2
    addiu        t1, a2, -1
    bgtz         t1, 2b
     addiu       a0, a0, 4
3:
    beqz         a2, 4f
     nop

    lhu          t1, 0(a0) /* t1 = destination (r5g6b5) */

    CONVERT_1x0565_TO_1x8888 t1, t2, s0, s1
    MIPS_UN8x4_MUL_UN8       t2, t0, t1, t4, s0, s1, s2
    addu_s.qb                t1, t1, a1
    CONVERT_1x8888_TO_1x0565 t1, t2, s0, s1

    sh           t2, 0(a0)

4:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2
5:
    j            ra
     nop

END(pixman_composite_over_n_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_n_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (32bit constant)
 * a2 - w
 */

    beqz         a2, 5f
     nop

    not          t0, a1
    srl          t0, t0, 24
    bgtz         t0, 1f
     nop
0:
    sw           a1, 0(a0)
    addiu        a2, a2, -1
    bgtz         a2, 0b
     addiu       a0, a0, 4
    j            ra
     nop

1:
    SAVE_REGS_ON_STACK 0, s0, s1, s2
    li           t4, 0x00ff00ff
    addiu        t1, a2, -1
    beqz         t1, 3f
     nop
2:
    lw           t2, 0(a0) /* t2 = destination (a8r8g8b8) */
    lw           t3, 4(a0) /* t3 = destination (a8r8g8b8) */

    MIPS_2xUN8x4_MUL_2xUN8 t2, t3, t0, t0, t7, t8, t4, t9, s0, s1, s2, t2, t3

    addu_s.qb    t7, t7, a1
    addu_s.qb    t8, t8, a1

    sw           t7, 0(a0)
    sw           t8, 4(a0)

    addiu        a2, a2, -2
    addiu        t1, a2, -1
    bgtz         t1, 2b
     addiu       a0, a0, 8
3:
    beqz         a2, 4f
     nop

    lw           t1, 0(a0) /* t1 = destination (a8r8g8b8) */

    MIPS_UN8x4_MUL_UN8 t1, t0, t3, t4, t5, t6, t7

    addu_s.qb    t3, t3, a1

    sw           t3, 0(a0)

4:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2
5:
    j            ra
     nop

END(pixman_composite_over_n_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_8_8_8_asm_mips)
/*
 * a0 - dst  (a8)
 * a1 - src  (a8)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, v0, v1
    li                t9, 0x00ff00ff
    beqz              a3, 3f
     nop

    srl               v0, a3, 2   /* v0 = how many multiples of 4 dst pixels */
    beqz              v0, 1f      /* branch if less than 4 src pixels */
     nop

0:
    beqz              v0, 1f
     addiu            v0, v0, -1
    lbu               t0, 0(a2)
    lbu               t1, 1(a2)
    lbu               t2, 2(a2)
    lbu               t3, 3(a2)
    lbu               t4, 0(a0)
    lbu               t5, 1(a0)
    lbu               t6, 2(a0)
    lbu               t7, 3(a0)

    addiu             a2, a2, 4

    precr_sra.ph.w    t1, t0, 0
    precr_sra.ph.w    t3, t2, 0
    precr_sra.ph.w    t5, t4, 0
    precr_sra.ph.w    t7, t6, 0

    precr.qb.ph       t0, t3, t1
    precr.qb.ph       t1, t7, t5

    lbu               t4, 0(a1)
    lbu               v1, 1(a1)
    lbu               t7, 2(a1)
    lbu               t8, 3(a1)

    addiu             a1, a1, 4

    precr_sra.ph.w    v1, t4, 0
    precr_sra.ph.w    t8, t7, 0

    muleu_s.ph.qbl    t2, t0, t8
    muleu_s.ph.qbr    t3, t0, v1
    shra_r.ph         t4, t2, 8
    shra_r.ph         t5, t3, 8
    and               t4, t4, t9
    and               t5, t5, t9
    addq.ph           t2, t2, t4
    addq.ph           t3, t3, t5
    shra_r.ph         t2, t2, 8
    shra_r.ph         t3, t3, 8
    precr.qb.ph       t0, t2, t3

    addu_s.qb         t2, t0, t1

    sb                t2, 0(a0)
    srl               t2, t2, 8
    sb                t2, 1(a0)
    srl               t2, t2, 8
    sb                t2, 2(a0)
    srl               t2, t2, 8
    sb                t2, 3(a0)
    addiu             a3, a3, -4
    b                 0b
     addiu            a0, a0, 4

1:
    beqz              a3, 3f
     nop
2:
    lbu               t8, 0(a1)
    lbu               t0, 0(a2)
    lbu               t1, 0(a0)
    addiu             a1, a1, 1
    addiu             a2, a2, 1

    mul               t2, t0, t8
    shra_r.ph         t3, t2, 8
    andi              t3, t3, 0xff
    addq.ph           t2, t2, t3
    shra_r.ph         t2, t2, 8
    andi              t2, t2, 0xff

    addu_s.qb         t2, t2, t1
    sb                t2, 0(a0)
    addiu             a3, a3, -1
    bnez              a3, 2b
     addiu            a0, a0, 1

3:
    RESTORE_REGS_FROM_STACK 0, v0, v1
    j                 ra
     nop

END(pixman_composite_add_8_8_8_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_n_8_8_asm_mips)
/*
 * a0 - dst  (a8)
 * a1 - src  (32bit constant)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, v0
    li                t9, 0x00ff00ff
    beqz              a3, 3f
     nop

    srl               v0, a3, 2   /* v0 = how many multiples of 4 dst pixels */
    beqz              v0, 1f      /* branch if less than 4 src pixels */
     nop

    srl               t8, a1, 24
    replv.ph          t8, t8

0:
    beqz              v0, 1f
     addiu            v0, v0, -1
    lbu               t0, 0(a2)
    lbu               t1, 1(a2)
    lbu               t2, 2(a2)
    lbu               t3, 3(a2)
    lbu               t4, 0(a0)
    lbu               t5, 1(a0)
    lbu               t6, 2(a0)
    lbu               t7, 3(a0)

    addiu             a2, a2, 4

    precr_sra.ph.w    t1, t0, 0
    precr_sra.ph.w    t3, t2, 0
    precr_sra.ph.w    t5, t4, 0
    precr_sra.ph.w    t7, t6, 0

    precr.qb.ph       t0, t3, t1
    precr.qb.ph       t1, t7, t5

    muleu_s.ph.qbl    t2, t0, t8
    muleu_s.ph.qbr    t3, t0, t8
    shra_r.ph         t4, t2, 8
    shra_r.ph         t5, t3, 8
    and               t4, t4, t9
    and               t5, t5, t9
    addq.ph           t2, t2, t4
    addq.ph           t3, t3, t5
    shra_r.ph         t2, t2, 8
    shra_r.ph         t3, t3, 8
    precr.qb.ph       t0, t2, t3

    addu_s.qb         t2, t0, t1

    sb                t2, 0(a0)
    srl               t2, t2, 8
    sb                t2, 1(a0)
    srl               t2, t2, 8
    sb                t2, 2(a0)
    srl               t2, t2, 8
    sb                t2, 3(a0)
    addiu             a3, a3, -4
    b                 0b
     addiu            a0, a0, 4

1:
    beqz              a3, 3f
     nop
    srl               t8, a1, 24
2:
    lbu               t0, 0(a2)
    lbu               t1, 0(a0)
    addiu             a2, a2, 1

    mul               t2, t0, t8
    shra_r.ph         t3, t2, 8
    andi              t3, t3, 0xff
    addq.ph           t2, t2, t3
    shra_r.ph         t2, t2, 8
    andi              t2, t2, 0xff

    addu_s.qb         t2, t2, t1
    sb                t2, 0(a0)
    addiu             a3, a3, -1
    bnez              a3, 2b
     addiu            a0, a0, 1

3:
    RESTORE_REGS_FROM_STACK 0, v0
    j                 ra
     nop

END(pixman_composite_add_n_8_8_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_n_8_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (32bit constant)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2
    li       t4, 0x00ff00ff
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
                       /* a1 = source      (32bit constant) */
    lbu      t0, 0(a2) /* t0 = mask        (a8) */
    lbu      t1, 1(a2) /* t1 = mask        (a8) */
    lw       t2, 0(a0) /* t2 = destination (a8r8g8b8) */
    lw       t3, 4(a0) /* t3 = destination (a8r8g8b8) */
    addiu    a2, a2, 2

    MIPS_2xUN8x4_MUL_2xUN8_ADD_2xUN8x4 a1, a1, \
                                       t0, t1, \
                                       t2, t3, \
                                       t5, t6, \
                                       t4, t7, t8, t9, s0, s1, s2

    sw       t5, 0(a0)
    sw       t6, 4(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a3, 3f
     nop
                       /* a1 = source      (32bit constant) */
    lbu      t0, 0(a2) /* t0 = mask        (a8) */
    lw       t1, 0(a0) /* t1 = destination (a8r8g8b8) */

    MIPS_UN8x4_MUL_UN8_ADD_UN8x4 a1, t0, t1, t2, t4, t3, t5, t6

    sw       t2, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2
    j        ra
     nop

END(pixman_composite_add_n_8_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_0565_8_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (r5g6b5)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 20, s0, s1, s2, s3, s4, s5, s6, s7
    li       t4, 0xf800f800
    li       t5, 0x07e007e0
    li       t6, 0x001F001F
    li       t7, 0x00ff00ff
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lhu      t0, 0(a1) /* t0 = source      (r5g6b5) */
    lhu      t1, 2(a1) /* t1 = source      (r5g6b5) */
    lbu      t2, 0(a2) /* t2 = mask        (a8) */
    lbu      t3, 1(a2) /* t3 = mask        (a8) */
    lhu      t8, 0(a0) /* t8 = destination (r5g6b5) */
    lhu      t9, 2(a0) /* t9 = destination (r5g6b5) */
    addiu    a1, a1, 4
    addiu    a2, a2, 2

    CONVERT_2x0565_TO_2x8888  t0, t1, s0, s1, t5, t6, s2, s3, s4, s5
    CONVERT_2x0565_TO_2x8888  t8, t9, s2, s3, t5, t6, s4, s5, s6, s7
    MIPS_2xUN8x4_MUL_2xUN8_ADD_2xUN8x4  s0, s1, \
                                        t2, t3, \
                                        s2, s3, \
                                        t0, t1, \
                                        t7, s4, s5, s6, s7, t8, t9
    CONVERT_2x8888_TO_2x0565  t0, t1, s0, s1, t4, t5, t6, s2, s3

    sh       s0, 0(a0)
    sh       s1, 2(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a3, 3f
     nop
    lhu      t0, 0(a1) /* t0 = source      (r5g6b5) */
    lbu      t1, 0(a2) /* t1 = mask        (a8) */
    lhu      t2, 0(a0) /* t2 = destination (r5g6b5) */

    CONVERT_1x0565_TO_1x8888  t0, t3, t4, t5
    CONVERT_1x0565_TO_1x8888  t2, t4, t5, t6
    MIPS_UN8x4_MUL_UN8_ADD_UN8x4  t3, t1, t4, t0, t7, t2, t5, t6
    CONVERT_1x8888_TO_1x0565  t0, t3, t4, t5

    sh       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 20, s0, s1, s2, s3, s4, s5, s6, s7
    j        ra
     nop

END(pixman_composite_add_0565_8_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_8888_8_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8r8g8b8)
 * a2 - mask (a8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2
    li       t4, 0x00ff00ff
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 4(a1) /* t1 = source      (a8r8g8b8) */
    lbu      t2, 0(a2) /* t2 = mask        (a8) */
    lbu      t3, 1(a2) /* t3 = mask        (a8) */
    lw       t5, 0(a0) /* t5 = destination (a8r8g8b8) */
    lw       t6, 4(a0) /* t6 = destination (a8r8g8b8) */
    addiu    a1, a1, 8
    addiu    a2, a2, 2

    MIPS_2xUN8x4_MUL_2xUN8_ADD_2xUN8x4 t0, t1, \
                                       t2, t3, \
                                       t5, t6, \
                                       t7, t8, \
                                       t4, t9, s0, s1, s2, t0, t1

    sw       t7, 0(a0)
    sw       t8, 4(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a3, 3f
     nop
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lbu      t1, 0(a2) /* t1 = mask        (a8) */
    lw       t2, 0(a0) /* t2 = destination (a8r8g8b8) */

    MIPS_UN8x4_MUL_UN8_ADD_UN8x4 t0, t1, t2, t3, t4, t5, t6, t7

    sw       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2
    j        ra
     nop

END(pixman_composite_add_8888_8_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_8888_n_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8r8g8b8)
 * a2 - mask (32bit constant)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2
    li       t4, 0x00ff00ff
    beqz     a3, 3f
     nop
    srl      a2, a2, 24
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 4(a1) /* t1 = source      (a8r8g8b8) */
                       /* a2 = mask        (32bit constant) */
    lw       t2, 0(a0) /* t2 = destination (a8r8g8b8) */
    lw       t3, 4(a0) /* t3 = destination (a8r8g8b8) */
    addiu    a1, a1, 8

    MIPS_2xUN8x4_MUL_2xUN8_ADD_2xUN8x4 t0, t1, \
                                       a2, a2, \
                                       t2, t3, \
                                       t5, t6, \
                                       t4, t7, t8, t9, s0, s1, s2

    sw       t5, 0(a0)
    sw       t6, 4(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a3, 3f
     nop
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
                       /* a2 = mask        (32bit constant) */
    lw       t1, 0(a0) /* t1 = destination (a8r8g8b8) */

    MIPS_UN8x4_MUL_UN8_ADD_UN8x4 t0, a2, t1, t3, t4, t5, t6, t7

    sw       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2
    j        ra
     nop

END(pixman_composite_add_8888_n_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_8888_8888_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8r8g8b8)
 * a2 - mask (a8r8g8b8)
 * a3 - w
 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2
    li       t4, 0x00ff00ff
    beqz     a3, 3f
     nop
    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 4(a1) /* t1 = source      (a8r8g8b8) */
    lw       t2, 0(a2) /* t2 = mask        (a8r8g8b8) */
    lw       t3, 4(a2) /* t3 = mask        (a8r8g8b8) */
    lw       t5, 0(a0) /* t5 = destination (a8r8g8b8) */
    lw       t6, 4(a0) /* t6 = destination (a8r8g8b8) */
    addiu    a1, a1, 8
    addiu    a2, a2, 8
    srl      t2, t2, 24
    srl      t3, t3, 24

    MIPS_2xUN8x4_MUL_2xUN8_ADD_2xUN8x4 t0, t1, \
                                       t2, t3, \
                                       t5, t6, \
                                       t7, t8, \
                                       t4, t9, s0, s1, s2, t0, t1

    sw       t7, 0(a0)
    sw       t8, 4(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a3, 3f
     nop
    lw       t0, 0(a1) /* t0 = source      (a8r8g8b8) */
    lw       t1, 0(a2) /* t1 = mask        (a8r8g8b8) */
    lw       t2, 0(a0) /* t2 = destination (a8r8g8b8) */
    srl      t1, t1, 24

    MIPS_UN8x4_MUL_UN8_ADD_UN8x4 t0, t1, t2, t3, t4, t5, t6, t7

    sw       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2
    j        ra
     nop

END(pixman_composite_add_8888_8888_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_8_8_asm_mips)
/*
 * a0 - dst  (a8)
 * a1 - src  (a8)
 * a2 - w
 */

    beqz              a2, 3f
     nop
    srl               t9, a2, 2   /* t9 = how many multiples of 4 dst pixels */
    beqz              t9, 1f      /* branch if less than 4 src pixels */
     nop

0:
    beqz              t9, 1f
     addiu            t9, t9, -1
    lbu               t0, 0(a1)
    lbu               t1, 1(a1)
    lbu               t2, 2(a1)
    lbu               t3, 3(a1)
    lbu               t4, 0(a0)
    lbu               t5, 1(a0)
    lbu               t6, 2(a0)
    lbu               t7, 3(a0)

    addiu             a1, a1, 4

    precr_sra.ph.w    t1, t0, 0
    precr_sra.ph.w    t3, t2, 0
    precr_sra.ph.w    t5, t4, 0
    precr_sra.ph.w    t7, t6, 0

    precr.qb.ph       t0, t3, t1
    precr.qb.ph       t1, t7, t5

    addu_s.qb         t2, t0, t1

    sb                t2, 0(a0)
    srl               t2, t2, 8
    sb                t2, 1(a0)
    srl               t2, t2, 8
    sb                t2, 2(a0)
    srl               t2, t2, 8
    sb                t2, 3(a0)
    addiu             a2, a2, -4
    b                 0b
     addiu            a0, a0, 4

1:
    beqz              a2, 3f
     nop
2:
    lbu               t0, 0(a1)
    lbu               t1, 0(a0)
    addiu             a1, a1, 1

    addu_s.qb         t2, t0, t1
    sb                t2, 0(a0)
    addiu             a2, a2, -1
    bnez              a2, 2b
     addiu            a0, a0, 1

3:
    j                 ra
     nop

END(pixman_composite_add_8_8_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_add_8888_8888_asm_mips)
/*
 * a0 - dst (a8r8g8b8)
 * a1 - src (a8r8g8b8)
 * a2 - w
 */

    beqz         a2, 4f
     nop

    srl          t9, a2, 2      /* t1 = how many multiples of 4 src pixels */
    beqz         t9, 3f         /* branch if less than 4 src pixels */
     nop
1:
    addiu        t9, t9, -1
    beqz         t9, 2f
     addiu       a2, a2, -4

    lw           t0, 0(a1)
    lw           t1, 4(a1)
    lw           t2, 8(a1)
    lw           t3, 12(a1)
    lw           t4, 0(a0)
    lw           t5, 4(a0)
    lw           t6, 8(a0)
    lw           t7, 12(a0)
    addiu        a1, a1, 16

    addu_s.qb    t4, t4, t0
    addu_s.qb    t5, t5, t1
    addu_s.qb    t6, t6, t2
    addu_s.qb    t7, t7, t3

    sw           t4, 0(a0)
    sw           t5, 4(a0)
    sw           t6, 8(a0)
    sw           t7, 12(a0)
    b            1b
     addiu       a0, a0, 16
2:
    lw           t0, 0(a1)
    lw           t1, 4(a1)
    lw           t2, 8(a1)
    lw           t3, 12(a1)
    lw           t4, 0(a0)
    lw           t5, 4(a0)
    lw           t6, 8(a0)
    lw           t7, 12(a0)
    addiu        a1, a1, 16

    addu_s.qb    t4, t4, t0
    addu_s.qb    t5, t5, t1
    addu_s.qb    t6, t6, t2
    addu_s.qb    t7, t7, t3

    sw           t4, 0(a0)
    sw           t5, 4(a0)
    sw           t6, 8(a0)
    sw           t7, 12(a0)

    beqz         a2, 4f
     addiu       a0, a0, 16
3:
    lw           t0, 0(a1)
    lw           t1, 0(a0)
    addiu        a1, a1, 4
    addiu        a2, a2, -1
    addu_s.qb    t1, t1, t0
    sw           t1, 0(a0)
    bnez         a2, 3b
     addiu       a0, a0, 4
4:
    jr           ra
     nop

END(pixman_composite_add_8888_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_out_reverse_8_0565_asm_mips)
/*
 * a0 - dst  (r5g6b5)
 * a1 - src  (a8)
 * a2 - w
 */

    beqz     a2, 4f
     nop

    SAVE_REGS_ON_STACK 0, s0, s1, s2, s3
    li       t2, 0xf800f800
    li       t3, 0x07e007e0
    li       t4, 0x001F001F
    li       t5, 0x00ff00ff

    addiu    t1, a2, -1
    beqz     t1, 2f
     nop
1:
    lbu      t0, 0(a1) /* t0 = source      (a8) */
    lbu      t1, 1(a1) /* t1 = source      (a8) */
    lhu      t6, 0(a0) /* t6 = destination (r5g6b5) */
    lhu      t7, 2(a0) /* t7 = destination (r5g6b5) */
    addiu    a1, a1, 2

    not      t0, t0
    not      t1, t1
    andi     t0, 0xff  /* t0 = neg source1 */
    andi     t1, 0xff  /* t1 = neg source2 */
    CONVERT_2x0565_TO_2x8888 t6, t7, t8, t9, t3, t4, s0, s1, s2, s3
    MIPS_2xUN8x4_MUL_2xUN8   t8, t9, t0, t1, t6, t7, t5, s0, s1, s2, s3, t8, t9
    CONVERT_2x8888_TO_2x0565 t6, t7, t8, t9, t2, t3, t4, s0, s1

    sh       t8, 0(a0)
    sh       t9, 2(a0)
    addiu    a2, a2, -2
    addiu    t1, a2, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a2, 3f
     nop
    lbu      t0, 0(a1) /* t0 = source      (a8) */
    lhu      t1, 0(a0) /* t1 = destination (r5g6b5) */

    not      t0, t0
    andi     t0, 0xff  /* t0 = neg source */
    CONVERT_1x0565_TO_1x8888 t1, t2, t3, t4
    MIPS_UN8x4_MUL_UN8        t2, t0, t1, t5, t3, t4, t6
    CONVERT_1x8888_TO_1x0565 t1, t2, t3, t4

    sh       t2, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2, s3
4:
    j        ra
     nop

END(pixman_composite_out_reverse_8_0565_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_out_reverse_8_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (a8)
 * a2 - w
 */

    beqz     a2, 3f
     nop
    li       t4, 0x00ff00ff
    addiu    t1, a2, -1
    beqz     t1, 2f
     nop
1:
    lbu      t0, 0(a1) /* t0 = source      (a8) */
    lbu      t1, 1(a1) /* t1 = source      (a8) */
    lw       t2, 0(a0) /* t2 = destination (a8r8g8b8) */
    lw       t3, 4(a0) /* t3 = destination (a8r8g8b8) */
    addiu    a1, a1, 2
    not      t0, t0
    not      t1, t1
    andi     t0, 0xff  /* t0 = neg source */
    andi     t1, 0xff  /* t1 = neg source */

    MIPS_2xUN8x4_MUL_2xUN8 t2, t3, t0, t1, t5, t6, t4, t7, t8, t9, t2, t3, t0

    sw       t5, 0(a0)
    sw       t6, 4(a0)
    addiu    a2, a2, -2
    addiu    t1, a2, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a2, 3f
     nop
    lbu      t0, 0(a1) /* t0 = source      (a8) */
    lw       t1, 0(a0) /* t1 = destination (a8r8g8b8) */
    not      t0, t0
    andi     t0, 0xff  /* t0 = neg source */

    MIPS_UN8x4_MUL_UN8 t1, t0, t2, t4, t3, t5, t6

    sw       t2, 0(a0)
3:
    j        ra
     nop

END(pixman_composite_out_reverse_8_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_over_reverse_n_8888_asm_mips)
/*
 * a0 - dst  (a8r8g8b8)
 * a1 - src  (32bit constant)
 * a2 - w
 */

    beqz              a2, 5f
     nop

    SAVE_REGS_ON_STACK 20, s0, s1, s2, s3, s4, s5, s6, s7
    li                t0, 0x00ff00ff
    srl               t9, a2, 2   /* t9 = how many multiples of 4 src pixels */
    beqz              t9, 2f      /* branch if less than 4 src pixels */
     nop
1:
    beqz              t9, 2f
     addiu            t9, t9, -1

    lw                t1, 0(a0)
    lw                t2, 4(a0)
    lw                t3, 8(a0)
    lw                t4, 12(a0)

    addiu             a2, a2, -4

    not               t5, t1
    not               t6, t2
    not               t7, t3
    not               t8, t4
    srl               t5, t5, 24
    srl               t6, t6, 24
    srl               t7, t7, 24
    srl               t8, t8, 24
    replv.ph          t5, t5
    replv.ph          t6, t6
    replv.ph          t7, t7
    replv.ph          t8, t8
    muleu_s.ph.qbl    s0, a1, t5
    muleu_s.ph.qbr    s1, a1, t5
    muleu_s.ph.qbl    s2, a1, t6
    muleu_s.ph.qbr    s3, a1, t6
    muleu_s.ph.qbl    s4, a1, t7
    muleu_s.ph.qbr    s5, a1, t7
    muleu_s.ph.qbl    s6, a1, t8
    muleu_s.ph.qbr    s7, a1, t8

    shra_r.ph         t5, s0, 8
    shra_r.ph         t6, s1, 8
    shra_r.ph         t7, s2, 8
    shra_r.ph         t8, s3, 8
    and               t5, t5, t0
    and               t6, t6, t0
    and               t7, t7, t0
    and               t8, t8, t0
    addq.ph           s0, s0, t5
    addq.ph           s1, s1, t6
    addq.ph           s2, s2, t7
    addq.ph           s3, s3, t8
    shra_r.ph         s0, s0, 8
    shra_r.ph         s1, s1, 8
    shra_r.ph         s2, s2, 8
    shra_r.ph         s3, s3, 8
    shra_r.ph         t5, s4, 8
    shra_r.ph         t6, s5, 8
    shra_r.ph         t7, s6, 8
    shra_r.ph         t8, s7, 8
    and               t5, t5, t0
    and               t6, t6, t0
    and               t7, t7, t0
    and               t8, t8, t0
    addq.ph           s4, s4, t5
    addq.ph           s5, s5, t6
    addq.ph           s6, s6, t7
    addq.ph           s7, s7, t8
    shra_r.ph         s4, s4, 8
    shra_r.ph         s5, s5, 8
    shra_r.ph         s6, s6, 8
    shra_r.ph         s7, s7, 8

    precr.qb.ph       t5, s0, s1
    precr.qb.ph       t6, s2, s3
    precr.qb.ph       t7, s4, s5
    precr.qb.ph       t8, s6, s7
    addu_s.qb         t5, t1, t5
    addu_s.qb         t6, t2, t6
    addu_s.qb         t7, t3, t7
    addu_s.qb         t8, t4, t8

    sw                t5, 0(a0)
    sw                t6, 4(a0)
    sw                t7, 8(a0)
    sw                t8, 12(a0)
    b                 1b
     addiu            a0, a0, 16

2:
    beqz              a2, 4f
     nop
3:
    lw                t1, 0(a0)

    not               t2, t1
    srl               t2, t2, 24
    replv.ph          t2, t2

    muleu_s.ph.qbl    t4, a1, t2
    muleu_s.ph.qbr    t5, a1, t2
    shra_r.ph         t6, t4, 8
    shra_r.ph         t7, t5, 8

    and               t6,t6,t0
    and               t7,t7,t0

    addq.ph           t8, t4, t6
    addq.ph           t9, t5, t7

    shra_r.ph         t8, t8, 8
    shra_r.ph         t9, t9, 8

    precr.qb.ph       t9, t8, t9

    addu_s.qb         t9, t1, t9
    sw                t9, 0(a0)

    addiu             a2, a2, -1
    bnez              a2, 3b
     addiu            a0, a0, 4
4:
    RESTORE_REGS_FROM_STACK 20, s0, s1, s2, s3, s4, s5, s6, s7
5:
    j                 ra
     nop

END(pixman_composite_over_reverse_n_8888_asm_mips)

LEAF_MIPS_DSPR2(pixman_composite_in_n_8_asm_mips)
/*
 * a0 - dst  (a8)
 * a1 - src  (32bit constant)
 * a2 - w
 */

    li                t9, 0x00ff00ff
    beqz              a2, 3f
     nop
    srl               t7, a2, 2   /* t7 = how many multiples of 4 dst pixels */
    beqz              t7, 1f      /* branch if less than 4 src pixels */
     nop

    srl               t8, a1, 24
    replv.ph          t8, t8

0:
    beqz              t7, 1f
     addiu            t7, t7, -1
    lbu               t0, 0(a0)
    lbu               t1, 1(a0)
    lbu               t2, 2(a0)
    lbu               t3, 3(a0)

    precr_sra.ph.w    t1, t0, 0
    precr_sra.ph.w    t3, t2, 0
    precr.qb.ph       t0, t3, t1

    muleu_s.ph.qbl    t2, t0, t8
    muleu_s.ph.qbr    t3, t0, t8
    shra_r.ph         t4, t2, 8
    shra_r.ph         t5, t3, 8
    and               t4, t4, t9
    and               t5, t5, t9
    addq.ph           t2, t2, t4
    addq.ph           t3, t3, t5
    shra_r.ph         t2, t2, 8
    shra_r.ph         t3, t3, 8
    precr.qb.ph       t2, t2, t3

    sb                t2, 0(a0)
    srl               t2, t2, 8
    sb                t2, 1(a0)
    srl               t2, t2, 8
    sb                t2, 2(a0)
    srl               t2, t2, 8
    sb                t2, 3(a0)
    addiu             a2, a2, -4
    b                 0b
     addiu            a0, a0, 4

1:
    beqz              a2, 3f
     nop
    srl               t8, a1, 24
2:
    lbu               t0, 0(a0)

    mul               t2, t0, t8
    shra_r.ph         t3, t2, 8
    andi              t3, t3, 0x00ff
    addq.ph           t2, t2, t3
    shra_r.ph         t2, t2, 8

    sb                t2, 0(a0)
    addiu             a2, a2, -1
    bnez              a2, 2b
     addiu            a0, a0, 1

3:
    j                 ra
     nop

END(pixman_composite_in_n_8_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_nearest_scanline_8888_8888_OVER_asm_mips)
/*
 * a0     - dst  (a8r8g8b8)
 * a1     - src  (a8r8g8b8)
 * a2     - w
 * a3     - vx
 * 16(sp) - unit_x
 */

    SAVE_REGS_ON_STACK 0, s0, s1, s2, s3
    lw       t8, 16(sp) /* t8 = unit_x */
    li       t6, 0x00ff00ff
    beqz     a2, 3f
     nop
    addiu    t1, a2, -1
    beqz     t1, 2f
     nop
1:
    sra      t0, a3, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 2  /* t0 = t0 * 4 (a8r8g8b8) */
    addu     t0, a1, t0
    lw       t0, 0(t0)  /* t0 = source      (a8r8g8b8) */
    addu     a3, a3, t8 /* a3 = vx + unit_x */

    sra      t1, a3, 16 /* t0 = vx >> 16 */
    sll      t1, t1, 2  /* t0 = t0 * 4 (a8r8g8b8) */
    addu     t1, a1, t1
    lw       t1, 0(t1)  /* t1 = source      (a8r8g8b8) */
    addu     a3, a3, t8 /* a3 = vx + unit_x */

    lw       t2, 0(a0)  /* t2 = destination (a8r8g8b8) */
    lw       t3, 4(a0)  /* t3 = destination (a8r8g8b8) */

    OVER_2x8888_2x8888 t0, t1, t2, t3, t4, t5, t6, t7, t9, s0, s1, s2, s3

    sw       t4, 0(a0)
    sw       t5, 4(a0)
    addiu    a2, a2, -2
    addiu    t1, a2, -1
    bgtz     t1, 1b
     addiu   a0, a0, 8
2:
    beqz     a2, 3f
     nop
    sra      t0, a3, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 2  /* t0 = t0 * 4 (a8r8g8b8) */
    addu     t0, a1, t0
    lw       t0, 0(t0)  /* t0 = source      (a8r8g8b8) */
    lw       t1, 0(a0)  /* t1 = destination (a8r8g8b8) */
    addu     a3, a3, t8 /* a3 = vx + unit_x */

    OVER_8888_8888 t0, t1, t2, t6, t4, t5, t3, t7

    sw       t2, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, s0, s1, s2, s3
    j        ra
     nop

END(pixman_scaled_nearest_scanline_8888_8888_OVER_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_nearest_scanline_8888_0565_OVER_asm_mips)
/*
 * a0     - dst  (r5g6b5)
 * a1     - src  (a8r8g8b8)
 * a2     - w
 * a3     - vx
 * 16(sp) - unit_x
 */

    SAVE_REGS_ON_STACK 24, s0, s1, s2, s3, s4, v0, v1
    lw       t8, 40(sp) /* t8 = unit_x */
    li       t4, 0x00ff00ff
    li       t5, 0xf800f800
    li       t6, 0x07e007e0
    li       t7, 0x001F001F
    beqz     a2, 3f
     nop
    addiu    t1, a2, -1
    beqz     t1, 2f
     nop
1:
    sra      t0, a3, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 2  /* t0 = t0 * 4 (a8r8g8b8) */
    addu     t0, a1, t0
    lw       t0, 0(t0)  /* t0 = source      (a8r8g8b8) */
    addu     a3, a3, t8 /* a3 = vx + unit_x */
    sra      t1, a3, 16 /* t0 = vx >> 16 */
    sll      t1, t1, 2  /* t0 = t0 * 4 (a8r8g8b8) */
    addu     t1, a1, t1
    lw       t1, 0(t1)  /* t1 = source      (a8r8g8b8) */
    addu     a3, a3, t8 /* a3 = vx + unit_x */
    lhu      t2, 0(a0)  /* t2 = destination (r5g6b5) */
    lhu      t3, 2(a0)  /* t3 = destination (r5g6b5) */

    CONVERT_2x0565_TO_2x8888 t2, t3, v0, v1, t6, t7, s0, s1, s2, s3
    OVER_2x8888_2x8888       t0, t1, v0, v1, t2, t3, t4, t9, s0, s1, s2, s3, s4
    CONVERT_2x8888_TO_2x0565 t2, t3, v0, v1, t5, t6, t7, t9, s2

    sh       v0, 0(a0)
    sh       v1, 2(a0)
    addiu    a2, a2, -2
    addiu    t1, a2, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a2, 3f
     nop
    sra      t0, a3, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 2  /* t0 = t0 * 4 (a8r8g8b8) */
    addu     t0, a1, t0
    lw       t0, 0(t0)  /* t0 = source      (a8r8g8b8) */
    lhu      t1, 0(a0)  /* t1 = destination (r5g6b5) */
    addu     a3, a3, t8 /* a3 = vx + unit_x */

    CONVERT_1x0565_TO_1x8888 t1, t2, t5, t6
    OVER_8888_8888           t0, t2, t1, t4, t3, t5, t6, t7
    CONVERT_1x8888_TO_1x0565 t1, t2, t5, t6

    sh       t2, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 24, s0, s1, s2, s3, s4, v0, v1
    j        ra
     nop

END(pixman_scaled_nearest_scanline_8888_0565_OVER_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_nearest_scanline_0565_8888_SRC_asm_mips)
/*
 * a0     - dst (a8r8g8b8)
 * a1     - src (r5g6b5)
 * a2     - w
 * a3     - vx
 * 16(sp) - unit_x
 */

    SAVE_REGS_ON_STACK 0, v0
    beqz     a2, 3f
     nop

    lw       v0, 16(sp) /* v0 = unit_x */
    addiu    t1, a2, -1
    beqz     t1, 2f
     nop

    li       t4, 0x07e007e0
    li       t5, 0x001F001F
1:
    sra      t0, a3, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 1  /* t0 = t0 * 2 ((r5g6b5)) */
    addu     t0, a1, t0
    lhu      t0, 0(t0)  /* t0 = source ((r5g6b5)) */
    addu     a3, a3, v0 /* a3 = vx + unit_x */
    sra      t1, a3, 16 /* t1 = vx >> 16 */
    sll      t1, t1, 1  /* t1 = t1 * 2 ((r5g6b5)) */
    addu     t1, a1, t1
    lhu      t1, 0(t1)  /* t1 = source ((r5g6b5)) */
    addu     a3, a3, v0 /* a3 = vx + unit_x */
    addiu    a2, a2, -2

    CONVERT_2x0565_TO_2x8888 t0, t1, t2, t3, t4, t5, t6, t7, t8, t9

    sw       t2, 0(a0)
    sw       t3, 4(a0)

    addiu    t2, a2, -1
    bgtz     t2, 1b
     addiu   a0, a0, 8
2:
    beqz     a2, 3f
     nop
    sra      t0, a3, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 1  /* t0 = t0 * 2 ((r5g6b5)) */
    addu     t0, a1, t0
    lhu      t0, 0(t0)  /* t0 = source ((r5g6b5)) */

    CONVERT_1x0565_TO_1x8888 t0, t1, t2, t3

    sw       t1, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 0, v0
    j        ra
     nop

END(pixman_scaled_nearest_scanline_0565_8888_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_nearest_scanline_8888_8_0565_OVER_asm_mips)
/*
 * a0     - dst  (r5g6b5)
 * a1     - src  (a8r8g8b8)
 * a2     - mask (a8)
 * a3     - w
 * 16(sp) - vx
 * 20(sp) - unit_x
 */
    beqz     a3, 4f
     nop

    SAVE_REGS_ON_STACK 20, v0, v1, s0, s1, s2, s3, s4, s5
    lw       v0, 36(sp) /* v0 = vx */
    lw       v1, 40(sp) /* v1 = unit_x */
    li       t6, 0x00ff00ff
    li       t7, 0xf800f800
    li       t8, 0x07e007e0
    li       t9, 0x001F001F

    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    sra      t0, v0, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 2  /* t0 = t0 * 4      (a8r8g8b8) */
    addu     t0, a1, t0
    lw       t0, 0(t0)  /* t0 = source      (a8r8g8b8) */
    addu     v0, v0, v1 /* v0 = vx + unit_x */
    sra      t1, v0, 16 /* t1 = vx >> 16 */
    sll      t1, t1, 2  /* t1 = t1 * 4      (a8r8g8b8) */
    addu     t1, a1, t1
    lw       t1, 0(t1)  /* t1 = source      (a8r8g8b8) */
    addu     v0, v0, v1 /* v0 = vx + unit_x */
    lbu      t2, 0(a2)  /* t2 = mask        (a8) */
    lbu      t3, 1(a2)  /* t3 = mask        (a8) */
    lhu      t4, 0(a0)  /* t4 = destination (r5g6b5) */
    lhu      t5, 2(a0)  /* t5 = destination (r5g6b5) */
    addiu    a2, a2, 2

    CONVERT_2x0565_TO_2x8888 t4, t5, s0, s1, t8, t9, s2, s3, s4, s5
    OVER_2x8888_2x8_2x8888   t0, t1, \
                             t2, t3, \
                             s0, s1, \
                             t4, t5, \
                             t6, s2, s3, s4, s5, t2, t3
    CONVERT_2x8888_TO_2x0565 t4, t5, s0, s1, t7, t8, t9, s2, s3

    sh       s0, 0(a0)
    sh       s1, 2(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a3, 3f
     nop
    sra      t0, v0, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 2  /* t0 = t0 * 4      (a8r8g8b8) */
    addu     t0, a1, t0
    lw       t0, 0(t0)  /* t0 = source      (a8r8g8b8) */
    lbu      t1, 0(a2)  /* t1 = mask        (a8) */
    lhu      t2, 0(a0)  /* t2 = destination (r5g6b5) */

    CONVERT_1x0565_TO_1x8888 t2, t3, t4, t5
    OVER_8888_8_8888         t0, t1, t3, t2, t6, t4, t5, t7, t8
    CONVERT_1x8888_TO_1x0565 t2, t3, t4, t5

    sh       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 20, v0, v1, s0, s1, s2, s3, s4, s5
4:
    j        ra
     nop

END(pixman_scaled_nearest_scanline_8888_8_0565_OVER_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_nearest_scanline_0565_8_0565_OVER_asm_mips)
/*
 * a0     - dst  (r5g6b5)
 * a1     - src  (r5g6b5)
 * a2     - mask (a8)
 * a3     - w
 * 16(sp) - vx
 * 20(sp) - unit_x
 */

    beqz     a3, 4f
     nop
    SAVE_REGS_ON_STACK 20, v0, v1, s0, s1, s2, s3, s4, s5
    lw       v0, 36(sp) /* v0 = vx */
    lw       v1, 40(sp) /* v1 = unit_x */
    li       t4, 0xf800f800
    li       t5, 0x07e007e0
    li       t6, 0x001F001F
    li       t7, 0x00ff00ff

    addiu    t1, a3, -1
    beqz     t1, 2f
     nop
1:
    sra      t0, v0, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 1  /* t0 = t0 * 2      (r5g6b5) */
    addu     t0, a1, t0
    lhu      t0, 0(t0)  /* t0 = source      (r5g6b5) */
    addu     v0, v0, v1 /* v0 = vx + unit_x */
    sra      t1, v0, 16 /* t1 = vx >> 16 */
    sll      t1, t1, 1  /* t1 = t1 * 2      (r5g6b5) */
    addu     t1, a1, t1
    lhu      t1, 0(t1)  /* t1 = source      (r5g6b5) */
    addu     v0, v0, v1 /* v0 = vx + unit_x */
    lbu      t2, 0(a2)  /* t2 = mask        (a8) */
    lbu      t3, 1(a2)  /* t3 = mask        (a8) */
    lhu      t8, 0(a0)  /* t8 = destination (r5g6b5) */
    lhu      t9, 2(a0)  /* t9 = destination (r5g6b5) */
    addiu    a2, a2, 2

    CONVERT_2x0565_TO_2x8888 t0, t1, s0, s1, t5, t6, s2, s3, s4, s5
    CONVERT_2x0565_TO_2x8888 t8, t9, s2, s3, t5, t6, s4, s5, t0, t1
    OVER_2x8888_2x8_2x8888   s0, s1, \
                             t2, t3, \
                             s2, s3, \
                             t0, t1, \
                             t7, t8, t9, s4, s5, s0, s1
    CONVERT_2x8888_TO_2x0565 t0, t1, s0, s1, t4, t5, t6, s2, s3

    sh       s0, 0(a0)
    sh       s1, 2(a0)
    addiu    a3, a3, -2
    addiu    t1, a3, -1
    bgtz     t1, 1b
     addiu   a0, a0, 4
2:
    beqz     a3, 3f
     nop
    sra      t0, v0, 16 /* t0 = vx >> 16 */
    sll      t0, t0, 1  /* t0 = t0 * 2      (r5g6b5) */
    addu     t0, a1, t0

    lhu      t0, 0(t0)  /* t0 = source      (r5g6b5) */
    lbu      t1, 0(a2)  /* t1 = mask        (a8) */
    lhu      t2, 0(a0)  /* t2 = destination (r5g6b5) */

    CONVERT_1x0565_TO_1x8888 t0, t3, t4, t5
    CONVERT_1x0565_TO_1x8888 t2, t4, t5, t6
    OVER_8888_8_8888         t3, t1, t4, t0, t7, t2, t5, t6, t8
    CONVERT_1x8888_TO_1x0565 t0, t3, t4, t5

    sh       t3, 0(a0)
3:
    RESTORE_REGS_FROM_STACK 20, v0, v1, s0, s1, s2, s3, s4, s5
4:
    j        ra
     nop

END(pixman_scaled_nearest_scanline_0565_8_0565_OVER_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_mips)
/*
 * a0     - *dst
 * a1     - *src_top
 * a2     - *src_bottom
 * a3     - w
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 */

    beqz     a3, 1f
     nop

    SAVE_REGS_ON_STACK 20, v0, s0, s1, s2, s3, s4, s5, s6, s7

    lw       s0, 36(sp)     /* s0 = wt */
    lw       s1, 40(sp)     /* s1 = wb */
    lw       s2, 44(sp)     /* s2 = vx */
    lw       s3, 48(sp)     /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4     /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5     /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4     /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5     /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4     /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 2
    addiu    t8, t9, 4
    lwx      t0, t9(a1)     /* t0 = tl */
    lwx      t1, t8(a1)     /* t1 = tr */
    addiu    a3, a3, -1
    lwx      t2, t9(a2)     /* t2 = bl */
    lwx      t3, t8(a2)     /* t3 = br */

    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7

    addu     s2, s2, s3     /* vx += unit_x; */
    sw       t0, 0(a0)
    bnez     a3, 0b
     addiu   a0, a0, 4

    RESTORE_REGS_FROM_STACK 20, v0, s0, s1, s2, s3, s4, s5, s6, s7
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_8888_0565_SRC_asm_mips)
/*
 * a0     - *dst
 * a1     - *src_top
 * a2     - *src_bottom
 * a3     - w
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 */

    beqz     a3, 1f
     nop

    SAVE_REGS_ON_STACK 20, v0, s0, s1, s2, s3, s4, s5, s6, s7

    lw       s0, 36(sp)     /* s0 = wt */
    lw       s1, 40(sp)     /* s1 = wb */
    lw       s2, 44(sp)     /* s2 = vx */
    lw       s3, 48(sp)     /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4     /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5     /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4     /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5     /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4     /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 2
    addiu    t8, t9, 4
    lwx      t0, t9(a1)     /* t0 = tl */
    lwx      t1, t8(a1)     /* t1 = tr */
    addiu    a3, a3, -1
    lwx      t2, t9(a2)     /* t2 = bl */
    lwx      t3, t8(a2)     /* t3 = br */

    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    CONVERT_1x8888_TO_1x0565 t0, t1, t2, t3

    addu     s2, s2, s3     /* vx += unit_x; */
    sh       t1, 0(a0)
    bnez     a3, 0b
     addiu   a0, a0, 2

    RESTORE_REGS_FROM_STACK 20, v0, s0, s1, s2, s3, s4, s5, s6, s7
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_8888_0565_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_0565_8888_SRC_asm_mips)
/*
 * a0     - *dst
 * a1     - *src_top
 * a2     - *src_bottom
 * a3     - w
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 */

    beqz     a3, 1f
     nop

    SAVE_REGS_ON_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8

    lw       s0, 44(sp)     /* s0 = wt */
    lw       s1, 48(sp)     /* s1 = wb */
    lw       s2, 52(sp)     /* s2 = vx */
    lw       s3, 56(sp)     /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE
    li       v1, 0x07e007e0
    li       s8, 0x001f001f

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4     /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5     /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4     /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5     /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4     /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 1
    addiu    t8, t9, 2
    lhx      t0, t9(a1)     /* t0 = tl */
    lhx      t1, t8(a1)     /* t1 = tr */
    andi     t1, t1, 0xffff
    addiu    a3, a3, -1
    lhx      t2, t9(a2)     /* t2 = bl */
    lhx      t3, t8(a2)     /* t3 = br */
    andi     t3, t3, 0xffff

    CONVERT_2x0565_TO_2x8888 t0, t1, t0, t1, v1, s8, t4, t5, t6, t7
    CONVERT_2x0565_TO_2x8888 t2, t3, t2, t3, v1, s8, t4, t5, t6, t7
    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7

    addu     s2, s2, s3     /* vx += unit_x; */
    sw       t0, 0(a0)
    bnez     a3, 0b
     addiu   a0, a0, 4

    RESTORE_REGS_FROM_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_0565_8888_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_0565_0565_SRC_asm_mips)
/*
 * a0     - *dst
 * a1     - *src_top
 * a2     - *src_bottom
 * a3     - w
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 */

    beqz     a3, 1f
     nop

    SAVE_REGS_ON_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8

    lw       s0, 44(sp)     /* s0 = wt */
    lw       s1, 48(sp)     /* s1 = wb */
    lw       s2, 52(sp)     /* s2 = vx */
    lw       s3, 56(sp)     /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE
    li       v1, 0x07e007e0
    li       s8, 0x001f001f

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4     /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5     /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4     /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5     /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4     /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 1
    addiu    t8, t9, 2
    lhx      t0, t9(a1)     /* t0 = tl */
    lhx      t1, t8(a1)     /* t1 = tr */
    andi     t1, t1, 0xffff
    addiu    a3, a3, -1
    lhx      t2, t9(a2)     /* t2 = bl */
    lhx      t3, t8(a2)     /* t3 = br */
    andi     t3, t3, 0xffff

    CONVERT_2x0565_TO_2x8888 t0, t1, t0, t1, v1, s8, t4, t5, t6, t7
    CONVERT_2x0565_TO_2x8888 t2, t3, t2, t3, v1, s8, t4, t5, t6, t7
    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    CONVERT_1x8888_TO_1x0565 t0, t1, t2, t3

    addu     s2, s2, s3     /* vx += unit_x; */
    sh       t1, 0(a0)
    bnez     a3, 0b
     addiu   a0, a0, 2

    RESTORE_REGS_FROM_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_0565_0565_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_8888_8888_OVER_asm_mips)
/*
 * a0     - *dst
 * a1     - *src_top
 * a2     - *src_bottom
 * a3     - w
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 */

    beqz     a3, 1f
     nop

    SAVE_REGS_ON_STACK 24, v0, s0, s1, s2, s3, s4, s5, s6, s7, s8

    lw       s0, 40(sp)     /* s0 = wt */
    lw       s1, 44(sp)     /* s1 = wb */
    lw       s2, 48(sp)     /* s2 = vx */
    lw       s3, 52(sp)     /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE
    li       s8, 0x00ff00ff

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4     /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5     /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4     /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5     /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4     /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 2
    addiu    t8, t9, 4
    lwx      t0, t9(a1)     /* t0 = tl */
    lwx      t1, t8(a1)     /* t1 = tr */
    addiu    a3, a3, -1
    lwx      t2, t9(a2)     /* t2 = bl */
    lwx      t3, t8(a2)     /* t3 = br */

    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    lw       t1, 0(a0)      /* t1 = dest */
    OVER_8888_8888 t0, t1, t2, s8, t3, t4, t5, t6

    addu     s2, s2, s3     /* vx += unit_x; */
    sw       t2, 0(a0)
    bnez     a3, 0b
     addiu   a0, a0, 4

    RESTORE_REGS_FROM_STACK 24, v0, s0, s1, s2, s3, s4, s5, s6, s7, s8
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_8888_8888_OVER_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_8888_8888_ADD_asm_mips)
/*
 * a0     - *dst
 * a1     - *src_top
 * a2     - *src_bottom
 * a3     - w
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 */

    beqz         a3, 1f
     nop

    SAVE_REGS_ON_STACK 20, v0, s0, s1, s2, s3, s4, s5, s6, s7

    lw           s0, 36(sp)     /* s0 = wt */
    lw           s1, 40(sp)     /* s1 = wb */
    lw           s2, 44(sp)     /* s2 = vx */
    lw           s3, 48(sp)     /* s3 = unit_x */
    li           v0, BILINEAR_INTERPOLATION_RANGE

    sll          s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll          s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi         t4, s2, 0xffff /* t4 = (short)vx */
    srl          t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu         t5, v0, t4     /* t5 = ( 256 - (vx>>8)) */

    mul          s4, s0, t5     /* s4 = wt*(256-(vx>>8)) */
    mul          s5, s0, t4     /* s5 = wt*(vx>>8) */
    mul          s6, s1, t5     /* s6 = wb*(256-(vx>>8)) */
    mul          s7, s1, t4     /* s7 = wb*(vx>>8) */

    sra          t9, s2, 16
    sll          t9, t9, 2
    addiu        t8, t9, 4
    lwx          t0, t9(a1)     /* t0 = tl */
    lwx          t1, t8(a1)     /* t1 = tr */
    addiu        a3, a3, -1
    lwx          t2, t9(a2)     /* t2 = bl */
    lwx          t3, t8(a2)     /* t3 = br */

    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    lw           t1, 0(a0)
    addu_s.qb    t2, t0, t1

    addu         s2, s2, s3     /* vx += unit_x; */
    sw           t2, 0(a0)
    bnez         a3, 0b
     addiu       a0, a0, 4

    RESTORE_REGS_FROM_STACK 20, v0, s0, s1, s2, s3, s4, s5, s6, s7
1:
    j            ra
     nop

END(pixman_scaled_bilinear_scanline_8888_8888_ADD_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_8888_8_8888_SRC_asm_mips)
/*
 * a0     - *dst
 * a1     - *mask
 * a2     - *src_top
 * a3     - *src_bottom
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 * 32(sp) - w
 */

    lw       v1, 32(sp)
    beqz     v1, 1f
     nop

    SAVE_REGS_ON_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8

    lw       s0, 44(sp)        /* s0 = wt */
    lw       s1, 48(sp)        /* s1 = wb */
    lw       s2, 52(sp)        /* s2 = vx */
    lw       s3, 56(sp)        /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE
    li       s8, 0x00ff00ff

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff    /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4        /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5        /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4        /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5        /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4        /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 2
    addiu    t8, t9, 4
    lwx      t0, t9(a2)        /* t0 = tl */
    lwx      t1, t8(a2)        /* t1 = tr */
    addiu    v1, v1, -1
    lwx      t2, t9(a3)        /* t2 = bl */
    lwx      t3, t8(a3)        /* t3 = br */

    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    lbu      t1, 0(a1)         /* t1 = mask */
    addiu    a1, a1, 1
    MIPS_UN8x4_MUL_UN8 t0, t1, t0, s8, t2, t3, t4

    addu     s2, s2, s3        /* vx += unit_x; */
    sw       t0, 0(a0)
    bnez     v1, 0b
     addiu   a0, a0, 4

    RESTORE_REGS_FROM_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_8888_8_8888_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_8888_8_0565_SRC_asm_mips)
/*
 * a0     - *dst
 * a1     - *mask
 * a2     - *src_top
 * a3     - *src_bottom
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 * 32(sp) - w
 */

    lw       v1, 32(sp)
    beqz     v1, 1f
     nop

    SAVE_REGS_ON_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8

    lw       s0, 44(sp)        /* s0 = wt */
    lw       s1, 48(sp)        /* s1 = wb */
    lw       s2, 52(sp)        /* s2 = vx */
    lw       s3, 56(sp)        /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE
    li       s8, 0x00ff00ff

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff    /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4        /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5        /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4        /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5        /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4        /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 2
    addiu    t8, t9, 4
    lwx      t0, t9(a2)        /* t0 = tl */
    lwx      t1, t8(a2)        /* t1 = tr */
    addiu    v1, v1, -1
    lwx      t2, t9(a3)        /* t2 = bl */
    lwx      t3, t8(a3)        /* t3 = br */

    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    lbu      t1, 0(a1)         /* t1 = mask */
    addiu    a1, a1, 1
    MIPS_UN8x4_MUL_UN8 t0, t1, t0, s8, t2, t3, t4
    CONVERT_1x8888_TO_1x0565 t0, t1, t2, t3

    addu     s2, s2, s3        /* vx += unit_x; */
    sh       t1, 0(a0)
    bnez     v1, 0b
     addiu   a0, a0, 2

    RESTORE_REGS_FROM_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_8888_8_0565_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_0565_8_x888_SRC_asm_mips)
/*
 * a0     - *dst
 * a1     - *mask
 * a2     - *src_top
 * a3     - *src_bottom
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 * 32(sp) - w
 */

    lw       t0, 32(sp)
    beqz     t0, 1f
     nop

    SAVE_REGS_ON_STACK 32, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8, ra

    lw       s0, 48(sp)        /* s0 = wt */
    lw       s1, 52(sp)        /* s1 = wb */
    lw       s2, 56(sp)        /* s2 = vx */
    lw       s3, 60(sp)        /* s3 = unit_x */
    lw       ra, 64(sp)        /* ra = w */
    li       v0, 0x00ff00ff
    li       v1, 0x07e007e0
    li       s8, 0x001f001f

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff    /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    li       t5, BILINEAR_INTERPOLATION_RANGE
    subu     t5, t5, t4        /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5        /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4        /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5        /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4        /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 1
    addiu    t8, t9, 2
    lhx      t0, t9(a2)        /* t0 = tl */
    lhx      t1, t8(a2)        /* t1 = tr */
    andi     t1, t1, 0xffff
    addiu    ra, ra, -1
    lhx      t2, t9(a3)        /* t2 = bl */
    lhx      t3, t8(a3)        /* t3 = br */
    andi     t3, t3, 0xffff

    CONVERT_2x0565_TO_2x8888 t0, t1, t0, t1, v1, s8, t4, t5, t6, t7
    CONVERT_2x0565_TO_2x8888 t2, t3, t2, t3, v1, s8, t4, t5, t6, t7
    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    lbu      t1, 0(a1)         /* t1 = mask */
    addiu    a1, a1, 1
    MIPS_UN8x4_MUL_UN8 t0, t1, t0, v0, t2, t3, t4

    addu     s2, s2, s3        /* vx += unit_x; */
    sw       t0, 0(a0)
    bnez     ra, 0b
     addiu   a0, a0, 4

    RESTORE_REGS_FROM_STACK 32, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8, ra
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_0565_8_x888_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_0565_8_0565_SRC_asm_mips)
/*
 * a0     - *dst
 * a1     - *mask
 * a2     - *src_top
 * a3     - *src_bottom
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 * 32(sp) - w
 */

    lw       t0, 32(sp)
    beqz     t0, 1f
     nop

    SAVE_REGS_ON_STACK 32, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8, ra

    lw       s0, 48(sp)        /* s0 = wt */
    lw       s1, 52(sp)        /* s1 = wb */
    lw       s2, 56(sp)        /* s2 = vx */
    lw       s3, 60(sp)        /* s3 = unit_x */
    lw       ra, 64(sp)        /* ra = w */
    li       v0, 0x00ff00ff
    li       v1, 0x07e007e0
    li       s8, 0x001f001f

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff    /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    li       t5, BILINEAR_INTERPOLATION_RANGE
    subu     t5, t5, t4        /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5        /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4        /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5        /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4        /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 1
    addiu    t8, t9, 2
    lhx      t0, t9(a2)        /* t0 = tl */
    lhx      t1, t8(a2)        /* t1 = tr */
    andi     t1, t1, 0xffff
    addiu    ra, ra, -1
    lhx      t2, t9(a3)        /* t2 = bl */
    lhx      t3, t8(a3)        /* t3 = br */
    andi     t3, t3, 0xffff

    CONVERT_2x0565_TO_2x8888 t0, t1, t0, t1, v1, s8, t4, t5, t6, t7
    CONVERT_2x0565_TO_2x8888 t2, t3, t2, t3, v1, s8, t4, t5, t6, t7
    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    lbu      t1, 0(a1)         /* t1 = mask */
    addiu    a1, a1, 1
    MIPS_UN8x4_MUL_UN8 t0, t1, t0, v0, t2, t3, t4
    CONVERT_1x8888_TO_1x0565 t0, t1, t2, t3

    addu     s2, s2, s3        /* vx += unit_x; */
    sh       t1, 0(a0)
    bnez     ra, 0b
     addiu   a0, a0, 2

    RESTORE_REGS_FROM_STACK 32, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8, ra
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_0565_8_0565_SRC_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_8888_8_8888_OVER_asm_mips)
/*
 * a0     - dst        (a8r8g8b8)
 * a1     - mask       (a8)
 * a2     - src_top    (a8r8g8b8)
 * a3     - src_bottom (a8r8g8b8)
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 * 32(sp) - w
 */

    SAVE_REGS_ON_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8

    lw       v1, 60(sp)        /* v1 = w(sp + 32 + 28 save regs stack offset)*/
    beqz     v1, 1f
     nop

    lw       s0, 44(sp)        /* s0 = wt */
    lw       s1, 48(sp)        /* s1 = wb */
    lw       s2, 52(sp)        /* s2 = vx */
    lw       s3, 56(sp)        /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE
    li       s8, 0x00ff00ff

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))

0:
    andi     t4, s2, 0xffff    /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4        /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5        /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4        /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5        /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4        /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 2
    addiu    t8, t9, 4
    lwx      t0, t9(a2)        /* t0 = tl */
    lwx      t1, t8(a2)        /* t1 = tr */
    addiu    v1, v1, -1
    lwx      t2, t9(a3)        /* t2 = bl */
    lwx      t3, t8(a3)        /* t3 = br */

    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, \
                                      t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    lbu      t1, 0(a1)         /* t1 = mask */
    lw       t2, 0(a0)         /* t2 = dst */
    addiu    a1, a1, 1
    OVER_8888_8_8888 t0, t1, t2, t0, s8, t3, t4, t5, t6

    addu     s2, s2, s3        /* vx += unit_x; */
    sw       t0, 0(a0)
    bnez     v1, 0b
     addiu   a0, a0, 4

1:
    RESTORE_REGS_FROM_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_8888_8_8888_OVER_asm_mips)

LEAF_MIPS_DSPR2(pixman_scaled_bilinear_scanline_8888_8_8888_ADD_asm_mips)
/*
 * a0     - *dst
 * a1     - *mask
 * a2     - *src_top
 * a3     - *src_bottom
 * 16(sp) - wt
 * 20(sp) - wb
 * 24(sp) - vx
 * 28(sp) - unit_x
 * 32(sp) - w
 */

    lw       v1, 32(sp)
    beqz     v1, 1f
     nop

    SAVE_REGS_ON_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8

    lw       s0, 44(sp)        /* s0 = wt */
    lw       s1, 48(sp)        /* s1 = wb */
    lw       s2, 52(sp)        /* s2 = vx */
    lw       s3, 56(sp)        /* s3 = unit_x */
    li       v0, BILINEAR_INTERPOLATION_RANGE
    li       s8, 0x00ff00ff

    sll      s0, s0, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
    sll      s1, s1, (2 * (8 - BILINEAR_INTERPOLATION_BITS))
0:
    andi     t4, s2, 0xffff    /* t4 = (short)vx */
    srl      t4, t4, (16 - BILINEAR_INTERPOLATION_BITS) /* t4 = vx >> 8 */
    subu     t5, v0, t4        /* t5 = ( 256 - (vx>>8)) */

    mul      s4, s0, t5        /* s4 = wt*(256-(vx>>8)) */
    mul      s5, s0, t4        /* s5 = wt*(vx>>8) */
    mul      s6, s1, t5        /* s6 = wb*(256-(vx>>8)) */
    mul      s7, s1, t4        /* s7 = wb*(vx>>8) */

    sra      t9, s2, 16
    sll      t9, t9, 2
    addiu    t8, t9, 4
    lwx      t0, t9(a2)        /* t0 = tl */
    lwx      t1, t8(a2)        /* t1 = tr */
    addiu    v1, v1, -1
    lwx      t2, t9(a3)        /* t2 = bl */
    lwx      t3, t8(a3)        /* t3 = br */

    BILINEAR_INTERPOLATE_SINGLE_PIXEL t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, s4, s5, s6, s7
    lbu      t1, 0(a1)         /* t1 = mask */
    lw       t2, 0(a0)         /* t2 = dst */
    addiu    a1, a1, 1
    MIPS_UN8x4_MUL_UN8_ADD_UN8x4 t0, t1, t2, t0, s8, t3, t4, t5

    addu     s2, s2, s3        /* vx += unit_x; */
    sw       t0, 0(a0)
    bnez     v1, 0b
     addiu   a0, a0, 4

    RESTORE_REGS_FROM_STACK 28, v0, v1, s0, s1, s2, s3, s4, s5, s6, s7, s8
1:
    j        ra
     nop

END(pixman_scaled_bilinear_scanline_8888_8_8888_ADD_asm_mips)
