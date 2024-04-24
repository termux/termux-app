/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include <X11/fonts/fontstruct.h>
#include "mistruct.h"
#include "region.h"

#include "Xnest.h"

#include "Display.h"
#include "XNGC.h"
#include "GCOps.h"
#include "Drawable.h"
#include "XNFont.h"
#include "Color.h"

DevPrivateKeyRec xnestGCPrivateKeyRec;

static GCFuncs xnestFuncs = {
    xnestValidateGC,
    xnestChangeGC,
    xnestCopyGC,
    xnestDestroyGC,
    xnestChangeClip,
    xnestDestroyClip,
    xnestCopyClip,
};

static GCOps xnestOps = {
    xnestFillSpans,
    xnestSetSpans,
    xnestPutImage,
    xnestCopyArea,
    xnestCopyPlane,
    xnestPolyPoint,
    xnestPolylines,
    xnestPolySegment,
    xnestPolyRectangle,
    xnestPolyArc,
    xnestFillPolygon,
    xnestPolyFillRect,
    xnestPolyFillArc,
    xnestPolyText8,
    xnestPolyText16,
    xnestImageText8,
    xnestImageText16,
    xnestImageGlyphBlt,
    xnestPolyGlyphBlt,
    xnestPushPixels
};

Bool
xnestCreateGC(GCPtr pGC)
{
    pGC->funcs = &xnestFuncs;
    pGC->ops = &xnestOps;

    pGC->miTranslate = 1;

    xnestGCPriv(pGC)->gc = XCreateGC(xnestDisplay,
                                     xnestDefaultDrawables[pGC->depth],
                                     0L, NULL);

    return True;
}

void
xnestValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
}

void
xnestChangeGC(GCPtr pGC, unsigned long mask)
{
    XGCValues values;

    if (mask & GCFunction)
        values.function = pGC->alu;

    if (mask & GCPlaneMask)
        values.plane_mask = pGC->planemask;

    if (mask & GCForeground)
        values.foreground = xnestPixel(pGC->fgPixel);

    if (mask & GCBackground)
        values.background = xnestPixel(pGC->bgPixel);

    if (mask & GCLineWidth)
        values.line_width = pGC->lineWidth;

    if (mask & GCLineStyle)
        values.line_style = pGC->lineStyle;

    if (mask & GCCapStyle)
        values.cap_style = pGC->capStyle;

    if (mask & GCJoinStyle)
        values.join_style = pGC->joinStyle;

    if (mask & GCFillStyle)
        values.fill_style = pGC->fillStyle;

    if (mask & GCFillRule)
        values.fill_rule = pGC->fillRule;

    if (mask & GCTile) {
        if (pGC->tileIsPixel)
            mask &= ~GCTile;
        else
            values.tile = xnestPixmap(pGC->tile.pixmap);
    }

    if (mask & GCStipple)
        values.stipple = xnestPixmap(pGC->stipple);

    if (mask & GCTileStipXOrigin)
        values.ts_x_origin = pGC->patOrg.x;

    if (mask & GCTileStipYOrigin)
        values.ts_y_origin = pGC->patOrg.y;

    if (mask & GCFont)
        values.font = xnestFont(pGC->font);

    if (mask & GCSubwindowMode)
        values.subwindow_mode = pGC->subWindowMode;

    if (mask & GCGraphicsExposures)
        values.graphics_exposures = pGC->graphicsExposures;

    if (mask & GCClipXOrigin)
        values.clip_x_origin = pGC->clipOrg.x;

    if (mask & GCClipYOrigin)
        values.clip_y_origin = pGC->clipOrg.y;

    if (mask & GCClipMask)      /* this is handled in change clip */
        mask &= ~GCClipMask;

    if (mask & GCDashOffset)
        values.dash_offset = pGC->dashOffset;

    if (mask & GCDashList) {
        mask &= ~GCDashList;
        XSetDashes(xnestDisplay, xnestGC(pGC),
                   pGC->dashOffset, (char *) pGC->dash, pGC->numInDashList);
    }

    if (mask & GCArcMode)
        values.arc_mode = pGC->arcMode;

    if (mask)
        XChangeGC(xnestDisplay, xnestGC(pGC), mask, &values);
}

