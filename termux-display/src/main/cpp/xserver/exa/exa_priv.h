/*
 *
 * Copyright (C) 2000 Keith Packard, member of The XFree86 Project, Inc.
 *               2005 Zack Rusin, Trolltech
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
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef EXAPRIV_H
#define EXAPRIV_H

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "exa.h"

#include <X11/X.h>
#include <X11/Xproto.h>
#ifdef MITSHM
#include "shmint.h"
#endif
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "mi.h"
#include "dix.h"
#include "fb.h"
#include "fboverlay.h"
#include "fbpict.h"
#include "glyphstr.h"
#include "damage.h"

#define DEBUG_TRACE_FALL	0
#define DEBUG_MIGRATE		0
#define DEBUG_PIXMAP		0
#define DEBUG_OFFSCREEN		0
#define DEBUG_GLYPH_CACHE	0

#if DEBUG_TRACE_FALL
#define EXA_FALLBACK(x)     					\
do {								\
	ErrorF("EXA fallback at %s: ", __FUNCTION__);		\
	ErrorF x;						\
} while (0)

char
 exaDrawableLocation(DrawablePtr pDrawable);
#else
#define EXA_FALLBACK(x)
#endif

#if DEBUG_PIXMAP
#define DBG_PIXMAP(a) ErrorF a
#else
#define DBG_PIXMAP(a)
#endif

#ifndef EXA_MAX_FB
#define EXA_MAX_FB   FB_OVERLAY_MAX
#endif

#ifdef DEBUG
#define EXA_FatalErrorDebug(x) FatalError x
#define EXA_FatalErrorDebugWithRet(x, ret) FatalError x
#else
#define EXA_FatalErrorDebug(x) ErrorF x
#define EXA_FatalErrorDebugWithRet(x, ret) \
do { \
    ErrorF x; \
    return ret; \
} while (0)
#endif

/**
 * This is the list of migration heuristics supported by EXA.  See
 * exaDoMigration() for what their implementations do.
 */
enum ExaMigrationHeuristic {
    ExaMigrationGreedy,
    ExaMigrationAlways,
    ExaMigrationSmart
};

typedef struct {
    unsigned char sha1[20];
} ExaCachedGlyphRec, *ExaCachedGlyphPtr;

typedef struct {
    /* The identity of the cache, statically configured at initialization */
    unsigned int format;
    int glyphWidth;
    int glyphHeight;

    int size;                   /* Size of cache; eventually this should be dynamically determined */

    /* Hash table mapping from glyph sha1 to position in the glyph; we use
     * open addressing with a hash table size determined based on size and large
     * enough so that we always have a good amount of free space, so we can
     * use linear probing. (Linear probing is preferable to double hashing
     * here because it allows us to easily remove entries.)
     */
    int *hashEntries;
    int hashSize;

    ExaCachedGlyphPtr glyphs;
    int glyphCount;             /* Current number of glyphs */

    PicturePtr picture;         /* Where the glyphs of the cache are stored */
    int yOffset;                /* y location within the picture where the cache starts */
    int columns;                /* Number of columns the glyphs are laid out in */
    int evictionPosition;       /* Next random position to evict a glyph */
} ExaGlyphCacheRec, *ExaGlyphCachePtr;

#define EXA_NUM_GLYPH_CACHES 4

#define EXA_FALLBACK_COPYWINDOW (1 << 0)
#define EXA_ACCEL_COPYWINDOW (1 << 1)

typedef struct _ExaMigrationRec {
    Bool as_dst;
    Bool as_src;
    PixmapPtr pPix;
    RegionPtr pReg;
} ExaMigrationRec, *ExaMigrationPtr;

