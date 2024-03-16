/*
 *
 * Copyright © 1999 Keith Packard
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

int
miCreatePicture(PicturePtr pPicture)
{
    return Success;
}

void
miDestroyPicture(PicturePtr pPicture)
{
    if (pPicture->freeCompClip)
        RegionDestroy(pPicture->pCompositeClip);
}

static void
miDestroyPictureClip(PicturePtr pPicture)
{
    if (pPicture->clientClip)
        RegionDestroy(pPicture->clientClip);
    pPicture->clientClip = NULL;
}

static int
miChangePictureClip(PicturePtr pPicture, int type, void *value, int n)
{
    ScreenPtr pScreen = pPicture->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);
    RegionPtr clientClip;

    switch (type) {
    case CT_PIXMAP:
        /* convert the pixmap to a region */
        clientClip = BitmapToRegion(pScreen, (PixmapPtr) value);
        if (!clientClip)
            return BadAlloc;
        (*pScreen->DestroyPixmap) ((PixmapPtr) value);
        break;
    case CT_REGION:
        clientClip = value;
        break;
    case CT_NONE:
        clientClip = 0;
        break;
    default:
        clientClip = RegionFromRects(n, (xRectangle *) value, type);
        if (!clientClip)
            return BadAlloc;
        free(value);
        break;
    }
    (*ps->DestroyPictureClip) (pPicture);
    pPicture->clientClip = clientClip;
    pPicture->stateChanges |= CPClipMask;
    return Success;
}

static void
miChangePicture(PicturePtr pPicture, Mask mask)
{
    return;
}

static void
miValidatePicture(PicturePtr pPicture, Mask mask)
{
    DrawablePtr pDrawable = pPicture->pDrawable;

    if ((mask & (CPClipXOrigin | CPClipYOrigin | CPClipMask | CPSubwindowMode))
        || (pDrawable->serialNumber !=
            (pPicture->serialNumber & DRAWABLE_SERIAL_BITS))) {
        if (pDrawable->type == DRAWABLE_WINDOW) {
            WindowPtr pWin = (WindowPtr) pDrawable;
            RegionPtr pregWin;
            Bool freeTmpClip, freeCompClip;

            if (pPicture->subWindowMode == IncludeInferiors) {
                pregWin = NotClippedByChildren(pWin);
                freeTmpClip = TRUE;
            }
            else {
                pregWin = &pWin->clipList;
                freeTmpClip = FALSE;
            }
            freeCompClip = pPicture->freeCompClip;

            /*
             * if there is no client clip, we can get by with just keeping the
             * pointer we got, and remembering whether or not should destroy
             * (or maybe re-use) it later.  this way, we avoid unnecessary
             * copying of regions.  (this wins especially if many clients clip
             * by children and have no client clip.)
             */
            if (!pPicture->clientClip) {
                if (freeCompClip)
                    RegionDestroy(pPicture->pCompositeClip);
                pPicture->pCompositeClip = pregWin;
                pPicture->freeCompClip = freeTmpClip;
            }
            else {
                /*
                 * we need one 'real' region to put into the composite clip. if
                 * pregWin the current composite clip are real, we can get rid of
                 * one. if pregWin is real and the current composite clip isn't,
                 * use pregWin for the composite clip. if the current composite
                 * clip is real and pregWin isn't, use the current composite
                 * clip. if neither is real, create a new region.
                 */

                RegionTranslate(pPicture->clientClip,
                                pDrawable->x + pPicture->clipOrigin.x,
                                pDrawable->y + pPicture->clipOrigin.y);

                if (freeCompClip) {
                    RegionIntersect(pPicture->pCompositeClip,
                                    pregWin, pPicture->clientClip);
                    if (freeTmpClip)
                        RegionDestroy(pregWin);
                }
                else if (freeTmpClip) {
                    RegionIntersect(pregWin, pregWin, pPicture->clientClip);
                    pPicture->pCompositeClip = pregWin;
                }
                else {
                    pPicture->pCompositeClip = RegionCreate(NullBox, 0);
                    RegionIntersect(pPicture->pCompositeClip,
                                    pregWin, pPicture->clientClip);
                }
                pPicture->freeCompClip = TRUE;
                RegionTranslate(pPicture->clientClip,
                                -(pDrawable->x + pPicture->clipOrigin.x),
                                -(pDrawable->y + pPicture->clipOrigin.y));
            }
        }                       /* end of composite clip for a window */
        else {
            BoxRec pixbounds;

            /* XXX should we translate by drawable.x/y here ? */
            /* If you want pixmaps in offscreen memory, yes */
            pixbounds.x1 = pDrawable->x;
            pixbounds.y1 = pDrawable->y;
            pixbounds.x2 = pDrawable->x + pDrawable->width;
            pixbounds.y2 = pDrawable->y + pDrawable->height;

            if (pPicture->freeCompClip) {
                RegionReset(pPicture->pCompositeClip, &pixbounds);
            }
            else {
                pPicture->freeCompClip = TRUE;
                pPicture->pCompositeClip = RegionCreate(&pixbounds, 1);
            }

            if (pPicture->clientClip) {
                if (pDrawable->x || pDrawable->y) {
                    RegionTranslate(pPicture->clientClip,
                                    pDrawable->x + pPicture->clipOrigin.x,
                                    pDrawable->y + pPicture->clipOrigin.y);
                    RegionIntersect(pPicture->pCompositeClip,
                                    pPicture->pCompositeClip,
                                    pPicture->clientClip);
                    RegionTranslate(pPicture->clientClip,
                                    -(pDrawable->x + pPicture->clipOrigin.x),
                                    -(pDrawable->y + pPicture->clipOrigin.y));
                }
                else {
                    RegionTranslate(pPicture->pCompositeClip,
                                    -pPicture->clipOrigin.x,
                                    -pPicture->clipOrigin.y);
                    RegionIntersect(pPicture->pCompositeClip,
                                    pPicture->pCompositeClip,
                                    pPicture->clientClip);
                    RegionTranslate(pPicture->pCompositeClip,
                                    pPicture->clipOrigin.x,
                                    pPicture->clipOrigin.y);
                }
            }
        }                       /* end of composite clip for pixmap */
    }
}

