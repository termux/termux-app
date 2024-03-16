/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 *		Kensuke Matsuzaki
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "winmsg.h"

/*
 * Determine what type of screen we are initializing
 * and call the appropriate procedure to initialize
 * that type of screen.
 */

Bool
winScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
    winScreenInfoPtr pScreenInfo = &g_ScreenInfo[pScreen->myNum];
    winPrivScreenPtr pScreenPriv;
    HDC hdc;
    DWORD dwInitialBPP;

#if CYGDEBUG || YES
    winDebug("winScreenInit - dwWidth: %u dwHeight: %u\n",
             (unsigned int)pScreenInfo->dwWidth, (unsigned int)pScreenInfo->dwHeight);
#endif

    /* Allocate privates for this screen */
    if (!winAllocatePrivates(pScreen)) {
        ErrorF("winScreenInit - Couldn't allocate screen privates\n");
        return FALSE;
    }

    /* Get a pointer to the privates structure that was allocated */
    pScreenPriv = winGetScreenPriv(pScreen);

    /* Save a pointer to this screen in the screen info structure */
    pScreenInfo->pScreen = pScreen;

    /* Save a pointer to the screen info in the screen privates structure */
    /* This allows us to get back to the screen info from a screen pointer */
    pScreenPriv->pScreenInfo = pScreenInfo;

    /*
     * Determine which engine to use.
     *
     * NOTE: This is done once per screen because each screen possibly has
     * a preferred engine specified on the command line.
     */
    if (!winSetEngine(pScreen)) {
        ErrorF("winScreenInit - winSetEngine () failed\n");
        return FALSE;
    }

    /* Horribly misnamed function: Allow engine to adjust BPP for screen */
    dwInitialBPP = pScreenInfo->dwBPP;

    if (!(*pScreenPriv->pwinAdjustVideoMode) (pScreen)) {
        ErrorF("winScreenInit - winAdjustVideoMode () failed\n");
        return FALSE;
    }

    if (dwInitialBPP == WIN_DEFAULT_BPP) {
        /* No -depth parameter was passed, let the user know the depth being used */
        ErrorF
            ("winScreenInit - Using Windows display depth of %d bits per pixel\n",
             (int) pScreenInfo->dwBPP);
    }
    else if (dwInitialBPP != pScreenInfo->dwBPP) {
        /* Warn user if engine forced a depth different to -depth parameter */
        ErrorF
            ("winScreenInit - Command line depth of %d bpp overridden by engine, using %d bpp\n",
             (int) dwInitialBPP, (int) pScreenInfo->dwBPP);
    }
    else {
        ErrorF("winScreenInit - Using command line depth of %d bpp\n",
               (int) pScreenInfo->dwBPP);
    }

    /* Check for supported display depth */
    if (!(WIN_SUPPORTED_BPPS & (1 << (pScreenInfo->dwBPP - 1)))) {
        ErrorF("winScreenInit - Unsupported display depth: %d\n"
               "Change your Windows display depth to 15, 16, 24, or 32 bits "
               "per pixel.\n", (int) pScreenInfo->dwBPP);
        ErrorF("winScreenInit - Supported depths: %08x\n", WIN_SUPPORTED_BPPS);
#if WIN_CHECK_DEPTH
        return FALSE;
#endif
    }

    /*
     * Check that all monitors have the same display depth if we are using
     * multiple monitors
     */
    if (pScreenInfo->fMultipleMonitors
        && !GetSystemMetrics(SM_SAMEDISPLAYFORMAT)) {
        ErrorF("winScreenInit - Monitors do not all have same pixel format / "
               "display depth.\n");
        if (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_GDI) {
            ErrorF
                ("winScreenInit - Performance may suffer off primary display.\n");
        }
        else {
            ErrorF("winScreenInit - Using primary display only.\n");
            pScreenInfo->fMultipleMonitors = FALSE;
        }
    }

    /* Create display window */
    if (!(*pScreenPriv->pwinCreateBoundingWindow) (pScreen)) {
        ErrorF("winScreenInit - pwinCreateBoundingWindow () " "failed\n");
        return FALSE;
    }

    /* Get a device context */
    hdc = GetDC(pScreenPriv->hwndScreen);

    /* Are we using multiple monitors? */
    if (pScreenInfo->fMultipleMonitors) {
        /*
         * In this case, some of the defaults set in
         * winInitializeScreenDefaults() are not correct ...
         */
        if (!pScreenInfo->fUserGaveHeightAndWidth) {
            pScreenInfo->dwWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            pScreenInfo->dwHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        }
    }

    /* Release the device context */
    ReleaseDC(pScreenPriv->hwndScreen, hdc);

    /* Clear the visuals list */
    miClearVisualTypes();

    /* Call the engine dependent screen initialization procedure */
    if (!((*pScreenPriv->pwinFinishScreenInit) (pScreen->myNum, pScreen, argc, argv))) {
        ErrorF("winScreenInit - winFinishScreenInit () failed\n");

        /* call the engine dependent screen close procedure to clean up from a failure */
        pScreenPriv->pwinCloseScreen(pScreen);

        return FALSE;
    }

    if (!g_fSoftwareCursor)
        winInitCursor(pScreen);
    else
        winErrorFVerb(2, "winScreenInit - Using software cursor\n");

    if (!noPanoramiXExtension) {
        /*
           Note the screen origin in a normalized coordinate space where (0,0) is at the top left
           of the native virtual desktop area
         */
        pScreen->x =
            pScreenInfo->dwInitialX - GetSystemMetrics(SM_XVIRTUALSCREEN);
        pScreen->y =
            pScreenInfo->dwInitialY - GetSystemMetrics(SM_YVIRTUALSCREEN);

        ErrorF("Screen %d added at virtual desktop coordinate (%d,%d).\n",
               pScreen->myNum, pScreen->x, pScreen->y);
    }

