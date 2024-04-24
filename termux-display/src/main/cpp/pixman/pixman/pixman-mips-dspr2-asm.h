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

#ifndef PIXMAN_MIPS_DSPR2_ASM_H
#define PIXMAN_MIPS_DSPR2_ASM_H

#define zero $0
#define AT   $1
#define v0   $2
#define v1   $3
#define a0   $4
#define a1   $5
#define a2   $6
#define a3   $7
#define t0   $8
#define t1   $9
#define t2   $10
#define t3   $11
#define t4   $12
#define t5   $13
#define t6   $14
#define t7   $15
#define s0   $16
#define s1   $17
#define s2   $18
#define s3   $19
#define s4   $20
#define s5   $21
#define s6   $22
#define s7   $23
#define t8   $24
#define t9   $25
#define k0   $26
#define k1   $27
#define gp   $28
#define sp   $29
#define fp   $30
#define s8   $30
#define ra   $31

/*
 * LEAF_MIPS32R2 - declare leaf routine for MIPS32r2
 */
#define LEAF_MIPS32R2(symbol)                           \
                .globl  symbol;                         \
                .align  2;                              \
                .hidden symbol;                         \
                .type   symbol, @function;              \
                .ent    symbol, 0;                      \
symbol:         .frame  sp, 0, ra;                      \
                .set    push;                           \
                .set    arch=mips32r2;                  \
                .set    noreorder;                      \
                .set    noat;

/*
 * LEAF_MIPS32R2 - declare leaf routine for MIPS DSPr2
 */
#define LEAF_MIPS_DSPR2(symbol)                         \
LEAF_MIPS32R2(symbol)                                   \
                .set    dspr2;

/*
 * END - mark end of function
 */
#define END(function)                                   \
                .set    pop;                            \
                .end    function;                       \
                .size   function,.-function

/*
 * Checks if stack offset is big enough for storing/restoring regs_num
 * number of register to/from stack. Stack offset must be greater than
 * or equal to the number of bytes needed for storing registers (regs_num*4).
 * Since MIPS ABI allows usage of first 16 bytes of stack frame (this is
 * preserved for input arguments of the functions, already stored in a0-a3),
 * stack size can be further optimized by utilizing this space.
 */
.macro CHECK_STACK_OFFSET regs_num, stack_offset
.if \stack_offset < \regs_num * 4 - 16
.error "Stack offset too small."
.endif
.endm

/*
 * Saves set of registers on stack. Maximum number of registers that
 * can be saved on stack is limitted to 14 (a0-a3, v0-v1 and s0-s7).
 * Stack offset is number of bytes that are added to stack pointer (sp)
 * before registers are pushed in order to provide enough space on stack
 * (offset must be multiple of 4, and must be big enough, as described by
 * CHECK_STACK_OFFSET macro). This macro is intended to be used in
 * combination with RESTORE_REGS_FROM_STACK macro. Example:
 *  SAVE_REGS_ON_STACK      4, v0, v1, s0, s1
 *  RESTORE_REGS_FROM_STACK 4, v0, v1, s0, s1
 */
.macro SAVE_REGS_ON_STACK stack_offset = 0, r1, \
                          r2  = 0, r3  = 0, r4  = 0, \
                          r5  = 0, r6  = 0, r7  = 0, \
                          r8  = 0, r9  = 0, r10 = 0, \
                          r11 = 0, r12 = 0, r13 = 0, \
                          r14 = 0
    .if (\stack_offset < 0) || (\stack_offset - (\stack_offset / 4) * 4)
    .error "Stack offset must be pozitive and multiple of 4."
    .endif
    .if \stack_offset != 0
    addiu           sp, sp, -\stack_offset
    .endif
    sw              \r1, 0(sp)
    .if \r2 != 0
    sw              \r2, 4(sp)
    .endif
    .if \r3 != 0
    sw              \r3, 8(sp)
    .endif
    .if \r4 != 0
    sw              \r4, 12(sp)
    .endif
    .if \r5 != 0
    CHECK_STACK_OFFSET 5, \stack_offset
    sw              \r5, 16(sp)
    .endif
    .if \r6 != 0
    CHECK_STACK_OFFSET 6, \stack_offset
    sw              \r6, 20(sp)
    .endif
    .if \r7 != 0
    CHECK_STACK_OFFSET 7, \stack_offset
    sw              \r7, 24(sp)
    .endif
    .if \r8 != 0
    CHECK_STACK_OFFSET 8, \stack_offset
    sw              \r8, 28(sp)
    .endif
    .if \r9 != 0
    CHECK_STACK_OFFSET 9, \stack_offset
    sw              \r9, 32(sp)
    .endif
    .if \r10 != 0
    CHECK_STACK_OFFSET 10, \stack_offset
    sw              \r10, 36(sp)
    .endif
    .if \r11 != 0
    CHECK_STACK_OFFSET 11, \stack_offset
    sw              \r11, 40(sp)
    .endif
    .if \r12 != 0
    CHECK_STACK_OFFSET 12, \stack_offset
    sw              \r12, 44(sp)
    .endif
    .if \r13 != 0
    CHECK_STACK_OFFSET 13, \stack_offset
    sw              \r13, 48(sp)
    .endif
    .if \r14 != 0
    CHECK_STACK_OFFSET 14, \stack_offset
    sw              \r14, 52(sp)
    .endif
