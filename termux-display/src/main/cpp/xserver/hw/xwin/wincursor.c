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
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "winmsg.h"
#include <cursorstr.h>
#include <mipointrst.h>
#include <servermd.h>
#include "misc.h"

#define BRIGHTNESS(x) (x##Red * 0.299 + x##Green * 0.587 + x##Blue * 0.114)

#if 0
#define WIN_DEBUG_MSG winDebug
#else
#define WIN_DEBUG_MSG(...)
#endif

/*
 * Local function prototypes
 */

static void
 winPointerWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y);

static Bool
 winCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y);

static void
 winCrossScreen(ScreenPtr pScreen, Bool fEntering);

miPointerScreenFuncRec g_winPointerCursorFuncs = {
    winCursorOffScreen,
    winCrossScreen,
    winPointerWarpCursor
};

static void
winPointerWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    winScreenPriv(pScreen);
    RECT rcClient;
    static Bool s_fInitialWarp = TRUE;

    /* Discard first warp call */
    if (s_fInitialWarp) {
        /* First warp moves mouse to center of window, just ignore it */

        /* Don't ignore subsequent warps */
        s_fInitialWarp = FALSE;

        winErrorFVerb(2,
                      "winPointerWarpCursor - Discarding first warp: %d %d\n",
                      x, y);

        return;
    }

    /*
       Only update the Windows cursor position if root window is active,
       or we are in a rootless mode
     */
    if ((pScreenPriv->hwndScreen == GetForegroundWindow())
        || pScreenPriv->pScreenInfo->fRootless
        || pScreenPriv->pScreenInfo->fMultiWindow
        ) {
        /* Get the client area coordinates */
        GetClientRect(pScreenPriv->hwndScreen, &rcClient);

        /* Translate the client area coords to screen coords */
        MapWindowPoints(pScreenPriv->hwndScreen,
                        HWND_DESKTOP, (LPPOINT) &rcClient, 2);

        /*
         * Update the Windows cursor position so that we don't
         * immediately warp back to the current position.
         */
        SetCursorPos(rcClient.left + x, rcClient.top + y);
    }

    /* Call the mi warp procedure to do the actual warping in X. */
    miPointerWarpCursor(pDev, pScreen, x, y);
}

static Bool
winCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
winCrossScreen(ScreenPtr pScreen, Bool fEntering)
{
}

static unsigned char
reverse(unsigned char c)
{
    int i;
    unsigned char ret = 0;

    for (i = 0; i < 8; ++i) {
        ret |= ((c >> i) & 1) << (7 - i);
    }
    return ret;
}

/*
 * Convert X cursor to Windows cursor
 * FIXME: Perhaps there are more smart code
 */