typedef void (*EnableDisableFBAccessProcPtr) (ScreenPtr, Bool);
typedef struct {
    ExaDriverPtr info;
    ScreenBlockHandlerProcPtr SavedBlockHandler;
    ScreenWakeupHandlerProcPtr SavedWakeupHandler;
    CreateGCProcPtr SavedCreateGC;
    CloseScreenProcPtr SavedCloseScreen;
    GetImageProcPtr SavedGetImage;
    GetSpansProcPtr SavedGetSpans;
    CreatePixmapProcPtr SavedCreatePixmap;
    DestroyPixmapProcPtr SavedDestroyPixmap;
    CopyWindowProcPtr SavedCopyWindow;
    ChangeWindowAttributesProcPtr SavedChangeWindowAttributes;
    BitmapToRegionProcPtr SavedBitmapToRegion;
    CreateScreenResourcesProcPtr SavedCreateScreenResources;
    ModifyPixmapHeaderProcPtr SavedModifyPixmapHeader;
    SharePixmapBackingProcPtr SavedSharePixmapBacking;
    SetSharedPixmapBackingProcPtr SavedSetSharedPixmapBacking;
    SourceValidateProcPtr SavedSourceValidate;
    CompositeProcPtr SavedComposite;
    TrianglesProcPtr SavedTriangles;
    GlyphsProcPtr SavedGlyphs;
    TrapezoidsProcPtr SavedTrapezoids;
    AddTrapsProcPtr SavedAddTraps;
    void (*do_migration) (ExaMigrationPtr pixmaps, int npixmaps,
                          Bool can_accel);
    Bool (*pixmap_has_gpu_copy) (PixmapPtr pPixmap);
    void (*do_move_in_pixmap) (PixmapPtr pPixmap);
    void (*do_move_out_pixmap) (PixmapPtr pPixmap);
    void (*prepare_access_reg) (PixmapPtr pPixmap, int index, RegionPtr pReg);

    Bool swappedOut;
    enum ExaMigrationHeuristic migration;
    Bool checkDirtyCorrectness;
    unsigned disableFbCount;
    Bool optimize_migration;
    unsigned offScreenCounter;
    unsigned numOffscreenAvailable;
    CARD32 lastDefragment;
    CARD32 nextDefragment;
    PixmapPtr deferred_mixed_pixmap;

    /* Reference counting for accessed pixmaps */
    struct {
        PixmapPtr pixmap;
        int count;
        Bool retval;
    } access[EXA_NUM_PREPARE_INDICES];

    /* Holds information on fallbacks that cannot be relayed otherwise. */
    unsigned int fallback_flags;
    unsigned int fallback_counter;

    ExaGlyphCacheRec glyphCaches[EXA_NUM_GLYPH_CACHES];

    /**
     * Regions affected by fallback composite source / mask operations.
     */

    RegionRec srcReg;
    RegionRec maskReg;
    PixmapPtr srcPix;
    PixmapPtr maskPix;

    DevPrivateKeyRec pixmapPrivateKeyRec;
    DevPrivateKeyRec gcPrivateKeyRec;
} ExaScreenPrivRec, *ExaScreenPrivPtr;

extern DevPrivateKeyRec exaScreenPrivateKeyRec;

#define exaScreenPrivateKey (&exaScreenPrivateKeyRec)

#define ExaGetScreenPriv(s) ((ExaScreenPrivPtr)dixGetPrivate(&(s)->devPrivates, exaScreenPrivateKey))
#define ExaScreenPriv(s)	ExaScreenPrivPtr    pExaScr = ExaGetScreenPriv(s)

#define ExaGetGCPriv(gc) ((ExaGCPrivPtr)dixGetPrivateAddr(&(gc)->devPrivates, &ExaGetScreenPriv(gc->pScreen)->gcPrivateKeyRec))
#define ExaGCPriv(gc) ExaGCPrivPtr pExaGC = ExaGetGCPriv(gc)

/*
 * Some macros to deal with function wrapping.
 */
#define wrap(priv, real, mem, func) {\
    priv->Saved##mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv, real, mem) {\
    real->mem = priv->Saved##mem; \
}

