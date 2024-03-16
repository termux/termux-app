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

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "dix.h"
#include "mi.h"
#include "micmap.h"
#include "colormapst.h"
#include "resource.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "XNGC.h"
#include "GCOps.h"
#include "Drawable.h"
#include "XNFont.h"
#include "Color.h"
#include "XNCursor.h"
#include "Visual.h"
#include "Events.h"
#include "Init.h"
#include "mipointer.h"
#include "Args.h"
#include "mipointrst.h"

Window xnestDefaultWindows[MAXSCREENS];
Window xnestScreenSaverWindows[MAXSCREENS];
DevPrivateKeyRec xnestCursorScreenKeyRec;

ScreenPtr
xnestScreen(Window window)
{
    int i;

    for (i = 0; i < xnestNumScreens; i++)
        if (xnestDefaultWindows[i] == window)
            return screenInfo.screens[i];

    return NULL;
}

static int
offset(unsigned long mask)
{
    int count;

    for (count = 0; !(mask & 1) && count < 32; count++)
        mask >>= 1;

    return count;
}

static Bool
xnestSaveScreen(ScreenPtr pScreen, int what)
{
    if (xnestSoftwareScreenSaver)
        return False;
    else {
        switch (what) {
        case SCREEN_SAVER_ON:
            XMapRaised(xnestDisplay, xnestScreenSaverWindows[pScreen->myNum]);
            xnestSetScreenSaverColormapWindow(pScreen);
            break;

        case SCREEN_SAVER_OFF:
            XUnmapWindow(xnestDisplay, xnestScreenSaverWindows[pScreen->myNum]);
            xnestSetInstalledColormapWindows(pScreen);
            break;

        case SCREEN_SAVER_FORCER:
            lastEventTime = GetTimeInMillis();
            XUnmapWindow(xnestDisplay, xnestScreenSaverWindows[pScreen->myNum]);
            xnestSetInstalledColormapWindows(pScreen);
            break;

        case SCREEN_SAVER_CYCLE:
            XUnmapWindow(xnestDisplay, xnestScreenSaverWindows[pScreen->myNum]);
            xnestSetInstalledColormapWindows(pScreen);
            break;
        }
        return True;
    }
}

static Bool
xnestCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
xnestCrossScreen(ScreenPtr pScreen, Bool entering)
{
}

static miPointerScreenFuncRec xnestPointerCursorFuncs = {
    xnestCursorOffScreen,
    xnestCrossScreen,
    miPointerWarpCursor
};

static miPointerSpriteFuncRec xnestPointerSpriteFuncs = {
    xnestRealizeCursor,
    xnestUnrealizeCursor,
    xnestSetCursor,
    xnestMoveCursor,
    xnestDeviceCursorInitialize,
    xnestDeviceCursorCleanup
};

