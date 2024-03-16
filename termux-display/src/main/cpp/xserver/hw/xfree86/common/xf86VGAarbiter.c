/*
 * This code was stolen from RAC and adapted to control the legacy vga
 * interface.
 *
 *
 * Copyright (c) 2007 Paulo R. Zanoni, Tiago Vignatti
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

#include "xorg-config.h"

#include "xf86VGAarbiter.h"
#include "xf86VGAarbiterPriv.h"
#include "xf86Bus.h"
#include "xf86Priv.h"
#include "pciaccess.h"

static GCFuncs VGAarbiterGCFuncs = {
    VGAarbiterValidateGC, VGAarbiterChangeGC, VGAarbiterCopyGC,
    VGAarbiterDestroyGC, VGAarbiterChangeClip, VGAarbiterDestroyClip,
    VGAarbiterCopyClip
};

static GCOps VGAarbiterGCOps = {
    VGAarbiterFillSpans, VGAarbiterSetSpans, VGAarbiterPutImage,
    VGAarbiterCopyArea, VGAarbiterCopyPlane, VGAarbiterPolyPoint,
    VGAarbiterPolylines, VGAarbiterPolySegment, VGAarbiterPolyRectangle,
    VGAarbiterPolyArc, VGAarbiterFillPolygon, VGAarbiterPolyFillRect,
    VGAarbiterPolyFillArc, VGAarbiterPolyText8, VGAarbiterPolyText16,
    VGAarbiterImageText8, VGAarbiterImageText16, VGAarbiterImageGlyphBlt,
    VGAarbiterPolyGlyphBlt, VGAarbiterPushPixels,
};

static miPointerSpriteFuncRec VGAarbiterSpriteFuncs = {
    VGAarbiterSpriteRealizeCursor, VGAarbiterSpriteUnrealizeCursor,
    VGAarbiterSpriteSetCursor, VGAarbiterSpriteMoveCursor,
    VGAarbiterDeviceCursorInitialize, VGAarbiterDeviceCursorCleanup
};

static DevPrivateKeyRec VGAarbiterScreenKeyRec;

#define VGAarbiterScreenKey (&VGAarbiterScreenKeyRec)
static DevPrivateKeyRec VGAarbiterGCKeyRec;

#define VGAarbiterGCKey (&VGAarbiterGCKeyRec)

static int vga_no_arb = 0;
void
xf86VGAarbiterInit(void)
{
    if (pci_device_vgaarb_init() != 0) {
        vga_no_arb = 1;
        xf86Msg(X_WARNING,
                "VGA arbiter: cannot open kernel arbiter, no multi-card support\n");
    }
}

void
xf86VGAarbiterFini(void)
{
    if (vga_no_arb)
        return;
    pci_device_vgaarb_fini();
}

void
xf86VGAarbiterLock(ScrnInfoPtr pScrn)
{
    if (vga_no_arb)
        return;
    pci_device_vgaarb_set_target(pScrn->vgaDev);
    pci_device_vgaarb_lock();
}

void
xf86VGAarbiterUnlock(ScrnInfoPtr pScrn)
{
    if (vga_no_arb)
        return;
    pci_device_vgaarb_unlock();
}

Bool
xf86VGAarbiterAllowDRI(ScreenPtr pScreen)
{
    int vga_count;
    int rsrc_decodes = 0;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

    if (vga_no_arb)
        return TRUE;

    pci_device_vgaarb_get_info(pScrn->vgaDev, &vga_count, &rsrc_decodes);
    if (vga_count > 1) {
        if (rsrc_decodes) {
            return FALSE;
        }
    }
    return TRUE;
}

void
xf86VGAarbiterScrnInit(ScrnInfoPtr pScrn)
{
    struct pci_device *dev;
    EntityPtr pEnt;

    if (vga_no_arb)
        return;

    pEnt = xf86Entities[pScrn->entityList[0]];
    if (pEnt->bus.type != BUS_PCI)
        return;

    dev = pEnt->bus.id.pci;
    pScrn->vgaDev = dev;
}

void
xf86VGAarbiterDeviceDecodes(ScrnInfoPtr pScrn, int rsrc)
{
    if (vga_no_arb)
        return;
    pci_device_vgaarb_set_target(pScrn->vgaDev);
    pci_device_vgaarb_decodes(rsrc);
}

Bool
xf86VGAarbiterWrapFunctions(void)
{
    ScrnInfoPtr pScrn;
    VGAarbiterScreenPtr pScreenPriv;
    miPointerScreenPtr PointPriv;
    PictureScreenPtr ps;
    ScreenPtr pScreen;
    int vga_count, i;

    if (vga_no_arb)
        return FALSE;

    /*
     * we need to wrap the arbiter if we have more than
     * one VGA card - hotplug cries.
     */
    pci_device_vgaarb_get_info(NULL, &vga_count, NULL);
    if (vga_count < 2 || !xf86Screens)
        return FALSE;

    xf86Msg(X_INFO, "Found %d VGA devices: arbiter wrapping enabled\n",
            vga_count);

    for (i = 0; i < xf86NumScreens; i++) {
        pScreen = xf86Screens[i]->pScreen;
        ps = GetPictureScreenIfSet(pScreen);
        pScrn = xf86ScreenToScrn(pScreen);
        PointPriv = dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

        if (!dixRegisterPrivateKey
            (&VGAarbiterGCKeyRec, PRIVATE_GC, sizeof(VGAarbiterGCRec)))
            return FALSE;

        if (!dixRegisterPrivateKey(&VGAarbiterScreenKeyRec, PRIVATE_SCREEN, 0))
            return FALSE;

        if (!(pScreenPriv = malloc(sizeof(VGAarbiterScreenRec))))
            return FALSE;

        dixSetPrivate(&pScreen->devPrivates, VGAarbiterScreenKey, pScreenPriv);

        WRAP_SCREEN(CloseScreen, VGAarbiterCloseScreen);
        WRAP_SCREEN(SaveScreen, VGAarbiterSaveScreen);
        WRAP_SCREEN(WakeupHandler, VGAarbiterWakeupHandler);
        WRAP_SCREEN(BlockHandler, VGAarbiterBlockHandler);
        WRAP_SCREEN(CreateGC, VGAarbiterCreateGC);
        WRAP_SCREEN(GetImage, VGAarbiterGetImage);
        WRAP_SCREEN(GetSpans, VGAarbiterGetSpans);
        WRAP_SCREEN(SourceValidate, VGAarbiterSourceValidate);
        WRAP_SCREEN(CopyWindow, VGAarbiterCopyWindow);
        WRAP_SCREEN(ClearToBackground, VGAarbiterClearToBackground);
        WRAP_SCREEN(CreatePixmap, VGAarbiterCreatePixmap);
        WRAP_SCREEN(StoreColors, VGAarbiterStoreColors);
        WRAP_SCREEN(DisplayCursor, VGAarbiterDisplayCursor);
        WRAP_SCREEN(RealizeCursor, VGAarbiterRealizeCursor);
        WRAP_SCREEN(UnrealizeCursor, VGAarbiterUnrealizeCursor);
        WRAP_SCREEN(RecolorCursor, VGAarbiterRecolorCursor);
        WRAP_SCREEN(SetCursorPosition, VGAarbiterSetCursorPosition);
        WRAP_PICT(Composite, VGAarbiterComposite);
        WRAP_PICT(Glyphs, VGAarbiterGlyphs);
        WRAP_PICT(CompositeRects, VGAarbiterCompositeRects);
        WRAP_SCREEN_INFO(AdjustFrame, VGAarbiterAdjustFrame);
        WRAP_SCREEN_INFO(SwitchMode, VGAarbiterSwitchMode);
        WRAP_SCREEN_INFO(EnterVT, VGAarbiterEnterVT);
        WRAP_SCREEN_INFO(LeaveVT, VGAarbiterLeaveVT);
        WRAP_SCREEN_INFO(FreeScreen, VGAarbiterFreeScreen);
        WRAP_SPRITE;
    }

    return TRUE;
}

