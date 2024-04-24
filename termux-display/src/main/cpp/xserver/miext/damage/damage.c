/*
 * Copyright © 2003 Keith Packard
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

#include <stdlib.h>

#include    <X11/X.h>
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    <X11/fonts/font.h>
#include    "dixfontstr.h"
#include    <X11/fonts/fontstruct.h>
#include    <X11/fonts/libxfont2.h>
#include    "mi.h"
#include    "mipict.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "damage.h"
#include    "damagestr.h"

#define wrap(priv, real, mem, func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv, real, mem) {\
    real->mem = priv->mem; \
}

#define BOX_SAME(a,b) \
    ((a)->x1 == (b)->x1 && \
     (a)->y1 == (b)->y1 && \
     (a)->x2 == (b)->x2 && \
     (a)->y2 == (b)->y2)

#define DAMAGE_VALIDATE_ENABLE 0
#define DAMAGE_DEBUG_ENABLE 0
#if DAMAGE_DEBUG_ENABLE
#define DAMAGE_DEBUG(x)	ErrorF x
#else
#define DAMAGE_DEBUG(x)
#endif

#define getPixmapDamageRef(pPixmap) ((DamagePtr *) \
    dixLookupPrivateAddr(&(pPixmap)->devPrivates, damagePixPrivateKey))

#define pixmapDamage(pPixmap)		damagePixPriv(pPixmap)

static DevPrivateKeyRec damageScrPrivateKeyRec;

#define damageScrPrivateKey (&damageScrPrivateKeyRec)
static DevPrivateKeyRec damagePixPrivateKeyRec;

#define damagePixPrivateKey (&damagePixPrivateKeyRec)
static DevPrivateKeyRec damageGCPrivateKeyRec;

#define damageGCPrivateKey (&damageGCPrivateKeyRec)
static DevPrivateKeyRec damageWinPrivateKeyRec;

#define damageWinPrivateKey (&damageWinPrivateKeyRec)

static DamagePtr *
getDrawableDamageRef(DrawablePtr pDrawable)
{
    PixmapPtr pPixmap;

    if (WindowDrawable(pDrawable->type)) {
        ScreenPtr pScreen = pDrawable->pScreen;

        pPixmap = 0;
        if (pScreen->GetWindowPixmap
#ifdef ROOTLESS_WORKAROUND
            && ((WindowPtr) pDrawable)->viewable
#endif
            )
            pPixmap = (*pScreen->GetWindowPixmap) ((WindowPtr) pDrawable);

        if (!pPixmap) {
            damageScrPriv(pScreen);

            return &pScrPriv->pScreenDamage;
        }
    }
    else
        pPixmap = (PixmapPtr) pDrawable;
    return getPixmapDamageRef(pPixmap);
}

#define getDrawableDamage(pDrawable)	(*getDrawableDamageRef (pDrawable))
#define getWindowDamage(pWin)		getDrawableDamage(&(pWin)->drawable)

#define drawableDamage(pDrawable)	\
    DamagePtr	pDamage = getDrawableDamage(pDrawable)

#define windowDamage(pWin)		drawableDamage(&(pWin)->drawable)

#define winDamageRef(pWindow) \
    DamagePtr	*pPrev = (DamagePtr *) \
	dixLookupPrivateAddr(&(pWindow)->devPrivates, damageWinPrivateKey)

#if DAMAGE_DEBUG_ENABLE
static void
_damageRegionAppend(DrawablePtr pDrawable, RegionPtr pRegion, Bool clip,
                    int subWindowMode, const char *where)
#define damageRegionAppend(d,r,c,m) _damageRegionAppend(d,r,c,m,__FUNCTION__)
#else
static void
damageRegionAppend(DrawablePtr pDrawable, RegionPtr pRegion, Bool clip,
                   int subWindowMode)
#endif
{
    ScreenPtr pScreen = pDrawable->pScreen;

    damageScrPriv(pScreen);
    drawableDamage(pDrawable);
    DamagePtr pNext;
    RegionRec clippedRec;
    RegionPtr pDamageRegion;
    RegionRec pixClip;
    int draw_x, draw_y;

#ifdef COMPOSITE
    int screen_x = 0, screen_y = 0;
#endif

    /* short circuit for empty regions */
    if (!RegionNotEmpty(pRegion))
        return;

#ifdef COMPOSITE
    /*
     * When drawing to a pixmap which is storing window contents,
     * the region presented is in pixmap relative coordinates which
     * need to be converted to screen relative coordinates
     */
    if (pDrawable->type != DRAWABLE_WINDOW) {
        screen_x = ((PixmapPtr) pDrawable)->screen_x - pDrawable->x;
        screen_y = ((PixmapPtr) pDrawable)->screen_y - pDrawable->y;
    }
    if (screen_x || screen_y)
        RegionTranslate(pRegion, screen_x, screen_y);
#endif

    if (pDrawable->type == DRAWABLE_WINDOW &&
        ((WindowPtr) (pDrawable))->backingStore == NotUseful) {
        if (subWindowMode == ClipByChildren) {
            RegionIntersect(pRegion, pRegion,
                            &((WindowPtr) (pDrawable))->clipList);
        }
        else if (subWindowMode == IncludeInferiors) {
            RegionPtr pTempRegion =
                NotClippedByChildren((WindowPtr) (pDrawable));
            RegionIntersect(pRegion, pRegion, pTempRegion);
            RegionDestroy(pTempRegion);
        }
        /* If subWindowMode is set to an invalid value, don't perform
         * any drawable-based clipping. */
    }

    RegionNull(&clippedRec);
    for (; pDamage; pDamage = pNext) {
        pNext = pDamage->pNext;
        /*
         * Check for internal damage and don't send events
         */
        if (pScrPriv->internalLevel > 0 && !pDamage->isInternal) {
            DAMAGE_DEBUG(("non internal damage, skipping at %d\n",
                          pScrPriv->internalLevel));
            continue;
        }
        /*
         * Check for unrealized windows
         */
        if (pDamage->pDrawable->type == DRAWABLE_WINDOW &&
            !((WindowPtr) (pDamage->pDrawable))->realized) {
            continue;
        }

        draw_x = pDamage->pDrawable->x;
        draw_y = pDamage->pDrawable->y;
#ifdef COMPOSITE
        /*
         * Need to move everyone to screen coordinates
         * XXX what about off-screen pixmaps with non-zero x/y?
         */
        if (!WindowDrawable(pDamage->pDrawable->type)) {
            draw_x += ((PixmapPtr) pDamage->pDrawable)->screen_x;
            draw_y += ((PixmapPtr) pDamage->pDrawable)->screen_y;
        }
#endif

        /*
         * Clip against border or pixmap bounds
         */

        pDamageRegion = pRegion;
        if (clip || pDamage->pDrawable != pDrawable) {
            pDamageRegion = &clippedRec;
            if (pDamage->pDrawable->type == DRAWABLE_WINDOW) {
                RegionIntersect(pDamageRegion, pRegion,
                                &((WindowPtr) (pDamage->pDrawable))->
                                borderClip);
            }
            else {
                BoxRec box;

                box.x1 = draw_x;
                box.y1 = draw_y;
                box.x2 = draw_x + pDamage->pDrawable->width;
                box.y2 = draw_y + pDamage->pDrawable->height;
                RegionInit(&pixClip, &box, 1);
                RegionIntersect(pDamageRegion, pRegion, &pixClip);
                RegionUninit(&pixClip);
            }
            /*
             * Short circuit empty results
             */
            if (!RegionNotEmpty(pDamageRegion))
                continue;
        }

        DAMAGE_DEBUG(("%s %d x %d +%d +%d (target 0x%lx monitor 0x%lx)\n",
                      where,
                      pDamageRegion->extents.x2 - pDamageRegion->extents.x1,
                      pDamageRegion->extents.y2 - pDamageRegion->extents.y1,
                      pDamageRegion->extents.x1, pDamageRegion->extents.y1,
                      pDrawable->id, pDamage->pDrawable->id));

        /*
         * Move region to target coordinate space
         */
        if (draw_x || draw_y)
            RegionTranslate(pDamageRegion, -draw_x, -draw_y);

        /* Store damage region if needed after submission. */
        if (pDamage->reportAfter)
            RegionUnion(&pDamage->pendingDamage,
                        &pDamage->pendingDamage, pDamageRegion);

        /* Report damage now, if desired. */
        if (!pDamage->reportAfter) {
            if (pDamage->damageReport)
                DamageReportDamage(pDamage, pDamageRegion);
            else
                RegionUnion(&pDamage->damage, &pDamage->damage, pDamageRegion);
        }

        /*
         * translate original region back
         */
        if (pDamageRegion == pRegion && (draw_x || draw_y))
            RegionTranslate(pDamageRegion, draw_x, draw_y);
    }
