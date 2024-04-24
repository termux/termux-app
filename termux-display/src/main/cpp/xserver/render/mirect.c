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

#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "mi.h"
#include "picturestr.h"
#include "mipict.h"

static void
miColorRects(PicturePtr pDst,
             PicturePtr pClipPict,
             xRenderColor * color,
             int nRect, xRectangle *rects, int xoff, int yoff)
{
    CARD32 pixel;
    GCPtr pGC;
    ChangeGCVal tmpval[5];
    RegionPtr pClip;
    unsigned long mask;

    miRenderColorToPixel(pDst->pFormat, color, &pixel);

    pGC = GetScratchGC(pDst->pDrawable->depth, pDst->pDrawable->pScreen);
    if (!pGC)
        return;
    tmpval[0].val = GXcopy;
    tmpval[1].val = pixel;
    tmpval[2].val = pDst->subWindowMode;
    mask = GCFunction | GCForeground | GCSubwindowMode;
    if (pClipPict->clientClip) {
        tmpval[3].val = pDst->clipOrigin.x - xoff;
        tmpval[4].val = pDst->clipOrigin.y - yoff;
        mask |= GCClipXOrigin | GCClipYOrigin;

        pClip = RegionCreate(NULL, 1);
        RegionCopy(pClip, (RegionPtr) pClipPict->clientClip);
        (*pGC->funcs->ChangeClip) (pGC, CT_REGION, pClip, 0);
    }

    ChangeGC(NullClient, pGC, mask, tmpval);
    ValidateGC(pDst->pDrawable, pGC);
    if (xoff || yoff) {
        int i;

        for (i = 0; i < nRect; i++) {
            rects[i].x -= xoff;
            rects[i].y -= yoff;
        }
    }
    (*pGC->ops->PolyFillRect) (pDst->pDrawable, pGC, nRect, rects);
    if (xoff || yoff) {
        int i;

        for (i = 0; i < nRect; i++) {
            rects[i].x += xoff;
            rects[i].y += yoff;
        }
    }
    FreeScratchGC(pGC);
}

void
miCompositeRects(CARD8 op,
                 PicturePtr pDst,
                 xRenderColor * color, int nRect, xRectangle *rects)
{
    if (color->alpha == 0xffff) {
        if (op == PictOpOver)
            op = PictOpSrc;
    }
    if (op == PictOpClear)
        color->red = color->green = color->blue = color->alpha = 0;

    if (op == PictOpSrc || op == PictOpClear) {
        miColorRects(pDst, pDst, color, nRect, rects, 0, 0);
        if (pDst->alphaMap)
            miColorRects(pDst->alphaMap, pDst,
                         color, nRect, rects,
                         pDst->alphaOrigin.x, pDst->alphaOrigin.y);
    }
    else {
        int error;
        PicturePtr pSrc = CreateSolidPicture(0, color, &error);

        if (pSrc) {
            while (nRect--) {
                CompositePicture(op, pSrc, 0, pDst, 0, 0, 0, 0,
                                 rects->x, rects->y,
                                 rects->width, rects->height);
                rects++;
            }

            FreePicture((void *) pSrc, 0);
        }
    }
}
