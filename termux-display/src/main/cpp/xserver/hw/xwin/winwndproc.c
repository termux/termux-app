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
 *		MATSUZAKI Kensuke
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include <commctrl.h>
#include "winprefs.h"
#include "winconfig.h"
#include "winmsg.h"
#include "winmonitors.h"
#include "inputstr.h"
#include "winclipboard/winclipboard.h"

/*
 * Global variables
 */

Bool g_fCursor = TRUE;
Bool g_fButton[3] = { FALSE, FALSE, FALSE };

/*
 * Called by winWakeupHandler
 * Processes current Windows message
 */

LRESULT CALLBACK
winWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static winPrivScreenPtr s_pScreenPriv = NULL;
    static winScreenInfo *s_pScreenInfo = NULL;
    static ScreenPtr s_pScreen = NULL;
    static HWND s_hwndLastPrivates = NULL;
    static Bool s_fTracking = FALSE;
    static unsigned long s_ulServerGeneration = 0;
    static UINT s_uTaskbarRestart = 0;
    int iScanCode;
    int i;

#if CYGDEBUG
    winDebugWin32Message("winWindowProc", hwnd, message, wParam, lParam);
#endif

    /* Watch for server regeneration */
    if (g_ulServerGeneration != s_ulServerGeneration) {
        /* Store new server generation */
        s_ulServerGeneration = g_ulServerGeneration;
    }

    /* Only retrieve new privates pointers if window handle is null or changed */
    if ((s_pScreenPriv == NULL || hwnd != s_hwndLastPrivates)
        && (s_pScreenPriv = GetProp(hwnd, WIN_SCR_PROP)) != NULL) {
#if CYGDEBUG
        winDebug("winWindowProc - Setting privates handle\n");
#endif
        s_pScreenInfo = s_pScreenPriv->pScreenInfo;
        s_pScreen = s_pScreenInfo->pScreen;
        s_hwndLastPrivates = hwnd;
    }
    else if (s_pScreenPriv == NULL) {
        /* For safety, handle case that should never happen */
        s_pScreenInfo = NULL;
        s_pScreen = NULL;
        s_hwndLastPrivates = NULL;
    }

    /* Branch on message type */
    switch (message) {
    case WM_TRAYICON:
        return winHandleIconMessage(hwnd, message, wParam, lParam,
                                    s_pScreenPriv);

    case WM_CREATE:
#if CYGDEBUG
        winDebug("winWindowProc - WM_CREATE\n");
#endif

        /*
         * Add a property to our display window that references
         * this screens' privates.
         *
         * This allows the window procedure to refer to the
         * appropriate window DC and shadow DC for the window that
         * it is processing.  We use this to repaint exposed
         * areas of our display window.
         */
        s_pScreenPriv = ((LPCREATESTRUCT) lParam)->lpCreateParams;
        s_pScreenInfo = s_pScreenPriv->pScreenInfo;
        s_pScreen = s_pScreenInfo->pScreen;
        s_hwndLastPrivates = hwnd;
        s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
        SetProp(hwnd, WIN_SCR_PROP, s_pScreenPriv);

        /* Setup tray icon */
        if (!s_pScreenInfo->fNoTrayIcon) {
            /*
             * NOTE: The WM_CREATE message is processed before CreateWindowEx
             * returns, so s_pScreenPriv->hwndScreen is invalid at this point.
             * We go ahead and copy our hwnd parameter over top of the screen
             * privates hwndScreen so that we have a valid value for
             * that member.  Otherwise, the tray icon will disappear
             * the first time you move the mouse over top of it.
             */

            s_pScreenPriv->hwndScreen = hwnd;

            winInitNotifyIcon(s_pScreenPriv);
        }
        return 0;

    case WM_DISPLAYCHANGE:
        /*
           WM_DISPLAYCHANGE seems to be sent when the monitor layout or
           any monitor's resolution or depth changes, but its lParam and
           wParam always indicate the resolution and bpp for the primary
           monitor (so ignore that as we could be on any monitor...)
         */

        /* We cannot handle a display mode change during initialization */
        if (s_pScreenInfo == NULL)
            FatalError("winWindowProc - WM_DISPLAYCHANGE - The display "
                       "mode changed while we were initializing.  This is "
                       "very bad and unexpected.  Exiting.\n");

        /*
         * We do not care about display changes with
         * fullscreen DirectDraw engines, because those engines set
         * their own mode when they become active.
         */
        if (s_pScreenInfo->fFullScreen
            && (s_pScreenInfo->dwEngine == WIN_SERVER_SHADOW_DDNL)) {
            break;
        }

        ErrorF("winWindowProc - WM_DISPLAYCHANGE - new width: %d "
               "new height: %d new bpp: %d\n",
               LOWORD(lParam), HIWORD(lParam), (int)wParam);

        /* 0 bpp has no defined meaning, ignore this message */
        if (wParam == 0)
            break;

        /*
         * Check for a disruptive change in depth.
         * We can only display a message for a disruptive depth change,
         * we cannot do anything to correct the situation.
         */
        /*
           XXX: maybe we need to check if GetSystemMetrics(SM_SAMEDISPLAYFORMAT)
           has changed as well...
         */
        if (s_pScreenInfo->dwBPP !=
            GetDeviceCaps(s_pScreenPriv->hdcScreen, BITSPIXEL)) {
            if (s_pScreenInfo->dwEngine == WIN_SERVER_SHADOW_DDNL) {
                /* Cannot display the visual until the depth is restored */
                ErrorF("winWindowProc - Disruptive change in depth\n");

                /* Display depth change dialog */
                winDisplayDepthChangeDialog(s_pScreenPriv);

                /* Flag that we have an invalid screen depth */
                s_pScreenPriv->fBadDepth = TRUE;

                /* Minimize the display window */
                ShowWindow(hwnd, SW_MINIMIZE);
            }
            else {
                /* For GDI, performance may suffer until original depth is restored */
                ErrorF
                    ("winWindowProc - Performance may be non-optimal after change in depth\n");
            }
        }
        else {
            /* Flag that we have a valid screen depth */
            s_pScreenPriv->fBadDepth = FALSE;
        }

        /*
           If we could cheaply check if this WM_DISPLAYCHANGE change
           affects the monitor(s) which this X screen is displayed on
           then we should do so here.  For the moment, assume it does.
           (this is probably usually the case so that might be an
           overoptimization)
         */
        {
            /*
               In rootless modes which are monitor or virtual desktop size
               use RandR to resize the X screen
             */
            if ((!s_pScreenInfo->fUserGaveHeightAndWidth) &&
                (s_pScreenInfo->iResizeMode == resizeWithRandr) && (s_pScreenInfo->
                                                                    fRootless
                                                                    ||
                                                                    s_pScreenInfo->
                                                                    fMultiWindow
                )) {
                DWORD dwWidth = 0, dwHeight = 0;

                if (s_pScreenInfo->fMultipleMonitors) {
                    /* resize to new virtual desktop size */
                    dwWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                    dwHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                }
                else {
                    /* resize to new size of specified monitor */
                    struct GetMonitorInfoData data;

                    if (QueryMonitor(s_pScreenInfo->iMonitor, &data)) {
                            dwWidth = data.monitorWidth;
                            dwHeight = data.monitorHeight;
                            /*
                               XXX: monitor may have changed position,
                               so we might need to update xinerama data
                             */
                        }
                        else {
                            ErrorF("Monitor number %d no longer exists!\n",
                                   s_pScreenInfo->iMonitor);
                        }
                }

                /*
                   XXX: probably a small bug here: we don't compute the work area
                   and allow for task bar

                   XXX: generally, we don't allow for the task bar being moved after
                   the server is started
                 */

                /* Set screen size to match new size, if it is different to current */
                if (((dwWidth != 0) && (dwHeight != 0)) &&
                    ((s_pScreenInfo->dwWidth != dwWidth) ||
                     (s_pScreenInfo->dwHeight != dwHeight))) {
                    winDoRandRScreenSetSize(s_pScreen,
                                            dwWidth,
                                            dwHeight,
                                            (dwWidth * 25.4) /
                                            monitorResolution,
                                            (dwHeight * 25.4) /
                                            monitorResolution);
                }
            }
            else {
                /*
                 * We can simply recreate the same-sized primary surface when
                 * the display dimensions change.
                 */

                winDebug
                    ("winWindowProc - WM_DISPLAYCHANGE - Releasing and recreating primary surface\n");

                /* Release the old primary surface */
                if (*s_pScreenPriv->pwinReleasePrimarySurface)
                    (*s_pScreenPriv->pwinReleasePrimarySurface) (s_pScreen);

                /* Create the new primary surface */
                if (*s_pScreenPriv->pwinCreatePrimarySurface)
                    (*s_pScreenPriv->pwinCreatePrimarySurface) (s_pScreen);
            }
        }

        break;

    case WM_SIZE:
    {
        SCROLLINFO si;
        RECT rcWindow;
        int iWidth, iHeight;

#if CYGDEBUG
        winDebug("winWindowProc - WM_SIZE\n");
#endif

        /* Break if we do not allow resizing */
        if ((s_pScreenInfo->iResizeMode == resizeNotAllowed)
            || !s_pScreenInfo->fDecoration
            || s_pScreenInfo->fRootless
            || s_pScreenInfo->fMultiWindow
            || s_pScreenInfo->fFullScreen)
            break;

        /* No need to resize if we get minimized */
        if (wParam == SIZE_MINIMIZED)
            return 0;

        ErrorF("winWindowProc - WM_SIZE - new client area w: %d h: %d\n",
               LOWORD(lParam), HIWORD(lParam));

        if (s_pScreenInfo->iResizeMode == resizeWithRandr) {
            /* Actual resizing is done on WM_EXITSIZEMOVE */
            return 0;
        }

        /* Otherwise iResizeMode == resizeWithScrollbars */

        /*
         * Get the size of the whole window, including client area,
         * scrollbars, and non-client area decorations (caption, borders).
         * We do this because we need to check if the client area
         * without scrollbars is large enough to display the whole visual.
         * The new client area size passed by lParam already subtracts
         * the size of the scrollbars if they are currently displayed.
         * So checking is LOWORD(lParam) == visual_width and
         * HIWORD(lParam) == visual_height will never tell us to hide
         * the scrollbars because the client area would always be too small.
         * GetClientRect returns the same sizes given by lParam, so we
         * cannot use GetClientRect either.
         */
        GetWindowRect(hwnd, &rcWindow);
        iWidth = rcWindow.right - rcWindow.left;
        iHeight = rcWindow.bottom - rcWindow.top;

        /* Subtract the frame size from the window size. */
        iWidth -= 2 * GetSystemMetrics(SM_CXSIZEFRAME);
        iHeight -= (2 * GetSystemMetrics(SM_CYSIZEFRAME)
                    + GetSystemMetrics(SM_CYCAPTION));

        /*
         * Update scrollbar page sizes.
         * NOTE: If page size == range, then the scrollbar is
         * automatically hidden.
         */

        /* Is the naked client area large enough to show the whole visual? */
        if (iWidth < s_pScreenInfo->dwWidth
            || iHeight < s_pScreenInfo->dwHeight) {
            /* Client area too small to display visual, use scrollbars */
            iWidth -= GetSystemMetrics(SM_CXVSCROLL);
            iHeight -= GetSystemMetrics(SM_CYHSCROLL);
        }

        /* Set the horizontal scrollbar page size */
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE | SIF_RANGE;
        si.nMin = 0;
        si.nMax = s_pScreenInfo->dwWidth - 1;
        si.nPage = iWidth;
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

        /* Set the vertical scrollbar page size */
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE | SIF_RANGE;
        si.nMin = 0;
        si.nMax = s_pScreenInfo->dwHeight - 1;
        si.nPage = iHeight;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

        /*
         * NOTE: Scrollbars may have moved if they were at the
         * far right/bottom, so we query their current position.
         */

        /* Get the horizontal scrollbar position and set the offset */
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        GetScrollInfo(hwnd, SB_HORZ, &si);
        s_pScreenInfo->dwXOffset = -si.nPos;

        /* Get the vertical scrollbar position and set the offset */
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        GetScrollInfo(hwnd, SB_VERT, &si);
        s_pScreenInfo->dwYOffset = -si.nPos;
    }
        return 0;

    case WM_SYSCOMMAND:
        if (s_pScreenInfo->iResizeMode == resizeWithRandr &&
            ((wParam & 0xfff0) == SC_MAXIMIZE ||
             (wParam & 0xfff0) == SC_RESTORE))
            PostMessage(hwnd, WM_EXITSIZEMOVE, 0, 0);
        break;

    case WM_ENTERSIZEMOVE:
        ErrorF("winWindowProc - WM_ENTERSIZEMOVE\n");
        break;

    case WM_EXITSIZEMOVE:
        ErrorF("winWindowProc - WM_EXITSIZEMOVE\n");

        if (s_pScreenInfo->iResizeMode == resizeWithRandr) {
            /* Set screen size to match new client area, if it is different to current */
            RECT rcClient;
            DWORD dwWidth, dwHeight;

            GetClientRect(hwnd, &rcClient);
            dwWidth = rcClient.right - rcClient.left;
            dwHeight = rcClient.bottom - rcClient.top;

            if ((s_pScreenInfo->dwWidth != dwWidth) ||
                (s_pScreenInfo->dwHeight != dwHeight)) {
                /* mm = dots * (25.4 mm / inch) / (dots / inch) */
                winDoRandRScreenSetSize(s_pScreen,
                                        dwWidth,
                                        dwHeight,
                                        (dwWidth * 25.4) / monitorResolution,
                                        (dwHeight * 25.4) / monitorResolution);
            }
        }

        break;

    case WM_VSCROLL:
    {
        SCROLLINFO si;
        int iVertPos;

#if CYGDEBUG
        winDebug("winWindowProc - WM_VSCROLL\n");
#endif

        /* Get vertical scroll bar info */
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        /* Save the vertical position for comparison later */
        iVertPos = si.nPos;

        /*
         * Don't forget:
         * moving the scrollbar to the DOWN, scroll the content UP
         */
        switch (LOWORD(wParam)) {
        case SB_TOP:
            si.nPos = si.nMin;
            break;

        case SB_BOTTOM:
            si.nPos = si.nMax - si.nPage + 1;
            break;

        case SB_LINEUP:
            si.nPos -= 1;
            break;

        case SB_LINEDOWN:
            si.nPos += 1;
            break;

        case SB_PAGEUP:
            si.nPos -= si.nPage;
            break;

        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            break;

        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;

        default:
            break;
        }

        /*
         * We retrieve the position after setting it,
         * because Windows may adjust it.
         */
        si.fMask = SIF_POS;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        GetScrollInfo(hwnd, SB_VERT, &si);

        /* Scroll the window if the position has changed */
        if (si.nPos != iVertPos) {
            /* Save the new offset for bit block transfers, etc. */
            s_pScreenInfo->dwYOffset = -si.nPos;

            /* Change displayed region in the window */
            ScrollWindowEx(hwnd,
                           0,
                           iVertPos - si.nPos,
                           NULL, NULL, NULL, NULL, SW_INVALIDATE);

            /* Redraw the window contents */
            UpdateWindow(hwnd);
        }
    }
        return 0;

    case WM_HSCROLL:
    {
        SCROLLINFO si;
        int iHorzPos;

#if CYGDEBUG
        winDebug("winWindowProc - WM_HSCROLL\n");
#endif

        /* Get horizontal scroll bar info */
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_HORZ, &si);

        /* Save the horizontal position for comparison later */
        iHorzPos = si.nPos;

        /*
         * Don't forget:
         * moving the scrollbar to the RIGHT, scroll the content LEFT
         */
        switch (LOWORD(wParam)) {
        case SB_LEFT:
            si.nPos = si.nMin;
            break;

        case SB_RIGHT:
            si.nPos = si.nMax - si.nPage + 1;
            break;

        case SB_LINELEFT:
            si.nPos -= 1;
            break;

        case SB_LINERIGHT:
            si.nPos += 1;
            break;

        case SB_PAGELEFT:
            si.nPos -= si.nPage;
            break;

        case SB_PAGERIGHT:
            si.nPos += si.nPage;
            break;

        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;

        default:
            break;
        }

        /*
         * We retrieve the position after setting it,
         * because Windows may adjust it.
         */
        si.fMask = SIF_POS;
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
        GetScrollInfo(hwnd, SB_HORZ, &si);

        /* Scroll the window if the position has changed */
        if (si.nPos != iHorzPos) {
            /* Save the new offset for bit block transfers, etc. */
            s_pScreenInfo->dwXOffset = -si.nPos;

            /* Change displayed region in the window */
            ScrollWindowEx(hwnd,
                           iHorzPos - si.nPos,
                           0, NULL, NULL, NULL, NULL, SW_INVALIDATE);

            /* Redraw the window contents */
            UpdateWindow(hwnd);
        }
    }
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *pMinMaxInfo = (MINMAXINFO *) lParam;
        int iCaptionHeight;
        int iBorderHeight, iBorderWidth;

