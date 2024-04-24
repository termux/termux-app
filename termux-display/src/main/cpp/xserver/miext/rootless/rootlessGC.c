/*
 * Graphics Context support for generic rootless X server
 */
/*
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
 * Copyright (c) 2002-2003 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stddef.h>             /* For NULL */
#include "mi.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "dixfontstr.h"
#include "mivalidate.h"
#include "fb.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rootlessCommon.h"

// GC functions
static void RootlessValidateGC(GCPtr pGC, unsigned long changes,
                               DrawablePtr pDrawable);
static void RootlessChangeGC(GCPtr pGC, unsigned long mask);
static void RootlessCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);
static void RootlessDestroyGC(GCPtr pGC);
static void RootlessChangeClip(GCPtr pGC, int type, void *pvalue, int nrects);
static void RootlessDestroyClip(GCPtr pGC);
static void RootlessCopyClip(GCPtr pgcDst, GCPtr pgcSrc);

Bool RootlessCreateGC(GCPtr pGC);

GCFuncs rootlessGCFuncs = {
    RootlessValidateGC,
    RootlessChangeGC,
    RootlessCopyGC,
    RootlessDestroyGC,
    RootlessChangeClip,
    RootlessDestroyClip,
    RootlessCopyClip,
};

// GC operations
static void RootlessFillSpans(DrawablePtr dst, GCPtr pGC, int nInit,
                              DDXPointPtr pptInit, int *pwidthInit, int sorted);
static void RootlessSetSpans(DrawablePtr dst, GCPtr pGC, char *pSrc,
                             DDXPointPtr pptInit, int *pwidthInit,
                             int nspans, int sorted);
static void RootlessPutImage(DrawablePtr dst, GCPtr pGC,
                             int depth, int x, int y, int w, int h,
                             int leftPad, int format, char *pBits);
static RegionPtr RootlessCopyArea(DrawablePtr pSrc, DrawablePtr dst, GCPtr pGC,
                                  int srcx, int srcy, int w, int h,
                                  int dstx, int dsty);
static RegionPtr RootlessCopyPlane(DrawablePtr pSrc, DrawablePtr dst,
                                   GCPtr pGC, int srcx, int srcy,
                                   int w, int h, int dstx, int dsty,
                                   unsigned long plane);
static void RootlessPolyPoint(DrawablePtr dst, GCPtr pGC,
                              int mode, int npt, DDXPointPtr pptInit);
static void RootlessPolylines(DrawablePtr dst, GCPtr pGC,
                              int mode, int npt, DDXPointPtr pptInit);
static void RootlessPolySegment(DrawablePtr dst, GCPtr pGC,
                                int nseg, xSegment * pSeg);
static void RootlessPolyRectangle(DrawablePtr dst, GCPtr pGC,
                                  int nRects, xRectangle *pRects);
static void RootlessPolyArc(DrawablePtr dst, GCPtr pGC, int narcs,
                            xArc * parcs);
static void RootlessFillPolygon(DrawablePtr dst, GCPtr pGC, int shape, int mode,
                                int count, DDXPointPtr pptInit);
static void RootlessPolyFillRect(DrawablePtr dst, GCPtr pGC, int nRectsInit,
                                 xRectangle *pRectsInit);
static void RootlessPolyFillArc(DrawablePtr dst, GCPtr pGC, int narcsInit,
                                xArc * parcsInit);
static int RootlessPolyText8(DrawablePtr dst, GCPtr pGC, int x, int y,
                             int count, char *chars);
static int RootlessPolyText16(DrawablePtr dst, GCPtr pGC, int x, int y,
                              int count, unsigned short *chars);
static void RootlessImageText8(DrawablePtr dst, GCPtr pGC, int x, int y,
                               int count, char *chars);
static void RootlessImageText16(DrawablePtr dst, GCPtr pGC, int x, int y,
                                int count, unsigned short *chars);
static void RootlessImageGlyphBlt(DrawablePtr dst, GCPtr pGC, int x, int y,
                                  unsigned int nglyphInit,
                                  CharInfoPtr * ppciInit, void *unused);
static void RootlessPolyGlyphBlt(DrawablePtr dst, GCPtr pGC, int x, int y,
                                 unsigned int nglyph, CharInfoPtr * ppci,
                                 void *pglyphBase);
static void RootlessPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr dst,
                               int dx, int dy, int xOrg, int yOrg);

static GCOps rootlessGCOps = {
    RootlessFillSpans,
    RootlessSetSpans,
    RootlessPutImage,
    RootlessCopyArea,
    RootlessCopyPlane,
    RootlessPolyPoint,
    RootlessPolylines,
    RootlessPolySegment,
    RootlessPolyRectangle,
    RootlessPolyArc,
    RootlessFillPolygon,
    RootlessPolyFillRect,
    RootlessPolyFillArc,
    RootlessPolyText8,
    RootlessPolyText16,
    RootlessImageText8,
    RootlessImageText16,
    RootlessImageGlyphBlt,
    RootlessPolyGlyphBlt,
    RootlessPushPixels
};

