/* Copyright (c) 2004, Oracle and/or its affiliates. All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef INLINE_ASM
#define FUNCTION_START(f,n)	.inline f,n
#define FUNCTION_END(f)		.end
#else
#define _ASM
#include <sys/asm_linkage.h>
#define FUNCTION_START(f,n)	ENTRY(f)
#define FUNCTION_END(f)		retl; nop; SET_SIZE(f)
#endif

/* Converted from common/compiler.h gcc inline format to Sun cc inline
 * format by Kenjiro Tsuji	
 *	
 *	The value 0x88 means ASI_PRIMARY_LITTLE.
 *	The store or load to/from the address space will be done
 *	as little-endian. In the original xrog code, the value
 *	is defined as the macro ASI_PL.
 *
 *	In the original xorg code, "membar #StoreStore|#StoreLoad"
 *	is directly implemented as an instruction "0x8143e00a".
 *
 */

	FUNCTION_START(outb, 0)
	stba	%o1, [%o0] 0x88
	membar	#StoreStore|#StoreLoad
	FUNCTION_END(outb)

	FUNCTION_START(outw, 0)
	stha	%o1, [%o0] 0x88
	membar	#StoreStore|#StoreLoad
	FUNCTION_END(outw)

	FUNCTION_START(outl, 0)
	sta	%o1, [%o0] 0x88
	membar	#StoreStore|#StoreLoad
	FUNCTION_END(outl)

	FUNCTION_START(inb, 0)
	lduba	[%o0] 0x88, %o0
	FUNCTION_END(inb)

	FUNCTION_START(inw, 0)
	lduha	[%o0] 0x88, %o0
	FUNCTION_END(inw)

	FUNCTION_START(inl, 0)
	lda	[%o0] 0x88, %o0
	FUNCTION_END(inl)

	FUNCTION_START(xf86ReadMmio8, 0)
	lduba	[%o0 + %o1] 0x88, %o0
	FUNCTION_END(xf86ReadMmio8)

	FUNCTION_START(xf86ReadMmio16Be, 0)
	lduh	[%o0 + %o1], %o0
	FUNCTION_END(xf86ReadMmio16Be)

	FUNCTION_START(xf86ReadMmio16Le, 0)
	lduha	[%o0 + %o1] 0x88, %o0
	FUNCTION_END(xf86ReadMmio16Le)

	FUNCTION_START(xf86ReadMmio32Be, 0)
	ld	[%o0 + %o1], %o0
	FUNCTION_END(xf86ReadMmio32Be)

	FUNCTION_START(xf86ReadMmio32Le, 0)
	lda	[%o0 + %o1] 0x88, %o0
	FUNCTION_END(xf86ReadMmio32Le)

	FUNCTION_START(xf86WriteMmio8, 0)
	stba	%o2, [%o0 + %o1] 0x88
	membar	#StoreStore|#StoreLoad
	FUNCTION_END(xf86WriteMmio8)

	FUNCTION_START(xf86WriteMmio16Be, 0)
	sth	%o2, [%o0 + %o1]
	membar	#StoreStore|#StoreLoad
	FUNCTION_END(xf86WriteMmio16Be)

	FUNCTION_START(xf86WriteMmio16Le, 0)
	stha	%o2, [%o0 + %o1] 0x88
	membar	#StoreStore|#StoreLoad
	FUNCTION_END(xf86WriteMmio16Le)

	FUNCTION_START(xf86WriteMmio32Be, 0)
	st	%o2, [%o0 + %o1]
	membar	#StoreStore|#StoreLoad
	FUNCTION_END(xf86WriteMmio32Be)

	FUNCTION_START(xf86WriteMmio32Le, 0)
	sta	%o2, [%o0 + %o1] 0x88
	membar	#StoreStore|#StoreLoad
	FUNCTION_END(xf86WriteMmio32Le)

	FUNCTION_START(xf86WriteMmio8NB, 0)
	add	%o0, %o1, %o0
	stba	%o2, [%o0] 0x88
	FUNCTION_END(xf86WriteMmio8NB)

	FUNCTION_START(xf86WriteMmio16BeNB, 0)
	sth	%o2, [%o0 + %o1]
	FUNCTION_END(xf86WriteMmio16BeNB)

	FUNCTION_START(xf86WriteMmio16LeNB, 0)
	stha	%o2, [%o0 + %o1] 0x88
	FUNCTION_END(xf86WriteMmio16LeNB)

	FUNCTION_START(xf86WriteMmio32BeNB, 0)
	st	%o2, [%o0 + %o1]
	FUNCTION_END(xf86WriteMmio32BeNB)

	FUNCTION_START(xf86WriteMmio32LeNB, 0)
	sta	%o2, [%o0 + %o1] 0x88
	FUNCTION_END(xf86WriteMmio32LeNB)

