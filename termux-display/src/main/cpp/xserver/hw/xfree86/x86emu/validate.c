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
* Language:     Watcom C 10.6 or later
* Environment:  32-bit DOS
* Developer:    Kendall Bennett
*
* Description:  Program to validate the x86 emulator library for
*               correctness. We run the emulator primitive operations
*               functions against the real x86 CPU, and compare the result
*               and flags to ensure correctness.
*
*               We use inline assembler to compile and build this program.
*
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "x86emu.h"
#include "x86emu/prim_asm.h"

/*-------------------------- Implementation -------------------------------*/

#define true 1
#define false 0

#define ALL_FLAGS   (F_CF | F_PF | F_AF | F_ZF | F_SF | F_OF)

#define VAL_START_BINARY(parm_type,res_type,dmax,smax,dincr,sincr)  \
{                                                                   \
    parm_type   d,s;                                                \
    res_type    r,r_asm;                                            \
	ulong     	flags,inflags;                                      \
	int         f,failed = false;                                   \
    char        buf1[80],buf2[80];                                  \
    for (d = 0; d < dmax; d += dincr) {                             \
        for (s = 0; s < smax; s += sincr) {                         \
            M.x86.R_EFLG = inflags = flags = def_flags;             \
            for (f = 0; f < 2; f++) {

#define VAL_TEST_BINARY(name)                                           \
                r_asm = name##_asm(&flags,d,s);                         \
                r = name(d,s);                                  \
                if (r != r_asm || M.x86.R_EFLG != flags)                \
                    failed = true;                                      \
                if (failed || trace) {

#define VAL_TEST_BINARY_VOID(name)                                      \
                name##_asm(&flags,d,s);                                 \
                name(d,s);                                      \
                r = r_asm = 0;                                          \
                if (M.x86.R_EFLG != flags)                              \
                    failed = true;                                      \
                if (failed || trace) {

#define VAL_FAIL_BYTE_BYTE_BINARY(name)                                                                 \
                    if (failed)                                                                         \
                        printk("fail\n");                                                               \
                    printk("0x%02X = %-15s(0x%02X,0x%02X), flags = %s -> %s\n",                         \
                        r, #name, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));     \
                    printk("0x%02X = %-15s(0x%02X,0x%02X), flags = %s -> %s\n",                         \
                        r_asm, #name"_asm", d, s, print_flags(buf1,inflags), print_flags(buf2,flags));

#define VAL_FAIL_WORD_WORD_BINARY(name)                                                                 \
                    if (failed)                                                                         \
                        printk("fail\n");                                                               \
                    printk("0x%04X = %-15s(0x%04X,0x%04X), flags = %s -> %s\n",                         \
                        r, #name, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));   \
                    printk("0x%04X = %-15s(0x%04X,0x%04X), flags = %s -> %s\n",                         \
                        r_asm, #name"_asm", d, s, print_flags(buf1,inflags), print_flags(buf2,flags));

#define VAL_FAIL_LONG_LONG_BINARY(name)                                                                 \
                    if (failed)                                                                         \
                        printk("fail\n");                                                               \
                    printk("0x%08X = %-15s(0x%08X,0x%08X), flags = %s -> %s\n",                         \
                        r, #name, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG)); \
                    printk("0x%08X = %-15s(0x%08X,0x%08X), flags = %s -> %s\n",                         \
                        r_asm, #name"_asm", d, s, print_flags(buf1,inflags), print_flags(buf2,flags));

#define VAL_END_BINARY()                                                    \
                    }                                                       \
                M.x86.R_EFLG = inflags = flags = def_flags | (ALL_FLAGS & ~F_OF);   \
                if (failed)                                                 \
                    break;                                                  \
                }                                                           \
            if (failed)                                                     \
                break;                                                      \
            }                                                               \
        if (failed)                                                         \
            break;                                                          \
        }                                                                   \
    if (!failed)                                                            \
        printk("passed\n");                                                 \
}

#define VAL_BYTE_BYTE_BINARY(name)          \
    printk("Validating %s ... ", #name);    \
    VAL_START_BINARY(u8,u8,0xFF,0xFF,1,1)   \
    VAL_TEST_BINARY(name)                   \
    VAL_FAIL_BYTE_BYTE_BINARY(name)         \
    VAL_END_BINARY()

#define VAL_WORD_WORD_BINARY(name)                      \
    printk("Validating %s ... ", #name);                \
    VAL_START_BINARY(u16,u16,0xFF00,0xFF00,0x100,0x100) \
    VAL_TEST_BINARY(name)                               \
    VAL_FAIL_WORD_WORD_BINARY(name)                     \
    VAL_END_BINARY()

#define VAL_LONG_LONG_BINARY(name)                                      \
    printk("Validating %s ... ", #name);                                \
    VAL_START_BINARY(u32,u32,0xFF000000,0xFF000000,0x1000000,0x1000000) \
    VAL_TEST_BINARY(name)                                               \
    VAL_FAIL_LONG_LONG_BINARY(name)                                     \
    VAL_END_BINARY()

#define VAL_VOID_BYTE_BINARY(name)          \
    printk("Validating %s ... ", #name);    \
    VAL_START_BINARY(u8,u8,0xFF,0xFF,1,1)   \
    VAL_TEST_BINARY_VOID(name)              \
    VAL_FAIL_BYTE_BYTE_BINARY(name)         \
    VAL_END_BINARY()

#define VAL_VOID_WORD_BINARY(name)                      \
    printk("Validating %s ... ", #name);                \
    VAL_START_BINARY(u16,u16,0xFF00,0xFF00,0x100,0x100) \
    VAL_TEST_BINARY_VOID(name)                          \
    VAL_FAIL_WORD_WORD_BINARY(name)                     \
    VAL_END_BINARY()

#define VAL_VOID_LONG_BINARY(name)                                      \
    printk("Validating %s ... ", #name);                                \
    VAL_START_BINARY(u32,u32,0xFF000000,0xFF000000,0x1000000,0x1000000) \
    VAL_TEST_BINARY_VOID(name)                                          \
    VAL_FAIL_LONG_LONG_BINARY(name)                                     \
    VAL_END_BINARY()

#define VAL_BYTE_ROTATE(name)               \
    printk("Validating %s ... ", #name);    \
    VAL_START_BINARY(u8,u8,0xFF,8,1,1)      \
    VAL_TEST_BINARY(name)                   \
    VAL_FAIL_BYTE_BYTE_BINARY(name)         \
    VAL_END_BINARY()

#define VAL_WORD_ROTATE(name)                           \
    printk("Validating %s ... ", #name);                \
    VAL_START_BINARY(u16,u16,0xFF00,16,0x100,1)         \
    VAL_TEST_BINARY(name)                               \
    VAL_FAIL_WORD_WORD_BINARY(name)                     \
    VAL_END_BINARY()

#define VAL_LONG_ROTATE(name)                                           \
    printk("Validating %s ... ", #name);                                \
    VAL_START_BINARY(u32,u32,0xFF000000,32,0x1000000,1)                 \
    VAL_TEST_BINARY(name)                                               \
    VAL_FAIL_LONG_LONG_BINARY(name)                                     \
    VAL_END_BINARY()

#define VAL_START_TERNARY(parm_type,res_type,dmax,smax,dincr,sincr,maxshift)\
{                                                                   \
    parm_type   d,s;                                                \
    res_type    r,r_asm;                                            \
    u8          shift;                                              \
	u32         flags,inflags;                                      \
    int         f,failed = false;                                   \
    char        buf1[80],buf2[80];                                  \
    for (d = 0; d < dmax; d += dincr) {                             \
        for (s = 0; s < smax; s += sincr) {                         \
            for (shift = 0; shift < maxshift; shift += 1) {        \
                M.x86.R_EFLG = inflags = flags = def_flags;         \
                for (f = 0; f < 2; f++) {

#define VAL_TEST_TERNARY(name)                                          \
                    r_asm = name##_asm(&flags,d,s,shift);               \
                    r = name(d,s,shift);                           \
                    if (r != r_asm || M.x86.R_EFLG != flags)            \
                        failed = true;                                  \
                    if (failed || trace) {

#define VAL_FAIL_WORD_WORD_TERNARY(name)                                                                \
                        if (failed)                                                                         \
                            printk("fail\n");                                                               \
                        printk("0x%04X = %-15s(0x%04X,0x%04X,%d), flags = %s -> %s\n",                      \
                            r, #name, d, s, shift, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));   \
                        printk("0x%04X = %-15s(0x%04X,0x%04X,%d), flags = %s -> %s\n",                      \
                            r_asm, #name"_asm", d, s, shift, print_flags(buf1,inflags), print_flags(buf2,flags));

#define VAL_FAIL_LONG_LONG_TERNARY(name)                                                                \
                        if (failed)                                                                         \
                            printk("fail\n");                                                               \
                        printk("0x%08X = %-15s(0x%08X,0x%08X,%d), flags = %s -> %s\n",                      \
                            r, #name, d, s, shift, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));  \
                        printk("0x%08X = %-15s(0x%08X,0x%08X,%d), flags = %s -> %s\n",                      \
                            r_asm, #name"_asm", d, s, shift, print_flags(buf1,inflags), print_flags(buf2,flags));

#define VAL_END_TERNARY()                                                   \
                        }                                                       \
                    M.x86.R_EFLG = inflags = flags = def_flags | (ALL_FLAGS & ~F_OF);   \
                    if (failed)                                                 \
                        break;                                                  \
                    }                                                           \
                if (failed)                                                     \
                    break;                                                      \
                }                                                               \
            if (failed)                                                     \
                break;                                                      \
            }                                                               \
        if (failed)                                                         \
            break;                                                          \
        }                                                                   \
    if (!failed)                                                            \
        printk("passed\n");                                                 \
}

#define VAL_WORD_ROTATE_DBL(name)                           \
    printk("Validating %s ... ", #name);                    \
    VAL_START_TERNARY(u16,u16,0xFF00,0xFF00,0x100,0x100,16) \
    VAL_TEST_TERNARY(name)                                  \
    VAL_FAIL_WORD_WORD_TERNARY(name)                        \
    VAL_END_TERNARY()

#define VAL_LONG_ROTATE_DBL(name)                                           \
    printk("Validating %s ... ", #name);                                    \
    VAL_START_TERNARY(u32,u32,0xFF000000,0xFF000000,0x1000000,0x1000000,32) \
    VAL_TEST_TERNARY(name)                                                  \
    VAL_FAIL_LONG_LONG_TERNARY(name)                                        \
    VAL_END_TERNARY()

#define VAL_START_UNARY(parm_type,max,incr)                 \
{                                                           \
    parm_type   d,r,r_asm;                                  \
	u32         flags,inflags;                              \
    int         f,failed = false;                           \
    char        buf1[80],buf2[80];                          \
    for (d = 0; d < max; d += incr) {                       \
        M.x86.R_EFLG = inflags = flags = def_flags;         \
        for (f = 0; f < 2; f++) {

#define VAL_TEST_UNARY(name)                                \
            r_asm = name##_asm(&flags,d);                   \
            r = name(d);                                \
            if (r != r_asm || M.x86.R_EFLG != flags) {      \
                failed = true;

#define VAL_FAIL_BYTE_UNARY(name)                                                               \
                printk("fail\n");                                                               \
                printk("0x%02X = %-15s(0x%02X), flags = %s -> %s\n",                            \
                    r, #name, d, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));    \
                printk("0x%02X = %-15s(0x%02X), flags = %s -> %s\n",                            \
                    r_asm, #name"_asm", d, print_flags(buf1,inflags), print_flags(buf2,flags));

#define VAL_FAIL_WORD_UNARY(name)                                                               \
                printk("fail\n");                                                               \
                printk("0x%04X = %-15s(0x%04X), flags = %s -> %s\n",                            \
                    r, #name, d, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));  \
                printk("0x%04X = %-15s(0x%04X), flags = %s -> %s\n",                            \
                    r_asm, #name"_asm", d, print_flags(buf1,inflags), print_flags(buf2,flags));

#define VAL_FAIL_LONG_UNARY(name)                                                               \
                printk("fail\n");                                                               \
                printk("0x%08X = %-15s(0x%08X), flags = %s -> %s\n",                            \
                    r, #name, d, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));    \
                printk("0x%08X = %-15s(0x%08X), flags = %s -> %s\n",                            \
                    r_asm, #name"_asm", d, print_flags(buf1,inflags), print_flags(buf2,flags));

#define VAL_END_UNARY()                                                 \
                }                                                       \
            M.x86.R_EFLG = inflags = flags = def_flags | ALL_FLAGS;     \
            if (failed)                                                 \
                break;                                                  \
            }                                                           \
        if (failed)                                                     \
            break;                                                      \
        }                                                               \
    if (!failed)                                                        \
        printk("passed\n");                                             \
}

#define VAL_BYTE_UNARY(name)                \
    printk("Validating %s ... ", #name);    \
    VAL_START_UNARY(u8,0xFF,0x1)            \
    VAL_TEST_UNARY(name)                    \
    VAL_FAIL_BYTE_UNARY(name)               \
    VAL_END_UNARY()

#define VAL_WORD_UNARY(name)                \
    printk("Validating %s ... ", #name);    \
    VAL_START_UNARY(u16,0xFF00,0x100)       \
    VAL_TEST_UNARY(name)                    \
    VAL_FAIL_WORD_UNARY(name)               \
    VAL_END_UNARY()

#define VAL_WORD_BYTE_UNARY(name)           \
    printk("Validating %s ... ", #name);    \
    VAL_START_UNARY(u16,0xFF,0x1)           \
    VAL_TEST_UNARY(name)                    \
    VAL_FAIL_WORD_UNARY(name)               \
    VAL_END_UNARY()

#define VAL_LONG_UNARY(name)                \
    printk("Validating %s ... ", #name);    \
    VAL_START_UNARY(u32,0xFF000000,0x1000000) \
    VAL_TEST_UNARY(name)                    \
    VAL_FAIL_LONG_UNARY(name)               \
    VAL_END_UNARY()

#define VAL_BYTE_MUL(name)                                              \
    printk("Validating %s ... ", #name);                                \
{                                                                       \
    u8          d,s;                                                    \
    u16         r,r_asm;                                                \
	u32         flags,inflags;                                          \
    int         f,failed = false;                                       \
    char        buf1[80],buf2[80];                                      \
    for (d = 0; d < 0xFF; d += 1) {                                     \
        for (s = 0; s < 0xFF; s += 1) {                                 \
            M.x86.R_EFLG = inflags = flags = def_flags;                 \
            for (f = 0; f < 2; f++) {                                   \
                name##_asm(&flags,&r_asm,d,s);                          \
                M.x86.R_AL = d;                                         \
                name(s);                                            \
                r = M.x86.R_AX;                                         \
                if (r != r_asm || M.x86.R_EFLG != flags)                \
                    failed = true;                                      \
                if (failed || trace) {                                  \
                    if (failed)                                         \
                        printk("fail\n");                               \
                    printk("0x%04X = %-15s(0x%02X,0x%02X), flags = %s -> %s\n",                         \
                        r, #name, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));     \
                    printk("0x%04X = %-15s(0x%02X,0x%02X), flags = %s -> %s\n",                         \
                        r_asm, #name"_asm", d, s, print_flags(buf1,inflags), print_flags(buf2,flags));  \
                    }                                                       \
                M.x86.R_EFLG = inflags = flags = def_flags | (ALL_FLAGS & ~F_OF);   \
                if (failed)                                                 \
                    break;                                                  \
                }                                                           \
            if (failed)                                                     \
                break;                                                      \
            }                                                               \
        if (failed)                                                         \
            break;                                                          \
        }                                                                   \
    if (!failed)                                                            \
        printk("passed\n");                                                 \
}

#define VAL_WORD_MUL(name)                                              \
    printk("Validating %s ... ", #name);                                \
{                                                                       \
    u16         d,s;                                                    \
    u16         r_lo,r_asm_lo;                                          \
    u16         r_hi,r_asm_hi;                                          \
	u32         flags,inflags;                                          \
    int         f,failed = false;                                       \
    char        buf1[80],buf2[80];                                      \
    for (d = 0; d < 0xFF00; d += 0x100) {                               \
        for (s = 0; s < 0xFF00; s += 0x100) {                           \
            M.x86.R_EFLG = inflags = flags = def_flags;                 \
            for (f = 0; f < 2; f++) {                                   \
                name##_asm(&flags,&r_asm_lo,&r_asm_hi,d,s);             \
                M.x86.R_AX = d;                                         \
                name(s);                                            \
                r_lo = M.x86.R_AX;                                      \
                r_hi = M.x86.R_DX;                                      \
                if (r_lo != r_asm_lo || r_hi != r_asm_hi || M.x86.R_EFLG != flags)\
                    failed = true;                                      \
                if (failed || trace) {                                  \
                    if (failed)                                         \
                        printk("fail\n");                               \
                    printk("0x%04X:0x%04X = %-15s(0x%04X,0x%04X), flags = %s -> %s\n",                              \
                        r_hi,r_lo, #name, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));       \
                    printk("0x%04X:0x%04X = %-15s(0x%04X,0x%04X), flags = %s -> %s\n",                              \
                        r_asm_hi,r_asm_lo, #name"_asm", d, s, print_flags(buf1,inflags), print_flags(buf2,flags));  \
                    }                                                                                               \
                M.x86.R_EFLG = inflags = flags = def_flags | (ALL_FLAGS & ~F_OF);   \
                if (failed)                                                 \
                    break;                                                  \
                }                                                           \
            if (failed)                                                     \
                break;                                                      \
            }                                                               \
        if (failed)                                                         \
            break;                                                          \
        }                                                                   \
    if (!failed)                                                            \
        printk("passed\n");                                                 \
}

#define VAL_LONG_MUL(name)                                              \
    printk("Validating %s ... ", #name);                                \
{                                                                       \
    u32         d,s;                                                    \
    u32         r_lo,r_asm_lo;                                          \
    u32         r_hi,r_asm_hi;                                          \
	u32         flags,inflags;                                          \
    int         f,failed = false;                                       \
    char        buf1[80],buf2[80];                                      \
    for (d = 0; d < 0xFF000000; d += 0x1000000) {                       \
        for (s = 0; s < 0xFF000000; s += 0x1000000) {                   \
            M.x86.R_EFLG = inflags = flags = def_flags;                 \
            for (f = 0; f < 2; f++) {                                   \
                name##_asm(&flags,&r_asm_lo,&r_asm_hi,d,s);             \
                M.x86.R_EAX = d;                                        \
                name(s);                                            \
                r_lo = M.x86.R_EAX;                                     \
                r_hi = M.x86.R_EDX;                                     \
                if (r_lo != r_asm_lo || r_hi != r_asm_hi || M.x86.R_EFLG != flags)\
                    failed = true;                                      \
                if (failed || trace) {                                  \
                    if (failed)                                         \
                        printk("fail\n");                               \
                    printk("0x%08X:0x%08X = %-15s(0x%08X,0x%08X), flags = %s -> %s\n",                              \
                        r_hi,r_lo, #name, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));       \
                    printk("0x%08X:0x%08X = %-15s(0x%08X,0x%08X), flags = %s -> %s\n",                              \
                        r_asm_hi,r_asm_lo, #name"_asm", d, s, print_flags(buf1,inflags), print_flags(buf2,flags));  \
                    }                                                                                               \
                M.x86.R_EFLG = inflags = flags = def_flags | (ALL_FLAGS & ~F_OF);   \
                if (failed)                                                 \
                    break;                                                  \
                }                                                           \
            if (failed)                                                     \
                break;                                                      \
            }                                                               \
        if (failed)                                                         \
            break;                                                          \
        }                                                                   \
    if (!failed)                                                            \
        printk("passed\n");                                                 \
}

#define VAL_BYTE_DIV(name)                                              \
    printk("Validating %s ... ", #name);                                \
{                                                                       \
    u16         d,s;                                                    \
    u8          r_quot,r_rem,r_asm_quot,r_asm_rem;                      \
	u32         flags,inflags;                                          \
    int         f,failed = false;                                       \
    char        buf1[80],buf2[80];                                      \
    for (d = 0; d < 0xFF00; d += 0x100) {                               \
        for (s = 1; s < 0xFF; s += 1) {                                 \
            M.x86.R_EFLG = inflags = flags = def_flags;                 \
            for (f = 0; f < 2; f++) {                                   \
                M.x86.intr = 0;                                         \
                M.x86.R_AX = d;                                         \
                name(s);                                            \
                r_quot = M.x86.R_AL;                                    \
                r_rem = M.x86.R_AH;                                     \
                if (M.x86.intr & INTR_SYNCH)                            \
                    continue;                                           \
                name##_asm(&flags,&r_asm_quot,&r_asm_rem,d,s);          \
                if (r_quot != r_asm_quot || r_rem != r_asm_rem || M.x86.R_EFLG != flags) \
                    failed = true;                                      \
                if (failed || trace) {                                  \
                    if (failed)                                         \
                        printk("fail\n");                               \
                    printk("0x%02X:0x%02X = %-15s(0x%04X,0x%02X), flags = %s -> %s\n",                      \
                        r_quot, r_rem, #name, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));     \
                    printk("0x%02X:0x%02X = %-15s(0x%04X,0x%02X), flags = %s -> %s\n",                      \
                        r_asm_quot, r_asm_rem, #name"_asm", d, s, print_flags(buf1,inflags), print_flags(buf2,flags));  \
                    }                                                       \
                M.x86.R_EFLG = inflags = flags = def_flags | (ALL_FLAGS & ~F_OF);   \
                if (failed)                                                 \
                    break;                                                  \
                }                                                           \
            if (failed)                                                     \
                break;                                                      \
            }                                                               \
        if (failed)                                                         \
            break;                                                          \
        }                                                                   \
    if (!failed)                                                            \
        printk("passed\n");                                                 \
}

#define VAL_WORD_DIV(name)                                              \
    printk("Validating %s ... ", #name);                                \
{                                                                       \
    u32         d,s;                                                    \
    u16         r_quot,r_rem,r_asm_quot,r_asm_rem;                      \
	u32         flags,inflags;                                          \
    int         f,failed = false;                                       \
    char        buf1[80],buf2[80];                                      \
    for (d = 0; d < 0xFF000000; d += 0x1000000) {                       \
        for (s = 0x100; s < 0xFF00; s += 0x100) {                       \
            M.x86.R_EFLG = inflags = flags = def_flags;                 \
            for (f = 0; f < 2; f++) {                                   \
                M.x86.intr = 0;                                         \
                M.x86.R_AX = d & 0xFFFF;                                \
                M.x86.R_DX = d >> 16;                                   \
                name(s);                                            \
                r_quot = M.x86.R_AX;                                    \
                r_rem = M.x86.R_DX;                                     \
                if (M.x86.intr & INTR_SYNCH)                            \
                    continue;                                           \
                name##_asm(&flags,&r_asm_quot,&r_asm_rem,d & 0xFFFF,d >> 16,s);\
                if (r_quot != r_asm_quot || r_rem != r_asm_rem || M.x86.R_EFLG != flags) \
                    failed = true;                                      \
                if (failed || trace) {                                  \
                    if (failed)                                         \
                        printk("fail\n");                               \
                    printk("0x%04X:0x%04X = %-15s(0x%08X,0x%04X), flags = %s -> %s\n",                      \
                        r_quot, r_rem, #name, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));     \
                    printk("0x%04X:0x%04X = %-15s(0x%08X,0x%04X), flags = %s -> %s\n",                      \
                        r_asm_quot, r_asm_rem, #name"_asm", d, s, print_flags(buf1,inflags), print_flags(buf2,flags));  \
                    }                                                       \
                M.x86.R_EFLG = inflags = flags = def_flags | (ALL_FLAGS & ~F_OF);   \
                if (failed)                                                 \
                    break;                                                  \
                }                                                           \
            if (failed)                                                     \
                break;                                                      \
            }                                                               \
        if (failed)                                                         \
            break;                                                          \
        }                                                                   \
    if (!failed)                                                            \
        printk("passed\n");                                                 \
}

#define VAL_LONG_DIV(name)                                              \
    printk("Validating %s ... ", #name);                                \
{                                                                       \
    u32         d,s;                                                    \
    u32         r_quot,r_rem,r_asm_quot,r_asm_rem;                      \
	u32         flags,inflags;                                          \
    int         f,failed = false;                                       \
    char        buf1[80],buf2[80];                                      \
    for (d = 0; d < 0xFF000000; d += 0x1000000) {                       \
        for (s = 0x100; s < 0xFF00; s += 0x100) {                       \
            M.x86.R_EFLG = inflags = flags = def_flags;                 \
            for (f = 0; f < 2; f++) {                                   \
                M.x86.intr = 0;                                         \
                M.x86.R_EAX = d;                                        \
                M.x86.R_EDX = 0;                                        \
                name(s);                                            \
                r_quot = M.x86.R_EAX;                                   \
                r_rem = M.x86.R_EDX;                                    \
                if (M.x86.intr & INTR_SYNCH)                            \
                    continue;                                           \
                name##_asm(&flags,&r_asm_quot,&r_asm_rem,d,0,s);        \
                if (r_quot != r_asm_quot || r_rem != r_asm_rem || M.x86.R_EFLG != flags) \
                    failed = true;                                      \
                if (failed || trace) {                                  \
                    if (failed)                                         \
                        printk("fail\n");                               \
                    printk("0x%08X:0x%08X = %-15s(0x%08X:0x%08X,0x%08X), flags = %s -> %s\n",                       \
                        r_quot, r_rem, #name, 0, d, s, print_flags(buf1,inflags), print_flags(buf2,M.x86.R_EFLG));  \
                    printk("0x%08X:0x%08X = %-15s(0x%08X:0x%08X,0x%08X), flags = %s -> %s\n",                       \
                        r_asm_quot, r_asm_rem, #name"_asm", 0, d, s, print_flags(buf1,inflags), print_flags(buf2,flags));   \
                    }                                                       \
                M.x86.R_EFLG = inflags = flags = def_flags | (ALL_FLAGS & ~F_OF);   \
                if (failed)                                                 \
                    break;                                                  \
                }                                                           \
            if (failed)                                                     \
                break;                                                      \
            }                                                               \
        if (failed)                                                         \
            break;                                                          \
        }                                                                   \
    if (!failed)                                                            \
        printk("passed\n");                                                 \
}

void
printk(const char *fmt, ...)
{
    va_list argptr;

    va_start(argptr, fmt);
    vfprintf(stdout, fmt, argptr);
    fflush(stdout);
    va_end(argptr);
}

char *
print_flags(char *buf, ulong flags)
{
    char *separator = "";

    buf[0] = 0;
    if (flags & F_CF) {
        strcat(buf, separator);
        strcat(buf, "CF");
        separator = ",";
    }
    if (flags & F_PF) {
        strcat(buf, separator);
        strcat(buf, "PF");
        separator = ",";
    }
    if (flags & F_AF) {
        strcat(buf, separator);
        strcat(buf, "AF");
        separator = ",";
    }
    if (flags & F_ZF) {
        strcat(buf, separator);
        strcat(buf, "ZF");
        separator = ",";
    }
    if (flags & F_SF) {
        strcat(buf, separator);
        strcat(buf, "SF");
        separator = ",";
    }
    if (flags & F_OF) {
        strcat(buf, separator);
        strcat(buf, "OF");
        separator = ",";
    }
    if (separator[0] == 0)
        strcpy(buf, "None");
    return buf;
}

int
main(int argc)
{
    ulong def_flags;
    int trace = false;

    if (argc > 1)
        trace = true;
    memset(&M, 0, sizeof(M));
    def_flags = get_flags_asm() & ~ALL_FLAGS;

    VAL_WORD_UNARY(aaa_word);
    VAL_WORD_UNARY(aas_word);

    VAL_WORD_UNARY(aad_word);
    VAL_WORD_UNARY(aam_word);

    VAL_BYTE_BYTE_BINARY(adc_byte);
    VAL_WORD_WORD_BINARY(adc_word);
    VAL_LONG_LONG_BINARY(adc_long);

    VAL_BYTE_BYTE_BINARY(add_byte);
    VAL_WORD_WORD_BINARY(add_word);
    VAL_LONG_LONG_BINARY(add_long);

    VAL_BYTE_BYTE_BINARY(and_byte);
    VAL_WORD_WORD_BINARY(and_word);
    VAL_LONG_LONG_BINARY(and_long);

    VAL_BYTE_BYTE_BINARY(cmp_byte);
    VAL_WORD_WORD_BINARY(cmp_word);
    VAL_LONG_LONG_BINARY(cmp_long);

    VAL_BYTE_UNARY(daa_byte);
    VAL_BYTE_UNARY(das_byte);   /* Fails for 0x9A (out of range anyway) */

    VAL_BYTE_UNARY(dec_byte);
    VAL_WORD_UNARY(dec_word);
    VAL_LONG_UNARY(dec_long);

    VAL_BYTE_UNARY(inc_byte);
    VAL_WORD_UNARY(inc_word);
    VAL_LONG_UNARY(inc_long);

    VAL_BYTE_BYTE_BINARY(or_byte);
    VAL_WORD_WORD_BINARY(or_word);
    VAL_LONG_LONG_BINARY(or_long);

    VAL_BYTE_UNARY(neg_byte);
    VAL_WORD_UNARY(neg_word);
    VAL_LONG_UNARY(neg_long);

    VAL_BYTE_UNARY(not_byte);
    VAL_WORD_UNARY(not_word);
    VAL_LONG_UNARY(not_long);

    VAL_BYTE_ROTATE(rcl_byte);
    VAL_WORD_ROTATE(rcl_word);
    VAL_LONG_ROTATE(rcl_long);

    VAL_BYTE_ROTATE(rcr_byte);
    VAL_WORD_ROTATE(rcr_word);
    VAL_LONG_ROTATE(rcr_long);

    VAL_BYTE_ROTATE(rol_byte);
    VAL_WORD_ROTATE(rol_word);
    VAL_LONG_ROTATE(rol_long);

    VAL_BYTE_ROTATE(ror_byte);
    VAL_WORD_ROTATE(ror_word);
    VAL_LONG_ROTATE(ror_long);

    VAL_BYTE_ROTATE(shl_byte);
    VAL_WORD_ROTATE(shl_word);
    VAL_LONG_ROTATE(shl_long);

    VAL_BYTE_ROTATE(shr_byte);
    VAL_WORD_ROTATE(shr_word);
    VAL_LONG_ROTATE(shr_long);

    VAL_BYTE_ROTATE(sar_byte);
    VAL_WORD_ROTATE(sar_word);
    VAL_LONG_ROTATE(sar_long);

    VAL_WORD_ROTATE_DBL(shld_word);
    VAL_LONG_ROTATE_DBL(shld_long);

    VAL_WORD_ROTATE_DBL(shrd_word);
    VAL_LONG_ROTATE_DBL(shrd_long);

    VAL_BYTE_BYTE_BINARY(sbb_byte);
    VAL_WORD_WORD_BINARY(sbb_word);
    VAL_LONG_LONG_BINARY(sbb_long);

    VAL_BYTE_BYTE_BINARY(sub_byte);
    VAL_WORD_WORD_BINARY(sub_word);
    VAL_LONG_LONG_BINARY(sub_long);

    VAL_BYTE_BYTE_BINARY(xor_byte);
    VAL_WORD_WORD_BINARY(xor_word);
    VAL_LONG_LONG_BINARY(xor_long);

    VAL_VOID_BYTE_BINARY(test_byte);
    VAL_VOID_WORD_BINARY(test_word);
    VAL_VOID_LONG_BINARY(test_long);

    VAL_BYTE_MUL(imul_byte);
    VAL_WORD_MUL(imul_word);
    VAL_LONG_MUL(imul_long);

    VAL_BYTE_MUL(mul_byte);
    VAL_WORD_MUL(mul_word);
    VAL_LONG_MUL(mul_long);

    VAL_BYTE_DIV(idiv_byte);
    VAL_WORD_DIV(idiv_word);
    VAL_LONG_DIV(idiv_long);

    VAL_BYTE_DIV(div_byte);
    VAL_WORD_DIV(div_word);
    VAL_LONG_DIV(div_long);

    return 0;
}
