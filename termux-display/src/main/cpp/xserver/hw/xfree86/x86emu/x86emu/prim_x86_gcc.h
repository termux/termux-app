/****************************************************************************
*
* Inline helpers for x86emu
*
* Copyright (C) 2008 Bart Trojanowski, Symbio Technologies, LLC
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
* Language:     GNU C
* Environment:  GCC on i386 or x86-64
* Developer:    Bart Trojanowski
*
* Description:  This file defines a few x86 macros that can be used by the
*               emulator to execute native instructions.
*
*               For PIC vs non-PIC code refer to:
*               http://sam.zoy.org/blog/2007-04-13-shlib-with-non-pic-code-have-inline-assembly-and-pic-mix-well
*
****************************************************************************/
#ifndef __X86EMU_PRIM_X86_GCC_H
#define __X86EMU_PRIM_X86_GCC_H

#include "x86emu/types.h"

#if !defined(__GNUC__) || !(defined (__i386__) || defined(__i386) || defined(__AMD64__) || defined(__amd64__))
#error This file is intended to be used by gcc on i386 or x86-64 system
#endif

#if defined(__PIC__) && defined(__i386__)

#define X86EMU_HAS_HW_CPUID 1
static inline void
hw_cpuid(u32 * a, u32 * b, u32 * c, u32 * d)
{
    __asm__ __volatile__("pushl %%ebx      \n\t"
                         "cpuid            \n\t"
                         "movl %%ebx, %1   \n\t"
                         "popl %%ebx       \n\t":"=a"(*a), "=r"(*b),
                         "=c"(*c), "=d"(*d)
                         :"a"(*a), "c"(*c)
                         :"cc");
}

#else                           /* ! (__PIC__ && __i386__) */

#define x86EMU_HAS_HW_CPUID 1
static inline void
hw_cpuid(u32 * a, u32 * b, u32 * c, u32 * d)
{
    __asm__ __volatile__("cpuid":"=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                         :"a"(*a), "c"(*c)
                         :"cc");
}

#endif                          /* __PIC__ && __i386__ */

#endif                          /* __X86EMU_PRIM_X86_GCC_H */
