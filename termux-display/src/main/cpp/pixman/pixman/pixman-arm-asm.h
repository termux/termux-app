/*
 * Copyright © 2008 Mozilla Corporation
 * Copyright © 2010 Nokia Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Mozilla Corporation not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Mozilla Corporation makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author:  Jeff Muizelaar (jeff@infidigm.net)
 *
 */


#include "pixman-config.h"


/* Supplementary macro for setting function attributes */
.macro pixman_asm_function_impl fname
#ifdef ASM_HAVE_FUNC_DIRECTIVE
	.func \fname
#endif
	.global \fname
#ifdef __ELF__
	.hidden \fname
	.type \fname, %function
#endif
\fname:
.endm

.macro pixman_asm_function fname
#ifdef ASM_LEADING_UNDERSCORE
	pixman_asm_function_impl _\fname
#else
	pixman_asm_function_impl \fname
#endif
.endm

.macro pixman_syntax_unified
#ifdef ASM_HAVE_SYNTAX_UNIFIED
	.syntax unified
#endif
.endm

.macro pixman_end_asm_function
#ifdef ASM_HAVE_FUNC_DIRECTIVE
	.endfunc
#endif
.endm
