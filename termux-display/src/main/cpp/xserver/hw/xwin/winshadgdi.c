/*
 *Copyright (C) 2001-2004 Harold L Hunt II All Rights Reserved.
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
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of Harold L Hunt II
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from Harold L Hunt II.
 *
 * Authors:	Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

/*
 * Local function prototypes
 */

static wBOOL CALLBACK winRedrawAllProcShadowGDI(HWND hwnd, LPARAM lParam);

static wBOOL CALLBACK winRedrawDamagedWindowShadowGDI(HWND hwnd, LPARAM lParam);

static Bool
 winAllocateFBShadowGDI(ScreenPtr pScreen);

static void
 winShadowUpdateGDI(ScreenPtr pScreen, shadowBufPtr pBuf);

static Bool
 winCloseScreenShadowGDI(ScreenPtr pScreen);

static Bool
 winInitVisualsShadowGDI(ScreenPtr pScreen);

static Bool
 winAdjustVideoModeShadowGDI(ScreenPtr pScreen);

static Bool
 winBltExposedRegionsShadowGDI(ScreenPtr pScreen);

static Bool
 winBltExposedWindowRegionShadowGDI(ScreenPtr pScreen, WindowPtr pWin);

static Bool
 winActivateAppShadowGDI(ScreenPtr pScreen);

static Bool
 winRedrawScreenShadowGDI(ScreenPtr pScreen);

static Bool
 winRealizeInstalledPaletteShadowGDI(ScreenPtr pScreen);

static Bool
 winInstallColormapShadowGDI(ColormapPtr pColormap);

static Bool
 winStoreColorsShadowGDI(ColormapPtr pmap, int ndef, xColorItem * pdefs);

static Bool
 winCreateColormapShadowGDI(ColormapPtr pColormap);

static Bool
 winDestroyColormapShadowGDI(ColormapPtr pColormap);

/*
 * Internal function to get the DIB format that is compatible with the screen
 */

static
    Bool
winQueryScreenDIBFormat(ScreenPtr pScreen, BITMAPINFOHEADER * pbmih)
{
    winScreenPriv(pScreen);
    HBITMAP hbmp;

#if CYGDEBUG
    LPDWORD pdw = NULL;
#endif

    /* Create a memory bitmap compatible with the screen */
    hbmp = CreateCompatibleBitmap(pScreenPriv->hdcScreen, 1, 1);
    if (hbmp == NULL) {
        ErrorF("winQueryScreenDIBFormat - CreateCompatibleBitmap failed\n");
        return FALSE;
    }

    /* Initialize our bitmap info header */
    ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    pbmih->biSize = sizeof(BITMAPINFOHEADER);

    /* Get the biBitCount */
    if (!GetDIBits(pScreenPriv->hdcScreen,
                   hbmp, 0, 1, NULL, (BITMAPINFO *) pbmih, DIB_RGB_COLORS)) {
        ErrorF("winQueryScreenDIBFormat - First call to GetDIBits failed\n");
        DeleteObject(hbmp);
        return FALSE;
    }

#if CYGDEBUG
    /* Get a pointer to bitfields */
    pdw = (DWORD *) ((CARD8 *) pbmih + sizeof(BITMAPINFOHEADER));

    winDebug("winQueryScreenDIBFormat - First call masks: %08x %08x %08x\n",
             (unsigned int)pdw[0], (unsigned int)pdw[1], (unsigned int)pdw[2]);
#endif

    /* Get optimal color table, or the optimal bitfields */
    if (!GetDIBits(pScreenPriv->hdcScreen,
                   hbmp, 0, 1, NULL, (BITMAPINFO *) pbmih, DIB_RGB_COLORS)) {
        ErrorF("winQueryScreenDIBFormat - Second call to GetDIBits "
               "failed\n");
        DeleteObject(hbmp);
        return FALSE;
    }

    /* Free memory */
    DeleteObject(hbmp);

    return TRUE;
}

/*
 * Internal function to determine the GDI bits per rgb and bit masks
 */

static
    Bool
