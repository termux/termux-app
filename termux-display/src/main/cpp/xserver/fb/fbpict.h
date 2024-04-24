/*
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _FBPICT_H_
#define _FBPICT_H_

/* fbpict.c */
extern _X_EXPORT void
fbComposite(CARD8 op,
            PicturePtr pSrc,
            PicturePtr pMask,
            PicturePtr pDst,
            INT16 xSrc,
            INT16 ySrc,
            INT16 xMask,
            INT16 yMask, INT16 xDst, INT16 yDst, CARD16 width, CARD16 height);

/* fbtrap.c */

extern _X_EXPORT void
fbAddTraps(PicturePtr pPicture,
           INT16 xOff, INT16 yOff, int ntrap, xTrap * traps);

extern _X_EXPORT void
fbRasterizeTrapezoid(PicturePtr alpha, xTrapezoid * trap, int x_off, int y_off);

extern _X_EXPORT void
fbAddTriangles(PicturePtr pPicture,
               INT16 xOff, INT16 yOff, int ntri, xTriangle * tris);

extern _X_EXPORT void
fbTrapezoids(CARD8 op,
             PicturePtr pSrc,
             PicturePtr pDst,
             PictFormatPtr maskFormat,
             INT16 xSrc, INT16 ySrc, int ntrap, xTrapezoid * traps);

extern _X_EXPORT void
fbTriangles(CARD8 op,
            PicturePtr pSrc,
            PicturePtr pDst,
            PictFormatPtr maskFormat,
            INT16 xSrc, INT16 ySrc, int ntris, xTriangle * tris);

extern _X_EXPORT void
fbGlyphs(CARD8 op,
	 PicturePtr pSrc,
	 PicturePtr pDst,
	 PictFormatPtr maskFormat,
	 INT16 xSrc,
	 INT16 ySrc, int nlist,
	 GlyphListPtr list,
	 GlyphPtr *glyphs);

#endif                          /* _FBPICT_H_ */
