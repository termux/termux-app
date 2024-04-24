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
* Description:  This file includes subroutines which are related to
*				instruction decoding and accessess of immediate data via IP.  etc.
*
****************************************************************************/

#include <stdlib.h>

#if defined(__sun) && defined(CS) /* avoid conflicts with Solaris sys/regset.h */
# undef CS
# undef DS
# undef SS
# undef ES
# undef FS
# undef GS
#endif

#include "x86emu/x86emui.h"

/*----------------------------- Implementation ----------------------------*/

/****************************************************************************
REMARKS:
Handles any pending asychronous interrupts.
****************************************************************************/
static void
x86emu_intr_handle(void)
{
    u8 intno;

    if (M.x86.intr & INTR_SYNCH) {
        intno = M.x86.intno;
        if (_X86EMU_intrTab[intno]) {
            (*_X86EMU_intrTab[intno]) (intno);
        }
        else {
            push_word((u16) M.x86.R_FLG);
            CLEAR_FLAG(F_IF);
            CLEAR_FLAG(F_TF);
            push_word(M.x86.R_CS);
            M.x86.R_CS = mem_access_word(intno * 4 + 2);
            push_word(M.x86.R_IP);
            M.x86.R_IP = mem_access_word(intno * 4);
            M.x86.intr = 0;
        }
    }
}

/****************************************************************************
PARAMETERS:
intrnum - Interrupt number to raise

REMARKS:
Raise the specified interrupt to be handled before the execution of the
next instruction.
****************************************************************************/
void
x86emu_intr_raise(u8 intrnum)
{
    M.x86.intno = intrnum;
    M.x86.intr |= INTR_SYNCH;
}

/****************************************************************************
REMARKS:
Main execution loop for the emulator. We return from here when the system
halts, which is normally caused by a stack fault when we return from the
original real mode call.
****************************************************************************/
void
X86EMU_exec(void)
{
    u8 op1;

    M.x86.intr = 0;
    DB(x86emu_end_instr();
        )

        for (;;) {
        DB(if (CHECK_IP_FETCH())
           x86emu_check_ip_access();)
            /* If debugging, save the IP and CS values. */
            SAVE_IP_CS(M.x86.R_CS, M.x86.R_IP);
        INC_DECODED_INST_LEN(1);
        if (M.x86.intr) {
            if (M.x86.intr & INTR_HALTED) {
                DB(if (M.x86.R_SP != 0) {
                   printk("halted\n"); X86EMU_trace_regs();}
                   else {
                   if (M.x86.debug)
                   printk("Service completed successfully\n");}
                )
                    return;
            }
            if (((M.x86.intr & INTR_SYNCH) &&
                 (M.x86.intno == 0 || M.x86.intno == 2)) ||
                !ACCESS_FLAG(F_IF)) {
                x86emu_intr_handle();
            }
        }
        op1 = (*sys_rdb) (((u32) M.x86.R_CS << 4) + (M.x86.R_IP++));
        (*x86emu_optab[op1]) (op1);
        if (M.x86.debug & DEBUG_EXIT) {
            M.x86.debug &= ~DEBUG_EXIT;
            return;
        }
    }
}

/****************************************************************************
REMARKS:
Halts the system by setting the halted system flag.
****************************************************************************/
void
X86EMU_halt_sys(void)
{
    M.x86.intr |= INTR_HALTED;
}

/****************************************************************************
PARAMETERS:
mod		- Mod value from decoded byte
regh	- Reg h value from decoded byte
regl	- Reg l value from decoded byte

REMARKS:
Raise the specified interrupt to be handled before the execution of the
next instruction.

NOTE: Do not inline this function, as (*sys_rdb) is already inline!
****************************************************************************/
void
fetch_decode_modrm(int *mod, int *regh, int *regl)
{
    int fetched;

    DB(if (CHECK_IP_FETCH())
       x86emu_check_ip_access();)
        fetched = (*sys_rdb) (((u32) M.x86.R_CS << 4) + (M.x86.R_IP++));
    INC_DECODED_INST_LEN(1);
    *mod = (fetched >> 6) & 0x03;
    *regh = (fetched >> 3) & 0x07;
    *regl = (fetched >> 0) & 0x07;
}