#ifdef HAVE_TYPEOF
#define swap(priv, real, mem) {\
    typeof(real->mem) tmp = priv->Saved##mem; \
    priv->Saved##mem = real->mem; \
    real->mem = tmp; \
}
#else
#define swap(priv, real, mem) {\
    const void *tmp = priv->Saved##mem; \
    priv->Saved##mem = real->mem; \
    real->mem = tmp; \
}
#endif

#define EXA_PRE_FALLBACK(_screen_) \
    ExaScreenPriv(_screen_); \
    pExaScr->fallback_counter++;

#define EXA_POST_FALLBACK(_screen_) \
    pExaScr->fallback_counter--;

#define EXA_PRE_FALLBACK_GC(_gc_) \
    ExaScreenPriv(_gc_->pScreen); \
    ExaGCPriv(_gc_); \
    pExaScr->fallback_counter++; \
    swap(pExaGC, _gc_, ops);

#define EXA_POST_FALLBACK_GC(_gc_) \
    pExaScr->fallback_counter--; \
    swap(pExaGC, _gc_, ops);

/** Align an offset to an arbitrary alignment */
#define EXA_ALIGN(offset, align) (((offset) + (align) - 1) - \
	(((offset) + (align) - 1) % (align)))
/** Align an offset to a power-of-two alignment */
#define EXA_ALIGN2(offset, align) (((offset) + (align) - 1) & ~((align) - 1))

#define EXA_PIXMAP_SCORE_MOVE_IN    10
#define EXA_PIXMAP_SCORE_MAX	    20
#define EXA_PIXMAP_SCORE_MOVE_OUT   -10
#define EXA_PIXMAP_SCORE_MIN	    -20
#define EXA_PIXMAP_SCORE_PINNED	    1000
#define EXA_PIXMAP_SCORE_INIT	    1001

#define ExaGetPixmapPriv(p) ((ExaPixmapPrivPtr)dixGetPrivateAddr(&(p)->devPrivates, &ExaGetScreenPriv((p)->drawable.pScreen)->pixmapPrivateKeyRec))
#define ExaPixmapPriv(p)	ExaPixmapPrivPtr pExaPixmap = ExaGetPixmapPriv(p)

#define EXA_RANGE_PITCH (1 << 0)
#define EXA_RANGE_WIDTH (1 << 1)
#define EXA_RANGE_HEIGHT (1 << 2)

typedef struct {
    ExaOffscreenArea *area;
    int score;                  /**< score for the move-in vs move-out heuristic */
    Bool use_gpu_copy;

    CARD8 *sys_ptr;             /**< pointer to pixmap data in system memory */
    int sys_pitch;              /**< pitch of pixmap in system memory */

    CARD8 *fb_ptr;              /**< pointer to pixmap data in framebuffer memory */
    int fb_pitch;               /**< pitch of pixmap in framebuffer memory */
    unsigned int fb_size;       /**< size of pixmap in framebuffer memory */

    /**
     * Holds information about whether this pixmap can be used for
     * acceleration (== 0) or not (> 0).
     *
     * Contains a OR'ed combination of the following values:
     * EXA_RANGE_PITCH - set if the pixmap's pitch is out of range
     * EXA_RANGE_WIDTH - set if the pixmap's width is out of range
     * EXA_RANGE_HEIGHT - set if the pixmap's height is out of range
     */
    unsigned int accel_blocked;

    /**
     * The damage record contains the areas of the pixmap's current location
     * (framebuffer or system) that have been damaged compared to the other
     * location.
     */
    DamagePtr pDamage;
    /**
     * The valid regions mark the valid bits (at least, as they're derived from
     * damage, which may be overreported) of a pixmap's system and FB copies.
     */
    RegionRec validSys, validFB;
    /**
     * Driver private storage per EXA pixmap
     */
    void *driverPriv;
} ExaPixmapPrivRec, *ExaPixmapPrivPtr;

