/*
 * Copyright (c) 2009 Tiago Vignatti
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "misc.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include <X11/X.h>
#include "colormapst.h"
#include "scrnintstr.h"
#include "screenint.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "pixmap.h"
#include "windowstr.h"
#include "window.h"
#include "xf86str.h"
#include "mipointer.h"
#include "mipointrst.h"
#include "picturestr.h"

#define WRAP_SCREEN(x,y) {pScreenPriv->x = pScreen->x; pScreen->x = y;}

#define UNWRAP_SCREEN(x) pScreen->x = pScreenPriv->x

#define SCREEN_PRIV()   ((VGAarbiterScreenPtr) dixLookupPrivate(&(pScreen)->devPrivates, VGAarbiterScreenKey))

#define SCREEN_PROLOG(x) (pScreen->x = SCREEN_PRIV()->x)

#define SCREEN_EPILOG(x,y) do {                 \
        SCREEN_PRIV()->x = pScreen->x;          \
        pScreen->x = y;                         \
    } while (0)

#define WRAP_PICT(x,y) if (ps) {pScreenPriv->x = ps->x;\
    ps->x = y;}

#define UNWRAP_PICT(x) if (ps) {ps->x = pScreenPriv->x;}

#define PICTURE_PROLOGUE(field) ps->field = \
    ((VGAarbiterScreenPtr)dixLookupPrivate(&(pScreen)->devPrivates, \
    VGAarbiterScreenKey))->field

#define PICTURE_EPILOGUE(field, wrap) ps->field = wrap

#define WRAP_SCREEN_INFO(x,y) do {pScreenPriv->x = pScrn->x; pScrn->x = y;} while(0)

#define UNWRAP_SCREEN_INFO(x) pScrn->x = pScreenPriv->x

#define SPRITE_PROLOG                                           \
    miPointerScreenPtr PointPriv;                               \
    VGAarbiterScreenPtr pScreenPriv;                            \
    input_lock();                                               \
    PointPriv = dixLookupPrivate(&pScreen->devPrivates,         \
                                 miPointerScreenKey);           \
    pScreenPriv = dixLookupPrivate(&(pScreen)->devPrivates,     \
                                   VGAarbiterScreenKey);        \
    PointPriv->spriteFuncs = pScreenPriv->miSprite;             \

#define SPRITE_EPILOG                                   \
    pScreenPriv->miSprite = PointPriv->spriteFuncs;     \
    PointPriv->spriteFuncs  = &VGAarbiterSpriteFuncs;   \
    input_unlock();

#define WRAP_SPRITE do { pScreenPriv->miSprite = PointPriv->spriteFuncs;\
    	PointPriv->spriteFuncs  = &VGAarbiterSpriteFuncs; 		\
	} while (0)

#define UNWRAP_SPRITE PointPriv->spriteFuncs = pScreenPriv->miSprite

#define GC_WRAP(x) pGCPriv->wrapOps = (x)->ops;\
    pGCPriv->wrapFuncs = (x)->funcs; (x)->ops = &VGAarbiterGCOps;\
    (x)->funcs = &VGAarbiterGCFuncs;

#define GC_UNWRAP(x) VGAarbiterGCPtr  pGCPriv = \
    (VGAarbiterGCPtr)dixLookupPrivate(&(x)->devPrivates, VGAarbiterGCKey);\
    (x)->ops = pGCPriv->wrapOps; (x)->funcs = pGCPriv->wrapFuncs;

static inline void
VGAGet(ScreenPtr pScreen)
{
    pci_device_vgaarb_set_target(xf86ScreenToScrn(pScreen)->vgaDev);
    pci_device_vgaarb_lock();
}

static inline void
VGAPut(void)
{
    pci_device_vgaarb_unlock();
}

typedef struct _VGAarbiterScreen {
    CreateGCProcPtr CreateGC;
    CloseScreenProcPtr CloseScreen;
    ScreenBlockHandlerProcPtr BlockHandler;
    ScreenWakeupHandlerProcPtr WakeupHandler;
    GetImageProcPtr GetImage;
    GetSpansProcPtr GetSpans;
    SourceValidateProcPtr SourceValidate;
    CopyWindowProcPtr CopyWindow;
    ClearToBackgroundProcPtr ClearToBackground;
    CreatePixmapProcPtr CreatePixmap;
    SaveScreenProcPtr SaveScreen;
    /* Colormap */
    StoreColorsProcPtr StoreColors;
    /* Cursor */
    DisplayCursorProcPtr DisplayCursor;
    RealizeCursorProcPtr RealizeCursor;
    UnrealizeCursorProcPtr UnrealizeCursor;
    RecolorCursorProcPtr RecolorCursor;
    SetCursorPositionProcPtr SetCursorPosition;
    void (*AdjustFrame) (ScrnInfoPtr, int, int);
    Bool (*SwitchMode) (ScrnInfoPtr, DisplayModePtr);
    Bool (*EnterVT) (ScrnInfoPtr);
    void (*LeaveVT) (ScrnInfoPtr);
    void (*FreeScreen) (ScrnInfoPtr);
    miPointerSpriteFuncPtr miSprite;
    CompositeProcPtr Composite;
    GlyphsProcPtr Glyphs;
    CompositeRectsProcPtr CompositeRects;
} VGAarbiterScreenRec, *VGAarbiterScreenPtr;