winQueryRGBBitsAndMasks(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    BITMAPINFOHEADER *pbmih = NULL;
    Bool fReturn = TRUE;
    LPDWORD pdw = NULL;
    DWORD dwRedBits, dwGreenBits, dwBlueBits;

    /* Color masks for 8 bpp are standardized */
    if (GetDeviceCaps(pScreenPriv->hdcScreen, RASTERCAPS) & RC_PALETTE) {
        /*
         * RGB BPP for 8 bit palletes is always 8
         * and the color masks are always 0.
         */
        pScreenPriv->dwBitsPerRGB = 8;
        pScreenPriv->dwRedMask = 0x0L;
        pScreenPriv->dwGreenMask = 0x0L;
        pScreenPriv->dwBlueMask = 0x0L;
        return TRUE;
    }

    /* Color masks for 24 bpp are standardized */
    if (GetDeviceCaps(pScreenPriv->hdcScreen, PLANES)
        * GetDeviceCaps(pScreenPriv->hdcScreen, BITSPIXEL) == 24) {
        ErrorF("winQueryRGBBitsAndMasks - GetDeviceCaps (BITSPIXEL) "
               "returned 24 for the screen.  Using default 24bpp masks.\n");

        /* 8 bits per primary color */
        pScreenPriv->dwBitsPerRGB = 8;

        /* Set screen privates masks */
        pScreenPriv->dwRedMask = WIN_24BPP_MASK_RED;
        pScreenPriv->dwGreenMask = WIN_24BPP_MASK_GREEN;
        pScreenPriv->dwBlueMask = WIN_24BPP_MASK_BLUE;

        return TRUE;
    }

    /* Allocate a bitmap header and color table */
    pbmih = malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    if (pbmih == NULL) {
        ErrorF("winQueryRGBBitsAndMasks - malloc failed\n");
        return FALSE;
    }

    /* Get screen description */
    if (winQueryScreenDIBFormat(pScreen, pbmih)) {
        /* Get a pointer to bitfields */
        pdw = (DWORD *) ((CARD8 *) pbmih + sizeof(BITMAPINFOHEADER));

#if CYGDEBUG
        winDebug("%s - Masks: %08x %08x %08x\n", __FUNCTION__,
                 (unsigned int)pdw[0], (unsigned int)pdw[1], (unsigned int)pdw[2]);
        winDebug("%s - Bitmap: %dx%d %d bpp %d planes\n", __FUNCTION__,
                 (int)pbmih->biWidth, (int)pbmih->biHeight, pbmih->biBitCount,
                 pbmih->biPlanes);
        winDebug("%s - Compression: %u %s\n", __FUNCTION__,
                 (unsigned int)pbmih->biCompression,
                 (pbmih->biCompression ==
                  BI_RGB ? "(BI_RGB)" : (pbmih->biCompression ==
                                         BI_RLE8 ? "(BI_RLE8)" : (pbmih->
                                                                  biCompression
                                                                  ==
                                                                  BI_RLE4 ?
                                                                  "(BI_RLE4)"
                                                                  : (pbmih->
                                                                     biCompression
                                                                     ==
                                                                     BI_BITFIELDS
                                                                     ?
                                                                     "(BI_BITFIELDS)"
                                                                     : "")))));
#endif

        /* Handle BI_RGB case, which is returned by Wine */
        if (pbmih->biCompression == BI_RGB) {
            dwRedBits = 5;
            dwGreenBits = 5;
            dwBlueBits = 5;

            pScreenPriv->dwBitsPerRGB = 5;

            /* Set screen privates masks */
            pScreenPriv->dwRedMask = 0x7c00;
            pScreenPriv->dwGreenMask = 0x03e0;
            pScreenPriv->dwBlueMask = 0x001f;
        }
        else {
            /* Count the number of bits in each mask */
            dwRedBits = winCountBits(pdw[0]);
            dwGreenBits = winCountBits(pdw[1]);
            dwBlueBits = winCountBits(pdw[2]);

            /* Find maximum bits per red, green, blue */
            if (dwRedBits > dwGreenBits && dwRedBits > dwBlueBits)
                pScreenPriv->dwBitsPerRGB = dwRedBits;
            else if (dwGreenBits > dwRedBits && dwGreenBits > dwBlueBits)
                pScreenPriv->dwBitsPerRGB = dwGreenBits;
            else
                pScreenPriv->dwBitsPerRGB = dwBlueBits;

            /* Set screen privates masks */
            pScreenPriv->dwRedMask = pdw[0];
            pScreenPriv->dwGreenMask = pdw[1];
            pScreenPriv->dwBlueMask = pdw[2];
        }
    }
    else {
        ErrorF("winQueryRGBBitsAndMasks - winQueryScreenDIBFormat failed\n");
        fReturn = FALSE;
    }

    /* Free memory */
    free(pbmih);

    return fReturn;
}

/*
 * Redraw all ---?
 */

static wBOOL CALLBACK
winRedrawAllProcShadowGDI(HWND hwnd, LPARAM lParam)
{
    if (hwnd == (HWND) lParam)
        return TRUE;
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
    return TRUE;
}

static wBOOL CALLBACK
winRedrawDamagedWindowShadowGDI(HWND hwnd, LPARAM lParam)
{
    BoxPtr pDamage = (BoxPtr) lParam;
    RECT rcClient, rcDamage, rcRedraw;
    POINT topLeft, bottomRight;

    if (IsIconic(hwnd))
        return TRUE;            /* Don't care minimized windows */

    /* Convert the damaged area from Screen coords to Client coords */
    topLeft.x = pDamage->x1;
    topLeft.y = pDamage->y1;
    bottomRight.x = pDamage->x2;
    bottomRight.y = pDamage->y2;
    topLeft.x += GetSystemMetrics(SM_XVIRTUALSCREEN);
    bottomRight.x += GetSystemMetrics(SM_XVIRTUALSCREEN);
    topLeft.y += GetSystemMetrics(SM_YVIRTUALSCREEN);
    bottomRight.y += GetSystemMetrics(SM_YVIRTUALSCREEN);
    ScreenToClient(hwnd, &topLeft);
    ScreenToClient(hwnd, &bottomRight);
    SetRect(&rcDamage, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);

    GetClientRect(hwnd, &rcClient);

    if (IntersectRect(&rcRedraw, &rcClient, &rcDamage)) {
        InvalidateRect(hwnd, &rcRedraw, FALSE);
        UpdateWindow(hwnd);
    }
    return TRUE;
}

/*
 * Allocate a DIB for the shadow framebuffer GDI server
 */