#if CYGDEBUG
        winDebug("winWindowProc - WM_GETMINMAXINFO - pScreenInfo: %p\n",
                 s_pScreenInfo);
#endif

        /* Can't do anything without screen info */
        if (s_pScreenInfo == NULL
            || (s_pScreenInfo->iResizeMode != resizeWithScrollbars)
            || s_pScreenInfo->fFullScreen || !s_pScreenInfo->fDecoration
            || s_pScreenInfo->fRootless
            || s_pScreenInfo->fMultiWindow
            )
            break;

        /*
         * Here we can override the maximum tracking size, which
         * is the largest size that can be assigned to our window
         * via the sizing border.
         */

        /*
         * FIXME: Do we only need to do this once, since our visual size
         * does not change?  Does Windows store this value statically
         * once we have set it once?
         */

        /* Get the border and caption sizes */
        iCaptionHeight = GetSystemMetrics(SM_CYCAPTION);
        iBorderWidth = 2 * GetSystemMetrics(SM_CXSIZEFRAME);
        iBorderHeight = 2 * GetSystemMetrics(SM_CYSIZEFRAME);

        /* Allow the full visual to be displayed */
        pMinMaxInfo->ptMaxTrackSize.x = s_pScreenInfo->dwWidth + iBorderWidth;
        pMinMaxInfo->ptMaxTrackSize.y
            = s_pScreenInfo->dwHeight + iBorderHeight + iCaptionHeight;
    }
        return 0;

    case WM_ERASEBKGND:
