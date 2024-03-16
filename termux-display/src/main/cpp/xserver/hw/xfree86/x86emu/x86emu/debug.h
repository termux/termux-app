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
* Description:  Header file for debug definitions.
*
****************************************************************************/

#ifndef __X86EMU_DEBUG_H
#define __X86EMU_DEBUG_H

/*---------------------- Macros and type definitions ----------------------*/

/* checks to be enabled for "runtime" */

#define CHECK_IP_FETCH_F                0x1
#define CHECK_SP_ACCESS_F               0x2
#define CHECK_MEM_ACCESS_F              0x4     /*using regular linear pointer */
#define CHECK_DATA_ACCESS_F             0x8     /*using segment:offset */

#ifdef DEBUG
#define CHECK_IP_FETCH()              	(M.x86.check & CHECK_IP_FETCH_F)
#define CHECK_SP_ACCESS()             	(M.x86.check & CHECK_SP_ACCESS_F)
#define CHECK_MEM_ACCESS()            	(M.x86.check & CHECK_MEM_ACCESS_F)
#define CHECK_DATA_ACCESS()           	(M.x86.check & CHECK_DATA_ACCESS_F)
#else
#define CHECK_IP_FETCH()
#define CHECK_SP_ACCESS()
#define CHECK_MEM_ACCESS()
#define CHECK_DATA_ACCESS()
#endif

#ifdef DEBUG
#define DEBUG_INSTRUMENT()    	(M.x86.debug & DEBUG_INSTRUMENT_F)
#define DEBUG_DECODE()        	(M.x86.debug & DEBUG_DECODE_F)
#define DEBUG_TRACE()         	(M.x86.debug & DEBUG_TRACE_F)
#define DEBUG_STEP()          	(M.x86.debug & DEBUG_STEP_F)
#define DEBUG_DISASSEMBLE()   	(M.x86.debug & DEBUG_DISASSEMBLE_F)
#define DEBUG_BREAK()         	(M.x86.debug & DEBUG_BREAK_F)
#define DEBUG_SVC()           	(M.x86.debug & DEBUG_SVC_F)
#define DEBUG_SAVE_IP_CS()     (M.x86.debug & DEBUG_SAVE_IP_CS_F)

#define DEBUG_FS()            	(M.x86.debug & DEBUG_FS_F)
#define DEBUG_PROC()          	(M.x86.debug & DEBUG_PROC_F)
#define DEBUG_SYSINT()        	(M.x86.debug & DEBUG_SYSINT_F)
#define DEBUG_TRACECALL()     	(M.x86.debug & DEBUG_TRACECALL_F)
#define DEBUG_TRACECALLREGS() 	(M.x86.debug & DEBUG_TRACECALL_REGS_F)
#define DEBUG_SYS()           	(M.x86.debug & DEBUG_SYS_F)
#define DEBUG_MEM_TRACE()     	(M.x86.debug & DEBUG_MEM_TRACE_F)
#define DEBUG_IO_TRACE()      	(M.x86.debug & DEBUG_IO_TRACE_F)
#define DEBUG_DECODE_NOPRINT() (M.x86.debug & DEBUG_DECODE_NOPRINT_F)
#else
#define DEBUG_INSTRUMENT()    	0
#define DEBUG_DECODE()        	0
#define DEBUG_TRACE()         	0
#define DEBUG_STEP()          	0
#define DEBUG_DISASSEMBLE()   	0
#define DEBUG_BREAK()         	0
#define DEBUG_SVC()           	0
#define DEBUG_SAVE_IP_CS()     0
#define DEBUG_FS()            	0
#define DEBUG_PROC()          	0
#define DEBUG_SYSINT()        	0
#define DEBUG_TRACECALL()     	0
#define DEBUG_TRACECALLREGS() 	0
#define DEBUG_SYS()           	0
#define DEBUG_MEM_TRACE()     	0
#define DEBUG_IO_TRACE()      	0
#define DEBUG_DECODE_NOPRINT() 0
#endif

#ifdef DEBUG

#define DECODE_PRINTF(x)     	if (DEBUG_DECODE()) \
									x86emu_decode_printf("%s",x)