typedef struct _VGAarbiterGC {
    const GCOps *wrapOps;
    const GCFuncs *wrapFuncs;
} VGAarbiterGCRec, *VGAarbiterGCPtr;

/* Screen funcs */
static void VGAarbiterBlockHandler(ScreenPtr pScreen, void *pTimeout);
static void VGAarbiterWakeupHandler(ScreenPtr pScreen, int result);
static Bool VGAarbiterCloseScreen(ScreenPtr pScreen);
static void VGAarbiterGetImage(DrawablePtr pDrawable, int sx, int sy, int w,
                               int h, unsigned int format,
                               unsigned long planemask, char *pdstLine);
static void VGAarbiterGetSpans(DrawablePtr pDrawable, int wMax, DDXPointPtr ppt,
                               int *pwidth, int nspans, char *pdstStart);
static void VGAarbiterSourceValidate(DrawablePtr pDrawable, int x, int y,
                                     int width, int height,
                                     unsigned int subWindowMode);
static void VGAarbiterCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg,
                                 RegionPtr prgnSrc);
static void VGAarbiterClearToBackground(WindowPtr pWin, int x, int y, int w,
                                        int h, Bool generateExposures);
static PixmapPtr VGAarbiterCreatePixmap(ScreenPtr pScreen, int w, int h,
                                        int depth, unsigned int usage_hint);
static Bool VGAarbiterCreateGC(GCPtr pGC);
static Bool VGAarbiterSaveScreen(ScreenPtr pScreen, Bool unblank);
static void VGAarbiterStoreColors(ColormapPtr pmap, int ndef, xColorItem
                                  * pdefs);
static void VGAarbiterRecolorCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                    CursorPtr pCurs, Bool displayed);
static Bool VGAarbiterRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                    CursorPtr pCursor);
static Bool VGAarbiterUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                      CursorPtr pCursor);
static Bool VGAarbiterDisplayCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                    CursorPtr pCursor);
static Bool VGAarbiterSetCursorPosition(DeviceIntPtr pDev, ScreenPtr
                                        pScreen, int x, int y,
                                        Bool generateEvent);
static void VGAarbiterAdjustFrame(ScrnInfoPtr pScrn, int x, int y);
static Bool VGAarbiterSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool VGAarbiterEnterVT(ScrnInfoPtr pScrn);
static void VGAarbiterLeaveVT(ScrnInfoPtr pScrn);
static void VGAarbiterFreeScreen(ScrnInfoPtr pScrn);

/* GC funcs */
static void VGAarbiterValidateGC(GCPtr pGC, unsigned long changes,
                                 DrawablePtr pDraw);
static void VGAarbiterChangeGC(GCPtr pGC, unsigned long mask);
static void VGAarbiterCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);
static void VGAarbiterDestroyGC(GCPtr pGC);
static void VGAarbiterChangeClip(GCPtr pGC, int type, void *pvalue,
                                 int nrects);
static void VGAarbiterDestroyClip(GCPtr pGC);
static void VGAarbiterCopyClip(GCPtr pgcDst, GCPtr pgcSrc);

/* GC ops */
static void VGAarbiterFillSpans(DrawablePtr pDraw, GC * pGC, int nInit,
                                DDXPointPtr pptInit, int *pwidthInit,
                                int fSorted);