static Bool
winAllocateFBShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    DIBSECTION dibsection;
    Bool fReturn = TRUE;

    /* Describe shadow bitmap to be created */
    pScreenPriv->pbmih->biWidth = pScreenInfo->dwWidth;
    pScreenPriv->pbmih->biHeight = -pScreenInfo->dwHeight;

    ErrorF("winAllocateFBShadowGDI - Creating DIB with width: %d height: %d "
           "depth: %d\n",
           (int) pScreenPriv->pbmih->biWidth,
           (int) -pScreenPriv->pbmih->biHeight, pScreenPriv->pbmih->biBitCount);

    /* Create a DI shadow bitmap with a bit pointer */
    pScreenPriv->hbmpShadow = CreateDIBSection(pScreenPriv->hdcScreen,
                                               (BITMAPINFO *) pScreenPriv->
                                               pbmih, DIB_RGB_COLORS,
                                               (VOID **) &pScreenInfo->pfb,
                                               NULL, 0);
    if (pScreenPriv->hbmpShadow == NULL || pScreenInfo->pfb == NULL) {
        winW32Error(2, "winAllocateFBShadowGDI - CreateDIBSection failed:");
        return FALSE;
    }
    else {
#if CYGDEBUG
        winDebug("winAllocateFBShadowGDI - Shadow buffer allocated\n");
#endif
    }

    /* Get information about the bitmap that was allocated */
    GetObject(pScreenPriv->hbmpShadow, sizeof(dibsection), &dibsection);

#if CYGDEBUG || YES
    /* Print information about bitmap allocated */
    winDebug("winAllocateFBShadowGDI - Dibsection width: %d height: %d "
             "depth: %d size image: %d\n",
             (int) dibsection.dsBmih.biWidth, (int) dibsection.dsBmih.biHeight,
             dibsection.dsBmih.biBitCount, (int) dibsection.dsBmih.biSizeImage);
#endif

    /* Select the shadow bitmap into the shadow DC */
    SelectObject(pScreenPriv->hdcShadow, pScreenPriv->hbmpShadow);

#if CYGDEBUG
    winDebug("winAllocateFBShadowGDI - Attempting a shadow blit\n");
#endif

    /* Do a test blit from the shadow to the screen, I think */
    fReturn = BitBlt(pScreenPriv->hdcScreen,
                     0, 0,
                     pScreenInfo->dwWidth, pScreenInfo->dwHeight,
                     pScreenPriv->hdcShadow, 0, 0, SRCCOPY);
    if (fReturn) {
#if CYGDEBUG
        winDebug("winAllocateFBShadowGDI - Shadow blit success\n");
#endif
    }
    else {
        winW32Error(2, "winAllocateFBShadowGDI - Shadow blit failure\n");
#if 0
        return FALSE;
#else
        /* ago: ignore this error. The blit fails with wine, but does not
         * cause any problems later. */

        fReturn = TRUE;
#endif
    }

    /* Look for height weirdness */
    if (dibsection.dsBmih.biHeight < 0) {
        dibsection.dsBmih.biHeight = -dibsection.dsBmih.biHeight;
    }

    /* Set screeninfo stride */
    pScreenInfo->dwStride = ((dibsection.dsBmih.biSizeImage
                              / dibsection.dsBmih.biHeight)
                             * 8) / pScreenInfo->dwBPP;

#if CYGDEBUG || YES
    winDebug("winAllocateFBShadowGDI - Created shadow stride: %d\n",
             (int) pScreenInfo->dwStride);
#endif

    /* Redraw all windows */
    if (pScreenInfo->fMultiWindow)
        EnumThreadWindows(g_dwCurrentThreadID, winRedrawAllProcShadowGDI, 0);

    return fReturn;
}

static void
winFreeFBShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    /* Free the shadow bitmap */
    DeleteObject(pScreenPriv->hbmpShadow);

    /* Invalidate the ScreenInfo's fb pointer */
    pScreenInfo->pfb = NULL;
}

/*
 * Blit the damaged regions of the shadow fb to the screen
 */