typedef struct {
    /* GC values from the layer below. */
    const GCOps *Savedops;
    const GCFuncs *Savedfuncs;
} ExaGCPrivRec, *ExaGCPrivPtr;

typedef struct {
    PicturePtr pDst;
    INT16 xSrc;
    INT16 ySrc;
    INT16 xMask;
    INT16 yMask;
    INT16 xDst;
    INT16 yDst;
    INT16 width;
    INT16 height;
} ExaCompositeRectRec, *ExaCompositeRectPtr;

/**
 * exaDDXDriverInit must be implemented by the DDX using EXA, and is the place
 * to set EXA options or hook in screen functions to handle using EXA as the AA.
  */
void exaDDXDriverInit(ScreenPtr pScreen);

/* exa_unaccel.c */
void
 exaPrepareAccessGC(GCPtr pGC);

void
 exaFinishAccessGC(GCPtr pGC);

void

ExaCheckFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nspans,
                  DDXPointPtr ppt, int *pwidth, int fSorted);

void

ExaCheckSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *psrc,
                 DDXPointPtr ppt, int *pwidth, int nspans, int fSorted);

void

ExaCheckPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                 int x, int y, int w, int h, int leftPad, int format,
                 char *bits);

void

ExaCheckCopyNtoN(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
                 BoxPtr pbox, int nbox, int dx, int dy, Bool reverse,
                 Bool upsidedown, Pixel bitplane, void *closure);

RegionPtr

ExaCheckCopyArea(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
                 int srcx, int srcy, int w, int h, int dstx, int dsty);

RegionPtr

ExaCheckCopyPlane(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
                  int srcx, int srcy, int w, int h, int dstx, int dsty,
                  unsigned long bitPlane);

void

ExaCheckPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
                  DDXPointPtr pptInit);

void

ExaCheckPolylines(DrawablePtr pDrawable, GCPtr pGC,
                  int mode, int npt, DDXPointPtr ppt);

void

ExaCheckPolySegment(DrawablePtr pDrawable, GCPtr pGC,
                    int nsegInit, xSegment * pSegInit);

void
 ExaCheckPolyArc(DrawablePtr pDrawable, GCPtr pGC, int narcs, xArc * pArcs);

void

ExaCheckPolyFillRect(DrawablePtr pDrawable, GCPtr pGC,
                     int nrect, xRectangle *prect);

void

ExaCheckImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC,
                      int x, int y, unsigned int nglyph,
                      CharInfoPtr * ppci, void *pglyphBase);

void

ExaCheckPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC,
                     int x, int y, unsigned int nglyph,
                     CharInfoPtr * ppci, void *pglyphBase);

void

ExaCheckPushPixels(GCPtr pGC, PixmapPtr pBitmap,
                   DrawablePtr pDrawable, int w, int h, int x, int y);

void
 ExaCheckCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

void

ExaCheckGetImage(DrawablePtr pDrawable, int x, int y, int w, int h,
                 unsigned int format, unsigned long planeMask, char *d);

void

ExaCheckGetSpans(DrawablePtr pDrawable,
                 int wMax,
                 DDXPointPtr ppt, int *pwidth, int nspans, char *pdstStart);

void

ExaCheckAddTraps(PicturePtr pPicture,
                 INT16 x_off, INT16 y_off, int ntrap, xTrap * traps);

/* exa_accel.c */

static _X_INLINE Bool
exaGCReadsDestination(DrawablePtr pDrawable, unsigned long planemask,
                      unsigned int fillStyle, unsigned char alu,
                      Bool clientClip)
{
    return ((alu != GXcopy && alu != GXclear && alu != GXset &&
             alu != GXcopyInverted) || fillStyle == FillStippled ||
            clientClip != FALSE || !EXA_PM_IS_SOLID(pDrawable, planemask));
}

void
 exaCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

Bool

