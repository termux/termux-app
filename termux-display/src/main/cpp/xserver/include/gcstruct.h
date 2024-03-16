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

#ifndef GCSTRUCT_H
#define GCSTRUCT_H

#include "gc.h"

#include "regionstr.h"
#include "region.h"
#include "pixmap.h"
#include "screenint.h"
#include "privates.h"
#include <X11/Xprotostr.h>

#define GCAllBits ((1 << (GCLastBit + 1)) - 1)

/*
 * functions which modify the state of the GC
 */

typedef struct _GCFuncs {
    void (*ValidateGC) (GCPtr /*pGC */ ,
                        unsigned long /*stateChanges */ ,
                        DrawablePtr /*pDrawable */ );

    void (*ChangeGC) (GCPtr /*pGC */ ,
                      unsigned long /*mask */ );

    void (*CopyGC) (GCPtr /*pGCSrc */ ,
                    unsigned long /*mask */ ,
                    GCPtr /*pGCDst */ );

    void (*DestroyGC) (GCPtr /*pGC */ );

    void (*ChangeClip) (GCPtr pGC,
                        int type,
                        void *pvalue,
                        int nrects);

    void (*DestroyClip) (GCPtr /*pGC */ );

    void (*CopyClip) (GCPtr /*pgcDst */ ,
                      GCPtr /*pgcSrc */ );
} GCFuncs;

/*
 * graphics operations invoked through a GC
 */

typedef struct _GCOps {
    void (*FillSpans) (DrawablePtr /*pDrawable */ ,
                       GCPtr /*pGC */ ,
                       int /*nInit */ ,
                       DDXPointPtr /*pptInit */ ,
                       int * /*pwidthInit */ ,
                       int /*fSorted */ );

    void (*SetSpans) (DrawablePtr /*pDrawable */ ,
                      GCPtr /*pGC */ ,
                      char * /*psrc */ ,
                      DDXPointPtr /*ppt */ ,
                      int * /*pwidth */ ,
                      int /*nspans */ ,
                      int /*fSorted */ );

    void (*PutImage) (DrawablePtr /*pDrawable */ ,
                      GCPtr /*pGC */ ,
                      int /*depth */ ,
                      int /*x */ ,
                      int /*y */ ,
                      int /*w */ ,
                      int /*h */ ,
                      int /*leftPad */ ,
                      int /*format */ ,
                      char * /*pBits */ );

    RegionPtr (*CopyArea) (DrawablePtr /*pSrc */ ,
                           DrawablePtr /*pDst */ ,
                           GCPtr /*pGC */ ,
                           int /*srcx */ ,
                           int /*srcy */ ,
                           int /*w */ ,
                           int /*h */ ,
                           int /*dstx */ ,
                           int /*dsty */ );

    RegionPtr (*CopyPlane) (DrawablePtr /*pSrcDrawable */ ,
                            DrawablePtr /*pDstDrawable */ ,
                            GCPtr /*pGC */ ,
                            int /*srcx */ ,
                            int /*srcy */ ,
                            int /*width */ ,
                            int /*height */ ,
                            int /*dstx */ ,
                            int /*dsty */ ,
                            unsigned long /*bitPlane */ );
    void (*PolyPoint) (DrawablePtr /*pDrawable */ ,
                       GCPtr /*pGC */ ,
                       int /*mode */ ,
                       int /*npt */ ,
                       DDXPointPtr /*pptInit */ );

    void (*Polylines) (DrawablePtr /*pDrawable */ ,
                       GCPtr /*pGC */ ,
                       int /*mode */ ,
                       int /*npt */ ,
                       DDXPointPtr /*pptInit */ );

    void (*PolySegment) (DrawablePtr /*pDrawable */ ,
                         GCPtr /*pGC */ ,
                         int /*nseg */ ,
                         xSegment * /*pSegs */ );

    void (*PolyRectangle) (DrawablePtr /*pDrawable */ ,
                           GCPtr /*pGC */ ,
                           int /*nrects */ ,
                           xRectangle * /*pRects */ );

    void (*PolyArc) (DrawablePtr /*pDrawable */ ,
                     GCPtr /*pGC */ ,
                     int /*narcs */ ,
                     xArc * /*parcs */ );

    void (*FillPolygon) (DrawablePtr /*pDrawable */ ,
                         GCPtr /*pGC */ ,
                         int /*shape */ ,
                         int /*mode */ ,
                         int /*count */ ,
                         DDXPointPtr /*pPts */ );

    void (*PolyFillRect) (DrawablePtr /*pDrawable */ ,
                          GCPtr /*pGC */ ,
                          int /*nrectFill */ ,
                          xRectangle * /*prectInit */ );

    void (*PolyFillArc) (DrawablePtr /*pDrawable */ ,
                         GCPtr /*pGC */ ,
                         int /*narcs */ ,
                         xArc * /*parcs */ );

    int (*PolyText8) (DrawablePtr /*pDrawable */ ,
                      GCPtr /*pGC */ ,
                      int /*x */ ,
                      int /*y */ ,
                      int /*count */ ,
                      char * /*chars */ );

    int (*PolyText16) (DrawablePtr /*pDrawable */ ,
                       GCPtr /*pGC */ ,
                       int /*x */ ,
                       int /*y */ ,
                       int /*count */ ,
                       unsigned short * /*chars */ );

    void (*ImageText8) (DrawablePtr /*pDrawable */ ,
                        GCPtr /*pGC */ ,
                        int /*x */ ,
                        int /*y */ ,
                        int /*count */ ,
                        char * /*chars */ );

    void (*ImageText16) (DrawablePtr /*pDrawable */ ,
                         GCPtr /*pGC */ ,
                         int /*x */ ,
                         int /*y */ ,
                         int /*count */ ,
                         unsigned short * /*chars */ );

    void (*ImageGlyphBlt) (DrawablePtr pDrawable,
                           GCPtr pGC,
                           int x,
                           int y,
                           unsigned int nglyph,
                           CharInfoPtr *ppci,
                           void *pglyphBase);

    void (*PolyGlyphBlt) (DrawablePtr pDrawable,
                          GCPtr pGC,
                          int x,
                          int y,
                          unsigned int nglyph,
                          CharInfoPtr *ppci,
                          void *pglyphBase);

    void (*PushPixels) (GCPtr /*pGC */ ,
                        PixmapPtr /*pBitMap */ ,
                        DrawablePtr /*pDst */ ,
                        int /*w */ ,
                        int /*h */ ,
                        int /*x */ ,
                        int /*y */ );
} GCOps;