static void
winShadowUpdateGDI(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    RegionPtr damage = DamageRegion(pBuf->pDamage);
    DWORD dwBox = RegionNumRects(damage);
    BoxPtr pBox = RegionRects(damage);
    int x, y, w, h;
    HRGN hrgnCombined = NULL;

#ifdef XWIN_UPDATESTATS
    static DWORD s_dwNonUnitRegions = 0;
    static DWORD s_dwTotalUpdates = 0;
    static DWORD s_dwTotalBoxes = 0;
#endif
    BoxPtr pBoxExtents = RegionExtents(damage);

    /*
     * Return immediately if the app is not active
     * and we are fullscreen, or if we have a bad display depth
     */
    if ((!pScreenPriv->fActive && pScreenInfo->fFullScreen)
        || pScreenPriv->fBadDepth)
        return;

#ifdef XWIN_UPDATESTATS
    ++s_dwTotalUpdates;
    s_dwTotalBoxes += dwBox;

    if (dwBox != 1) {
        ++s_dwNonUnitRegions;
        ErrorF("winShadowUpdatGDI - dwBox: %d\n", dwBox);
    }

    if ((s_dwTotalUpdates % 100) == 0)
        ErrorF("winShadowUpdateGDI - %d%% non-unity regions, avg boxes: %d "
               "nu: %d tu: %d\n",
               (s_dwNonUnitRegions * 100) / s_dwTotalUpdates,
               s_dwTotalBoxes / s_dwTotalUpdates,
               s_dwNonUnitRegions, s_dwTotalUpdates);
#endif                          /* XWIN_UPDATESTATS */

    /*
     * Handle small regions with multiple blits,
     * handle large regions by creating a clipping region and
     * doing a single blit constrained to that clipping region.
     */
    if (!pScreenInfo->fMultiWindow &&
        (pScreenInfo->dwClipUpdatesNBoxes == 0 ||
         dwBox < pScreenInfo->dwClipUpdatesNBoxes)) {
        /* Loop through all boxes in the damaged region */
        while (dwBox--) {
            /*
             * Calculate x offset, y offset, width, and height for
             * current damage box
             */
            x = pBox->x1;
            y = pBox->y1;
            w = pBox->x2 - pBox->x1;
            h = pBox->y2 - pBox->y1;

            BitBlt(pScreenPriv->hdcScreen,
                   x, y, w, h, pScreenPriv->hdcShadow, x, y, SRCCOPY);

            /* Get a pointer to the next box */
            ++pBox;
        }
    }
    else if (!pScreenInfo->fMultiWindow) {

        /* Compute a GDI region from the damaged region */
        hrgnCombined =
            CreateRectRgn(pBoxExtents->x1, pBoxExtents->y1, pBoxExtents->x2,
                          pBoxExtents->y2);

        /* Install the GDI region as a clipping region */
        SelectClipRgn(pScreenPriv->hdcScreen, hrgnCombined);
        DeleteObject(hrgnCombined);
        hrgnCombined = NULL;

        /*
         * Blit the shadow buffer to the screen,
         * constrained to the clipping region.
         */
        BitBlt(pScreenPriv->hdcScreen,
               pBoxExtents->x1, pBoxExtents->y1,
               pBoxExtents->x2 - pBoxExtents->x1,
               pBoxExtents->y2 - pBoxExtents->y1,
               pScreenPriv->hdcShadow,
               pBoxExtents->x1, pBoxExtents->y1, SRCCOPY);

        /* Reset the clip region */
        SelectClipRgn(pScreenPriv->hdcScreen, NULL);
    }

    /* Redraw all multiwindow windows */
    if (pScreenInfo->fMultiWindow)
        EnumThreadWindows(g_dwCurrentThreadID,
                          winRedrawDamagedWindowShadowGDI,
                          (LPARAM) pBoxExtents);
}

static Bool
winInitScreenShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);

    /* Get device contexts for the screen and shadow bitmap */
    pScreenPriv->hdcScreen = GetDC(pScreenPriv->hwndScreen);
    pScreenPriv->hdcShadow = CreateCompatibleDC(pScreenPriv->hdcScreen);

    /* Allocate bitmap info header */
    pScreenPriv->pbmih = malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    if (pScreenPriv->pbmih == NULL) {
        ErrorF("winInitScreenShadowGDI - malloc () failed\n");
        return FALSE;
    }

    /* Query the screen format */
    if (!winQueryScreenDIBFormat(pScreen, pScreenPriv->pbmih)) {
        ErrorF("winInitScreenShadowGDI - winQueryScreenDIBFormat failed\n");
        return FALSE;
    }

    /* Determine our color masks */
    if (!winQueryRGBBitsAndMasks(pScreen)) {
        ErrorF("winInitScreenShadowGDI - winQueryRGBBitsAndMasks failed\n");
        return FALSE;
    }

    return winAllocateFBShadowGDI(pScreen);
}

/* See Porting Layer Definition - p. 33 */
/*
 * We wrap whatever CloseScreen procedure was specified by fb;
 * a pointer to said procedure is stored in our privates.
 */

static Bool
winCloseScreenShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    Bool fReturn = TRUE;

#if CYGDEBUG
    winDebug("winCloseScreenShadowGDI - Freeing screen resources\n");
#endif

    /* Flag that the screen is closed */
    pScreenPriv->fClosed = TRUE;
    pScreenPriv->fActive = FALSE;

    /* Call the wrapped CloseScreen procedure */
    WIN_UNWRAP(CloseScreen);
    if (pScreen->CloseScreen)
        fReturn = (*pScreen->CloseScreen) (pScreen);

    /* Delete the window property */
    RemoveProp(pScreenPriv->hwndScreen, WIN_SCR_PROP);

    /* Free the shadow DC; which allows the bitmap to be freed */
    DeleteDC(pScreenPriv->hdcShadow);

    winFreeFBShadowGDI(pScreen);

    /* Free the screen DC */
    ReleaseDC(pScreenPriv->hwndScreen, pScreenPriv->hdcScreen);

    /* Delete tray icon, if we have one */
    if (!pScreenInfo->fNoTrayIcon)
        winDeleteNotifyIcon(pScreenPriv);

    /* Free the exit confirmation dialog box, if it exists */
    if (g_hDlgExit != NULL) {
        DestroyWindow(g_hDlgExit);
        g_hDlgExit = NULL;
    }

    /* Kill our window */
    if (pScreenPriv->hwndScreen) {
        DestroyWindow(pScreenPriv->hwndScreen);
        pScreenPriv->hwndScreen = NULL;
    }

    /* Destroy the thread startup mutex */
    pthread_mutex_destroy(&pScreenPriv->pmServerStarted);

    /* Invalidate our screeninfo's pointer to the screen */
    pScreenInfo->pScreen = NULL;

    /* Free the screen privates for this screen */
    free((void *) pScreenPriv);

    return fReturn;
}

/*
 * Tell mi what sort of visuals we need.
 *
 * Generally we only need one visual, as our screen can only
 * handle one format at a time, I believe.  You may want
 * to verify that last sentence.
 */