/* Screen funcs */
static Bool
VGAarbiterCloseScreen(ScreenPtr pScreen)
{
    Bool val;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VGAarbiterScreenPtr pScreenPriv =
        (VGAarbiterScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               VGAarbiterScreenKey);
    miPointerScreenPtr PointPriv =
        (miPointerScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                              miPointerScreenKey);
    PictureScreenPtr ps = GetPictureScreenIfSet(pScreen);

    UNWRAP_SCREEN(CreateGC);
    UNWRAP_SCREEN(CloseScreen);
    UNWRAP_SCREEN(GetImage);
    UNWRAP_SCREEN(GetSpans);
    UNWRAP_SCREEN(SourceValidate);
    UNWRAP_SCREEN(CopyWindow);
    UNWRAP_SCREEN(ClearToBackground);
    UNWRAP_SCREEN(SaveScreen);
    UNWRAP_SCREEN(StoreColors);
    UNWRAP_SCREEN(DisplayCursor);
    UNWRAP_SCREEN(RealizeCursor);
    UNWRAP_SCREEN(UnrealizeCursor);
    UNWRAP_SCREEN(RecolorCursor);
    UNWRAP_SCREEN(SetCursorPosition);
    UNWRAP_PICT(Composite);
    UNWRAP_PICT(Glyphs);
    UNWRAP_PICT(CompositeRects);
    UNWRAP_SCREEN_INFO(AdjustFrame);
    UNWRAP_SCREEN_INFO(SwitchMode);
    UNWRAP_SCREEN_INFO(EnterVT);
    UNWRAP_SCREEN_INFO(LeaveVT);
    UNWRAP_SCREEN_INFO(FreeScreen);
    UNWRAP_SPRITE;

    free((void *) pScreenPriv);
    xf86VGAarbiterLock(xf86ScreenToScrn(pScreen));
    val = (*pScreen->CloseScreen) (pScreen);
    xf86VGAarbiterUnlock(xf86ScreenToScrn(pScreen));
    return val;
}

