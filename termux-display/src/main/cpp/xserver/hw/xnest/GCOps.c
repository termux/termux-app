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
#include "regionstr.h"
#include <X11/fonts/fontstruct.h>
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "region.h"
#include "servermd.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "XNGC.h"
#include "XNFont.h"
#include "GCOps.h"
#include "Drawable.h"
#include "Visual.h"

void
xnestFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nSpans, xPoint * pPoints,
               int *pWidths, int fSorted)
{
    ErrorF("xnest warning: function xnestFillSpans not implemented\n");
}

void
xnestSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *pSrc,
              xPoint * pPoints, int *pWidths, int nSpans, int fSorted)
{
    ErrorF("xnest warning: function xnestSetSpans not implemented\n");
}

void
xnestGetSpans(DrawablePtr pDrawable, int maxWidth, DDXPointPtr pPoints,
              int *pWidths, int nSpans, char *pBuffer)
{
    ErrorF("xnest warning: function xnestGetSpans not implemented\n");
}

void
xnestQueryBestSize(int class, unsigned short *pWidth, unsigned short *pHeight,
                   ScreenPtr pScreen)
{
    unsigned int width, height;

    width = *pWidth;
    height = *pHeight;

    XQueryBestSize(xnestDisplay, class,
                   xnestDefaultWindows[pScreen->myNum],
                   width, height, &width, &height);

    *pWidth = width;
    *pHeight = height;
}

void
xnestPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y,
              int w, int h, int leftPad, int format, char *pImage)
{
    XImage *ximage;

    ximage = XCreateImage(xnestDisplay, xnestDefaultVisual(pDrawable->pScreen),
                          depth, format, leftPad, (char *) pImage,
                          w, h, BitmapPad(xnestDisplay),
                          (format == ZPixmap) ?
                          PixmapBytePad(w, depth) : BitmapBytePad(w + leftPad));

    if (ximage) {
        XPutImage(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                  ximage, 0, 0, x, y, w, h);
        XFree(ximage);
    }
}

static int
xnestIgnoreErrorHandler (Display     *dpy,
                         XErrorEvent *event)
{
    return False; /* return value is ignored */
}

void
xnestGetImage(DrawablePtr pDrawable, int x, int y, int w, int h,
              unsigned int format, unsigned long planeMask, char *pImage)
{
    XImage *ximage;
    int length;
    int (*old_handler)(Display*, XErrorEvent*);

    /* we may get BadMatch error when xnest window is minimized */
    XSync(xnestDisplay, False);
    old_handler = XSetErrorHandler (xnestIgnoreErrorHandler);

    ximage = XGetImage(xnestDisplay, xnestDrawable(pDrawable),
                       x, y, w, h, planeMask, format);
    XSetErrorHandler(old_handler);

    if (ximage) {
        length = ximage->bytes_per_line * ximage->height;

        memmove(pImage, ximage->data, length);

        XDestroyImage(ximage);
    }
}

static Bool
xnestBitBlitPredicate(Display * dpy, XEvent * event, char *args)
{
    return event->type == GraphicsExpose || event->type == NoExpose;
}

static RegionPtr
xnestBitBlitHelper(GCPtr pGC)
{
    if (!pGC->graphicsExposures)
        return NullRegion;
    else {
        XEvent event;
        RegionPtr pReg, pTmpReg;
        BoxRec Box;
        Bool pending, overlap;

        pReg = RegionCreate(NULL, 1);
        pTmpReg = RegionCreate(NULL, 1);
        if (!pReg || !pTmpReg)
            return NullRegion;

        pending = True;
        while (pending) {
            XIfEvent(xnestDisplay, &event, xnestBitBlitPredicate, NULL);

            switch (event.type) {
            case NoExpose:
                pending = False;
                break;

            case GraphicsExpose:
                Box.x1 = event.xgraphicsexpose.x;
                Box.y1 = event.xgraphicsexpose.y;
                Box.x2 = event.xgraphicsexpose.x + event.xgraphicsexpose.width;
                Box.y2 = event.xgraphicsexpose.y + event.xgraphicsexpose.height;
                RegionReset(pTmpReg, &Box);
                RegionAppend(pReg, pTmpReg);
                pending = event.xgraphicsexpose.count;
                break;
            }
        }

        RegionDestroy(pTmpReg);
        RegionValidate(pReg, &overlap);
        return pReg;
    }
}

