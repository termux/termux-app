/*
 * misprite.c
 *
 * machine independent software sprite routines
 */

/*

Copyright 1989, 1998  The Open Group

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
*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include   <X11/X.h>
#include   <X11/Xproto.h>
#include   "misc.h"
#include   "pixmapstr.h"
#include   "input.h"
#include   "mi.h"
#include   "cursorstr.h"
#include   <X11/fonts/font.h>
#include   "scrnintstr.h"
#include   "colormapst.h"
#include   "windowstr.h"
#include   "gcstruct.h"
#include   "mipointer.h"
#include   "misprite.h"
#include   "dixfontstr.h"
#include   <X11/fonts/fontstruct.h>
#include   "inputstr.h"
#include   "damage.h"

typedef struct {
    CursorPtr pCursor;
    int x;                      /* cursor hotspot */
    int y;
    BoxRec saved;               /* saved area from the screen */
    Bool isUp;                  /* cursor in frame buffer */
    Bool shouldBeUp;            /* cursor should be displayed */
    Bool checkPixels;           /* check colormap collision */
    ScreenPtr pScreen;
} miCursorInfoRec, *miCursorInfoPtr;

/*
 * per screen information
 */

typedef struct {
    /* screen procedures */
    CloseScreenProcPtr CloseScreen;
    SourceValidateProcPtr SourceValidate;

    /* window procedures */
    CopyWindowProcPtr CopyWindow;

    /* colormap procedures */
    InstallColormapProcPtr InstallColormap;
    StoreColorsProcPtr StoreColors;

    /* os layer procedures */
    ScreenBlockHandlerProcPtr BlockHandler;

    xColorItem colors[2];
    ColormapPtr pInstalledMap;
    ColormapPtr pColormap;
    VisualPtr pVisual;
    DamagePtr pDamage;          /* damage tracking structure */
    Bool damageRegistered;
    int numberOfCursors;
} miSpriteScreenRec, *miSpriteScreenPtr;

#define SOURCE_COLOR	0
#define MASK_COLOR	1

/*
 * Overlap BoxPtr and Box elements
 */
#define BOX_OVERLAP(pCbox,X1,Y1,X2,Y2) \
 	(((pCbox)->x1 <= (X2)) && ((X1) <= (pCbox)->x2) && \
	 ((pCbox)->y1 <= (Y2)) && ((Y1) <= (pCbox)->y2))

/*
 * Overlap BoxPtr, origins, and rectangle
 */
#define ORG_OVERLAP(pCbox,xorg,yorg,x,y,w,h) \
    BOX_OVERLAP((pCbox),(x)+(xorg),(y)+(yorg),(x)+(xorg)+(w),(y)+(yorg)+(h))

/*
 * Overlap BoxPtr, origins and RectPtr
 */
#define ORGRECT_OVERLAP(pCbox,xorg,yorg,pRect) \
    ORG_OVERLAP((pCbox),(xorg),(yorg),(pRect)->x,(pRect)->y, \
		(int)((pRect)->width), (int)((pRect)->height))
/*
 * Overlap BoxPtr and horizontal span
 */
#define SPN_OVERLAP(pCbox,y,x,w) BOX_OVERLAP((pCbox),(x),(y),(x)+(w),(y))

#define LINE_SORT(x1,y1,x2,y2) \
{ int _t; \
  if (x1 > x2) { _t = x1; x1 = x2; x2 = _t; } \
  if (y1 > y2) { _t = y1; y1 = y2; y2 = _t; } }

#define LINE_OVERLAP(pCbox,x1,y1,x2,y2,lw2) \
    BOX_OVERLAP((pCbox), (x1)-(lw2), (y1)-(lw2), (x2)+(lw2), (y2)+(lw2))

#define SPRITE_DEBUG_ENABLE 0
#if SPRITE_DEBUG_ENABLE
#define SPRITE_DEBUG(x)	ErrorF x
#else
#define SPRITE_DEBUG(x)
#endif

static DevPrivateKeyRec miSpriteScreenKeyRec;
static DevPrivateKeyRec miSpriteDevPrivatesKeyRec;

static miSpriteScreenPtr
GetSpriteScreen(ScreenPtr pScreen)
{
    return dixLookupPrivate(&pScreen->devPrivates, &miSpriteScreenKeyRec);
}