static void
VGAarbiterBlockHandler(ScreenPtr pScreen, void *pTimeout)
{
    SCREEN_PROLOG(BlockHandler);
    VGAGet(pScreen);
    pScreen->BlockHandler(pScreen, pTimeout);
    VGAPut();
    SCREEN_EPILOG(BlockHandler, VGAarbiterBlockHandler);
}

static void
VGAarbiterWakeupHandler(ScreenPtr pScreen, int result)
{
    SCREEN_PROLOG(WakeupHandler);
    VGAGet(pScreen);
    pScreen->WakeupHandler(pScreen, result);
    VGAPut();
    SCREEN_EPILOG(WakeupHandler, VGAarbiterWakeupHandler);
}

static void
VGAarbiterGetImage(DrawablePtr pDrawable,
                   int sx, int sy, int w, int h,
                   unsigned int format, unsigned long planemask, char *pdstLine)
{
    ScreenPtr pScreen = pDrawable->pScreen;

    SCREEN_PROLOG(GetImage);
    VGAGet(pScreen);
    (*pScreen->GetImage) (pDrawable, sx, sy, w, h, format, planemask, pdstLine);
    VGAPut();
    SCREEN_EPILOG(GetImage, VGAarbiterGetImage);
}

static void
VGAarbiterGetSpans(DrawablePtr pDrawable,
                   int wMax,
                   DDXPointPtr ppt, int *pwidth, int nspans, char *pdstStart)
{
    ScreenPtr pScreen = pDrawable->pScreen;

    SCREEN_PROLOG(GetSpans);
    VGAGet(pScreen);
    (*pScreen->GetSpans) (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);
    VGAPut();
    SCREEN_EPILOG(GetSpans, VGAarbiterGetSpans);
}

static void
VGAarbiterSourceValidate(DrawablePtr pDrawable,
                         int x, int y, int width, int height,
                         unsigned int subWindowMode)
{
    ScreenPtr pScreen = pDrawable->pScreen;

    SCREEN_PROLOG(SourceValidate);
    VGAGet(pScreen);
    (*pScreen->SourceValidate) (pDrawable, x, y, width, height,
                                subWindowMode);
    VGAPut();
    SCREEN_EPILOG(SourceValidate, VGAarbiterSourceValidate);
}

static void
VGAarbiterCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    SCREEN_PROLOG(CopyWindow);
    VGAGet(pScreen);
    (*pScreen->CopyWindow) (pWin, ptOldOrg, prgnSrc);
    VGAPut();
    SCREEN_EPILOG(CopyWindow, VGAarbiterCopyWindow);
}