.endm

/*
 * Restores set of registers from stack. Maximum number of registers that
 * can be restored from stack is limitted to 14 (a0-a3, v0-v1 and s0-s7).
 * Stack offset is number of bytes that are added to stack pointer (sp)
 * after registers are restored (offset must be multiple of 4, and must
 * be big enough, as described by CHECK_STACK_OFFSET macro). This macro is
 * intended to be used in combination with RESTORE_REGS_FROM_STACK macro.
 * Example:
 *  SAVE_REGS_ON_STACK      4, v0, v1, s0, s1
 *  RESTORE_REGS_FROM_STACK 4, v0, v1, s0, s1
 */
.macro RESTORE_REGS_FROM_STACK stack_offset = 0, r1, \
                               r2  = 0, r3  = 0, r4  = 0, \
                               r5  = 0, r6  = 0, r7  = 0, \
                               r8  = 0, r9  = 0, r10 = 0, \
                               r11 = 0, r12 = 0, r13 = 0, \
                               r14 = 0
    .if (\stack_offset < 0) || (\stack_offset - (\stack_offset/4)*4)
    .error "Stack offset must be pozitive and multiple of 4."
    .endif
    lw              \r1, 0(sp)
    .if \r2 != 0
    lw              \r2, 4(sp)
    .endif
    .if \r3 != 0
    lw              \r3, 8(sp)
    .endif
    .if \r4 != 0
    lw              \r4, 12(sp)
    .endif
    .if \r5 != 0
    CHECK_STACK_OFFSET 5, \stack_offset
    lw              \r5, 16(sp)
    .endif
    .if \r6 != 0
    CHECK_STACK_OFFSET 6, \stack_offset
    lw              \r6, 20(sp)
    .endif
    .if \r7 != 0
    CHECK_STACK_OFFSET 7, \stack_offset
    lw              \r7, 24(sp)
    .endif
    .if \r8 != 0
    CHECK_STACK_OFFSET 8, \stack_offset
    lw              \r8, 28(sp)
    .endif
    .if \r9 != 0
    CHECK_STACK_OFFSET 9, \stack_offset
    lw              \r9, 32(sp)
    .endif
    .if \r10 != 0
    CHECK_STACK_OFFSET 10, \stack_offset
    lw              \r10, 36(sp)
    .endif
    .if \r11 != 0
    CHECK_STACK_OFFSET 11, \stack_offset
    lw              \r11, 40(sp)
    .endif
    .if \r12 != 0
    CHECK_STACK_OFFSET 12, \stack_offset
    lw              \r12, 44(sp)
    .endif
    .if \r13 != 0
    CHECK_STACK_OFFSET 13, \stack_offset
    lw              \r13, 48(sp)
    .endif
    .if \r14 != 0
    CHECK_STACK_OFFSET 14, \stack_offset
    lw              \r14, 52(sp)
    .endif
    .if \stack_offset != 0
    addiu           sp, sp, \stack_offset
    .endif
.endm

/*
 * Conversion of single r5g6b5 pixel (in_565) to single a8r8g8b8 pixel
 * returned in (out_8888) register. Requires two temporary registers
 * (scratch1 and scratch2).
 */