/* there is padding in the bit fields because the Sun compiler doesn't
 * force alignment to 32-bit boundaries.  losers.
 */
typedef struct _GC {
    ScreenPtr pScreen;
    unsigned char depth;
    unsigned char alu;
    unsigned short lineWidth;
    unsigned short dashOffset;
    unsigned short numInDashList;
    unsigned char *dash;
    unsigned int lineStyle:2;
    unsigned int capStyle:2;
    unsigned int joinStyle:2;
    unsigned int fillStyle:2;
    unsigned int fillRule:1;
    unsigned int arcMode:1;
    unsigned int subWindowMode:1;
    unsigned int graphicsExposures:1;
    unsigned int miTranslate:1; /* should mi things translate? */
    unsigned int tileIsPixel:1; /* tile is solid pixel */
    unsigned int fExpose:1;     /* Call exposure handling */
    unsigned int freeCompClip:1;        /* Free composite clip */
    unsigned int scratch_inuse:1;       /* is this GC in a pool for reuse? */
    unsigned int unused:15;     /* see comment above */
    unsigned int planemask;
    unsigned int fgPixel;
    unsigned int bgPixel;
    /*
     * alas -- both tile and stipple must be here as they
     * are independently specifiable
     */
    PixUnion tile;
    PixmapPtr stipple;
    DDXPointRec patOrg;         /* origin for (tile, stipple) */
    DDXPointRec clipOrg;
    struct _Font *font;
    RegionPtr clientClip;
    unsigned int stateChanges; /* masked with GC_<kind> */
    unsigned int serialNumber;
    const GCFuncs *funcs;
    const GCOps *ops;
    PrivateRec *devPrivates;
    RegionPtr pCompositeClip;
} GC;

#endif                          /* GCSTRUCT_H */