static Bool
winInitVisualsShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    /* Display debugging information */
    ErrorF("winInitVisualsShadowGDI - Masks %08x %08x %08x BPRGB %d d %d "
           "bpp %d\n",
           (unsigned int) pScreenPriv->dwRedMask,
           (unsigned int) pScreenPriv->dwGreenMask,
           (unsigned int) pScreenPriv->dwBlueMask,
           (int) pScreenPriv->dwBitsPerRGB,
           (int) pScreenInfo->dwDepth, (int) pScreenInfo->dwBPP);

    /* Create a single visual according to the Windows screen depth */
    switch (pScreenInfo->dwDepth) {
    case 24:
    case 16:
    case 15:
        /* Setup the real visual */
        if (!miSetVisualTypesAndMasks(pScreenInfo->dwDepth,
                                      TrueColorMask,
                                      pScreenPriv->dwBitsPerRGB,
                                      -1,
                                      pScreenPriv->dwRedMask,
                                      pScreenPriv->dwGreenMask,
                                      pScreenPriv->dwBlueMask)) {
            ErrorF("winInitVisualsShadowGDI - miSetVisualTypesAndMasks "
                   "failed\n");
            return FALSE;
        }

#ifdef XWIN_EMULATEPSEUDO
        if (!pScreenInfo->fEmulatePseudo)
            break;

        /* Setup a pseudocolor visual */
        if (!miSetVisualTypesAndMasks(8, PseudoColorMask, 8, -1, 0, 0, 0)) {
            ErrorF("winInitVisualsShadowGDI - miSetVisualTypesAndMasks "
                   "failed for PseudoColor\n");
            return FALSE;
        }
#endif
        break;

    case 8:
        if (!miSetVisualTypesAndMasks(pScreenInfo->dwDepth,
                                      PseudoColorMask,
                                      pScreenPriv->dwBitsPerRGB,
                                      PseudoColor,
                                      pScreenPriv->dwRedMask,
                                      pScreenPriv->dwGreenMask,
                                      pScreenPriv->dwBlueMask)) {
            ErrorF("winInitVisualsShadowGDI - miSetVisualTypesAndMasks "
                   "failed\n");
            return FALSE;
        }
        break;

    default:
        ErrorF("winInitVisualsShadowGDI - Unknown screen depth\n");
        return FALSE;
    }

#if CYGDEBUG
    winDebug("winInitVisualsShadowGDI - Returning\n");
#endif

    return TRUE;
}

/*
 * Adjust the proposed video mode
 */

static Bool
winAdjustVideoModeShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    HDC hdc;
    DWORD dwBPP;

    hdc = GetDC(NULL);

    /* We're in serious trouble if we can't get a DC */
    if (hdc == NULL) {
        ErrorF("winAdjustVideoModeShadowGDI - GetDC () failed\n");
        return FALSE;
    }

    /* Query GDI for current display depth */
    dwBPP = GetDeviceCaps(hdc, BITSPIXEL);

    /* GDI cannot change the screen depth, so always use GDI's depth */
    pScreenInfo->dwBPP = dwBPP;

    /* Release our DC */
    ReleaseDC(NULL, hdc);
    hdc = NULL;

    return TRUE;
}

/*
 * Blt exposed regions to the screen
 */

static Bool
winBltExposedRegionsShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    winPrivCmapPtr pCmapPriv = NULL;
    HDC hdcUpdate;
    PAINTSTRUCT ps;

    /* BeginPaint gives us an hdc that clips to the invalidated region */
    hdcUpdate = BeginPaint(pScreenPriv->hwndScreen, &ps);
    /* Avoid the BitBlt if the PAINTSTRUCT region is bogus */
    if (ps.rcPaint.right == 0 && ps.rcPaint.bottom == 0 &&
        ps.rcPaint.left == 0 && ps.rcPaint.top == 0) {
        EndPaint(pScreenPriv->hwndScreen, &ps);
        return 0;
    }

    /* Realize the palette, if we have one */
    if (pScreenPriv->pcmapInstalled != NULL) {
        pCmapPriv = winGetCmapPriv(pScreenPriv->pcmapInstalled);

        SelectPalette(hdcUpdate, pCmapPriv->hPalette, FALSE);
        RealizePalette(hdcUpdate);
    }

    /* Try to copy from the shadow buffer to the invalidated region */
    if (!BitBlt(hdcUpdate,
                ps.rcPaint.left, ps.rcPaint.top,
                ps.rcPaint.right - ps.rcPaint.left,
                ps.rcPaint.bottom - ps.rcPaint.top,
                pScreenPriv->hdcShadow,
                ps.rcPaint.left,
                ps.rcPaint.top,
                SRCCOPY)) {
        LPVOID lpMsgBuf;

        /* Display an error message */
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf, 0, NULL);

        ErrorF("winBltExposedRegionsShadowGDI - BitBlt failed: %s\n",
               (LPSTR) lpMsgBuf);
        LocalFree(lpMsgBuf);
    }

    /* EndPaint frees the DC */
    EndPaint(pScreenPriv->hwndScreen, &ps);

    /* Redraw all windows */
    if (pScreenInfo->fMultiWindow)
        EnumThreadWindows(g_dwCurrentThreadID, winRedrawAllProcShadowGDI,
                          (LPARAM) pScreenPriv->hwndScreen);

    return TRUE;
}

/*
 * Blt exposed region to the given HWND
 */