#if CYGDEBUG
        winDebug("winWindowProc - WM_ERASEBKGND\n");
#endif
        /*
         * Pretend that we did erase the background but we don't care,
         * the application uses the full window estate. This avoids some
         * flickering when resizing.
         */
        return TRUE;

    case WM_PAINT:
#if CYGDEBUG
        winDebug("winWindowProc - WM_PAINT\n");
#endif
        /* Only paint if we have privates and the server is enabled */
        if (s_pScreenPriv == NULL
            || !s_pScreenPriv->fEnabled
            || (s_pScreenInfo->fFullScreen && !s_pScreenPriv->fActive)
            || s_pScreenPriv->fBadDepth) {
            /* We don't want to paint */
            break;
        }

        /* Break out here if we don't have a valid paint routine */
        if (s_pScreenPriv->pwinBltExposedRegions == NULL)
            break;

        /* Call the engine dependent repainter */
        (*s_pScreenPriv->pwinBltExposedRegions) (s_pScreen);
        return 0;

    case WM_PALETTECHANGED:
    {
#if CYGDEBUG
        winDebug("winWindowProc - WM_PALETTECHANGED\n");
#endif
        /*
         * Don't process if we don't have privates or a colormap,
         * or if we have an invalid depth.
         */
        if (s_pScreenPriv == NULL
            || s_pScreenPriv->pcmapInstalled == NULL
            || s_pScreenPriv->fBadDepth)
            break;

        /* Return if we caused the palette to change */
        if ((HWND) wParam == hwnd) {
            /* Redraw the screen */
            (*s_pScreenPriv->pwinRedrawScreen) (s_pScreen);
            return 0;
        }

        /* Reinstall the windows palette */
        (*s_pScreenPriv->pwinRealizeInstalledPalette) (s_pScreen);

        /* Redraw the screen */
        (*s_pScreenPriv->pwinRedrawScreen) (s_pScreen);
        return 0;
    }

    case WM_MOUSEMOVE:
        /* We can't do anything without privates */
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /* We can't do anything without g_pwinPointer */
        if (g_pwinPointer == NULL)
            break;

        /* Has the mouse pointer crossed screens? */
        if (s_pScreen != miPointerGetScreen(g_pwinPointer))
            miPointerSetScreen(g_pwinPointer, s_pScreenInfo->dwScreen,
                               GET_X_LPARAM(lParam) - s_pScreenInfo->dwXOffset,
                               GET_Y_LPARAM(lParam) - s_pScreenInfo->dwYOffset);

        /* Are we tracking yet? */
        if (!s_fTracking) {
            TRACKMOUSEEVENT tme;

            /* Setup data structure */
            ZeroMemory(&tme, sizeof(tme));
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;

            /* Call the tracking function */
            if (!TrackMouseEvent(&tme))
                ErrorF("winWindowProc - TrackMouseEvent failed\n");

            /* Flag that we are tracking now */
            s_fTracking = TRUE;
        }

        /* Hide or show the Windows mouse cursor */
        if (g_fSoftwareCursor && g_fCursor &&
            (s_pScreenPriv->fActive || s_pScreenInfo->fLessPointer)) {
            /* Hide Windows cursor */
            g_fCursor = FALSE;
            ShowCursor(FALSE);
        }
        else if (g_fSoftwareCursor && !g_fCursor && !s_pScreenPriv->fActive
                 && !s_pScreenInfo->fLessPointer) {
            /* Show Windows cursor */
            g_fCursor = TRUE;
            ShowCursor(TRUE);
        }

        /* Deliver absolute cursor position to X Server */
        winEnqueueMotion(GET_X_LPARAM(lParam) - s_pScreenInfo->dwXOffset,
                         GET_Y_LPARAM(lParam) - s_pScreenInfo->dwYOffset);
        return 0;

    case WM_NCMOUSEMOVE:
        /*
         * We break instead of returning 0 since we need to call
         * DefWindowProc to get the mouse cursor changes
         * and min/max/close button highlighting in Windows XP.
         * The Platform SDK says that you should return 0 if you
         * process this message, but it fails to mention that you
         * will give up any default functionality if you do return 0.
         */

        /* We can't do anything without privates */
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /* Non-client mouse movement, show Windows cursor */
        if (g_fSoftwareCursor && !g_fCursor) {
            g_fCursor = TRUE;
            ShowCursor(TRUE);
        }
        break;

    case WM_MOUSELEAVE:
        /* Mouse has left our client area */

        /* Flag that we are no longer tracking */
        s_fTracking = FALSE;

        /* Show the mouse cursor, if necessary */
        if (g_fSoftwareCursor && !g_fCursor) {
            g_fCursor = TRUE;
            ShowCursor(TRUE);
        }
        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        if (s_pScreenInfo->fRootless)
            SetCapture(hwnd);
        return winMouseButtonsHandle(s_pScreen, ButtonPress, Button1, wParam);

    case WM_LBUTTONUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        if (s_pScreenInfo->fRootless)
            ReleaseCapture();
        return winMouseButtonsHandle(s_pScreen, ButtonRelease, Button1, wParam);

    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        if (s_pScreenInfo->fRootless)
            SetCapture(hwnd);
        return winMouseButtonsHandle(s_pScreen, ButtonPress, Button2, wParam);

    case WM_MBUTTONUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        if (s_pScreenInfo->fRootless)
            ReleaseCapture();
        return winMouseButtonsHandle(s_pScreen, ButtonRelease, Button2, wParam);

    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        if (s_pScreenInfo->fRootless)
            SetCapture(hwnd);
        return winMouseButtonsHandle(s_pScreen, ButtonPress, Button3, wParam);

    case WM_RBUTTONUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        if (s_pScreenInfo->fRootless)
            ReleaseCapture();
        return winMouseButtonsHandle(s_pScreen, ButtonRelease, Button3, wParam);

    case WM_XBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        if (s_pScreenInfo->fRootless)
            SetCapture(hwnd);
        return winMouseButtonsHandle(s_pScreen, ButtonPress, HIWORD(wParam) + 7,
                                     wParam);
    case WM_XBUTTONUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        if (s_pScreenInfo->fRootless)
            ReleaseCapture();
        return winMouseButtonsHandle(s_pScreen, ButtonRelease,
                                     HIWORD(wParam) + 7, wParam);

    case WM_TIMER:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /* Branch on the timer id */
        switch (wParam) {
        case WIN_E3B_TIMER_ID:
            /* Send delayed button press */
            winMouseButtonsSendEvent(ButtonPress,
                                     s_pScreenPriv->iE3BCachedPress);

            /* Kill this timer */
            KillTimer(s_pScreenPriv->hwndScreen, WIN_E3B_TIMER_ID);

            /* Clear screen privates flags */
            s_pScreenPriv->iE3BCachedPress = 0;
            break;

        case WIN_POLLING_MOUSE_TIMER_ID:
        {
            static POINT last_point;
            POINT point;
            WPARAM wL, wM, wR, wShift, wCtrl;
            LPARAM lPos;

            /* Get the current position of the mouse cursor */
            GetCursorPos(&point);

            /* Map from screen (-X, -Y) to root (0, 0) */
            point.x -= GetSystemMetrics(SM_XVIRTUALSCREEN);
            point.y -= GetSystemMetrics(SM_YVIRTUALSCREEN);

            /* If the mouse pointer has moved, deliver absolute cursor position to X Server */
            if (last_point.x != point.x || last_point.y != point.y) {
                winEnqueueMotion(point.x, point.y);
                last_point.x = point.x;
                last_point.y = point.y;
            }

            /* Check if a button was released but we didn't see it */
            GetCursorPos(&point);
            wL = (GetKeyState(VK_LBUTTON) & 0x8000) ? MK_LBUTTON : 0;
            wM = (GetKeyState(VK_MBUTTON) & 0x8000) ? MK_MBUTTON : 0;
            wR = (GetKeyState(VK_RBUTTON) & 0x8000) ? MK_RBUTTON : 0;
            wShift = (GetKeyState(VK_SHIFT) & 0x8000) ? MK_SHIFT : 0;
            wCtrl = (GetKeyState(VK_CONTROL) & 0x8000) ? MK_CONTROL : 0;
            lPos = MAKELPARAM(point.x, point.y);
            if (g_fButton[0] && !wL)
                PostMessage(hwnd, WM_LBUTTONUP, wCtrl | wM | wR | wShift, lPos);
            if (g_fButton[1] && !wM)
                PostMessage(hwnd, WM_MBUTTONUP, wCtrl | wL | wR | wShift, lPos);
            if (g_fButton[2] && !wR)
                PostMessage(hwnd, WM_RBUTTONUP, wCtrl | wL | wM | wShift, lPos);
        }
        }
        return 0;

    case WM_CTLCOLORSCROLLBAR:
        FatalError("winWindowProc - WM_CTLCOLORSCROLLBAR - We are not "
                   "supposed to get this message.  Exiting.\n");
        return 0;

    case WM_MOUSEWHEEL:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