static miCursorInfoPtr
GetSprite(DeviceIntPtr dev)
{
    if (IsFloating(dev))
       return dixLookupPrivate(&dev->devPrivates, &miSpriteDevPrivatesKeyRec);

    return dixLookupPrivate(&(GetMaster(dev, MASTER_POINTER))->devPrivates,
                            &miSpriteDevPrivatesKeyRec);
}

static void
miSpriteDisableDamage(ScreenPtr pScreen, miSpriteScreenPtr pScreenPriv)
{
    if (pScreenPriv->damageRegistered) {
        DamageUnregister(pScreenPriv->pDamage);
        pScreenPriv->damageRegistered = 0;
    }
}

static void
miSpriteEnableDamage(ScreenPtr pScreen, miSpriteScreenPtr pScreenPriv)
{
    if (!pScreenPriv->damageRegistered) {
        pScreenPriv->damageRegistered = 1;
        DamageRegister(&(pScreen->GetScreenPixmap(pScreen)->drawable),
                       pScreenPriv->pDamage);
    }
}

static void
miSpriteIsUp(miCursorInfoPtr pDevCursor)
{
    pDevCursor->isUp = TRUE;
}

static void
miSpriteIsDown(miCursorInfoPtr pDevCursor)
{
    pDevCursor->isUp = FALSE;
}

/*
 * screen wrappers
 */

static Bool miSpriteCloseScreen(ScreenPtr pScreen);
static void miSpriteSourceValidate(DrawablePtr pDrawable, int x, int y,
                                   int width, int height,
                                   unsigned int subWindowMode);
static void miSpriteCopyWindow(WindowPtr pWindow,
                               DDXPointRec ptOldOrg, RegionPtr prgnSrc);
static void miSpriteBlockHandler(ScreenPtr pScreen, void *timeout);
static void miSpriteInstallColormap(ColormapPtr pMap);
static void miSpriteStoreColors(ColormapPtr pMap, int ndef, xColorItem * pdef);

static void miSpriteComputeSaved(DeviceIntPtr pDev, ScreenPtr pScreen);

static Bool miSpriteDeviceCursorInitialize(DeviceIntPtr pDev,
                                           ScreenPtr pScreen);
static void miSpriteDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen);

#define SCREEN_PROLOGUE(pPriv, pScreen, field) ((pScreen)->field = \
   (pPriv)->field)
#define SCREEN_EPILOGUE(pPriv, pScreen, field)\
    ((pPriv)->field = (pScreen)->field, (pScreen)->field = miSprite##field)

/*
 * pointer-sprite method table
 */

static Bool miSpriteRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                  CursorPtr pCursor);
static Bool miSpriteUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                    CursorPtr pCursor);
static void miSpriteSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                              CursorPtr pCursor, int x, int y);
static void miSpriteMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                               int x, int y);

miPointerSpriteFuncRec miSpritePointerFuncs = {
    miSpriteRealizeCursor,
    miSpriteUnrealizeCursor,
    miSpriteSetCursor,
    miSpriteMoveCursor,
    miSpriteDeviceCursorInitialize,
    miSpriteDeviceCursorCleanup,
};

/*
 * other misc functions
 */

static void miSpriteRemoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen);
static void miSpriteSaveUnderCursor(DeviceIntPtr pDev, ScreenPtr pScreen);
static void miSpriteRestoreCursor(DeviceIntPtr pDev, ScreenPtr pScreen);

static void
miSpriteRegisterBlockHandler(ScreenPtr pScreen, miSpriteScreenPtr pScreenPriv)
{
    if (!pScreenPriv->BlockHandler) {
        pScreenPriv->BlockHandler = pScreen->BlockHandler;
        pScreen->BlockHandler = miSpriteBlockHandler;
    }
}

static void
miSpriteReportDamage(DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    ScreenPtr pScreen = closure;
    miCursorInfoPtr pCursorInfo;
    DeviceIntPtr pDev;

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
        if (DevHasCursor(pDev)) {
            pCursorInfo = GetSprite(pDev);

            if (pCursorInfo->isUp &&
                pCursorInfo->pScreen == pScreen &&
                RegionContainsRect(pRegion, &pCursorInfo->saved) != rgnOUT) {
                SPRITE_DEBUG(("Damage remove\n"));
                miSpriteRemoveCursor(pDev, pScreen);
            }
        }
    }
}