/****************************************************************************
RETURNS:
Immediate byte value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.

NOTE: Do not inline this function, as (*sys_rdb) is already inline!
****************************************************************************/
u8
fetch_byte_imm(void)
{
    u8 fetched;

    DB(if (CHECK_IP_FETCH())
       x86emu_check_ip_access();)
        fetched = (*sys_rdb) (((u32) M.x86.R_CS << 4) + (M.x86.R_IP++));
    INC_DECODED_INST_LEN(1);
    return fetched;
}

/****************************************************************************
RETURNS:
Immediate word value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.

NOTE: Do not inline this function, as (*sys_rdw) is already inline!
****************************************************************************/
u16
fetch_word_imm(void)
{
    u16 fetched;

    DB(if (CHECK_IP_FETCH())
       x86emu_check_ip_access();)
        fetched = (*sys_rdw) (((u32) M.x86.R_CS << 4) + (M.x86.R_IP));
    M.x86.R_IP += 2;
    INC_DECODED_INST_LEN(2);
    return fetched;
}

/****************************************************************************
RETURNS:
Immediate lone value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.

NOTE: Do not inline this function, as (*sys_rdw) is already inline!
****************************************************************************/
u32
fetch_long_imm(void)
{
    u32 fetched;

    DB(if (CHECK_IP_FETCH())
       x86emu_check_ip_access();)
        fetched = (*sys_rdl) (((u32) M.x86.R_CS << 4) + (M.x86.R_IP));
    M.x86.R_IP += 4;
    INC_DECODED_INST_LEN(4);
    return fetched;
}

