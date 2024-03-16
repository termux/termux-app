/*
 * Copyright (c) 2009-2012 Apple Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stddef.h>
#include "mi.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "dixfontstr.h"
#include "mivalidate.h"
#include "driWrap.h"
#include "dri.h"

#include <OpenGL/OpenGL.h>

typedef struct {
    GCOps const *originalOps;
} DRIGCRec;

typedef struct {
    GCOps *originalOps;
    CreateGCProcPtr CreateGC;
} DRIWrapScreenRec;

typedef struct {
    Bool didSave;
    int devKind;
    DevUnion devPrivate;
} DRISavedDrawableState;

static DevPrivateKeyRec driGCKeyRec;
#define driGCKey (&driGCKeyRec)

static DevPrivateKeyRec driWrapScreenKeyRec;
#define driWrapScreenKey (&driWrapScreenKeyRec)

static GCOps driGCOps;

#define wrap(priv, real, member, func) { \
        priv->member = real->member; \
        real->member = func; \
}

#define unwrap(priv, real, member)     { \
        real->member = priv->member; \
}

static DRIGCRec *
DRIGetGCPriv(GCPtr pGC)
{
    return dixLookupPrivate(&pGC->devPrivates, driGCKey);
}

static void
DRIUnwrapGC(GCPtr pGC)
{
    DRIGCRec *pGCPriv = DRIGetGCPriv(pGC);

    pGC->ops = pGCPriv->originalOps;
}

static void
DRIWrapGC(GCPtr pGC)
{
    pGC->ops = &driGCOps;
}

static void
DRISurfaceSetDrawable(DrawablePtr pDraw,
                      DRISavedDrawableState *saved)
{
    saved->didSave = FALSE;

    if (pDraw->type == DRAWABLE_PIXMAP) {
        int pitch, width, height, bpp;
        void *buffer;

        if (DRIGetPixmapData(pDraw, &width, &height, &pitch, &bpp,
                             &buffer)) {
            PixmapPtr pPix = (PixmapPtr)pDraw;

            saved->devKind = pPix->devKind;
            saved->devPrivate.ptr = pPix->devPrivate.ptr;
            saved->didSave = TRUE;

            pPix->devKind = pitch;
            pPix->devPrivate.ptr = buffer;
        }
    }
}

static void
DRISurfaceRestoreDrawable(DrawablePtr pDraw,
                          DRISavedDrawableState *saved)
{
    PixmapPtr pPix = (PixmapPtr)pDraw;

    if (!saved->didSave)
        return;

    pPix->devKind = saved->devKind;
    pPix->devPrivate.ptr = saved->devPrivate.ptr;
}

static void
DRIFillSpans(DrawablePtr dst, GCPtr pGC, int nInit,
             DDXPointPtr pptInit, int *pwidthInit,
             int sorted)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->FillSpans(dst, pGC, nInit, pptInit, pwidthInit, sorted);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRISetSpans(DrawablePtr dst, GCPtr pGC, char *pSrc,
            DDXPointPtr pptInit, int *pwidthInit,
            int nspans, int sorted)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->SetSpans(dst, pGC, pSrc, pptInit, pwidthInit, nspans, sorted);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIPutImage(DrawablePtr dst, GCPtr pGC,
            int depth, int x, int y, int w, int h,
            int leftPad, int format, char *pBits)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->PutImage(dst, pGC, depth, x, y, w, h, leftPad, format, pBits);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static RegionPtr
DRICopyArea(DrawablePtr pSrc, DrawablePtr dst, GCPtr pGC,
            int srcx, int srcy, int w, int h,
            int dstx, int dsty)
{
    RegionPtr pReg;
    DRISavedDrawableState pSrcSaved, dstSaved;

    DRISurfaceSetDrawable(pSrc, &pSrcSaved);
    DRISurfaceSetDrawable(dst, &dstSaved);

    DRIUnwrapGC(pGC);

    pReg = pGC->ops->CopyArea(pSrc, dst, pGC, srcx, srcy, w, h, dstx, dsty);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(pSrc, &pSrcSaved);
    DRISurfaceRestoreDrawable(dst, &dstSaved);

    return pReg;
}

static RegionPtr
DRICopyPlane(DrawablePtr pSrc, DrawablePtr dst,
             GCPtr pGC, int srcx, int srcy,
             int w, int h, int dstx, int dsty,
             unsigned long plane)
{
    RegionPtr pReg;
    DRISavedDrawableState pSrcSaved, dstSaved;

    DRISurfaceSetDrawable(pSrc, &pSrcSaved);
    DRISurfaceSetDrawable(dst, &dstSaved);

    DRIUnwrapGC(pGC);

    pReg = pGC->ops->CopyPlane(pSrc, dst, pGC, srcx, srcy, w, h, dstx, dsty,
                               plane);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(pSrc, &pSrcSaved);
    DRISurfaceRestoreDrawable(dst, &dstSaved);

    return pReg;
}

static void
DRIPolyPoint(DrawablePtr dst, GCPtr pGC,
             int mode, int npt, DDXPointPtr pptInit)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->PolyPoint(dst, pGC, mode, npt, pptInit);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIPolylines(DrawablePtr dst, GCPtr pGC,
             int mode, int npt, DDXPointPtr pptInit)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->Polylines(dst, pGC, mode, npt, pptInit);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIPolySegment(DrawablePtr dst, GCPtr pGC,
               int nseg, xSegment *pSeg)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->PolySegment(dst, pGC, nseg, pSeg);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIPolyRectangle(DrawablePtr dst, GCPtr pGC,
                 int nRects, xRectangle *pRects)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->PolyRectangle(dst, pGC, nRects, pRects);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}
static void
DRIPolyArc(DrawablePtr dst, GCPtr pGC, int narcs, xArc *parcs)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->PolyArc(dst, pGC, narcs, parcs);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIFillPolygon(DrawablePtr dst, GCPtr pGC,
               int shape, int mode, int count,
               DDXPointPtr pptInit)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->FillPolygon(dst, pGC, shape, mode, count, pptInit);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIPolyFillRect(DrawablePtr dst, GCPtr pGC,
                int nRectsInit, xRectangle *pRectsInit)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->PolyFillRect(dst, pGC, nRectsInit, pRectsInit);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIPolyFillArc(DrawablePtr dst, GCPtr pGC,
               int narcsInit, xArc *parcsInit)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->PolyFillArc(dst, pGC, narcsInit, parcsInit);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static int
DRIPolyText8(DrawablePtr dst, GCPtr pGC,
             int x, int y, int count, char *chars)
{
    int ret;
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    ret = pGC->ops->PolyText8(dst, pGC, x, y, count, chars);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);

    return ret;
}

static int
DRIPolyText16(DrawablePtr dst, GCPtr pGC,
              int x, int y, int count, unsigned short *chars)
{
    int ret;
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    ret = pGC->ops->PolyText16(dst, pGC, x, y, count, chars);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);

    return ret;
}

static void
DRIImageText8(DrawablePtr dst, GCPtr pGC,
              int x, int y, int count, char *chars)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->ImageText8(dst, pGC, x, y, count, chars);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIImageText16(DrawablePtr dst, GCPtr pGC,
               int x, int y, int count, unsigned short *chars)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->ImageText16(dst, pGC, x, y, count, chars);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIImageGlyphBlt(DrawablePtr dst, GCPtr pGC,
                 int x, int y, unsigned int nglyphInit,
                 CharInfoPtr *ppciInit, void *unused)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->ImageGlyphBlt(dst, pGC, x, y, nglyphInit, ppciInit, unused);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIPolyGlyphBlt(DrawablePtr dst, GCPtr pGC,
                int x, int y, unsigned int nglyph,
                CharInfoPtr *ppci, void *pglyphBase)
{
    DRISavedDrawableState saved;

    DRISurfaceSetDrawable(dst, &saved);

    DRIUnwrapGC(pGC);

    pGC->ops->PolyGlyphBlt(dst, pGC, x, y, nglyph, ppci, pglyphBase);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(dst, &saved);
}

static void
DRIPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr dst,
              int dx, int dy, int xOrg, int yOrg)
{
    DRISavedDrawableState bitMapSaved, dstSaved;

    DRISurfaceSetDrawable(&pBitMap->drawable, &bitMapSaved);
    DRISurfaceSetDrawable(dst, &dstSaved);

    DRIUnwrapGC(pGC);

    pGC->ops->PushPixels(pGC, pBitMap, dst, dx, dy, xOrg, yOrg);

    DRIWrapGC(pGC);

    DRISurfaceRestoreDrawable(&pBitMap->drawable, &bitMapSaved);
    DRISurfaceRestoreDrawable(dst, &dstSaved);
}

static GCOps driGCOps = {
    DRIFillSpans,
    DRISetSpans,
    DRIPutImage,
    DRICopyArea,
    DRICopyPlane,
    DRIPolyPoint,
    DRIPolylines,
    DRIPolySegment,
    DRIPolyRectangle,
    DRIPolyArc,
    DRIFillPolygon,
    DRIPolyFillRect,
    DRIPolyFillArc,
    DRIPolyText8,
    DRIPolyText16,
    DRIImageText8,
    DRIImageText16,
    DRIImageGlyphBlt,
    DRIPolyGlyphBlt,
    DRIPushPixels
};

static Bool
DRICreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;
    DRIWrapScreenRec *pScreenPriv;
    DRIGCRec *pGCPriv;
    Bool ret;

    pScreenPriv = dixLookupPrivate(&pScreen->devPrivates, driWrapScreenKey);

    pGCPriv = DRIGetGCPriv(pGC);

    unwrap(pScreenPriv, pScreen, CreateGC);
    ret = pScreen->CreateGC(pGC);

    if (ret) {
        pGCPriv->originalOps = pGC->ops;
        pGC->ops = &driGCOps;
    }

    wrap(pScreenPriv, pScreen, CreateGC, DRICreateGC);

    return ret;
}

/* Return false if an error occurred. */
Bool
DRIWrapInit(ScreenPtr pScreen)
{
    DRIWrapScreenRec *pScreenPriv;

    if (!dixRegisterPrivateKey(&driGCKeyRec, PRIVATE_GC, sizeof(DRIGCRec)))
        return FALSE;

    if (!dixRegisterPrivateKey(&driWrapScreenKeyRec, PRIVATE_SCREEN,
                               sizeof(DRIWrapScreenRec)))
        return FALSE;

    pScreenPriv = dixGetPrivateAddr(&pScreen->devPrivates,
                                    &driWrapScreenKeyRec);
    pScreenPriv->CreateGC = pScreen->CreateGC;
    pScreen->CreateGC = DRICreateGC;

    return TRUE;
}