static int
miChangePictureTransform(PicturePtr pPicture, PictTransform * transform)
{
    return Success;
}

static int
miChangePictureFilter(PicturePtr pPicture,
                      int filter, xFixed * params, int nparams)
{
    return Success;
}

#define BOUND(v)	(INT16) ((v) < MINSHORT ? MINSHORT : (v) > MAXSHORT ? MAXSHORT : (v))

static inline pixman_bool_t
miClipPictureReg(pixman_region16_t * pRegion,
                 pixman_region16_t * pClip, int dx, int dy)
{
    if (pixman_region_n_rects(pRegion) == 1 &&
        pixman_region_n_rects(pClip) == 1) {
        pixman_box16_t *pRbox = pixman_region_rectangles(pRegion, NULL);
        pixman_box16_t *pCbox = pixman_region_rectangles(pClip, NULL);
        int v;

        if (pRbox->x1 < (v = pCbox->x1 + dx))
            pRbox->x1 = BOUND(v);
        if (pRbox->x2 > (v = pCbox->x2 + dx))
            pRbox->x2 = BOUND(v);
        if (pRbox->y1 < (v = pCbox->y1 + dy))
            pRbox->y1 = BOUND(v);
        if (pRbox->y2 > (v = pCbox->y2 + dy))
            pRbox->y2 = BOUND(v);
        if (pRbox->x1 >= pRbox->x2 || pRbox->y1 >= pRbox->y2) {
            pixman_region_init(pRegion);
        }
    }
    else if (!pixman_region_not_empty(pClip))
        return FALSE;
    else {
        if (dx || dy)
            pixman_region_translate(pRegion, -dx, -dy);
        if (!pixman_region_intersect(pRegion, pRegion, pClip))
            return FALSE;
        if (dx || dy)
            pixman_region_translate(pRegion, dx, dy);
    }
    return pixman_region_not_empty(pRegion);
}

static inline Bool
miClipPictureSrc(RegionPtr pRegion, PicturePtr pPicture, int dx, int dy)
{
    if (pPicture->clientClip) {
        Bool result;

        pixman_region_translate(pPicture->clientClip,
                                pPicture->clipOrigin.x + dx,
                                pPicture->clipOrigin.y + dy);

        result = RegionIntersect(pRegion, pRegion, pPicture->clientClip);

        pixman_region_translate(pPicture->clientClip,
                                -(pPicture->clipOrigin.x + dx),
                                -(pPicture->clipOrigin.y + dy));

        if (!result)
            return FALSE;
    }
    return TRUE;
}

static void
SourceValidateOnePicture(PicturePtr pPicture)
{
    DrawablePtr pDrawable = pPicture->pDrawable;
    ScreenPtr pScreen;

    if (!pDrawable)
        return;

    pScreen = pDrawable->pScreen;

    pScreen->SourceValidate(pDrawable, 0, 0, pDrawable->width,
                            pDrawable->height, pPicture->subWindowMode);
}

void
miCompositeSourceValidate(PicturePtr pPicture)
{
    SourceValidateOnePicture(pPicture);
    if (pPicture->alphaMap)
        SourceValidateOnePicture(pPicture->alphaMap);
}

/*
 * returns FALSE if the final region is empty.  Indistinguishable from
 * an allocation failure, but rendering ignores those anyways.
 */

