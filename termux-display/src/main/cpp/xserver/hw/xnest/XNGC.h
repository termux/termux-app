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

#ifndef XNESTGC_H
#define XNESTGC_H

/* This file uses the GC definition form Xlib.h as XlibGC. */

typedef struct {
    XlibGC gc;
} xnestPrivGC;

extern DevPrivateKeyRec xnestGCPrivateKeyRec;

#define xnestGCPrivateKey (&xnestGCPrivateKeyRec)

#define xnestGCPriv(pGC) ((xnestPrivGC *) \
    dixLookupPrivate(&(pGC)->devPrivates, xnestGCPrivateKey))

#define xnestGC(pGC) (xnestGCPriv(pGC)->gc)

Bool xnestCreateGC(GCPtr pGC);
void xnestValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable);
void xnestChangeGC(GCPtr pGC, unsigned long mask);
void xnestCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);
void xnestDestroyGC(GCPtr pGC);
void xnestChangeClip(GCPtr pGC, int type, void *pValue, int nRects);
void xnestDestroyClip(GCPtr pGC);
void xnestCopyClip(GCPtr pGCDst, GCPtr pGCSrc);

#endif                          /* XNESTGC_H */