static void
VGAarbiterClearToBackground(WindowPtr pWin,
                            int x, int y, int w, int h, Bool generateExposures)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    SCREEN_PROLOG(ClearToBackground);
    VGAGet(pScreen);
    (*pScreen->ClearToBackground) (pWin, x, y, w, h, generateExposures);
    VGAPut();
    SCREEN_EPILOG(ClearToBackground, VGAarbiterClearToBackground);
}

static PixmapPtr
VGAarbiterCreatePixmap(ScreenPtr pScreen, int w, int h, int depth,
                       unsigned usage_hint)
{
    PixmapPtr pPix;

    SCREEN_PROLOG(CreatePixmap);
    VGAGet(pScreen);
    pPix = (*pScreen->CreatePixmap) (pScreen, w, h, depth, usage_hint);
    VGAPut();
    SCREEN_EPILOG(CreatePixmap, VGAarbiterCreatePixmap);

    return pPix;
}

static Bool
VGAarbiterSaveScreen(ScreenPtr pScreen, Bool unblank)
{
    Bool val;

    SCREEN_PROLOG(SaveScreen);
    VGAGet(pScreen);
    val = (*pScreen->SaveScreen) (pScreen, unblank);
    VGAPut();
    SCREEN_EPILOG(SaveScreen, VGAarbiterSaveScreen);

    return val;
}

static void
VGAarbiterStoreColors(ColormapPtr pmap, int ndef, xColorItem * pdefs)
{
    ScreenPtr pScreen = pmap->pScreen;

    SCREEN_PROLOG(StoreColors);
    VGAGet(pScreen);
    (*pScreen->StoreColors) (pmap, ndef, pdefs);
    VGAPut();
    SCREEN_EPILOG(StoreColors, VGAarbiterStoreColors);
}

static void
VGAarbiterRecolorCursor(DeviceIntPtr pDev,
                        ScreenPtr pScreen, CursorPtr pCurs, Bool displayed)
{
    SCREEN_PROLOG(RecolorCursor);
    VGAGet(pScreen);
    (*pScreen->RecolorCursor) (pDev, pScreen, pCurs, displayed);
    VGAPut();
    SCREEN_EPILOG(RecolorCursor, VGAarbiterRecolorCursor);
}

static Bool
VGAarbiterRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    Bool val;

    SCREEN_PROLOG(RealizeCursor);
    VGAGet(pScreen);
    val = (*pScreen->RealizeCursor) (pDev, pScreen, pCursor);
    VGAPut();
    SCREEN_EPILOG(RealizeCursor, VGAarbiterRealizeCursor);
    return val;
}

static Bool
VGAarbiterUnrealizeCursor(DeviceIntPtr pDev,
                          ScreenPtr pScreen, CursorPtr pCursor)
{
    Bool val;

    SCREEN_PROLOG(UnrealizeCursor);
    VGAGet(pScreen);
    val = (*pScreen->UnrealizeCursor) (pDev, pScreen, pCursor);
    VGAPut();
    SCREEN_EPILOG(UnrealizeCursor, VGAarbiterUnrealizeCursor);
    return val;
}

static Bool
VGAarbiterDisplayCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    Bool val;

    SCREEN_PROLOG(DisplayCursor);
    VGAGet(pScreen);
    val = (*pScreen->DisplayCursor) (pDev, pScreen, pCursor);
    VGAPut();
    SCREEN_EPILOG(DisplayCursor, VGAarbiterDisplayCursor);
    return val;
}

static Bool
VGAarbiterSetCursorPosition(DeviceIntPtr pDev,
                            ScreenPtr pScreen, int x, int y, Bool generateEvent)
{
    Bool val;

    SCREEN_PROLOG(SetCursorPosition);
    VGAGet(pScreen);
    val = (*pScreen->SetCursorPosition) (pDev, pScreen, x, y, generateEvent);
    VGAPut();
    SCREEN_EPILOG(SetCursorPosition, VGAarbiterSetCursorPosition);
    return val;
}

static void
VGAarbiterAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    VGAarbiterScreenPtr pScreenPriv =
        (VGAarbiterScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               VGAarbiterScreenKey);

    VGAGet(pScreen);
    (*pScreenPriv->AdjustFrame) (pScrn, x, y);
    VGAPut();
}