.macro CONVERT_1x0565_TO_1x8888 in_565,   \
                                out_8888, \
                                scratch1, scratch2
    lui     \out_8888, 0xff00
    sll     \scratch1, \in_565,   0x3
    andi    \scratch2, \scratch1, 0xff
    ext     \scratch1, \in_565,   0x2, 0x3
    or      \scratch1, \scratch2, \scratch1
    or      \out_8888, \out_8888, \scratch1

    sll     \scratch1, \in_565,   0x5
    andi    \scratch1, \scratch1, 0xfc00
    srl     \scratch2, \in_565,   0x1
    andi    \scratch2, \scratch2, 0x300
    or      \scratch2, \scratch1, \scratch2
    or      \out_8888, \out_8888, \scratch2

    andi    \scratch1, \in_565,   0xf800
    srl     \scratch2, \scratch1, 0x5
    andi    \scratch2, \scratch2, 0xff00
    or      \scratch1, \scratch1, \scratch2
    sll     \scratch1, \scratch1, 0x8
    or      \out_8888, \out_8888, \scratch1
.endm

/*
 * Conversion of two r5g6b5 pixels (in1_565 and in2_565) to two a8r8g8b8 pixels
 * returned in (out1_8888 and out2_8888) registers. Requires four scratch
 * registers (scratch1 ... scratch4). It also requires maskG and maskB for
 * color component extractions. These masks must have following values:
 *   li       maskG, 0x07e007e0
 *   li       maskB, 0x001F001F
 */
.macro CONVERT_2x0565_TO_2x8888 in1_565, in2_565,     \
                                out1_8888, out2_8888, \
                                maskG, maskB,         \
                                scratch1, scratch2, scratch3, scratch4
    sll               \scratch1,  \in1_565,   16
    or                \scratch1,  \scratch1,  \in2_565
    lui               \out2_8888, 0xff00
    ori               \out2_8888, \out2_8888, 0xff00
    shrl.ph           \scratch2,  \scratch1,  11
    and               \scratch3,  \scratch1,  \maskG
    shra.ph           \scratch4,  \scratch2,  2
    shll.ph           \scratch2,  \scratch2,  3
    shll.ph           \scratch3,  \scratch3,  5
    or                \scratch2,  \scratch2,  \scratch4
    shrl.qb           \scratch4,  \scratch3,  6
    or                \out2_8888, \out2_8888, \scratch2
    or                \scratch3,  \scratch3,  \scratch4
    and               \scratch1,  \scratch1,  \maskB
    shll.ph           \scratch2,  \scratch1,  3
    shra.ph           \scratch4,  \scratch1,  2
    or                \scratch2,  \scratch2,  \scratch4
    or                \scratch3,  \scratch2,  \scratch3
    precrq.ph.w       \out1_8888, \out2_8888, \scratch3
    precr_sra.ph.w    \out2_8888, \scratch3,  0
.endm

/*
 * Conversion of single a8r8g8b8 pixel (in_8888) to single r5g6b5 pixel
 * returned in (out_565) register. Requires two temporary registers
 * (scratch1 and scratch2).
 */
.macro CONVERT_1x8888_TO_1x0565 in_8888, \
                                out_565, \
                                scratch1, scratch2
    ext     \out_565,  \in_8888,  0x3, 0x5
    srl     \scratch1, \in_8888,  0x5
    andi    \scratch1, \scratch1, 0x07e0
    srl     \scratch2, \in_8888,  0x8
    andi    \scratch2, \scratch2, 0xf800
    or      \out_565,  \out_565,  \scratch1
    or      \out_565,  \out_565,  \scratch2
.endm

/*
 * Conversion of two a8r8g8b8 pixels (in1_8888 and in2_8888) to two r5g6b5
 * pixels returned in (out1_565 and out2_565) registers. Requires two temporary
 * registers (scratch1 and scratch2). It also requires maskR, maskG and maskB
 * for color component extractions. These masks must have following values:
 *   li       maskR, 0xf800f800
 *   li       maskG, 0x07e007e0
 *   li       maskB, 0x001F001F
 * Value of input register in2_8888 is lost.
 */
.macro CONVERT_2x8888_TO_2x0565 in1_8888, in2_8888,  \
                                out1_565, out2_565,  \
                                maskR, maskG, maskB, \
                                scratch1, scratch2
    precr.qb.ph    \scratch1, \in2_8888, \in1_8888
    precrq.qb.ph   \in2_8888, \in2_8888, \in1_8888
    and            \out1_565, \scratch1, \maskR
    shrl.ph        \scratch1, \scratch1, 3
    shll.ph        \in2_8888, \in2_8888, 3
    and            \scratch1, \scratch1, \maskB
    or             \out1_565, \out1_565, \scratch1
    and            \in2_8888, \in2_8888, \maskG
    or             \out1_565, \out1_565, \in2_8888
    srl            \out2_565, \out1_565, 16
