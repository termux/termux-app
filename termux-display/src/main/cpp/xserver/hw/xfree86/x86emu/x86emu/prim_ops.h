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
* Description:  Header file for primitive operation functions.
*
****************************************************************************/

#ifndef __X86EMU_PRIM_OPS_H
#define __X86EMU_PRIM_OPS_H

#ifdef  __cplusplus
extern "C" {                    /* Use "C" linkage when in C++ mode */
#endif

    u16 aaa_word(u16 d);
    u16 aas_word(u16 d);
    u16 aad_word(u16 d);
    u16 aam_word(u8 d);
    u8 adc_byte(u8 d, u8 s);
    u16 adc_word(u16 d, u16 s);
    u32 adc_long(u32 d, u32 s);
    u8 add_byte(u8 d, u8 s);
    u16 add_word(u16 d, u16 s);
    u32 add_long(u32 d, u32 s);
    u8 and_byte(u8 d, u8 s);
    u16 and_word(u16 d, u16 s);
    u32 and_long(u32 d, u32 s);
    u8 cmp_byte(u8 d, u8 s);
    u16 cmp_word(u16 d, u16 s);
    u32 cmp_long(u32 d, u32 s);
    u8 daa_byte(u8 d);
    u8 das_byte(u8 d);
    u8 dec_byte(u8 d);
    u16 dec_word(u16 d);
    u32 dec_long(u32 d);
    u8 inc_byte(u8 d);
    u16 inc_word(u16 d);
    u32 inc_long(u32 d);
    u8 or_byte(u8 d, u8 s);
    u16 or_word(u16 d, u16 s);
    u32 or_long(u32 d, u32 s);
    u8 neg_byte(u8 s);
    u16 neg_word(u16 s);
    u32 neg_long(u32 s);
    u8 not_byte(u8 s);
    u16 not_word(u16 s);
    u32 not_long(u32 s);
    u8 rcl_byte(u8 d, u8 s);
    u16 rcl_word(u16 d, u8 s);
    u32 rcl_long(u32 d, u8 s);
    u8 rcr_byte(u8 d, u8 s);
    u16 rcr_word(u16 d, u8 s);
    u32 rcr_long(u32 d, u8 s);
    u8 rol_byte(u8 d, u8 s);
    u16 rol_word(u16 d, u8 s);
    u32 rol_long(u32 d, u8 s);
    u8 ror_byte(u8 d, u8 s);
    u16 ror_word(u16 d, u8 s);
    u32 ror_long(u32 d, u8 s);
    u8 shl_byte(u8 d, u8 s);
    u16 shl_word(u16 d, u8 s);
    u32 shl_long(u32 d, u8 s);
    u8 shr_byte(u8 d, u8 s);
    u16 shr_word(u16 d, u8 s);
    u32 shr_long(u32 d, u8 s);
    u8 sar_byte(u8 d, u8 s);
    u16 sar_word(u16 d, u8 s);
    u32 sar_long(u32 d, u8 s);
    u16 shld_word(u16 d, u16 fill, u8 s);
    u32 shld_long(u32 d, u32 fill, u8 s);
    u16 shrd_word(u16 d, u16 fill, u8 s);
    u32 shrd_long(u32 d, u32 fill, u8 s);
    u8 sbb_byte(u8 d, u8 s);
    u16 sbb_word(u16 d, u16 s);
    u32 sbb_long(u32 d, u32 s);
    u8 sub_byte(u8 d, u8 s);
    u16 sub_word(u16 d, u16 s);
    u32 sub_long(u32 d, u32 s);
    void test_byte(u8 d, u8 s);
    void test_word(u16 d, u16 s);
    void test_long(u32 d, u32 s);
    u8 xor_byte(u8 d, u8 s);
    u16 xor_word(u16 d, u16 s);
    u32 xor_long(u32 d, u32 s);
    void imul_byte(u8 s);
    void imul_word(u16 s);
    void imul_long(u32 s);
    void imul_long_direct(u32 * res_lo, u32 * res_hi, u32 d, u32 s);
    void mul_byte(u8 s);
    void mul_word(u16 s);
    void mul_long(u32 s);
    void idiv_byte(u8 s);
    void idiv_word(u16 s);
    void idiv_long(u32 s);
    void div_byte(u8 s);
    void div_word(u16 s);
    void div_long(u32 s);
    void ins(int size);
    void outs(int size);
    u16 mem_access_word(int addr);
    void push_word(u16 w);
    void push_long(u32 w);
    u16 pop_word(void);
    u32 pop_long(void);
    void cpuid(void);

#ifdef  __cplusplus
}                               /* End of "C" linkage for C++           */
#endif
#endif                          /* __X86EMU_PRIM_OPS_H */