static Bool
VGAarbiterSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    Bool val;
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    VGAarbiterScreenPtr pScreenPriv =
        (VGAarbiterScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               VGAarbiterScreenKey);

    VGAGet(pScreen);
    val = (*pScreenPriv->SwitchMode) (pScrn, mode);
    VGAPut();
    return val;
}

static Bool
VGAarbiterEnterVT(ScrnInfoPtr pScrn)
{
    Bool val;
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    VGAarbiterScreenPtr pScreenPriv =
        (VGAarbiterScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               VGAarbiterScreenKey);

    VGAGet(pScreen);
    pScrn->EnterVT = pScreenPriv->EnterVT;
    val = (*pScrn->EnterVT) (pScrn);
    pScreenPriv->EnterVT = pScrn->EnterVT;
    pScrn->EnterVT = VGAarbiterEnterVT;
    VGAPut();
    return val;
}

static void
VGAarbiterLeaveVT(ScrnInfoPtr pScrn)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    VGAarbiterScreenPtr pScreenPriv =
        (VGAarbiterScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               VGAarbiterScreenKey);

    VGAGet(pScreen);
    pScrn->LeaveVT = pScreenPriv->LeaveVT;
    (*pScreenPriv->LeaveVT) (pScrn);
    pScreenPriv->LeaveVT = pScrn->LeaveVT;
    pScrn->LeaveVT = VGAarbiterLeaveVT;
    VGAPut();
}

static void
VGAarbiterFreeScreen(ScrnInfoPtr pScrn)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    VGAarbiterScreenPtr pScreenPriv =
        (VGAarbiterScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               VGAarbiterScreenKey);

    VGAGet(pScreen);
    (*pScreenPriv->FreeScreen) (pScrn);
    VGAPut();
}

static Bool
VGAarbiterCreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;
    VGAarbiterGCPtr pGCPriv =
        (VGAarbiterGCPtr) dixLookupPrivate(&pGC->devPrivates, VGAarbiterGCKey);
    Bool ret;

    SCREEN_PROLOG(CreateGC);
    ret = (*pScreen->CreateGC) (pGC);
    GC_WRAP(pGC);
    SCREEN_EPILOG(CreateGC, VGAarbiterCreateGC);

    return ret;
}

/* GC funcs */
static void
VGAarbiterValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDraw)
{
    GC_UNWRAP(pGC);
    (*pGC->funcs->ValidateGC) (pGC, changes, pDraw);
    GC_WRAP(pGC);
}

static void
VGAarbiterDestroyGC(GCPtr pGC)
{
    GC_UNWRAP(pGC);
    (*pGC->funcs->DestroyGC) (pGC);
    GC_WRAP(pGC);
}

static void
VGAarbiterChangeGC(GCPtr pGC, unsigned long mask)
{
    GC_UNWRAP(pGC);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    GC_WRAP(pGC);
}

static void
VGAarbiterCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    GC_UNWRAP(pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    GC_WRAP(pGCDst);
}

static void
VGAarbiterChangeClip(GCPtr pGC, int type, void *pvalue, int nrects)
{
    GC_UNWRAP(pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    GC_WRAP(pGC);
}

static void
VGAarbiterCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    GC_UNWRAP(pgcDst);
    (*pgcDst->funcs->CopyClip) (pgcDst, pgcSrc);
    GC_WRAP(pgcDst);
}

static void
VGAarbiterDestroyClip(GCPtr pGC)
{
    GC_UNWRAP(pGC);
    (*pGC->funcs->DestroyClip) (pGC);
    GC_WRAP(pGC);
}

/* GC Ops */
static void
VGAarbiterFillSpans(DrawablePtr pDraw,
                    GC * pGC,
                    int nInit,
                    DDXPointPtr pptInit, int *pwidthInit, int fSorted)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->FillSpans) (pDraw, pGC, nInit, pptInit, pwidthInit, fSorted);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterSetSpans(DrawablePtr pDraw,
                   GCPtr pGC,
                   char *pcharsrc,
                   register DDXPointPtr ppt,
                   int *pwidth, int nspans, int fSorted)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->SetSpans) (pDraw, pGC, pcharsrc, ppt, pwidth, nspans, fSorted);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPutImage(DrawablePtr pDraw,
                   GCPtr pGC,
                   int depth,
                   int x, int y, int w, int h,
                   int leftPad, int format, char *pImage)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PutImage) (pDraw, pGC, depth, x, y, w, h,
                           leftPad, format, pImage);
    VGAPut();
    GC_WRAP(pGC);
}