static Bool
winBltExposedWindowRegionShadowGDI(ScreenPtr pScreen, WindowPtr pWin)
{
    winScreenPriv(pScreen);
    winPrivWinPtr pWinPriv = winGetWindowPriv(pWin);

    HWND hWnd = pWinPriv->hWnd;
    HDC hdcUpdate;
    PAINTSTRUCT ps;

    hdcUpdate = BeginPaint(hWnd, &ps);
    /* Avoid the BitBlt if the PAINTSTRUCT region is bogus */
    if (ps.rcPaint.right == 0 && ps.rcPaint.bottom == 0 &&
        ps.rcPaint.left == 0 && ps.rcPaint.top == 0) {
        EndPaint(hWnd, &ps);
        return 0;
    }

#ifdef COMPOSITE
    if (pWin->redirectDraw != RedirectDrawNone) {
        HBITMAP hBitmap;
        HDC hdcPixmap;
        PixmapPtr pPixmap = (*pScreen->GetWindowPixmap) (pWin);
        winPrivPixmapPtr pPixmapPriv = winGetPixmapPriv(pPixmap);

        /* window pixmap format is the same as the screen pixmap */
        assert(pPixmap->drawable.bitsPerPixel > 8);

        /* Get the window bitmap from the pixmap */
        hBitmap = pPixmapPriv->hBitmap;

        /* XXX: There may be a need for a slow-path here: If hBitmap is NULL
           (because we couldn't back the pixmap with a Windows DIB), we should
           fall-back to creating a Windows DIB from the pixmap, then deleting it
           after the BitBlt (as this this code did before the fast-path was
           added). */
        if (!hBitmap) {
            ErrorF("winBltExposedWindowRegionShadowGDI - slow path unimplemented\n");
        }

        /* Select the window bitmap into a screen-compatible DC */
        hdcPixmap = CreateCompatibleDC(pScreenPriv->hdcScreen);
        SelectObject(hdcPixmap, hBitmap);

        /* Blt from the window bitmap to the invalidated region */
        if (!BitBlt(hdcUpdate,
                    ps.rcPaint.left, ps.rcPaint.top,
                    ps.rcPaint.right - ps.rcPaint.left,
                    ps.rcPaint.bottom - ps.rcPaint.top,
                    hdcPixmap,
                    ps.rcPaint.left + pWin->borderWidth,
                    ps.rcPaint.top + pWin->borderWidth,
                    SRCCOPY))
            ErrorF("winBltExposedWindowRegionShadowGDI - BitBlt failed: 0x%08x\n",
                   GetLastError());

        /* Release DC */
        DeleteDC(hdcPixmap);
    }
    else
#endif
    {
    /* Try to copy from the shadow buffer to the invalidated region */
    if (!BitBlt(hdcUpdate,
                ps.rcPaint.left, ps.rcPaint.top,
                ps.rcPaint.right - ps.rcPaint.left,
                ps.rcPaint.bottom - ps.rcPaint.top,
                pScreenPriv->hdcShadow,
                ps.rcPaint.left + pWin->drawable.x,
                ps.rcPaint.top + pWin->drawable.y,
                SRCCOPY)) {
        LPVOID lpMsgBuf;

        /* Display an error message */
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf, 0, NULL);

        ErrorF("winBltExposedWindowRegionShadowGDI - BitBlt failed: %s\n",
               (LPSTR) lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
    }

    /* If part of the invalidated region is outside the window (which can happen
       if the native window is being re-sized), fill that area with black */
    if (ps.rcPaint.right > ps.rcPaint.left + pWin->drawable.width) {
        BitBlt(hdcUpdate,
               ps.rcPaint.left + pWin->drawable.width,
               ps.rcPaint.top,
               ps.rcPaint.right - (ps.rcPaint.left + pWin->drawable.width),
               ps.rcPaint.bottom - ps.rcPaint.top,
               NULL,
               0, 0,
               BLACKNESS);
    }

    if (ps.rcPaint.bottom > ps.rcPaint.top + pWin->drawable.height) {
        BitBlt(hdcUpdate,
               ps.rcPaint.left,
               ps.rcPaint.top + pWin->drawable.height,
               ps.rcPaint.right - ps.rcPaint.left,
               ps.rcPaint.bottom - (ps.rcPaint.top + pWin->drawable.height),
               NULL,
               0, 0,
               BLACKNESS);
    }

    /* EndPaint frees the DC */
    EndPaint(hWnd, &ps);

    return TRUE;
}

/*
 * Do any engine-specific appliation-activation processing
 */

static Bool
winActivateAppShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    /*
     * 2004/04/12 - Harold - We perform the restoring or minimizing
     * manually for ShadowGDI in fullscreen modes so that this engine
     * will perform just like ShadowDD and ShadowDDNL in fullscreen mode;
     * if we do not do this then our fullscreen window will appear in the
     * z-order when it is deactivated and it can be uncovered by resizing
     * or minimizing another window that is on top of it, which is not how
     * the DirectDraw engines work.  Therefore we keep this code here to
     * make sure that all engines work the same in fullscreen mode.
     */

    /*
     * Are we active?
     * Are we fullscreen?
     */
    if (pScreenPriv->fActive && pScreenInfo->fFullScreen) {
        /*
         * Activating, attempt to bring our window
         * to the top of the display
         */
        ShowWindow(pScreenPriv->hwndScreen, SW_RESTORE);
    }
    else if (!pScreenPriv->fActive && pScreenInfo->fFullScreen) {
        /*
         * Deactivating, stuff our window onto the
         * task bar.
         */
        ShowWindow(pScreenPriv->hwndScreen, SW_MINIMIZE);
    }

    return TRUE;
}