static void VGAarbiterSetSpans(DrawablePtr pDraw, GCPtr pGC, char *pcharsrc,
                               register DDXPointPtr ppt, int *pwidth,
                               int nspans, int fSorted);
static void VGAarbiterPutImage(DrawablePtr pDraw, GCPtr pGC, int depth, int x,
                               int y, int w, int h, int leftPad, int format,
                               char *pImage);
static RegionPtr VGAarbiterCopyArea(DrawablePtr pSrc, DrawablePtr pDst,
                                    GC * pGC, int srcx, int srcy, int width,
                                    int height, int dstx, int dsty);
static RegionPtr VGAarbiterCopyPlane(DrawablePtr pSrc, DrawablePtr pDst,
                                     GCPtr pGC, int srcx, int srcy, int width,
                                     int height, int dstx, int dsty,
                                     unsigned long bitPlane);
static void VGAarbiterPolyPoint(DrawablePtr pDraw, GCPtr pGC, int mode, int npt,
                                xPoint * pptInit);
static void VGAarbiterPolylines(DrawablePtr pDraw, GCPtr pGC, int mode, int npt,
                                DDXPointPtr pptInit);
static void VGAarbiterPolySegment(DrawablePtr pDraw, GCPtr pGC, int nseg,
                                  xSegment * pSeg);
static void VGAarbiterPolyRectangle(DrawablePtr pDraw, GCPtr pGC,
                                    int nRectsInit, xRectangle *pRectsInit);
static void VGAarbiterPolyArc(DrawablePtr pDraw, GCPtr pGC, int narcs,
                              xArc * parcs);
static void VGAarbiterFillPolygon(DrawablePtr pDraw, GCPtr pGC, int shape,
                                  int mode, int count, DDXPointPtr ptsIn);
static void VGAarbiterPolyFillRect(DrawablePtr pDraw, GCPtr pGC, int nrectFill,
                                   xRectangle *prectInit);
static void VGAarbiterPolyFillArc(DrawablePtr pDraw, GCPtr pGC, int narcs,
                                  xArc * parcs);
static int VGAarbiterPolyText8(DrawablePtr pDraw, GCPtr pGC, int x, int y,
                               int count, char *chars);
static int VGAarbiterPolyText16(DrawablePtr pDraw, GCPtr pGC, int x, int y,
                                int count, unsigned short *chars);
static void VGAarbiterImageText8(DrawablePtr pDraw, GCPtr pGC, int x, int y,
                                 int count, char *chars);
static void VGAarbiterImageText16(DrawablePtr pDraw, GCPtr pGC, int x, int y,
                                  int count, unsigned short *chars);
static void VGAarbiterImageGlyphBlt(DrawablePtr pDraw, GCPtr pGC, int xInit,
                                    int yInit, unsigned int nglyph,
                                    CharInfoPtr * ppci, void *pglyphBase);
static void VGAarbiterPolyGlyphBlt(DrawablePtr pDraw, GCPtr pGC, int xInit,
                                   int yInit, unsigned int nglyph,
                                   CharInfoPtr * ppci, void *pglyphBase);
static void VGAarbiterPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr
                                 pDraw, int dx, int dy, int xOrg, int yOrg);

/* miSpriteFuncs */
static Bool VGAarbiterSpriteRealizeCursor(DeviceIntPtr pDev, ScreenPtr
                                          pScreen, CursorPtr pCur);
static Bool VGAarbiterSpriteUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr
                                            pScreen, CursorPtr pCur);
static void VGAarbiterSpriteSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                      CursorPtr pCur, int x, int y);
static void VGAarbiterSpriteMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                       int x, int y);
static Bool VGAarbiterDeviceCursorInitialize(DeviceIntPtr pDev,
                                             ScreenPtr pScreen);
static void VGAarbiterDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen);

static void VGAarbiterComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask,
                                PicturePtr pDst, INT16 xSrc, INT16 ySrc,
                                INT16 xMask, INT16 yMask, INT16 xDst,
                                INT16 yDst, CARD16 width, CARD16 height);
static void VGAarbiterGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                             PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                             int nlist, GlyphListPtr list, GlyphPtr * glyphs);
static void VGAarbiterCompositeRects(CARD8 op, PicturePtr pDst,
                                     xRenderColor * color, int nRect,
                                     xRectangle *rects);