#ifdef COMPOSITE
    if (screen_x || screen_y)
        RegionTranslate(pRegion, -screen_x, -screen_y);
#endif

    RegionUninit(&clippedRec);
}

static void
damageRegionProcessPending(DrawablePtr pDrawable)
{
    drawableDamage(pDrawable);

    for (; pDamage != NULL; pDamage = pDamage->pNext) {
        if (pDamage->reportAfter) {
            /* It's possible that there is only interest in postRendering reporting. */
            if (pDamage->damageReport)
                DamageReportDamage(pDamage, &pDamage->pendingDamage);
            else
                RegionUnion(&pDamage->damage, &pDamage->damage,
                            &pDamage->pendingDamage);
        }

        if (pDamage->reportAfter)
            RegionEmpty(&pDamage->pendingDamage);
    }

}

#if DAMAGE_DEBUG_ENABLE
#define damageDamageBox(d,b,m) _damageDamageBox(d,b,m,__FUNCTION__)
static void
_damageDamageBox(DrawablePtr pDrawable, BoxPtr pBox, int subWindowMode,
                 const char *where)
#else
static void
damageDamageBox(DrawablePtr pDrawable, BoxPtr pBox, int subWindowMode)
#endif
{
    RegionRec region;

    RegionInit(&region, pBox, 1);
#if DAMAGE_DEBUG_ENABLE
    _damageRegionAppend(pDrawable, &region, TRUE, subWindowMode, where);
#else
    damageRegionAppend(pDrawable, &region, TRUE, subWindowMode);
#endif
    RegionUninit(&region);
}

static void damageValidateGC(GCPtr, unsigned long, DrawablePtr);
static void damageChangeGC(GCPtr, unsigned long);
static void damageCopyGC(GCPtr, unsigned long, GCPtr);
static void damageDestroyGC(GCPtr);
static void damageChangeClip(GCPtr, int, void *, int);
static void damageDestroyClip(GCPtr);
static void damageCopyClip(GCPtr, GCPtr);

static GCFuncs damageGCFuncs = {
    damageValidateGC, damageChangeGC, damageCopyGC, damageDestroyGC,
    damageChangeClip, damageDestroyClip, damageCopyClip
};

static GCOps damageGCOps;

static Bool
damageCreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;

    damageScrPriv(pScreen);
    damageGCPriv(pGC);
    Bool ret;

    unwrap(pScrPriv, pScreen, CreateGC);
    if ((ret = (*pScreen->CreateGC) (pGC))) {
        pGCPriv->ops = NULL;
        pGCPriv->funcs = pGC->funcs;
        pGC->funcs = &damageGCFuncs;
    }
    wrap(pScrPriv, pScreen, CreateGC, damageCreateGC);

    return ret;
}

#define DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable) \
    damageGCPriv(pGC);  \
    const GCFuncs *oldFuncs = pGC->funcs; \
    unwrap(pGCPriv, pGC, funcs);  \
    unwrap(pGCPriv, pGC, ops); \

#define DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable) \
    wrap(pGCPriv, pGC, funcs, oldFuncs); \
    wrap(pGCPriv, pGC, ops, &damageGCOps)

#define DAMAGE_GC_FUNC_PROLOGUE(pGC) \
    damageGCPriv(pGC); \
    unwrap(pGCPriv, pGC, funcs); \
    if (pGCPriv->ops) unwrap(pGCPriv, pGC, ops)

#define DAMAGE_GC_FUNC_EPILOGUE(pGC) \
    wrap(pGCPriv, pGC, funcs, &damageGCFuncs);  \
    if (pGCPriv->ops) wrap(pGCPriv, pGC, ops, &damageGCOps)

static void
damageValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
    DAMAGE_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->ValidateGC) (pGC, changes, pDrawable);
    pGCPriv->ops = pGC->ops; /* just so it's not NULL */
    DAMAGE_GC_FUNC_EPILOGUE(pGC);
}

static void
damageDestroyGC(GCPtr pGC)
{
    DAMAGE_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->DestroyGC) (pGC);
    DAMAGE_GC_FUNC_EPILOGUE(pGC);
}

static void
damageChangeGC(GCPtr pGC, unsigned long mask)
{
    DAMAGE_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    DAMAGE_GC_FUNC_EPILOGUE(pGC);
}

static void
damageCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    DAMAGE_GC_FUNC_PROLOGUE(pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    DAMAGE_GC_FUNC_EPILOGUE(pGCDst);
}

static void
damageChangeClip(GCPtr pGC, int type, void *pvalue, int nrects)
{
    DAMAGE_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    DAMAGE_GC_FUNC_EPILOGUE(pGC);
}

static void
damageCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    DAMAGE_GC_FUNC_PROLOGUE(pgcDst);
    (*pgcDst->funcs->CopyClip) (pgcDst, pgcSrc);
    DAMAGE_GC_FUNC_EPILOGUE(pgcDst);
}

static void
damageDestroyClip(GCPtr pGC)
{
    DAMAGE_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->DestroyClip) (pGC);
    DAMAGE_GC_FUNC_EPILOGUE(pGC);
}

#define TRIM_BOX(box, pGC) if (pGC->pCompositeClip) { \
    BoxPtr extents = &pGC->pCompositeClip->extents;\
    if(box.x1 < extents->x1) box.x1 = extents->x1; \
    if(box.x2 > extents->x2) box.x2 = extents->x2; \
    if(box.y1 < extents->y1) box.y1 = extents->y1; \
    if(box.y2 > extents->y2) box.y2 = extents->y2; \
    }

#define TRANSLATE_BOX(box, pDrawable) { \
    box.x1 += pDrawable->x; \
    box.x2 += pDrawable->x; \
    box.y1 += pDrawable->y; \
    box.y2 += pDrawable->y; \
    }

#define TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC) { \
    TRANSLATE_BOX(box, pDrawable); \
    TRIM_BOX(box, pGC); \
    }

#define BOX_NOT_EMPTY(box) \
    (((box.x2 - box.x1) > 0) && ((box.y2 - box.y1) > 0))

#define checkGCDamage(d,g)	(getDrawableDamage(d) && \
				 (!g->pCompositeClip ||\
				  RegionNotEmpty(g->pCompositeClip)))