/*
 * miSpriteInitialize -- called from device-dependent screen
 * initialization proc after all of the function pointers have
 * been stored in the screen structure.
 */

Bool
miSpriteInitialize(ScreenPtr pScreen, miPointerScreenFuncPtr screenFuncs)
{
    miSpriteScreenPtr pScreenPriv;
    VisualPtr pVisual;

    if (!DamageSetup(pScreen))
        return FALSE;

    if (!dixRegisterPrivateKey(&miSpriteScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (!dixRegisterPrivateKey
        (&miSpriteDevPrivatesKeyRec, PRIVATE_DEVICE, sizeof(miCursorInfoRec)))
        return FALSE;

    pScreenPriv = malloc(sizeof(miSpriteScreenRec));
    if (!pScreenPriv)
        return FALSE;

    pScreenPriv->pDamage = DamageCreate(miSpriteReportDamage,
                                        NULL,
                                        DamageReportRawRegion,
                                        TRUE, pScreen, pScreen);

    if (!miPointerInitialize(pScreen, &miSpritePointerFuncs, screenFuncs, TRUE)) {
        free(pScreenPriv);
        return FALSE;
    }
    for (pVisual = pScreen->visuals;
         pVisual->vid != pScreen->rootVisual; pVisual++);
    pScreenPriv->pVisual = pVisual;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreenPriv->SourceValidate = pScreen->SourceValidate;

    pScreenPriv->CopyWindow = pScreen->CopyWindow;

    pScreenPriv->InstallColormap = pScreen->InstallColormap;
    pScreenPriv->StoreColors = pScreen->StoreColors;

    pScreenPriv->BlockHandler = NULL;

    pScreenPriv->pInstalledMap = NULL;
    pScreenPriv->pColormap = NULL;
    pScreenPriv->colors[SOURCE_COLOR].red = 0;
    pScreenPriv->colors[SOURCE_COLOR].green = 0;
    pScreenPriv->colors[SOURCE_COLOR].blue = 0;
    pScreenPriv->colors[MASK_COLOR].red = 0;
    pScreenPriv->colors[MASK_COLOR].green = 0;
    pScreenPriv->colors[MASK_COLOR].blue = 0;
    pScreenPriv->damageRegistered = 0;
    pScreenPriv->numberOfCursors = 0;

    dixSetPrivate(&pScreen->devPrivates, &miSpriteScreenKeyRec, pScreenPriv);

    pScreen->CloseScreen = miSpriteCloseScreen;
    pScreen->SourceValidate = miSpriteSourceValidate;

    pScreen->CopyWindow = miSpriteCopyWindow;
    pScreen->InstallColormap = miSpriteInstallColormap;
    pScreen->StoreColors = miSpriteStoreColors;

    return TRUE;
}

/*
 * Screen wrappers
 */

/*
 * CloseScreen wrapper -- unwrap everything, free the private data
 * and call the wrapped function
 */

static Bool
miSpriteCloseScreen(ScreenPtr pScreen)
{
    miSpriteScreenPtr pScreenPriv = GetSpriteScreen(pScreen);

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->SourceValidate = pScreenPriv->SourceValidate;
    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    pScreen->StoreColors = pScreenPriv->StoreColors;

    DamageDestroy(pScreenPriv->pDamage);

    free(pScreenPriv);

    return (*pScreen->CloseScreen) (pScreen);
}

static void
miSpriteSourceValidate(DrawablePtr pDrawable, int x, int y, int width,
                       int height, unsigned int subWindowMode)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    DeviceIntPtr pDev;
    miCursorInfoPtr pCursorInfo;
    miSpriteScreenPtr pPriv = GetSpriteScreen(pScreen);

    SCREEN_PROLOGUE(pPriv, pScreen, SourceValidate);

    if (pDrawable->type == DRAWABLE_WINDOW) {
        for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
            if (DevHasCursor(pDev)) {
                pCursorInfo = GetSprite(pDev);
                if (pCursorInfo->isUp && pCursorInfo->pScreen == pScreen &&
                    ORG_OVERLAP(&pCursorInfo->saved, pDrawable->x, pDrawable->y,
                                x, y, width, height)) {
                    SPRITE_DEBUG(("SourceValidate remove\n"));
                    miSpriteRemoveCursor(pDev, pScreen);
                }
            }
        }
    }

    (*pScreen->SourceValidate) (pDrawable, x, y, width, height,
                                subWindowMode);

    SCREEN_EPILOGUE(pPriv, pScreen, SourceValidate);
}

