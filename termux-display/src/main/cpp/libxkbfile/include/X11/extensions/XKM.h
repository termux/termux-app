/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/
#ifndef XKM_H
#define	XKM_H 1

#define	XkmFileVersion		15

#define	XkmIllegalFile		-1
#define	XkmSemanticsFile	20
#define	XkmLayoutFile		21
#define	XkmKeymapFile		22
#define	XkmGeometryFile		23

#define	XkmTypesIndex		0
#define	XkmCompatMapIndex	1
#define	XkmSymbolsIndex		2
#define	XkmIndicatorsIndex	3
#define	XkmKeyNamesIndex	4
#define	XkmGeometryIndex	5
#define	XkmVirtualModsIndex	6
#define	XkmLastIndex		XkmVirtualModsIndex

#define	XkmTypesMask		(1<<0)
#define	XkmCompatMapMask	(1<<1)
#define	XkmSymbolsMask		(1<<2)
#define	XkmIndicatorsMask	(1<<3)
#define	XkmKeyNamesMask		(1<<4)
#define	XkmGeometryMask		(1<<5)
#define	XkmVirtualModsMask	(1<<6)
#define	XkmLegalIndexMask	(0x7f)
#define	XkmAllIndicesMask	(0x7f)

#define	XkmSemanticsRequired	(XkmCompatMapMask)
#define	XkmSemanticsOptional	(XkmTypesMask|XkmVirtualModsMask|XkmIndicatorsMask)
#define	XkmSemanticsLegal	(XkmSemanticsRequired|XkmSemanticsOptional)
#define	XkmLayoutRequired	(XkmKeyNamesMask|XkmSymbolsMask|XkmTypesMask)
#define	XkmLayoutOptional	(XkmVirtualModsMask|XkmGeometryMask)
#define	XkmLayoutLegal		(XkmLayoutRequired|XkmLayoutOptional)
#define	XkmKeymapRequired	(XkmSemanticsRequired|XkmLayoutRequired)
#define	XkmKeymapOptional	((XkmSemanticsOptional|XkmLayoutOptional)&(~XkmKeymapRequired))
#define	XkmKeymapLegal		(XkmKeymapRequired|XkmKeymapOptional)

#define	XkmLegalSection(m)	(((m)&(~XkmKeymapLegal))==0)
#define	XkmSingleSection(m)	(XkmLegalSection(m)&&(((m)&(~(m)+1))==(m)))

#endif /* XKM_H */