/*
 * Reblit the shadow framebuffer to the screen.
 */

static Bool
winRedrawScreenShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    /* Redraw the whole window, to take account for the new colors */
    BitBlt(pScreenPriv->hdcScreen,
           0, 0,
           pScreenInfo->dwWidth, pScreenInfo->dwHeight,
           pScreenPriv->hdcShadow, 0, 0, SRCCOPY);

    /* Redraw all windows */
    if (pScreenInfo->fMultiWindow)
        EnumThreadWindows(g_dwCurrentThreadID, winRedrawAllProcShadowGDI, 0);

    return TRUE;
}

/*
 * Realize the currently installed colormap
 */

static Bool
winRealizeInstalledPaletteShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winPrivCmapPtr pCmapPriv = NULL;

#if CYGDEBUG
    winDebug("winRealizeInstalledPaletteShadowGDI\n");
#endif

    /* Don't do anything if there is not a colormap */
    if (pScreenPriv->pcmapInstalled == NULL) {
#if CYGDEBUG
        winDebug("winRealizeInstalledPaletteShadowGDI - No colormap "
                 "installed\n");
#endif
        return TRUE;
    }

    pCmapPriv = winGetCmapPriv(pScreenPriv->pcmapInstalled);

    /* Realize our palette for the screen */
    if (RealizePalette(pScreenPriv->hdcScreen) == GDI_ERROR) {
        ErrorF("winRealizeInstalledPaletteShadowGDI - RealizePalette () "
               "failed\n");
        return FALSE;
    }

    /* Set the DIB color table */
    if (SetDIBColorTable(pScreenPriv->hdcShadow,
                         0,
                         WIN_NUM_PALETTE_ENTRIES, pCmapPriv->rgbColors) == 0) {
        ErrorF("winRealizeInstalledPaletteShadowGDI - SetDIBColorTable () "
               "failed\n");
        return FALSE;
    }

    return TRUE;
}

/*
 * Install the specified colormap
 */

static Bool
winInstallColormapShadowGDI(ColormapPtr pColormap)
{
    ScreenPtr pScreen = pColormap->pScreen;

    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    winCmapPriv(pColormap);

    /*
     * Tell Windows to install the new colormap
     */
    if (SelectPalette(pScreenPriv->hdcScreen,
                      pCmapPriv->hPalette, FALSE) == NULL) {
        ErrorF("winInstallColormapShadowGDI - SelectPalette () failed\n");
        return FALSE;
    }

    /* Realize the palette */
    if (GDI_ERROR == RealizePalette(pScreenPriv->hdcScreen)) {
        ErrorF("winInstallColormapShadowGDI - RealizePalette () failed\n");
        return FALSE;
    }

    /* Set the DIB color table */
    if (SetDIBColorTable(pScreenPriv->hdcShadow,
                         0,
                         WIN_NUM_PALETTE_ENTRIES, pCmapPriv->rgbColors) == 0) {
        ErrorF("winInstallColormapShadowGDI - SetDIBColorTable () failed\n");
        return FALSE;
    }

    /* Redraw the whole window, to take account for the new colors */
    BitBlt(pScreenPriv->hdcScreen,
           0, 0,
           pScreenInfo->dwWidth, pScreenInfo->dwHeight,
           pScreenPriv->hdcShadow, 0, 0, SRCCOPY);

    /* Save a pointer to the newly installed colormap */
    pScreenPriv->pcmapInstalled = pColormap;

    /* Redraw all windows */
    if (pScreenInfo->fMultiWindow)
        EnumThreadWindows(g_dwCurrentThreadID, winRedrawAllProcShadowGDI, 0);

    return TRUE;
}

/*
 * Store the specified colors in the specified colormap
 */

static Bool
winStoreColorsShadowGDI(ColormapPtr pColormap, int ndef, xColorItem * pdefs)
{
    ScreenPtr pScreen = pColormap->pScreen;

    winScreenPriv(pScreen);
    winCmapPriv(pColormap);
    ColormapPtr curpmap = pScreenPriv->pcmapInstalled;

    /* Put the X colormap entries into the Windows logical palette */
    if (SetPaletteEntries(pCmapPriv->hPalette,
                          pdefs[0].pixel,
                          ndef, pCmapPriv->peColors + pdefs[0].pixel) == 0) {
        ErrorF("winStoreColorsShadowGDI - SetPaletteEntries () failed\n");
        return FALSE;
    }

    /* Don't install the Windows palette if the colormap is not installed */
    if (pColormap != curpmap) {
        return TRUE;
    }

    /* Try to install the newly modified colormap */
    if (!winInstallColormapShadowGDI(pColormap)) {
        ErrorF("winInstallColormapShadowGDI - winInstallColormapShadowGDI "
               "failed\n");
        return FALSE;
    }

#if 0
    /* Tell Windows that the palette has changed */
    RealizePalette(pScreenPriv->hdcScreen);

    /* Set the DIB color table */
    if (SetDIBColorTable(pScreenPriv->hdcShadow,
                         pdefs[0].pixel,
                         ndef, pCmapPriv->rgbColors + pdefs[0].pixel) == 0) {
        ErrorF("winInstallColormapShadowGDI - SetDIBColorTable () failed\n");
        return FALSE;
    }

    /* Save a pointer to the newly installed colormap */
    pScreenPriv->pcmapInstalled = pColormap;
#endif

    return TRUE;
}