static void
miSpriteCopyWindow(WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWindow->drawable.pScreen;
    DeviceIntPtr pDev;
    miCursorInfoPtr pCursorInfo;
    miSpriteScreenPtr pPriv = GetSpriteScreen(pScreen);

    SCREEN_PROLOGUE(pPriv, pScreen, CopyWindow);

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
        if (DevHasCursor(pDev)) {
            pCursorInfo = GetSprite(pDev);
            /*
             * Damage will take care of destination check
             */
            if (pCursorInfo->isUp && pCursorInfo->pScreen == pScreen &&
                RegionContainsRect(prgnSrc, &pCursorInfo->saved) != rgnOUT) {
                SPRITE_DEBUG(("CopyWindow remove\n"));
                miSpriteRemoveCursor(pDev, pScreen);
            }
        }
    }

    (*pScreen->CopyWindow) (pWindow, ptOldOrg, prgnSrc);
    SCREEN_EPILOGUE(pPriv, pScreen, CopyWindow);
}

static void
miSpriteBlockHandler(ScreenPtr pScreen, void *timeout)
{
    miSpriteScreenPtr pPriv = GetSpriteScreen(pScreen);
    DeviceIntPtr pDev;
    miCursorInfoPtr pCursorInfo;
    Bool WorkToDo = FALSE;

    SCREEN_PROLOGUE(pPriv, pScreen, BlockHandler);

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
        if (DevHasCursor(pDev)) {
            pCursorInfo = GetSprite(pDev);
            if (pCursorInfo && !pCursorInfo->isUp
                && pCursorInfo->pScreen == pScreen && pCursorInfo->shouldBeUp) {
                SPRITE_DEBUG(("BlockHandler save"));
                miSpriteSaveUnderCursor(pDev, pScreen);
            }
        }
    }
    for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
        if (DevHasCursor(pDev)) {
            pCursorInfo = GetSprite(pDev);
            if (pCursorInfo && !pCursorInfo->isUp &&
                pCursorInfo->pScreen == pScreen && pCursorInfo->shouldBeUp) {
                SPRITE_DEBUG(("BlockHandler restore\n"));
                miSpriteRestoreCursor(pDev, pScreen);
                if (!pCursorInfo->isUp)
                    WorkToDo = TRUE;
            }
        }
    }

    (*pScreen->BlockHandler) (pScreen, timeout);

    if (WorkToDo)
        SCREEN_EPILOGUE(pPriv, pScreen, BlockHandler);
    else
        pPriv->BlockHandler = NULL;
}

static void
miSpriteInstallColormap(ColormapPtr pMap)
{
    ScreenPtr pScreen = pMap->pScreen;
    miSpriteScreenPtr pPriv = GetSpriteScreen(pScreen);

    SCREEN_PROLOGUE(pPriv, pScreen, InstallColormap);

    (*pScreen->InstallColormap) (pMap);

    SCREEN_EPILOGUE(pPriv, pScreen, InstallColormap);

    /* InstallColormap can be called before devices are initialized. */
    pPriv->pInstalledMap = pMap;
    if (pPriv->pColormap != pMap) {
        DeviceIntPtr pDev;
        miCursorInfoPtr pCursorInfo;

        for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
            if (DevHasCursor(pDev)) {
                pCursorInfo = GetSprite(pDev);
                pCursorInfo->checkPixels = TRUE;
                if (pCursorInfo->isUp && pCursorInfo->pScreen == pScreen)
                    miSpriteRemoveCursor(pDev, pScreen);
            }
        }

    }
}