.endm

/*
 * Multiply pixel (a8) with single pixel (a8r8g8b8). It requires maskLSR needed
 * for rounding process. maskLSR must have following value:
 *   li       maskLSR, 0x00ff00ff
 */
.macro MIPS_UN8x4_MUL_UN8 s_8888,  \
                          m_8,     \
                          d_8888,  \
                          maskLSR, \
                          scratch1, scratch2, scratch3
    replv.ph          \m_8,      \m_8                 /*   0 | M | 0 | M */
    muleu_s.ph.qbl    \scratch1, \s_8888,   \m_8      /*    A*M  |  R*M */
    muleu_s.ph.qbr    \scratch2, \s_8888,   \m_8      /*    G*M  |  B*M */
    shra_r.ph         \scratch3, \scratch1, 8
    shra_r.ph         \d_8888,   \scratch2, 8
    and               \scratch3, \scratch3, \maskLSR  /*   0 |A*M| 0 |R*M */
    and               \d_8888,   \d_8888,   \maskLSR  /*   0 |G*M| 0 |B*M */
    addq.ph           \scratch1, \scratch1, \scratch3 /* A*M+A*M | R*M+R*M */
    addq.ph           \scratch2, \scratch2, \d_8888   /* G*M+G*M | B*M+B*M */
    shra_r.ph         \scratch1, \scratch1, 8
    shra_r.ph         \scratch2, \scratch2, 8
    precr.qb.ph       \d_8888,   \scratch1, \scratch2
.endm

/*
 * Multiply two pixels (a8) with two pixels (a8r8g8b8). It requires maskLSR
 * needed for rounding process. maskLSR must have following value:
 *   li       maskLSR, 0x00ff00ff
 */
.macro MIPS_2xUN8x4_MUL_2xUN8 s1_8888, \
                              s2_8888, \
                              m1_8,    \
                              m2_8,    \
                              d1_8888, \
                              d2_8888, \
                              maskLSR, \
                              scratch1, scratch2, scratch3, \
                              scratch4, scratch5, scratch6
    replv.ph          \m1_8,     \m1_8                /*  0 | M1 | 0 | M1 */
    replv.ph          \m2_8,     \m2_8                /*  0 | M2 | 0 | M2 */
    muleu_s.ph.qbl    \scratch1, \s1_8888,  \m1_8     /*  A1*M1  |  R1*M1 */
    muleu_s.ph.qbr    \scratch2, \s1_8888,  \m1_8     /*  G1*M1  |  B1*M1 */
    muleu_s.ph.qbl    \scratch3, \s2_8888,  \m2_8     /*  A2*M2  |  R2*M2 */
    muleu_s.ph.qbr    \scratch4, \s2_8888,  \m2_8     /*  G2*M2  |  B2*M2 */
    shra_r.ph         \scratch5, \scratch1, 8
    shra_r.ph         \d1_8888,  \scratch2, 8
    shra_r.ph         \scratch6, \scratch3, 8
    shra_r.ph         \d2_8888,  \scratch4, 8
    and               \scratch5, \scratch5, \maskLSR  /* 0 |A1*M1| 0 |R1*M1 */
    and               \d1_8888,  \d1_8888,  \maskLSR  /* 0 |G1*M1| 0 |B1*M1 */
    and               \scratch6, \scratch6, \maskLSR  /* 0 |A2*M2| 0 |R2*M2 */
    and               \d2_8888,  \d2_8888,  \maskLSR  /* 0 |G2*M2| 0 |B2*M2 */
    addq.ph           \scratch1, \scratch1, \scratch5
    addq.ph           \scratch2, \scratch2, \d1_8888
    addq.ph           \scratch3, \scratch3, \scratch6
    addq.ph           \scratch4, \scratch4, \d2_8888
    shra_r.ph         \scratch1, \scratch1, 8
    shra_r.ph         \scratch2, \scratch2, 8
    shra_r.ph         \scratch3, \scratch3, 8
    shra_r.ph         \scratch4, \scratch4, 8
    precr.qb.ph       \d1_8888,  \scratch1, \scratch2
    precr.qb.ph       \d2_8888,  \scratch3, \scratch4