#define DECODE_PRINTF2(x,y)  	if (DEBUG_DECODE()) \
									x86emu_decode_printf(x,y)

/*
 * The following allows us to look at the bytes of an instruction.  The
 * first INCR_INSTRN_LEN, is called every time bytes are consumed in
 * the decoding process.  The SAVE_IP_CS is called initially when the
 * major opcode of the instruction is accessed.
 */
#define INC_DECODED_INST_LEN(x)                    	\
	if (DEBUG_DECODE())  	                       	\
		x86emu_inc_decoded_inst_len(x)

#define SAVE_IP_CS(x,y)                               			\
	if (DEBUG_DECODE() | DEBUG_TRACECALL() | DEBUG_BREAK() \
              | DEBUG_IO_TRACE() | DEBUG_SAVE_IP_CS()) { \
		M.x86.saved_cs = x;                          			\
		M.x86.saved_ip = y;                          			\
	}
#else
#define INC_DECODED_INST_LEN(x)
#define DECODE_PRINTF(x)
#define DECODE_PRINTF2(x,y)
#define SAVE_IP_CS(x,y)
#endif

#ifdef DEBUG
#define TRACE_REGS()                                   		\
	if (DEBUG_DISASSEMBLE()) {                         		\
		x86emu_just_disassemble();                        	\
		goto EndOfTheInstructionProcedure;             		\
	}                                                   	\
	if (DEBUG_TRACE() || DEBUG_DECODE()) X86EMU_trace_regs()
#else
#define TRACE_REGS()
#endif

#ifdef DEBUG
#define SINGLE_STEP()		if (DEBUG_STEP()) x86emu_single_step()
#else
#define SINGLE_STEP()
#endif

#define TRACE_AND_STEP()	\
	TRACE_REGS();			\
	SINGLE_STEP()

#ifdef DEBUG
#define START_OF_INSTR()
#define END_OF_INSTR()		EndOfTheInstructionProcedure: x86emu_end_instr();
#define END_OF_INSTR_NO_TRACE()	x86emu_end_instr();
#else
#define START_OF_INSTR()
#define END_OF_INSTR()
#define END_OF_INSTR_NO_TRACE()
#endif

#ifdef DEBUG
#define  CALL_TRACE(u,v,w,x,s)                                 \
	if (DEBUG_TRACECALLREGS())									\
		x86emu_dump_regs();                                     \
	if (DEBUG_TRACECALL())                                     	\
		printk("%04x:%04x: CALL %s%04x:%04x\n", u , v, s, w, x);
#define RETURN_TRACE(n,u,v)                                    \
	if (DEBUG_TRACECALLREGS())									\
		x86emu_dump_regs();                                     \
	if (DEBUG_TRACECALL())                                     	\
		printk("%04x:%04x: %s\n",u,v,n);
#else
#define CALL_TRACE(u,v,w,x,s)
#define RETURN_TRACE(n,u,v)
#endif

#ifdef DEBUG
#define	DB(x)	x
#else
#define	DB(x)
#endif

/*-------------------------- Function Prototypes --------------------------*/

#ifdef  __cplusplus
extern "C" {                    /* Use "C" linkage when in C++ mode */
#endif

    extern void x86emu_inc_decoded_inst_len(int x);
    extern void x86emu_decode_printf(const char *x, ...) _X_ATTRIBUTE_PRINTF(1,2);
    extern void x86emu_just_disassemble(void);
    extern void x86emu_single_step(void);
    extern void x86emu_end_instr(void);
    extern void x86emu_dump_regs(void);
    extern void x86emu_dump_xregs(void);
    extern void x86emu_print_int_vect(u16 iv);
    extern void x86emu_instrument_instruction(void);
    extern void x86emu_check_ip_access(void);
    extern void x86emu_check_sp_access(void);
    extern void x86emu_check_mem_access(u32 p);
    extern void x86emu_check_data_access(uint s, uint o);

#ifdef  __cplusplus
}                               /* End of "C" linkage for C++           */
#endif
#endif                          /* __X86EMU_DEBUG_H */