#define TRIM_PICTURE_BOX(box, pDst) { \
    BoxPtr extents = &pDst->pCompositeClip->extents;\
    if(box.x1 < extents->x1) box.x1 = extents->x1; \
    if(box.x2 > extents->x2) box.x2 = extents->x2; \
    if(box.y1 < extents->y1) box.y1 = extents->y1; \
    if(box.y2 > extents->y2) box.y2 = extents->y2; \
    }

#define checkPictureDamage(p)	(getDrawableDamage(p->pDrawable) && \
				 RegionNotEmpty(p->pCompositeClip))

static void
damageComposite(CARD8 op,
                PicturePtr pSrc,
                PicturePtr pMask,
                PicturePtr pDst,
                INT16 xSrc,
                INT16 ySrc,
                INT16 xMask,
                INT16 yMask,
                INT16 xDst, INT16 yDst, CARD16 width, CARD16 height)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);

    damageScrPriv(pScreen);

    if (checkPictureDamage(pDst)) {
        BoxRec box;

        box.x1 = xDst + pDst->pDrawable->x;
        box.y1 = yDst + pDst->pDrawable->y;
        box.x2 = box.x1 + width;
        box.y2 = box.y1 + height;
        TRIM_PICTURE_BOX(box, pDst);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDst->pDrawable, &box, pDst->subWindowMode);
    }
    /*
     * Validating a source picture bound to a window may trigger other
     * composite operations. Do it before unwrapping to make sure damage
     * is reported correctly.
     */
    if (pSrc->pDrawable && WindowDrawable(pSrc->pDrawable->type))
        miCompositeSourceValidate(pSrc);
    if (pMask && pMask->pDrawable && WindowDrawable(pMask->pDrawable->type))
        miCompositeSourceValidate(pMask);
    unwrap(pScrPriv, ps, Composite);
    (*ps->Composite) (op,
                      pSrc,
                      pMask,
                      pDst,
                      xSrc, ySrc, xMask, yMask, xDst, yDst, width, height);
    damageRegionProcessPending(pDst->pDrawable);
    wrap(pScrPriv, ps, Composite, damageComposite);
}

static void
damageGlyphs(CARD8 op,
             PicturePtr pSrc,
             PicturePtr pDst,
             PictFormatPtr maskFormat,
             INT16 xSrc,
             INT16 ySrc, int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);

    damageScrPriv(pScreen);

    if (checkPictureDamage(pDst)) {
        int nlistTmp = nlist;
        GlyphListPtr listTmp = list;
        GlyphPtr *glyphsTmp = glyphs;
        int x, y;
        int n;
        GlyphPtr glyph;
        BoxRec box;
        int x1, y1, x2, y2;

        box.x1 = 32767;
        box.y1 = 32767;
        box.x2 = -32767;
        box.y2 = -32767;
        x = pDst->pDrawable->x;
        y = pDst->pDrawable->y;
        while (nlistTmp--) {
            x += listTmp->xOff;
            y += listTmp->yOff;
            n = listTmp->len;
            while (n--) {
                glyph = *glyphsTmp++;
                x1 = x - glyph->info.x;
                y1 = y - glyph->info.y;
                x2 = x1 + glyph->info.width;
                y2 = y1 + glyph->info.height;
                if (x1 < box.x1)
                    box.x1 = x1;
                if (y1 < box.y1)
                    box.y1 = y1;
                if (x2 > box.x2)
                    box.x2 = x2;
                if (y2 > box.y2)
                    box.y2 = y2;
                x += glyph->info.xOff;
                y += glyph->info.yOff;
            }
            listTmp++;
        }
        TRIM_PICTURE_BOX(box, pDst);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDst->pDrawable, &box, pDst->subWindowMode);
    }
    unwrap(pScrPriv, ps, Glyphs);
    (*ps->Glyphs) (op, pSrc, pDst, maskFormat, xSrc, ySrc, nlist, list, glyphs);
    damageRegionProcessPending(pDst->pDrawable);
    wrap(pScrPriv, ps, Glyphs, damageGlyphs);
}

static void
damageAddTraps(PicturePtr pPicture,
               INT16 x_off, INT16 y_off, int ntrap, xTrap * traps)
{
    ScreenPtr pScreen = pPicture->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);

    damageScrPriv(pScreen);

    if (checkPictureDamage(pPicture)) {
        BoxRec box;
        int i;
        int x, y;
        xTrap *t = traps;

        box.x1 = 32767;
        box.y1 = 32767;
        box.x2 = -32767;
        box.y2 = -32767;
        x = pPicture->pDrawable->x + x_off;
        y = pPicture->pDrawable->y + y_off;
        for (i = 0; i < ntrap; i++) {
            pixman_fixed_t l = min(t->top.l, t->bot.l);
            pixman_fixed_t r = max(t->top.r, t->bot.r);
            int x1 = x + pixman_fixed_to_int(l);
            int x2 = x + pixman_fixed_to_int(pixman_fixed_ceil(r));
            int y1 = y + pixman_fixed_to_int(t->top.y);
            int y2 = y + pixman_fixed_to_int(pixman_fixed_ceil(t->bot.y));

            if (x1 < box.x1)
                box.x1 = x1;
            if (x2 > box.x2)
                box.x2 = x2;
            if (y1 < box.y1)
                box.y1 = y1;
            if (y2 > box.y2)
                box.y2 = y2;
        }
        TRIM_PICTURE_BOX(box, pPicture);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pPicture->pDrawable, &box, pPicture->subWindowMode);
    }
    unwrap(pScrPriv, ps, AddTraps);
    (*ps->AddTraps) (pPicture, x_off, y_off, ntrap, traps);
    damageRegionProcessPending(pPicture->pDrawable);
    wrap(pScrPriv, ps, AddTraps, damageAddTraps);
}

/**********************************************************/

static void
damageFillSpans(DrawablePtr pDrawable,
                GC * pGC, int npt, DDXPointPtr ppt, int *pwidth, int fSorted)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (npt && checkGCDamage(pDrawable, pGC)) {
        int nptTmp = npt;
        DDXPointPtr pptTmp = ppt;
        int *pwidthTmp = pwidth;
        BoxRec box;

        box.x1 = pptTmp->x;
        box.x2 = box.x1 + *pwidthTmp;
        box.y2 = box.y1 = pptTmp->y;

        while (--nptTmp) {
            pptTmp++;
            pwidthTmp++;
            if (box.x1 > pptTmp->x)
                box.x1 = pptTmp->x;
            if (box.x2 < (pptTmp->x + *pwidthTmp))
                box.x2 = pptTmp->x + *pwidthTmp;
            if (box.y1 > pptTmp->y)
                box.y1 = pptTmp->y;
            else if (box.y2 < pptTmp->y)
                box.y2 = pptTmp->y;
        }

        box.y2++;

        if (!pGC->miTranslate) {
            TRANSLATE_BOX(box, pDrawable);
        }
        TRIM_BOX(box, pGC);

        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }

    (*pGC->ops->FillSpans) (pDrawable, pGC, npt, ppt, pwidth, fSorted);

    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damageSetSpans(DrawablePtr pDrawable,
               GCPtr pGC,
               char *pcharsrc,
               DDXPointPtr ppt, int *pwidth, int npt, int fSorted)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (npt && checkGCDamage(pDrawable, pGC)) {
        DDXPointPtr pptTmp = ppt;
        int *pwidthTmp = pwidth;
        int nptTmp = npt;
        BoxRec box;

        box.x1 = pptTmp->x;
        box.x2 = box.x1 + *pwidthTmp;
        box.y2 = box.y1 = pptTmp->y;

        while (--nptTmp) {
            pptTmp++;
            pwidthTmp++;
            if (box.x1 > pptTmp->x)
                box.x1 = pptTmp->x;
            if (box.x2 < (pptTmp->x + *pwidthTmp))
                box.x2 = pptTmp->x + *pwidthTmp;
            if (box.y1 > pptTmp->y)
                box.y1 = pptTmp->y;
            else if (box.y2 < pptTmp->y)
                box.y2 = pptTmp->y;
        }

        box.y2++;

        if (!pGC->miTranslate) {
            TRANSLATE_BOX(box, pDrawable);
        }
        TRIM_BOX(box, pGC);

        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->SetSpans) (pDrawable, pGC, pcharsrc, ppt, pwidth, npt, fSorted);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePutImage(DrawablePtr pDrawable,
               GCPtr pGC,
               int depth,
               int x,
               int y, int w, int h, int leftPad, int format, char *pImage)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    if (checkGCDamage(pDrawable, pGC)) {
        BoxRec box;

        box.x1 = x + pDrawable->x;
        box.x2 = box.x1 + w;
        box.y1 = y + pDrawable->y;
        box.y2 = box.y1 + h;

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->PutImage) (pDrawable, pGC, depth, x, y, w, h,
                           leftPad, format, pImage);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static RegionPtr