exaFillRegionTiled(DrawablePtr pDrawable, RegionPtr pRegion, PixmapPtr pTile,
                   DDXPointPtr pPatOrg, CARD32 planemask, CARD32 alu,
                   Bool clientClip);

void

exaGetImage(DrawablePtr pDrawable, int x, int y, int w, int h,
            unsigned int format, unsigned long planeMask, char *d);

RegionPtr

exaCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
            int srcx, int srcy, int width, int height, int dstx, int dsty);

Bool

exaHWCopyNtoN(DrawablePtr pSrcDrawable,
              DrawablePtr pDstDrawable,
              GCPtr pGC,
              BoxPtr pbox,
              int nbox, int dx, int dy, Bool reverse, Bool upsidedown);

void

exaCopyNtoN(DrawablePtr pSrcDrawable,
            DrawablePtr pDstDrawable,
            GCPtr pGC,
            BoxPtr pbox,
            int nbox,
            int dx,
            int dy,
            Bool reverse, Bool upsidedown, Pixel bitplane, void *closure);

extern const GCOps exaOps;

void

ExaCheckComposite(CARD8 op,
                  PicturePtr pSrc,
                  PicturePtr pMask,
                  PicturePtr pDst,
                  INT16 xSrc,
                  INT16 ySrc,
                  INT16 xMask,
                  INT16 yMask,
                  INT16 xDst, INT16 yDst, CARD16 width, CARD16 height);

void

ExaCheckGlyphs(CARD8 op,
               PicturePtr pSrc,
               PicturePtr pDst,
               PictFormatPtr maskFormat,
               INT16 xSrc,
               INT16 ySrc, int nlist, GlyphListPtr list, GlyphPtr * glyphs);

/* exa_offscreen.c */
void
 ExaOffscreenSwapOut(ScreenPtr pScreen);

void
 ExaOffscreenSwapIn(ScreenPtr pScreen);

ExaOffscreenArea *ExaOffscreenDefragment(ScreenPtr pScreen);

Bool
 exaOffscreenInit(ScreenPtr pScreen);

void
 ExaOffscreenFini(ScreenPtr pScreen);

/* exa.c */
Bool
 ExaDoPrepareAccess(PixmapPtr pPixmap, int index);

void
 exaPrepareAccess(DrawablePtr pDrawable, int index);

void
 exaFinishAccess(DrawablePtr pDrawable, int index);

void
 exaDestroyPixmap(PixmapPtr pPixmap);

void
 exaPixmapDirty(PixmapPtr pPix, int x1, int y1, int x2, int y2);

void

exaGetDrawableDeltas(DrawablePtr pDrawable, PixmapPtr pPixmap,
                     int *xp, int *yp);

Bool
 exaPixmapHasGpuCopy(PixmapPtr p);

PixmapPtr
 exaGetOffscreenPixmap(DrawablePtr pDrawable, int *xp, int *yp);

PixmapPtr
 exaGetDrawablePixmap(DrawablePtr pDrawable);

void

exaSetFbPitch(ExaScreenPrivPtr pExaScr, ExaPixmapPrivPtr pExaPixmap,
              int w, int h, int bpp);

void

exaSetAccelBlock(ExaScreenPrivPtr pExaScr, ExaPixmapPrivPtr pExaPixmap,
                 int w, int h, int bpp);

void
 exaDoMigration(ExaMigrationPtr pixmaps, int npixmaps, Bool can_accel);

Bool
 exaPixmapIsPinned(PixmapPtr pPix);

extern const GCFuncs exaGCFuncs;

/* exa_classic.c */
PixmapPtr

exaCreatePixmap_classic(ScreenPtr pScreen, int w, int h, int depth,
                        unsigned usage_hint);

Bool

exaModifyPixmapHeader_classic(PixmapPtr pPixmap, int width, int height,
                              int depth, int bitsPerPixel, int devKind,
                              void *pPixData);

Bool
 exaDestroyPixmap_classic(PixmapPtr pPixmap);