static void
miSpriteStoreColors(ColormapPtr pMap, int ndef, xColorItem * pdef)
{
    ScreenPtr pScreen = pMap->pScreen;
    miSpriteScreenPtr pPriv = GetSpriteScreen(pScreen);
    int i;
    int updated;
    VisualPtr pVisual;
    DeviceIntPtr pDev;
    miCursorInfoPtr pCursorInfo;

    SCREEN_PROLOGUE(pPriv, pScreen, StoreColors);

    (*pScreen->StoreColors) (pMap, ndef, pdef);

    SCREEN_EPILOGUE(pPriv, pScreen, StoreColors);

    if (pPriv->pColormap == pMap) {
        updated = 0;
        pVisual = pMap->pVisual;
        if (pVisual->class == DirectColor) {
            /* Direct color - match on any of the subfields */

#define MaskMatch(a,b,mask) (((a) & (pVisual->mask)) == ((b) & (pVisual->mask)))

#define UpdateDAC(dev, plane,dac,mask) {\
    if (MaskMatch (dev->colors[plane].pixel,pdef[i].pixel,mask)) {\
	dev->colors[plane].dac = pdef[i].dac; \
	updated = 1; \
    } \
}

#define CheckDirect(dev, plane) \
	    UpdateDAC(dev, plane,red,redMask) \
	    UpdateDAC(dev, plane,green,greenMask) \
	    UpdateDAC(dev, plane,blue,blueMask)

            for (i = 0; i < ndef; i++) {
                CheckDirect(pPriv, SOURCE_COLOR)
                    CheckDirect(pPriv, MASK_COLOR)
            }
        }
        else {
            /* PseudoColor/GrayScale - match on exact pixel */
            for (i = 0; i < ndef; i++) {
                if (pdef[i].pixel == pPriv->colors[SOURCE_COLOR].pixel) {
                    pPriv->colors[SOURCE_COLOR] = pdef[i];
                    if (++updated == 2)
                        break;
                }
                if (pdef[i].pixel == pPriv->colors[MASK_COLOR].pixel) {
                    pPriv->colors[MASK_COLOR] = pdef[i];
                    if (++updated == 2)
                        break;
                }
            }
        }
        if (updated) {
            for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
                if (DevHasCursor(pDev)) {
                    pCursorInfo = GetSprite(pDev);
                    pCursorInfo->checkPixels = TRUE;
                    if (pCursorInfo->isUp && pCursorInfo->pScreen == pScreen)
                        miSpriteRemoveCursor(pDev, pScreen);
                }
            }
        }
    }
}

static void
miSpriteFindColors(miCursorInfoPtr pDevCursor, ScreenPtr pScreen)
{
    miSpriteScreenPtr pScreenPriv = GetSpriteScreen(pScreen);
    CursorPtr pCursor;
    xColorItem *sourceColor, *maskColor;

    pCursor = pDevCursor->pCursor;
    sourceColor = &pScreenPriv->colors[SOURCE_COLOR];
    maskColor = &pScreenPriv->colors[MASK_COLOR];
    if (pScreenPriv->pColormap != pScreenPriv->pInstalledMap ||
        !(pCursor->foreRed == sourceColor->red &&
          pCursor->foreGreen == sourceColor->green &&
          pCursor->foreBlue == sourceColor->blue &&
          pCursor->backRed == maskColor->red &&
          pCursor->backGreen == maskColor->green &&
          pCursor->backBlue == maskColor->blue)) {
        pScreenPriv->pColormap = pScreenPriv->pInstalledMap;
        sourceColor->red = pCursor->foreRed;
        sourceColor->green = pCursor->foreGreen;
        sourceColor->blue = pCursor->foreBlue;
        FakeAllocColor(pScreenPriv->pColormap, sourceColor);
        maskColor->red = pCursor->backRed;
        maskColor->green = pCursor->backGreen;
        maskColor->blue = pCursor->backBlue;
        FakeAllocColor(pScreenPriv->pColormap, maskColor);
        /* "free" the pixels right away, don't let this confuse you */
        FakeFreeColor(pScreenPriv->pColormap, sourceColor->pixel);
        FakeFreeColor(pScreenPriv->pColormap, maskColor->pixel);
    }

    pDevCursor->checkPixels = FALSE;

}

/*
 * miPointer interface routines
 */

#define SPRITE_PAD  8