/*
 * Colormap initialization procedure
 */

static Bool
winCreateColormapShadowGDI(ColormapPtr pColormap)
{
    LPLOGPALETTE lpPaletteNew = NULL;
    DWORD dwEntriesMax;
    VisualPtr pVisual;
    HPALETTE hpalNew = NULL;

    winCmapPriv(pColormap);

    /* Get a pointer to the visual that the colormap belongs to */
    pVisual = pColormap->pVisual;

    /* Get the maximum number of palette entries for this visual */
    dwEntriesMax = pVisual->ColormapEntries;

    /* Allocate a Windows logical color palette with max entries */
    lpPaletteNew = malloc(sizeof(LOGPALETTE)
                          + (dwEntriesMax - 1) * sizeof(PALETTEENTRY));
    if (lpPaletteNew == NULL) {
        ErrorF("winCreateColormapShadowGDI - Couldn't allocate palette "
               "with %d entries\n", (int) dwEntriesMax);
        return FALSE;
    }

    /* Zero out the colormap */
    ZeroMemory(lpPaletteNew, sizeof(LOGPALETTE)
               + (dwEntriesMax - 1) * sizeof(PALETTEENTRY));

    /* Set the logical palette structure */
    lpPaletteNew->palVersion = 0x0300;
    lpPaletteNew->palNumEntries = dwEntriesMax;

    /* Tell Windows to create the palette */
    hpalNew = CreatePalette(lpPaletteNew);
    if (hpalNew == NULL) {
        ErrorF("winCreateColormapShadowGDI - CreatePalette () failed\n");
        free(lpPaletteNew);
        return FALSE;
    }

    /* Save the Windows logical palette handle in the X colormaps' privates */
    pCmapPriv->hPalette = hpalNew;

    /* Free the palette initialization memory */
    free(lpPaletteNew);

    return TRUE;
}

/*
 * Colormap destruction procedure
 */

static Bool
winDestroyColormapShadowGDI(ColormapPtr pColormap)
{
    winScreenPriv(pColormap->pScreen);
    winCmapPriv(pColormap);

    /*
     * Is colormap to be destroyed the default?
     *
     * Non-default colormaps should have had winUninstallColormap
     * called on them before we get here.  The default colormap
     * will not have had winUninstallColormap called on it.  Thus,
     * we need to handle the default colormap in a special way.
     */
    if (pColormap->flags & IsDefault) {
#if CYGDEBUG
        winDebug("winDestroyColormapShadowGDI - Destroying default "
                 "colormap\n");
#endif

        /*
         * FIXME: Walk the list of all screens, popping the default
         * palette out of each screen device context.
         */

        /* Pop the palette out of the device context */
        SelectPalette(pScreenPriv->hdcScreen,
                      GetStockObject(DEFAULT_PALETTE), FALSE);

        /* Clear our private installed colormap pointer */
        pScreenPriv->pcmapInstalled = NULL;
    }

    /* Try to delete the logical palette */
    if (DeleteObject(pCmapPriv->hPalette) == 0) {
        ErrorF("winDestroyColormap - DeleteObject () failed\n");
        return FALSE;
    }

    /* Invalidate the colormap privates */
    pCmapPriv->hPalette = NULL;

    return TRUE;
}

/*
 * Set engine specific functions
 */

Bool
winSetEngineFunctionsShadowGDI(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    /* Set our pointers */
    pScreenPriv->pwinAllocateFB = winAllocateFBShadowGDI;
    pScreenPriv->pwinFreeFB = winFreeFBShadowGDI;
    pScreenPriv->pwinShadowUpdate = winShadowUpdateGDI;
    pScreenPriv->pwinInitScreen = winInitScreenShadowGDI;
    pScreenPriv->pwinCloseScreen = winCloseScreenShadowGDI;
    pScreenPriv->pwinInitVisuals = winInitVisualsShadowGDI;
    pScreenPriv->pwinAdjustVideoMode = winAdjustVideoModeShadowGDI;
    if (pScreenInfo->fFullScreen)
        pScreenPriv->pwinCreateBoundingWindow =
            winCreateBoundingWindowFullScreen;
    else
        pScreenPriv->pwinCreateBoundingWindow = winCreateBoundingWindowWindowed;
    pScreenPriv->pwinFinishScreenInit = winFinishScreenInitFB;
    pScreenPriv->pwinBltExposedRegions = winBltExposedRegionsShadowGDI;
    pScreenPriv->pwinBltExposedWindowRegion = winBltExposedWindowRegionShadowGDI;
    pScreenPriv->pwinActivateApp = winActivateAppShadowGDI;
    pScreenPriv->pwinRedrawScreen = winRedrawScreenShadowGDI;
    pScreenPriv->pwinRealizeInstalledPalette =
        winRealizeInstalledPaletteShadowGDI;
    pScreenPriv->pwinInstallColormap = winInstallColormapShadowGDI;
    pScreenPriv->pwinStoreColors = winStoreColorsShadowGDI;
    pScreenPriv->pwinCreateColormap = winCreateColormapShadowGDI;
    pScreenPriv->pwinDestroyColormap = winDestroyColormapShadowGDI;
    pScreenPriv->pwinCreatePrimarySurface = NULL;
    pScreenPriv->pwinReleasePrimarySurface = NULL;

    return TRUE;
}