static HCURSOR
winLoadCursor(ScreenPtr pScreen, CursorPtr pCursor, int screen)
{
    winScreenPriv(pScreen);
    HCURSOR hCursor = NULL;
    unsigned char *pAnd;
    unsigned char *pXor;
    int nCX, nCY;
    int nBytes;
    double dForeY, dBackY;
    BOOL fReverse;
    HBITMAP hAnd, hXor;
    ICONINFO ii;
    unsigned char *pCur;
    unsigned char bit;
    HDC hDC;
    BITMAPV4HEADER bi;
    BITMAPINFO *pbmi;
    uint32_t *lpBits;

    WIN_DEBUG_MSG("winLoadCursor: Win32: %dx%d X11: %dx%d hotspot: %d,%d\n",
                  pScreenPriv->cursor.sm_cx, pScreenPriv->cursor.sm_cy,
                  pCursor->bits->width, pCursor->bits->height,
                  pCursor->bits->xhot, pCursor->bits->yhot);

    /* We can use only White and Black, so calc brightness of color
     * Also check if the cursor is inverted */
    dForeY = BRIGHTNESS(pCursor->fore);
    dBackY = BRIGHTNESS(pCursor->back);
    fReverse = dForeY < dBackY;

    /* Check whether the X11 cursor is bigger than the win32 cursor */
    if (pScreenPriv->cursor.sm_cx < pCursor->bits->width ||
        pScreenPriv->cursor.sm_cy < pCursor->bits->height) {
        winErrorFVerb(3,
                      "winLoadCursor - Windows requires %dx%d cursor but X requires %dx%d\n",
                      pScreenPriv->cursor.sm_cx, pScreenPriv->cursor.sm_cy,
                      pCursor->bits->width, pCursor->bits->height);
    }

    /* Get the number of bytes required to store the whole cursor image
     * This is roughly (sm_cx * sm_cy) / 8
     * round up to 8 pixel boundary so we can convert whole bytes */
    nBytes =
        bits_to_bytes(pScreenPriv->cursor.sm_cx) * pScreenPriv->cursor.sm_cy;

    /* Get the effective width and height */
    nCX = min(pScreenPriv->cursor.sm_cx, pCursor->bits->width);
    nCY = min(pScreenPriv->cursor.sm_cy, pCursor->bits->height);

    /* Allocate memory for the bitmaps */
    pAnd = malloc(nBytes);
    memset(pAnd, 0xFF, nBytes);
    pXor = calloc(1, nBytes);

    /* Convert the X11 bitmap to a win32 bitmap
     * The first is for an empty mask */
    if (pCursor->bits->emptyMask) {
        int x, y, xmax = bits_to_bytes(nCX);

        for (y = 0; y < nCY; ++y)
            for (x = 0; x < xmax; ++x) {
                int nWinPix = bits_to_bytes(pScreenPriv->cursor.sm_cx) * y + x;
                int nXPix = BitmapBytePad(pCursor->bits->width) * y + x;

                pAnd[nWinPix] = 0;
                if (fReverse)
                    pXor[nWinPix] = reverse(~pCursor->bits->source[nXPix]);
                else
                    pXor[nWinPix] = reverse(pCursor->bits->source[nXPix]);
            }
    }
    else {
        int x, y, xmax = bits_to_bytes(nCX);

        for (y = 0; y < nCY; ++y)
            for (x = 0; x < xmax; ++x) {
                int nWinPix = bits_to_bytes(pScreenPriv->cursor.sm_cx) * y + x;
                int nXPix = BitmapBytePad(pCursor->bits->width) * y + x;

                unsigned char mask = pCursor->bits->mask[nXPix];

                pAnd[nWinPix] = reverse(~mask);
                if (fReverse)
                    pXor[nWinPix] =
                        reverse(~pCursor->bits->source[nXPix] & mask);
                else
                    pXor[nWinPix] =
                        reverse(pCursor->bits->source[nXPix] & mask);
            }
    }

    /* prepare the pointers */
    hCursor = NULL;
    lpBits = NULL;

    /* We have a truecolor alpha-blended cursor and can use it! */
    if (pCursor->bits->argb) {
        WIN_DEBUG_MSG("winLoadCursor: Trying truecolor alphablended cursor\n");
        memset(&bi, 0, sizeof(BITMAPV4HEADER));
        bi.bV4Size = sizeof(BITMAPV4HEADER);
        bi.bV4Width = pScreenPriv->cursor.sm_cx;
        bi.bV4Height = -(pScreenPriv->cursor.sm_cy);    /* right-side up */
        bi.bV4Planes = 1;
        bi.bV4BitCount = 32;
        bi.bV4V4Compression = BI_BITFIELDS;
        bi.bV4RedMask = 0x00FF0000;
        bi.bV4GreenMask = 0x0000FF00;
        bi.bV4BlueMask = 0x000000FF;
        bi.bV4AlphaMask = 0xFF000000;

        lpBits = calloc(pScreenPriv->cursor.sm_cx * pScreenPriv->cursor.sm_cy,
                        sizeof(uint32_t));

        if (lpBits) {
            int y;
            for (y = 0; y < nCY; y++) {
                void *src, *dst;
                src = &(pCursor->bits->argb[y * pCursor->bits->width]);
                dst = &(lpBits[y * pScreenPriv->cursor.sm_cx]);
                memcpy(dst, src, 4 * nCX);
            }
        }
    }                           /* End if-truecolor-icon */

    if (!lpBits) {
        RGBQUAD *pbmiColors;
        /* Bicolor, use a palettized DIB */
        WIN_DEBUG_MSG("winLoadCursor: Trying two color cursor\n");
        pbmi = (BITMAPINFO *) &bi;
        pbmiColors = &(pbmi->bmiColors[0]);

        memset(pbmi, 0, sizeof(BITMAPINFOHEADER));
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth = pScreenPriv->cursor.sm_cx;
        pbmi->bmiHeader.biHeight = -abs(pScreenPriv->cursor.sm_cy);     /* right-side up */
        pbmi->bmiHeader.biPlanes = 1;
        pbmi->bmiHeader.biBitCount = 8;
        pbmi->bmiHeader.biCompression = BI_RGB;
        pbmi->bmiHeader.biSizeImage = 0;
        pbmi->bmiHeader.biClrUsed = 3;
        pbmi->bmiHeader.biClrImportant = 3;

        pbmiColors[0].rgbRed = 0;  /* Empty */
        pbmiColors[0].rgbGreen = 0;
        pbmiColors[0].rgbBlue = 0;
        pbmiColors[0].rgbReserved = 0;
        pbmiColors[1].rgbRed = pCursor->backRed >> 8;      /* Background */
        pbmiColors[1].rgbGreen = pCursor->backGreen >> 8;
        pbmiColors[1].rgbBlue = pCursor->backBlue >> 8;
        pbmiColors[1].rgbReserved = 0;
        pbmiColors[2].rgbRed = pCursor->foreRed >> 8;      /* Foreground */
        pbmiColors[2].rgbGreen = pCursor->foreGreen >> 8;
        pbmiColors[2].rgbBlue = pCursor->foreBlue >> 8;
        pbmiColors[2].rgbReserved = 0;

        lpBits = calloc(pScreenPriv->cursor.sm_cx * pScreenPriv->cursor.sm_cy, 1);

        pCur = (unsigned char *) lpBits;
        if (lpBits) {
	    int x, y;
            for (y = 0; y < pScreenPriv->cursor.sm_cy; y++) {
                for (x = 0; x < pScreenPriv->cursor.sm_cx; x++) {
                    if (x >= nCX || y >= nCY)   /* Outside of X11 icon bounds */
                        (*pCur++) = 0;
                    else {      /* Within X11 icon bounds */

                        int nWinPix =
                            bits_to_bytes(pScreenPriv->cursor.sm_cx) * y +
                            (x / 8);

                        bit = pAnd[nWinPix];
                        bit = bit & (1 << (7 - (x & 7)));
                        if (!bit) {     /* Within the cursor mask? */
                            int nXPix =
                                BitmapBytePad(pCursor->bits->width) * y +
                                (x / 8);
                            bit =
                                ~reverse(~pCursor->bits->
                                         source[nXPix] & pCursor->bits->
                                         mask[nXPix]);
                            bit = bit & (1 << (7 - (x & 7)));
                            if (bit)    /* Draw foreground */
                                (*pCur++) = 2;
                            else        /* Draw background */
                                (*pCur++) = 1;
                        }
                        else    /* Outside the cursor mask */
                            (*pCur++) = 0;
                    }
                }               /* end for (x) */
            }                   /* end for (y) */
        }                       /* end if (lpbits) */
    }

    /* If one of the previous two methods gave us the bitmap we need, make a cursor */
    if (lpBits) {
        WIN_DEBUG_MSG("winLoadCursor: Creating bitmap cursor: hotspot %d,%d\n",
                      pCursor->bits->xhot, pCursor->bits->yhot);

        hAnd = NULL;
        hXor = NULL;

        hAnd =
            CreateBitmap(pScreenPriv->cursor.sm_cx, pScreenPriv->cursor.sm_cy,
                         1, 1, pAnd);

        hDC = GetDC(NULL);
        if (hDC) {
            hXor =
                CreateCompatibleBitmap(hDC, pScreenPriv->cursor.sm_cx,
                                       pScreenPriv->cursor.sm_cy);
            SetDIBits(hDC, hXor, 0, pScreenPriv->cursor.sm_cy, lpBits,
                      (BITMAPINFO *) &bi, DIB_RGB_COLORS);
            ReleaseDC(NULL, hDC);
        }
        free(lpBits);

        if (hAnd && hXor) {
            ii.fIcon = FALSE;
            ii.xHotspot = pCursor->bits->xhot;
            ii.yHotspot = pCursor->bits->yhot;
            ii.hbmMask = hAnd;
            ii.hbmColor = hXor;
            hCursor = (HCURSOR) CreateIconIndirect(&ii);

            if (hCursor == NULL)
                winW32Error(2, "winLoadCursor - CreateIconIndirect failed:");
            else {
                if (GetIconInfo(hCursor, &ii)) {
                    if (ii.fIcon) {
                        WIN_DEBUG_MSG
                            ("winLoadCursor: CreateIconIndirect returned  no cursor. Trying again.\n");

                        DestroyCursor(hCursor);

                        ii.fIcon = FALSE;
                        ii.xHotspot = pCursor->bits->xhot;
                        ii.yHotspot = pCursor->bits->yhot;
                        hCursor = (HCURSOR) CreateIconIndirect(&ii);

                        if (hCursor == NULL)
                            winW32Error(2,
                                        "winLoadCursor - CreateIconIndirect failed:");
                    }
                    /* GetIconInfo creates new bitmaps. Destroy them again */
                    if (ii.hbmMask)
                        DeleteObject(ii.hbmMask);
                    if (ii.hbmColor)
                        DeleteObject(ii.hbmColor);
                }
            }
        }

        if (hAnd)
            DeleteObject(hAnd);
        if (hXor)
            DeleteObject(hXor);
    }

    if (!hCursor) {
        /* We couldn't make a color cursor for this screen, use
           black and white instead */
        hCursor = CreateCursor(g_hInstance,
                               pCursor->bits->xhot, pCursor->bits->yhot,
                               pScreenPriv->cursor.sm_cx,
                               pScreenPriv->cursor.sm_cy, pAnd, pXor);
        if (hCursor == NULL)
            winW32Error(2, "winLoadCursor - CreateCursor failed:");
    }
    free(pAnd);
    free(pXor);

    return hCursor;
}

