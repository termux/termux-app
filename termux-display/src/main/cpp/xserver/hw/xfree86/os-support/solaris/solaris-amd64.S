/ Copyright (c) 2005, Oracle and/or its affiliates. All rights reserved.
/ 
/ Permission is hereby granted, free of charge, to any person obtaining a
/ copy of this software and associated documentation files (the "Software"),
/ to deal in the Software without restriction, including without limitation
/ the rights to use, copy, modify, merge, publish, distribute, sublicense,
/ and/or sell copies of the Software, and to permit persons to whom the
/ Software is furnished to do so, subject to the following conditions:
/
/ The above copyright notice and this permission notice (including the next
/ paragraph) shall be included in all copies or substantial portions of the
/ Software.
/
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/ IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/ FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
/ THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/ LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
/ DEALINGS IN THE SOFTWARE.

#ifdef INLINE_ASM
#define FUNCTION_START(f,n)	.inline f,n
#define FUNCTION_END(f)		.end
#else
#define _ASM
#include <sys/asm_linkage.h>
#define FUNCTION_START(f,n)	ENTRY(f)
#define FUNCTION_END(f)		ret; SET_SIZE(f)
#endif

	FUNCTION_START(inb,4)
	movq	%rdi, %rdx
	xorq    %rax, %rax
	inb	(%dx)
	FUNCTION_END(inb)

	FUNCTION_START(inw,4)
	movq	%rdi, %rdx
	xorq    %rax, %rax
	inw	(%dx)
	FUNCTION_END(inw)

	FUNCTION_START(inl,4)
	movq	%rdi, %rdx
	xorq    %rax, %rax
	inl	(%dx)
	FUNCTION_END(inl)

	FUNCTION_START(outb,8)
	movq	%rdi, %rdx
	movq    %rsi, %rax
	outb	(%dx)
	FUNCTION_END(outb)

	FUNCTION_START(outw,8)
	movq	%rdi, %rdx
	movq    %rsi, %rax
	outw	(%dx)
	FUNCTION_END(outw)

	FUNCTION_START(outl,8)
	movq	%rdi, %rdx
	movq    %rsi, %rax
	outl	(%dx)
	FUNCTION_END(outl)