#if CYGDEBUG
        winDebug("winWindowProc - WM_MOUSEWHEEL\n");
#endif
        /* Button4 = WheelUp */
        /* Button5 = WheelDown */
        winMouseWheel(&(s_pScreenPriv->iDeltaZ), GET_WHEEL_DELTA_WPARAM(wParam), Button4, Button5);
        break;

    case WM_MOUSEHWHEEL:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
#if CYGDEBUG
        winDebug("winWindowProc - WM_MOUSEHWHEEL\n");
#endif
        /* Button7 = TiltRight */
        /* Button6 = TiltLeft */
        winMouseWheel(&(s_pScreenPriv->iDeltaV), GET_WHEEL_DELTA_WPARAM(wParam), 7, 6);
        break;

    case WM_SETFOCUS:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /* Restore the state of all mode keys */
        winRestoreModeKeyStates();

        /* Add the keyboard hook if possible */
        if (g_fKeyboardHookLL)
            g_fKeyboardHookLL = winInstallKeyboardHookLL();
        return 0;

    case WM_KILLFOCUS:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /* Release any pressed keys */
        winKeybdReleaseKeys();

        /* Remove our keyboard hook if it is installed */
        winRemoveKeyboardHookLL();
        return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /*
         * FIXME: Catching Alt-F4 like this is really terrible.  This should
         * be generalized to handle other Windows keyboard signals.  Actually,
         * the list keys to catch and the actions to perform when caught should
         * be configurable; that way user's can customize the keys that they
         * need to have passed through to their window manager or apps, or they
         * can remap certain actions to new key codes that do not conflict
         * with the X apps that they are using.  Yeah, that'll take awhile.
         */
        if ((s_pScreenInfo->fUseWinKillKey && wParam == VK_F4
             && (GetKeyState(VK_MENU) & 0x8000))
            || (s_pScreenInfo->fUseUnixKillKey && wParam == VK_BACK
                && (GetKeyState(VK_MENU) & 0x8000)
                && (GetKeyState(VK_CONTROL) & 0x8000))) {
            /*
             * Better leave this message here, just in case some unsuspecting
             * user enters Alt + F4 and is surprised when the application
             * quits.
             */
            ErrorF("winWindowProc - WM_*KEYDOWN - Closekey hit, quitting\n");

            /* Display Exit dialog */
            winDisplayExitDialog(s_pScreenPriv);
            return 0;
        }

        /*
         * Don't do anything for the Windows keys, as focus will soon
         * be returned to Windows.  We may be able to trap the Windows keys,
         * but we should determine if that is desirable before doing so.
         */
        if ((wParam == VK_LWIN || wParam == VK_RWIN) && !g_fKeyboardHookLL)
            break;

        /* Discard fake Ctrl_L events that precede AltGR on non-US keyboards */
        if (winIsFakeCtrl_L(message, wParam, lParam))
            return 0;

        /*
         * Discard presses generated from Windows auto-repeat
         */
        if (lParam & (1 << 30)) {
            switch (wParam) {
                /* ago: Pressing LControl while RControl is pressed is
                 * Indicated as repeat. Fix this!
                 */
            case VK_CONTROL:
            case VK_SHIFT:
                if (winCheckKeyPressed(wParam, lParam))
                    return 0;
                break;
            default:
                return 0;
            }
        }

        /* Translate Windows key code to X scan code */
        iScanCode = winTranslateKey(wParam, lParam);

        /* Ignore repeats for CapsLock */
        if (wParam == VK_CAPITAL)
            lParam = 1;

        /* Send the key event(s) */
        for (i = 0; i < LOWORD(lParam); ++i)
            winSendKeyEvent(iScanCode, TRUE);
        return 0;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /*
         * Don't do anything for the Windows keys, as focus will soon
         * be returned to Windows.  We may be able to trap the Windows keys,
         * but we should determine if that is desirable before doing so.
         */
        if ((wParam == VK_LWIN || wParam == VK_RWIN) && !g_fKeyboardHookLL)
            break;

        /* Ignore the fake Ctrl_L that follows an AltGr release */
        if (winIsFakeCtrl_L(message, wParam, lParam))
            return 0;

        /* Enqueue a keyup event */
        iScanCode = winTranslateKey(wParam, lParam);
        winSendKeyEvent(iScanCode, FALSE);

        /* Release all pressed shift keys */
        if (wParam == VK_SHIFT)
            winFixShiftKeys(iScanCode);
        return 0;

    case WM_ACTIVATE:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /* TODO: Override display of window when we have a bad depth */
        if (LOWORD(wParam) != WA_INACTIVE && s_pScreenPriv->fBadDepth) {
            ErrorF("winWindowProc - WM_ACTIVATE - Bad depth, trying "
                   "to override window activation\n");

            /* Minimize the window */
            ShowWindow(hwnd, SW_MINIMIZE);

            /* Display dialog box */
            if (g_hDlgDepthChange != NULL) {
                /* Make the existing dialog box active */
                SetActiveWindow(g_hDlgDepthChange);
            }
            else {
                /* TODO: Recreate the dialog box and bring to the top */
                ShowWindow(g_hDlgDepthChange, SW_SHOWDEFAULT);
            }

            /* Don't do any other processing of this message */
            return 0;
        }