#if CYGDEBUG || YES
    winDebug("winScreenInit - returning\n");
#endif

    return TRUE;
}

static Bool
winCreateScreenResources(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    Bool result;

    result = pScreenPriv->pwinCreateScreenResources(pScreen);

    /* Now the screen bitmap has been wrapped in a pixmap,
       add that to the Shadow framebuffer */
    if (!shadowAdd(pScreen, pScreen->devPrivate,
                   pScreenPriv->pwinShadowUpdate, NULL, 0, 0)) {
        ErrorF("winCreateScreenResources - shadowAdd () failed\n");
        return FALSE;
    }

    return result;
}

/* See Porting Layer Definition - p. 20 */
Bool
winFinishScreenInitFB(int i, ScreenPtr pScreen, int argc, char **argv)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    VisualPtr pVisual = NULL;

    int iReturn;

    /* Create framebuffer */
    if (!(*pScreenPriv->pwinInitScreen) (pScreen)) {
        ErrorF("winFinishScreenInitFB - Could not allocate framebuffer\n");
        return FALSE;
    }

    /*
     * Calculate the number of bits that are used to represent color in each pixel,
     * the color depth for the screen
     */
    if (pScreenInfo->dwBPP == 8)
        pScreenInfo->dwDepth = 8;
    else
        pScreenInfo->dwDepth = winCountBits(pScreenPriv->dwRedMask)
            + winCountBits(pScreenPriv->dwGreenMask)
            + winCountBits(pScreenPriv->dwBlueMask);

    winErrorFVerb(2, "winFinishScreenInitFB - Masks: %08x %08x %08x\n",
                  (unsigned int) pScreenPriv->dwRedMask,
                  (unsigned int) pScreenPriv->dwGreenMask,
                  (unsigned int) pScreenPriv->dwBlueMask);

    /* Init visuals */
    if (!(*pScreenPriv->pwinInitVisuals) (pScreen)) {
        ErrorF("winFinishScreenInitFB - winInitVisuals failed\n");
        return FALSE;
    }

    if ((pScreenInfo->dwBPP == 8) && (pScreenInfo->fCompositeWM)) {
        ErrorF("-compositewm disabled due to 8bpp depth\n");
        pScreenInfo->fCompositeWM = FALSE;
    }

    /* Apparently we need this for the render extension */
    miSetPixmapDepths();

    /* Start fb initialization */
    if (!fbSetupScreen(pScreen,
                       pScreenInfo->pfb,
                       pScreenInfo->dwWidth, pScreenInfo->dwHeight,
                       monitorResolution, monitorResolution,
                       pScreenInfo->dwStride, pScreenInfo->dwBPP)) {
        ErrorF("winFinishScreenInitFB - fbSetupScreen failed\n");
        return FALSE;
    }

    /* Override default colormap routines if visual class is dynamic */
    if (pScreenInfo->dwDepth == 8
        && (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_GDI
            || (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_DDNL
                && pScreenInfo->fFullScreen))) {
        winSetColormapFunctions(pScreen);

        /*
         * NOTE: Setting whitePixel to 255 causes Magic 7.1 to allocate its
         * own colormap, as it cannot allocate 7 planes in the default
         * colormap.  Setting whitePixel to 1 allows Magic to get 7
         * planes in the default colormap, so it doesn't create its
         * own colormap.  This latter situation is highly desirable,
         * as it keeps the Magic window viewable when switching to
         * other X clients that use the default colormap.
         */
        pScreen->blackPixel = 0;
        pScreen->whitePixel = 1;
    }

    /* Finish fb initialization */
    if (!fbFinishScreenInit(pScreen,
                            pScreenInfo->pfb,
                            pScreenInfo->dwWidth, pScreenInfo->dwHeight,
                            monitorResolution, monitorResolution,
                            pScreenInfo->dwStride, pScreenInfo->dwBPP)) {
        ErrorF("winFinishScreenInitFB - fbFinishScreenInit failed\n");
        return FALSE;
    }

    /* Save a pointer to the root visual */
    for (pVisual = pScreen->visuals;
         pVisual->vid != pScreen->rootVisual; pVisual++);
    pScreenPriv->pRootVisual = pVisual;

    /*
     * Setup points to the block and wakeup handlers.  Pass a pointer
     * to the current screen as pWakeupdata.
     */
    pScreen->BlockHandler = winBlockHandler;
    pScreen->WakeupHandler = winWakeupHandler;

    /* Render extension initialization, calls miPictureInit */
    if (!fbPictureInit(pScreen, NULL, 0)) {
        ErrorF("winFinishScreenInitFB - fbPictureInit () failed\n");
        return FALSE;
    }

