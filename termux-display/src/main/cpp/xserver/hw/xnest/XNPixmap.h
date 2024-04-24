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

#ifndef XNESTPIXMAP_H
#define XNESTPIXMAP_H

extern DevPrivateKeyRec xnestPixmapPrivateKeyRec;

#define xnestPixmapPrivateKey (&xnestPixmapPrivateKeyRec)

typedef struct {
    Pixmap pixmap;
} xnestPrivPixmap;

#define xnestPixmapPriv(pPixmap) ((xnestPrivPixmap *) \
    dixLookupPrivate(&(pPixmap)->devPrivates, xnestPixmapPrivateKey))

#define xnestPixmap(pPixmap) (xnestPixmapPriv(pPixmap)->pixmap)

#define xnestSharePixmap(pPixmap) ((pPixmap)->refcnt++)

PixmapPtr xnestCreatePixmap(ScreenPtr pScreen, int width, int height,
                            int depth, unsigned usage_hint);
Bool xnestDestroyPixmap(PixmapPtr pPixmap);
Bool xnestModifyPixmapHeader(PixmapPtr pPixmap, int width, int height, int depth,
                             int bitsPerPixel, int devKind, void *pPixData);
RegionPtr xnestPixmapToRegion(PixmapPtr pPixmap);

#endif                          /* XNESTPIXMAP_H */