Bool
 exaPixmapHasGpuCopy_classic(PixmapPtr pPixmap);

/* exa_driver.c */
PixmapPtr

exaCreatePixmap_driver(ScreenPtr pScreen, int w, int h, int depth,
                       unsigned usage_hint);

Bool

exaModifyPixmapHeader_driver(PixmapPtr pPixmap, int width, int height,
                             int depth, int bitsPerPixel, int devKind,
                             void *pPixData);

Bool
 exaDestroyPixmap_driver(PixmapPtr pPixmap);

Bool
 exaPixmapHasGpuCopy_driver(PixmapPtr pPixmap);

/* exa_mixed.c */
PixmapPtr

exaCreatePixmap_mixed(ScreenPtr pScreen, int w, int h, int depth,
                      unsigned usage_hint);

Bool

exaModifyPixmapHeader_mixed(PixmapPtr pPixmap, int width, int height, int depth,
                            int bitsPerPixel, int devKind, void *pPixData);

Bool
 exaDestroyPixmap_mixed(PixmapPtr pPixmap);

Bool
 exaPixmapHasGpuCopy_mixed(PixmapPtr pPixmap);

/* exa_migration_mixed.c */
void
 exaCreateDriverPixmap_mixed(PixmapPtr pPixmap);

void
 exaDoMigration_mixed(ExaMigrationPtr pixmaps, int npixmaps, Bool can_accel);

void
 exaMoveInPixmap_mixed(PixmapPtr pPixmap);

void
 exaDamageReport_mixed(DamagePtr pDamage, RegionPtr pRegion, void *closure);

void
 exaPrepareAccessReg_mixed(PixmapPtr pPixmap, int index, RegionPtr pReg);

Bool
exaSetSharedPixmapBacking_mixed(PixmapPtr pPixmap, void *handle);
Bool
exaSharePixmapBacking_mixed(PixmapPtr pPixmap, ScreenPtr secondary, void **handle_p);

/* exa_render.c */
Bool
 exaOpReadsDestination(CARD8 op);

void

exaComposite(CARD8 op,
             PicturePtr pSrc,
             PicturePtr pMask,
             PicturePtr pDst,
             INT16 xSrc,
             INT16 ySrc,
             INT16 xMask,
             INT16 yMask, INT16 xDst, INT16 yDst, CARD16 width, CARD16 height);

void

exaCompositeRects(CARD8 op,
                  PicturePtr Src,
                  PicturePtr pMask,
                  PicturePtr pDst, int nrect, ExaCompositeRectPtr rects);

void

exaTrapezoids(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
              PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
              int ntrap, xTrapezoid * traps);

void

exaTriangles(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
             PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
             int ntri, xTriangle * tris);

/* exa_glyph.c */
void
 exaGlyphsInit(ScreenPtr pScreen);

void
 exaGlyphsFini(ScreenPtr pScreen);

void

exaGlyphs(CARD8 op,
          PicturePtr pSrc,
          PicturePtr pDst,
          PictFormatPtr maskFormat,
          INT16 xSrc,
          INT16 ySrc, int nlist, GlyphListPtr list, GlyphPtr * glyphs);

/* exa_migration_classic.c */
void
 exaCopyDirtyToSys(ExaMigrationPtr migrate);

void
 exaCopyDirtyToFb(ExaMigrationPtr migrate);

void
 exaDoMigration_classic(ExaMigrationPtr pixmaps, int npixmaps, Bool can_accel);

void
 exaPixmapSave(ScreenPtr pScreen, ExaOffscreenArea * area);

void
 exaMoveOutPixmap_classic(PixmapPtr pPixmap);

void
 exaMoveInPixmap_classic(PixmapPtr pPixmap);

void
 exaPrepareAccessReg_classic(PixmapPtr pPixmap, int index, RegionPtr pReg);

#endif                          /* EXAPRIV_H */