damageCopyArea(DrawablePtr pSrc,
               DrawablePtr pDst,
               GC * pGC,
               int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    RegionPtr ret;

    DAMAGE_GC_OP_PROLOGUE(pGC, pDst);

    if (checkGCDamage(pDst, pGC)) {
        BoxRec box;

        box.x1 = dstx + pDst->x;
        box.x2 = box.x1 + width;
        box.y1 = dsty + pDst->y;
        box.y2 = box.y1 + height;

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDst, &box, pGC->subWindowMode);
    }

    ret = (*pGC->ops->CopyArea) (pSrc, pDst,
                                 pGC, srcx, srcy, width, height, dstx, dsty);
    damageRegionProcessPending(pDst);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDst);
    return ret;
}

static RegionPtr
damageCopyPlane(DrawablePtr pSrc,
                DrawablePtr pDst,
                GCPtr pGC,
                int srcx,
                int srcy,
                int width,
                int height, int dstx, int dsty, unsigned long bitPlane)
{
    RegionPtr ret;

    DAMAGE_GC_OP_PROLOGUE(pGC, pDst);

    if (checkGCDamage(pDst, pGC)) {
        BoxRec box;

        box.x1 = dstx + pDst->x;
        box.x2 = box.x1 + width;
        box.y1 = dsty + pDst->y;
        box.y2 = box.y1 + height;

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDst, &box, pGC->subWindowMode);
    }

    ret = (*pGC->ops->CopyPlane) (pSrc, pDst,
                                  pGC, srcx, srcy, width, height, dstx, dsty,
                                  bitPlane);
    damageRegionProcessPending(pDst);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDst);
    return ret;
}

