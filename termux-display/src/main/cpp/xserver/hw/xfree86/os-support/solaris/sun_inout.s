/ $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sunos/sun_inout.s,v 1.1 2001/05/28 02:42:31 tsi Exp $
/
/ Copyright 1994-2001 The XFree86 Project, Inc.  All Rights Reserved.
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights
/ to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/ copies of the Software, and to permit persons to whom the Software is
/ furnished to do so, subject to the following conditions:
/
/ The above copyright notice and this permission notice shall be included in
/ all copies or substantial portions of the Software.
/
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/ IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/ FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
/ XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
/ IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
/ CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
/
/ Except as contained in this notice, the name of the XFree86 Project shall not
/ be used in advertising or otherwise to promote the sale, use or other
/ dealings in this Software without prior written authorization from the
/ XFree86 Project.
/
/
/ File: sun_inout.s
/
/ Purpose: Provide inb(), inw(), inl(), outb(), outw(), outl() functions
/	   for Solaris x86 using the ProWorks compiler by SunPro
/
/ Author:  Installed into XFree86 SuperProbe by Doug Anson (danson@lgc.com)
/	   Portions donated to XFree86 by Steve Dever (Steve.Dever@Eng.Sun.Com)
/
/ Synopsis: (c callable external declarations)
/	   extern unsigned char inb(int port);
/	   extern unsigned short inw(int port);
/	   extern unsigned long inl(int port);
/	   extern void outb(int port, unsigned char value);
/	   extern void outw(int port, unsigned short value);
/	   extern void outl(int port, unsigned long value);
/

.file "sunos_inout.s"
.text

.globl	inb
.globl	inw
.globl	inl
.globl	outb
.globl	outw
.globl	outl

/
/ unsigned char inb(int port);
/
.align	4
inb:
	movl 4(%esp),%edx
	subl %eax,%eax
	inb  (%dx)
	ret
.type	inb,@function
.size	inb,.-inb

/
/ unsigned short inw(int port);
/
.align	4
inw:
	movl 4(%esp),%edx
	subl %eax,%eax
	inw  (%dx)
	ret
.type	inw,@function
.size	inw,.-inw

/
/ unsigned long inl(int port);
/
.align	4
inl:
	movl 4(%esp),%edx
	inl  (%dx)
	ret
.type	inl,@function
.size	inl,.-inl

/
/     void outb(int port, unsigned char value);
/
.align	4
outb:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	outb (%dx)
	ret
.type	outb,@function
.size	outb,.-outb

/
/     void outw(int port, unsigned short value);
/
.align	4
outw:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	outw (%dx)
	ret
.type	outw,@function
.size	outw,.-outw

/
/     void outl(int port, unsigned long value);
/
.align	4
outl:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	outl (%dx)
	ret
.type	outl,@function
.size	outl,.-outl
