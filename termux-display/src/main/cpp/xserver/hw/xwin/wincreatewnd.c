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
#include "shellapi.h"

/*
 * Local function prototypes
 */

static Bool
 winGetWorkArea(RECT * prcWorkArea, winScreenInfo * pScreenInfo);

static Bool
 winAdjustForAutoHide(RECT * prcWorkArea, winScreenInfo * pScreenInfo);

/*
 * Create a full screen window
 */

Bool
winCreateBoundingWindowFullScreen(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    int iX = pScreenInfo->dwInitialX;
    int iY = pScreenInfo->dwInitialY;
    int iWidth = pScreenInfo->dwWidth;
    int iHeight = pScreenInfo->dwHeight;
    HWND *phwnd = &pScreenPriv->hwndScreen;
    WNDCLASSEX wc;
    char szTitle[256];

#if CYGDEBUG
    winDebug("winCreateBoundingWindowFullScreen\n");
#endif

    /* Setup our window class */
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = winWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstance;
    wc.hIcon = pScreenInfo->hIcon;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIconSm = pScreenInfo->hIconSm;
    RegisterClassEx(&wc);

    /* Set display and screen-specific tooltip text */
    if (g_pszQueryHost != NULL)
        snprintf(szTitle,
                 sizeof(szTitle),
                 WINDOW_TITLE_XDMCP,
                 g_pszQueryHost, display, (int) pScreenInfo->dwScreen);
    else
        snprintf(szTitle,
                 sizeof(szTitle),
                 WINDOW_TITLE, display, (int) pScreenInfo->dwScreen);

    /* Create the window */
    *phwnd = CreateWindowExA(0, /* Extended styles */
                             WINDOW_CLASS,      /* Class name */
                             szTitle,   /* Window name */
                             WS_POPUP, iX,      /* Horizontal position */
                             iY,        /* Vertical position */
                             iWidth,    /* Right edge */
                             iHeight,   /* Bottom edge */
                             (HWND) NULL,       /* No parent or owner window */
                             (HMENU) NULL,      /* No menu */
                             GetModuleHandle(NULL),     /* Instance handle */
                             pScreenPriv);      /* ScreenPrivates */

    /* Hide the window */
    ShowWindow(*phwnd, SW_SHOWNORMAL);

    /* Send first paint message */
    UpdateWindow(*phwnd);

    /* Attempt to bring our window to the top of the display */
    BringWindowToTop(*phwnd);

    return TRUE;
}

/*
 * Create our primary Windows display window
 */