.endm

/*
 * Multiply pixel (a8r8g8b8) with single pixel (a8r8g8b8). It requires maskLSR
 * needed for rounding process. maskLSR must have following value:
 *   li       maskLSR, 0x00ff00ff
 */
.macro MIPS_UN8x4_MUL_UN8x4 s_8888,  \
                            m_8888,  \
                            d_8888,  \
                            maskLSR, \
                            scratch1, scratch2, scratch3, scratch4
    preceu.ph.qbl     \scratch1, \m_8888              /*   0 | A | 0 | R */
    preceu.ph.qbr     \scratch2, \m_8888              /*   0 | G | 0 | B */
    muleu_s.ph.qbl    \scratch3, \s_8888,   \scratch1 /*    A*A  |  R*R */
    muleu_s.ph.qbr    \scratch4, \s_8888,   \scratch2 /*    G*G  |  B*B */
    shra_r.ph         \scratch1, \scratch3, 8
    shra_r.ph         \scratch2, \scratch4, 8
    and               \scratch1, \scratch1, \maskLSR  /*   0 |A*A| 0 |R*R */
    and               \scratch2, \scratch2, \maskLSR  /*   0 |G*G| 0 |B*B */
    addq.ph           \scratch1, \scratch1, \scratch3
    addq.ph           \scratch2, \scratch2, \scratch4
    shra_r.ph         \scratch1, \scratch1, 8
    shra_r.ph         \scratch2, \scratch2, 8
    precr.qb.ph       \d_8888,   \scratch1, \scratch2
.endm

/*
 * Multiply two pixels (a8r8g8b8) with two pixels (a8r8g8b8). It requires
 * maskLSR needed for rounding process. maskLSR must have following value:
 *   li       maskLSR, 0x00ff00ff
 */

.macro MIPS_2xUN8x4_MUL_2xUN8x4 s1_8888,  \
                                s2_8888,  \
                                m1_8888,  \
                                m2_8888,  \
                                d1_8888,  \
                                d2_8888,  \
                                maskLSR,  \
                                scratch1, scratch2, scratch3, \
                                scratch4, scratch5, scratch6
    preceu.ph.qbl     \scratch1, \m1_8888             /*   0 | A | 0 | R */
    preceu.ph.qbr     \scratch2, \m1_8888             /*   0 | G | 0 | B */
    preceu.ph.qbl     \scratch3, \m2_8888             /*   0 | A | 0 | R */
    preceu.ph.qbr     \scratch4, \m2_8888             /*   0 | G | 0 | B */
    muleu_s.ph.qbl    \scratch5, \s1_8888,  \scratch1 /*    A*A  |  R*R */
    muleu_s.ph.qbr    \scratch6, \s1_8888,  \scratch2 /*    G*G  |  B*B */
    muleu_s.ph.qbl    \scratch1, \s2_8888,  \scratch3 /*    A*A  |  R*R */
    muleu_s.ph.qbr    \scratch2, \s2_8888,  \scratch4 /*    G*G  |  B*B */
    shra_r.ph         \scratch3, \scratch5, 8
    shra_r.ph         \scratch4, \scratch6, 8
    shra_r.ph         \d1_8888,  \scratch1, 8
    shra_r.ph         \d2_8888,  \scratch2, 8
    and               \scratch3, \scratch3, \maskLSR  /*   0 |A*A| 0 |R*R */
    and               \scratch4, \scratch4, \maskLSR  /*   0 |G*G| 0 |B*B */
    and               \d1_8888,  \d1_8888,  \maskLSR  /*   0 |A*A| 0 |R*R */
    and               \d2_8888,  \d2_8888,  \maskLSR  /*   0 |G*G| 0 |B*B */
    addq.ph           \scratch3, \scratch3, \scratch5
    addq.ph           \scratch4, \scratch4, \scratch6
    addq.ph           \d1_8888,  \d1_8888,  \scratch1
    addq.ph           \d2_8888,  \d2_8888,  \scratch2
    shra_r.ph         \scratch3, \scratch3, 8
    shra_r.ph         \scratch4, \scratch4, 8
    shra_r.ph         \scratch5, \d1_8888,  8
    shra_r.ph         \scratch6, \d2_8888,  8
    precr.qb.ph       \d1_8888,  \scratch3, \scratch4
    precr.qb.ph       \d2_8888,  \scratch5, \scratch6
