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
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "gc.h"
#include "servermd.h"
#include "privates.h"
#include "mi.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "XNPixmap.h"

DevPrivateKeyRec xnestPixmapPrivateKeyRec;

PixmapPtr
xnestCreatePixmap(ScreenPtr pScreen, int width, int height, int depth,
                  unsigned usage_hint)
{
    PixmapPtr pPixmap;

    pPixmap = AllocatePixmap(pScreen, 0);
    if (!pPixmap)
        return NullPixmap;
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.class = 0;
    pPixmap->drawable.depth = depth;
    pPixmap->drawable.bitsPerPixel = depth;
    pPixmap->drawable.id = 0;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = width;
    pPixmap->drawable.height = height;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->refcnt = 1;
    pPixmap->devKind = PixmapBytePad(width, depth);
    pPixmap->usage_hint = usage_hint;
    if (width && height)
        xnestPixmapPriv(pPixmap)->pixmap =
            XCreatePixmap(xnestDisplay,
                          xnestDefaultWindows[pScreen->myNum],
                          width, height, depth);
    else
        xnestPixmapPriv(pPixmap)->pixmap = 0;

    return pPixmap;
}

Bool
xnestDestroyPixmap(PixmapPtr pPixmap)
{
    if (--pPixmap->refcnt)
        return TRUE;
    XFreePixmap(xnestDisplay, xnestPixmap(pPixmap));
    FreePixmap(pPixmap);
    return TRUE;
}

Bool
xnestModifyPixmapHeader(PixmapPtr pPixmap, int width, int height, int depth,
                        int bitsPerPixel, int devKind, void *pPixData)
{
  if(!xnestPixmapPriv(pPixmap)->pixmap && width > 0 && height > 0) {
    xnestPixmapPriv(pPixmap)->pixmap =
        XCreatePixmap(xnestDisplay,
                      xnestDefaultWindows[pPixmap->drawable.pScreen->myNum],
                      width, height, depth);
  }

  return miModifyPixmapHeader(pPixmap, width, height, depth,
                              bitsPerPixel, devKind, pPixData);
}

RegionPtr
xnestPixmapToRegion(PixmapPtr pPixmap)
{
    XImage *ximage;
    register RegionPtr pReg, pTmpReg;
    register int x, y;
    unsigned long previousPixel, currentPixel;
    BoxRec Box = { 0, 0, 0, 0 };
    Bool overlap;

    ximage = XGetImage(xnestDisplay, xnestPixmap(pPixmap), 0, 0,
                       pPixmap->drawable.width, pPixmap->drawable.height,
                       1, XYPixmap);

    pReg = RegionCreate(NULL, 1);
    pTmpReg = RegionCreate(NULL, 1);
    if (!pReg || !pTmpReg) {
        XDestroyImage(ximage);
        return NullRegion;
    }

    for (y = 0; y < pPixmap->drawable.height; y++) {
        Box.y1 = y;
        Box.y2 = y + 1;
        previousPixel = 0L;
        for (x = 0; x < pPixmap->drawable.width; x++) {
            currentPixel = XGetPixel(ximage, x, y);
            if (previousPixel != currentPixel) {
                if (previousPixel == 0L) {
                    /* left edge */
                    Box.x1 = x;
                }
                else if (currentPixel == 0L) {
                    /* right edge */
                    Box.x2 = x;
                    RegionReset(pTmpReg, &Box);
                    RegionAppend(pReg, pTmpReg);
                }
                previousPixel = currentPixel;
            }
        }
        if (previousPixel != 0L) {
            /* right edge because of the end of pixmap */
            Box.x2 = pPixmap->drawable.width;
            RegionReset(pTmpReg, &Box);
            RegionAppend(pReg, pTmpReg);
        }
    }

    RegionDestroy(pTmpReg);
    XDestroyImage(ximage);

    RegionValidate(pReg, &overlap);

    return pReg;
}
