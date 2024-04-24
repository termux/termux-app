/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

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
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "gcstruct.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "mi.h"
#include "servermd.h"

#define NPT 128

/* These were stolen from mfb.  They don't really belong here. */
#define LONG2CHARSSAMEORDER(x) ((MiBits)(x))
#define LONG2CHARSDIFFORDER( x ) ( ( ( ( x ) & (MiBits)0x000000FF ) << 0x18 ) \
                        | ( ( ( x ) & (MiBits)0x0000FF00 ) << 0x08 ) \
                        | ( ( ( x ) & (MiBits)0x00FF0000 ) >> 0x08 ) \
                        | ( ( ( x ) & (MiBits)0xFF000000 ) >> 0x18 ) )

#define PGSZB	4
#define PPW	(PGSZB<<3)      /* assuming 8 bits per byte */
#define PGSZ	PPW
#define PLST	(PPW-1)
#define PIM	PLST
#define PWSH	5

/* miPushPixels -- squeegees the fill style of pGC through pBitMap
 * into pDrawable.  pBitMap is a stencil (dx by dy of it is used, it may
 * be bigger) which is placed on the drawable at xOrg, yOrg.  Where a 1 bit
 * is set in the bitmap, the fill style is put onto the drawable using
 * the GC's logical function. The drawable is not changed where the bitmap
 * has a zero bit or outside the area covered by the stencil.

WARNING:
    this code works if the 1-bit deep pixmap format returned by GetSpans
is the same as the format defined by the mfb code (i.e. 32-bit padding
per scanline, scanline unit = 32 bits; later, this might mean
bitsizeof(int) padding and sacnline unit == bitsizeof(int).)

 */

/*
 * in order to have both (MSB_FIRST and LSB_FIRST) versions of this
 * in the server, we need to rename one of them
 */