Bool
winCreateBoundingWindowWindowed(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    int iWidth = pScreenInfo->dwUserWidth;
    int iHeight = pScreenInfo->dwUserHeight;
    int iPosX;
    int iPosY;
    HWND *phwnd = &pScreenPriv->hwndScreen;
    WNDCLASSEX wc;
    RECT rcClient, rcWorkArea;
    DWORD dwWindowStyle;
    BOOL fForceShowWindow = FALSE;
    char szTitle[256];

    winDebug("winCreateBoundingWindowWindowed - User w: %d h: %d\n",
             (int) pScreenInfo->dwUserWidth, (int) pScreenInfo->dwUserHeight);
    winDebug("winCreateBoundingWindowWindowed - Current w: %d h: %d\n",
             (int) pScreenInfo->dwWidth, (int) pScreenInfo->dwHeight);

    /* Set the common window style flags */
    dwWindowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX;

    /* Decorated or undecorated window */
    if (pScreenInfo->fDecoration
        && !pScreenInfo->fRootless
        && !pScreenInfo->fMultiWindow
        ) {
        /* Try to handle startup via run.exe. run.exe instructs Windows to
         * hide all created windows. Detect this case and make sure the
         * window is shown nevertheless */
        STARTUPINFO startupInfo;

        GetStartupInfo(&startupInfo);
        if (startupInfo.dwFlags & STARTF_USESHOWWINDOW &&
            startupInfo.wShowWindow == SW_HIDE) {
            fForceShowWindow = TRUE;
        }
        dwWindowStyle |= WS_CAPTION;
        if (pScreenInfo->iResizeMode != resizeNotAllowed)
            dwWindowStyle |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }
    else
        dwWindowStyle |= WS_POPUP;

    /* Setup our window class */
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = winWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstance;
    wc.hIcon = pScreenInfo->hIcon;
    wc.hCursor = 0;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIconSm = pScreenInfo->hIconSm;
    RegisterClassEx(&wc);

    /* Get size of work area */
    winGetWorkArea(&rcWorkArea, pScreenInfo);

    /* Adjust for auto-hide taskbars */
    winAdjustForAutoHide(&rcWorkArea, pScreenInfo);

    /* Did the user specify a position? */
    if (pScreenInfo->fUserGavePosition) {
        iPosX = pScreenInfo->dwInitialX;
        iPosY = pScreenInfo->dwInitialY;
    }
    else {
        iPosX = rcWorkArea.left;
        iPosY = rcWorkArea.top;
    }

    /* Clean up the scrollbars flag, if necessary */
    if ((!pScreenInfo->fDecoration
         || pScreenInfo->fRootless
         || pScreenInfo->fMultiWindow
        )
        && (pScreenInfo->iResizeMode == resizeWithScrollbars)) {
        /* We cannot have scrollbars if we do not have a window border */
        pScreenInfo->iResizeMode = resizeNotAllowed;
    }

    /* Did the user specify a height and width? */
    if (pScreenInfo->fUserGaveHeightAndWidth) {
        /* User gave a desired height and width, try to accommodate */
#if CYGDEBUG
        winDebug("winCreateBoundingWindowWindowed - User gave height "
                 "and width\n");
#endif

        /* Adjust the window width and height for borders and title bar */
        if (pScreenInfo->fDecoration
            && !pScreenInfo->fRootless
            && !pScreenInfo->fMultiWindow
            ) {
#if CYGDEBUG
            winDebug
                ("winCreateBoundingWindowWindowed - Window has decoration\n");
#endif

            /* Are we resizable */
            if (pScreenInfo->iResizeMode != resizeNotAllowed) {
#if CYGDEBUG
                winDebug
                    ("winCreateBoundingWindowWindowed - Window is resizable\n");
#endif

                iWidth += 2 * GetSystemMetrics(SM_CXSIZEFRAME);
                iHeight += 2 * GetSystemMetrics(SM_CYSIZEFRAME)
                    + GetSystemMetrics(SM_CYCAPTION);
            }
            else {
#if CYGDEBUG
                winDebug
                    ("winCreateBoundingWindowWindowed - Window is not resizable\n");
#endif

                iWidth += 2 * GetSystemMetrics(SM_CXFIXEDFRAME);
                iHeight += 2 * GetSystemMetrics(SM_CYFIXEDFRAME)
                    + GetSystemMetrics(SM_CYCAPTION);
            }
        }
    }
    else {
        /* By default, we are creating a window that is as large as possible */
#if CYGDEBUG
        winDebug("winCreateBoundingWindowWindowed - User did not give "
                 "height and width\n");
#endif
        /* Defaults are wrong if we have multiple monitors */
        if (pScreenInfo->fMultipleMonitors) {
            iWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            iHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        }
    }

    /* Make sure window is no bigger than work area */
    if (TRUE
        && !pScreenInfo->fMultiWindow
        ) {
        /* Trim window width to fit work area */
        if (iWidth > (rcWorkArea.right - rcWorkArea.left))
            iWidth = rcWorkArea.right - rcWorkArea.left;

        /* Trim window height to fit work area */
        if (iHeight >= (rcWorkArea.bottom - rcWorkArea.top))
            iHeight = rcWorkArea.bottom - rcWorkArea.top;

#if CYGDEBUG
        winDebug("winCreateBoundingWindowWindowed - Adjusted width: %d "
                 "height: %d\n", iWidth, iHeight);
#endif
    }

    /* Set display and screen-specific tooltip text */
    if (g_pszQueryHost != NULL)
        snprintf(szTitle,
                 sizeof(szTitle),
                 WINDOW_TITLE_XDMCP,
                 g_pszQueryHost, display, (int) pScreenInfo->dwScreen);
    else
        snprintf(szTitle,
                 sizeof(szTitle),
                 WINDOW_TITLE, display, (int) pScreenInfo->dwScreen);

    /* Create the window */
    *phwnd = CreateWindowExA(0, /* Extended styles */
                             WINDOW_CLASS,      /* Class name */
                             szTitle,   /* Window name */
                             dwWindowStyle, iPosX,      /* Horizontal position */
                             iPosY,     /* Vertical position */
                             iWidth,    /* Right edge */
                             iHeight,   /* Bottom edge */
                             (HWND) NULL,       /* No parent or owner window */
                             (HMENU) NULL,      /* No menu */
                             GetModuleHandle(NULL),     /* Instance handle */
                             pScreenPriv);      /* ScreenPrivates */
    if (*phwnd == NULL) {
        ErrorF("winCreateBoundingWindowWindowed - CreateWindowEx () failed\n");
        return FALSE;
    }

#if CYGDEBUG
    winDebug("winCreateBoundingWindowWindowed - CreateWindowEx () returned\n");
#endif

    if (fForceShowWindow) {
        ErrorF
            ("winCreateBoundingWindowWindowed - Setting normal windowstyle\n");
        ShowWindow(*phwnd, SW_SHOW);
    }

    /* Get the client area coordinates */
    if (!GetClientRect(*phwnd, &rcClient)) {
        ErrorF("winCreateBoundingWindowWindowed - GetClientRect () "
               "failed\n");
        return FALSE;
    }

    winDebug("winCreateBoundingWindowWindowed - WindowClient "
             "w %d  h %d r %d l %d b %d t %d\n",
             (int)(rcClient.right - rcClient.left),
             (int)(rcClient.bottom - rcClient.top),
             (int)rcClient.right, (int)rcClient.left,
             (int)rcClient.bottom, (int)rcClient.top);

    /* We adjust the visual size if the user did not specify it */
    if (!
        ((pScreenInfo->iResizeMode == resizeWithScrollbars) &&
         pScreenInfo->fUserGaveHeightAndWidth)) {
        /*
         * User did not give a height and width with scrollbars enabled,
         * so we will resize the underlying visual to be as large as
         * the initial view port (page size).  This way scrollbars will
         * not appear until the user shrinks the window, if they ever do.
         *
         * NOTE: We have to store the viewport size here because
         * the user may have an autohide taskbar, which would
         * cause the viewport size to be one less in one dimension
         * than the viewport size that we calculated by subtracting
         * the size of the borders and caption.
         */
        pScreenInfo->dwWidth = rcClient.right - rcClient.left;
        pScreenInfo->dwHeight = rcClient.bottom - rcClient.top;
    }

#if 0
    /*
     * NOTE: For the uninitiated, the page size is the number of pixels
     * that we can display in the x or y direction at a time and the
     * range is the total number of pixels in the x or y direction that we
     * have available to display.  In other words, the page size is the
     * size of the window area minus the space the caption, borders, and
     * scrollbars (if any) occupy, and the range is the size of the
     * underlying X visual.  Notice that, contrary to what some of the
     * MSDN Library arcticles lead you to believe, the windows
     * ``client area'' size does not include the scrollbars.  In other words,
     * the whole client area size that is reported to you is drawable by
     * you; you do not have to subtract the size of the scrollbars from
     * the client area size, and if you did it would result in the size
     * of the scrollbars being double counted.
     */

    /* Setup scrollbar page and range, if scrollbars are enabled */
    if (pScreenInfo->fScrollbars) {
        SCROLLINFO si;

        /* Initialize the scrollbar info structure */
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;

        /* Setup the width range and page size */
        si.nMax = pScreenInfo->dwWidth - 1;
        si.nPage = rcClient.right - rcClient.left;
        winDebug("winCreateBoundingWindowWindowed - HORZ nMax: %d nPage :%d\n",
                 si.nMax, si.nPage);
        SetScrollInfo(*phwnd, SB_HORZ, &si, TRUE);

        /* Setup the height range and page size */
        si.nMax = pScreenInfo->dwHeight - 1;
        si.nPage = rcClient.bottom - rcClient.top;
        winDebug("winCreateBoundingWindowWindowed - VERT nMax: %d nPage :%d\n",
                 si.nMax, si.nPage);
        SetScrollInfo(*phwnd, SB_VERT, &si, TRUE);
    }
#endif

    /* Show the window */
    if (FALSE
        || pScreenInfo->fMultiWindow
        ) {
        pScreenPriv->fRootWindowShown = FALSE;
        ShowWindow(*phwnd, SW_HIDE);
    }
    else
        ShowWindow(*phwnd, SW_SHOWNORMAL);
    if (!UpdateWindow(*phwnd)) {
        ErrorF("winCreateBoundingWindowWindowed - UpdateWindow () failed\n");
        return FALSE;
    }

    /* Attempt to bring our window to the top of the display */
    if (TRUE
        && !pScreenInfo->fRootless
        && !pScreenInfo->fMultiWindow
        ) {
        if (!BringWindowToTop(*phwnd)) {
            ErrorF("winCreateBoundingWindowWindowed - BringWindowToTop () "
                   "failed\n");
            return FALSE;
        }
    }

    winDebug("winCreateBoundingWindowWindowed -  Returning\n");

    return TRUE;
}