Bool
miComputeCompositeRegion(RegionPtr pRegion,
                         PicturePtr pSrc,
                         PicturePtr pMask,
                         PicturePtr pDst,
                         INT16 xSrc,
                         INT16 ySrc,
                         INT16 xMask,
                         INT16 yMask,
                         INT16 xDst, INT16 yDst, CARD16 width, CARD16 height)
{

    int v;

    pRegion->extents.x1 = xDst;
    v = xDst + width;
    pRegion->extents.x2 = BOUND(v);
    pRegion->extents.y1 = yDst;
    v = yDst + height;
    pRegion->extents.y2 = BOUND(v);
    pRegion->data = 0;
    /* Check for empty operation */
    if (pRegion->extents.x1 >= pRegion->extents.x2 ||
        pRegion->extents.y1 >= pRegion->extents.y2) {
        pixman_region_init(pRegion);
        return FALSE;
    }
    /* clip against dst */
    if (!miClipPictureReg(pRegion, pDst->pCompositeClip, 0, 0)) {
        pixman_region_fini(pRegion);
        return FALSE;
    }
    if (pDst->alphaMap) {
        if (!miClipPictureReg(pRegion, pDst->alphaMap->pCompositeClip,
                              -pDst->alphaOrigin.x, -pDst->alphaOrigin.y)) {
            pixman_region_fini(pRegion);
            return FALSE;
        }
    }
    /* clip against src */
    if (!miClipPictureSrc(pRegion, pSrc, xDst - xSrc, yDst - ySrc)) {
        pixman_region_fini(pRegion);
        return FALSE;
    }
    if (pSrc->alphaMap) {
        if (!miClipPictureSrc(pRegion, pSrc->alphaMap,
                              xDst - (xSrc - pSrc->alphaOrigin.x),
                              yDst - (ySrc - pSrc->alphaOrigin.y))) {
            pixman_region_fini(pRegion);
            return FALSE;
        }
    }
    /* clip against mask */
    if (pMask) {
        if (!miClipPictureSrc(pRegion, pMask, xDst - xMask, yDst - yMask)) {
            pixman_region_fini(pRegion);
            return FALSE;
        }
        if (pMask->alphaMap) {
            if (!miClipPictureSrc(pRegion, pMask->alphaMap,
                                  xDst - (xMask - pMask->alphaOrigin.x),
                                  yDst - (yMask - pMask->alphaOrigin.y))) {
                pixman_region_fini(pRegion);
                return FALSE;
            }
        }
    }

    miCompositeSourceValidate(pSrc);
    if (pMask)
        miCompositeSourceValidate(pMask);

    return TRUE;
}

void
miRenderColorToPixel(PictFormatPtr format, xRenderColor * color, CARD32 *pixel)
{
    CARD32 r, g, b, a;
    miIndexedPtr pIndexed;

    switch (format->type) {
    case PictTypeDirect:
        r = color->red >> (16 - Ones(format->direct.redMask));
        g = color->green >> (16 - Ones(format->direct.greenMask));
        b = color->blue >> (16 - Ones(format->direct.blueMask));
        a = color->alpha >> (16 - Ones(format->direct.alphaMask));
        r = r << format->direct.red;
        g = g << format->direct.green;
        b = b << format->direct.blue;
        a = a << format->direct.alpha;
        *pixel = r | g | b | a;
        break;
    case PictTypeIndexed:
        pIndexed = (miIndexedPtr) (format->index.devPrivate);
        if (pIndexed->color) {
            r = color->red >> 11;
            g = color->green >> 11;
            b = color->blue >> 11;
            *pixel = miIndexToEnt15(pIndexed, (r << 10) | (g << 5) | b);
        }
        else {
            r = color->red >> 8;
            g = color->green >> 8;
            b = color->blue >> 8;
            *pixel = miIndexToEntY24(pIndexed, (r << 16) | (g << 8) | b);
        }
        break;
    }
}

static CARD16
miFillColor(CARD32 pixel, int bits)
{
    while (bits < 16) {
        pixel |= pixel << bits;
        bits <<= 1;
    }
    return (CARD16) pixel;
}

Bool
miIsSolidAlpha(PicturePtr pSrc)
{
    ScreenPtr pScreen;
    char line[1];

    if (!pSrc->pDrawable)
        return FALSE;

    pScreen = pSrc->pDrawable->pScreen;

    /* Alpha-only */
    if (PICT_FORMAT_TYPE(pSrc->format) != PICT_TYPE_A)
        return FALSE;
    /* repeat */
    if (!pSrc->repeat)
        return FALSE;
    /* 1x1 */
    if (pSrc->pDrawable->width != 1 || pSrc->pDrawable->height != 1)
        return FALSE;
    line[0] = 1;
    (*pScreen->GetImage) (pSrc->pDrawable, 0, 0, 1, 1, ZPixmap, ~0L, line);
    switch (pSrc->pDrawable->bitsPerPixel) {
    case 1:
        return (CARD8) line[0] == 1 || (CARD8) line[0] == 0x80;
    case 4:
        return (CARD8) line[0] == 0xf || (CARD8) line[0] == 0xf0;
    case 8:
        return (CARD8) line[0] == 0xff;
    default:
        return FALSE;
    }
}