void
miPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr pDrawable,
             int dx, int dy, int xOrg, int yOrg)
{
    int h, dxDivPPW, ibEnd;
    MiBits *pwLineStart;
    MiBits *pw, *pwEnd;
    MiBits msk;
    int ib, w;
    int ipt;                    /* index into above arrays */
    Bool fInBox;
    DDXPointRec pt[NPT], ptThisLine;
    int width[NPT];

#if 1
    MiBits startmask;

    if (screenInfo.bitmapBitOrder == IMAGE_BYTE_ORDER)
        if (screenInfo.bitmapBitOrder == LSBFirst)
            startmask = (MiBits) (-1) ^ LONG2CHARSSAMEORDER((MiBits) (-1) << 1);
        else
            startmask = (MiBits) (-1) ^ LONG2CHARSSAMEORDER((MiBits) (-1) >> 1);
    else if (screenInfo.bitmapBitOrder == LSBFirst)
        startmask = (MiBits) (-1) ^ LONG2CHARSDIFFORDER((MiBits) (-1) << 1);
    else
        startmask = (MiBits) (-1) ^ LONG2CHARSDIFFORDER((MiBits) (-1) >> 1);
#endif

    pwLineStart = malloc(BitmapBytePad(dx));
    if (!pwLineStart)
        return;
    ipt = 0;
    dxDivPPW = dx / PPW;

    for (h = 0, ptThisLine.x = 0, ptThisLine.y = 0; h < dy; h++, ptThisLine.y++) {

        (*pBitMap->drawable.pScreen->GetSpans) ((DrawablePtr) pBitMap, dx,
                                                &ptThisLine, &dx, 1,
                                                (char *) pwLineStart);

        pw = pwLineStart;
        /* Process all words which are fully in the pixmap */

        fInBox = FALSE;
        pwEnd = pwLineStart + dxDivPPW;
        while (pw < pwEnd) {
            w = *pw;
#if 1
            msk = startmask;
#else
            msk = (MiBits) (-1) ^ SCRRIGHT((MiBits) (-1), 1);
#endif
            for (ib = 0; ib < PPW; ib++) {
                if (w & msk) {
                    if (!fInBox) {
                        pt[ipt].x = ((pw - pwLineStart) << PWSH) + ib + xOrg;
                        pt[ipt].y = h + yOrg;
                        /* start new box */
                        fInBox = TRUE;
                    }
                }
                else {
                    if (fInBox) {
                        width[ipt] = ((pw - pwLineStart) << PWSH) +
                            ib + xOrg - pt[ipt].x;
                        if (++ipt >= NPT) {
                            (*pGC->ops->FillSpans) (pDrawable, pGC,
                                                    NPT, pt, width, TRUE);
                            ipt = 0;
                        }
                        /* end box */
                        fInBox = FALSE;
                    }
                }
#if 1
                /* This is not quite right, but it'll do for now */
                if (screenInfo.bitmapBitOrder == IMAGE_BYTE_ORDER)
                    if (screenInfo.bitmapBitOrder == LSBFirst)
                        msk =
                            LONG2CHARSSAMEORDER(LONG2CHARSSAMEORDER(msk) << 1);
                    else
                        msk =
                            LONG2CHARSSAMEORDER(LONG2CHARSSAMEORDER(msk) >> 1);
                else if (screenInfo.bitmapBitOrder == LSBFirst)
                    msk = LONG2CHARSDIFFORDER(LONG2CHARSDIFFORDER(msk) << 1);
                else
                    msk = LONG2CHARSDIFFORDER(LONG2CHARSDIFFORDER(msk) >> 1);
#else
                msk = SCRRIGHT(msk, 1);
#endif
            }
            pw++;
        }
        ibEnd = dx & PIM;
        if (ibEnd) {
            /* Process final partial word on line */
            w = *pw;
#if 1
            msk = startmask;
#else
            msk = (MiBits) (-1) ^ SCRRIGHT((MiBits) (-1), 1);
#endif
            for (ib = 0; ib < ibEnd; ib++) {
                if (w & msk) {
                    if (!fInBox) {
                        /* start new box */
                        pt[ipt].x = ((pw - pwLineStart) << PWSH) + ib + xOrg;
                        pt[ipt].y = h + yOrg;
                        fInBox = TRUE;
                    }
                }
                else {
                    if (fInBox) {
                        /* end box */
                        width[ipt] = ((pw - pwLineStart) << PWSH) +
                            ib + xOrg - pt[ipt].x;
                        if (++ipt >= NPT) {
                            (*pGC->ops->FillSpans) (pDrawable,
                                                    pGC, NPT, pt, width, TRUE);
                            ipt = 0;
                        }
                        fInBox = FALSE;
                    }
                }
#if 1
                /* This is not quite right, but it'll do for now */
                if (screenInfo.bitmapBitOrder == IMAGE_BYTE_ORDER)
                    if (screenInfo.bitmapBitOrder == LSBFirst)
                        msk =
                            LONG2CHARSSAMEORDER(LONG2CHARSSAMEORDER(msk) << 1);
                    else
                        msk =
                            LONG2CHARSSAMEORDER(LONG2CHARSSAMEORDER(msk) >> 1);
                else if (screenInfo.bitmapBitOrder == LSBFirst)
                    msk = LONG2CHARSDIFFORDER(LONG2CHARSDIFFORDER(msk) << 1);
                else
                    msk = LONG2CHARSDIFFORDER(LONG2CHARSDIFFORDER(msk) >> 1);
#else
                msk = SCRRIGHT(msk, 1);
#endif
            }
        }
        /* If scanline ended with last bit set, end the box */
        if (fInBox) {
            width[ipt] = dx + xOrg - pt[ipt].x;
            if (++ipt >= NPT) {
                (*pGC->ops->FillSpans) (pDrawable, pGC, NPT, pt, width, TRUE);
                ipt = 0;
            }
        }
    }
    free(pwLineStart);
    /* Flush any remaining spans */
    if (ipt) {
        (*pGC->ops->FillSpans) (pDrawable, pGC, ipt, pt, width, TRUE);
    }
}