void
xnestCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    XCopyGC(xnestDisplay, xnestGC(pGCSrc), mask, xnestGC(pGCDst));
}

void
xnestDestroyGC(GCPtr pGC)
{
    XFreeGC(xnestDisplay, xnestGC(pGC));
}

void
xnestChangeClip(GCPtr pGC, int type, void *pValue, int nRects)
{
    int i;
    BoxPtr pBox;
    XRectangle *pRects;

    xnestDestroyClip(pGC);

    switch (type) {
    case CT_NONE:
        XSetClipMask(xnestDisplay, xnestGC(pGC), None);
        pValue = NULL;
        break;

    case CT_REGION:
        nRects = RegionNumRects((RegionPtr) pValue);
        pRects = xallocarray(nRects, sizeof(*pRects));
        pBox = RegionRects((RegionPtr) pValue);
        for (i = nRects; i-- > 0;) {
            pRects[i].x = pBox[i].x1;
            pRects[i].y = pBox[i].y1;
            pRects[i].width = pBox[i].x2 - pBox[i].x1;
            pRects[i].height = pBox[i].y2 - pBox[i].y1;
        }
        XSetClipRectangles(xnestDisplay, xnestGC(pGC), 0, 0,
                           pRects, nRects, Unsorted);
        free((char *) pRects);
        break;

    case CT_PIXMAP:
        XSetClipMask(xnestDisplay, xnestGC(pGC),
                     xnestPixmap((PixmapPtr) pValue));
        /*
         * Need to change into region, so subsequent uses are with
         * current pixmap contents.
         */
        pGC->clientClip = (*pGC->pScreen->BitmapToRegion) ((PixmapPtr) pValue);
        (*pGC->pScreen->DestroyPixmap) ((PixmapPtr) pValue);
        pValue = pGC->clientClip;
        break;

    case CT_UNSORTED:
        XSetClipRectangles(xnestDisplay, xnestGC(pGC),
                           pGC->clipOrg.x, pGC->clipOrg.y,
                           (XRectangle *) pValue, nRects, Unsorted);
        break;

    case CT_YSORTED:
        XSetClipRectangles(xnestDisplay, xnestGC(pGC),
                           pGC->clipOrg.x, pGC->clipOrg.y,
                           (XRectangle *) pValue, nRects, YSorted);
        break;

    case CT_YXSORTED:
        XSetClipRectangles(xnestDisplay, xnestGC(pGC),
                           pGC->clipOrg.x, pGC->clipOrg.y,
                           (XRectangle *) pValue, nRects, YXSorted);
        break;

    case CT_YXBANDED:
        XSetClipRectangles(xnestDisplay, xnestGC(pGC),
                           pGC->clipOrg.x, pGC->clipOrg.y,
                           (XRectangle *) pValue, nRects, YXBanded);
        break;
    }

    switch (type) {
    default:
        break;

    case CT_UNSORTED:
    case CT_YSORTED:
    case CT_YXSORTED:
    case CT_YXBANDED:
        /* server clip representation is a region */
        pGC->clientClip = RegionFromRects(nRects, (xRectangle *) pValue, type);
        free(pValue);
        pValue = pGC->clientClip;
        break;
    }

    pGC->clientClip = pValue;
}

void
xnestDestroyClip(GCPtr pGC)
{
    if (pGC->clientClip) {
        RegionDestroy(pGC->clientClip);
        XSetClipMask(xnestDisplay, xnestGC(pGC), None);
        pGC->clientClip = NULL;
    }
}

void
xnestCopyClip(GCPtr pGCDst, GCPtr pGCSrc)
{
    if (pGCSrc->clientClip) {
        RegionPtr pRgn = RegionCreate(NULL, 1);
        RegionCopy(pRgn, pGCSrc->clientClip);
        xnestChangeClip(pGCDst, CT_REGION, pRgn, 0);
    } else {
        xnestDestroyClip(pGCDst);
    }
}
