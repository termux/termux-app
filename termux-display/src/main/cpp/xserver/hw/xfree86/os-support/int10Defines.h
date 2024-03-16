/*
 * Copyright (c) 2000-2001 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _INT10DEFINES_H_
#define _INT10DEFINES_H_ 1

#ifdef _VM86_LINUX

#include <asm/vm86.h>

#define CPU_R(type,name,num) \
	(((type *)&(((struct vm86_struct *)REG->cpuRegs)->regs.name))[num])
#define CPU_RD(name,num) CPU_R(CARD32,name,num)
#define CPU_RW(name,num) CPU_R(CARD16,name,num)
#define CPU_RB(name,num) CPU_R(CARD8,name,num)

#define X86_EAX CPU_RD(eax,0)
#define X86_EBX CPU_RD(ebx,0)
#define X86_ECX CPU_RD(ecx,0)
#define X86_EDX CPU_RD(edx,0)
#define X86_ESI CPU_RD(esi,0)
#define X86_EDI CPU_RD(edi,0)
#define X86_EBP CPU_RD(ebp,0)
#define X86_EIP CPU_RD(eip,0)
#define X86_ESP CPU_RD(esp,0)
#define X86_EFLAGS CPU_RD(eflags,0)

#define X86_FLAGS CPU_RW(eflags,0)
#define X86_AX CPU_RW(eax,0)
#define X86_BX CPU_RW(ebx,0)
#define X86_CX CPU_RW(ecx,0)
#define X86_DX CPU_RW(edx,0)
#define X86_SI CPU_RW(esi,0)
#define X86_DI CPU_RW(edi,0)
#define X86_BP CPU_RW(ebp,0)
#define X86_IP CPU_RW(eip,0)
#define X86_SP CPU_RW(esp,0)
#define X86_CS CPU_RW(cs,0)
#define X86_DS CPU_RW(ds,0)
#define X86_ES CPU_RW(es,0)
#define X86_SS CPU_RW(ss,0)
#define X86_FS CPU_RW(fs,0)
#define X86_GS CPU_RW(gs,0)

#define X86_AL CPU_RB(eax,0)
#define X86_BL CPU_RB(ebx,0)
#define X86_CL CPU_RB(ecx,0)
#define X86_DL CPU_RB(edx,0)

#define X86_AH CPU_RB(eax,1)
#define X86_BH CPU_RB(ebx,1)
#define X86_CH CPU_RB(ecx,1)
#define X86_DH CPU_RB(edx,1)

#elif defined(_X86EMU)

#include "xf86x86emu.h"

#endif

#endif