static RegionPtr
VGAarbiterCopyArea(DrawablePtr pSrc,
                   DrawablePtr pDst,
                   GC * pGC,
                   int srcx, int srcy,
                   int width, int height, int dstx, int dsty)
{
    RegionPtr ret;
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    ret = (*pGC->ops->CopyArea) (pSrc, pDst,
                                 pGC, srcx, srcy, width, height, dstx, dsty);
    VGAPut();
    GC_WRAP(pGC);
    return ret;
}

static RegionPtr
VGAarbiterCopyPlane(DrawablePtr pSrc,
                    DrawablePtr pDst,
                    GCPtr pGC,
                    int srcx, int srcy,
                    int width, int height,
                    int dstx, int dsty, unsigned long bitPlane)
{
    RegionPtr ret;
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    ret = (*pGC->ops->CopyPlane) (pSrc, pDst, pGC, srcx, srcy,
                                  width, height, dstx, dsty, bitPlane);
    VGAPut();
    GC_WRAP(pGC);
    return ret;
}

static void
VGAarbiterPolyPoint(DrawablePtr pDraw,
                    GCPtr pGC, int mode, int npt, xPoint * pptInit)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PolyPoint) (pDraw, pGC, mode, npt, pptInit);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPolylines(DrawablePtr pDraw,
                    GCPtr pGC, int mode, int npt, DDXPointPtr pptInit)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->Polylines) (pDraw, pGC, mode, npt, pptInit);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPolySegment(DrawablePtr pDraw, GCPtr pGC, int nseg, xSegment * pSeg)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PolySegment) (pDraw, pGC, nseg, pSeg);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPolyRectangle(DrawablePtr pDraw,
                        GCPtr pGC, int nRectsInit, xRectangle *pRectsInit)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PolyRectangle) (pDraw, pGC, nRectsInit, pRectsInit);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPolyArc(DrawablePtr pDraw, GCPtr pGC, int narcs, xArc * parcs)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PolyArc) (pDraw, pGC, narcs, parcs);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterFillPolygon(DrawablePtr pDraw,
                      GCPtr pGC,
                      int shape, int mode, int count, DDXPointPtr ptsIn)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->FillPolygon) (pDraw, pGC, shape, mode, count, ptsIn);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPolyFillRect(DrawablePtr pDraw,
                       GCPtr pGC, int nrectFill, xRectangle *prectInit)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PolyFillRect) (pDraw, pGC, nrectFill, prectInit);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPolyFillArc(DrawablePtr pDraw, GCPtr pGC, int narcs, xArc * parcs)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PolyFillArc) (pDraw, pGC, narcs, parcs);
    VGAPut();
    GC_WRAP(pGC);
}

static int
VGAarbiterPolyText8(DrawablePtr pDraw,
                    GCPtr pGC, int x, int y, int count, char *chars)
{
    int ret;
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    ret = (*pGC->ops->PolyText8) (pDraw, pGC, x, y, count, chars);
    VGAPut();
    GC_WRAP(pGC);
    return ret;
}

static int
VGAarbiterPolyText16(DrawablePtr pDraw,
                     GCPtr pGC, int x, int y, int count, unsigned short *chars)
{
    int ret;
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    ret = (*pGC->ops->PolyText16) (pDraw, pGC, x, y, count, chars);
    VGAPut();
    GC_WRAP(pGC);
    return ret;
}

static void
VGAarbiterImageText8(DrawablePtr pDraw,
                     GCPtr pGC, int x, int y, int count, char *chars)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->ImageText8) (pDraw, pGC, x, y, count, chars);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterImageText16(DrawablePtr pDraw,
                      GCPtr pGC, int x, int y, int count, unsigned short *chars)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->ImageText16) (pDraw, pGC, x, y, count, chars);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterImageGlyphBlt(DrawablePtr pDraw,
                        GCPtr pGC,
                        int xInit, int yInit,
                        unsigned int nglyph,
                        CharInfoPtr * ppci, void *pglyphBase)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->ImageGlyphBlt) (pDraw, pGC, xInit, yInit,
                                nglyph, ppci, pglyphBase);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPolyGlyphBlt(DrawablePtr pDraw,
                       GCPtr pGC,
                       int xInit, int yInit,
                       unsigned int nglyph,
                       CharInfoPtr * ppci, void *pglyphBase)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PolyGlyphBlt) (pDraw, pGC, xInit, yInit,
                               nglyph, ppci, pglyphBase);
    VGAPut();
    GC_WRAP(pGC);
}