static void
damagePolyPoint(DrawablePtr pDrawable,
                GCPtr pGC, int mode, int npt, xPoint * ppt)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (npt && checkGCDamage(pDrawable, pGC)) {
        BoxRec box;
        int nptTmp = npt;
        xPoint *pptTmp = ppt;

        box.x2 = box.x1 = pptTmp->x;
        box.y2 = box.y1 = pptTmp->y;

        /* this could be slow if the points were spread out */

        if (mode == CoordModePrevious) {
            int x = box.x1;
            int y = box.y1;

            while (--nptTmp) {
                pptTmp++;
                x += pptTmp->x;
                y += pptTmp->y;
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
            while (--nptTmp) {
                pptTmp++;
                if (box.x1 > pptTmp->x)
                    box.x1 = pptTmp->x;
                else if (box.x2 < pptTmp->x)
                    box.x2 = pptTmp->x;
                if (box.y1 > pptTmp->y)
                    box.y1 = pptTmp->y;
                else if (box.y2 < pptTmp->y)
                    box.y2 = pptTmp->y;
            }
        }

        box.x2++;
        box.y2++;

        TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->PolyPoint) (pDrawable, pGC, mode, npt, ppt);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePolylines(DrawablePtr pDrawable,
                GCPtr pGC, int mode, int npt, DDXPointPtr ppt)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (npt && checkGCDamage(pDrawable, pGC)) {
        int nptTmp = npt;
        DDXPointPtr pptTmp = ppt;
        BoxRec box;
        int extra = pGC->lineWidth >> 1;

        box.x2 = box.x1 = pptTmp->x;
        box.y2 = box.y1 = pptTmp->y;

        if (nptTmp > 1) {
            if (pGC->joinStyle == JoinMiter)
                extra = 6 * pGC->lineWidth;
            else if (pGC->capStyle == CapProjecting)
                extra = pGC->lineWidth;
        }

        if (mode == CoordModePrevious) {
            int x = box.x1;
            int y = box.y1;

            while (--nptTmp) {
                pptTmp++;
                x += pptTmp->x;
                y += pptTmp->y;
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
            while (--nptTmp) {
                pptTmp++;
                if (box.x1 > pptTmp->x)
                    box.x1 = pptTmp->x;
                else if (box.x2 < pptTmp->x)
                    box.x2 = pptTmp->x;
                if (box.y1 > pptTmp->y)
                    box.y1 = pptTmp->y;
                else if (box.y2 < pptTmp->y)
                    box.y2 = pptTmp->y;
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

        TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->Polylines) (pDrawable, pGC, mode, npt, ppt);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePolySegment(DrawablePtr pDrawable, GCPtr pGC, int nSeg, xSegment * pSeg)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (nSeg && checkGCDamage(pDrawable, pGC)) {
        BoxRec box;
        int extra = pGC->lineWidth;
        int nsegTmp = nSeg;
        xSegment *pSegTmp = pSeg;

        if (pGC->capStyle != CapProjecting)
            extra >>= 1;

        if (pSegTmp->x2 > pSegTmp->x1) {
            box.x1 = pSegTmp->x1;
            box.x2 = pSegTmp->x2;
        }
        else {
            box.x2 = pSegTmp->x1;
            box.x1 = pSegTmp->x2;
        }

        if (pSegTmp->y2 > pSegTmp->y1) {
            box.y1 = pSegTmp->y1;
            box.y2 = pSegTmp->y2;
        }
        else {
            box.y2 = pSegTmp->y1;
            box.y1 = pSegTmp->y2;
        }

        while (--nsegTmp) {
            pSegTmp++;
            if (pSegTmp->x2 > pSegTmp->x1) {
                if (pSegTmp->x1 < box.x1)
                    box.x1 = pSegTmp->x1;
                if (pSegTmp->x2 > box.x2)
                    box.x2 = pSegTmp->x2;
            }
            else {
                if (pSegTmp->x2 < box.x1)
                    box.x1 = pSegTmp->x2;
                if (pSegTmp->x1 > box.x2)
                    box.x2 = pSegTmp->x1;
            }
            if (pSegTmp->y2 > pSegTmp->y1) {
                if (pSegTmp->y1 < box.y1)
                    box.y1 = pSegTmp->y1;
                if (pSegTmp->y2 > box.y2)
                    box.y2 = pSegTmp->y2;
            }
            else {
                if (pSegTmp->y2 < box.y1)
                    box.y1 = pSegTmp->y2;
                if (pSegTmp->y1 > box.y2)
                    box.y2 = pSegTmp->y1;
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

        TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->PolySegment) (pDrawable, pGC, nSeg, pSeg);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePolyRectangle(DrawablePtr pDrawable,
                    GCPtr pGC, int nRects, xRectangle *pRects)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (nRects && checkGCDamage(pDrawable, pGC)) {
        BoxRec box;
        int offset1, offset2, offset3;
        int nRectsTmp = nRects;
        xRectangle *pRectsTmp = pRects;

        offset2 = pGC->lineWidth;
        if (!offset2)
            offset2 = 1;
        offset1 = offset2 >> 1;
        offset3 = offset2 - offset1;

        while (nRectsTmp--) {
            box.x1 = pRectsTmp->x - offset1;
            box.y1 = pRectsTmp->y - offset1;
            box.x2 = box.x1 + pRectsTmp->width + offset2;
            box.y2 = box.y1 + offset2;
            TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
            if (BOX_NOT_EMPTY(box))
                damageDamageBox(pDrawable, &box, pGC->subWindowMode);

            box.x1 = pRectsTmp->x - offset1;
            box.y1 = pRectsTmp->y + offset3;
            box.x2 = box.x1 + offset2;
            box.y2 = box.y1 + pRectsTmp->height - offset2;
            TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
            if (BOX_NOT_EMPTY(box))
                damageDamageBox(pDrawable, &box, pGC->subWindowMode);

            box.x1 = pRectsTmp->x + pRectsTmp->width - offset1;
            box.y1 = pRectsTmp->y + offset3;
            box.x2 = box.x1 + offset2;
            box.y2 = box.y1 + pRectsTmp->height - offset2;
            TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
            if (BOX_NOT_EMPTY(box))
                damageDamageBox(pDrawable, &box, pGC->subWindowMode);

            box.x1 = pRectsTmp->x - offset1;
            box.y1 = pRectsTmp->y + pRectsTmp->height - offset1;
            box.x2 = box.x1 + pRectsTmp->width + offset2;
            box.y2 = box.y1 + offset2;
            TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
            if (BOX_NOT_EMPTY(box))
                damageDamageBox(pDrawable, &box, pGC->subWindowMode);

            pRectsTmp++;
        }
    }
    (*pGC->ops->PolyRectangle) (pDrawable, pGC, nRects, pRects);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePolyArc(DrawablePtr pDrawable, GCPtr pGC, int nArcs, xArc * pArcs)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (nArcs && checkGCDamage(pDrawable, pGC)) {
        int extra = pGC->lineWidth >> 1;
        BoxRec box;
        int nArcsTmp = nArcs;
        xArc *pArcsTmp = pArcs;

        box.x1 = pArcsTmp->x;
        box.x2 = box.x1 + pArcsTmp->width;
        box.y1 = pArcsTmp->y;
        box.y2 = box.y1 + pArcsTmp->height;

        while (--nArcsTmp) {
            pArcsTmp++;
            if (box.x1 > pArcsTmp->x)
                box.x1 = pArcsTmp->x;
            if (box.x2 < (pArcsTmp->x + pArcsTmp->width))
                box.x2 = pArcsTmp->x + pArcsTmp->width;
            if (box.y1 > pArcsTmp->y)
                box.y1 = pArcsTmp->y;
            if (box.y2 < (pArcsTmp->y + pArcsTmp->height))
                box.y2 = pArcsTmp->y + pArcsTmp->height;
        }

        if (extra) {
            box.x1 -= extra;
            box.x2 += extra;
            box.y1 -= extra;
            box.y2 += extra;
        }

        box.x2++;
        box.y2++;

        TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->PolyArc) (pDrawable, pGC, nArcs, pArcs);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damageFillPolygon(DrawablePtr pDrawable,
                  GCPtr pGC, int shape, int mode, int npt, DDXPointPtr ppt)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (npt > 2 && checkGCDamage(pDrawable, pGC)) {
        DDXPointPtr pptTmp = ppt;
        int nptTmp = npt;
        BoxRec box;

        box.x2 = box.x1 = pptTmp->x;
        box.y2 = box.y1 = pptTmp->y;

        if (mode != CoordModeOrigin) {
            int x = box.x1;
            int y = box.y1;

            while (--nptTmp) {
                pptTmp++;
                x += pptTmp->x;
                y += pptTmp->y;
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
            while (--nptTmp) {
                pptTmp++;
                if (box.x1 > pptTmp->x)
                    box.x1 = pptTmp->x;
                else if (box.x2 < pptTmp->x)
                    box.x2 = pptTmp->x;
                if (box.y1 > pptTmp->y)
                    box.y1 = pptTmp->y;
                else if (box.y2 < pptTmp->y)
                    box.y2 = pptTmp->y;
            }
        }

        box.x2++;
        box.y2++;

        TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }

    (*pGC->ops->FillPolygon) (pDrawable, pGC, shape, mode, npt, ppt);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePolyFillRect(DrawablePtr pDrawable,
                   GCPtr pGC, int nRects, xRectangle *pRects)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    if (nRects && checkGCDamage(pDrawable, pGC)) {
        BoxRec box;
        xRectangle *pRectsTmp = pRects;
        int nRectsTmp = nRects;

        box.x1 = pRectsTmp->x;
        box.x2 = box.x1 + pRectsTmp->width;
        box.y1 = pRectsTmp->y;
        box.y2 = box.y1 + pRectsTmp->height;

        while (--nRectsTmp) {
            pRectsTmp++;
            if (box.x1 > pRectsTmp->x)
                box.x1 = pRectsTmp->x;
            if (box.x2 < (pRectsTmp->x + pRectsTmp->width))
                box.x2 = pRectsTmp->x + pRectsTmp->width;
            if (box.y1 > pRectsTmp->y)
                box.y1 = pRectsTmp->y;
            if (box.y2 < (pRectsTmp->y + pRectsTmp->height))
                box.y2 = pRectsTmp->y + pRectsTmp->height;
        }

        TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->PolyFillRect) (pDrawable, pGC, nRects, pRects);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePolyFillArc(DrawablePtr pDrawable, GCPtr pGC, int nArcs, xArc * pArcs)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (nArcs && checkGCDamage(pDrawable, pGC)) {
        BoxRec box;
        int nArcsTmp = nArcs;
        xArc *pArcsTmp = pArcs;

        box.x1 = pArcsTmp->x;
        box.x2 = box.x1 + pArcsTmp->width;
        box.y1 = pArcsTmp->y;
        box.y2 = box.y1 + pArcsTmp->height;

        while (--nArcsTmp) {
            pArcsTmp++;
            if (box.x1 > pArcsTmp->x)
                box.x1 = pArcsTmp->x;
            if (box.x2 < (pArcsTmp->x + pArcsTmp->width))
                box.x2 = pArcsTmp->x + pArcsTmp->width;
            if (box.y1 > pArcsTmp->y)
                box.y1 = pArcsTmp->y;
            if (box.y2 < (pArcsTmp->y + pArcsTmp->height))
                box.y2 = pArcsTmp->y + pArcsTmp->height;
        }

        TRIM_AND_TRANSLATE_BOX(box, pDrawable, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->PolyFillArc) (pDrawable, pGC, nArcs, pArcs);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

/*
 * general Poly/Image text function.  Extract glyph information,
 * compute bounding box and remove cursor if it is overlapped.
 */

static void
damageDamageChars(DrawablePtr pDrawable,
                  FontPtr font,
                  int x,
                  int y,
                  unsigned int n,
                  CharInfoPtr * charinfo, Bool imageblt, int subWindowMode)
{
    ExtentInfoRec extents;
    BoxRec box;

    xfont2_query_glyph_extents(font, charinfo, n, &extents);
    if (imageblt) {
        if (extents.overallWidth > extents.overallRight)
            extents.overallRight = extents.overallWidth;
        if (extents.overallWidth < extents.overallLeft)
            extents.overallLeft = extents.overallWidth;
        if (extents.overallLeft > 0)
            extents.overallLeft = 0;
        if (extents.fontAscent > extents.overallAscent)
            extents.overallAscent = extents.fontAscent;
        if (extents.fontDescent > extents.overallDescent)
            extents.overallDescent = extents.fontDescent;
    }
    box.x1 = x + extents.overallLeft;
    box.y1 = y - extents.overallAscent;
    box.x2 = x + extents.overallRight;
    box.y2 = y + extents.overallDescent;
    damageDamageBox(pDrawable, &box, subWindowMode);
}

/*
 * values for textType:
 */
#define TT_POLY8   0
#define TT_IMAGE8  1
#define TT_POLY16  2
#define TT_IMAGE16 3

static void
damageText(DrawablePtr pDrawable,
           GCPtr pGC,
           int x,
           int y,
           unsigned long count,
           char *chars, FontEncoding fontEncoding, Bool textType)
{
    CharInfoPtr *charinfo;
    unsigned long i;
    unsigned int n;
    Bool imageblt;

    imageblt = (textType == TT_IMAGE8) || (textType == TT_IMAGE16);

    if (!checkGCDamage(pDrawable, pGC))
        return;

    charinfo = xallocarray(count, sizeof(CharInfoPtr));
    if (!charinfo)
        return;

    GetGlyphs(pGC->font, count, (unsigned char *) chars,
              fontEncoding, &i, charinfo);
    n = (unsigned int) i;

    if (n != 0) {
        damageDamageChars(pDrawable, pGC->font, x + pDrawable->x,
                          y + pDrawable->y, n, charinfo, imageblt,
                          pGC->subWindowMode);
    }
    free(charinfo);
}

static int
damagePolyText8(DrawablePtr pDrawable,
                GCPtr pGC, int x, int y, int count, char *chars)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    damageText(pDrawable, pGC, x, y, (unsigned long) count, chars, Linear8Bit,
               TT_POLY8);
    x = (*pGC->ops->PolyText8) (pDrawable, pGC, x, y, count, chars);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
    return x;
}

static int
damagePolyText16(DrawablePtr pDrawable,
                 GCPtr pGC, int x, int y, int count, unsigned short *chars)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    damageText(pDrawable, pGC, x, y, (unsigned long) count, (char *) chars,
               FONTLASTROW(pGC->font) == 0 ? Linear16Bit : TwoD16Bit,
               TT_POLY16);
    x = (*pGC->ops->PolyText16) (pDrawable, pGC, x, y, count, chars);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
    return x;
}

static void
damageImageText8(DrawablePtr pDrawable,
                 GCPtr pGC, int x, int y, int count, char *chars)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    damageText(pDrawable, pGC, x, y, (unsigned long) count, chars, Linear8Bit,
               TT_IMAGE8);
    (*pGC->ops->ImageText8) (pDrawable, pGC, x, y, count, chars);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damageImageText16(DrawablePtr pDrawable,
                  GCPtr pGC, int x, int y, int count, unsigned short *chars)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    damageText(pDrawable, pGC, x, y, (unsigned long) count, (char *) chars,
               FONTLASTROW(pGC->font) == 0 ? Linear16Bit : TwoD16Bit,
               TT_IMAGE16);
    (*pGC->ops->ImageText16) (pDrawable, pGC, x, y, count, chars);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damageImageGlyphBlt(DrawablePtr pDrawable,
                    GCPtr pGC,
                    int x,
                    int y,
                    unsigned int nglyph, CharInfoPtr * ppci, void *pglyphBase)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    damageDamageChars(pDrawable, pGC->font, x + pDrawable->x, y + pDrawable->y,
                      nglyph, ppci, TRUE, pGC->subWindowMode);
    (*pGC->ops->ImageGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePolyGlyphBlt(DrawablePtr pDrawable,
                   GCPtr pGC,
                   int x,
                   int y,
                   unsigned int nglyph, CharInfoPtr * ppci, void *pglyphBase)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    damageDamageChars(pDrawable, pGC->font, x + pDrawable->x, y + pDrawable->y,
                      nglyph, ppci, FALSE, pGC->subWindowMode);
    (*pGC->ops->PolyGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damagePushPixels(GCPtr pGC,
                 PixmapPtr pBitMap,
                 DrawablePtr pDrawable, int dx, int dy, int xOrg, int yOrg)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);
    if (checkGCDamage(pDrawable, pGC)) {
        BoxRec box;

        box.x1 = xOrg;
        box.y1 = yOrg;

        if (!pGC->miTranslate) {
            box.x1 += pDrawable->x;
            box.y1 += pDrawable->y;
        }

        box.x2 = box.x1 + dx;
        box.y2 = box.y1 + dy;

        TRIM_BOX(box, pGC);
        if (BOX_NOT_EMPTY(box))
            damageDamageBox(pDrawable, &box, pGC->subWindowMode);
    }
    (*pGC->ops->PushPixels) (pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg);
    damageRegionProcessPending(pDrawable);
    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damageRemoveDamage(DamagePtr * pPrev, DamagePtr pDamage)
{
    while (*pPrev) {
        if (*pPrev == pDamage) {
            *pPrev = pDamage->pNext;
            return;
        }
        pPrev = &(*pPrev)->pNext;
    }
#if DAMAGE_VALIDATE_ENABLE
    ErrorF("Damage not on list\n");
    OsAbort();
#endif
}

static void
damageInsertDamage(DamagePtr * pPrev, DamagePtr pDamage)
{
#if DAMAGE_VALIDATE_ENABLE
    DamagePtr pOld;

    for (pOld = *pPrev; pOld; pOld = pOld->pNext)
        if (pOld == pDamage) {
            ErrorF("Damage already on list\n");
            OsAbort();
        }
#endif
    pDamage->pNext = *pPrev;
    *pPrev = pDamage;
}

static Bool
damageDestroyPixmap(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    damageScrPriv(pScreen);

    if (pPixmap->refcnt == 1) {
        DamagePtr *pPrev = getPixmapDamageRef(pPixmap);
        DamagePtr pDamage;

        while ((pDamage = *pPrev)) {
            damageRemoveDamage(pPrev, pDamage);
            if (!pDamage->isWindow)
                DamageDestroy(pDamage);
        }
    }
    unwrap(pScrPriv, pScreen, DestroyPixmap);
    (*pScreen->DestroyPixmap) (pPixmap);
    wrap(pScrPriv, pScreen, DestroyPixmap, damageDestroyPixmap);
    return TRUE;
}

static void
damageCopyWindow(WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWindow->drawable.pScreen;

    damageScrPriv(pScreen);

    if (getWindowDamage(pWindow)) {
        int dx = pWindow->drawable.x - ptOldOrg.x;
        int dy = pWindow->drawable.y - ptOldOrg.y;

        /*
         * The region comes in source relative, but the damage occurs
         * at the destination location.  Translate back and forth.
         */
        RegionTranslate(prgnSrc, dx, dy);
        damageRegionAppend(&pWindow->drawable, prgnSrc, FALSE, -1);
        RegionTranslate(prgnSrc, -dx, -dy);
    }
    unwrap(pScrPriv, pScreen, CopyWindow);
    (*pScreen->CopyWindow) (pWindow, ptOldOrg, prgnSrc);
    damageRegionProcessPending(&pWindow->drawable);
    wrap(pScrPriv, pScreen, CopyWindow, damageCopyWindow);
}

static GCOps damageGCOps = {
    damageFillSpans, damageSetSpans,
    damagePutImage, damageCopyArea,
    damageCopyPlane, damagePolyPoint,
    damagePolylines, damagePolySegment,
    damagePolyRectangle, damagePolyArc,
    damageFillPolygon, damagePolyFillRect,
    damagePolyFillArc, damagePolyText8,
    damagePolyText16, damageImageText8,
    damageImageText16, damageImageGlyphBlt,
    damagePolyGlyphBlt, damagePushPixels,
};

static void
damageSetWindowPixmap(WindowPtr pWindow, PixmapPtr pPixmap)
{
    DamagePtr pDamage;
    ScreenPtr pScreen = pWindow->drawable.pScreen;

    damageScrPriv(pScreen);

    if ((pDamage = damageGetWinPriv(pWindow))) {
        PixmapPtr pOldPixmap = (*pScreen->GetWindowPixmap) (pWindow);
        DamagePtr *pPrev = getPixmapDamageRef(pOldPixmap);

        while (pDamage) {
            damageRemoveDamage(pPrev, pDamage);
            pDamage = pDamage->pNextWin;
        }
    }
    unwrap(pScrPriv, pScreen, SetWindowPixmap);
    (*pScreen->SetWindowPixmap) (pWindow, pPixmap);
    wrap(pScrPriv, pScreen, SetWindowPixmap, damageSetWindowPixmap);
    if ((pDamage = damageGetWinPriv(pWindow))) {
        DamagePtr *pPrev = getPixmapDamageRef(pPixmap);

        while (pDamage) {
            damageInsertDamage(pPrev, pDamage);
            pDamage = pDamage->pNextWin;
        }
    }
}

static Bool
damageDestroyWindow(WindowPtr pWindow)
{
    DamagePtr pDamage;
    ScreenPtr pScreen = pWindow->drawable.pScreen;
    Bool ret;

    damageScrPriv(pScreen);

    while ((pDamage = damageGetWinPriv(pWindow))) {
        DamageDestroy(pDamage);
    }
    unwrap(pScrPriv, pScreen, DestroyWindow);
    ret = (*pScreen->DestroyWindow) (pWindow);
    wrap(pScrPriv, pScreen, DestroyWindow, damageDestroyWindow);
    return ret;
}

static Bool
damageCloseScreen(ScreenPtr pScreen)
{
    damageScrPriv(pScreen);

    unwrap(pScrPriv, pScreen, DestroyPixmap);
    unwrap(pScrPriv, pScreen, CreateGC);
    unwrap(pScrPriv, pScreen, CopyWindow);
    unwrap(pScrPriv, pScreen, CloseScreen);
    free(pScrPriv);
    return (*pScreen->CloseScreen) (pScreen);
}

/**
 * Default implementations of the damage management functions.
 */
void
miDamageCreate(DamagePtr pDamage)
{
}

/*
 * We only wrap into the GC when there's a registered listener.  For windows,
 * damage includes damage to children.  So if there's a GC validated against
 * a subwindow and we then register a damage on the parent, we need to bump
 * the serial numbers of the children to re-trigger validation.
 *
 * Since we can't know if a GC has been validated against one of the affected
 * children, just bump them all to be safe.
 */
static int
damageRegisterVisit(WindowPtr pWin, void *data)
{
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    return WT_WALKCHILDREN;
}

void
miDamageRegister(DrawablePtr pDrawable, DamagePtr pDamage)
{
    if (pDrawable->type == DRAWABLE_WINDOW)
        TraverseTree((WindowPtr)pDrawable, damageRegisterVisit, NULL);
    else
        pDrawable->serialNumber = NEXT_SERIAL_NUMBER;
}

void
miDamageUnregister(DrawablePtr pDrawable, DamagePtr pDamage)
{
    if (pDrawable->type == DRAWABLE_WINDOW)
        TraverseTree((WindowPtr)pDrawable, damageRegisterVisit, NULL);
    else
        pDrawable->serialNumber = NEXT_SERIAL_NUMBER;
}

void
miDamageDestroy(DamagePtr pDamage)
{
}

/**
 * Public functions for consumption outside this file.
 */

Bool
DamageSetup(ScreenPtr pScreen)
{
    DamageScrPrivPtr pScrPriv;
    PictureScreenPtr ps = GetPictureScreenIfSet(pScreen);

    const DamageScreenFuncsRec miFuncs = {
        miDamageCreate, miDamageRegister, miDamageUnregister, miDamageDestroy
    };

    if (!dixRegisterPrivateKey(&damageScrPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (dixLookupPrivate(&pScreen->devPrivates, damageScrPrivateKey))
        return TRUE;

    if (!dixRegisterPrivateKey
        (&damageGCPrivateKeyRec, PRIVATE_GC, sizeof(DamageGCPrivRec)))
        return FALSE;

    if (!dixRegisterPrivateKey(&damagePixPrivateKeyRec, PRIVATE_PIXMAP, 0))
        return FALSE;

    if (!dixRegisterPrivateKey(&damageWinPrivateKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;

    pScrPriv = malloc(sizeof(DamageScrPrivRec));
    if (!pScrPriv)
        return FALSE;

    pScrPriv->internalLevel = 0;
    pScrPriv->pScreenDamage = 0;

    wrap(pScrPriv, pScreen, DestroyPixmap, damageDestroyPixmap);
    wrap(pScrPriv, pScreen, CreateGC, damageCreateGC);
    wrap(pScrPriv, pScreen, DestroyWindow, damageDestroyWindow);
    wrap(pScrPriv, pScreen, SetWindowPixmap, damageSetWindowPixmap);
    wrap(pScrPriv, pScreen, CopyWindow, damageCopyWindow);
    wrap(pScrPriv, pScreen, CloseScreen, damageCloseScreen);
    if (ps) {
        wrap(pScrPriv, ps, Glyphs, damageGlyphs);
        wrap(pScrPriv, ps, Composite, damageComposite);
        wrap(pScrPriv, ps, AddTraps, damageAddTraps);
    }

    pScrPriv->funcs = miFuncs;

    dixSetPrivate(&pScreen->devPrivates, damageScrPrivateKey, pScrPriv);
    return TRUE;
}

DamagePtr
DamageCreate(DamageReportFunc damageReport,
             DamageDestroyFunc damageDestroy,
             DamageReportLevel damageLevel,
             Bool isInternal, ScreenPtr pScreen, void *closure)
{
    damageScrPriv(pScreen);
    DamagePtr pDamage;

    pDamage = calloc(1, sizeof(DamageRec));
    if (!pDamage)
        return 0;
    pDamage->pNext = 0;
    pDamage->pNextWin = 0;
    RegionNull(&pDamage->damage);
    RegionNull(&pDamage->pendingDamage);

    pDamage->damageLevel = damageLevel;
    pDamage->isInternal = isInternal;
    pDamage->closure = closure;
    pDamage->isWindow = FALSE;
    pDamage->pDrawable = 0;
    pDamage->reportAfter = FALSE;

    pDamage->damageReport = damageReport;
    pDamage->damageDestroy = damageDestroy;
    pDamage->pScreen = pScreen;

    (*pScrPriv->funcs.Create) (pDamage);

    return pDamage;
}

void
DamageRegister(DrawablePtr pDrawable, DamagePtr pDamage)
{
    ScreenPtr pScreen = pDrawable->pScreen;

    damageScrPriv(pScreen);

#if DAMAGE_VALIDATE_ENABLE
    if (pDrawable->pScreen != pDamage->pScreen) {
        ErrorF("DamageRegister called with mismatched screens\n");
        OsAbort();
    }
#endif

    if (pDrawable->type == DRAWABLE_WINDOW) {
        WindowPtr pWindow = (WindowPtr) pDrawable;

        winDamageRef(pWindow);

#if DAMAGE_VALIDATE_ENABLE
        DamagePtr pOld;

        for (pOld = *pPrev; pOld; pOld = pOld->pNextWin)
            if (pOld == pDamage) {
                ErrorF("Damage already on window list\n");
                OsAbort();
            }
#endif
        pDamage->pNextWin = *pPrev;
        *pPrev = pDamage;
        pDamage->isWindow = TRUE;
    }
    else
        pDamage->isWindow = FALSE;
    pDamage->pDrawable = pDrawable;
    damageInsertDamage(getDrawableDamageRef(pDrawable), pDamage);
    (*pScrPriv->funcs.Register) (pDrawable, pDamage);
}

void
DamageDrawInternal(ScreenPtr pScreen, Bool enable)
{
    damageScrPriv(pScreen);

    pScrPriv->internalLevel += enable ? 1 : -1;
}

void
DamageUnregister(DamagePtr pDamage)
{
    DrawablePtr pDrawable = pDamage->pDrawable;
    ScreenPtr pScreen = pDrawable->pScreen;

    damageScrPriv(pScreen);

    (*pScrPriv->funcs.Unregister) (pDrawable, pDamage);

    if (pDrawable->type == DRAWABLE_WINDOW) {
        WindowPtr pWindow = (WindowPtr) pDrawable;

        winDamageRef(pWindow);
#if DAMAGE_VALIDATE_ENABLE
        int found = 0;
#endif

        while (*pPrev) {
            if (*pPrev == pDamage) {
                *pPrev = pDamage->pNextWin;
#if DAMAGE_VALIDATE_ENABLE
                found = 1;
#endif
                break;
            }
            pPrev = &(*pPrev)->pNextWin;
        }
#if DAMAGE_VALIDATE_ENABLE
        if (!found) {
            ErrorF("Damage not on window list\n");
            OsAbort();
        }
#endif
    }
    pDamage->pDrawable = 0;
    damageRemoveDamage(getDrawableDamageRef(pDrawable), pDamage);
}

void
DamageDestroy(DamagePtr pDamage)
{
    ScreenPtr pScreen = pDamage->pScreen;

    damageScrPriv(pScreen);

    if (pDamage->pDrawable)
        DamageUnregister(pDamage);

    if (pDamage->damageDestroy)
        (*pDamage->damageDestroy) (pDamage, pDamage->closure);
    (*pScrPriv->funcs.Destroy) (pDamage);
    RegionUninit(&pDamage->damage);
    RegionUninit(&pDamage->pendingDamage);
    free(pDamage);
}

Bool
DamageSubtract(DamagePtr pDamage, const RegionPtr pRegion)
{
    RegionPtr pClip;
    RegionRec pixmapClip;
    DrawablePtr pDrawable = pDamage->pDrawable;

    RegionSubtract(&pDamage->damage, &pDamage->damage, pRegion);
    if (pDrawable) {
        if (pDrawable->type == DRAWABLE_WINDOW)
            pClip = &((WindowPtr) pDrawable)->borderClip;
        else {
            BoxRec box;

            box.x1 = pDrawable->x;
            box.y1 = pDrawable->y;
            box.x2 = pDrawable->x + pDrawable->width;
            box.y2 = pDrawable->y + pDrawable->height;
            RegionInit(&pixmapClip, &box, 1);
            pClip = &pixmapClip;
        }
        RegionTranslate(&pDamage->damage, pDrawable->x, pDrawable->y);
        RegionIntersect(&pDamage->damage, &pDamage->damage, pClip);
        RegionTranslate(&pDamage->damage, -pDrawable->x, -pDrawable->y);
        if (pDrawable->type != DRAWABLE_WINDOW)
            RegionUninit(&pixmapClip);
    }
    return RegionNotEmpty(&pDamage->damage);
}

void
DamageEmpty(DamagePtr pDamage)
{
    RegionEmpty(&pDamage->damage);
}

RegionPtr
DamageRegion(DamagePtr pDamage)
{
    return &pDamage->damage;
}

RegionPtr
DamagePendingRegion(DamagePtr pDamage)
{
    return &pDamage->pendingDamage;
}

void
DamageRegionAppend(DrawablePtr pDrawable, RegionPtr pRegion)
{
    damageRegionAppend(pDrawable, pRegion, FALSE, -1);
}

void
DamageRegionProcessPending(DrawablePtr pDrawable)
{
    damageRegionProcessPending(pDrawable);
}

/* This call is very odd, i'm leaving it intact for API sake, but please don't use it. */
void
DamageDamageRegion(DrawablePtr pDrawable, RegionPtr pRegion)
{
    damageRegionAppend(pDrawable, pRegion, FALSE, -1);

    /* Go back and report this damage for DamagePtrs with reportAfter set, since
     * this call isn't part of an in-progress drawing op in the call chain and
     * the DDX probably just wants to know about it right away.
     */
    damageRegionProcessPending(pDrawable);
}

void
DamageSetReportAfterOp(DamagePtr pDamage, Bool reportAfter)
{
    pDamage->reportAfter = reportAfter;
}

DamageScreenFuncsPtr
DamageGetScreenFuncs(ScreenPtr pScreen)
{
    damageScrPriv(pScreen);
    return &pScrPriv->funcs;
}

void
DamageReportDamage(DamagePtr pDamage, RegionPtr pDamageRegion)
{
    BoxRec tmpBox;
    RegionRec tmpRegion;
    Bool was_empty;

    switch (pDamage->damageLevel) {
    case DamageReportRawRegion:
        RegionUnion(&pDamage->damage, &pDamage->damage, pDamageRegion);
        (*pDamage->damageReport) (pDamage, pDamageRegion, pDamage->closure);
        break;
    case DamageReportDeltaRegion:
        RegionNull(&tmpRegion);
        RegionSubtract(&tmpRegion, pDamageRegion, &pDamage->damage);
        if (RegionNotEmpty(&tmpRegion)) {
            RegionUnion(&pDamage->damage, &pDamage->damage, pDamageRegion);
            (*pDamage->damageReport) (pDamage, &tmpRegion, pDamage->closure);
        }
        RegionUninit(&tmpRegion);
        break;
    case DamageReportBoundingBox:
        tmpBox = *RegionExtents(&pDamage->damage);
        RegionUnion(&pDamage->damage, &pDamage->damage, pDamageRegion);
        if (!BOX_SAME(&tmpBox, RegionExtents(&pDamage->damage))) {
            (*pDamage->damageReport) (pDamage, &pDamage->damage,
                                      pDamage->closure);
        }
        break;
    case DamageReportNonEmpty:
        was_empty = !RegionNotEmpty(&pDamage->damage);
        RegionUnion(&pDamage->damage, &pDamage->damage, pDamageRegion);
        if (was_empty && RegionNotEmpty(&pDamage->damage)) {
            (*pDamage->damageReport) (pDamage, &pDamage->damage,
                                      pDamage->closure);
        }
        break;
    case DamageReportNone:
        RegionUnion(&pDamage->damage, &pDamage->damage, pDamageRegion);
        break;
    }
}