static Bool
miSpriteRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    miCursorInfoPtr pCursorInfo;

    if (IsFloating(pDev))
        return FALSE;

    pCursorInfo = GetSprite(pDev);

    if (pCursor == pCursorInfo->pCursor)
        pCursorInfo->checkPixels = TRUE;

    return miDCRealizeCursor(pScreen, pCursor);
}

static Bool
miSpriteUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    return miDCUnrealizeCursor(pScreen, pCursor);
}

static void
miSpriteSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                  CursorPtr pCursor, int x, int y)
{
    miCursorInfoPtr pPointer;
    miSpriteScreenPtr pScreenPriv;

    if (IsFloating(pDev))
        return;

    pPointer = GetSprite(pDev);
    pScreenPriv = GetSpriteScreen(pScreen);

    if (!pCursor) {
        if (pPointer->shouldBeUp)
            --pScreenPriv->numberOfCursors;
        pPointer->shouldBeUp = FALSE;
        if (pPointer->isUp)
            miSpriteRemoveCursor(pDev, pScreen);
        if (pScreenPriv->numberOfCursors == 0)
            miSpriteDisableDamage(pScreen, pScreenPriv);
        pPointer->pCursor = 0;
        return;
    }
    if (!pPointer->shouldBeUp)
        pScreenPriv->numberOfCursors++;
    pPointer->shouldBeUp = TRUE;
    if (!pPointer->isUp)
        miSpriteRegisterBlockHandler(pScreen, pScreenPriv);
    if (pPointer->x == x &&
        pPointer->y == y &&
        pPointer->pCursor == pCursor && !pPointer->checkPixels) {
        return;
    }
    pPointer->x = x;
    pPointer->y = y;
    if (pPointer->checkPixels || pPointer->pCursor != pCursor) {
        pPointer->pCursor = pCursor;
        miSpriteFindColors(pPointer, pScreen);
    }
    if (pPointer->isUp) {
        /* TODO: reimplement flicker-free MoveCursor */
        SPRITE_DEBUG(("SetCursor remove %d\n", pDev->id));
        miSpriteRemoveCursor(pDev, pScreen);
    }

    if (!pPointer->isUp && pPointer->pCursor) {
        SPRITE_DEBUG(("SetCursor restore %d\n", pDev->id));
        miSpriteSaveUnderCursor(pDev, pScreen);
        miSpriteRestoreCursor(pDev, pScreen);
    }

}

static void
miSpriteMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    CursorPtr pCursor;

    if (IsFloating(pDev))
        return;

    pCursor = GetSprite(pDev)->pCursor;

    miSpriteSetCursor(pDev, pScreen, pCursor, x, y);
}

static Bool
miSpriteDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    int ret = miDCDeviceInitialize(pDev, pScreen);

    if (ret) {
        miCursorInfoPtr pCursorInfo;

        pCursorInfo =
            dixLookupPrivate(&pDev->devPrivates, &miSpriteDevPrivatesKeyRec);
        pCursorInfo->pCursor = NULL;
        pCursorInfo->x = 0;
        pCursorInfo->y = 0;
        pCursorInfo->isUp = FALSE;
        pCursorInfo->shouldBeUp = FALSE;
        pCursorInfo->checkPixels = TRUE;
        pCursorInfo->pScreen = FALSE;
    }

    return ret;
}

static void
miSpriteDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    miCursorInfoPtr pCursorInfo =
        dixLookupPrivate(&pDev->devPrivates, &miSpriteDevPrivatesKeyRec);

    if (DevHasCursor(pDev))
        miDCDeviceCleanup(pDev, pScreen);

    memset(pCursorInfo, 0, sizeof(miCursorInfoRec));
}

/*
 * undraw/draw cursor
 */