static void
VGAarbiterPushPixels(GCPtr pGC,
                     PixmapPtr pBitMap,
                     DrawablePtr pDraw, int dx, int dy, int xOrg, int yOrg)
{
    ScreenPtr pScreen = pGC->pScreen;

    GC_UNWRAP(pGC);
    VGAGet(pScreen);
    (*pGC->ops->PushPixels) (pGC, pBitMap, pDraw, dx, dy, xOrg, yOrg);
    VGAPut();
    GC_WRAP(pGC);
}

/* miSpriteFuncs */
static Bool
VGAarbiterSpriteRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                              CursorPtr pCur)
{
    Bool val;

    SPRITE_PROLOG;
    VGAGet(pScreen);
    val = PointPriv->spriteFuncs->RealizeCursor(pDev, pScreen, pCur);
    VGAPut();
    SPRITE_EPILOG;
    return val;
}

static Bool
VGAarbiterSpriteUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                CursorPtr pCur)
{
    Bool val;

    SPRITE_PROLOG;
    VGAGet(pScreen);
    val = PointPriv->spriteFuncs->UnrealizeCursor(pDev, pScreen, pCur);
    VGAPut();
    SPRITE_EPILOG;
    return val;
}

static void
VGAarbiterSpriteSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCur,
                          int x, int y)
{
    SPRITE_PROLOG;
    VGAGet(pScreen);
    PointPriv->spriteFuncs->SetCursor(pDev, pScreen, pCur, x, y);
    VGAPut();
    SPRITE_EPILOG;
}

static void
VGAarbiterSpriteMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    SPRITE_PROLOG;
    VGAGet(pScreen);
    PointPriv->spriteFuncs->MoveCursor(pDev, pScreen, x, y);
    VGAPut();
    SPRITE_EPILOG;
}

static Bool
VGAarbiterDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    Bool val;

    SPRITE_PROLOG;
    VGAGet(pScreen);
    val = PointPriv->spriteFuncs->DeviceCursorInitialize(pDev, pScreen);
    VGAPut();
    SPRITE_EPILOG;
    return val;
}

static void
VGAarbiterDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    SPRITE_PROLOG;
    VGAGet(pScreen);
    PointPriv->spriteFuncs->DeviceCursorCleanup(pDev, pScreen);
    VGAPut();
    SPRITE_EPILOG;
}

static void
VGAarbiterComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask,
                    PicturePtr pDst, INT16 xSrc, INT16 ySrc, INT16 xMask,
                    INT16 yMask, INT16 xDst, INT16 yDst, CARD16 width,
                    CARD16 height)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);

    PICTURE_PROLOGUE(Composite);

    VGAGet(pScreen);
    (*ps->Composite) (op, pSrc, pMask, pDst, xSrc, ySrc, xMask, yMask, xDst,
                      yDst, width, height);
    VGAPut();
    PICTURE_EPILOGUE(Composite, VGAarbiterComposite);
}

static void
VGAarbiterGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                 PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc, int nlist,
                 GlyphListPtr list, GlyphPtr * glyphs)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);

    PICTURE_PROLOGUE(Glyphs);

    VGAGet(pScreen);
    (*ps->Glyphs) (op, pSrc, pDst, maskFormat, xSrc, ySrc, nlist, list, glyphs);
    VGAPut();
    PICTURE_EPILOGUE(Glyphs, VGAarbiterGlyphs);
}

static void
VGAarbiterCompositeRects(CARD8 op, PicturePtr pDst, xRenderColor * color,
                         int nRect, xRectangle *rects)
{
    ScreenPtr pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr ps = GetPictureScreen(pScreen);

    PICTURE_PROLOGUE(CompositeRects);

    VGAGet(pScreen);
    (*ps->CompositeRects) (op, pDst, color, nRect, rects);
    VGAPut();
    PICTURE_EPILOGUE(CompositeRects, VGAarbiterCompositeRects);
}