#if CYGDEBUG
        winDebug("winWindowProc - WM_ACTIVATE\n");
#endif

        /*
         * Focus is being changed to another window.
         * The other window may or may not belong to
         * our process.
         */

        /* Clear any lingering wheel delta */
        s_pScreenPriv->iDeltaZ = 0;
        s_pScreenPriv->iDeltaV = 0;

        /* Reshow the Windows mouse cursor if we are being deactivated */
        if (g_fSoftwareCursor && LOWORD(wParam) == WA_INACTIVE && !g_fCursor) {
            /* Show Windows cursor */
            g_fCursor = TRUE;
            ShowCursor(TRUE);
        }
        return 0;

    case WM_ACTIVATEAPP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

#if CYGDEBUG || TRUE
        winDebug("winWindowProc - WM_ACTIVATEAPP\n");
#endif

        /* Activate or deactivate */
        s_pScreenPriv->fActive = wParam;

        /* Reshow the Windows mouse cursor if we are being deactivated */
        if (g_fSoftwareCursor && !s_pScreenPriv->fActive && !g_fCursor) {
            /* Show Windows cursor */
            g_fCursor = TRUE;
            ShowCursor(TRUE);
        }

        /* Call engine specific screen activation/deactivation function */
        (*s_pScreenPriv->pwinActivateApp) (s_pScreen);

        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_APP_EXIT:
            /* Display Exit dialog */
            winDisplayExitDialog(s_pScreenPriv);
            return 0;

        case ID_APP_HIDE_ROOT:
            if (s_pScreenPriv->fRootWindowShown)
                ShowWindow(s_pScreenPriv->hwndScreen, SW_HIDE);
            else
                ShowWindow(s_pScreenPriv->hwndScreen, SW_SHOW);
            s_pScreenPriv->fRootWindowShown = !s_pScreenPriv->fRootWindowShown;
            return 0;

        case ID_APP_MONITOR_PRIMARY:
            fPrimarySelection = !fPrimarySelection;
            return 0;

        case ID_APP_ABOUT:
            /* Display the About box */
            winDisplayAboutDialog(s_pScreenPriv);
            return 0;

        default:
            /* It's probably one of the custom menus... */
            if (HandleCustomWM_COMMAND(0, LOWORD(wParam), s_pScreenPriv))
                return 0;
        }
        break;

    case WM_GIVEUP:
        /* Tell X that we are giving up */
        if (s_pScreenInfo->fMultiWindow)
            winDeinitMultiWindowWM();
        GiveUp(0);
        return 0;

    case WM_CLOSE:
        /* Display Exit dialog */
        winDisplayExitDialog(s_pScreenPriv);
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            if (!g_fSoftwareCursor)
                SetCursor(s_pScreenPriv->cursor.handle);
            return TRUE;
        }
        break;

    default:
        if ((message == s_uTaskbarRestart) && !s_pScreenInfo->fNoTrayIcon)  {
            winInitNotifyIcon(s_pScreenPriv);
        }
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