static void
miSpriteRemoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    miSpriteScreenPtr pScreenPriv;
    miCursorInfoPtr pCursorInfo;

    if (IsFloating(pDev))
        return;

    DamageDrawInternal(pScreen, TRUE);
    pScreenPriv = GetSpriteScreen(pScreen);
    pCursorInfo = GetSprite(pDev);

    miSpriteIsDown(pCursorInfo);
    miSpriteRegisterBlockHandler(pScreen, pScreenPriv);
    miSpriteDisableDamage(pScreen, pScreenPriv);
    if (!miDCRestoreUnderCursor(pDev,
                                pScreen,
                                pCursorInfo->saved.x1,
                                pCursorInfo->saved.y1,
                                pCursorInfo->saved.x2 -
                                pCursorInfo->saved.x1,
                                pCursorInfo->saved.y2 -
                                pCursorInfo->saved.y1)) {
        miSpriteIsUp(pCursorInfo);
    }
    miSpriteEnableDamage(pScreen, pScreenPriv);
    DamageDrawInternal(pScreen, FALSE);
}

/*
 * Called from the block handler, saves area under cursor
 * before waiting for something to do.
 */

static void
miSpriteSaveUnderCursor(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    miSpriteScreenPtr pScreenPriv;
    miCursorInfoPtr pCursorInfo;

    if (IsFloating(pDev))
        return;

    DamageDrawInternal(pScreen, TRUE);
    pScreenPriv = GetSpriteScreen(pScreen);
    pCursorInfo = GetSprite(pDev);

    miSpriteComputeSaved(pDev, pScreen);

    miSpriteDisableDamage(pScreen, pScreenPriv);

    miDCSaveUnderCursor(pDev,
                        pScreen,
                        pCursorInfo->saved.x1,
                        pCursorInfo->saved.y1,
                        pCursorInfo->saved.x2 -
                        pCursorInfo->saved.x1,
                        pCursorInfo->saved.y2 - pCursorInfo->saved.y1);
    SPRITE_DEBUG(("SaveUnderCursor %d\n", pDev->id));
    miSpriteEnableDamage(pScreen, pScreenPriv);
    DamageDrawInternal(pScreen, FALSE);
}

/*
 * Called from the block handler, restores the cursor
 * before waiting for something to do.
 */

static void
miSpriteRestoreCursor(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    miSpriteScreenPtr pScreenPriv;
    int x, y;
    CursorPtr pCursor;
    miCursorInfoPtr pCursorInfo;

    if (IsFloating(pDev))
        return;

    DamageDrawInternal(pScreen, TRUE);
    pScreenPriv = GetSpriteScreen(pScreen);
    pCursorInfo = GetSprite(pDev);

    miSpriteComputeSaved(pDev, pScreen);
    pCursor = pCursorInfo->pCursor;

    x = pCursorInfo->x - (int) pCursor->bits->xhot;
    y = pCursorInfo->y - (int) pCursor->bits->yhot;
    miSpriteDisableDamage(pScreen, pScreenPriv);
    SPRITE_DEBUG(("RestoreCursor %d\n", pDev->id));
    if (pCursorInfo->checkPixels)
        miSpriteFindColors(pCursorInfo, pScreen);
    if (miDCPutUpCursor(pDev, pScreen,
                        pCursor, x, y,
                        pScreenPriv->colors[SOURCE_COLOR].pixel,
                        pScreenPriv->colors[MASK_COLOR].pixel)) {
        miSpriteIsUp(pCursorInfo);
        pCursorInfo->pScreen = pScreen;
    }
    miSpriteEnableDamage(pScreen, pScreenPriv);
    DamageDrawInternal(pScreen, FALSE);
}

/*
 * compute the desired area of the screen to save
 */

static void
miSpriteComputeSaved(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    int x, y, w, h;
    int wpad, hpad;
    CursorPtr pCursor;
    miCursorInfoPtr pCursorInfo;

    if (IsFloating(pDev))
        return;

    pCursorInfo = GetSprite(pDev);

    pCursor = pCursorInfo->pCursor;
    x = pCursorInfo->x - (int) pCursor->bits->xhot;
    y = pCursorInfo->y - (int) pCursor->bits->yhot;
    w = pCursor->bits->width;
    h = pCursor->bits->height;
    wpad = SPRITE_PAD;
    hpad = SPRITE_PAD;
    pCursorInfo->saved.x1 = x - wpad;
    pCursorInfo->saved.y1 = y - hpad;
    pCursorInfo->saved.x2 = pCursorInfo->saved.x1 + w + wpad * 2;
    pCursorInfo->saved.y2 = pCursorInfo->saved.y1 + h + hpad * 2;
}