Bool
xnestOpenScreen(ScreenPtr pScreen, int argc, char *argv[])
{
    VisualPtr visuals;
    DepthPtr depths;
    int numVisuals, numDepths;
    int i, j, depthIndex;
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    XWindowAttributes gattributes;
    XSizeHints sizeHints;
    VisualID defaultVisual;
    int rootDepth;
    miPointerScreenPtr PointPriv;

    if (!dixRegisterPrivateKey
        (&xnestWindowPrivateKeyRec, PRIVATE_WINDOW, sizeof(xnestPrivWin)))
        return FALSE;
    if (!dixRegisterPrivateKey
        (&xnestGCPrivateKeyRec, PRIVATE_GC, sizeof(xnestPrivGC)))
        return FALSE;
    if (!dixRegisterPrivateKey
        (&xnestPixmapPrivateKeyRec, PRIVATE_PIXMAP, sizeof(xnestPrivPixmap)))
        return FALSE;
    if (!dixRegisterPrivateKey
        (&xnestColormapPrivateKeyRec, PRIVATE_COLORMAP,
         sizeof(xnestPrivColormap)))
        return FALSE;
    if (!dixRegisterPrivateKey(&xnestCursorScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    visuals = xallocarray(xnestNumVisuals, sizeof(VisualRec));
    numVisuals = 0;

    depths = (DepthPtr) malloc(MAXDEPTH * sizeof(DepthRec));
    depths[0].depth = 1;
    depths[0].numVids = 0;
    depths[0].vids = (VisualID *) malloc(MAXVISUALSPERDEPTH * sizeof(VisualID));
    numDepths = 1;

    for (i = 0; i < xnestNumVisuals; i++) {
        visuals[numVisuals].class = xnestVisuals[i].class;
        visuals[numVisuals].bitsPerRGBValue = xnestVisuals[i].bits_per_rgb;
        visuals[numVisuals].ColormapEntries = xnestVisuals[i].colormap_size;
        visuals[numVisuals].nplanes = xnestVisuals[i].depth;
        visuals[numVisuals].redMask = xnestVisuals[i].red_mask;
        visuals[numVisuals].greenMask = xnestVisuals[i].green_mask;
        visuals[numVisuals].blueMask = xnestVisuals[i].blue_mask;
        visuals[numVisuals].offsetRed = offset(xnestVisuals[i].red_mask);
        visuals[numVisuals].offsetGreen = offset(xnestVisuals[i].green_mask);
        visuals[numVisuals].offsetBlue = offset(xnestVisuals[i].blue_mask);

        /* Check for and remove duplicates. */
        for (j = 0; j < numVisuals; j++) {
            if (visuals[numVisuals].class == visuals[j].class &&
                visuals[numVisuals].bitsPerRGBValue ==
                visuals[j].bitsPerRGBValue &&
                visuals[numVisuals].ColormapEntries ==
                visuals[j].ColormapEntries &&
                visuals[numVisuals].nplanes == visuals[j].nplanes &&
                visuals[numVisuals].redMask == visuals[j].redMask &&
                visuals[numVisuals].greenMask == visuals[j].greenMask &&
                visuals[numVisuals].blueMask == visuals[j].blueMask &&
                visuals[numVisuals].offsetRed == visuals[j].offsetRed &&
                visuals[numVisuals].offsetGreen == visuals[j].offsetGreen &&
                visuals[numVisuals].offsetBlue == visuals[j].offsetBlue)
                break;
        }
        if (j < numVisuals)
            break;

        visuals[numVisuals].vid = FakeClientID(0);

        depthIndex = UNDEFINED;
        for (j = 0; j < numDepths; j++)
            if (depths[j].depth == xnestVisuals[i].depth) {
                depthIndex = j;
                break;
            }

        if (depthIndex == UNDEFINED) {
            depthIndex = numDepths;
            depths[depthIndex].depth = xnestVisuals[i].depth;
            depths[depthIndex].numVids = 0;
            depths[depthIndex].vids =
                (VisualID *) malloc(MAXVISUALSPERDEPTH * sizeof(VisualID));
            numDepths++;
        }
        if (depths[depthIndex].numVids >= MAXVISUALSPERDEPTH) {
            FatalError("Visual table overflow");
        }
        depths[depthIndex].vids[depths[depthIndex].numVids] =
            visuals[numVisuals].vid;
        depths[depthIndex].numVids++;

        numVisuals++;
    }
    visuals = reallocarray(visuals, numVisuals, sizeof(VisualRec));

    defaultVisual = visuals[xnestDefaultVisualIndex].vid;
    rootDepth = visuals[xnestDefaultVisualIndex].nplanes;

    if (xnestParentWindow != 0) {
        XGetWindowAttributes(xnestDisplay, xnestParentWindow, &gattributes);
        xnestWidth = gattributes.width;
        xnestHeight = gattributes.height;
    }

    /* myNum */
    /* id */
    miScreenInit(pScreen, NULL, xnestWidth, xnestHeight, 1, 1, xnestWidth, rootDepth, numDepths, depths, defaultVisual, /* root visual */
                 numVisuals, visuals);

    pScreen->defColormap = (Colormap) FakeClientID(0);
    pScreen->minInstalledCmaps = MINCMAPS;
    pScreen->maxInstalledCmaps = MAXCMAPS;
    pScreen->backingStoreSupport = NotUseful;
    pScreen->saveUnderSupport = NotUseful;
    pScreen->whitePixel = xnestWhitePixel;
    pScreen->blackPixel = xnestBlackPixel;
    /* GCperDepth */
    /* defaultStipple */
    pScreen->devPrivate = NULL;
    /* WindowPrivateLen */
    /* WindowPrivateSizes */
    /* totalWindowSize */
    /* GCPrivateLen */
    /* GCPrivateSizes */
    /* totalGCSize */

    /* Random screen procedures */

    pScreen->QueryBestSize = xnestQueryBestSize;
    pScreen->SaveScreen = xnestSaveScreen;
    pScreen->GetImage = xnestGetImage;
    pScreen->GetSpans = xnestGetSpans;

    /* Window Procedures */

    pScreen->CreateWindow = xnestCreateWindow;
    pScreen->DestroyWindow = xnestDestroyWindow;
    pScreen->PositionWindow = xnestPositionWindow;
    pScreen->ChangeWindowAttributes = xnestChangeWindowAttributes;
    pScreen->RealizeWindow = xnestRealizeWindow;
    pScreen->UnrealizeWindow = xnestUnrealizeWindow;
    pScreen->PostValidateTree = NULL;
    pScreen->WindowExposures = xnestWindowExposures;
    pScreen->CopyWindow = xnestCopyWindow;
    pScreen->ClipNotify = xnestClipNotify;

    /* Pixmap procedures */

    pScreen->CreatePixmap = xnestCreatePixmap;
    pScreen->DestroyPixmap = xnestDestroyPixmap;
    pScreen->ModifyPixmapHeader = xnestModifyPixmapHeader;

    /* Font procedures */

    pScreen->RealizeFont = xnestRealizeFont;
    pScreen->UnrealizeFont = xnestUnrealizeFont;

    /* GC procedures */

    pScreen->CreateGC = xnestCreateGC;

    /* Colormap procedures */

    pScreen->CreateColormap = xnestCreateColormap;
    pScreen->DestroyColormap = xnestDestroyColormap;
    pScreen->InstallColormap = xnestInstallColormap;
    pScreen->UninstallColormap = xnestUninstallColormap;
    pScreen->ListInstalledColormaps = xnestListInstalledColormaps;
    pScreen->StoreColors = xnestStoreColors;
    pScreen->ResolveColor = xnestResolveColor;

    pScreen->BitmapToRegion = xnestPixmapToRegion;

    /* OS layer procedures */

    pScreen->BlockHandler = (ScreenBlockHandlerProcPtr) NoopDDA;
    pScreen->WakeupHandler = (ScreenWakeupHandlerProcPtr) NoopDDA;

    miDCInitialize(pScreen, &xnestPointerCursorFuncs);  /* init SW rendering */
    PointPriv = dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);
    xnestCursorFuncs.spriteFuncs = PointPriv->spriteFuncs;
    dixSetPrivate(&pScreen->devPrivates, xnestCursorScreenKey,
                  &xnestCursorFuncs);
    PointPriv->spriteFuncs = &xnestPointerSpriteFuncs;

    pScreen->mmWidth = xnestWidth * DisplayWidthMM(xnestDisplay,
                                                   DefaultScreen(xnestDisplay))
        / DisplayWidth(xnestDisplay, DefaultScreen(xnestDisplay));
    pScreen->mmHeight =
        xnestHeight * DisplayHeightMM(xnestDisplay,
                                      DefaultScreen(xnestDisplay)) /
        DisplayHeight(xnestDisplay, DefaultScreen(xnestDisplay));

    /* overwrite miCloseScreen with our own */
    pScreen->CloseScreen = xnestCloseScreen;

    if (!miScreenDevPrivateInit(pScreen, xnestWidth, NULL))
        return FALSE;

    /* overwrite miSetShape with our own */
    pScreen->SetShape = xnestSetShape;

    /* devPrivates */

#define POSITION_OFFSET (pScreen->myNum * (xnestWidth + xnestHeight) / 32)

    if (xnestDoFullGeneration) {

        valuemask = CWBackPixel | CWEventMask | CWColormap;
        attributes.background_pixel = xnestWhitePixel;
        attributes.event_mask = xnestEventMask;
        attributes.colormap =
            xnestDefaultVisualColormap(xnestDefaultVisual(pScreen));

        if (xnestParentWindow != 0) {
            xnestDefaultWindows[pScreen->myNum] = xnestParentWindow;
            XSelectInput(xnestDisplay, xnestDefaultWindows[pScreen->myNum],
                         xnestEventMask);
        }
        else
            xnestDefaultWindows[pScreen->myNum] =
                XCreateWindow(xnestDisplay,
                              DefaultRootWindow(xnestDisplay),
                              xnestX + POSITION_OFFSET,
                              xnestY + POSITION_OFFSET,
                              xnestWidth, xnestHeight,
                              xnestBorderWidth,
                              pScreen->rootDepth,
                              InputOutput,
                              xnestDefaultVisual(pScreen),
                              valuemask, &attributes);

        if (!xnestWindowName)
            xnestWindowName = argv[0];

        sizeHints.flags = PPosition | PSize | PMaxSize;
        sizeHints.x = xnestX + POSITION_OFFSET;
        sizeHints.y = xnestY + POSITION_OFFSET;
        sizeHints.width = sizeHints.max_width = xnestWidth;
        sizeHints.height = sizeHints.max_height = xnestHeight;
        if (xnestUserGeometry & XValue || xnestUserGeometry & YValue)
            sizeHints.flags |= USPosition;
        if (xnestUserGeometry & WidthValue || xnestUserGeometry & HeightValue)
            sizeHints.flags |= USSize;
        XSetStandardProperties(xnestDisplay,
                               xnestDefaultWindows[pScreen->myNum],
                               xnestWindowName,
                               xnestWindowName,
                               xnestIconBitmap, argv, argc, &sizeHints);

        XMapWindow(xnestDisplay, xnestDefaultWindows[pScreen->myNum]);

        valuemask = CWBackPixmap | CWColormap;
        attributes.background_pixmap = xnestScreenSaverPixmap;
        attributes.colormap =
            DefaultColormap(xnestDisplay, DefaultScreen(xnestDisplay));
        xnestScreenSaverWindows[pScreen->myNum] =
            XCreateWindow(xnestDisplay,
                          xnestDefaultWindows[pScreen->myNum],
                          0, 0, xnestWidth, xnestHeight, 0,
                          DefaultDepth(xnestDisplay,
                                       DefaultScreen(xnestDisplay)),
                          InputOutput, DefaultVisual(xnestDisplay,
                                                     DefaultScreen
                                                     (xnestDisplay)), valuemask,
                          &attributes);
    }

    if (!xnestCreateDefaultColormap(pScreen))
        return False;

    return True;
}

Bool
xnestCloseScreen(ScreenPtr pScreen)
{
    int i;

    for (i = 0; i < pScreen->numDepths; i++)
        free(pScreen->allowedDepths[i].vids);
    free(pScreen->allowedDepths);
    free(pScreen->visuals);
    free(pScreen->devPrivate);

    /*
       If xnestDoFullGeneration all x resources will be destroyed upon closing
       the display connection.  There is no need to generate extra protocol.
     */

    return True;
}