.endm

/*
 * OVER operation on single a8r8g8b8 source pixel (s_8888) and single a8r8g8b8
 * destination pixel (d_8888) using a8 mask (m_8). It also requires maskLSR
 * needed for rounding process. maskLSR must have following value:
 *   li       maskLSR, 0x00ff00ff
 */
.macro OVER_8888_8_8888 s_8888,   \
                        m_8,      \
                        d_8888,   \
                        out_8888, \
                        maskLSR,  \
                        scratch1, scratch2, scratch3, scratch4
    MIPS_UN8x4_MUL_UN8 \s_8888,   \m_8, \
                       \scratch1, \maskLSR, \
                       \scratch2, \scratch3, \scratch4

    not                \scratch2, \scratch1
    srl                \scratch2, \scratch2, 24

    MIPS_UN8x4_MUL_UN8 \d_8888,   \scratch2, \
                       \d_8888,   \maskLSR,  \
                       \scratch3, \scratch4, \out_8888

    addu_s.qb          \out_8888, \d_8888,   \scratch1
.endm

/*
 * OVER operation on two a8r8g8b8 source pixels (s1_8888 and s2_8888) and two
 * a8r8g8b8 destination pixels (d1_8888 and d2_8888) using a8 masks (m1_8 and
 * m2_8). It also requires maskLSR needed for rounding process. maskLSR must
 * have following value:
 *   li       maskLSR, 0x00ff00ff
 */
.macro OVER_2x8888_2x8_2x8888 s1_8888,   \
                              s2_8888,   \
                              m1_8,      \
                              m2_8,      \
                              d1_8888,   \
                              d2_8888,   \
                              out1_8888, \
                              out2_8888, \
                              maskLSR,   \
                              scratch1, scratch2, scratch3, \
                              scratch4, scratch5, scratch6
    MIPS_2xUN8x4_MUL_2xUN8 \s1_8888,   \s2_8888, \
                           \m1_8,      \m2_8, \
                           \scratch1,  \scratch2, \
                           \maskLSR, \
                           \scratch3,  \scratch4, \out1_8888, \
                           \out2_8888, \scratch5, \scratch6

    not                    \scratch3,  \scratch1
    srl                    \scratch3,  \scratch3, 24
    not                    \scratch4,  \scratch2
    srl                    \scratch4,  \scratch4, 24

    MIPS_2xUN8x4_MUL_2xUN8 \d1_8888,   \d2_8888, \
                           \scratch3,  \scratch4, \
                           \d1_8888,   \d2_8888, \
                           \maskLSR, \
                           \scratch5,  \scratch6, \out1_8888, \
                           \out2_8888, \scratch3, \scratch4

    addu_s.qb              \out1_8888, \d1_8888,  \scratch1
    addu_s.qb              \out2_8888, \d2_8888,  \scratch2
.endm

/*
 * OVER operation on single a8r8g8b8 source pixel (s_8888) and single a8r8g8b8
 * destination pixel (d_8888). It also requires maskLSR needed for rounding
 * process. maskLSR must have following value:
 *   li       maskLSR, 0x00ff00ff
 */
.macro OVER_8888_8888 s_8888,   \
                      d_8888,   \
                      out_8888, \
                      maskLSR,  \
                      scratch1, scratch2, scratch3, scratch4
    not                \scratch1, \s_8888
    srl                \scratch1, \scratch1, 24

    MIPS_UN8x4_MUL_UN8 \d_8888,   \scratch1, \
                       \out_8888, \maskLSR, \
                       \scratch2, \scratch3, \scratch4

    addu_s.qb          \out_8888, \out_8888, \s_8888
.endm

/*
 * OVER operation on two a8r8g8b8 source pixels (s1_8888 and s2_8888) and two
 * a8r8g8b8 destination pixels (d1_8888 and d2_8888). It also requires maskLSR
 * needed for rounding process. maskLSR must have following value:
 *   li       maskLSR, 0x00ff00ff
 */