/****************************************************************************
RETURNS:
Value of the default data segment

REMARKS:
Inline function that returns the default data segment for the current
instruction.

On the x86 processor, the default segment is not always DS if there is
no segment override. Address modes such as -3[BP] or 10[BP+SI] all refer to
addresses relative to SS (ie: on the stack). So, at the minimum, all
decodings of addressing modes would have to set/clear a bit describing
whether the access is relative to DS or SS.  That is the function of the
cpu-state-varible M.x86.mode. There are several potential states:

	repe prefix seen  (handled elsewhere)
	repne prefix seen  (ditto)

	cs segment override
	ds segment override
	es segment override
	fs segment override
	gs segment override
	ss segment override

	ds/ss select (in absence of override)

Each of the above 7 items are handled with a bit in the mode field.
****************************************************************************/
_INLINE u32
get_data_segment(void)
{
#define	GET_SEGMENT(segment)
    switch (M.x86.mode & SYSMODE_SEGMASK) {
    case 0:                    /* default case: use ds register */
    case SYSMODE_SEGOVR_DS:
    case SYSMODE_SEGOVR_DS | SYSMODE_SEG_DS_SS:
        return M.x86.R_DS;
    case SYSMODE_SEG_DS_SS:    /* non-overridden, use ss register */
        return M.x86.R_SS;
    case SYSMODE_SEGOVR_CS:
    case SYSMODE_SEGOVR_CS | SYSMODE_SEG_DS_SS:
        return M.x86.R_CS;
    case SYSMODE_SEGOVR_ES:
    case SYSMODE_SEGOVR_ES | SYSMODE_SEG_DS_SS:
        return M.x86.R_ES;
    case SYSMODE_SEGOVR_FS:
    case SYSMODE_SEGOVR_FS | SYSMODE_SEG_DS_SS:
        return M.x86.R_FS;
    case SYSMODE_SEGOVR_GS:
    case SYSMODE_SEGOVR_GS | SYSMODE_SEG_DS_SS:
        return M.x86.R_GS;
    case SYSMODE_SEGOVR_SS:
    case SYSMODE_SEGOVR_SS | SYSMODE_SEG_DS_SS:
        return M.x86.R_SS;
    default:
#ifdef	DEBUG
        printk("error: should not happen:  multiple overrides.\n");
#endif
        HALT_SYS();
        return 0;
    }
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Byte value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u8
fetch_data_byte(uint offset)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access((u16) get_data_segment(), offset);
#endif
    return (*sys_rdb) ((get_data_segment() << 4) + offset);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Word value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u16
fetch_data_word(uint offset)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access((u16) get_data_segment(), offset);
#endif
    return (*sys_rdw) ((get_data_segment() << 4) + offset);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Long value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u32
fetch_data_long(uint offset)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access((u16) get_data_segment(), offset);
#endif
    return (*sys_rdl) ((get_data_segment() << 4) + offset);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Byte value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u8
fetch_data_byte_abs(uint segment, uint offset)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access(segment, offset);
#endif
    return (*sys_rdb) (((u32) segment << 4) + offset);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Word value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u16
fetch_data_word_abs(uint segment, uint offset)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access(segment, offset);
#endif
    return (*sys_rdw) (((u32) segment << 4) + offset);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Long value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u32
fetch_data_long_abs(uint segment, uint offset)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access(segment, offset);
#endif
    return (*sys_rdl) (((u32) segment << 4) + offset);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void
store_data_byte(uint offset, u8 val)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access((u16) get_data_segment(), offset);
#endif
    (*sys_wrb) ((get_data_segment() << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void
store_data_word(uint offset, u16 val)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access((u16) get_data_segment(), offset);
#endif
    (*sys_wrw) ((get_data_segment() << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a long value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void
store_data_long(uint offset, u32 val)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access((u16) get_data_segment(), offset);
#endif
    (*sys_wrl) ((get_data_segment() << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a byte value to an absolute memory location.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void
store_data_byte_abs(uint segment, uint offset, u8 val)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access(segment, offset);
#endif
    (*sys_wrb) (((u32) segment << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an absolute memory location.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void
store_data_word_abs(uint segment, uint offset, u16 val)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access(segment, offset);
#endif
    (*sys_wrw) (((u32) segment << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a long value to an absolute memory location.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void
store_data_long_abs(uint segment, uint offset, u32 val)
{
#ifdef DEBUG
    if (CHECK_DATA_ACCESS())
        x86emu_check_data_access(segment, offset);
#endif
    (*sys_wrl) (((u32) segment << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for byte operands. Also enables the decoding of instructions.
****************************************************************************/
u8 *
decode_rm_byte_register(int reg)
{
    switch (reg) {
    case 0:
        DECODE_PRINTF("AL");
        return &M.x86.R_AL;
    case 1:
        DECODE_PRINTF("CL");
        return &M.x86.R_CL;
    case 2:
        DECODE_PRINTF("DL");
        return &M.x86.R_DL;
    case 3:
        DECODE_PRINTF("BL");
        return &M.x86.R_BL;
    case 4:
        DECODE_PRINTF("AH");
        return &M.x86.R_AH;
    case 5:
        DECODE_PRINTF("CH");
        return &M.x86.R_CH;
    case 6:
        DECODE_PRINTF("DH");
        return &M.x86.R_DH;
    case 7:
        DECODE_PRINTF("BH");
        return &M.x86.R_BH;
    }
    HALT_SYS();
    return NULL;                /* NOT REACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for word operands.  Also enables the decoding of instructions.
****************************************************************************/
u16 *
decode_rm_word_register(int reg)
{
    switch (reg) {
    case 0:
        DECODE_PRINTF("AX");
        return &M.x86.R_AX;
    case 1:
        DECODE_PRINTF("CX");
        return &M.x86.R_CX;
    case 2:
        DECODE_PRINTF("DX");
        return &M.x86.R_DX;
    case 3:
        DECODE_PRINTF("BX");
        return &M.x86.R_BX;
    case 4:
        DECODE_PRINTF("SP");
        return &M.x86.R_SP;
    case 5:
        DECODE_PRINTF("BP");
        return &M.x86.R_BP;
    case 6:
        DECODE_PRINTF("SI");
        return &M.x86.R_SI;
    case 7:
        DECODE_PRINTF("DI");
        return &M.x86.R_DI;
    }
    HALT_SYS();
    return NULL;                /* NOTREACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for dword operands.  Also enables the decoding of instructions.
****************************************************************************/
u32 *
decode_rm_long_register(int reg)
{
    switch (reg) {
    case 0:
        DECODE_PRINTF("EAX");
        return &M.x86.R_EAX;
    case 1:
        DECODE_PRINTF("ECX");
        return &M.x86.R_ECX;
    case 2:
        DECODE_PRINTF("EDX");
        return &M.x86.R_EDX;
    case 3:
        DECODE_PRINTF("EBX");
        return &M.x86.R_EBX;
    case 4:
        DECODE_PRINTF("ESP");
        return &M.x86.R_ESP;
    case 5:
        DECODE_PRINTF("EBP");
        return &M.x86.R_EBP;
    case 6:
        DECODE_PRINTF("ESI");
        return &M.x86.R_ESI;
    case 7:
        DECODE_PRINTF("EDI");
        return &M.x86.R_EDI;
    }
    HALT_SYS();
    return NULL;                /* NOTREACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for word operands, modified from above for the weirdo
special case of segreg operands.  Also enables the decoding of instructions.
****************************************************************************/
u16 *
decode_rm_seg_register(int reg)
{
    switch (reg) {
    case 0:
        DECODE_PRINTF("ES");
        return &M.x86.R_ES;
    case 1:
        DECODE_PRINTF("CS");
        return &M.x86.R_CS;
    case 2:
        DECODE_PRINTF("SS");
        return &M.x86.R_SS;
    case 3:
        DECODE_PRINTF("DS");
        return &M.x86.R_DS;
    case 4:
        DECODE_PRINTF("FS");
        return &M.x86.R_FS;
    case 5:
        DECODE_PRINTF("GS");
        return &M.x86.R_GS;
    case 6:
    case 7:
        DECODE_PRINTF("ILLEGAL SEGREG");
        break;
    }
    HALT_SYS();
    return NULL;                /* NOT REACHED OR REACHED ON ERROR */
}

/*
 *
 * return offset from the SIB Byte
 */
u32
decode_sib_address(int sib, int mod)
{
    u32 base = 0, i = 0, scale = 1;

    switch (sib & 0x07) {
    case 0:
        DECODE_PRINTF("[EAX]");
        base = M.x86.R_EAX;
        break;
    case 1:
        DECODE_PRINTF("[ECX]");
        base = M.x86.R_ECX;
        break;
    case 2:
        DECODE_PRINTF("[EDX]");
        base = M.x86.R_EDX;
        break;
    case 3:
        DECODE_PRINTF("[EBX]");
        base = M.x86.R_EBX;
        break;
    case 4:
        DECODE_PRINTF("[ESP]");
        base = M.x86.R_ESP;
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        break;
    case 5:
        if (mod == 0) {
            base = fetch_long_imm();
            DECODE_PRINTF2("%08x", base);
        }
        else {
            DECODE_PRINTF("[EBP]");
            base = M.x86.R_ESP;
            M.x86.mode |= SYSMODE_SEG_DS_SS;
        }
        break;
    case 6:
        DECODE_PRINTF("[ESI]");
        base = M.x86.R_ESI;
        break;
    case 7:
        DECODE_PRINTF("[EDI]");
        base = M.x86.R_EDI;
        break;
    }
    switch ((sib >> 3) & 0x07) {
    case 0:
        DECODE_PRINTF("[EAX");
        i = M.x86.R_EAX;
        break;
    case 1:
        DECODE_PRINTF("[ECX");
        i = M.x86.R_ECX;
        break;
    case 2:
        DECODE_PRINTF("[EDX");
        i = M.x86.R_EDX;
        break;
    case 3:
        DECODE_PRINTF("[EBX");
        i = M.x86.R_EBX;
        break;
    case 4:
        i = 0;
        break;
    case 5:
        DECODE_PRINTF("[EBP");
        i = M.x86.R_EBP;
        break;
    case 6:
        DECODE_PRINTF("[ESI");
        i = M.x86.R_ESI;
        break;
    case 7:
        DECODE_PRINTF("[EDI");
        i = M.x86.R_EDI;
        break;
    }
    scale = 1 << ((sib >> 6) & 0x03);
    if (((sib >> 3) & 0x07) != 4) {
        if (scale == 1) {
            DECODE_PRINTF("]");
        }
        else {
            DECODE_PRINTF2("*%d]", scale);
        }
    }
    return base + (i * scale);
}

/****************************************************************************
PARAMETERS:
rm	- RM value to decode

RETURNS:
Offset in memory for the address decoding

REMARKS:
Return the offset given by mod=00 addressing.  Also enables the
decoding of instructions.

NOTE: 	The code which specifies the corresponding segment (ds vs ss)
		below in the case of [BP+..].  The assumption here is that at the
		point that this subroutine is called, the bit corresponding to
		SYSMODE_SEG_DS_SS will be zero.  After every instruction
		except the segment override instructions, this bit (as well
		as any bits indicating segment overrides) will be clear.  So
		if a SS access is needed, set this bit.  Otherwise, DS access
		occurs (unless any of the segment override bits are set).
****************************************************************************/
u32
decode_rm00_address(int rm)
{
    u32 offset;
    int sib;

    if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
        /* 32-bit addressing */
        switch (rm) {
        case 0:
            DECODE_PRINTF("[EAX]");
            return M.x86.R_EAX;
        case 1:
            DECODE_PRINTF("[ECX]");
            return M.x86.R_ECX;
        case 2:
            DECODE_PRINTF("[EDX]");
            return M.x86.R_EDX;
        case 3:
            DECODE_PRINTF("[EBX]");
            return M.x86.R_EBX;
        case 4:
            sib = fetch_byte_imm();
            return decode_sib_address(sib, 0);
        case 5:
            offset = fetch_long_imm();
            DECODE_PRINTF2("[%08x]", offset);
            return offset;
        case 6:
            DECODE_PRINTF("[ESI]");
            return M.x86.R_ESI;
        case 7:
            DECODE_PRINTF("[EDI]");
            return M.x86.R_EDI;
        }
        HALT_SYS();
    }
    else {
        /* 16-bit addressing */
        switch (rm) {
        case 0:
            DECODE_PRINTF("[BX+SI]");
            return (M.x86.R_BX + M.x86.R_SI) & 0xffff;
        case 1:
            DECODE_PRINTF("[BX+DI]");
            return (M.x86.R_BX + M.x86.R_DI) & 0xffff;
        case 2:
            DECODE_PRINTF("[BP+SI]");
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_SI) & 0xffff;
        case 3:
            DECODE_PRINTF("[BP+DI]");
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_DI) & 0xffff;
        case 4:
            DECODE_PRINTF("[SI]");
            return M.x86.R_SI;
        case 5:
            DECODE_PRINTF("[DI]");
            return M.x86.R_DI;
        case 6:
            offset = fetch_word_imm();
            DECODE_PRINTF2("[%04x]", offset);
            return offset;
        case 7:
            DECODE_PRINTF("[BX]");
            return M.x86.R_BX;
        }
        HALT_SYS();
    }
    return 0;
}

/****************************************************************************
PARAMETERS:
rm	- RM value to decode

RETURNS:
Offset in memory for the address decoding

REMARKS:
Return the offset given by mod=01 addressing.  Also enables the
decoding of instructions.
****************************************************************************/
u32
decode_rm01_address(int rm)
{
    int displacement = 0;
    int sib;

    /* Fetch disp8 if no SIB byte */
    if (!((M.x86.mode & SYSMODE_PREFIX_ADDR) && (rm == 4)))
        displacement = (s8) fetch_byte_imm();

    if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
        /* 32-bit addressing */
        switch (rm) {
        case 0:
            DECODE_PRINTF2("%d[EAX]", displacement);
            return M.x86.R_EAX + displacement;
        case 1:
            DECODE_PRINTF2("%d[ECX]", displacement);
            return M.x86.R_ECX + displacement;
        case 2:
            DECODE_PRINTF2("%d[EDX]", displacement);
            return M.x86.R_EDX + displacement;
        case 3:
            DECODE_PRINTF2("%d[EBX]", displacement);
            return M.x86.R_EBX + displacement;
        case 4:
            sib = fetch_byte_imm();
            displacement = (s8) fetch_byte_imm();
            DECODE_PRINTF2("%d", displacement);
            return decode_sib_address(sib, 1) + displacement;
        case 5:
            DECODE_PRINTF2("%d[EBP]", displacement);
            return M.x86.R_EBP + displacement;
        case 6:
            DECODE_PRINTF2("%d[ESI]", displacement);
            return M.x86.R_ESI + displacement;
        case 7:
            DECODE_PRINTF2("%d[EDI]", displacement);
            return M.x86.R_EDI + displacement;
        }
        HALT_SYS();
    }
    else {
        /* 16-bit addressing */
        switch (rm) {
        case 0:
            DECODE_PRINTF2("%d[BX+SI]", displacement);
            return (M.x86.R_BX + M.x86.R_SI + displacement) & 0xffff;
        case 1:
            DECODE_PRINTF2("%d[BX+DI]", displacement);
            return (M.x86.R_BX + M.x86.R_DI + displacement) & 0xffff;
        case 2:
            DECODE_PRINTF2("%d[BP+SI]", displacement);
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_SI + displacement) & 0xffff;
        case 3:
            DECODE_PRINTF2("%d[BP+DI]", displacement);
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_DI + displacement) & 0xffff;
        case 4:
            DECODE_PRINTF2("%d[SI]", displacement);
            return (M.x86.R_SI + displacement) & 0xffff;
        case 5:
            DECODE_PRINTF2("%d[DI]", displacement);
            return (M.x86.R_DI + displacement) & 0xffff;
        case 6:
            DECODE_PRINTF2("%d[BP]", displacement);
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + displacement) & 0xffff;
        case 7:
            DECODE_PRINTF2("%d[BX]", displacement);
            return (M.x86.R_BX + displacement) & 0xffff;
        }
        HALT_SYS();
    }
    return 0;                   /* SHOULD NOT HAPPEN */
}

/****************************************************************************
PARAMETERS:
rm	- RM value to decode

RETURNS:
Offset in memory for the address decoding

REMARKS:
Return the offset given by mod=10 addressing.  Also enables the
decoding of instructions.
****************************************************************************/
u32
decode_rm10_address(int rm)
{
    u32 displacement = 0;
    int sib;

    /* Fetch disp16 if 16-bit addr mode */
    if (!(M.x86.mode & SYSMODE_PREFIX_ADDR))
        displacement = (u16) fetch_word_imm();
    else {
        /* Fetch disp32 if no SIB byte */
        if (rm != 4)
            displacement = (u32) fetch_long_imm();
    }

    if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
        /* 32-bit addressing */
        switch (rm) {
        case 0:
            DECODE_PRINTF2("%08x[EAX]", displacement);
            return M.x86.R_EAX + displacement;
        case 1:
            DECODE_PRINTF2("%08x[ECX]", displacement);
            return M.x86.R_ECX + displacement;
        case 2:
            DECODE_PRINTF2("%08x[EDX]", displacement);
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return M.x86.R_EDX + displacement;
        case 3:
            DECODE_PRINTF2("%08x[EBX]", displacement);
            return M.x86.R_EBX + displacement;
        case 4:
            sib = fetch_byte_imm();
            displacement = (u32) fetch_long_imm();
            DECODE_PRINTF2("%08x", displacement);
            return decode_sib_address(sib, 2) + displacement;
            break;
        case 5:
            DECODE_PRINTF2("%08x[EBP]", displacement);
            return M.x86.R_EBP + displacement;
        case 6:
            DECODE_PRINTF2("%08x[ESI]", displacement);
            return M.x86.R_ESI + displacement;
        case 7:
            DECODE_PRINTF2("%08x[EDI]", displacement);
            return M.x86.R_EDI + displacement;
        }
        HALT_SYS();
    }
    else {
        /* 16-bit addressing */
        switch (rm) {
        case 0:
            DECODE_PRINTF2("%04x[BX+SI]", displacement);
            return (M.x86.R_BX + M.x86.R_SI + displacement) & 0xffff;
        case 1:
            DECODE_PRINTF2("%04x[BX+DI]", displacement);
            return (M.x86.R_BX + M.x86.R_DI + displacement) & 0xffff;
        case 2:
            DECODE_PRINTF2("%04x[BP+SI]", displacement);
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_SI + displacement) & 0xffff;
        case 3:
            DECODE_PRINTF2("%04x[BP+DI]", displacement);
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_DI + displacement) & 0xffff;
        case 4:
            DECODE_PRINTF2("%04x[SI]", displacement);
            return (M.x86.R_SI + displacement) & 0xffff;
        case 5:
            DECODE_PRINTF2("%04x[DI]", displacement);
            return (M.x86.R_DI + displacement) & 0xffff;
        case 6:
            DECODE_PRINTF2("%04x[BP]", displacement);
            M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + displacement) & 0xffff;
        case 7:
            DECODE_PRINTF2("%04x[BX]", displacement);
            return (M.x86.R_BX + displacement) & 0xffff;
        }
        HALT_SYS();
    }
    return 0;
    /*NOTREACHED */
}
