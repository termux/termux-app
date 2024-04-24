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

#ifndef XNESTGCOPS_H
#define XNESTGCOPS_H

void xnestFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nSpans,
                    xPoint * pPoints, int *pWidths, int fSorted);
void xnestSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *pSrc,
                   xPoint * pPoints, int *pWidths, int nSpans, int fSorted);
void xnestGetSpans(DrawablePtr pDrawable, int maxWidth, DDXPointPtr pPoints,
                   int *pWidths, int nSpans, char *pBuffer);
void xnestQueryBestSize(int class, unsigned short *pWidth,
                        unsigned short *pHeight, ScreenPtr pScreen);
void xnestPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y,
                   int w, int h, int leftPad, int format, char *pImage);
void xnestGetImage(DrawablePtr pDrawable, int x, int y, int w, int h,
                   unsigned int format, unsigned long planeMask, char *pImage);
RegionPtr xnestCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
                        GCPtr pGC, int srcx, int srcy, int width, int height,
                        int dstx, int dsty);
RegionPtr xnestCopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
                         GCPtr pGC, int srcx, int srcy, int width, int height,
                         int dstx, int dsty, unsigned long plane);
void xnestPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode, int nPoints,
                    DDXPointPtr pPoints);
void xnestPolylines(DrawablePtr pDrawable, GCPtr pGC, int mode, int nPoints,
                    DDXPointPtr pPoints);
void xnestPolySegment(DrawablePtr pDrawable, GCPtr pGC, int nSegments,
                      xSegment * pSegments);
void xnestPolyRectangle(DrawablePtr pDrawable, GCPtr pGC, int nRectangles,
                        xRectangle *pRectangles);
void xnestPolyArc(DrawablePtr pDrawable, GCPtr pGC, int nArcs, xArc * pArcs);
void xnestFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape, int mode,
                      int nPoints, DDXPointPtr pPoints);
void xnestPolyFillRect(DrawablePtr pDrawable, GCPtr pGC, int nRectangles,
                       xRectangle *pRectangles);
void xnestPolyFillArc(DrawablePtr pDrawable, GCPtr pGC, int nArcs,
                      xArc * pArcs);
int xnestPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                   char *string);
int xnestPolyText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                    unsigned short *string);
void xnestImageText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                     char *string);
void xnestImageText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                      unsigned short *string);
void xnestImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                        unsigned int nGlyphs, CharInfoPtr * pCharInfo,
                        void *pGlyphBase);
void xnestPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                       unsigned int nGlyphs, CharInfoPtr * pCharInfo,
                       void *pGlyphBase);
void xnestPushPixels(GCPtr pGC, PixmapPtr pBitmap, DrawablePtr pDrawable,
                     int width, int height, int x, int y);

#endif                          /* XNESTGCOPS_H */