.macro OVER_2x8888_2x8888 s1_8888,   \
                          s2_8888,   \
                          d1_8888,   \
                          d2_8888,   \
                          out1_8888, \
                          out2_8888, \
                          maskLSR,   \
                          scratch1, scratch2, scratch3, \
                          scratch4, scratch5, scratch6
    not                    \scratch1,  \s1_8888
    srl                    \scratch1,  \scratch1,  24
    not                    \scratch2,  \s2_8888
    srl                    \scratch2,  \scratch2,  24
    MIPS_2xUN8x4_MUL_2xUN8 \d1_8888,   \d2_8888, \
                           \scratch1,  \scratch2,  \
                           \out1_8888, \out2_8888, \
                           \maskLSR, \
                           \scratch3,  \scratch4, \scratch5, \
                           \scratch6,  \d1_8888,  \d2_8888

    addu_s.qb              \out1_8888, \out1_8888, \s1_8888
    addu_s.qb              \out2_8888, \out2_8888, \s2_8888
.endm

.macro MIPS_UN8x4_MUL_UN8_ADD_UN8x4 s_8888,   \
                                    m_8,      \
                                    d_8888,   \
                                    out_8888, \
                                    maskLSR,  \
                                    scratch1, scratch2, scratch3
    MIPS_UN8x4_MUL_UN8 \s_8888, \m_8, \
                       \out_8888, \maskLSR, \
                       \scratch1, \scratch2, \scratch3

    addu_s.qb          \out_8888, \out_8888, \d_8888
.endm

.macro MIPS_2xUN8x4_MUL_2xUN8_ADD_2xUN8x4 s1_8888,   \
                             s2_8888,   \
                             m1_8,      \
                             m2_8,      \
                             d1_8888,   \
                             d2_8888,   \
                             out1_8888, \
                             out2_8888, \
                             maskLSR,   \
                             scratch1,  scratch2, scratch3, \
                             scratch4, scratch5, scratch6
    MIPS_2xUN8x4_MUL_2xUN8 \s1_8888,   \s2_8888, \
                           \m1_8,      \m2_8, \
                           \out1_8888, \out2_8888, \
                           \maskLSR, \
                           \scratch1,  \scratch2, \scratch3, \
                           \scratch4,  \scratch5, \scratch6

    addu_s.qb             \out1_8888, \out1_8888, \d1_8888
    addu_s.qb             \out2_8888, \out2_8888, \d2_8888
.endm

.macro BILINEAR_INTERPOLATE_SINGLE_PIXEL tl, tr, bl, br,         \
                                         scratch1, scratch2,     \
                                         alpha, red, green, blue \
                                         wt1, wt2, wb1, wb2
    andi            \scratch1, \tl,  0xff
    andi            \scratch2, \tr,  0xff
    andi            \alpha,    \bl,  0xff
    andi            \red,      \br,  0xff

    multu           $ac0,      \wt1, \scratch1
    maddu           $ac0,      \wt2, \scratch2
    maddu           $ac0,      \wb1, \alpha
    maddu           $ac0,      \wb2, \red

    ext             \scratch1, \tl,  8, 8
    ext             \scratch2, \tr,  8, 8
    ext             \alpha,    \bl,  8, 8
    ext             \red,      \br,  8, 8

    multu           $ac1,      \wt1, \scratch1
    maddu           $ac1,      \wt2, \scratch2
    maddu           $ac1,      \wb1, \alpha
    maddu           $ac1,      \wb2, \red

    ext             \scratch1, \tl,  16, 8
    ext             \scratch2, \tr,  16, 8
    ext             \alpha,    \bl,  16, 8
    ext             \red,      \br,  16, 8

    mflo            \blue,     $ac0

    multu           $ac2,      \wt1, \scratch1
    maddu           $ac2,      \wt2, \scratch2
    maddu           $ac2,      \wb1, \alpha
    maddu           $ac2,      \wb2, \red

    ext             \scratch1, \tl,  24, 8
    ext             \scratch2, \tr,  24, 8
    ext             \alpha,    \bl,  24, 8
    ext             \red,      \br,  24, 8

    mflo            \green,    $ac1

    multu           $ac3,      \wt1, \scratch1
    maddu           $ac3,      \wt2, \scratch2
    maddu           $ac3,      \wb1, \alpha
    maddu           $ac3,      \wb2, \red

    mflo            \red,      $ac2
    mflo            \alpha,    $ac3

    precr.qb.ph     \alpha,    \alpha, \red
    precr.qb.ph     \scratch1, \green, \blue
    precrq.qb.ph    \tl,       \alpha, \scratch1
.endm

#endif //PIXMAN_MIPS_DSPR2_ASM_H