/*
 * Find the work area of all attached monitors
 */

static Bool
winGetWorkArea(RECT * prcWorkArea, winScreenInfo * pScreenInfo)
{
    int iPrimaryWidth, iPrimaryHeight;
    int iWidth, iHeight;
    int iLeft, iTop;
    int iPrimaryNonWorkAreaWidth, iPrimaryNonWorkAreaHeight;

    /* Use GetMonitorInfo to get work area for monitor */
    if (!pScreenInfo->fMultipleMonitors) {
        MONITORINFO mi;

        mi.cbSize = sizeof(MONITORINFO);
        if (GetMonitorInfo(pScreenInfo->hMonitor, &mi)) {
            *prcWorkArea = mi.rcWork;

            winDebug("winGetWorkArea - Monitor %d WorkArea: %d %d %d %d\n",
                     pScreenInfo->iMonitor,
                     (int) prcWorkArea->top, (int) prcWorkArea->left,
                     (int) prcWorkArea->bottom, (int) prcWorkArea->right);
        }
        else {
            ErrorF("winGetWorkArea - GetMonitorInfo() failed for monitor %d\n",
                   pScreenInfo->iMonitor);
        }

        /* Bail out here if we aren't using multiple monitors */
        return TRUE;
    }

    /* SPI_GETWORKAREA only gets the work area of the primary screen. */
    SystemParametersInfo(SPI_GETWORKAREA, 0, prcWorkArea, 0);

    winDebug("winGetWorkArea - Primary Monitor WorkArea: %d %d %d %d\n",
             (int) prcWorkArea->top, (int) prcWorkArea->left,
             (int) prcWorkArea->bottom, (int) prcWorkArea->right);

    /* Get size of full virtual screen */
    iWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    iHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    winDebug("winGetWorkArea - Virtual screen is %d x %d\n", iWidth, iHeight);

    /* Get origin of full virtual screen */
    iLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    iTop = GetSystemMetrics(SM_YVIRTUALSCREEN);

    winDebug("winGetWorkArea - Virtual screen origin is %d, %d\n", iLeft, iTop);

    /* Get size of primary screen */
    iPrimaryWidth = GetSystemMetrics(SM_CXSCREEN);
    iPrimaryHeight = GetSystemMetrics(SM_CYSCREEN);

    winDebug("winGetWorkArea - Primary screen is %d x %d\n",
             iPrimaryWidth, iPrimaryHeight);

    /* Work out how much of the primary screen we aren't using */
    iPrimaryNonWorkAreaWidth = iPrimaryWidth - (prcWorkArea->right -
                                                prcWorkArea->left);
    iPrimaryNonWorkAreaHeight = iPrimaryHeight - (prcWorkArea->bottom
                                                  - prcWorkArea->top);

    /* Update the rectangle to include all monitors */
    if (iLeft < 0) {
        prcWorkArea->left = iLeft;
    }
    if (iTop < 0) {
        prcWorkArea->top = iTop;
    }
    prcWorkArea->right = prcWorkArea->left + iWidth - iPrimaryNonWorkAreaWidth;
    prcWorkArea->bottom = prcWorkArea->top + iHeight -
        iPrimaryNonWorkAreaHeight;

    winDebug("winGetWorkArea - Adjusted WorkArea for multiple "
             "monitors: %d %d %d %d\n",
             (int) prcWorkArea->top, (int) prcWorkArea->left,
             (int) prcWorkArea->bottom, (int) prcWorkArea->right);

    return TRUE;
}