/*
   If ROOTLESS_PROTECT_ALPHA is set, we have to make sure that the alpha
   channel of the on screen windows is always opaque. fb makes this harder
   than it would otherwise be by noticing that a planemask of 0x00ffffff
   includes all bits when depth==24, and so it "optimizes" the planemask to
   0xffffffff. We work around this by temporarily setting depth=bpp while
   changing the GC.

   So the normal situation (in 32 bit mode) is that the planemask is
   0x00ffffff and thus fb leaves the alpha channel alone. The rootless
   implementation is responsible for setting the alpha channel opaque
   initially.

   Unfortunately drawing with a planemask that doesn't have all bits set
   normally causes fb to fall off its fastest paths when blitting and
   filling.  So we try to recognize when we can relax the planemask back to
   0xffffffff, and do that for the duration of the drawing operation,
   setting the alpha channel in fg/bg pixels to opaque at the same time. We
   can do this when drawing op is GXcopy. We can also do this when copying
   from another window since its alpha channel must also be opaque.

   The three macros below are used to implement this. Drawing ops that can
   potentially have their planemask relaxed look like:

   OP {
       GC_SAVE(gc);
       GCOP_UNWRAP(gc);

       ...

       if (canAccelxxx(..) && otherwise-suitable)
            GC_UNSET_PM(gc, dst);

       gc->funcs->OP(gc, ...);

       GC_RESTORE(gc, dst);
       GCOP_WRAP(gc);
   }

 */

#define GC_SAVE(pGC) 				\
    unsigned long _save_fg = (pGC)->fgPixel;	\
    unsigned long _save_bg = (pGC)->bgPixel;	\
    unsigned long _save_pm = (pGC)->planemask;	\
    Bool _changed = FALSE

#define GC_RESTORE(pGC, pDraw)					\
    do {							\
        if (_changed) {						\
            unsigned int depth = (pDraw)->depth;		\
            (pGC)->fgPixel = _save_fg;				\
            (pGC)->bgPixel = _save_bg;				\
            (pGC)->planemask = _save_pm;			\
            (pDraw)->depth = (pDraw)->bitsPerPixel;		\
            VALIDATE_GC(pGC, GCForeground | GCBackground |	\
                        GCPlaneMask, pDraw);			\
            (pDraw)->depth = depth;				\
        }							\
    } while (0)

#define GC_UNSET_PM(pGC, pDraw)						\
    do {								\
        unsigned int mask = RootlessAlphaMask ((pDraw)->bitsPerPixel);	\
        if (((pGC)->planemask & mask) != mask) {			\
            unsigned int depth = (pDraw)->depth;			\
            (pGC)->fgPixel |= mask;					\
            (pGC)->bgPixel |= mask;					\
            (pGC)->planemask |= mask;					\
            (pDraw)->depth = (pDraw)->bitsPerPixel;			\
            VALIDATE_GC(pGC, GCForeground |				\
                        GCBackground | GCPlaneMask, pDraw);		\
            (pDraw)->depth = depth;					\
            _changed = TRUE;						\
        }								\
    } while (0)

#define VALIDATE_GC(pGC, changes, pDrawable)				\
    do {								\
        pGC->funcs->ValidateGC(pGC, changes, pDrawable);		\
        if (((WindowPtr) pDrawable)->viewable) {			\
            gcrec->originalOps = pGC->ops;				\
        }								\
    } while(0)

static RootlessWindowRec *
canAccelBlit(DrawablePtr pDraw, GCPtr pGC)
{
    WindowPtr pTop;
    RootlessWindowRec *winRec;
    unsigned int pm;

    if (pGC->alu != GXcopy)
        return NULL;

    if (pDraw->type != DRAWABLE_WINDOW)
        return NULL;

    pm = ~RootlessAlphaMask(pDraw->bitsPerPixel);
    if ((pGC->planemask & pm) != pm)
        return NULL;

    pTop = TopLevelParent((WindowPtr) pDraw);
    if (pTop == NULL)
        return NULL;

    winRec = WINREC(pTop);
    if (winRec == NULL)
        return NULL;

    return winRec;
}

static inline RootlessWindowRec *
canAccelFill(DrawablePtr pDraw, GCPtr pGC)
{
    if (pGC->fillStyle != FillSolid)
        return NULL;

    return canAccelBlit(pDraw, pGC);
}

/*
 * Screen function to create a graphics context
 */
Bool
RootlessCreateGC(GCPtr pGC)
{
    RootlessGCRec *gcrec;
    RootlessScreenRec *s;
    Bool result;

    SCREEN_UNWRAP(pGC->pScreen, CreateGC);
    s = SCREENREC(pGC->pScreen);
    result = s->CreateGC(pGC);

    gcrec = (RootlessGCRec *)
        dixLookupPrivate(&pGC->devPrivates, rootlessGCPrivateKey);
    gcrec->originalOps = NULL;  // don't wrap ops yet
    gcrec->originalFuncs = pGC->funcs;
    pGC->funcs = &rootlessGCFuncs;

    SCREEN_WRAP(pGC->pScreen, CreateGC);
    return result;
}

/*
 * GC funcs
 *
 * These wrap lower level GC funcs.
 * ValidateGC wraps the GC ops iff dest is viewable.
 * All the others just unwrap and call.
 */

// GCFUNC_UNRAP assumes funcs have been wrapped and
// does not assume ops have been wrapped
#define GCFUNC_UNWRAP(pGC) \
    RootlessGCRec *gcrec = (RootlessGCRec *) \
	dixLookupPrivate(&(pGC)->devPrivates, rootlessGCPrivateKey); \
    (pGC)->funcs = gcrec->originalFuncs; \
    if (gcrec->originalOps) { \
        (pGC)->ops = gcrec->originalOps; \
}

#define GCFUNC_WRAP(pGC) \
    gcrec->originalFuncs = (pGC)->funcs; \
    (pGC)->funcs = &rootlessGCFuncs; \
    if (gcrec->originalOps) { \
        gcrec->originalOps = (pGC)->ops; \
        (pGC)->ops = &rootlessGCOps; \
}