/*
===========================================================================

 Pointer sprite functions

===========================================================================
*/

/*
 * winRealizeCursor
 *  Convert the X cursor representation to native format if possible.
 */
static Bool
winRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    if (pCursor == NULL || pCursor->bits == NULL)
        return FALSE;

    /* FIXME: cache ARGB8888 representation? */

    return TRUE;
}

/*
 * winUnrealizeCursor
 *  Free the storage space associated with a realized cursor.
 */
static Bool
winUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

/*
 * winSetCursor
 *  Set the cursor sprite and position.
 */
static void
winSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor, int x,
             int y)
{
    POINT ptCurPos, ptTemp;
    HWND hwnd;
    RECT rcClient;
    BOOL bInhibit;

    winScreenPriv(pScreen);
    WIN_DEBUG_MSG("winSetCursor: cursor=%p\n", pCursor);

    /* Inhibit changing the cursor if the mouse is not in a client area */
    bInhibit = FALSE;
    if (GetCursorPos(&ptCurPos)) {
        hwnd = WindowFromPoint(ptCurPos);
        if (hwnd) {
            if (GetClientRect(hwnd, &rcClient)) {
                ptTemp.x = rcClient.left;
                ptTemp.y = rcClient.top;
                if (ClientToScreen(hwnd, &ptTemp)) {
                    rcClient.left = ptTemp.x;
                    rcClient.top = ptTemp.y;
                    ptTemp.x = rcClient.right;
                    ptTemp.y = rcClient.bottom;
                    if (ClientToScreen(hwnd, &ptTemp)) {
                        rcClient.right = ptTemp.x;
                        rcClient.bottom = ptTemp.y;
                        if (!PtInRect(&rcClient, ptCurPos))
                            bInhibit = TRUE;
                    }
                }
            }
        }
    }

    if (pCursor == NULL) {
        if (pScreenPriv->cursor.visible) {
            if (!bInhibit && g_fSoftwareCursor)
                ShowCursor(FALSE);
            pScreenPriv->cursor.visible = FALSE;
        }
    }
    else {
        if (pScreenPriv->cursor.handle) {
            if (!bInhibit)
                SetCursor(NULL);
            DestroyCursor(pScreenPriv->cursor.handle);
            pScreenPriv->cursor.handle = NULL;
        }
        pScreenPriv->cursor.handle =
            winLoadCursor(pScreen, pCursor, pScreen->myNum);
        WIN_DEBUG_MSG("winSetCursor: handle=%p\n", pScreenPriv->cursor.handle);

        if (!bInhibit)
            SetCursor(pScreenPriv->cursor.handle);

        if (!pScreenPriv->cursor.visible) {
            if (!bInhibit && g_fSoftwareCursor)
                ShowCursor(TRUE);
            pScreenPriv->cursor.visible = TRUE;
        }
    }
}