RegionPtr
xnestCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
              GCPtr pGC, int srcx, int srcy, int width, int height,
              int dstx, int dsty)
{
    XCopyArea(xnestDisplay,
              xnestDrawable(pSrcDrawable), xnestDrawable(pDstDrawable),
              xnestGC(pGC), srcx, srcy, width, height, dstx, dsty);

    return xnestBitBlitHelper(pGC);
}

RegionPtr
xnestCopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
               GCPtr pGC, int srcx, int srcy, int width, int height,
               int dstx, int dsty, unsigned long plane)
{
    XCopyPlane(xnestDisplay,
               xnestDrawable(pSrcDrawable), xnestDrawable(pDstDrawable),
               xnestGC(pGC), srcx, srcy, width, height, dstx, dsty, plane);

    return xnestBitBlitHelper(pGC);
}

void
xnestPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode, int nPoints,
               DDXPointPtr pPoints)
{
    XDrawPoints(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                (XPoint *) pPoints, nPoints, mode);
}

void
xnestPolylines(DrawablePtr pDrawable, GCPtr pGC, int mode, int nPoints,
               DDXPointPtr pPoints)
{
    XDrawLines(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
               (XPoint *) pPoints, nPoints, mode);
}

void
xnestPolySegment(DrawablePtr pDrawable, GCPtr pGC, int nSegments,
                 xSegment * pSegments)
{
    XDrawSegments(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                  (XSegment *) pSegments, nSegments);
}

void
xnestPolyRectangle(DrawablePtr pDrawable, GCPtr pGC, int nRectangles,
                   xRectangle *pRectangles)
{
    XDrawRectangles(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                    (XRectangle *) pRectangles, nRectangles);
}

void
xnestPolyArc(DrawablePtr pDrawable, GCPtr pGC, int nArcs, xArc * pArcs)
{
    XDrawArcs(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
              (XArc *) pArcs, nArcs);
}

void
xnestFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape, int mode,
                 int nPoints, DDXPointPtr pPoints)
{
    XFillPolygon(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                 (XPoint *) pPoints, nPoints, shape, mode);
}

void
xnestPolyFillRect(DrawablePtr pDrawable, GCPtr pGC, int nRectangles,
                  xRectangle *pRectangles)
{
    XFillRectangles(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                    (XRectangle *) pRectangles, nRectangles);
}

void
xnestPolyFillArc(DrawablePtr pDrawable, GCPtr pGC, int nArcs, xArc * pArcs)
{
    XFillArcs(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
              (XArc *) pArcs, nArcs);
}

int
xnestPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
               char *string)
{
    int width;

    XDrawString(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                x, y, string, count);

    width = XTextWidth(xnestFontStruct(pGC->font), string, count);

    return width + x;
}

int
xnestPolyText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                unsigned short *string)
{
    int width;

    XDrawString16(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                  x, y, (XChar2b *) string, count);

    width = XTextWidth16(xnestFontStruct(pGC->font), (XChar2b *) string, count);

    return width + x;
}

void
xnestImageText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                char *string)
{
    XDrawImageString(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                     x, y, string, count);
}

void
xnestImageText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                 unsigned short *string)
{
    XDrawImageString16(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                       x, y, (XChar2b *) string, count);
}

void
xnestImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                   unsigned int nGlyphs, CharInfoPtr * pCharInfo,
                   void *pGlyphBase)
{
    ErrorF("xnest warning: function xnestImageGlyphBlt not implemented\n");
}

void
xnestPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                  unsigned int nGlyphs, CharInfoPtr * pCharInfo,
                  void *pGlyphBase)
{
    ErrorF("xnest warning: function xnestPolyGlyphBlt not implemented\n");
}

void
xnestPushPixels(GCPtr pGC, PixmapPtr pBitmap, DrawablePtr pDst,
                int width, int height, int x, int y)
{
    /* only works for solid bitmaps */
    if (pGC->fillStyle == FillSolid) {
        XSetStipple(xnestDisplay, xnestGC(pGC), xnestPixmap(pBitmap));
        XSetTSOrigin(xnestDisplay, xnestGC(pGC), x, y);
        XSetFillStyle(xnestDisplay, xnestGC(pGC), FillStippled);
        XFillRectangle(xnestDisplay, xnestDrawable(pDst),
                       xnestGC(pGC), x, y, width, height);
        XSetFillStyle(xnestDisplay, xnestGC(pGC), FillSolid);
    }
    else
        ErrorF("xnest warning: function xnestPushPixels not implemented\n");
}