#ifdef RANDR
    /* Initialize resize and rotate support */
    if (!winRandRInit(pScreen)) {
        ErrorF("winFinishScreenInitFB - winRandRInit () failed\n");
        return FALSE;
    }
#endif

    /* Setup the cursor routines */
#if CYGDEBUG
    winDebug("winFinishScreenInitFB - Calling miDCInitialize ()\n");
#endif
    miDCInitialize(pScreen, &g_winPointerCursorFuncs);

    /* KDrive does winCreateDefColormap right after miDCInitialize */
    /* Create a default colormap */
#if CYGDEBUG
    winDebug("winFinishScreenInitFB - Calling winCreateDefColormap ()\n");
#endif
    if (!winCreateDefColormap(pScreen)) {
        ErrorF("winFinishScreenInitFB - Could not create colormap\n");
        return FALSE;
    }

    /* Initialize the shadow framebuffer layer */
    if ((pScreenInfo->dwEngine == WIN_SERVER_SHADOW_GDI
         || pScreenInfo->dwEngine == WIN_SERVER_SHADOW_DDNL)) {
#if CYGDEBUG
        winDebug("winFinishScreenInitFB - Calling shadowSetup ()\n");
#endif
        if (!shadowSetup(pScreen)) {
            ErrorF("winFinishScreenInitFB - shadowSetup () failed\n");
            return FALSE;
        }

        /* Wrap CreateScreenResources so we can add the screen pixmap
           to the Shadow framebuffer after it's been created */
        pScreenPriv->pwinCreateScreenResources = pScreen->CreateScreenResources;
        pScreen->CreateScreenResources = winCreateScreenResources;
    }

    /* Handle rootless mode */
    if (pScreenInfo->fRootless) {
        /* Define the WRAP macro temporarily for local use */
#define WRAP(a) \
    if (pScreen->a) { \
        pScreenPriv->a = pScreen->a; \
    } else { \
        winDebug("winScreenInit - null screen fn " #a "\n"); \
        pScreenPriv->a = NULL; \
    }

        /* Save a pointer to each lower-level window procedure */
        WRAP(CreateWindow);
        WRAP(DestroyWindow);
        WRAP(RealizeWindow);
        WRAP(UnrealizeWindow);
        WRAP(PositionWindow);
        WRAP(ChangeWindowAttributes);
        WRAP(SetShape);

        /* Assign rootless window procedures to be top level procedures */
        pScreen->CreateWindow = winCreateWindowRootless;
        pScreen->DestroyWindow = winDestroyWindowRootless;
        pScreen->PositionWindow = winPositionWindowRootless;
        /*pScreen->ChangeWindowAttributes = winChangeWindowAttributesRootless; */
        pScreen->RealizeWindow = winMapWindowRootless;
        pScreen->UnrealizeWindow = winUnmapWindowRootless;
        pScreen->SetShape = winSetShapeRootless;

        /* Undefine the WRAP macro, as it is not needed elsewhere */
#undef WRAP
    }

    /* Handle multi window mode */
    else if (pScreenInfo->fMultiWindow) {
        /* Define the WRAP macro temporarily for local use */
#define WRAP(a) \
    if (pScreen->a) { \
        pScreenPriv->a = pScreen->a; \
    } else { \
        winDebug("null screen fn " #a "\n"); \
        pScreenPriv->a = NULL; \
    }

        /* Save a pointer to each lower-level window procedure */
        WRAP(CreateWindow);
        WRAP(DestroyWindow);
        WRAP(RealizeWindow);
        WRAP(UnrealizeWindow);
        WRAP(PositionWindow);
        WRAP(ChangeWindowAttributes);
        WRAP(ReparentWindow);
        WRAP(RestackWindow);
        WRAP(ResizeWindow);
        WRAP(MoveWindow);
        WRAP(CopyWindow);
        WRAP(SetShape);
        WRAP(ModifyPixmapHeader);

        /* Assign multi-window window procedures to be top level procedures */
        pScreen->CreateWindow = winCreateWindowMultiWindow;
        pScreen->DestroyWindow = winDestroyWindowMultiWindow;
        pScreen->PositionWindow = winPositionWindowMultiWindow;
        /*pScreen->ChangeWindowAttributes = winChangeWindowAttributesMultiWindow; */
        pScreen->RealizeWindow = winMapWindowMultiWindow;
        pScreen->UnrealizeWindow = winUnmapWindowMultiWindow;
        pScreen->ReparentWindow = winReparentWindowMultiWindow;
        pScreen->RestackWindow = winRestackWindowMultiWindow;
        pScreen->ResizeWindow = winResizeWindowMultiWindow;
        pScreen->MoveWindow = winMoveWindowMultiWindow;
        pScreen->CopyWindow = winCopyWindowMultiWindow;
        pScreen->SetShape = winSetShapeMultiWindow;

        if (pScreenInfo->fCompositeWM) {
            pScreen->CreatePixmap = winCreatePixmapMultiwindow;
            pScreen->DestroyPixmap = winDestroyPixmapMultiwindow;
            pScreen->ModifyPixmapHeader = winModifyPixmapHeaderMultiwindow;
        }

        /* Undefine the WRAP macro, as it is not needed elsewhere */
#undef WRAP
    }

    /* Wrap either fb's or shadow's CloseScreen with our CloseScreen */
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = pScreenPriv->pwinCloseScreen;

    /* Create a mutex for modules in separate threads to wait for */
    iReturn = pthread_mutex_init(&pScreenPriv->pmServerStarted, NULL);
    if (iReturn != 0) {
        ErrorF("winFinishScreenInitFB - pthread_mutex_init () failed: %d\n",
               iReturn);
        return FALSE;
    }

    /* Own the mutex for modules in separate threads */
    iReturn = pthread_mutex_lock(&pScreenPriv->pmServerStarted);
    if (iReturn != 0) {
        ErrorF("winFinishScreenInitFB - pthread_mutex_lock () failed: %d\n",
               iReturn);
        return FALSE;
    }

    /* Set the ServerStarted flag to false */
    pScreenPriv->fServerStarted = FALSE;


    if (pScreenInfo->fMultiWindow) {
#if CYGDEBUG || YES
        winDebug("winFinishScreenInitFB - Calling winInitWM.\n");
#endif

        /* Initialize multi window mode */
        if (!winInitWM(&pScreenPriv->pWMInfo,
                       &pScreenPriv->ptWMProc,
                       &pScreenPriv->ptXMsgProc,
                       &pScreenPriv->pmServerStarted,
                       pScreenInfo->dwScreen,
                       (HWND) &pScreenPriv->hwndScreen,
                       pScreenInfo->fCompositeWM)) {
            ErrorF("winFinishScreenInitFB - winInitWM () failed.\n");
            return FALSE;
        }
    }

    /* Tell the server that we are enabled */
    pScreenPriv->fEnabled = TRUE;

    /* Tell the server that we have a valid depth */
    pScreenPriv->fBadDepth = FALSE;

#if CYGDEBUG || YES
    winDebug("winFinishScreenInitFB - returning\n");
#endif

    return TRUE;
}