static Bool
winTaskbarOnScreenEdge(unsigned int uEdge, winScreenInfo * pScreenInfo)
{
    APPBARDATA abd;
    HWND hwndAutoHide;

    ZeroMemory(&abd, sizeof(abd));
    abd.cbSize = sizeof(abd);
    abd.uEdge = uEdge;

    hwndAutoHide = (HWND) SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
    if (hwndAutoHide != NULL) {
        /*
           Found an autohide taskbar on that edge, but is it on the
           same monitor as the screen window?
         */
        if (pScreenInfo->fMultipleMonitors ||
            (MonitorFromWindow(hwndAutoHide, MONITOR_DEFAULTTONULL) ==
             pScreenInfo->hMonitor))
            return TRUE;
    }
    return FALSE;
}

/*
 * Adjust the client area so that any auto-hide toolbars
 * will work correctly.
 */

static Bool
winAdjustForAutoHide(RECT * prcWorkArea, winScreenInfo * pScreenInfo)
{
    APPBARDATA abd;

    winDebug("winAdjustForAutoHide - Original WorkArea: %d %d %d %d\n",
             (int) prcWorkArea->top, (int) prcWorkArea->left,
             (int) prcWorkArea->bottom, (int) prcWorkArea->right);

    /* Find out if the Windows taskbar is set to auto-hide */
    ZeroMemory(&abd, sizeof(abd));
    abd.cbSize = sizeof(abd);
    if (SHAppBarMessage(ABM_GETSTATE, &abd) & ABS_AUTOHIDE)
        winDebug("winAdjustForAutoHide - Taskbar is auto hide\n");

    /*
       Despite the forgoing, we are checking for any AppBar
       hiding along a monitor edge, not just the Windows TaskBar.
     */

    /* Look for a TOP auto-hide taskbar */
    if (winTaskbarOnScreenEdge(ABE_TOP, pScreenInfo)) {
        winDebug("winAdjustForAutoHide - Found TOP auto-hide taskbar\n");
        prcWorkArea->top += 1;
    }

    /* Look for a LEFT auto-hide taskbar */
    if (winTaskbarOnScreenEdge(ABE_LEFT, pScreenInfo)) {
        winDebug("winAdjustForAutoHide - Found LEFT auto-hide taskbar\n");
        prcWorkArea->left += 1;
    }

    /* Look for a BOTTOM auto-hide taskbar */
    if (winTaskbarOnScreenEdge(ABE_BOTTOM, pScreenInfo)) {
        winDebug("winAdjustForAutoHide - Found BOTTOM auto-hide taskbar\n");
        prcWorkArea->bottom -= 1;
    }

    /* Look for a RIGHT auto-hide taskbar */
    if (winTaskbarOnScreenEdge(ABE_RIGHT, pScreenInfo)) {
        winDebug("winAdjustForAutoHide - Found RIGHT auto-hide taskbar\n");
        prcWorkArea->right -= 1;
    }

    winDebug("winAdjustForAutoHide - Adjusted WorkArea: %d %d %d %d\n",
             (int) prcWorkArea->top, (int) prcWorkArea->left,
             (int) prcWorkArea->bottom, (int) prcWorkArea->right);

    return TRUE;
}