/*
 * winMoveCursor
 *  Move the cursor. This is a noop for us.
 */
static void
winMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
}

static Bool
winDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScr)
{
    winScreenPriv(pScr);
    return pScreenPriv->cursor.spriteFuncs->DeviceCursorInitialize(pDev, pScr);
}

static void
winDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScr)
{
    winScreenPriv(pScr);
    pScreenPriv->cursor.spriteFuncs->DeviceCursorCleanup(pDev, pScr);
}

static miPointerSpriteFuncRec winSpriteFuncsRec = {
    winRealizeCursor,
    winUnrealizeCursor,
    winSetCursor,
    winMoveCursor,
    winDeviceCursorInitialize,
    winDeviceCursorCleanup
};

/*
===========================================================================

 Other screen functions

===========================================================================
*/

/*
 * winCursorQueryBestSize
 *  Handle queries for best cursor size
 */
static void
winCursorQueryBestSize(int class, unsigned short *width,
                       unsigned short *height, ScreenPtr pScreen)
{
    winScreenPriv(pScreen);

    if (class == CursorShape) {
        *width = pScreenPriv->cursor.sm_cx;
        *height = pScreenPriv->cursor.sm_cy;
    }
    else {
        if (pScreenPriv->cursor.QueryBestSize)
            (*pScreenPriv->cursor.QueryBestSize) (class, width, height,
                                                  pScreen);
    }
}

/*
 * winInitCursor
 *  Initialize cursor support
 */
Bool
winInitCursor(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    miPointerScreenPtr pPointPriv;

    /* override some screen procedures */
    pScreenPriv->cursor.QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = winCursorQueryBestSize;

    pPointPriv = (miPointerScreenPtr)
        dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

    pScreenPriv->cursor.spriteFuncs = pPointPriv->spriteFuncs;
    pPointPriv->spriteFuncs = &winSpriteFuncsRec;

    pScreenPriv->cursor.handle = NULL;
    pScreenPriv->cursor.visible = FALSE;

    pScreenPriv->cursor.sm_cx = GetSystemMetrics(SM_CXCURSOR);
    pScreenPriv->cursor.sm_cy = GetSystemMetrics(SM_CYCURSOR);

    return TRUE;
}
