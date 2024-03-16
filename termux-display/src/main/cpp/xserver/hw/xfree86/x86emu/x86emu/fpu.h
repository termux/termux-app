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
* Description:  Header file for FPU instruction decoding.
*
****************************************************************************/

#ifndef __X86EMU_FPU_H
#define __X86EMU_FPU_H

#ifdef  __cplusplus
extern "C" {                    /* Use "C" linkage when in C++ mode */
#endif

/* these have to be defined, whether 8087 support compiled in or not. */

    extern void x86emuOp_esc_coprocess_d8(u8 op1);
    extern void x86emuOp_esc_coprocess_d9(u8 op1);
    extern void x86emuOp_esc_coprocess_da(u8 op1);
    extern void x86emuOp_esc_coprocess_db(u8 op1);
    extern void x86emuOp_esc_coprocess_dc(u8 op1);
    extern void x86emuOp_esc_coprocess_dd(u8 op1);
    extern void x86emuOp_esc_coprocess_de(u8 op1);
    extern void x86emuOp_esc_coprocess_df(u8 op1);

#ifdef  __cplusplus
}                               /* End of "C" linkage for C++           */
#endif
#endif                          /* __X86EMU_FPU_H */
