/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*            	Copyright (C) 1996-1999 SciTech Software, Inc.
* 				     Copyright (C) David Mosberger-Tang
* 					   Copyright (C) 1999 Egbert Eich
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:		ANSI C
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  This file includes subroutines to implement the decoding
*               and emulation of all the x86 processor instructions.
*
* There are approximately 250 subroutines in here, which correspond
* to the 256 byte-"opcodes" found on the 8086.  The table which
* dispatches this is found in the files optab.[ch].
*
* Each opcode proc has a comment preceding it which gives its table
* address.  Several opcodes are missing (undefined) in the table.
*
* Each proc includes information for decoding (DECODE_PRINTF and
* DECODE_PRINTF2), debugging (TRACE_REGS, SINGLE_STEP), and misc
* functions (START_OF_INSTR, END_OF_INSTR).
*
* Many of the procedures are *VERY* similar in coding.  This has
* allowed for a very large amount of code to be generated in a fairly
* short amount of time (i.e. cut, paste, and modify).  The result is
* that much of the code below could have been folded into subroutines
* for a large reduction in size of this file.  The downside would be
* that there would be a penalty in execution speed.  The file could
* also have been *MUCH* larger by inlining certain functions which
* were called.  This could have resulted even faster execution.  The
* prime directive I used to decide whether to inline the code or to
* modularize it, was basically: 1) no unnecessary subroutine calls,
* 2) no routines more than about 200 lines in size, and 3) modularize
* any code that I might not get right the first time.  The fetch_*
* subroutines fall into the latter category.  The The decode_* fall
* into the second category.  The coding of the "switch(mod){ .... }"
* in many of the subroutines below falls into the first category.
* Especially, the coding of {add,and,or,sub,...}_{byte,word}
* subroutines are an especially glaring case of the third guideline.
* Since so much of the code is cloned from other modules (compare
* opcode #00 to opcode #01), making the basic operations subroutine
* calls is especially important; otherwise mistakes in coding an
* "add" would represent a nightmare in maintenance.
*
****************************************************************************/

#include "x86emu/x86emui.h"

/*----------------------------- Implementation ----------------------------*/

/****************************************************************************
PARAMETERS:
op1 - Instruction op code

REMARKS:
Handles illegal opcodes.
****************************************************************************/
static void
x86emuOp_illegal_op(u8 op1)
{
    START_OF_INSTR();
    if (M.x86.R_SP != 0) {
        DECODE_PRINTF("ILLEGAL X86 OPCODE\n");
        TRACE_REGS();
        DB(printk("%04x:%04x: %02X ILLEGAL X86 OPCODE!\n",
                  M.x86.R_CS, M.x86.R_IP - 1, op1));
        HALT_SYS();
    }
    else {
        /* If we get here, it means the stack pointer is back to zero
         * so we are just returning from an emulator service call
         * so therte is no need to display an error message. We trap
         * the emulator with an 0xF1 opcode to finish the service
         * call.
         */
        X86EMU_halt_sys();
    }
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x00
****************************************************************************/
static void
x86emuOp_add_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;
    u8 *destreg, *srcreg;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("ADD\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = add_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = add_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = add_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = add_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x01
****************************************************************************/
static void
x86emuOp_add_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("ADD\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = add_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = add_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = add_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = add_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = add_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = add_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x02
****************************************************************************/
static void
x86emuOp_add_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("ADD\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = add_byte(*destreg, srcval);
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = add_byte(*destreg, srcval);
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = add_byte(*destreg, srcval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = add_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x03
****************************************************************************/
static void
x86emuOp_add_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("ADD\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_word(*destreg, srcval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_word(*destreg, srcval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_word(*destreg, srcval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = add_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x04
****************************************************************************/
static void
x86emuOp_add_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("ADD\tAL,");
    srcval = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    M.x86.R_AL = add_byte(M.x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x05
****************************************************************************/
static void
x86emuOp_add_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("ADD\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("ADD\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = add_long(M.x86.R_EAX, srcval);
    }
    else {
        M.x86.R_AX = add_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x06
****************************************************************************/
static void
x86emuOp_push_ES(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("PUSH\tES\n");
    TRACE_AND_STEP();
    push_word(M.x86.R_ES);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x07
****************************************************************************/
static void
x86emuOp_pop_ES(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("POP\tES\n");
    TRACE_AND_STEP();
    M.x86.R_ES = pop_word();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x08
****************************************************************************/
static void
x86emuOp_or_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("OR\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = or_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = or_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = or_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = or_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x09
****************************************************************************/
static void
x86emuOp_or_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("OR\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = or_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = or_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = or_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = or_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = or_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = or_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x0a
****************************************************************************/
static void
x86emuOp_or_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("OR\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = or_byte(*destreg, srcval);
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = or_byte(*destreg, srcval);
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = or_byte(*destreg, srcval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = or_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x0b
****************************************************************************/
static void
x86emuOp_or_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("OR\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_word(*destreg, srcval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_word(*destreg, srcval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_word(*destreg, srcval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = or_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x0c
****************************************************************************/
static void
x86emuOp_or_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("OR\tAL,");
    srcval = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    M.x86.R_AL = or_byte(M.x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x0d
****************************************************************************/
static void
x86emuOp_or_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("OR\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("OR\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = or_long(M.x86.R_EAX, srcval);
    }
    else {
        M.x86.R_AX = or_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x0e
****************************************************************************/
static void
x86emuOp_push_CS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("PUSH\tCS\n");
    TRACE_AND_STEP();
    push_word(M.x86.R_CS);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x0f. Escape for two-byte opcode (286 or better)
****************************************************************************/
static void
x86emuOp_two_byte(u8 X86EMU_UNUSED(op1))
{
    u8 op2 = (*sys_rdb) (((u32) M.x86.R_CS << 4) + (M.x86.R_IP++));

    INC_DECODED_INST_LEN(1);
    (*x86emu_optab2[op2]) (op2);
}

/****************************************************************************
REMARKS:
Handles opcode 0x10
****************************************************************************/
static void
x86emuOp_adc_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("ADC\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = adc_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = adc_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = adc_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = adc_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x11
****************************************************************************/
static void
x86emuOp_adc_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("ADC\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = adc_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = adc_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = adc_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = adc_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = adc_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = adc_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x12
****************************************************************************/
static void
x86emuOp_adc_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("ADC\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = adc_byte(*destreg, srcval);
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = adc_byte(*destreg, srcval);
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = adc_byte(*destreg, srcval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = adc_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x13
****************************************************************************/
static void
x86emuOp_adc_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("ADC\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_word(*destreg, srcval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_word(*destreg, srcval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_word(*destreg, srcval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = adc_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x14
****************************************************************************/
static void
x86emuOp_adc_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("ADC\tAL,");
    srcval = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    M.x86.R_AL = adc_byte(M.x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x15
****************************************************************************/
static void
x86emuOp_adc_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("ADC\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("ADC\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = adc_long(M.x86.R_EAX, srcval);
    }
    else {
        M.x86.R_AX = adc_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x16
****************************************************************************/
static void
x86emuOp_push_SS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("PUSH\tSS\n");
    TRACE_AND_STEP();
    push_word(M.x86.R_SS);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x17
****************************************************************************/
static void
x86emuOp_pop_SS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("POP\tSS\n");
    TRACE_AND_STEP();
    M.x86.R_SS = pop_word();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x18
****************************************************************************/
static void
x86emuOp_sbb_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("SBB\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = sbb_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = sbb_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = sbb_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sbb_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x19
****************************************************************************/
static void
x86emuOp_sbb_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("SBB\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sbb_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sbb_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sbb_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sbb_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sbb_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sbb_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x1a
****************************************************************************/
static void
x86emuOp_sbb_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("SBB\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sbb_byte(*destreg, srcval);
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sbb_byte(*destreg, srcval);
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sbb_byte(*destreg, srcval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sbb_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x1b
****************************************************************************/
static void
x86emuOp_sbb_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("SBB\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_word(*destreg, srcval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_word(*destreg, srcval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_word(*destreg, srcval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sbb_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x1c
****************************************************************************/
static void
x86emuOp_sbb_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("SBB\tAL,");
    srcval = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    M.x86.R_AL = sbb_byte(M.x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x1d
****************************************************************************/
static void
x86emuOp_sbb_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("SBB\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("SBB\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = sbb_long(M.x86.R_EAX, srcval);
    }
    else {
        M.x86.R_AX = sbb_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x1e
****************************************************************************/
static void
x86emuOp_push_DS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("PUSH\tDS\n");
    TRACE_AND_STEP();
    push_word(M.x86.R_DS);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x1f
****************************************************************************/
static void
x86emuOp_pop_DS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("POP\tDS\n");
    TRACE_AND_STEP();
    M.x86.R_DS = pop_word();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x20
****************************************************************************/
static void
x86emuOp_and_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("AND\t");
    FETCH_DECODE_MODRM(mod, rh, rl);

    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = and_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;

    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = and_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;

    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = and_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;

    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = and_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x21
****************************************************************************/
static void
x86emuOp_and_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("AND\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = and_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = and_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = and_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = and_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = and_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = and_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x22
****************************************************************************/
static void
x86emuOp_and_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("AND\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = and_byte(*destreg, srcval);
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = and_byte(*destreg, srcval);
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = and_byte(*destreg, srcval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = and_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x23
****************************************************************************/
static void
x86emuOp_and_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("AND\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_word(*destreg, srcval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_long(*destreg, srcval);
            break;
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_word(*destreg, srcval);
            break;
        }
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_word(*destreg, srcval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = and_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x24
****************************************************************************/
static void
x86emuOp_and_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("AND\tAL,");
    srcval = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    M.x86.R_AL = and_byte(M.x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x25
****************************************************************************/
static void
x86emuOp_and_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("AND\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("AND\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = and_long(M.x86.R_EAX, srcval);
    }
    else {
        M.x86.R_AX = and_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x26
****************************************************************************/
static void
x86emuOp_segovr_ES(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("ES:\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_SEGOVR_ES;
    /*
     * note the lack of DECODE_CLEAR_SEGOVR(r) since, here is one of 4
     * opcode subroutines we do not want to do this.
     */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x27
****************************************************************************/
static void
x86emuOp_daa(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("DAA\n");
    TRACE_AND_STEP();
    M.x86.R_AL = daa_byte(M.x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x28
****************************************************************************/
static void
x86emuOp_sub_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("SUB\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = sub_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = sub_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = sub_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sub_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x29
****************************************************************************/
static void
x86emuOp_sub_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("SUB\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sub_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sub_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sub_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sub_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sub_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = sub_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x2a
****************************************************************************/
static void
x86emuOp_sub_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("SUB\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sub_byte(*destreg, srcval);
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sub_byte(*destreg, srcval);
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sub_byte(*destreg, srcval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = sub_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x2b
****************************************************************************/
static void
x86emuOp_sub_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("SUB\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_word(*destreg, srcval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_word(*destreg, srcval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_word(*destreg, srcval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = sub_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x2c
****************************************************************************/
static void
x86emuOp_sub_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("SUB\tAL,");
    srcval = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    M.x86.R_AL = sub_byte(M.x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x2d
****************************************************************************/
static void
x86emuOp_sub_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("SUB\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("SUB\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = sub_long(M.x86.R_EAX, srcval);
    }
    else {
        M.x86.R_AX = sub_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x2e
****************************************************************************/
static void
x86emuOp_segovr_CS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("CS:\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_SEGOVR_CS;
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x2f
****************************************************************************/
static void
x86emuOp_das(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("DAS\n");
    TRACE_AND_STEP();
    M.x86.R_AL = das_byte(M.x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x30
****************************************************************************/
static void
x86emuOp_xor_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("XOR\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = xor_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = xor_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = xor_byte(destval, *srcreg);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = xor_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x31
****************************************************************************/
static void
x86emuOp_xor_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("XOR\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = xor_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = xor_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = xor_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = xor_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = xor_long(destval, *srcreg);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = xor_word(destval, *srcreg);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x32
****************************************************************************/
static void
x86emuOp_xor_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("XOR\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = xor_byte(*destreg, srcval);
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = xor_byte(*destreg, srcval);
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = xor_byte(*destreg, srcval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = xor_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x33
****************************************************************************/
static void
x86emuOp_xor_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("XOR\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_word(*destreg, srcval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_word(*destreg, srcval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_word(*destreg, srcval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = xor_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x34
****************************************************************************/
static void
x86emuOp_xor_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("XOR\tAL,");
    srcval = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    M.x86.R_AL = xor_byte(M.x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x35
****************************************************************************/
static void
x86emuOp_xor_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("XOR\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("XOR\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = xor_long(M.x86.R_EAX, srcval);
    }
    else {
        M.x86.R_AX = xor_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x36
****************************************************************************/
static void
x86emuOp_segovr_SS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("SS:\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_SEGOVR_SS;
    /* no DECODE_CLEAR_SEGOVR ! */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x37
****************************************************************************/
static void
x86emuOp_aaa(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("AAA\n");
    TRACE_AND_STEP();
    M.x86.R_AX = aaa_word(M.x86.R_AX);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x38
****************************************************************************/
static void
x86emuOp_cmp_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;
    u8 *destreg, *srcreg;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("CMP\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        cmp_byte(destval, *srcreg);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        cmp_byte(destval, *srcreg);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        cmp_byte(destval, *srcreg);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        cmp_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x39
****************************************************************************/
static void
x86emuOp_cmp_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("CMP\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_long(destval, *srcreg);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_word(destval, *srcreg);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_long(destval, *srcreg);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_word(destval, *srcreg);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_long(destval, *srcreg);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_word(destval, *srcreg);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x3a
****************************************************************************/
static void
x86emuOp_cmp_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("CMP\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        cmp_byte(*destreg, srcval);
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        cmp_byte(*destreg, srcval);
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        cmp_byte(*destreg, srcval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        cmp_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x3b
****************************************************************************/
static void
x86emuOp_cmp_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("CMP\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_word(*destreg, srcval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_word(*destreg, srcval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_long(*destreg, srcval);
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_word(*destreg, srcval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            cmp_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x3c
****************************************************************************/
static void
x86emuOp_cmp_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("CMP\tAL,");
    srcval = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    cmp_byte(M.x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x3d
****************************************************************************/
static void
x86emuOp_cmp_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("CMP\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("CMP\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        cmp_long(M.x86.R_EAX, srcval);
    }
    else {
        cmp_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x3e
****************************************************************************/
static void
x86emuOp_segovr_DS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("DS:\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_SEGOVR_DS;
    /* NO DECODE_CLEAR_SEGOVR! */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x3f
****************************************************************************/
static void
x86emuOp_aas(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("AAS\n");
    TRACE_AND_STEP();
    M.x86.R_AX = aas_word(M.x86.R_AX);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x40
****************************************************************************/
static void
x86emuOp_inc_AX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INC\tEAX\n");
    }
    else {
        DECODE_PRINTF("INC\tAX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = inc_long(M.x86.R_EAX);
    }
    else {
        M.x86.R_AX = inc_word(M.x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x41
****************************************************************************/
static void
x86emuOp_inc_CX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INC\tECX\n");
    }
    else {
        DECODE_PRINTF("INC\tCX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ECX = inc_long(M.x86.R_ECX);
    }
    else {
        M.x86.R_CX = inc_word(M.x86.R_CX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x42
****************************************************************************/
static void
x86emuOp_inc_DX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INC\tEDX\n");
    }
    else {
        DECODE_PRINTF("INC\tDX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDX = inc_long(M.x86.R_EDX);
    }
    else {
        M.x86.R_DX = inc_word(M.x86.R_DX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x43
****************************************************************************/
static void
x86emuOp_inc_BX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INC\tEBX\n");
    }
    else {
        DECODE_PRINTF("INC\tBX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBX = inc_long(M.x86.R_EBX);
    }
    else {
        M.x86.R_BX = inc_word(M.x86.R_BX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x44
****************************************************************************/
static void
x86emuOp_inc_SP(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INC\tESP\n");
    }
    else {
        DECODE_PRINTF("INC\tSP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ESP = inc_long(M.x86.R_ESP);
    }
    else {
        M.x86.R_SP = inc_word(M.x86.R_SP);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x45
****************************************************************************/
static void
x86emuOp_inc_BP(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INC\tEBP\n");
    }
    else {
        DECODE_PRINTF("INC\tBP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBP = inc_long(M.x86.R_EBP);
    }
    else {
        M.x86.R_BP = inc_word(M.x86.R_BP);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x46
****************************************************************************/
static void
x86emuOp_inc_SI(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INC\tESI\n");
    }
    else {
        DECODE_PRINTF("INC\tSI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ESI = inc_long(M.x86.R_ESI);
    }
    else {
        M.x86.R_SI = inc_word(M.x86.R_SI);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x47
****************************************************************************/
static void
x86emuOp_inc_DI(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INC\tEDI\n");
    }
    else {
        DECODE_PRINTF("INC\tDI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDI = inc_long(M.x86.R_EDI);
    }
    else {
        M.x86.R_DI = inc_word(M.x86.R_DI);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x48
****************************************************************************/
static void
x86emuOp_dec_AX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("DEC\tEAX\n");
    }
    else {
        DECODE_PRINTF("DEC\tAX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = dec_long(M.x86.R_EAX);
    }
    else {
        M.x86.R_AX = dec_word(M.x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x49
****************************************************************************/
static void
x86emuOp_dec_CX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("DEC\tECX\n");
    }
    else {
        DECODE_PRINTF("DEC\tCX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ECX = dec_long(M.x86.R_ECX);
    }
    else {
        M.x86.R_CX = dec_word(M.x86.R_CX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x4a
****************************************************************************/
static void
x86emuOp_dec_DX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("DEC\tEDX\n");
    }
    else {
        DECODE_PRINTF("DEC\tDX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDX = dec_long(M.x86.R_EDX);
    }
    else {
        M.x86.R_DX = dec_word(M.x86.R_DX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x4b
****************************************************************************/
static void
x86emuOp_dec_BX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("DEC\tEBX\n");
    }
    else {
        DECODE_PRINTF("DEC\tBX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBX = dec_long(M.x86.R_EBX);
    }
    else {
        M.x86.R_BX = dec_word(M.x86.R_BX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x4c
****************************************************************************/
static void
x86emuOp_dec_SP(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("DEC\tESP\n");
    }
    else {
        DECODE_PRINTF("DEC\tSP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ESP = dec_long(M.x86.R_ESP);
    }
    else {
        M.x86.R_SP = dec_word(M.x86.R_SP);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x4d
****************************************************************************/
static void
x86emuOp_dec_BP(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("DEC\tEBP\n");
    }
    else {
        DECODE_PRINTF("DEC\tBP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBP = dec_long(M.x86.R_EBP);
    }
    else {
        M.x86.R_BP = dec_word(M.x86.R_BP);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x4e
****************************************************************************/
static void
x86emuOp_dec_SI(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("DEC\tESI\n");
    }
    else {
        DECODE_PRINTF("DEC\tSI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ESI = dec_long(M.x86.R_ESI);
    }
    else {
        M.x86.R_SI = dec_word(M.x86.R_SI);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x4f
****************************************************************************/
static void
x86emuOp_dec_DI(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("DEC\tEDI\n");
    }
    else {
        DECODE_PRINTF("DEC\tDI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDI = dec_long(M.x86.R_EDI);
    }
    else {
        M.x86.R_DI = dec_word(M.x86.R_DI);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x50
****************************************************************************/
static void
x86emuOp_push_AX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSH\tEAX\n");
    }
    else {
        DECODE_PRINTF("PUSH\tAX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_EAX);
    }
    else {
        push_word(M.x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x51
****************************************************************************/
static void
x86emuOp_push_CX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSH\tECX\n");
    }
    else {
        DECODE_PRINTF("PUSH\tCX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_ECX);
    }
    else {
        push_word(M.x86.R_CX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x52
****************************************************************************/
static void
x86emuOp_push_DX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSH\tEDX\n");
    }
    else {
        DECODE_PRINTF("PUSH\tDX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_EDX);
    }
    else {
        push_word(M.x86.R_DX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x53
****************************************************************************/
static void
x86emuOp_push_BX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSH\tEBX\n");
    }
    else {
        DECODE_PRINTF("PUSH\tBX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_EBX);
    }
    else {
        push_word(M.x86.R_BX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x54
****************************************************************************/
static void
x86emuOp_push_SP(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSH\tESP\n");
    }
    else {
        DECODE_PRINTF("PUSH\tSP\n");
    }
    TRACE_AND_STEP();
    /* Always push (E)SP, since we are emulating an i386 and above
     * processor. This is necessary as some BIOS'es use this to check
     * what type of processor is in the system.
     */
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_ESP);
    }
    else {
        push_word((u16) (M.x86.R_SP));
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x55
****************************************************************************/
static void
x86emuOp_push_BP(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSH\tEBP\n");
    }
    else {
        DECODE_PRINTF("PUSH\tBP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_EBP);
    }
    else {
        push_word(M.x86.R_BP);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x56
****************************************************************************/
static void
x86emuOp_push_SI(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSH\tESI\n");
    }
    else {
        DECODE_PRINTF("PUSH\tSI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_ESI);
    }
    else {
        push_word(M.x86.R_SI);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x57
****************************************************************************/
static void
x86emuOp_push_DI(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSH\tEDI\n");
    }
    else {
        DECODE_PRINTF("PUSH\tDI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_EDI);
    }
    else {
        push_word(M.x86.R_DI);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x58
****************************************************************************/
static void
x86emuOp_pop_AX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POP\tEAX\n");
    }
    else {
        DECODE_PRINTF("POP\tAX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = pop_long();
    }
    else {
        M.x86.R_AX = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x59
****************************************************************************/
static void
x86emuOp_pop_CX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POP\tECX\n");
    }
    else {
        DECODE_PRINTF("POP\tCX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ECX = pop_long();
    }
    else {
        M.x86.R_CX = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x5a
****************************************************************************/
static void
x86emuOp_pop_DX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POP\tEDX\n");
    }
    else {
        DECODE_PRINTF("POP\tDX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDX = pop_long();
    }
    else {
        M.x86.R_DX = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x5b
****************************************************************************/
static void
x86emuOp_pop_BX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POP\tEBX\n");
    }
    else {
        DECODE_PRINTF("POP\tBX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBX = pop_long();
    }
    else {
        M.x86.R_BX = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x5c
****************************************************************************/
static void
x86emuOp_pop_SP(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POP\tESP\n");
    }
    else {
        DECODE_PRINTF("POP\tSP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ESP = pop_long();
    }
    else {
        M.x86.R_SP = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x5d
****************************************************************************/
static void
x86emuOp_pop_BP(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POP\tEBP\n");
    }
    else {
        DECODE_PRINTF("POP\tBP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBP = pop_long();
    }
    else {
        M.x86.R_BP = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x5e
****************************************************************************/
static void
x86emuOp_pop_SI(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POP\tESI\n");
    }
    else {
        DECODE_PRINTF("POP\tSI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ESI = pop_long();
    }
    else {
        M.x86.R_SI = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x5f
****************************************************************************/
static void
x86emuOp_pop_DI(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POP\tEDI\n");
    }
    else {
        DECODE_PRINTF("POP\tDI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDI = pop_long();
    }
    else {
        M.x86.R_DI = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x60
****************************************************************************/
static void
x86emuOp_push_all(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSHAD\n");
    }
    else {
        DECODE_PRINTF("PUSHA\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        u32 old_sp = M.x86.R_ESP;

        push_long(M.x86.R_EAX);
        push_long(M.x86.R_ECX);
        push_long(M.x86.R_EDX);
        push_long(M.x86.R_EBX);
        push_long(old_sp);
        push_long(M.x86.R_EBP);
        push_long(M.x86.R_ESI);
        push_long(M.x86.R_EDI);
    }
    else {
        u16 old_sp = M.x86.R_SP;

        push_word(M.x86.R_AX);
        push_word(M.x86.R_CX);
        push_word(M.x86.R_DX);
        push_word(M.x86.R_BX);
        push_word(old_sp);
        push_word(M.x86.R_BP);
        push_word(M.x86.R_SI);
        push_word(M.x86.R_DI);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x61
****************************************************************************/
static void
x86emuOp_pop_all(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POPAD\n");
    }
    else {
        DECODE_PRINTF("POPA\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDI = pop_long();
        M.x86.R_ESI = pop_long();
        M.x86.R_EBP = pop_long();
        M.x86.R_ESP += 4;       /* skip ESP */
        M.x86.R_EBX = pop_long();
        M.x86.R_EDX = pop_long();
        M.x86.R_ECX = pop_long();
        M.x86.R_EAX = pop_long();
    }
    else {
        M.x86.R_DI = pop_word();
        M.x86.R_SI = pop_word();
        M.x86.R_BP = pop_word();
        M.x86.R_SP += 2;        /* skip SP */
        M.x86.R_BX = pop_word();
        M.x86.R_DX = pop_word();
        M.x86.R_CX = pop_word();
        M.x86.R_AX = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/*opcode 0x62   ILLEGAL OP, calls x86emuOp_illegal_op() */
/*opcode 0x63   ILLEGAL OP, calls x86emuOp_illegal_op() */

/****************************************************************************
REMARKS:
Handles opcode 0x64
****************************************************************************/
static void
x86emuOp_segovr_FS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("FS:\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_SEGOVR_FS;
    /*
     * note the lack of DECODE_CLEAR_SEGOVR(r) since, here is one of 4
     * opcode subroutines we do not want to do this.
     */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x65
****************************************************************************/
static void
x86emuOp_segovr_GS(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("GS:\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_SEGOVR_GS;
    /*
     * note the lack of DECODE_CLEAR_SEGOVR(r) since, here is one of 4
     * opcode subroutines we do not want to do this.
     */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x66 - prefix for 32-bit register
****************************************************************************/
static void
x86emuOp_prefix_data(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("DATA:\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_PREFIX_DATA;
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x67 - prefix for 32-bit address
****************************************************************************/
static void
x86emuOp_prefix_addr(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("ADDR:\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_PREFIX_ADDR;
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x68
****************************************************************************/
static void
x86emuOp_push_word_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 imm;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        imm = fetch_long_imm();
    }
    else {
        imm = fetch_word_imm();
    }
    DECODE_PRINTF2("PUSH\t%x\n", imm);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(imm);
    }
    else {
        push_word((u16) imm);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x69
****************************************************************************/
static void
x86emuOp_imul_word_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("IMUL\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;
            u32 res_lo, res_hi;
            s32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo, &res_hi, (s32) srcval, (s32) imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32) res_lo;
        }
        else {
            u16 *destreg;
            u16 srcval;
            u32 res;
            s16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            res = (s16) srcval *(s16) imm;

            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16) res;
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;
            u32 res_lo, res_hi;
            s32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo, &res_hi, (s32) srcval, (s32) imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32) res_lo;
        }
        else {
            u16 *destreg;
            u16 srcval;
            u32 res;
            s16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            res = (s16) srcval *(s16) imm;

            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16) res;
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;
            u32 res_lo, res_hi;
            s32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo, &res_hi, (s32) srcval, (s32) imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32) res_lo;
        }
        else {
            u16 *destreg;
            u16 srcval;
            u32 res;
            s16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            res = (s16) srcval *(s16) imm;

            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16) res;
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;
            u32 res_lo, res_hi;
            s32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo, &res_hi, (s32) * srcreg, (s32) imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32) res_lo;
        }
        else {
            u16 *destreg, *srcreg;
            u32 res;
            s16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            res = (s16) * srcreg * (s16) imm;
            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16) res;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6a
****************************************************************************/
static void
x86emuOp_push_byte_IMM(u8 X86EMU_UNUSED(op1))
{
    s16 imm;

    START_OF_INSTR();
    imm = (s8) fetch_byte_imm();
    DECODE_PRINTF2("PUSH\t%d\n", imm);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long((s32) imm);
    }
    else {
        push_word(imm);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6b
****************************************************************************/
static void
x86emuOp_imul_byte_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;
    s8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("IMUL\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;
            u32 res_lo, res_hi;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo, &res_hi, (s32) srcval, (s32) imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32) res_lo;
        }
        else {
            u16 *destreg;
            u16 srcval;
            u32 res;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            res = (s16) srcval *(s16) imm;

            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16) res;
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;
            u32 res_lo, res_hi;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo, &res_hi, (s32) srcval, (s32) imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32) res_lo;
        }
        else {
            u16 *destreg;
            u16 srcval;
            u32 res;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            res = (s16) srcval *(s16) imm;

            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16) res;
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;
            u32 res_lo, res_hi;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo, &res_hi, (s32) srcval, (s32) imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32) res_lo;
        }
        else {
            u16 *destreg;
            u16 srcval;
            u32 res;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            res = (s16) srcval *(s16) imm;

            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16) res;
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;
            u32 res_lo, res_hi;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo, &res_hi, (s32) * srcreg, (s32) imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32) res_lo;
        }
        else {
            u16 *destreg, *srcreg;
            u32 res;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32) imm);
            res = (s16) * srcreg * (s16) imm;
            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            }
            else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16) res;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6c
****************************************************************************/
static void
x86emuOp_ins_byte(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("INSB\n");
    ins(1);
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6d
****************************************************************************/
static void
x86emuOp_ins_word(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INSD\n");
        ins(4);
    }
    else {
        DECODE_PRINTF("INSW\n");
        ins(2);
    }
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6e
****************************************************************************/
static void
x86emuOp_outs_byte(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("OUTSB\n");
    outs(1);
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6f
****************************************************************************/
static void
x86emuOp_outs_word(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("OUTSD\n");
        outs(4);
    }
    else {
        DECODE_PRINTF("OUTSW\n");
        outs(2);
    }
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x70
****************************************************************************/
static void
x86emuOp_jump_near_O(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if overflow flag is set */
    START_OF_INSTR();
    DECODE_PRINTF("JO\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_OF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x71
****************************************************************************/
static void
x86emuOp_jump_near_NO(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if overflow is not set */
    START_OF_INSTR();
    DECODE_PRINTF("JNO\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (!ACCESS_FLAG(F_OF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x72
****************************************************************************/
static void
x86emuOp_jump_near_B(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if carry flag is set. */
    START_OF_INSTR();
    DECODE_PRINTF("JB\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_CF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x73
****************************************************************************/
static void
x86emuOp_jump_near_NB(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if carry flag is clear. */
    START_OF_INSTR();
    DECODE_PRINTF("JNB\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (!ACCESS_FLAG(F_CF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x74
****************************************************************************/
static void
x86emuOp_jump_near_Z(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if zero flag is set. */
    START_OF_INSTR();
    DECODE_PRINTF("JZ\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_ZF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x75
****************************************************************************/
static void
x86emuOp_jump_near_NZ(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if zero flag is clear. */
    START_OF_INSTR();
    DECODE_PRINTF("JNZ\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (!ACCESS_FLAG(F_ZF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x76
****************************************************************************/
static void
x86emuOp_jump_near_BE(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if carry flag is set or if the zero
       flag is set. */
    START_OF_INSTR();
    DECODE_PRINTF("JBE\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_CF) || ACCESS_FLAG(F_ZF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x77
****************************************************************************/
static void
x86emuOp_jump_near_NBE(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if carry flag is clear and if the zero
       flag is clear */
    START_OF_INSTR();
    DECODE_PRINTF("JNBE\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (!(ACCESS_FLAG(F_CF) || ACCESS_FLAG(F_ZF)))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x78
****************************************************************************/
static void
x86emuOp_jump_near_S(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if sign flag is set */
    START_OF_INSTR();
    DECODE_PRINTF("JS\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_SF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x79
****************************************************************************/
static void
x86emuOp_jump_near_NS(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if sign flag is clear */
    START_OF_INSTR();
    DECODE_PRINTF("JNS\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (!ACCESS_FLAG(F_SF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x7a
****************************************************************************/
static void
x86emuOp_jump_near_P(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if parity flag is set (even parity) */
    START_OF_INSTR();
    DECODE_PRINTF("JP\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_PF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x7b
****************************************************************************/
static void
x86emuOp_jump_near_NP(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;

    /* jump to byte offset if parity flag is clear (odd parity) */
    START_OF_INSTR();
    DECODE_PRINTF("JNP\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (!ACCESS_FLAG(F_PF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x7c
****************************************************************************/
static void
x86emuOp_jump_near_L(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;
    int sf, of;

    /* jump to byte offset if sign flag not equal to overflow flag. */
    START_OF_INSTR();
    DECODE_PRINTF("JL\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    sf = ACCESS_FLAG(F_SF) != 0;
    of = ACCESS_FLAG(F_OF) != 0;
    if (sf ^ of)
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x7d
****************************************************************************/
static void
x86emuOp_jump_near_NL(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;
    int sf, of;

    /* jump to byte offset if sign flag not equal to overflow flag. */
    START_OF_INSTR();
    DECODE_PRINTF("JNL\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    sf = ACCESS_FLAG(F_SF) != 0;
    of = ACCESS_FLAG(F_OF) != 0;
    /* note: inverse of above, but using == instead of xor. */
    if (sf == of)
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x7e
****************************************************************************/
static void
x86emuOp_jump_near_LE(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;
    int sf, of;

    /* jump to byte offset if sign flag not equal to overflow flag
       or the zero flag is set */
    START_OF_INSTR();
    DECODE_PRINTF("JLE\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    sf = ACCESS_FLAG(F_SF) != 0;
    of = ACCESS_FLAG(F_OF) != 0;
    if ((sf ^ of) || ACCESS_FLAG(F_ZF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x7f
****************************************************************************/
static void
x86emuOp_jump_near_NLE(u8 X86EMU_UNUSED(op1))
{
    s8 offset;
    u16 target;
    int sf, of;

    /* jump to byte offset if sign flag equal to overflow flag.
       and the zero flag is clear */
    START_OF_INSTR();
    DECODE_PRINTF("JNLE\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + (s16) offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    sf = ACCESS_FLAG(F_SF) != 0;
    of = ACCESS_FLAG(F_OF) != 0;
    if ((sf == of) && !ACCESS_FLAG(F_ZF))
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

static u8(*opc80_byte_operation[]) (u8 d, u8 s) = {
    add_byte,                   /* 00 */
        or_byte,                /* 01 */
        adc_byte,               /* 02 */
        sbb_byte,               /* 03 */
        and_byte,               /* 04 */
        sub_byte,               /* 05 */
        xor_byte,               /* 06 */
        cmp_byte,               /* 07 */
};

/****************************************************************************
REMARKS:
Handles opcode 0x80
****************************************************************************/
static void
x86emuOp_opc80_byte_RM_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg;
    uint destoffset;
    u8 imm;
    u8 destval;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */

        switch (rh) {
        case 0:
            DECODE_PRINTF("ADD\t");
            break;
        case 1:
            DECODE_PRINTF("OR\t");
            break;
        case 2:
            DECODE_PRINTF("ADC\t");
            break;
        case 3:
            DECODE_PRINTF("SBB\t");
            break;
        case 4:
            DECODE_PRINTF("AND\t");
            break;
        case 5:
            DECODE_PRINTF("SUB\t");
            break;
        case 6:
            DECODE_PRINTF("XOR\t");
            break;
        case 7:
            DECODE_PRINTF("CMP\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    switch (mod) {
    case 0:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        imm = fetch_byte_imm();
        DECODE_PRINTF2("%x\n", imm);
        TRACE_AND_STEP();
        destval = (*opc80_byte_operation[rh]) (destval, imm);
        if (rh != 7)
            store_data_byte(destoffset, destval);
        break;
    case 1:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        imm = fetch_byte_imm();
        DECODE_PRINTF2("%x\n", imm);
        TRACE_AND_STEP();
        destval = (*opc80_byte_operation[rh]) (destval, imm);
        if (rh != 7)
            store_data_byte(destoffset, destval);
        break;
    case 2:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        imm = fetch_byte_imm();
        DECODE_PRINTF2("%x\n", imm);
        TRACE_AND_STEP();
        destval = (*opc80_byte_operation[rh]) (destval, imm);
        if (rh != 7)
            store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        imm = fetch_byte_imm();
        DECODE_PRINTF2("%x\n", imm);
        TRACE_AND_STEP();
        destval = (*opc80_byte_operation[rh]) (*destreg, imm);
        if (rh != 7)
            *destreg = destval;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

static u16(*opc81_word_operation[]) (u16 d, u16 s) = {
    add_word,                   /*00 */
        or_word,                /*01 */
        adc_word,               /*02 */
        sbb_word,               /*03 */
        and_word,               /*04 */
        sub_word,               /*05 */
        xor_word,               /*06 */
        cmp_word,               /*07 */
};

static u32(*opc81_long_operation[]) (u32 d, u32 s) = {
    add_long,                   /*00 */
        or_long,                /*01 */
        adc_long,               /*02 */
        sbb_long,               /*03 */
        and_long,               /*04 */
        sub_long,               /*05 */
        xor_long,               /*06 */
        cmp_long,               /*07 */
};

/****************************************************************************
REMARKS:
Handles opcode 0x81
****************************************************************************/
static void
x86emuOp_opc81_word_RM_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */

        switch (rh) {
        case 0:
            DECODE_PRINTF("ADD\t");
            break;
        case 1:
            DECODE_PRINTF("OR\t");
            break;
        case 2:
            DECODE_PRINTF("ADC\t");
            break;
        case 3:
            DECODE_PRINTF("SBB\t");
            break;
        case 4:
            DECODE_PRINTF("AND\t");
            break;
        case 5:
            DECODE_PRINTF("SUB\t");
            break;
        case 6:
            DECODE_PRINTF("XOR\t");
            break;
        case 7:
            DECODE_PRINTF("CMP\t");
            break;
        }
    }
#endif
    /*
     * Know operation, decode the mod byte to find the addressing
     * mode.
     */
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval, imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            imm = fetch_long_imm();
            DECODE_PRINTF2("%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc81_long_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_long(destoffset, destval);
        }
        else {
            u16 destval, imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            imm = fetch_word_imm();
            DECODE_PRINTF2("%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc81_word_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval, imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            imm = fetch_long_imm();
            DECODE_PRINTF2("%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc81_long_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_long(destoffset, destval);
        }
        else {
            u16 destval, imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            imm = fetch_word_imm();
            DECODE_PRINTF2("%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc81_word_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval, imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            imm = fetch_long_imm();
            DECODE_PRINTF2("%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc81_long_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_long(destoffset, destval);
        }
        else {
            u16 destval, imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            imm = fetch_word_imm();
            DECODE_PRINTF2("%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc81_word_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 destval, imm;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            imm = fetch_long_imm();
            DECODE_PRINTF2("%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc81_long_operation[rh]) (*destreg, imm);
            if (rh != 7)
                *destreg = destval;
        }
        else {
            u16 *destreg;
            u16 destval, imm;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            imm = fetch_word_imm();
            DECODE_PRINTF2("%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc81_word_operation[rh]) (*destreg, imm);
            if (rh != 7)
                *destreg = destval;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

static u8(*opc82_byte_operation[]) (u8 s, u8 d) = {
    add_byte,                   /*00 */
        or_byte,                /*01 *//*YYY UNUSED ???? */
        adc_byte,               /*02 */
        sbb_byte,               /*03 */
        and_byte,               /*04 *//*YYY UNUSED ???? */
        sub_byte,               /*05 */
        xor_byte,               /*06 *//*YYY UNUSED ???? */
        cmp_byte,               /*07 */
};

/****************************************************************************
REMARKS:
Handles opcode 0x82
****************************************************************************/
static void
x86emuOp_opc82_byte_RM_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg;
    uint destoffset;
    u8 imm;
    u8 destval;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction Similar to opcode 81, except that
     * the immediate byte is sign extended to a word length.
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */
        switch (rh) {
        case 0:
            DECODE_PRINTF("ADD\t");
            break;
        case 1:
            DECODE_PRINTF("OR\t");
            break;
        case 2:
            DECODE_PRINTF("ADC\t");
            break;
        case 3:
            DECODE_PRINTF("SBB\t");
            break;
        case 4:
            DECODE_PRINTF("AND\t");
            break;
        case 5:
            DECODE_PRINTF("SUB\t");
            break;
        case 6:
            DECODE_PRINTF("XOR\t");
            break;
        case 7:
            DECODE_PRINTF("CMP\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    switch (mod) {
    case 0:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm00_address(rl);
        destval = fetch_data_byte(destoffset);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", imm);
        TRACE_AND_STEP();
        destval = (*opc82_byte_operation[rh]) (destval, imm);
        if (rh != 7)
            store_data_byte(destoffset, destval);
        break;
    case 1:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm01_address(rl);
        destval = fetch_data_byte(destoffset);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", imm);
        TRACE_AND_STEP();
        destval = (*opc82_byte_operation[rh]) (destval, imm);
        if (rh != 7)
            store_data_byte(destoffset, destval);
        break;
    case 2:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm10_address(rl);
        destval = fetch_data_byte(destoffset);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", imm);
        TRACE_AND_STEP();
        destval = (*opc82_byte_operation[rh]) (destval, imm);
        if (rh != 7)
            store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", imm);
        TRACE_AND_STEP();
        destval = (*opc82_byte_operation[rh]) (*destreg, imm);
        if (rh != 7)
            *destreg = destval;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

static u16(*opc83_word_operation[]) (u16 s, u16 d) = {
    add_word,                   /*00 */
        or_word,                /*01 *//*YYY UNUSED ???? */
        adc_word,               /*02 */
        sbb_word,               /*03 */
        and_word,               /*04 *//*YYY UNUSED ???? */
        sub_word,               /*05 */
        xor_word,               /*06 *//*YYY UNUSED ???? */
        cmp_word,               /*07 */
};

static u32(*opc83_long_operation[]) (u32 s, u32 d) = {
    add_long,                   /*00 */
        or_long,                /*01 *//*YYY UNUSED ???? */
        adc_long,               /*02 */
        sbb_long,               /*03 */
        and_long,               /*04 *//*YYY UNUSED ???? */
        sub_long,               /*05 */
        xor_long,               /*06 *//*YYY UNUSED ???? */
        cmp_long,               /*07 */
};

/****************************************************************************
REMARKS:
Handles opcode 0x83
****************************************************************************/
static void
x86emuOp_opc83_word_RM_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction Similar to opcode 81, except that
     * the immediate byte is sign extended to a word length.
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */
        switch (rh) {
        case 0:
            DECODE_PRINTF("ADD\t");
            break;
        case 1:
            DECODE_PRINTF("OR\t");
            break;
        case 2:
            DECODE_PRINTF("ADC\t");
            break;
        case 3:
            DECODE_PRINTF("SBB\t");
            break;
        case 4:
            DECODE_PRINTF("AND\t");
            break;
        case 5:
            DECODE_PRINTF("SUB\t");
            break;
        case 6:
            DECODE_PRINTF("XOR\t");
            break;
        case 7:
            DECODE_PRINTF("CMP\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval, imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm00_address(rl);
            destval = fetch_data_long(destoffset);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc83_long_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_long(destoffset, destval);
        }
        else {
            u16 destval, imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm00_address(rl);
            destval = fetch_data_word(destoffset);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc83_word_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval, imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm01_address(rl);
            destval = fetch_data_long(destoffset);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc83_long_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_long(destoffset, destval);
        }
        else {
            u16 destval, imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm01_address(rl);
            destval = fetch_data_word(destoffset);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc83_word_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval, imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm10_address(rl);
            destval = fetch_data_long(destoffset);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc83_long_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_long(destoffset, destval);
        }
        else {
            u16 destval, imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm10_address(rl);
            destval = fetch_data_word(destoffset);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc83_word_operation[rh]) (destval, imm);
            if (rh != 7)
                store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 destval, imm;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc83_long_operation[rh]) (*destreg, imm);
            if (rh != 7)
                *destreg = destval;
        }
        else {
            u16 *destreg;
            u16 destval, imm;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            destval = (*opc83_word_operation[rh]) (*destreg, imm);
            if (rh != 7)
                *destreg = destval;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x84
****************************************************************************/
static void
x86emuOp_test_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("TEST\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        test_byte(destval, *srcreg);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        test_byte(destval, *srcreg);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        test_byte(destval, *srcreg);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        test_byte(*destreg, *srcreg);
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x85
****************************************************************************/
static void
x86emuOp_test_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("TEST\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_long(destval, *srcreg);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_word(destval, *srcreg);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_long(destval, *srcreg);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_word(destval, *srcreg);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_long(destval, *srcreg);
        }
        else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_word(destval, *srcreg);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_long(*destreg, *srcreg);
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_word(*destreg, *srcreg);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x86
****************************************************************************/
static void
x86emuOp_xchg_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;
    u8 destval;
    u8 tmp;

    START_OF_INSTR();
    DECODE_PRINTF("XCHG\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        tmp = *srcreg;
        *srcreg = destval;
        destval = tmp;
        store_data_byte(destoffset, destval);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        tmp = *srcreg;
        *srcreg = destval;
        destval = tmp;
        store_data_byte(destoffset, destval);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        tmp = *srcreg;
        *srcreg = destval;
        destval = tmp;
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        tmp = *srcreg;
        *srcreg = *destreg;
        *destreg = tmp;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x87
****************************************************************************/
static void
x86emuOp_xchg_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("XCHG\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *srcreg;
            u32 destval, tmp;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = destval;
            destval = tmp;
            store_data_long(destoffset, destval);
        }
        else {
            u16 *srcreg;
            u16 destval, tmp;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = destval;
            destval = tmp;
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *srcreg;
            u32 destval, tmp;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = destval;
            destval = tmp;
            store_data_long(destoffset, destval);
        }
        else {
            u16 *srcreg;
            u16 destval, tmp;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = destval;
            destval = tmp;
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *srcreg;
            u32 destval, tmp;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = destval;
            destval = tmp;
            store_data_long(destoffset, destval);
        }
        else {
            u16 *srcreg;
            u16 destval, tmp;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = destval;
            destval = tmp;
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;
            u32 tmp;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = *destreg;
            *destreg = tmp;
        }
        else {
            u16 *destreg, *srcreg;
            u16 tmp;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = *destreg;
            *destreg = tmp;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x88
****************************************************************************/
static void
x86emuOp_mov_byte_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        store_data_byte(destoffset, *srcreg);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        store_data_byte(destoffset, *srcreg);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        store_data_byte(destoffset, *srcreg);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = *srcreg;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x89
****************************************************************************/
static void
x86emuOp_mov_word_RM_R(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u32 destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            store_data_long(destoffset, *srcreg);
        }
        else {
            u16 *srcreg;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            store_data_word(destoffset, *srcreg);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            store_data_long(destoffset, *srcreg);
        }
        else {
            u16 *srcreg;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            store_data_word(destoffset, *srcreg);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            store_data_long(destoffset, *srcreg);
        }
        else {
            u16 *srcreg;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            store_data_word(destoffset, *srcreg);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = *srcreg;
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = *srcreg;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8a
****************************************************************************/
static void
x86emuOp_mov_byte_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    uint srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = srcval;
        break;
    case 1:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = srcval;
        break;
    case 2:
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = srcval;
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = *srcreg;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8b
****************************************************************************/
static void
x86emuOp_mov_word_R_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = srcval;
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm00_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = srcval;
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = srcval;
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm01_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = srcval;
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = srcval;
        }
        else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm10_address(rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = srcval;
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = *srcreg;
        }
        else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = *srcreg;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8c
****************************************************************************/
static void
x86emuOp_mov_word_RM_SR(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u16 *destreg, *srcreg;
    uint destoffset;
    u16 destval;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",");
        srcreg = decode_rm_seg_register(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = *srcreg;
        store_data_word(destoffset, destval);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",");
        srcreg = decode_rm_seg_register(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = *srcreg;
        store_data_word(destoffset, destval);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",");
        srcreg = decode_rm_seg_register(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = *srcreg;
        store_data_word(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_WORD_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = decode_rm_seg_register(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = *srcreg;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8d
****************************************************************************/
static void
x86emuOp_lea_word_R_M(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("LEA\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
            u32 *srcreg = DECODE_RM_LONG_REGISTER(rh);

            DECODE_PRINTF(",");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *srcreg = (u32) destoffset;
        }
        else {
            u16 *srcreg = DECODE_RM_WORD_REGISTER(rh);

            DECODE_PRINTF(",");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *srcreg = (u16) destoffset;
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
            u32 *srcreg = DECODE_RM_LONG_REGISTER(rh);

            DECODE_PRINTF(",");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *srcreg = (u32) destoffset;
        }
        else {
            u16 *srcreg = DECODE_RM_WORD_REGISTER(rh);

            DECODE_PRINTF(",");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *srcreg = (u16) destoffset;
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
            u32 *srcreg = DECODE_RM_LONG_REGISTER(rh);

            DECODE_PRINTF(",");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *srcreg = (u32) destoffset;
        }
        else {
            u16 *srcreg = DECODE_RM_WORD_REGISTER(rh);

            DECODE_PRINTF(",");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *srcreg = (u16) destoffset;
        }
        break;
    case 3:                    /* register to register */
        /* undefined.  Do nothing. */
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8e
****************************************************************************/
static void
x86emuOp_mov_word_SR_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u16 *destreg, *srcreg;
    uint srcoffset;
    u16 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        destreg = decode_rm_seg_register(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        srcval = fetch_data_word(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = srcval;
        break;
    case 1:
        destreg = decode_rm_seg_register(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        srcval = fetch_data_word(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = srcval;
        break;
    case 2:
        destreg = decode_rm_seg_register(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        srcval = fetch_data_word(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = srcval;
        break;
    case 3:                    /* register to register */
        destreg = decode_rm_seg_register(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_WORD_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = *srcreg;
        break;
    }
    /*
     * Clean up, and reset all the R_xSP pointers to the correct
     * locations.  This is about 3x too much overhead (doing all the
     * segreg ptrs when only one is needed, but this instruction
     * *cannot* be that common, and this isn't too much work anyway.
     */
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8f
****************************************************************************/
static void
x86emuOp_pop_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("POP\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (rh != 0) {
        DECODE_PRINTF("ILLEGAL DECODE OF OPCODE 8F\n");
        HALT_SYS();
    }
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = pop_long();
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = pop_word();
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = pop_long();
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = pop_word();
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = pop_long();
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = pop_word();
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = pop_long();
        }
        else {
            u16 *destreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = pop_word();
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x90
****************************************************************************/
static void
x86emuOp_nop(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("NOP\n");
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x91
****************************************************************************/
static void
x86emuOp_xchg_word_AX_CX(u8 X86EMU_UNUSED(op1))
{
    u32 tmp;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("XCHG\tEAX,ECX\n");
    }
    else {
        DECODE_PRINTF("XCHG\tAX,CX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        tmp = M.x86.R_EAX;
        M.x86.R_EAX = M.x86.R_ECX;
        M.x86.R_ECX = tmp;
    }
    else {
        tmp = M.x86.R_AX;
        M.x86.R_AX = M.x86.R_CX;
        M.x86.R_CX = (u16) tmp;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x92
****************************************************************************/
static void
x86emuOp_xchg_word_AX_DX(u8 X86EMU_UNUSED(op1))
{
    u32 tmp;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("XCHG\tEAX,EDX\n");
    }
    else {
        DECODE_PRINTF("XCHG\tAX,DX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        tmp = M.x86.R_EAX;
        M.x86.R_EAX = M.x86.R_EDX;
        M.x86.R_EDX = tmp;
    }
    else {
        tmp = M.x86.R_AX;
        M.x86.R_AX = M.x86.R_DX;
        M.x86.R_DX = (u16) tmp;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x93
****************************************************************************/
static void
x86emuOp_xchg_word_AX_BX(u8 X86EMU_UNUSED(op1))
{
    u32 tmp;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("XCHG\tEAX,EBX\n");
    }
    else {
        DECODE_PRINTF("XCHG\tAX,BX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        tmp = M.x86.R_EAX;
        M.x86.R_EAX = M.x86.R_EBX;
        M.x86.R_EBX = tmp;
    }
    else {
        tmp = M.x86.R_AX;
        M.x86.R_AX = M.x86.R_BX;
        M.x86.R_BX = (u16) tmp;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x94
****************************************************************************/
static void
x86emuOp_xchg_word_AX_SP(u8 X86EMU_UNUSED(op1))
{
    u32 tmp;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("XCHG\tEAX,ESP\n");
    }
    else {
        DECODE_PRINTF("XCHG\tAX,SP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        tmp = M.x86.R_EAX;
        M.x86.R_EAX = M.x86.R_ESP;
        M.x86.R_ESP = tmp;
    }
    else {
        tmp = M.x86.R_AX;
        M.x86.R_AX = M.x86.R_SP;
        M.x86.R_SP = (u16) tmp;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x95
****************************************************************************/
static void
x86emuOp_xchg_word_AX_BP(u8 X86EMU_UNUSED(op1))
{
    u32 tmp;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("XCHG\tEAX,EBP\n");
    }
    else {
        DECODE_PRINTF("XCHG\tAX,BP\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        tmp = M.x86.R_EAX;
        M.x86.R_EAX = M.x86.R_EBP;
        M.x86.R_EBP = tmp;
    }
    else {
        tmp = M.x86.R_AX;
        M.x86.R_AX = M.x86.R_BP;
        M.x86.R_BP = (u16) tmp;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x96
****************************************************************************/
static void
x86emuOp_xchg_word_AX_SI(u8 X86EMU_UNUSED(op1))
{
    u32 tmp;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("XCHG\tEAX,ESI\n");
    }
    else {
        DECODE_PRINTF("XCHG\tAX,SI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        tmp = M.x86.R_EAX;
        M.x86.R_EAX = M.x86.R_ESI;
        M.x86.R_ESI = tmp;
    }
    else {
        tmp = M.x86.R_AX;
        M.x86.R_AX = M.x86.R_SI;
        M.x86.R_SI = (u16) tmp;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x97
****************************************************************************/
static void
x86emuOp_xchg_word_AX_DI(u8 X86EMU_UNUSED(op1))
{
    u32 tmp;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("XCHG\tEAX,EDI\n");
    }
    else {
        DECODE_PRINTF("XCHG\tAX,DI\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        tmp = M.x86.R_EAX;
        M.x86.R_EAX = M.x86.R_EDI;
        M.x86.R_EDI = tmp;
    }
    else {
        tmp = M.x86.R_AX;
        M.x86.R_AX = M.x86.R_DI;
        M.x86.R_DI = (u16) tmp;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x98
****************************************************************************/
static void
x86emuOp_cbw(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("CWDE\n");
    }
    else {
        DECODE_PRINTF("CBW\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        if (M.x86.R_AX & 0x8000) {
            M.x86.R_EAX |= 0xffff0000;
        }
        else {
            M.x86.R_EAX &= 0x0000ffff;
        }
    }
    else {
        if (M.x86.R_AL & 0x80) {
            M.x86.R_AH = 0xff;
        }
        else {
            M.x86.R_AH = 0x0;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x99
****************************************************************************/
static void
x86emuOp_cwd(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("CDQ\n");
    }
    else {
        DECODE_PRINTF("CWD\n");
    }
    DECODE_PRINTF("CWD\n");
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        if (M.x86.R_EAX & 0x80000000) {
            M.x86.R_EDX = 0xffffffff;
        }
        else {
            M.x86.R_EDX = 0x0;
        }
    }
    else {
        if (M.x86.R_AX & 0x8000) {
            M.x86.R_DX = 0xffff;
        }
        else {
            M.x86.R_DX = 0x0;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9a
****************************************************************************/
static void
x86emuOp_call_far_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 farseg, faroff;

    START_OF_INSTR();
    DECODE_PRINTF("CALL\t");
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        faroff = fetch_long_imm();
        farseg = fetch_word_imm();
    }
    else {
        faroff = fetch_word_imm();
        farseg = fetch_word_imm();
    }
    DECODE_PRINTF2("%04x:", farseg);
    DECODE_PRINTF2("%04x\n", faroff);
    CALL_TRACE(M.x86.saved_cs, M.x86.saved_ip, farseg, faroff, "FAR ");

    /* XXX
     *
     * Hooked interrupt vectors calling into our "BIOS" will cause
     * problems unless all intersegment stuff is checked for BIOS
     * access.  Check needed here.  For moment, let it alone.
     */
    TRACE_AND_STEP();
    push_word(M.x86.R_CS);
    M.x86.R_CS = farseg;
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_EIP);
    }
    else {
        push_word(M.x86.R_IP);
    }
    M.x86.R_EIP = faroff & 0xffff;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9b
****************************************************************************/
static void
x86emuOp_wait(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("WAIT");
    TRACE_AND_STEP();
    /* NADA.  */
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9c
****************************************************************************/
static void
x86emuOp_pushf_word(u8 X86EMU_UNUSED(op1))
{
    u32 flags;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSHFD\n");
    }
    else {
        DECODE_PRINTF("PUSHF\n");
    }
    TRACE_AND_STEP();

    /* clear out *all* bits not representing flags, and turn on real bits */
    flags = (M.x86.R_EFLG & F_MSK) | F_ALWAYS_ON;
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(flags);
    }
    else {
        push_word((u16) flags);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9d
****************************************************************************/
static void
x86emuOp_popf_word(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POPFD\n");
    }
    else {
        DECODE_PRINTF("POPF\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EFLG = pop_long();
    }
    else {
        M.x86.R_FLG = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9e
****************************************************************************/
static void
x86emuOp_sahf(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("SAHF\n");
    TRACE_AND_STEP();
    /* clear the lower bits of the flag register */
    M.x86.R_FLG &= 0xffffff00;
    /* or in the AH register into the flags register */
    M.x86.R_FLG |= M.x86.R_AH;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9f
****************************************************************************/
static void
x86emuOp_lahf(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("LAHF\n");
    TRACE_AND_STEP();
    M.x86.R_AH = (u8) (M.x86.R_FLG & 0xff);
    /* Undocumented TC++ behavior??? Nope.  It's documented, but
       you have to look real hard to notice it. */
    M.x86.R_AH |= 0x2;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa0
****************************************************************************/
static void
x86emuOp_mov_AL_M_IMM(u8 X86EMU_UNUSED(op1))
{
    u16 offset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tAL,");
    offset = fetch_word_imm();
    DECODE_PRINTF2("[%04x]\n", offset);
    TRACE_AND_STEP();
    M.x86.R_AL = fetch_data_byte(offset);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa1
****************************************************************************/
static void
x86emuOp_mov_AX_M_IMM(u8 X86EMU_UNUSED(op1))
{
    u16 offset;

    START_OF_INSTR();
    offset = fetch_word_imm();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF2("MOV\tEAX,[%04x]\n", offset);
    }
    else {
        DECODE_PRINTF2("MOV\tAX,[%04x]\n", offset);
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = fetch_data_long(offset);
    }
    else {
        M.x86.R_AX = fetch_data_word(offset);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa2
****************************************************************************/
static void
x86emuOp_mov_M_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u16 offset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    offset = fetch_word_imm();
    DECODE_PRINTF2("[%04x],AL\n", offset);
    TRACE_AND_STEP();
    store_data_byte(offset, M.x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa3
****************************************************************************/
static void
x86emuOp_mov_M_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u16 offset;

    START_OF_INSTR();
    offset = fetch_word_imm();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF2("MOV\t[%04x],EAX\n", offset);
    }
    else {
        DECODE_PRINTF2("MOV\t[%04x],AX\n", offset);
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        store_data_long(offset, M.x86.R_EAX);
    }
    else {
        store_data_word(offset, M.x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa4
****************************************************************************/
static void
x86emuOp_movs_byte(u8 X86EMU_UNUSED(op1))
{
    u8 val;
    u32 count;
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("MOVS\tBYTE\n");
    if (ACCESS_FLAG(F_DF))      /* down */
        inc = -1;
    else
        inc = 1;
    TRACE_AND_STEP();
    count = 1;
    if (M.x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* don't care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        count = M.x86.R_CX;
        M.x86.R_CX = 0;
        M.x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    while (count--) {
        val = fetch_data_byte(M.x86.R_SI);
        store_data_byte_abs(M.x86.R_ES, M.x86.R_DI, val);
        M.x86.R_SI += inc;
        M.x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa5
****************************************************************************/
static void
x86emuOp_movs_word(u8 X86EMU_UNUSED(op1))
{
    u32 val;
    int inc;
    u32 count;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOVS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -4;
        else
            inc = 4;
    }
    else {
        DECODE_PRINTF("MOVS\tWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    count = 1;
    if (M.x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* don't care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        count = M.x86.R_CX;
        M.x86.R_CX = 0;
        M.x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    while (count--) {
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            val = fetch_data_long(M.x86.R_SI);
            store_data_long_abs(M.x86.R_ES, M.x86.R_DI, val);
        }
        else {
            val = fetch_data_word(M.x86.R_SI);
            store_data_word_abs(M.x86.R_ES, M.x86.R_DI, (u16) val);
        }
        M.x86.R_SI += inc;
        M.x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa6
****************************************************************************/
static void
x86emuOp_cmps_byte(u8 X86EMU_UNUSED(op1))
{
    s8 val1, val2;
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("CMPS\tBYTE\n");
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_DF))      /* down */
        inc = -1;
    else
        inc = 1;

    if (M.x86.mode & SYSMODE_PREFIX_REPE) {
        /* REPE  */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            val1 = fetch_data_byte(M.x86.R_SI);
            val2 = fetch_data_byte_abs(M.x86.R_ES, M.x86.R_DI);
            cmp_byte(val1, val2);
            M.x86.R_CX -= 1;
            M.x86.R_SI += inc;
            M.x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF) == 0)
                break;
        }
        M.x86.mode &= ~SYSMODE_PREFIX_REPE;
    }
    else if (M.x86.mode & SYSMODE_PREFIX_REPNE) {
        /* REPNE  */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            val1 = fetch_data_byte(M.x86.R_SI);
            val2 = fetch_data_byte_abs(M.x86.R_ES, M.x86.R_DI);
            cmp_byte(val1, val2);
            M.x86.R_CX -= 1;
            M.x86.R_SI += inc;
            M.x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF))
                break;          /* zero flag set means equal */
        }
        M.x86.mode &= ~SYSMODE_PREFIX_REPNE;
    }
    else {
        val1 = fetch_data_byte(M.x86.R_SI);
        val2 = fetch_data_byte_abs(M.x86.R_ES, M.x86.R_DI);
        cmp_byte(val1, val2);
        M.x86.R_SI += inc;
        M.x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa7
****************************************************************************/
static void
x86emuOp_cmps_word(u8 X86EMU_UNUSED(op1))
{
    u32 val1, val2;
    int inc;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("CMPS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -4;
        else
            inc = 4;
    }
    else {
        DECODE_PRINTF("CMPS\tWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_REPE) {
        /* REPE  */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                val1 = fetch_data_long(M.x86.R_SI);
                val2 = fetch_data_long_abs(M.x86.R_ES, M.x86.R_DI);
                cmp_long(val1, val2);
            }
            else {
                val1 = fetch_data_word(M.x86.R_SI);
                val2 = fetch_data_word_abs(M.x86.R_ES, M.x86.R_DI);
                cmp_word((u16) val1, (u16) val2);
            }
            M.x86.R_CX -= 1;
            M.x86.R_SI += inc;
            M.x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF) == 0)
                break;
        }
        M.x86.mode &= ~SYSMODE_PREFIX_REPE;
    }
    else if (M.x86.mode & SYSMODE_PREFIX_REPNE) {
        /* REPNE  */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                val1 = fetch_data_long(M.x86.R_SI);
                val2 = fetch_data_long_abs(M.x86.R_ES, M.x86.R_DI);
                cmp_long(val1, val2);
            }
            else {
                val1 = fetch_data_word(M.x86.R_SI);
                val2 = fetch_data_word_abs(M.x86.R_ES, M.x86.R_DI);
                cmp_word((u16) val1, (u16) val2);
            }
            M.x86.R_CX -= 1;
            M.x86.R_SI += inc;
            M.x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF))
                break;          /* zero flag set means equal */
        }
        M.x86.mode &= ~SYSMODE_PREFIX_REPNE;
    }
    else {
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            val1 = fetch_data_long(M.x86.R_SI);
            val2 = fetch_data_long_abs(M.x86.R_ES, M.x86.R_DI);
            cmp_long(val1, val2);
        }
        else {
            val1 = fetch_data_word(M.x86.R_SI);
            val2 = fetch_data_word_abs(M.x86.R_ES, M.x86.R_DI);
            cmp_word((u16) val1, (u16) val2);
        }
        M.x86.R_SI += inc;
        M.x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa8
****************************************************************************/
static void
x86emuOp_test_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    int imm;

    START_OF_INSTR();
    DECODE_PRINTF("TEST\tAL,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%04x\n", imm);
    TRACE_AND_STEP();
    test_byte(M.x86.R_AL, (u8) imm);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa9
****************************************************************************/
static void
x86emuOp_test_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("TEST\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("TEST\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        test_long(M.x86.R_EAX, srcval);
    }
    else {
        test_word(M.x86.R_AX, (u16) srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xaa
****************************************************************************/
static void
x86emuOp_stos_byte(u8 X86EMU_UNUSED(op1))
{
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("STOS\tBYTE\n");
    if (ACCESS_FLAG(F_DF))      /* down */
        inc = -1;
    else
        inc = 1;
    TRACE_AND_STEP();
    if (M.x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* don't care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            store_data_byte_abs(M.x86.R_ES, M.x86.R_DI, M.x86.R_AL);
            M.x86.R_CX -= 1;
            M.x86.R_DI += inc;
        }
        M.x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    else {
        store_data_byte_abs(M.x86.R_ES, M.x86.R_DI, M.x86.R_AL);
        M.x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xab
****************************************************************************/
static void
x86emuOp_stos_word(u8 X86EMU_UNUSED(op1))
{
    int inc;
    u32 count;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("STOS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -4;
        else
            inc = 4;
    }
    else {
        DECODE_PRINTF("STOS\tWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    count = 1;
    if (M.x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* don't care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        count = M.x86.R_CX;
        M.x86.R_CX = 0;
        M.x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    while (count--) {
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            store_data_long_abs(M.x86.R_ES, M.x86.R_DI, M.x86.R_EAX);
        }
        else {
            store_data_word_abs(M.x86.R_ES, M.x86.R_DI, M.x86.R_AX);
        }
        M.x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xac
****************************************************************************/
static void
x86emuOp_lods_byte(u8 X86EMU_UNUSED(op1))
{
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("LODS\tBYTE\n");
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_DF))      /* down */
        inc = -1;
    else
        inc = 1;
    if (M.x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* don't care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            M.x86.R_AL = fetch_data_byte(M.x86.R_SI);
            M.x86.R_CX -= 1;
            M.x86.R_SI += inc;
        }
        M.x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    else {
        M.x86.R_AL = fetch_data_byte(M.x86.R_SI);
        M.x86.R_SI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xad
****************************************************************************/
static void
x86emuOp_lods_word(u8 X86EMU_UNUSED(op1))
{
    int inc;
    u32 count;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("LODS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -4;
        else
            inc = 4;
    }
    else {
        DECODE_PRINTF("LODS\tWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    count = 1;
    if (M.x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* don't care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        count = M.x86.R_CX;
        M.x86.R_CX = 0;
        M.x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    while (count--) {
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            M.x86.R_EAX = fetch_data_long(M.x86.R_SI);
        }
        else {
            M.x86.R_AX = fetch_data_word(M.x86.R_SI);
        }
        M.x86.R_SI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xae
****************************************************************************/
static void
x86emuOp_scas_byte(u8 X86EMU_UNUSED(op1))
{
    s8 val2;
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("SCAS\tBYTE\n");
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_DF))      /* down */
        inc = -1;
    else
        inc = 1;
    if (M.x86.mode & SYSMODE_PREFIX_REPE) {
        /* REPE  */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            val2 = fetch_data_byte_abs(M.x86.R_ES, M.x86.R_DI);
            cmp_byte(M.x86.R_AL, val2);
            M.x86.R_CX -= 1;
            M.x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF) == 0)
                break;
        }
        M.x86.mode &= ~SYSMODE_PREFIX_REPE;
    }
    else if (M.x86.mode & SYSMODE_PREFIX_REPNE) {
        /* REPNE  */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            val2 = fetch_data_byte_abs(M.x86.R_ES, M.x86.R_DI);
            cmp_byte(M.x86.R_AL, val2);
            M.x86.R_CX -= 1;
            M.x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF))
                break;          /* zero flag set means equal */
        }
        M.x86.mode &= ~SYSMODE_PREFIX_REPNE;
    }
    else {
        val2 = fetch_data_byte_abs(M.x86.R_ES, M.x86.R_DI);
        cmp_byte(M.x86.R_AL, val2);
        M.x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xaf
****************************************************************************/
static void
x86emuOp_scas_word(u8 X86EMU_UNUSED(op1))
{
    int inc;
    u32 val;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("SCAS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -4;
        else
            inc = 4;
    }
    else {
        DECODE_PRINTF("SCAS\tWORD\n");
        if (ACCESS_FLAG(F_DF))  /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_REPE) {
        /* REPE  */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                val = fetch_data_long_abs(M.x86.R_ES, M.x86.R_DI);
                cmp_long(M.x86.R_EAX, val);
            }
            else {
                val = fetch_data_word_abs(M.x86.R_ES, M.x86.R_DI);
                cmp_word(M.x86.R_AX, (u16) val);
            }
            M.x86.R_CX -= 1;
            M.x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF) == 0)
                break;
        }
        M.x86.mode &= ~SYSMODE_PREFIX_REPE;
    }
    else if (M.x86.mode & SYSMODE_PREFIX_REPNE) {
        /* REPNE  */
        /* move them until CX is ZERO. */
        while (M.x86.R_CX != 0) {
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                val = fetch_data_long_abs(M.x86.R_ES, M.x86.R_DI);
                cmp_long(M.x86.R_EAX, val);
            }
            else {
                val = fetch_data_word_abs(M.x86.R_ES, M.x86.R_DI);
                cmp_word(M.x86.R_AX, (u16) val);
            }
            M.x86.R_CX -= 1;
            M.x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF))
                break;          /* zero flag set means equal */
        }
        M.x86.mode &= ~SYSMODE_PREFIX_REPNE;
    }
    else {
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            val = fetch_data_long_abs(M.x86.R_ES, M.x86.R_DI);
            cmp_long(M.x86.R_EAX, val);
        }
        else {
            val = fetch_data_word_abs(M.x86.R_ES, M.x86.R_DI);
            cmp_word(M.x86.R_AX, (u16) val);
        }
        M.x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb0
****************************************************************************/
static void
x86emuOp_mov_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tAL,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", imm);
    TRACE_AND_STEP();
    M.x86.R_AL = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb1
****************************************************************************/
static void
x86emuOp_mov_byte_CL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tCL,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", imm);
    TRACE_AND_STEP();
    M.x86.R_CL = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb2
****************************************************************************/
static void
x86emuOp_mov_byte_DL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tDL,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", imm);
    TRACE_AND_STEP();
    M.x86.R_DL = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb3
****************************************************************************/
static void
x86emuOp_mov_byte_BL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tBL,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", imm);
    TRACE_AND_STEP();
    M.x86.R_BL = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb4
****************************************************************************/
static void
x86emuOp_mov_byte_AH_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tAH,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", imm);
    TRACE_AND_STEP();
    M.x86.R_AH = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb5
****************************************************************************/
static void
x86emuOp_mov_byte_CH_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tCH,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", imm);
    TRACE_AND_STEP();
    M.x86.R_CH = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb6
****************************************************************************/
static void
x86emuOp_mov_byte_DH_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tDH,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", imm);
    TRACE_AND_STEP();
    M.x86.R_DH = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb7
****************************************************************************/
static void
x86emuOp_mov_byte_BH_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tBH,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", imm);
    TRACE_AND_STEP();
    M.x86.R_BH = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb8
****************************************************************************/
static void
x86emuOp_mov_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOV\tEAX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("MOV\tAX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = srcval;
    }
    else {
        M.x86.R_AX = (u16) srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb9
****************************************************************************/
static void
x86emuOp_mov_word_CX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOV\tECX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("MOV\tCX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ECX = srcval;
    }
    else {
        M.x86.R_CX = (u16) srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xba
****************************************************************************/
static void
x86emuOp_mov_word_DX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOV\tEDX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("MOV\tDX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDX = srcval;
    }
    else {
        M.x86.R_DX = (u16) srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xbb
****************************************************************************/
static void
x86emuOp_mov_word_BX_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOV\tEBX,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("MOV\tBX,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBX = srcval;
    }
    else {
        M.x86.R_BX = (u16) srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xbc
****************************************************************************/
static void
x86emuOp_mov_word_SP_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOV\tESP,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("MOV\tSP,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ESP = srcval;
    }
    else {
        M.x86.R_SP = (u16) srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xbd
****************************************************************************/
static void
x86emuOp_mov_word_BP_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOV\tEBP,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("MOV\tBP,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBP = srcval;
    }
    else {
        M.x86.R_BP = (u16) srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xbe
****************************************************************************/
static void
x86emuOp_mov_word_SI_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOV\tESI,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("MOV\tSI,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_ESI = srcval;
    }
    else {
        M.x86.R_SI = (u16) srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xbf
****************************************************************************/
static void
x86emuOp_mov_word_DI_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 srcval;

    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOV\tEDI,");
        srcval = fetch_long_imm();
    }
    else {
        DECODE_PRINTF("MOV\tDI,");
        srcval = fetch_word_imm();
    }
    DECODE_PRINTF2("%x\n", srcval);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EDI = srcval;
    }
    else {
        M.x86.R_DI = (u16) srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/* used by opcodes c0, d0, and d2. */
static u8(*opcD0_byte_operation[]) (u8 d, u8 s) = {
    rol_byte, ror_byte, rcl_byte, rcr_byte, shl_byte, shr_byte, shl_byte,       /* sal_byte === shl_byte  by definition */
sar_byte,};

/****************************************************************************
REMARKS:
Handles opcode 0xc0
****************************************************************************/
static void
x86emuOp_opcC0_byte_RM_MEM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg;
    uint destoffset;
    u8 destval;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */

        switch (rh) {
        case 0:
            DECODE_PRINTF("ROL\t");
            break;
        case 1:
            DECODE_PRINTF("ROR\t");
            break;
        case 2:
            DECODE_PRINTF("RCL\t");
            break;
        case 3:
            DECODE_PRINTF("RCR\t");
            break;
        case 4:
            DECODE_PRINTF("SHL\t");
            break;
        case 5:
            DECODE_PRINTF("SHR\t");
            break;
        case 6:
            DECODE_PRINTF("SAL\t");
            break;
        case 7:
            DECODE_PRINTF("SAR\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    switch (mod) {
    case 0:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm00_address(rl);
        amt = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", amt);
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, amt);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm01_address(rl);
        amt = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", amt);
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, amt);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm10_address(rl);
        amt = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", amt);
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, amt);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        amt = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", amt);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (*destreg, amt);
        *destreg = destval;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/* used by opcodes c1, d1, and d3. */
static u16(*opcD1_word_operation[]) (u16 s, u8 d) = {
    rol_word, ror_word, rcl_word, rcr_word, shl_word, shr_word, shl_word,       /* sal_byte === shl_byte  by definition */
sar_word,};

/* used by opcodes c1, d1, and d3. */
static u32(*opcD1_long_operation[]) (u32 s, u8 d) = {
    rol_long, ror_long, rcl_long, rcr_long, shl_long, shr_long, shl_long,       /* sal_byte === shl_byte  by definition */
sar_long,};

/****************************************************************************
REMARKS:
Handles opcode 0xc1
****************************************************************************/
static void
x86emuOp_opcC1_word_RM_MEM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */

        switch (rh) {
        case 0:
            DECODE_PRINTF("ROL\t");
            break;
        case 1:
            DECODE_PRINTF("ROR\t");
            break;
        case 2:
            DECODE_PRINTF("RCL\t");
            break;
        case 3:
            DECODE_PRINTF("RCR\t");
            break;
        case 4:
            DECODE_PRINTF("SHL\t");
            break;
        case 5:
            DECODE_PRINTF("SHR\t");
            break;
        case 6:
            DECODE_PRINTF("SAL\t");
            break;
        case 7:
            DECODE_PRINTF("SAR\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm00_address(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, amt);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm00_address(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, amt);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm01_address(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, amt);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm01_address(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, amt);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm10_address(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, amt);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm10_address(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, amt);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            TRACE_AND_STEP();
            *destreg = (*opcD1_long_operation[rh]) (*destreg, amt);
        }
        else {
            u16 *destreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            TRACE_AND_STEP();
            *destreg = (*opcD1_word_operation[rh]) (*destreg, amt);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc2
****************************************************************************/
static void
x86emuOp_ret_near_IMM(u8 X86EMU_UNUSED(op1))
{
    u16 imm;

    START_OF_INSTR();
    DECODE_PRINTF("RET\t");
    imm = fetch_word_imm();
    DECODE_PRINTF2("%x\n", imm);
    RETURN_TRACE("RET", M.x86.saved_cs, M.x86.saved_ip);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EIP = pop_long();
    } else {
        M.x86.R_IP = pop_word();
    }
    M.x86.R_SP += imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc3
****************************************************************************/
static void
x86emuOp_ret_near(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("RET\n");
    RETURN_TRACE("RET", M.x86.saved_cs, M.x86.saved_ip);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EIP = pop_long();
    } else {
        M.x86.R_IP = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc4
****************************************************************************/
static void
x86emuOp_les_R_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rh, rl;
    u16 *dstreg;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("LES\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        dstreg = DECODE_RM_WORD_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *dstreg = fetch_data_word(srcoffset);
        M.x86.R_ES = fetch_data_word(srcoffset + 2);
        break;
    case 1:
        dstreg = DECODE_RM_WORD_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *dstreg = fetch_data_word(srcoffset);
        M.x86.R_ES = fetch_data_word(srcoffset + 2);
        break;
    case 2:
        dstreg = DECODE_RM_WORD_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *dstreg = fetch_data_word(srcoffset);
        M.x86.R_ES = fetch_data_word(srcoffset + 2);
        break;
    case 3:                    /* register to register */
        /* UNDEFINED! */
        TRACE_AND_STEP();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc5
****************************************************************************/
static void
x86emuOp_lds_R_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rh, rl;
    u16 *dstreg;
    uint srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("LDS\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:
        dstreg = DECODE_RM_WORD_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *dstreg = fetch_data_word(srcoffset);
        M.x86.R_DS = fetch_data_word(srcoffset + 2);
        break;
    case 1:
        dstreg = DECODE_RM_WORD_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *dstreg = fetch_data_word(srcoffset);
        M.x86.R_DS = fetch_data_word(srcoffset + 2);
        break;
    case 2:
        dstreg = DECODE_RM_WORD_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *dstreg = fetch_data_word(srcoffset);
        M.x86.R_DS = fetch_data_word(srcoffset + 2);
        break;
    case 3:                    /* register to register */
        /* UNDEFINED! */
        TRACE_AND_STEP();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc6
****************************************************************************/
static void
x86emuOp_mov_byte_RM_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg;
    uint destoffset;
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (rh != 0) {
        DECODE_PRINTF("ILLEGAL DECODE OF OPCODE c6\n");
        HALT_SYS();
    }
    switch (mod) {
    case 0:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm00_address(rl);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%2x\n", imm);
        TRACE_AND_STEP();
        store_data_byte(destoffset, imm);
        break;
    case 1:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm01_address(rl);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%2x\n", imm);
        TRACE_AND_STEP();
        store_data_byte(destoffset, imm);
        break;
    case 2:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm10_address(rl);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%2x\n", imm);
        TRACE_AND_STEP();
        store_data_byte(destoffset, imm);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%2x\n", imm);
        TRACE_AND_STEP();
        *destreg = imm;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc7
****************************************************************************/
static void
x86emuOp_mov_word_RM_IMM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (rh != 0) {
        DECODE_PRINTF("ILLEGAL DECODE OF OPCODE 8F\n");
        HALT_SYS();
    }
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm00_address(rl);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            store_data_long(destoffset, imm);
        }
        else {
            u16 imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm00_address(rl);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            store_data_word(destoffset, imm);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm01_address(rl);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            store_data_long(destoffset, imm);
        }
        else {
            u16 imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm01_address(rl);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            store_data_word(destoffset, imm);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm10_address(rl);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            store_data_long(destoffset, imm);
        }
        else {
            u16 imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm10_address(rl);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            store_data_word(destoffset, imm);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            *destreg = imm;
        }
        else {
            u16 *destreg;
            u16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%x\n", imm);
            TRACE_AND_STEP();
            *destreg = imm;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc8
****************************************************************************/
static void
x86emuOp_enter(u8 X86EMU_UNUSED(op1))
{
    u16 local, frame_pointer;
    u8 nesting;
    int i;

    START_OF_INSTR();
    local = fetch_word_imm();
    nesting = fetch_byte_imm();
    DECODE_PRINTF2("ENTER %x\n", local);
    DECODE_PRINTF2(",%x\n", nesting);
    TRACE_AND_STEP();
    push_word(M.x86.R_BP);
    frame_pointer = M.x86.R_SP;
    if (nesting > 0) {
        for (i = 1; i < nesting; i++) {
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                M.x86.R_BP -= 4;
                push_long(fetch_data_long_abs(M.x86.R_SS, M.x86.R_BP));
            } else {
                M.x86.R_BP -= 2;
                push_word(fetch_data_word_abs(M.x86.R_SS, M.x86.R_BP));
            }
        }
        push_word(frame_pointer);
    }
    M.x86.R_BP = frame_pointer;
    M.x86.R_SP = (u16) (M.x86.R_SP - local);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc9
****************************************************************************/
static void
x86emuOp_leave(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("LEAVE\n");
    TRACE_AND_STEP();
    M.x86.R_SP = M.x86.R_BP;
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EBP = pop_long();
    } else {
        M.x86.R_BP = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xca
****************************************************************************/
static void
x86emuOp_ret_far_IMM(u8 X86EMU_UNUSED(op1))
{
    u16 imm;

    START_OF_INSTR();
    DECODE_PRINTF("RETF\t");
    imm = fetch_word_imm();
    DECODE_PRINTF2("%x\n", imm);
    RETURN_TRACE("RETF", M.x86.saved_cs, M.x86.saved_ip);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EIP = pop_long();
        M.x86.R_CS = pop_long() & 0xffff;
    } else {
        M.x86.R_IP = pop_word();
        M.x86.R_CS = pop_word();
    }
    M.x86.R_SP += imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xcb
****************************************************************************/
static void
x86emuOp_ret_far(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("RETF\n");
    RETURN_TRACE("RETF", M.x86.saved_cs, M.x86.saved_ip);
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EIP = pop_long();
        M.x86.R_CS = pop_long() & 0xffff;
    } else {
        M.x86.R_IP = pop_word();
        M.x86.R_CS = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xcc
****************************************************************************/
static void
x86emuOp_int3(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("INT 3\n");
    TRACE_AND_STEP();
    if (_X86EMU_intrTab[3]) {
        (*_X86EMU_intrTab[3]) (3);
    }
    else {
        push_word((u16) M.x86.R_FLG);
        CLEAR_FLAG(F_IF);
        CLEAR_FLAG(F_TF);
        push_word(M.x86.R_CS);
        M.x86.R_CS = mem_access_word(3 * 4 + 2);
        push_word(M.x86.R_IP);
        M.x86.R_IP = mem_access_word(3 * 4);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xcd
****************************************************************************/
static void
x86emuOp_int_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 intnum;

    START_OF_INSTR();
    DECODE_PRINTF("INT\t");
    intnum = fetch_byte_imm();
    DECODE_PRINTF2("%x\n", intnum);
    TRACE_AND_STEP();
    if (_X86EMU_intrTab[intnum]) {
        (*_X86EMU_intrTab[intnum]) (intnum);
    }
    else {
        push_word((u16) M.x86.R_FLG);
        CLEAR_FLAG(F_IF);
        CLEAR_FLAG(F_TF);
        push_word(M.x86.R_CS);
        M.x86.R_CS = mem_access_word(intnum * 4 + 2);
        push_word(M.x86.R_IP);
        M.x86.R_IP = mem_access_word(intnum * 4);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xce
****************************************************************************/
static void
x86emuOp_into(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("INTO\n");
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_OF)) {
        if (_X86EMU_intrTab[4]) {
            (*_X86EMU_intrTab[4]) (4);
        }
        else {
            push_word((u16) M.x86.R_FLG);
            CLEAR_FLAG(F_IF);
            CLEAR_FLAG(F_TF);
            push_word(M.x86.R_CS);
            M.x86.R_CS = mem_access_word(4 * 4 + 2);
            push_word(M.x86.R_IP);
            M.x86.R_IP = mem_access_word(4 * 4);
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xcf
****************************************************************************/
static void
x86emuOp_iret(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("IRET\n");

    TRACE_AND_STEP();

    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EIP = pop_long();
        M.x86.R_CS = pop_long() & 0xffff;
        M.x86.R_EFLG = (pop_long() & 0x257FD5) | (M.x86.R_EFLG & 0x1A0000);
    } else {
        M.x86.R_IP = pop_word();
        M.x86.R_CS = pop_word();
        M.x86.R_FLG = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd0
****************************************************************************/
static void
x86emuOp_opcD0_byte_RM_1(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg;
    uint destoffset;
    u8 destval;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */
        switch (rh) {
        case 0:
            DECODE_PRINTF("ROL\t");
            break;
        case 1:
            DECODE_PRINTF("ROR\t");
            break;
        case 2:
            DECODE_PRINTF("RCL\t");
            break;
        case 3:
            DECODE_PRINTF("RCR\t");
            break;
        case 4:
            DECODE_PRINTF("SHL\t");
            break;
        case 5:
            DECODE_PRINTF("SHR\t");
            break;
        case 6:
            DECODE_PRINTF("SAL\t");
            break;
        case 7:
            DECODE_PRINTF("SAR\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    switch (mod) {
    case 0:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",1\n");
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, 1);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",1\n");
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, 1);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",1\n");
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, 1);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",1\n");
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (*destreg, 1);
        *destreg = destval;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd1
****************************************************************************/
static void
x86emuOp_opcD1_word_RM_1(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */
        switch (rh) {
        case 0:
            DECODE_PRINTF("ROL\t");
            break;
        case 1:
            DECODE_PRINTF("ROR\t");
            break;
        case 2:
            DECODE_PRINTF("RCL\t");
            break;
        case 3:
            DECODE_PRINTF("RCR\t");
            break;
        case 4:
            DECODE_PRINTF("SHL\t");
            break;
        case 5:
            DECODE_PRINTF("SHR\t");
            break;
        case 6:
            DECODE_PRINTF("SAL\t");
            break;
        case 7:
            DECODE_PRINTF("SAR\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",1\n");
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, 1);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",1\n");
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, 1);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",1\n");
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, 1);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",1\n");
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, 1);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",1\n");
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, 1);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("BYTE PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",1\n");
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, 1);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *destreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",1\n");
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (*destreg, 1);
            *destreg = destval;
        }
        else {
            u16 destval;
            u16 *destreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",1\n");
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (*destreg, 1);
            *destreg = destval;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd2
****************************************************************************/
static void
x86emuOp_opcD2_byte_RM_CL(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg;
    uint destoffset;
    u8 destval;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */
        switch (rh) {
        case 0:
            DECODE_PRINTF("ROL\t");
            break;
        case 1:
            DECODE_PRINTF("ROR\t");
            break;
        case 2:
            DECODE_PRINTF("RCL\t");
            break;
        case 3:
            DECODE_PRINTF("RCR\t");
            break;
        case 4:
            DECODE_PRINTF("SHL\t");
            break;
        case 5:
            DECODE_PRINTF("SHR\t");
            break;
        case 6:
            DECODE_PRINTF("SAL\t");
            break;
        case 7:
            DECODE_PRINTF("SAR\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    amt = M.x86.R_CL;
    switch (mod) {
    case 0:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF(",CL\n");
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, amt);
        store_data_byte(destoffset, destval);
        break;
    case 1:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF(",CL\n");
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, amt);
        store_data_byte(destoffset, destval);
        break;
    case 2:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF(",CL\n");
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (destval, amt);
        store_data_byte(destoffset, destval);
        break;
    case 3:                    /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",CL\n");
        TRACE_AND_STEP();
        destval = (*opcD0_byte_operation[rh]) (*destreg, amt);
        *destreg = destval;
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd3
****************************************************************************/
static void
x86emuOp_opcD3_word_RM_CL(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */
        switch (rh) {
        case 0:
            DECODE_PRINTF("ROL\t");
            break;
        case 1:
            DECODE_PRINTF("ROR\t");
            break;
        case 2:
            DECODE_PRINTF("RCL\t");
            break;
        case 3:
            DECODE_PRINTF("RCR\t");
            break;
        case 4:
            DECODE_PRINTF("SHL\t");
            break;
        case 5:
            DECODE_PRINTF("SHR\t");
            break;
        case 6:
            DECODE_PRINTF("SAL\t");
            break;
        case 7:
            DECODE_PRINTF("SAR\t");
            break;
        }
    }
#endif
    /* know operation, decode the mod byte to find the addressing
       mode. */
    amt = M.x86.R_CL;
    switch (mod) {
    case 0:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",CL\n");
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, amt);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",CL\n");
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, amt);
            store_data_word(destoffset, destval);
        }
        break;
    case 1:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",CL\n");
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, amt);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",CL\n");
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, amt);
            store_data_word(destoffset, destval);
        }
        break;
    case 2:
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",CL\n");
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_long_operation[rh]) (destval, amt);
            store_data_long(destoffset, destval);
        }
        else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",CL\n");
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*opcD1_word_operation[rh]) (destval, amt);
            store_data_word(destoffset, destval);
        }
        break;
    case 3:                    /* register to register */
        if (M.x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",CL\n");
            TRACE_AND_STEP();
            *destreg = (*opcD1_long_operation[rh]) (*destreg, amt);
        }
        else {
            u16 *destreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",CL\n");
            TRACE_AND_STEP();
            *destreg = (*opcD1_word_operation[rh]) (*destreg, amt);
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd4
****************************************************************************/
static void
x86emuOp_aam(u8 X86EMU_UNUSED(op1))
{
    u8 a;

    START_OF_INSTR();
    DECODE_PRINTF("AAM\n");
    a = fetch_byte_imm();       /* this is a stupid encoding. */
    if (a != 10) {
        /* fix: add base decoding
           aam_word(u8 val, int base a) */
        DECODE_PRINTF("ERROR DECODING AAM\n");
        TRACE_REGS();
        HALT_SYS();
    }
    TRACE_AND_STEP();
    /* note the type change here --- returning AL and AH in AX. */
    M.x86.R_AX = aam_word(M.x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd5
****************************************************************************/
static void
x86emuOp_aad(u8 X86EMU_UNUSED(op1))
{
    u8 a;

    START_OF_INSTR();
    DECODE_PRINTF("AAD\n");
    a = fetch_byte_imm();
    if (a != 10) {
        /* fix: add base decoding
           aad_word(u16 val, int base a) */
        DECODE_PRINTF("ERROR DECODING AAM\n");
        TRACE_REGS();
        HALT_SYS();
    }
    TRACE_AND_STEP();
    M.x86.R_AX = aad_word(M.x86.R_AX);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/* opcode 0xd6 ILLEGAL OPCODE */

/****************************************************************************
REMARKS:
Handles opcode 0xd7
****************************************************************************/
static void
x86emuOp_xlat(u8 X86EMU_UNUSED(op1))
{
    u16 addr;

    START_OF_INSTR();
    DECODE_PRINTF("XLAT\n");
    TRACE_AND_STEP();
    addr = (u16) (M.x86.R_BX + (u8) M.x86.R_AL);
    M.x86.R_AL = fetch_data_byte(addr);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/* instuctions  D8 .. DF are in i87_ops.c */

/****************************************************************************
REMARKS:
Handles opcode 0xe0
****************************************************************************/
static void
x86emuOp_loopne(u8 X86EMU_UNUSED(op1))
{
    s16 ip;

    START_OF_INSTR();
    DECODE_PRINTF("LOOPNE\t");
    ip = (s8) fetch_byte_imm();
    ip += (s16) M.x86.R_IP;
    DECODE_PRINTF2("%04x\n", ip);
    TRACE_AND_STEP();
    M.x86.R_CX -= 1;
    if (M.x86.R_CX != 0 && !ACCESS_FLAG(F_ZF))  /* CX != 0 and !ZF */
        M.x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe1
****************************************************************************/
static void
x86emuOp_loope(u8 X86EMU_UNUSED(op1))
{
    s16 ip;

    START_OF_INSTR();
    DECODE_PRINTF("LOOPE\t");
    ip = (s8) fetch_byte_imm();
    ip += (s16) M.x86.R_IP;
    DECODE_PRINTF2("%04x\n", ip);
    TRACE_AND_STEP();
    M.x86.R_CX -= 1;
    if (M.x86.R_CX != 0 && ACCESS_FLAG(F_ZF))   /* CX != 0 and ZF */
        M.x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe2
****************************************************************************/
static void
x86emuOp_loop(u8 X86EMU_UNUSED(op1))
{
    s16 ip;

    START_OF_INSTR();
    DECODE_PRINTF("LOOP\t");
    ip = (s8) fetch_byte_imm();
    ip += (s16) M.x86.R_IP;
    DECODE_PRINTF2("%04x\n", ip);
    TRACE_AND_STEP();
    M.x86.R_CX -= 1;
    if (M.x86.R_CX != 0)
        M.x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe3
****************************************************************************/
static void
x86emuOp_jcxz(u8 X86EMU_UNUSED(op1))
{
    u16 target;
    s8 offset;

    /* jump to byte offset if overflow flag is set */
    START_OF_INSTR();
    DECODE_PRINTF("JCXZ\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    if (M.x86.R_CX == 0)
        M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe4
****************************************************************************/
static void
x86emuOp_in_byte_AL_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 port;

    START_OF_INSTR();
    DECODE_PRINTF("IN\t");
    port = (u8) fetch_byte_imm();
    DECODE_PRINTF2("%x,AL\n", port);
    TRACE_AND_STEP();
    M.x86.R_AL = (*sys_inb) (port);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe5
****************************************************************************/
static void
x86emuOp_in_word_AX_IMM(u8 X86EMU_UNUSED(op1))
{
    u8 port;

    START_OF_INSTR();
    DECODE_PRINTF("IN\t");
    port = (u8) fetch_byte_imm();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF2("EAX,%x\n", port);
    }
    else {
        DECODE_PRINTF2("AX,%x\n", port);
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = (*sys_inl) (port);
    }
    else {
        M.x86.R_AX = (*sys_inw) (port);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe6
****************************************************************************/
static void
x86emuOp_out_byte_IMM_AL(u8 X86EMU_UNUSED(op1))
{
    u8 port;

    START_OF_INSTR();
    DECODE_PRINTF("OUT\t");
    port = (u8) fetch_byte_imm();
    DECODE_PRINTF2("%x,AL\n", port);
    TRACE_AND_STEP();
    (*sys_outb) (port, M.x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe7
****************************************************************************/
static void
x86emuOp_out_word_IMM_AX(u8 X86EMU_UNUSED(op1))
{
    u8 port;

    START_OF_INSTR();
    DECODE_PRINTF("OUT\t");
    port = (u8) fetch_byte_imm();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF2("%x,EAX\n", port);
    }
    else {
        DECODE_PRINTF2("%x,AX\n", port);
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        (*sys_outl) (port, M.x86.R_EAX);
    }
    else {
        (*sys_outw) (port, M.x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe8
****************************************************************************/
static void
x86emuOp_call_near_IMM(u8 X86EMU_UNUSED(op1))
{
    s16 ip16 = 0;
    s32 ip32 = 0;

    START_OF_INSTR();
    DECODE_PRINTF("CALL\t");
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        ip32 = (s32) fetch_long_imm();
        ip32 += (s16) M.x86.R_IP;       /* CHECK SIGN */
        DECODE_PRINTF2("%04x\n", (u16) ip32);
        CALL_TRACE(M.x86.saved_cs, M.x86.saved_ip, M.x86.R_CS, ip32, "");
    }
    else {
        ip16 = (s16) fetch_word_imm();
        ip16 += (s16) M.x86.R_IP;       /* CHECK SIGN */
        DECODE_PRINTF2("%04x\n", (u16) ip16);
        CALL_TRACE(M.x86.saved_cs, M.x86.saved_ip, M.x86.R_CS, ip16, "");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(M.x86.R_EIP);
        M.x86.R_EIP = ip32 & 0xffff;
    }
    else {
        push_word(M.x86.R_IP);
        M.x86.R_EIP = ip16;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe9
****************************************************************************/
static void
x86emuOp_jump_near_IMM(u8 X86EMU_UNUSED(op1))
{
    u32 ip;

    START_OF_INSTR();
    DECODE_PRINTF("JMP\t");
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        ip = (u32) fetch_long_imm();
        ip += (u32) M.x86.R_EIP;
        DECODE_PRINTF2("%08x\n", (u32) ip);
        TRACE_AND_STEP();
        M.x86.R_EIP = (u32) ip;
    }
    else {
        ip = (s16) fetch_word_imm();
        ip += (s16) M.x86.R_IP;
        DECODE_PRINTF2("%04x\n", (u16) ip);
        TRACE_AND_STEP();
        M.x86.R_IP = (u16) ip;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xea
****************************************************************************/
static void
x86emuOp_jump_far_IMM(u8 X86EMU_UNUSED(op1))
{
    u16 cs;
    u32 ip;

    START_OF_INSTR();
    DECODE_PRINTF("JMP\tFAR ");
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        ip = fetch_long_imm();
    }
    else {
        ip = fetch_word_imm();
    }
    cs = fetch_word_imm();
    DECODE_PRINTF2("%04x:", cs);
    DECODE_PRINTF2("%04x\n", ip);
    TRACE_AND_STEP();
    M.x86.R_EIP = ip & 0xffff;
    M.x86.R_CS = cs;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xeb
****************************************************************************/
static void
x86emuOp_jump_byte_IMM(u8 X86EMU_UNUSED(op1))
{
    u16 target;
    s8 offset;

    START_OF_INSTR();
    DECODE_PRINTF("JMP\t");
    offset = (s8) fetch_byte_imm();
    target = (u16) (M.x86.R_IP + offset);
    DECODE_PRINTF2("%x\n", target);
    TRACE_AND_STEP();
    M.x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xec
****************************************************************************/
static void
x86emuOp_in_byte_AL_DX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("IN\tAL,DX\n");
    TRACE_AND_STEP();
    M.x86.R_AL = (*sys_inb) (M.x86.R_DX);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xed
****************************************************************************/
static void
x86emuOp_in_word_AX_DX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("IN\tEAX,DX\n");
    }
    else {
        DECODE_PRINTF("IN\tAX,DX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        M.x86.R_EAX = (*sys_inl) (M.x86.R_DX);
    }
    else {
        M.x86.R_AX = (*sys_inw) (M.x86.R_DX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xee
****************************************************************************/
static void
x86emuOp_out_byte_DX_AL(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("OUT\tDX,AL\n");
    TRACE_AND_STEP();
    (*sys_outb) (M.x86.R_DX, M.x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xef
****************************************************************************/
static void
x86emuOp_out_word_DX_AX(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("OUT\tDX,EAX\n");
    }
    else {
        DECODE_PRINTF("OUT\tDX,AX\n");
    }
    TRACE_AND_STEP();
    if (M.x86.mode & SYSMODE_PREFIX_DATA) {
        (*sys_outl) (M.x86.R_DX, M.x86.R_EAX);
    }
    else {
        (*sys_outw) (M.x86.R_DX, M.x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf0
****************************************************************************/
static void
x86emuOp_lock(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("LOCK:\n");
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/*opcode 0xf1 ILLEGAL OPERATION */

/****************************************************************************
REMARKS:
Handles opcode 0xf2
****************************************************************************/
static void
x86emuOp_repne(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("REPNE\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_PREFIX_REPNE;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf3
****************************************************************************/
static void
x86emuOp_repe(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("REPE\n");
    TRACE_AND_STEP();
    M.x86.mode |= SYSMODE_PREFIX_REPE;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf4
****************************************************************************/
static void
x86emuOp_halt(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("HALT\n");
    TRACE_AND_STEP();
    HALT_SYS();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf5
****************************************************************************/
static void
x86emuOp_cmc(u8 X86EMU_UNUSED(op1))
{
    /* complement the carry flag. */
    START_OF_INSTR();
    DECODE_PRINTF("CMC\n");
    TRACE_AND_STEP();
    TOGGLE_FLAG(F_CF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf6
****************************************************************************/
static void
x86emuOp_opcF6_byte_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    u8 *destreg;
    uint destoffset;
    u8 destval, srcval;

    /* long, drawn out code follows.  Double switch for a total
       of 32 cases.  */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:                    /* mod=00 */
        switch (rh) {
        case 0:                /* test byte imm */
            DECODE_PRINTF("TEST\tBYTE PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF(",");
            srcval = fetch_byte_imm();
            DECODE_PRINTF2("%02x\n", srcval);
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            test_byte(destval, srcval);
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=00 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            DECODE_PRINTF("NOT\tBYTE PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = not_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 3:
            DECODE_PRINTF("NEG\tBYTE PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = neg_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 4:
            DECODE_PRINTF("MUL\tBYTE PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            mul_byte(destval);
            break;
        case 5:
            DECODE_PRINTF("IMUL\tBYTE PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            imul_byte(destval);
            break;
        case 6:
            DECODE_PRINTF("DIV\tBYTE PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            div_byte(destval);
            break;
        case 7:
            DECODE_PRINTF("IDIV\tBYTE PTR ");
            destoffset = decode_rm00_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            idiv_byte(destval);
            break;
        }
        break;                  /* end mod==00 */
    case 1:                    /* mod=01 */
        switch (rh) {
        case 0:                /* test byte imm */
            DECODE_PRINTF("TEST\tBYTE PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF(",");
            srcval = fetch_byte_imm();
            DECODE_PRINTF2("%02x\n", srcval);
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            test_byte(destval, srcval);
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=01 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            DECODE_PRINTF("NOT\tBYTE PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = not_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 3:
            DECODE_PRINTF("NEG\tBYTE PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = neg_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 4:
            DECODE_PRINTF("MUL\tBYTE PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            mul_byte(destval);
            break;
        case 5:
            DECODE_PRINTF("IMUL\tBYTE PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            imul_byte(destval);
            break;
        case 6:
            DECODE_PRINTF("DIV\tBYTE PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            div_byte(destval);
            break;
        case 7:
            DECODE_PRINTF("IDIV\tBYTE PTR ");
            destoffset = decode_rm01_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            idiv_byte(destval);
            break;
        }
        break;                  /* end mod==01 */
    case 2:                    /* mod=10 */
        switch (rh) {
        case 0:                /* test byte imm */
            DECODE_PRINTF("TEST\tBYTE PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF(",");
            srcval = fetch_byte_imm();
            DECODE_PRINTF2("%02x\n", srcval);
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            test_byte(destval, srcval);
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=10 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            DECODE_PRINTF("NOT\tBYTE PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = not_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 3:
            DECODE_PRINTF("NEG\tBYTE PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = neg_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 4:
            DECODE_PRINTF("MUL\tBYTE PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            mul_byte(destval);
            break;
        case 5:
            DECODE_PRINTF("IMUL\tBYTE PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            imul_byte(destval);
            break;
        case 6:
            DECODE_PRINTF("DIV\tBYTE PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            div_byte(destval);
            break;
        case 7:
            DECODE_PRINTF("IDIV\tBYTE PTR ");
            destoffset = decode_rm10_address(rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            idiv_byte(destval);
            break;
        }
        break;                  /* end mod==10 */
    case 3:                    /* mod=11 */
        switch (rh) {
        case 0:                /* test byte imm */
            DECODE_PRINTF("TEST\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF(",");
            srcval = fetch_byte_imm();
            DECODE_PRINTF2("%02x\n", srcval);
            TRACE_AND_STEP();
            test_byte(*destreg, srcval);
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=00 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            DECODE_PRINTF("NOT\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = not_byte(*destreg);
            break;
        case 3:
            DECODE_PRINTF("NEG\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = neg_byte(*destreg);
            break;
        case 4:
            DECODE_PRINTF("MUL\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            mul_byte(*destreg); /*!!!  */
            break;
        case 5:
            DECODE_PRINTF("IMUL\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            imul_byte(*destreg);
            break;
        case 6:
            DECODE_PRINTF("DIV\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            div_byte(*destreg);
            break;
        case 7:
            DECODE_PRINTF("IDIV\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            idiv_byte(*destreg);
            break;
        }
        break;                  /* end mod==11 */
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf7
****************************************************************************/
static void
x86emuOp_opcF7_word_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset;

    /* long, drawn out code follows.  Double switch for a total
       of 32 cases.  */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    switch (mod) {
    case 0:                    /* mod=00 */
        switch (rh) {
        case 0:                /* test word imm */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval, srcval;

                DECODE_PRINTF("TEST\tDWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF(",");
                srcval = fetch_long_imm();
                DECODE_PRINTF2("%x\n", srcval);
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                test_long(destval, srcval);
            }
            else {
                u16 destval, srcval;

                DECODE_PRINTF("TEST\tWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF(",");
                srcval = fetch_word_imm();
                DECODE_PRINTF2("%x\n", srcval);
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                test_word(destval, srcval);
            }
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=00 RH=01 OP=F7\n");
            HALT_SYS();
            break;
        case 2:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("NOT\tDWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = not_long(destval);
                store_data_long(destoffset, destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("NOT\tWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = not_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 3:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("NEG\tDWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = neg_long(destval);
                store_data_long(destoffset, destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("NEG\tWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = neg_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 4:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("MUL\tDWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                mul_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("MUL\tWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                mul_word(destval);
            }
            break;
        case 5:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("IMUL\tDWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                imul_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("IMUL\tWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                imul_word(destval);
            }
            break;
        case 6:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("DIV\tDWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                div_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("DIV\tWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                div_word(destval);
            }
            break;
        case 7:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("IDIV\tDWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                idiv_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("IDIV\tWORD PTR ");
                destoffset = decode_rm00_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                idiv_word(destval);
            }
            break;
        }
        break;                  /* end mod==00 */
    case 1:                    /* mod=01 */
        switch (rh) {
        case 0:                /* test word imm */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval, srcval;

                DECODE_PRINTF("TEST\tDWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF(",");
                srcval = fetch_long_imm();
                DECODE_PRINTF2("%x\n", srcval);
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                test_long(destval, srcval);
            }
            else {
                u16 destval, srcval;

                DECODE_PRINTF("TEST\tWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF(",");
                srcval = fetch_word_imm();
                DECODE_PRINTF2("%x\n", srcval);
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                test_word(destval, srcval);
            }
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=01 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("NOT\tDWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = not_long(destval);
                store_data_long(destoffset, destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("NOT\tWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = not_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 3:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("NEG\tDWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = neg_long(destval);
                store_data_long(destoffset, destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("NEG\tWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = neg_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 4:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("MUL\tDWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                mul_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("MUL\tWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                mul_word(destval);
            }
            break;
        case 5:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("IMUL\tDWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                imul_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("IMUL\tWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                imul_word(destval);
            }
            break;
        case 6:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("DIV\tDWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                div_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("DIV\tWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                div_word(destval);
            }
            break;
        case 7:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("IDIV\tDWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                idiv_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("IDIV\tWORD PTR ");
                destoffset = decode_rm01_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                idiv_word(destval);
            }
            break;
        }
        break;                  /* end mod==01 */
    case 2:                    /* mod=10 */
        switch (rh) {
        case 0:                /* test word imm */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval, srcval;

                DECODE_PRINTF("TEST\tDWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF(",");
                srcval = fetch_long_imm();
                DECODE_PRINTF2("%x\n", srcval);
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                test_long(destval, srcval);
            }
            else {
                u16 destval, srcval;

                DECODE_PRINTF("TEST\tWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF(",");
                srcval = fetch_word_imm();
                DECODE_PRINTF2("%x\n", srcval);
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                test_word(destval, srcval);
            }
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=10 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("NOT\tDWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = not_long(destval);
                store_data_long(destoffset, destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("NOT\tWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = not_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 3:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("NEG\tDWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = neg_long(destval);
                store_data_long(destoffset, destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("NEG\tWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = neg_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 4:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("MUL\tDWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                mul_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("MUL\tWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                mul_word(destval);
            }
            break;
        case 5:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("IMUL\tDWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                imul_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("IMUL\tWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                imul_word(destval);
            }
            break;
        case 6:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("DIV\tDWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                div_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("DIV\tWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                div_word(destval);
            }
            break;
        case 7:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("IDIV\tDWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                idiv_long(destval);
            }
            else {
                u16 destval;

                DECODE_PRINTF("IDIV\tWORD PTR ");
                destoffset = decode_rm10_address(rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                idiv_word(destval);
            }
            break;
        }
        break;                  /* end mod==10 */
    case 3:                    /* mod=11 */
        switch (rh) {
        case 0:                /* test word imm */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;
                u32 srcval;

                DECODE_PRINTF("TEST\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF(",");
                srcval = fetch_long_imm();
                DECODE_PRINTF2("%x\n", srcval);
                TRACE_AND_STEP();
                test_long(*destreg, srcval);
            }
            else {
                u16 *destreg;
                u16 srcval;

                DECODE_PRINTF("TEST\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF(",");
                srcval = fetch_word_imm();
                DECODE_PRINTF2("%x\n", srcval);
                TRACE_AND_STEP();
                test_word(*destreg, srcval);
            }
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=00 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("NOT\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = not_long(*destreg);
            }
            else {
                u16 *destreg;

                DECODE_PRINTF("NOT\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = not_word(*destreg);
            }
            break;
        case 3:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("NEG\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = neg_long(*destreg);
            }
            else {
                u16 *destreg;

                DECODE_PRINTF("NEG\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = neg_word(*destreg);
            }
            break;
        case 4:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("MUL\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                mul_long(*destreg);     /*!!!  */
            }
            else {
                u16 *destreg;

                DECODE_PRINTF("MUL\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                mul_word(*destreg);     /*!!!  */
            }
            break;
        case 5:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("IMUL\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                imul_long(*destreg);
            }
            else {
                u16 *destreg;

                DECODE_PRINTF("IMUL\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                imul_word(*destreg);
            }
            break;
        case 6:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("DIV\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                div_long(*destreg);
            }
            else {
                u16 *destreg;

                DECODE_PRINTF("DIV\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                div_word(*destreg);
            }
            break;
        case 7:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("IDIV\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                idiv_long(*destreg);
            }
            else {
                u16 *destreg;

                DECODE_PRINTF("IDIV\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                idiv_word(*destreg);
            }
            break;
        }
        break;                  /* end mod==11 */
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf8
****************************************************************************/
static void
x86emuOp_clc(u8 X86EMU_UNUSED(op1))
{
    /* clear the carry flag. */
    START_OF_INSTR();
    DECODE_PRINTF("CLC\n");
    TRACE_AND_STEP();
    CLEAR_FLAG(F_CF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf9
****************************************************************************/
static void
x86emuOp_stc(u8 X86EMU_UNUSED(op1))
{
    /* set the carry flag. */
    START_OF_INSTR();
    DECODE_PRINTF("STC\n");
    TRACE_AND_STEP();
    SET_FLAG(F_CF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfa
****************************************************************************/
static void
x86emuOp_cli(u8 X86EMU_UNUSED(op1))
{
    /* clear interrupts. */
    START_OF_INSTR();
    DECODE_PRINTF("CLI\n");
    TRACE_AND_STEP();
    CLEAR_FLAG(F_IF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfb
****************************************************************************/
static void
x86emuOp_sti(u8 X86EMU_UNUSED(op1))
{
    /* enable  interrupts. */
    START_OF_INSTR();
    DECODE_PRINTF("STI\n");
    TRACE_AND_STEP();
    SET_FLAG(F_IF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfc
****************************************************************************/
static void
x86emuOp_cld(u8 X86EMU_UNUSED(op1))
{
    /* clear interrupts. */
    START_OF_INSTR();
    DECODE_PRINTF("CLD\n");
    TRACE_AND_STEP();
    CLEAR_FLAG(F_DF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfd
****************************************************************************/
static void
x86emuOp_std(u8 X86EMU_UNUSED(op1))
{
    /* clear interrupts. */
    START_OF_INSTR();
    DECODE_PRINTF("STD\n");
    TRACE_AND_STEP();
    SET_FLAG(F_DF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfe
****************************************************************************/
static void
x86emuOp_opcFE_byte_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rh, rl;
    u8 destval;
    uint destoffset;
    u8 *destreg;

    /* Yet another special case instruction. */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */

        switch (rh) {
        case 0:
            DECODE_PRINTF("INC\t");
            break;
        case 1:
            DECODE_PRINTF("DEC\t");
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            DECODE_PRINTF2("ILLEGAL OP MAJOR OP 0xFE MINOR OP %x \n", mod);
            HALT_SYS();
            break;
        }
    }
#endif
    switch (mod) {
    case 0:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:                /* inc word ptr ... */
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = inc_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 1:                /* dec word ptr ... */
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = dec_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        }
        break;
    case 1:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = inc_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 1:
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = dec_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        }
        break;
    case 2:
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = inc_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 1:
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = dec_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        }
        break;
    case 3:
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:
            TRACE_AND_STEP();
            *destreg = inc_byte(*destreg);
            break;
        case 1:
            TRACE_AND_STEP();
            *destreg = dec_byte(*destreg);
            break;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xff
****************************************************************************/
static void
x86emuOp_opcFF_word_RM(u8 X86EMU_UNUSED(op1))
{
    int mod, rh, rl;
    uint destoffset = 0;
    u16 *destreg;
    u16 destval, destval2;

    /* Yet another special case instruction. */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE()) {
        /* XXX DECODE_PRINTF may be changed to something more
           general, so that it is important to leave the strings
           in the same format, even though the result is that the
           above test is done twice. */

        switch (rh) {
        case 0:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                DECODE_PRINTF("INC\tDWORD PTR ");
            }
            else {
                DECODE_PRINTF("INC\tWORD PTR ");
            }
            break;
        case 1:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                DECODE_PRINTF("DEC\tDWORD PTR ");
            }
            else {
                DECODE_PRINTF("DEC\tWORD PTR ");
            }
            break;
        case 2:
            DECODE_PRINTF("CALL\t");
            break;
        case 3:
            DECODE_PRINTF("CALL\tFAR ");
            break;
        case 4:
            DECODE_PRINTF("JMP\t");
            break;
        case 5:
            DECODE_PRINTF("JMP\tFAR ");
            break;
        case 6:
            DECODE_PRINTF("PUSH\t");
            break;
        case 7:
            DECODE_PRINTF("ILLEGAL DECODING OF OPCODE FF\t");
            HALT_SYS();
            break;
        }
    }
#endif
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:                /* inc word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval32 = inc_long(destval32);
                store_data_long(destoffset, destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval16 = inc_word(destval16);
                store_data_word(destoffset, destval16);
            }
            break;
        case 1:                /* dec word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval32 = dec_long(destval32);
                store_data_long(destoffset, destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval16 = dec_word(destval16);
                store_data_word(destoffset, destval16);
            }
            break;
        case 2:                /* call word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                push_long(M.x86.R_EIP);
                M.x86.R_EIP = destval;
            } else {
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                push_word(M.x86.R_IP);
                M.x86.R_IP = destval;
            }
            break;
        case 3:                /* call far ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                destval = fetch_data_long(destoffset);
                destval2 = fetch_data_word(destoffset + 4);
                TRACE_AND_STEP();
                push_long(M.x86.R_CS);
                M.x86.R_CS = destval2;
                push_long(M.x86.R_EIP);
                M.x86.R_EIP = destval;
            } else {
                destval = fetch_data_word(destoffset);
                destval2 = fetch_data_word(destoffset + 2);
                TRACE_AND_STEP();
                push_word(M.x86.R_CS);
                M.x86.R_CS = destval2;
                push_word(M.x86.R_IP);
                M.x86.R_IP = destval;
            }
            break;
        case 4:                /* jmp word ptr ... */
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            M.x86.R_IP = destval;
            break;
        case 5:                /* jmp far ptr ... */
            destval = fetch_data_word(destoffset);
            destval2 = fetch_data_word(destoffset + 2);
            TRACE_AND_STEP();
            M.x86.R_IP = destval;
            M.x86.R_CS = destval2;
            break;
        case 6:                /*  push word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                push_long(destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                push_word(destval16);
            }
            break;
        }
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval32 = inc_long(destval32);
                store_data_long(destoffset, destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval16 = inc_word(destval16);
                store_data_word(destoffset, destval16);
            }
            break;
        case 1:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval32 = dec_long(destval32);
                store_data_long(destoffset, destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval16 = dec_word(destval16);
                store_data_word(destoffset, destval16);
            }
            break;
        case 2:                /* call word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                push_long(M.x86.R_EIP);
                M.x86.R_EIP = destval;
            } else {
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                push_word(M.x86.R_IP);
                M.x86.R_IP = destval;
            }
            break;
        case 3:                /* call far ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                destval = fetch_data_long(destoffset);
                destval2 = fetch_data_word(destoffset + 4);
                TRACE_AND_STEP();
                push_long(M.x86.R_CS);
                M.x86.R_CS = destval2;
                push_long(M.x86.R_EIP);
                M.x86.R_EIP = destval;
            } else {
                destval = fetch_data_word(destoffset);
                destval2 = fetch_data_word(destoffset + 2);
                TRACE_AND_STEP();
                push_word(M.x86.R_CS);
                M.x86.R_CS = destval2;
                push_word(M.x86.R_IP);
                M.x86.R_IP = destval;
            }
            break;
        case 4:                /* jmp word ptr ... */
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            M.x86.R_IP = destval;
            break;
        case 5:                /* jmp far ptr ... */
            destval = fetch_data_word(destoffset);
            destval2 = fetch_data_word(destoffset + 2);
            TRACE_AND_STEP();
            M.x86.R_IP = destval;
            M.x86.R_CS = destval2;
            break;
        case 6:                /*  push word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                push_long(destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                push_word(destval16);
            }
            break;
        }
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval32 = inc_long(destval32);
                store_data_long(destoffset, destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval16 = inc_word(destval16);
                store_data_word(destoffset, destval16);
            }
            break;
        case 1:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval32 = dec_long(destval32);
                store_data_long(destoffset, destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval16 = dec_word(destval16);
                store_data_word(destoffset, destval16);
            }
            break;
        case 2:                /* call word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                push_long(M.x86.R_EIP);
                M.x86.R_EIP = destval;
            } else {
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                push_word(M.x86.R_IP);
                M.x86.R_IP = destval;
            }
            break;
        case 3:                /* call far ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                destval = fetch_data_long(destoffset);
                destval2 = fetch_data_word(destoffset + 4);
                TRACE_AND_STEP();
                push_long(M.x86.R_CS);
                M.x86.R_CS = destval2;
                push_long(M.x86.R_EIP);
                M.x86.R_EIP = destval;
            } else {
                destval = fetch_data_word(destoffset);
                destval2 = fetch_data_word(destoffset + 2);
                TRACE_AND_STEP();
                push_word(M.x86.R_CS);
                M.x86.R_CS = destval2;
                push_word(M.x86.R_IP);
                M.x86.R_IP = destval;
            }
            break;
        case 4:                /* jmp word ptr ... */
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            M.x86.R_IP = destval;
            break;
        case 5:                /* jmp far ptr ... */
            destval = fetch_data_word(destoffset);
            destval2 = fetch_data_word(destoffset + 2);
            TRACE_AND_STEP();
            M.x86.R_IP = destval;
            M.x86.R_CS = destval2;
            break;
        case 6:                /*  push word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval32;

                destval32 = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                push_long(destval32);
            }
            else {
                u16 destval16;

                destval16 = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                push_word(destval16);
            }
            break;
        }
        break;
    case 3:
        switch (rh) {
        case 0:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg32;

                destreg32 = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg32 = inc_long(*destreg32);
            }
            else {
                u16 *destreg16;

                destreg16 = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg16 = inc_word(*destreg16);
            }
            break;
        case 1:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg32;

                destreg32 = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg32 = dec_long(*destreg32);
            }
            else {
                u16 *destreg16;

                destreg16 = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg16 = dec_word(*destreg16);
            }
            break;
        case 2:                /* call word ptr ... */
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                destreg = (u16 *)DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                push_long(M.x86.R_EIP);
                M.x86.R_EIP = *destreg;
            } else {
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                push_word(M.x86.R_IP);
                M.x86.R_IP = *destreg;
            }
            break;
        case 3:                /* jmp far ptr ... */
            DECODE_PRINTF("OPERATION UNDEFINED 0XFF \n");
            TRACE_AND_STEP();
            HALT_SYS();
            break;

        case 4:                /* jmp  ... */
            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            M.x86.R_IP = (u16) (*destreg);
            break;
        case 5:                /* jmp far ptr ... */
            DECODE_PRINTF("OPERATION UNDEFINED 0XFF \n");
            TRACE_AND_STEP();
            HALT_SYS();
            break;
        case 6:
            if (M.x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg32;

                destreg32 = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                push_long(*destreg32);
            }
            else {
                u16 *destreg16;

                destreg16 = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                push_word(*destreg16);
            }
            break;
        }
        break;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/***************************************************************************
 * Single byte operation code table:
 **************************************************************************/
void (*x86emu_optab[256]) (u8) = {
/*  0x00 */ x86emuOp_add_byte_RM_R,
/*  0x01 */ x86emuOp_add_word_RM_R,
/*  0x02 */ x86emuOp_add_byte_R_RM,
/*  0x03 */ x86emuOp_add_word_R_RM,
/*  0x04 */ x86emuOp_add_byte_AL_IMM,
/*  0x05 */ x86emuOp_add_word_AX_IMM,
/*  0x06 */ x86emuOp_push_ES,
/*  0x07 */ x86emuOp_pop_ES,
/*  0x08 */ x86emuOp_or_byte_RM_R,
/*  0x09 */ x86emuOp_or_word_RM_R,
/*  0x0a */ x86emuOp_or_byte_R_RM,
/*  0x0b */ x86emuOp_or_word_R_RM,
/*  0x0c */ x86emuOp_or_byte_AL_IMM,
/*  0x0d */ x86emuOp_or_word_AX_IMM,
/*  0x0e */ x86emuOp_push_CS,
/*  0x0f */ x86emuOp_two_byte,
/*  0x10 */ x86emuOp_adc_byte_RM_R,
/*  0x11 */ x86emuOp_adc_word_RM_R,
/*  0x12 */ x86emuOp_adc_byte_R_RM,
/*  0x13 */ x86emuOp_adc_word_R_RM,
/*  0x14 */ x86emuOp_adc_byte_AL_IMM,
/*  0x15 */ x86emuOp_adc_word_AX_IMM,
/*  0x16 */ x86emuOp_push_SS,
/*  0x17 */ x86emuOp_pop_SS,
/*  0x18 */ x86emuOp_sbb_byte_RM_R,
/*  0x19 */ x86emuOp_sbb_word_RM_R,
/*  0x1a */ x86emuOp_sbb_byte_R_RM,
/*  0x1b */ x86emuOp_sbb_word_R_RM,
/*  0x1c */ x86emuOp_sbb_byte_AL_IMM,
/*  0x1d */ x86emuOp_sbb_word_AX_IMM,
/*  0x1e */ x86emuOp_push_DS,
/*  0x1f */ x86emuOp_pop_DS,
/*  0x20 */ x86emuOp_and_byte_RM_R,
/*  0x21 */ x86emuOp_and_word_RM_R,
/*  0x22 */ x86emuOp_and_byte_R_RM,
/*  0x23 */ x86emuOp_and_word_R_RM,
/*  0x24 */ x86emuOp_and_byte_AL_IMM,
/*  0x25 */ x86emuOp_and_word_AX_IMM,
/*  0x26 */ x86emuOp_segovr_ES,
/*  0x27 */ x86emuOp_daa,
/*  0x28 */ x86emuOp_sub_byte_RM_R,
/*  0x29 */ x86emuOp_sub_word_RM_R,
/*  0x2a */ x86emuOp_sub_byte_R_RM,
/*  0x2b */ x86emuOp_sub_word_R_RM,
/*  0x2c */ x86emuOp_sub_byte_AL_IMM,
/*  0x2d */ x86emuOp_sub_word_AX_IMM,
/*  0x2e */ x86emuOp_segovr_CS,
/*  0x2f */ x86emuOp_das,
/*  0x30 */ x86emuOp_xor_byte_RM_R,
/*  0x31 */ x86emuOp_xor_word_RM_R,
/*  0x32 */ x86emuOp_xor_byte_R_RM,
/*  0x33 */ x86emuOp_xor_word_R_RM,
/*  0x34 */ x86emuOp_xor_byte_AL_IMM,
/*  0x35 */ x86emuOp_xor_word_AX_IMM,
/*  0x36 */ x86emuOp_segovr_SS,
/*  0x37 */ x86emuOp_aaa,
/*  0x38 */ x86emuOp_cmp_byte_RM_R,
/*  0x39 */ x86emuOp_cmp_word_RM_R,
/*  0x3a */ x86emuOp_cmp_byte_R_RM,
/*  0x3b */ x86emuOp_cmp_word_R_RM,
/*  0x3c */ x86emuOp_cmp_byte_AL_IMM,
/*  0x3d */ x86emuOp_cmp_word_AX_IMM,
/*  0x3e */ x86emuOp_segovr_DS,
/*  0x3f */ x86emuOp_aas,
/*  0x40 */ x86emuOp_inc_AX,
/*  0x41 */ x86emuOp_inc_CX,
/*  0x42 */ x86emuOp_inc_DX,
/*  0x43 */ x86emuOp_inc_BX,
/*  0x44 */ x86emuOp_inc_SP,
/*  0x45 */ x86emuOp_inc_BP,
/*  0x46 */ x86emuOp_inc_SI,
/*  0x47 */ x86emuOp_inc_DI,
/*  0x48 */ x86emuOp_dec_AX,
/*  0x49 */ x86emuOp_dec_CX,
/*  0x4a */ x86emuOp_dec_DX,
/*  0x4b */ x86emuOp_dec_BX,
/*  0x4c */ x86emuOp_dec_SP,
/*  0x4d */ x86emuOp_dec_BP,
/*  0x4e */ x86emuOp_dec_SI,
/*  0x4f */ x86emuOp_dec_DI,
/*  0x50 */ x86emuOp_push_AX,
/*  0x51 */ x86emuOp_push_CX,
/*  0x52 */ x86emuOp_push_DX,
/*  0x53 */ x86emuOp_push_BX,
/*  0x54 */ x86emuOp_push_SP,
/*  0x55 */ x86emuOp_push_BP,
/*  0x56 */ x86emuOp_push_SI,
/*  0x57 */ x86emuOp_push_DI,
/*  0x58 */ x86emuOp_pop_AX,
/*  0x59 */ x86emuOp_pop_CX,
/*  0x5a */ x86emuOp_pop_DX,
/*  0x5b */ x86emuOp_pop_BX,
/*  0x5c */ x86emuOp_pop_SP,
/*  0x5d */ x86emuOp_pop_BP,
/*  0x5e */ x86emuOp_pop_SI,
/*  0x5f */ x86emuOp_pop_DI,
/*  0x60 */ x86emuOp_push_all,
/*  0x61 */ x86emuOp_pop_all,
                                                /*  0x62 */ x86emuOp_illegal_op,
                                                /* bound */
                                                /*  0x63 */ x86emuOp_illegal_op,
                                                /* arpl */
/*  0x64 */ x86emuOp_segovr_FS,
/*  0x65 */ x86emuOp_segovr_GS,
/*  0x66 */ x86emuOp_prefix_data,
/*  0x67 */ x86emuOp_prefix_addr,
/*  0x68 */ x86emuOp_push_word_IMM,
/*  0x69 */ x86emuOp_imul_word_IMM,
/*  0x6a */ x86emuOp_push_byte_IMM,
/*  0x6b */ x86emuOp_imul_byte_IMM,
/*  0x6c */ x86emuOp_ins_byte,
/*  0x6d */ x86emuOp_ins_word,
/*  0x6e */ x86emuOp_outs_byte,
/*  0x6f */ x86emuOp_outs_word,
/*  0x70 */ x86emuOp_jump_near_O,
/*  0x71 */ x86emuOp_jump_near_NO,
/*  0x72 */ x86emuOp_jump_near_B,
/*  0x73 */ x86emuOp_jump_near_NB,
/*  0x74 */ x86emuOp_jump_near_Z,
/*  0x75 */ x86emuOp_jump_near_NZ,
/*  0x76 */ x86emuOp_jump_near_BE,
/*  0x77 */ x86emuOp_jump_near_NBE,
/*  0x78 */ x86emuOp_jump_near_S,
/*  0x79 */ x86emuOp_jump_near_NS,
/*  0x7a */ x86emuOp_jump_near_P,
/*  0x7b */ x86emuOp_jump_near_NP,
/*  0x7c */ x86emuOp_jump_near_L,
/*  0x7d */ x86emuOp_jump_near_NL,
/*  0x7e */ x86emuOp_jump_near_LE,
/*  0x7f */ x86emuOp_jump_near_NLE,
/*  0x80 */ x86emuOp_opc80_byte_RM_IMM,
/*  0x81 */ x86emuOp_opc81_word_RM_IMM,
/*  0x82 */ x86emuOp_opc82_byte_RM_IMM,
/*  0x83 */ x86emuOp_opc83_word_RM_IMM,
/*  0x84 */ x86emuOp_test_byte_RM_R,
/*  0x85 */ x86emuOp_test_word_RM_R,
/*  0x86 */ x86emuOp_xchg_byte_RM_R,
/*  0x87 */ x86emuOp_xchg_word_RM_R,
/*  0x88 */ x86emuOp_mov_byte_RM_R,
/*  0x89 */ x86emuOp_mov_word_RM_R,
/*  0x8a */ x86emuOp_mov_byte_R_RM,
/*  0x8b */ x86emuOp_mov_word_R_RM,
/*  0x8c */ x86emuOp_mov_word_RM_SR,
/*  0x8d */ x86emuOp_lea_word_R_M,
/*  0x8e */ x86emuOp_mov_word_SR_RM,
/*  0x8f */ x86emuOp_pop_RM,
/*  0x90 */ x86emuOp_nop,
/*  0x91 */ x86emuOp_xchg_word_AX_CX,
/*  0x92 */ x86emuOp_xchg_word_AX_DX,
/*  0x93 */ x86emuOp_xchg_word_AX_BX,
/*  0x94 */ x86emuOp_xchg_word_AX_SP,
/*  0x95 */ x86emuOp_xchg_word_AX_BP,
/*  0x96 */ x86emuOp_xchg_word_AX_SI,
/*  0x97 */ x86emuOp_xchg_word_AX_DI,
/*  0x98 */ x86emuOp_cbw,
/*  0x99 */ x86emuOp_cwd,
/*  0x9a */ x86emuOp_call_far_IMM,
/*  0x9b */ x86emuOp_wait,
/*  0x9c */ x86emuOp_pushf_word,
/*  0x9d */ x86emuOp_popf_word,
/*  0x9e */ x86emuOp_sahf,
/*  0x9f */ x86emuOp_lahf,
/*  0xa0 */ x86emuOp_mov_AL_M_IMM,
/*  0xa1 */ x86emuOp_mov_AX_M_IMM,
/*  0xa2 */ x86emuOp_mov_M_AL_IMM,
/*  0xa3 */ x86emuOp_mov_M_AX_IMM,
/*  0xa4 */ x86emuOp_movs_byte,
/*  0xa5 */ x86emuOp_movs_word,
/*  0xa6 */ x86emuOp_cmps_byte,
/*  0xa7 */ x86emuOp_cmps_word,
/*  0xa8 */ x86emuOp_test_AL_IMM,
/*  0xa9 */ x86emuOp_test_AX_IMM,
/*  0xaa */ x86emuOp_stos_byte,
/*  0xab */ x86emuOp_stos_word,
/*  0xac */ x86emuOp_lods_byte,
/*  0xad */ x86emuOp_lods_word,
/*  0xac */ x86emuOp_scas_byte,
/*  0xad */ x86emuOp_scas_word,
/*  0xb0 */ x86emuOp_mov_byte_AL_IMM,
/*  0xb1 */ x86emuOp_mov_byte_CL_IMM,
/*  0xb2 */ x86emuOp_mov_byte_DL_IMM,
/*  0xb3 */ x86emuOp_mov_byte_BL_IMM,
/*  0xb4 */ x86emuOp_mov_byte_AH_IMM,
/*  0xb5 */ x86emuOp_mov_byte_CH_IMM,
/*  0xb6 */ x86emuOp_mov_byte_DH_IMM,
/*  0xb7 */ x86emuOp_mov_byte_BH_IMM,
/*  0xb8 */ x86emuOp_mov_word_AX_IMM,
/*  0xb9 */ x86emuOp_mov_word_CX_IMM,
/*  0xba */ x86emuOp_mov_word_DX_IMM,
/*  0xbb */ x86emuOp_mov_word_BX_IMM,
/*  0xbc */ x86emuOp_mov_word_SP_IMM,
/*  0xbd */ x86emuOp_mov_word_BP_IMM,
/*  0xbe */ x86emuOp_mov_word_SI_IMM,
/*  0xbf */ x86emuOp_mov_word_DI_IMM,
/*  0xc0 */ x86emuOp_opcC0_byte_RM_MEM,
/*  0xc1 */ x86emuOp_opcC1_word_RM_MEM,
/*  0xc2 */ x86emuOp_ret_near_IMM,
/*  0xc3 */ x86emuOp_ret_near,
/*  0xc4 */ x86emuOp_les_R_IMM,
/*  0xc5 */ x86emuOp_lds_R_IMM,
/*  0xc6 */ x86emuOp_mov_byte_RM_IMM,
/*  0xc7 */ x86emuOp_mov_word_RM_IMM,
/*  0xc8 */ x86emuOp_enter,
/*  0xc9 */ x86emuOp_leave,
/*  0xca */ x86emuOp_ret_far_IMM,
/*  0xcb */ x86emuOp_ret_far,
/*  0xcc */ x86emuOp_int3,
/*  0xcd */ x86emuOp_int_IMM,
/*  0xce */ x86emuOp_into,
/*  0xcf */ x86emuOp_iret,
/*  0xd0 */ x86emuOp_opcD0_byte_RM_1,
/*  0xd1 */ x86emuOp_opcD1_word_RM_1,
/*  0xd2 */ x86emuOp_opcD2_byte_RM_CL,
/*  0xd3 */ x86emuOp_opcD3_word_RM_CL,
/*  0xd4 */ x86emuOp_aam,
/*  0xd5 */ x86emuOp_aad,
                                                /*  0xd6 */ x86emuOp_illegal_op,
                                                /* Undocumented SETALC instruction */
/*  0xd7 */ x86emuOp_xlat,
/*  0xd8 */ x86emuOp_esc_coprocess_d8,
/*  0xd9 */ x86emuOp_esc_coprocess_d9,
/*  0xda */ x86emuOp_esc_coprocess_da,
/*  0xdb */ x86emuOp_esc_coprocess_db,
/*  0xdc */ x86emuOp_esc_coprocess_dc,
/*  0xdd */ x86emuOp_esc_coprocess_dd,
/*  0xde */ x86emuOp_esc_coprocess_de,
/*  0xdf */ x86emuOp_esc_coprocess_df,
/*  0xe0 */ x86emuOp_loopne,
/*  0xe1 */ x86emuOp_loope,
/*  0xe2 */ x86emuOp_loop,
/*  0xe3 */ x86emuOp_jcxz,
/*  0xe4 */ x86emuOp_in_byte_AL_IMM,
/*  0xe5 */ x86emuOp_in_word_AX_IMM,
/*  0xe6 */ x86emuOp_out_byte_IMM_AL,
/*  0xe7 */ x86emuOp_out_word_IMM_AX,
/*  0xe8 */ x86emuOp_call_near_IMM,
/*  0xe9 */ x86emuOp_jump_near_IMM,
/*  0xea */ x86emuOp_jump_far_IMM,
/*  0xeb */ x86emuOp_jump_byte_IMM,
/*  0xec */ x86emuOp_in_byte_AL_DX,
/*  0xed */ x86emuOp_in_word_AX_DX,
/*  0xee */ x86emuOp_out_byte_DX_AL,
/*  0xef */ x86emuOp_out_word_DX_AX,
/*  0xf0 */ x86emuOp_lock,
/*  0xf1 */ x86emuOp_illegal_op,
/*  0xf2 */ x86emuOp_repne,
/*  0xf3 */ x86emuOp_repe,
/*  0xf4 */ x86emuOp_halt,
/*  0xf5 */ x86emuOp_cmc,
/*  0xf6 */ x86emuOp_opcF6_byte_RM,
/*  0xf7 */ x86emuOp_opcF7_word_RM,
/*  0xf8 */ x86emuOp_clc,
/*  0xf9 */ x86emuOp_stc,
/*  0xfa */ x86emuOp_cli,
/*  0xfb */ x86emuOp_sti,
/*  0xfc */ x86emuOp_cld,
/*  0xfd */ x86emuOp_std,
/*  0xfe */ x86emuOp_opcFE_byte_RM,
/*  0xff */ x86emuOp_opcFF_word_RM,
};