static void
RootlessValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
    GCFUNC_UNWRAP(pGC);

    gcrec->originalOps = NULL;

    if (pDrawable->type == DRAWABLE_WINDOW) {
#ifdef ROOTLESS_PROTECT_ALPHA
        unsigned int depth = pDrawable->depth;

        // We force a planemask so fb doesn't overwrite the alpha channel.
        // Left to its own devices, fb will optimize away the planemask.
        pDrawable->depth = pDrawable->bitsPerPixel;
        pGC->planemask &= ~RootlessAlphaMask(pDrawable->bitsPerPixel);
        VALIDATE_GC(pGC, changes | GCPlaneMask, pDrawable);
        pDrawable->depth = depth;
#else
        VALIDATE_GC(pGC, changes, pDrawable);
#endif
    }
    else {
        pGC->funcs->ValidateGC(pGC, changes, pDrawable);
    }

    GCFUNC_WRAP(pGC);
}

static void
RootlessChangeGC(GCPtr pGC, unsigned long mask)
{
    GCFUNC_UNWRAP(pGC);
    pGC->funcs->ChangeGC(pGC, mask);
    GCFUNC_WRAP(pGC);
}

static void
RootlessCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    GCFUNC_UNWRAP(pGCDst);
    pGCDst->funcs->CopyGC(pGCSrc, mask, pGCDst);
    GCFUNC_WRAP(pGCDst);
}

static void
RootlessDestroyGC(GCPtr pGC)
{
    GCFUNC_UNWRAP(pGC);
    pGC->funcs->DestroyGC(pGC);
    GCFUNC_WRAP(pGC);
}

static void
RootlessChangeClip(GCPtr pGC, int type, void *pvalue, int nrects)
{
    GCFUNC_UNWRAP(pGC);
    pGC->funcs->ChangeClip(pGC, type, pvalue, nrects);
    GCFUNC_WRAP(pGC);
}

static void
RootlessDestroyClip(GCPtr pGC)
{
    GCFUNC_UNWRAP(pGC);
    pGC->funcs->DestroyClip(pGC);
    GCFUNC_WRAP(pGC);
}

static void
RootlessCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    GCFUNC_UNWRAP(pgcDst);
    pgcDst->funcs->CopyClip(pgcDst, pgcSrc);
    GCFUNC_WRAP(pgcDst);
}

/*
 * GC ops
 *
 * We can't use shadowfb because shadowfb assumes one pixmap
 * and our root window is a special case.
 * However, much of this code is copied from shadowfb.
 */

// assumes both funcs and ops are wrapped
#define GCOP_UNWRAP(pGC) \
    RootlessGCRec *gcrec = (RootlessGCRec *) \
        dixLookupPrivate(&(pGC)->devPrivates, rootlessGCPrivateKey); \
    const GCFuncs *saveFuncs = pGC->funcs; \
    (pGC)->funcs = gcrec->originalFuncs; \
    (pGC)->ops = gcrec->originalOps;

#define GCOP_WRAP(pGC) \
    gcrec->originalOps = (pGC)->ops; \
    (pGC)->funcs = saveFuncs; \
    (pGC)->ops = &rootlessGCOps;