void
miRenderPixelToColor(PictFormatPtr format, CARD32 pixel, xRenderColor * color)
{
    CARD32 r, g, b, a;
    miIndexedPtr pIndexed;

    switch (format->type) {
    case PictTypeDirect:
        r = (pixel >> format->direct.red) & format->direct.redMask;
        g = (pixel >> format->direct.green) & format->direct.greenMask;
        b = (pixel >> format->direct.blue) & format->direct.blueMask;
        a = (pixel >> format->direct.alpha) & format->direct.alphaMask;
        color->red = miFillColor(r, Ones(format->direct.redMask));
        color->green = miFillColor(g, Ones(format->direct.greenMask));
        color->blue = miFillColor(b, Ones(format->direct.blueMask));
        color->alpha = miFillColor(a, Ones(format->direct.alphaMask));
        break;
    case PictTypeIndexed:
        pIndexed = (miIndexedPtr) (format->index.devPrivate);
        pixel = pIndexed->rgba[pixel & (MI_MAX_INDEXED - 1)];
        r = (pixel >> 16) & 0xff;
        g = (pixel >> 8) & 0xff;
        b = (pixel) & 0xff;
        color->red = miFillColor(r, 8);
        color->green = miFillColor(g, 8);
        color->blue = miFillColor(b, 8);
        color->alpha = 0xffff;
        break;
    }
}

static void
miTriStrip(CARD8 op,
           PicturePtr pSrc,
           PicturePtr pDst,
           PictFormatPtr maskFormat,
           INT16 xSrc, INT16 ySrc, int npoints, xPointFixed * points)
{
    xTriangle *tris, *tri;
    int ntri;

    ntri = npoints - 2;
    tris = xallocarray(ntri, sizeof(xTriangle));
    if (!tris)
        return;

    for (tri = tris; npoints >= 3; npoints--, points++, tri++) {
        tri->p1 = points[0];
        tri->p2 = points[1];
        tri->p3 = points[2];
    }
    CompositeTriangles(op, pSrc, pDst, maskFormat, xSrc, ySrc, ntri, tris);
    free(tris);
}

static void
miTriFan(CARD8 op,
         PicturePtr pSrc,
         PicturePtr pDst,
         PictFormatPtr maskFormat,
         INT16 xSrc, INT16 ySrc, int npoints, xPointFixed * points)
{
    xTriangle *tris, *tri;
    xPointFixed *first;
    int ntri;

    ntri = npoints - 2;
    tris = xallocarray(ntri, sizeof(xTriangle));
    if (!tris)
        return;

    first = points++;
    for (tri = tris; npoints >= 3; npoints--, points++, tri++) {
        tri->p1 = *first;
        tri->p2 = points[0];
        tri->p3 = points[1];
    }
    CompositeTriangles(op, pSrc, pDst, maskFormat, xSrc, ySrc, ntri, tris);
    free(tris);
}

Bool
miPictureInit(ScreenPtr pScreen, PictFormatPtr formats, int nformats)
{
    PictureScreenPtr ps;

    if (!PictureInit(pScreen, formats, nformats))
        return FALSE;
    ps = GetPictureScreen(pScreen);
    ps->CreatePicture = miCreatePicture;
    ps->DestroyPicture = miDestroyPicture;
    ps->ChangePictureClip = miChangePictureClip;
    ps->DestroyPictureClip = miDestroyPictureClip;
    ps->ChangePicture = miChangePicture;
    ps->ValidatePicture = miValidatePicture;
    ps->InitIndexed = miInitIndexed;
    ps->CloseIndexed = miCloseIndexed;
    ps->UpdateIndexed = miUpdateIndexed;
    ps->ChangePictureTransform = miChangePictureTransform;
    ps->ChangePictureFilter = miChangePictureFilter;
    ps->RealizeGlyph = miRealizeGlyph;
    ps->UnrealizeGlyph = miUnrealizeGlyph;

    /* MI rendering routines */
    ps->Composite = 0;          /* requires DDX support */
    ps->Glyphs = miGlyphs;
    ps->CompositeRects = miCompositeRects;
    ps->Trapezoids = 0;
    ps->Triangles = 0;

    ps->RasterizeTrapezoid = 0; /* requires DDX support */
    ps->AddTraps = 0;           /* requires DDX support */
    ps->AddTriangles = 0;       /* requires DDX support */

    ps->TriStrip = miTriStrip;  /* converts call to CompositeTriangles */
    ps->TriFan = miTriFan;

    return TRUE;
}
