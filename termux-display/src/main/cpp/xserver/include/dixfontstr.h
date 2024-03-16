/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef DIXFONTSTRUCT_H
#define DIXFONTSTRUCT_H

#include "servermd.h"
#include "dixfont.h"
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/libxfont2.h>
#include "closure.h"
#include <X11/Xproto.h>         /* for xQueryFontReply */

#define FONTCHARSET(font)	  (font)
#define FONTMAXBOUNDS(font,field) (font)->info.maxbounds.field
#define FONTMINBOUNDS(font,field) (font)->info.minbounds.field
#define TERMINALFONT(font)	  (font)->info.terminalFont
#define FONTASCENT(font)	  (font)->info.fontAscent
#define FONTDESCENT(font)	  (font)->info.fontDescent
#define FONTGLYPHS(font)	  0
#define FONTCONSTMETRICS(font)	  (font)->info.constantMetrics
#define FONTCONSTWIDTH(font)	  (font)->info.constantWidth
#define FONTALLEXIST(font)	  (font)->info.allExist
#define FONTFIRSTCOL(font)	  (font)->info.firstCol
#define FONTLASTCOL(font)	  (font)->info.lastCol
#define FONTFIRSTROW(font)	  (font)->info.firstRow
#define FONTLASTROW(font)	  (font)->info.lastRow
#define FONTDEFAULTCH(font)	  (font)->info.defaultCh
#define FONTINKMIN(font)	  (&((font)->info.ink_minbounds))
#define FONTINKMAX(font)	  (&((font)->info.ink_maxbounds))
#define FONTPROPS(font)		  (font)->info.props
#define FONTGLYPHBITS(base,pci)	  ((unsigned char *) (pci)->bits)
#define FONTINFONPROPS(font)	  (font)->info.nprops

/* some things haven't changed names, but we'll be careful anyway */

#define FONTREFCNT(font)	  (font)->refcnt

/*
 * for linear char sets
 */
#define N1dChars(pfont)	(FONTLASTCOL(pfont) - FONTFIRSTCOL(pfont) + 1)

/*
 * for 2D char sets
 */
#define N2dChars(pfont)	(N1dChars(pfont) * \
			 (FONTLASTROW(pfont) - FONTFIRSTROW(pfont) + 1))

#ifndef GLYPHPADBYTES
#define GLYPHPADBYTES -1
#endif

#if GLYPHPADBYTES == 0 || GLYPHPADBYTES == 1
#define	GLYPHWIDTHBYTESPADDED(pci)	(GLYPHWIDTHBYTES(pci))
#define	PADGLYPHWIDTHBYTES(w)		(((w)+7)>>3)
#endif

#if GLYPHPADBYTES == 2
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+1) & ~0x1)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+1) & ~0x1)
#endif

#if GLYPHPADBYTES == 4
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+3) & ~0x3)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+3) & ~0x3)
#endif

#if GLYPHPADBYTES == 8          /* for a cray? */
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+7) & ~0x7)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+7) & ~0x7)
#endif

#endif                          /* DIXFONTSTRUCT_H */