static void
RootlessFillSpans(DrawablePtr dst, GCPtr pGC, int nInit,
                  DDXPointPtr pptInit, int *pwidthInit, int sorted)
{
    GC_SAVE(pGC);
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("fill spans start ");

    if (nInit <= 0) {
        pGC->ops->FillSpans(dst, pGC, nInit, pptInit, pwidthInit, sorted);
    }
    else {
        DDXPointPtr ppt = pptInit;
        int *pwidth = pwidthInit;
        int i = nInit;
        BoxRec box;

        box.x1 = ppt->x;
        box.x2 = box.x1 + *pwidth;
        box.y2 = box.y1 = ppt->y;

        while (--i) {
            ppt++;
            pwidth++;
            if (box.x1 > ppt->x)
                box.x1 = ppt->x;
            if (box.x2 < (ppt->x + *pwidth))
                box.x2 = ppt->x + *pwidth;
            if (box.y1 > ppt->y)
                box.y1 = ppt->y;
            else if (box.y2 < ppt->y)
                box.y2 = ppt->y;
        }

        box.y2++;

        RootlessStartDrawing((WindowPtr) dst);

        if (canAccelFill(dst, pGC)) {
            GC_UNSET_PM(pGC, dst);
        }

        pGC->ops->FillSpans(dst, pGC, nInit, pptInit, pwidthInit, sorted);

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GC_RESTORE(pGC, dst);
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("fill spans end\n");
}

static void
RootlessSetSpans(DrawablePtr dst, GCPtr pGC, char *pSrc,
                 DDXPointPtr pptInit, int *pwidthInit, int nspans, int sorted)
{
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("set spans start ");

    if (nspans <= 0) {
        pGC->ops->SetSpans(dst, pGC, pSrc, pptInit, pwidthInit, nspans, sorted);
    }
    else {
        DDXPointPtr ppt = pptInit;
        int *pwidth = pwidthInit;
        int i = nspans;
        BoxRec box;

        box.x1 = ppt->x;
        box.x2 = box.x1 + *pwidth;
        box.y2 = box.y1 = ppt->y;

        while (--i) {
            ppt++;
            pwidth++;
            if (box.x1 > ppt->x)
                box.x1 = ppt->x;
            if (box.x2 < (ppt->x + *pwidth))
                box.x2 = ppt->x + *pwidth;
            if (box.y1 > ppt->y)
                box.y1 = ppt->y;
            else if (box.y2 < ppt->y)
                box.y2 = ppt->y;
        }

        box.y2++;

        RootlessStartDrawing((WindowPtr) dst);
        pGC->ops->SetSpans(dst, pGC, pSrc, pptInit, pwidthInit, nspans, sorted);

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("set spans end\n");
}

static void
RootlessPutImage(DrawablePtr dst, GCPtr pGC,
                 int depth, int x, int y, int w, int h,
                 int leftPad, int format, char *pBits)
{
    BoxRec box;

    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("put image start ");

    RootlessStartDrawing((WindowPtr) dst);
    pGC->ops->PutImage(dst, pGC, depth, x, y, w, h, leftPad, format, pBits);

    box.x1 = x + dst->x;
    box.x2 = box.x1 + w;
    box.y1 = y + dst->y;
    box.y2 = box.y1 + h;

    TRIM_BOX(box, pGC);
    if (BOX_NOT_EMPTY(box))
        RootlessDamageBox((WindowPtr) dst, &box);

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("put image end\n");
}

/* changed area is *dest* rect */
static RegionPtr
RootlessCopyArea(DrawablePtr pSrc, DrawablePtr dst, GCPtr pGC,
                 int srcx, int srcy, int w, int h, int dstx, int dsty)
{
    RegionPtr result;
    BoxRec box;

    GC_SAVE(pGC);
    GCOP_UNWRAP(pGC);

    RL_DEBUG_MSG("copy area start (src 0x%x, dst 0x%x)", pSrc, dst);

    if (pSrc->type == DRAWABLE_WINDOW && IsFramedWindow((WindowPtr) pSrc)) {
        /* If both source and dest are windows, and we're doing
           a simple copy operation, we can remove the alpha-protecting
           planemask (since source has opaque alpha as well) */

        if (canAccelBlit(pSrc, pGC)) {
            GC_UNSET_PM(pGC, dst);
        }

        RootlessStartDrawing((WindowPtr) pSrc);
    }
    RootlessStartDrawing((WindowPtr) dst);
    result = pGC->ops->CopyArea(pSrc, dst, pGC, srcx, srcy, w, h, dstx, dsty);

    box.x1 = dstx + dst->x;
    box.x2 = box.x1 + w;
    box.y1 = dsty + dst->y;
    box.y2 = box.y1 + h;

    TRIM_BOX(box, pGC);
    if (BOX_NOT_EMPTY(box))
        RootlessDamageBox((WindowPtr) dst, &box);

    GC_RESTORE(pGC, dst);
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("copy area end\n");
    return result;
}

/* changed area is *dest* rect */
static RegionPtr
RootlessCopyPlane(DrawablePtr pSrc, DrawablePtr dst,
                  GCPtr pGC, int srcx, int srcy,
                  int w, int h, int dstx, int dsty, unsigned long plane)
{
    RegionPtr result;
    BoxRec box;

    GCOP_UNWRAP(pGC);

    RL_DEBUG_MSG("copy plane start ");

    if (pSrc->type == DRAWABLE_WINDOW && IsFramedWindow((WindowPtr) pSrc)) {
        RootlessStartDrawing((WindowPtr) pSrc);
    }
    RootlessStartDrawing((WindowPtr) dst);
    result = pGC->ops->CopyPlane(pSrc, dst, pGC, srcx, srcy, w, h,
                                 dstx, dsty, plane);

    box.x1 = dstx + dst->x;
    box.x2 = box.x1 + w;
    box.y1 = dsty + dst->y;
    box.y2 = box.y1 + h;

    TRIM_BOX(box, pGC);
    if (BOX_NOT_EMPTY(box))
        RootlessDamageBox((WindowPtr) dst, &box);

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("copy plane end\n");
    return result;
}

// Options for size of changed area:
//  0 = box per point
//  1 = big box around all points
//  2 = accumulate point in 20 pixel radius
#define ROOTLESS_CHANGED_AREA 1
#define abs(a) ((a) > 0 ? (a) : -(a))

/* changed area is box around all points */
static void
RootlessPolyPoint(DrawablePtr dst, GCPtr pGC,
                  int mode, int npt, DDXPointPtr pptInit)
{
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("polypoint start ");

    RootlessStartDrawing((WindowPtr) dst);
    pGC->ops->PolyPoint(dst, pGC, mode, npt, pptInit);

    if (npt > 0) {
#if ROOTLESS_CHANGED_AREA==0
        // box per point
        BoxRec box;

        while (npt) {
            box.x1 = pptInit->x;
            box.y1 = pptInit->y;
            box.x2 = box.x1 + 1;
            box.y2 = box.y1 + 1;

            TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
            if (BOX_NOT_EMPTY(box))
                RootlessDamageBox((WindowPtr) dst, &box);

            npt--;
            pptInit++;
        }

#elif ROOTLESS_CHANGED_AREA==1
        // one big box
        BoxRec box;

        box.x2 = box.x1 = pptInit->x;
        box.y2 = box.y1 = pptInit->y;
        while (--npt) {
            pptInit++;
            if (box.x1 > pptInit->x)
                box.x1 = pptInit->x;
            else if (box.x2 < pptInit->x)
                box.x2 = pptInit->x;
            if (box.y1 > pptInit->y)
                box.y1 = pptInit->y;
            else if (box.y2 < pptInit->y)
                box.y2 = pptInit->y;
        }

        box.x2++;
        box.y2++;

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);

#elif ROOTLESS_CHANGED_AREA==2
        // clever(?) method: accumulate point in 20-pixel radius
        BoxRec box;
        int firstx, firsty;

        box.x2 = box.x1 = firstx = pptInit->x;
        box.y2 = box.y1 = firsty = pptInit->y;
        while (--npt) {
            pptInit++;
            if (abs(pptInit->x - firstx) > 20 || abs(pptInit->y - firsty) > 20) {
                box.x2++;
                box.y2++;
                TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
                if (BOX_NOT_EMPTY(box))
                    RootlessDamageBox((WindowPtr) dst, &box);
                box.x2 = box.x1 = firstx = pptInit->x;
                box.y2 = box.y1 = firsty = pptInit->y;
            }
            else {
                if (box.x1 > pptInit->x)
                    box.x1 = pptInit->x;
                else if (box.x2 < pptInit->x)
                    box.x2 = pptInit->x;
                if (box.y1 > pptInit->y)
                    box.y1 = pptInit->y;
                else if (box.y2 < pptInit->y)
                    box.y2 = pptInit->y;
            }
        }
        box.x2++;
        box.y2++;
        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
#endif                          /* ROOTLESS_CHANGED_AREA */
    }

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("polypoint end\n");
}

#undef ROOTLESS_CHANGED_AREA

/* changed area is box around each line */
static void
RootlessPolylines(DrawablePtr dst, GCPtr pGC,
                  int mode, int npt, DDXPointPtr pptInit)
{
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("poly lines start ");

    RootlessStartDrawing((WindowPtr) dst);
    pGC->ops->Polylines(dst, pGC, mode, npt, pptInit);

    if (npt > 0) {
        BoxRec box;
        int extra = pGC->lineWidth >> 1;

        box.x2 = box.x1 = pptInit->x;
        box.y2 = box.y1 = pptInit->y;

        if (npt > 1) {
            if (pGC->joinStyle == JoinMiter)
                extra = 6 * pGC->lineWidth;
            else if (pGC->capStyle == CapProjecting)
                extra = pGC->lineWidth;
        }

        if (mode == CoordModePrevious) {
            int x = box.x1;
            int y = box.y1;

            while (--npt) {
                pptInit++;
                x += pptInit->x;
                y += pptInit->y;
                if (box.x1 > x)
                    box.x1 = x;
                else if (box.x2 < x)
                    box.x2 = x;
                if (box.y1 > y)
                    box.y1 = y;
                else if (box.y2 < y)
                    box.y2 = y;
            }
        }
        else {
            while (--npt) {
                pptInit++;
                if (box.x1 > pptInit->x)
                    box.x1 = pptInit->x;
                else if (box.x2 < pptInit->x)
                    box.x2 = pptInit->x;
                if (box.y1 > pptInit->y)
                    box.y1 = pptInit->y;
                else if (box.y2 < pptInit->y)
                    box.y2 = pptInit->y;
            }
        }

        box.x2++;
        box.y2++;

        if (extra) {
            box.x1 -= extra;
            box.x2 += extra;
            box.y1 -= extra;
            box.y2 += extra;
        }

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("poly lines end\n");
}

/* changed area is box around each line segment */
static void
RootlessPolySegment(DrawablePtr dst, GCPtr pGC, int nseg, xSegment * pSeg)
{
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("poly segment start (win 0x%x)", dst);

    RootlessStartDrawing((WindowPtr) dst);
    pGC->ops->PolySegment(dst, pGC, nseg, pSeg);

    if (nseg > 0) {
        BoxRec box;
        int extra = pGC->lineWidth;

        if (pGC->capStyle != CapProjecting)
            extra >>= 1;

        if (pSeg->x2 > pSeg->x1) {
            box.x1 = pSeg->x1;
            box.x2 = pSeg->x2;
        }
        else {
            box.x2 = pSeg->x1;
            box.x1 = pSeg->x2;
        }

        if (pSeg->y2 > pSeg->y1) {
            box.y1 = pSeg->y1;
            box.y2 = pSeg->y2;
        }
        else {
            box.y2 = pSeg->y1;
            box.y1 = pSeg->y2;
        }

        while (--nseg) {
            pSeg++;
            if (pSeg->x2 > pSeg->x1) {
                if (pSeg->x1 < box.x1)
                    box.x1 = pSeg->x1;
                if (pSeg->x2 > box.x2)
                    box.x2 = pSeg->x2;
            }
            else {
                if (pSeg->x2 < box.x1)
                    box.x1 = pSeg->x2;
                if (pSeg->x1 > box.x2)
                    box.x2 = pSeg->x1;
            }
            if (pSeg->y2 > pSeg->y1) {
                if (pSeg->y1 < box.y1)
                    box.y1 = pSeg->y1;
                if (pSeg->y2 > box.y2)
                    box.y2 = pSeg->y2;
            }
            else {
                if (pSeg->y2 < box.y1)
                    box.y1 = pSeg->y2;
                if (pSeg->y1 > box.y2)
                    box.y2 = pSeg->y1;
            }
        }

        box.x2++;
        box.y2++;

        if (extra) {
            box.x1 -= extra;
            box.x2 += extra;
            box.y1 -= extra;
            box.y2 += extra;
        }

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("poly segment end\n");
}

/* changed area is box around each line (not entire rects) */
static void
RootlessPolyRectangle(DrawablePtr dst, GCPtr pGC,
                      int nRects, xRectangle *pRects)
{
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("poly rectangle start ");

    RootlessStartDrawing((WindowPtr) dst);
    pGC->ops->PolyRectangle(dst, pGC, nRects, pRects);

    if (nRects > 0) {
        BoxRec box;
        int offset1, offset2, offset3;

        offset2 = pGC->lineWidth;
        if (!offset2)
            offset2 = 1;
        offset1 = offset2 >> 1;
        offset3 = offset2 - offset1;

        while (nRects--) {
            box.x1 = pRects->x - offset1;
            box.y1 = pRects->y - offset1;
            box.x2 = box.x1 + pRects->width + offset2;
            box.y2 = box.y1 + offset2;
            TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
            if (BOX_NOT_EMPTY(box))
                RootlessDamageBox((WindowPtr) dst, &box);

            box.x1 = pRects->x - offset1;
            box.y1 = pRects->y + offset3;
            box.x2 = box.x1 + offset2;
            box.y2 = box.y1 + pRects->height - offset2;
            TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
            if (BOX_NOT_EMPTY(box))
                RootlessDamageBox((WindowPtr) dst, &box);

            box.x1 = pRects->x + pRects->width - offset1;
            box.y1 = pRects->y + offset3;
            box.x2 = box.x1 + offset2;
            box.y2 = box.y1 + pRects->height - offset2;
            TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
            if (BOX_NOT_EMPTY(box))
                RootlessDamageBox((WindowPtr) dst, &box);

            box.x1 = pRects->x - offset1;
            box.y1 = pRects->y + pRects->height - offset1;
            box.x2 = box.x1 + pRects->width + offset2;
            box.y2 = box.y1 + offset2;
            TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
            if (BOX_NOT_EMPTY(box))
                RootlessDamageBox((WindowPtr) dst, &box);

            pRects++;
        }
    }

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("poly rectangle end\n");
}

/* changed area is box around each arc (assumes all arcs are 360 degrees) */
static void
RootlessPolyArc(DrawablePtr dst, GCPtr pGC, int narcs, xArc * parcs)
{
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("poly arc start ");

    RootlessStartDrawing((WindowPtr) dst);
    pGC->ops->PolyArc(dst, pGC, narcs, parcs);

    if (narcs > 0) {
        int extra = pGC->lineWidth >> 1;
        BoxRec box;

        box.x1 = parcs->x;
        box.x2 = box.x1 + parcs->width;
        box.y1 = parcs->y;
        box.y2 = box.y1 + parcs->height;

        /* should I break these up instead ? */

        while (--narcs) {
            parcs++;
            if (box.x1 > parcs->x)
                box.x1 = parcs->x;
            if (box.x2 < (parcs->x + parcs->width))
                box.x2 = parcs->x + parcs->width;
            if (box.y1 > parcs->y)
                box.y1 = parcs->y;
            if (box.y2 < (parcs->y + parcs->height))
                box.y2 = parcs->y + parcs->height;
        }

        if (extra) {
            box.x1 -= extra;
            box.x2 += extra;
            box.y1 -= extra;
            box.y2 += extra;
        }

        box.x2++;
        box.y2++;

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("poly arc end\n");
}

/* changed area is box around each poly */
static void
RootlessFillPolygon(DrawablePtr dst, GCPtr pGC,
                    int shape, int mode, int count, DDXPointPtr pptInit)
{
    GC_SAVE(pGC);
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("fill poly start (win 0x%x, fillStyle 0x%x)", dst,
                 pGC->fillStyle);

    if (count <= 2) {
        pGC->ops->FillPolygon(dst, pGC, shape, mode, count, pptInit);
    }
    else {
        DDXPointPtr ppt = pptInit;
        int i = count;
        BoxRec box;

        box.x2 = box.x1 = ppt->x;
        box.y2 = box.y1 = ppt->y;

        if (mode != CoordModeOrigin) {
            int x = box.x1;
            int y = box.y1;

            while (--i) {
                ppt++;
                x += ppt->x;
                y += ppt->y;
                if (box.x1 > x)
                    box.x1 = x;
                else if (box.x2 < x)
                    box.x2 = x;
                if (box.y1 > y)
                    box.y1 = y;
                else if (box.y2 < y)
                    box.y2 = y;
            }
        }
        else {
            while (--i) {
                ppt++;
                if (box.x1 > ppt->x)
                    box.x1 = ppt->x;
                else if (box.x2 < ppt->x)
                    box.x2 = ppt->x;
                if (box.y1 > ppt->y)
                    box.y1 = ppt->y;
                else if (box.y2 < ppt->y)
                    box.y2 = ppt->y;
            }
        }

        box.x2++;
        box.y2++;

        RootlessStartDrawing((WindowPtr) dst);

        if (canAccelFill(dst, pGC)) {
            GC_UNSET_PM(pGC, dst);
        }

        pGC->ops->FillPolygon(dst, pGC, shape, mode, count, pptInit);

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GC_RESTORE(pGC, dst);
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("fill poly end\n");
}

/* changed area is the rects */
static void
RootlessPolyFillRect(DrawablePtr dst, GCPtr pGC,
                     int nRectsInit, xRectangle *pRectsInit)
{
    GC_SAVE(pGC);
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("fill rect start (win 0x%x, fillStyle 0x%x)", dst,
                 pGC->fillStyle);

    if (nRectsInit <= 0) {
        pGC->ops->PolyFillRect(dst, pGC, nRectsInit, pRectsInit);
    }
    else {
        BoxRec box;
        xRectangle *pRects = pRectsInit;
        int nRects = nRectsInit;

        box.x1 = pRects->x;
        box.x2 = box.x1 + pRects->width;
        box.y1 = pRects->y;
        box.y2 = box.y1 + pRects->height;

        while (--nRects) {
            pRects++;
            if (box.x1 > pRects->x)
                box.x1 = pRects->x;
            if (box.x2 < (pRects->x + pRects->width))
                box.x2 = pRects->x + pRects->width;
            if (box.y1 > pRects->y)
                box.y1 = pRects->y;
            if (box.y2 < (pRects->y + pRects->height))
                box.y2 = pRects->y + pRects->height;
        }

        RootlessStartDrawing((WindowPtr) dst);

        if (canAccelFill(dst, pGC)) {
            GC_UNSET_PM(pGC, dst);
        }

        pGC->ops->PolyFillRect(dst, pGC, nRectsInit, pRectsInit);

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GC_RESTORE(pGC, dst);
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("fill rect end\n");
}

/* changed area is box around each arc (assuming arcs are all 360 degrees) */
static void
RootlessPolyFillArc(DrawablePtr dst, GCPtr pGC, int narcsInit, xArc * parcsInit)
{
    GC_SAVE(pGC);
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("fill arc start ");

    if (narcsInit > 0) {
        BoxRec box;
        int narcs = narcsInit;
        xArc *parcs = parcsInit;

        box.x1 = parcs->x;
        box.x2 = box.x1 + parcs->width;
        box.y1 = parcs->y;
        box.y2 = box.y1 + parcs->height;

        /* should I break these up instead ? */

        while (--narcs) {
            parcs++;
            if (box.x1 > parcs->x)
                box.x1 = parcs->x;
            if (box.x2 < (parcs->x + parcs->width))
                box.x2 = parcs->x + parcs->width;
            if (box.y1 > parcs->y)
                box.y1 = parcs->y;
            if (box.y2 < (parcs->y + parcs->height))
                box.y2 = parcs->y + parcs->height;
        }

        RootlessStartDrawing((WindowPtr) dst);

        if (canAccelFill(dst, pGC)) {
            GC_UNSET_PM(pGC, dst);
        }

        pGC->ops->PolyFillArc(dst, pGC, narcsInit, parcsInit);

        TRIM_AND_TRANSLATE_BOX(box, dst, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }
    else {
        pGC->ops->PolyFillArc(dst, pGC, narcsInit, parcsInit);
    }

    GC_RESTORE(pGC, dst);
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("fill arc end\n");
}

static void
RootlessImageText8(DrawablePtr dst, GCPtr pGC,
                   int x, int y, int count, char *chars)
{
    GC_SAVE(pGC);
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("imagetext8 start ");

    if (count > 0) {
        int top, bot, Min, Max;
        BoxRec box;

        top = max(FONTMAXBOUNDS(pGC->font, ascent), FONTASCENT(pGC->font));
        bot = max(FONTMAXBOUNDS(pGC->font, descent), FONTDESCENT(pGC->font));

        Min = count * FONTMINBOUNDS(pGC->font, characterWidth);
        if (Min > 0)
            Min = 0;
        Max = count * FONTMAXBOUNDS(pGC->font, characterWidth);
        if (Max < 0)
            Max = 0;

        /* ugh */
        box.x1 = dst->x + x + Min + FONTMINBOUNDS(pGC->font, leftSideBearing);
        box.x2 = dst->x + x + Max + FONTMAXBOUNDS(pGC->font, rightSideBearing);

        box.y1 = dst->y + y - top;
        box.y2 = dst->y + y + bot;

        RootlessStartDrawing((WindowPtr) dst);

        if (canAccelFill(dst, pGC)) {
            GC_UNSET_PM(pGC, dst);
        }

        pGC->ops->ImageText8(dst, pGC, x, y, count, chars);

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }
    else {
        pGC->ops->ImageText8(dst, pGC, x, y, count, chars);
    }

    GC_RESTORE(pGC, dst);
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("imagetext8 end\n");
}

static int
RootlessPolyText8(DrawablePtr dst, GCPtr pGC,
                  int x, int y, int count, char *chars)
{
    int width;                  // the result, sorta

    GCOP_UNWRAP(pGC);

    RL_DEBUG_MSG("polytext8 start ");

    RootlessStartDrawing((WindowPtr) dst);
    width = pGC->ops->PolyText8(dst, pGC, x, y, count, chars);
    width -= x;

    if (width > 0) {
        BoxRec box;

        /* ugh */
        box.x1 = dst->x + x + FONTMINBOUNDS(pGC->font, leftSideBearing);
        box.x2 = dst->x + x + FONTMAXBOUNDS(pGC->font, rightSideBearing);

        if (count > 1) {
            box.x2 += width;
        }

        box.y1 = dst->y + y - FONTMAXBOUNDS(pGC->font, ascent);
        box.y2 = dst->y + y + FONTMAXBOUNDS(pGC->font, descent);

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("polytext8 end\n");
    return width + x;
}

static void
RootlessImageText16(DrawablePtr dst, GCPtr pGC,
                    int x, int y, int count, unsigned short *chars)
{
    GC_SAVE(pGC);
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("imagetext16 start ");

    if (count > 0) {
        int top, bot, Min, Max;
        BoxRec box;

        top = max(FONTMAXBOUNDS(pGC->font, ascent), FONTASCENT(pGC->font));
        bot = max(FONTMAXBOUNDS(pGC->font, descent), FONTDESCENT(pGC->font));

        Min = count * FONTMINBOUNDS(pGC->font, characterWidth);
        if (Min > 0)
            Min = 0;
        Max = count * FONTMAXBOUNDS(pGC->font, characterWidth);
        if (Max < 0)
            Max = 0;

        /* ugh */
        box.x1 = dst->x + x + Min + FONTMINBOUNDS(pGC->font, leftSideBearing);
        box.x2 = dst->x + x + Max + FONTMAXBOUNDS(pGC->font, rightSideBearing);

        box.y1 = dst->y + y - top;
        box.y2 = dst->y + y + bot;

        RootlessStartDrawing((WindowPtr) dst);

        if (canAccelFill(dst, pGC)) {
            GC_UNSET_PM(pGC, dst);
        }

        pGC->ops->ImageText16(dst, pGC, x, y, count, chars);

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }
    else {
        pGC->ops->ImageText16(dst, pGC, x, y, count, chars);
    }

    GC_RESTORE(pGC, dst);
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("imagetext16 end\n");
}

static int
RootlessPolyText16(DrawablePtr dst, GCPtr pGC,
                   int x, int y, int count, unsigned short *chars)
{
    int width;                  // the result, sorta

    GCOP_UNWRAP(pGC);

    RL_DEBUG_MSG("polytext16 start ");

    RootlessStartDrawing((WindowPtr) dst);
    width = pGC->ops->PolyText16(dst, pGC, x, y, count, chars);
    width -= x;

    if (width > 0) {
        BoxRec box;

        /* ugh */
        box.x1 = dst->x + x + FONTMINBOUNDS(pGC->font, leftSideBearing);
        box.x2 = dst->x + x + FONTMAXBOUNDS(pGC->font, rightSideBearing);

        if (count > 1) {
            box.x2 += width;
        }

        box.y1 = dst->y + y - FONTMAXBOUNDS(pGC->font, ascent);
        box.y2 = dst->y + y + FONTMAXBOUNDS(pGC->font, descent);

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("polytext16 end\n");
    return width + x;
}

static void
RootlessImageGlyphBlt(DrawablePtr dst, GCPtr pGC,
                      int x, int y, unsigned int nglyphInit,
                      CharInfoPtr * ppciInit, void *unused)
{
    GC_SAVE(pGC);
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("imageglyph start ");

    if (nglyphInit > 0) {
        int top, bot, width = 0;
        BoxRec box;
        unsigned int nglyph = nglyphInit;
        CharInfoPtr *ppci = ppciInit;

        top = max(FONTMAXBOUNDS(pGC->font, ascent), FONTASCENT(pGC->font));
        bot = max(FONTMAXBOUNDS(pGC->font, descent), FONTDESCENT(pGC->font));

        box.x1 = ppci[0]->metrics.leftSideBearing;
        if (box.x1 > 0)
            box.x1 = 0;
        box.x2 = ppci[nglyph - 1]->metrics.rightSideBearing -
            ppci[nglyph - 1]->metrics.characterWidth;
        if (box.x2 < 0)
            box.x2 = 0;

        box.x2 += dst->x + x;
        box.x1 += dst->x + x;

        while (nglyph--) {
            width += (*ppci)->metrics.characterWidth;
            ppci++;
        }

        if (width > 0)
            box.x2 += width;
        else
            box.x1 += width;

        box.y1 = dst->y + y - top;
        box.y2 = dst->y + y + bot;

        RootlessStartDrawing((WindowPtr) dst);

        if (canAccelFill(dst, pGC)) {
            GC_UNSET_PM(pGC, dst);
        }

        pGC->ops->ImageGlyphBlt(dst, pGC, x, y, nglyphInit, ppciInit, unused);

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }
    else {
        pGC->ops->ImageGlyphBlt(dst, pGC, x, y, nglyphInit, ppciInit, unused);
    }

    GC_RESTORE(pGC, dst);
    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("imageglyph end\n");
}

static void
RootlessPolyGlyphBlt(DrawablePtr dst, GCPtr pGC,
                     int x, int y, unsigned int nglyph,
                     CharInfoPtr * ppci, void *pglyphBase)
{
    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("polyglyph start ");

    RootlessStartDrawing((WindowPtr) dst);
    pGC->ops->PolyGlyphBlt(dst, pGC, x, y, nglyph, ppci, pglyphBase);

    if (nglyph > 0) {
        BoxRec box;

        /* ugh */
        box.x1 = dst->x + x + ppci[0]->metrics.leftSideBearing;
        box.x2 = dst->x + x + ppci[nglyph - 1]->metrics.rightSideBearing;

        if (nglyph > 1) {
            int width = 0;

            while (--nglyph) {
                width += (*ppci)->metrics.characterWidth;
                ppci++;
            }

            if (width > 0)
                box.x2 += width;
            else
                box.x1 += width;
        }

        box.y1 = dst->y + y - FONTMAXBOUNDS(pGC->font, ascent);
        box.y2 = dst->y + y + FONTMAXBOUNDS(pGC->font, descent);

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            RootlessDamageBox((WindowPtr) dst, &box);
    }

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("polyglyph end\n");
}

/* changed area is in dest */
static void
RootlessPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr dst,
                   int dx, int dy, int xOrg, int yOrg)
{
    BoxRec box;

    GCOP_UNWRAP(pGC);
    RL_DEBUG_MSG("push pixels start ");

    RootlessStartDrawing((WindowPtr) dst);
    pGC->ops->PushPixels(pGC, pBitMap, dst, dx, dy, xOrg, yOrg);

    box.x1 = xOrg + dst->x;
    box.x2 = box.x1 + dx;
    box.y1 = yOrg + dst->y;
    box.y2 = box.y1 + dy;

    TRIM_BOX(box, pGC);
    if (BOX_NOT_EMPTY(box))
        RootlessDamageBox((WindowPtr) dst, &box);

    GCOP_WRAP(pGC);
    RL_DEBUG_MSG("push pixels end\n");
}
