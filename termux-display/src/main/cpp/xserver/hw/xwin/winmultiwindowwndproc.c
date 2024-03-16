/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *Copyright (C) Colin Harrison 2005-2008
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
 * Authors:	Kensuke Matsuzaki
 *		Earle F. Philhower, III
 *		Harold L Hunt II
 *              Colin Harrison
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "win.h"
#include "dixevents.h"
#include "winmultiwindowclass.h"
#include "winprefs.h"
#include "winmsg.h"
#include "inputstr.h"
#include <dwmapi.h>

#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031e
#endif

extern void winUpdateWindowPosition(HWND hWnd, HWND * zstyle);

/*
 * Local globals
 */

static UINT_PTR g_uipMousePollingTimerID = 0;

/*
 * Constant defines
 */

#define WIN_MULTIWINDOW_SHAPE		YES

/*
 * ConstrainSize - Taken from TWM sources - Respects hints for sizing
 */
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
static void
ConstrainSize(WinXSizeHints hints, int *widthp, int *heightp)
{
    int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
    int baseWidth, baseHeight;
    int dwidth = *widthp, dheight = *heightp;

    if (hints.flags & PMinSize) {
        minWidth = hints.min_width;
        minHeight = hints.min_height;
    }
    else if (hints.flags & PBaseSize) {
        minWidth = hints.base_width;
        minHeight = hints.base_height;
    }
    else
        minWidth = minHeight = 1;

    if (hints.flags & PBaseSize) {
        baseWidth = hints.base_width;
        baseHeight = hints.base_height;
    }
    else if (hints.flags & PMinSize) {
        baseWidth = hints.min_width;
        baseHeight = hints.min_height;
    }
    else
        baseWidth = baseHeight = 0;

    if (hints.flags & PMaxSize) {
        maxWidth = hints.max_width;
        maxHeight = hints.max_height;
    }
    else {
        maxWidth = MAXINT;
        maxHeight = MAXINT;
    }

    if (hints.flags & PResizeInc) {
        xinc = hints.width_inc;
        yinc = hints.height_inc;
    }
    else
        xinc = yinc = 1;

    /*
     * First, clamp to min and max values
     */
    if (dwidth < minWidth)
        dwidth = minWidth;
    if (dheight < minHeight)
        dheight = minHeight;

    if (dwidth > maxWidth)
        dwidth = maxWidth;
    if (dheight > maxHeight)
        dheight = maxHeight;

    /*
     * Second, fit to base + N * inc
     */
    dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
    dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;

    /*
     * Third, adjust for aspect ratio
     */

    /*
     * The math looks like this:
     *
     * minAspectX    dwidth     maxAspectX
     * ---------- <= ------- <= ----------
     * minAspectY    dheight    maxAspectY
     *
     * If that is multiplied out, then the width and height are
     * invalid in the following situations:
     *
     * minAspectX * dheight > minAspectY * dwidth
     * maxAspectX * dheight < maxAspectY * dwidth
     *
     */

    if (hints.flags & PAspect) {
        if (hints.min_aspect.x * dheight > hints.min_aspect.y * dwidth) {
            delta =
                makemult(hints.min_aspect.x * dheight / hints.min_aspect.y -
                         dwidth, xinc);
            if (dwidth + delta <= maxWidth)
                dwidth += delta;
            else {
                delta =
                    makemult(dheight -
                             dwidth * hints.min_aspect.y / hints.min_aspect.x,
                             yinc);
                if (dheight - delta >= minHeight)
                    dheight -= delta;
            }
        }

        if (hints.max_aspect.x * dheight < hints.max_aspect.y * dwidth) {
            delta =
                makemult(dwidth * hints.max_aspect.y / hints.max_aspect.x -
                         dheight, yinc);
            if (dheight + delta <= maxHeight)
                dheight += delta;
            else {
                delta =
                    makemult(dwidth -
                             hints.max_aspect.x * dheight / hints.max_aspect.y,
                             xinc);
                if (dwidth - delta >= minWidth)
                    dwidth -= delta;
            }
        }
    }

    /* Return computed values */
    *widthp = dwidth;
    *heightp = dheight;
}

#undef makemult

/*
 * ValidateSizing - Ensures size request respects hints
 */
static int
ValidateSizing(HWND hwnd, WindowPtr pWin, WPARAM wParam, LPARAM lParam)
{
    WinXSizeHints sizeHints;
    RECT *rect;
    int iWidth, iHeight;
    RECT rcClient, rcWindow;
    int iBorderWidthX, iBorderWidthY;

    /* Invalid input checking */
    if (pWin == NULL || lParam == 0)
        return FALSE;

    /* No size hints, no checking */
    if (!winMultiWindowGetWMNormalHints(pWin, &sizeHints))
        return FALSE;

    /* Avoid divide-by-zero */
    if (sizeHints.flags & PResizeInc) {
        if (sizeHints.width_inc == 0)
            sizeHints.width_inc = 1;
        if (sizeHints.height_inc == 0)
            sizeHints.height_inc = 1;
    }

    rect = (RECT *) lParam;

    iWidth = rect->right - rect->left;
    iHeight = rect->bottom - rect->top;

    /* Now remove size of any borders and title bar */
    GetClientRect(hwnd, &rcClient);
    GetWindowRect(hwnd, &rcWindow);
    iBorderWidthX =
        (rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left);
    iBorderWidthY =
        (rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top);
    iWidth -= iBorderWidthX;
    iHeight -= iBorderWidthY;

    /* Constrain the size to legal values */
    ConstrainSize(sizeHints, &iWidth, &iHeight);

    /* Add back the size of borders and title bar */
    iWidth += iBorderWidthX;
    iHeight += iBorderWidthY;

    /* Adjust size according to where we're dragging from */
    switch (wParam) {
    case WMSZ_TOP:
    case WMSZ_TOPRIGHT:
    case WMSZ_BOTTOM:
    case WMSZ_BOTTOMRIGHT:
    case WMSZ_RIGHT:
        rect->right = rect->left + iWidth;
        break;
    default:
        rect->left = rect->right - iWidth;
        break;
    }
    switch (wParam) {
    case WMSZ_BOTTOM:
    case WMSZ_BOTTOMRIGHT:
    case WMSZ_BOTTOMLEFT:
    case WMSZ_RIGHT:
    case WMSZ_LEFT:
        rect->bottom = rect->top + iHeight;
        break;
    default:
        rect->top = rect->bottom - iHeight;
        break;
    }
    return TRUE;
}

extern Bool winInDestroyWindowsWindow;
static Bool winInRaiseWindow = FALSE;
static void
winRaiseWindow(WindowPtr pWin)
{
    if (!winInDestroyWindowsWindow && !winInRaiseWindow) {
        BOOL oldstate = winInRaiseWindow;
        XID vlist[1] = { 0 };
        winInRaiseWindow = TRUE;
        /* Call configure window directly to make sure it gets processed
         * in time
         */
        ConfigureWindow(pWin, CWStackMode, vlist, serverClient);
        winInRaiseWindow = oldstate;
    }
}

static
    void
winStartMousePolling(winPrivScreenPtr s_pScreenPriv)
{
    /*
     * Timer to poll mouse position.  This is needed to make
     * programs like xeyes follow the mouse properly when the
     * mouse pointer is outside of any X window.
     */
    if (g_uipMousePollingTimerID == 0)
        g_uipMousePollingTimerID = SetTimer(s_pScreenPriv->hwndScreen,
                                            WIN_POLLING_MOUSE_TIMER_ID,
                                            MOUSE_POLLING_INTERVAL, NULL);
}

/* Undocumented */
typedef struct _ACCENTPOLICY
{
    ULONG AccentState;
    ULONG AccentFlags;
    ULONG GradientColor;
    ULONG AnimationId;
} ACCENTPOLICY;

#define ACCENT_ENABLE_BLURBEHIND 3

typedef struct _WINCOMPATTR
{
    DWORD attribute;
    PVOID pData;
    ULONG dataSize;
} WINCOMPATTR;

#define WCA_ACCENT_POLICY 19

typedef WINBOOL WINAPI (*PFNSETWINDOWCOMPOSITIONATTRIBUTE)(HWND, WINCOMPATTR *);

static void
CheckForAlpha(HWND hWnd, WindowPtr pWin, winScreenInfo *pScreenInfo)
{
    /* Check (once) which API we should use */
    static Bool doOnce = TRUE;
    static PFNSETWINDOWCOMPOSITIONATTRIBUTE pSetWindowCompositionAttribute = NULL;
    static Bool useDwmEnableBlurBehindWindow = FALSE;

    if (doOnce)
        {
            OSVERSIONINFOEX osvi = {0};
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            GetVersionEx((LPOSVERSIONINFO)&osvi);

            /* SetWindowCompositionAttribute() exists on Windows 7 and later,
               but doesn't work for this purpose, so first check for Windows 10
               or later */
            if (osvi.dwMajorVersion >= 10)
                {
                    HMODULE hUser32 = GetModuleHandle("user32");

                    if (hUser32)
                        pSetWindowCompositionAttribute = (PFNSETWINDOWCOMPOSITIONATTRIBUTE) GetProcAddress(hUser32, "SetWindowCompositionAttribute");
                    winDebug("SetWindowCompositionAttribute %s\n", pSetWindowCompositionAttribute ? "found" : "not found");
                }
            /* On Windows 7 and Windows Vista, use DwmEnableBlurBehindWindow() */
            else if ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion <= 1))
                {
                    useDwmEnableBlurBehindWindow = TRUE;
                }
            /* On Windows 8 and Windows 8.1, using the alpha channel on those
               seems near impossible, so we don't do anything. */

            doOnce = FALSE;
        }

    /* alpha-channel use is wanted */
    if (!g_fCompositeAlpha || !pScreenInfo->fCompositeWM)
        return;

    /* Image has alpha ... */
    if (pWin->drawable.depth != 32)
        return;

    /* ... and we can do something useful with it? */
    if (pSetWindowCompositionAttribute)
        {
            WINBOOL rc;
            /* Use the (undocumented) SetWindowCompositionAttribute, if
               available, to turn on alpha channel use on Windows 10. */
            ACCENTPOLICY policy = { ACCENT_ENABLE_BLURBEHIND, 0, 0, 0 } ;
            WINCOMPATTR data = { WCA_ACCENT_POLICY,  &policy, sizeof(ACCENTPOLICY) };

            /* This turns on DWM looking at the alpha-channel of this window */
            winDebug("enabling alpha for XID %08x hWnd %p, using SetWindowCompositionAttribute()\n", (unsigned int)pWin->drawable.id, hWnd);
            rc = pSetWindowCompositionAttribute(hWnd, &data);
            if (!rc)
                ErrorF("SetWindowCompositionAttribute failed: %d\n", (int)GetLastError());
        }
    else if (useDwmEnableBlurBehindWindow)
        {
            HRESULT rc;
            WINBOOL enabled;

            rc = DwmIsCompositionEnabled(&enabled);
            if ((rc == S_OK) && enabled)
                {
                    /* Use DwmEnableBlurBehindWindow, to turn on alpha channel
                       use on Windows Vista and Windows 7 */
                    DWM_BLURBEHIND bbh;
                    bbh.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION | DWM_BB_TRANSITIONONMAXIMIZED;
                    bbh.fEnable = TRUE;
                    bbh.hRgnBlur = NULL;
                    bbh.fTransitionOnMaximized = TRUE; /* What does this do ??? */

                    /* This terribly-named function actually controls if DWM
                       looks at the alpha channel of this window */
                    winDebug("enabling alpha for XID %08x hWnd %p, using DwmEnableBlurBehindWindow()\n", (unsigned int)pWin->drawable.id, hWnd);
                    rc = DwmEnableBlurBehindWindow(hWnd, &bbh);
                    if (rc != S_OK)
                        ErrorF("DwmEnableBlurBehindWindow failed: %x, %d\n", (int)rc, (int)GetLastError());
                }
        }
}

/*
 * winTopLevelWindowProc - Window procedure for all top-level Windows windows.
 */

LRESULT CALLBACK
winTopLevelWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT ptMouse;
    PAINTSTRUCT ps;
    WindowPtr pWin = NULL;
    winPrivWinPtr pWinPriv = NULL;
    ScreenPtr s_pScreen = NULL;
    winPrivScreenPtr s_pScreenPriv = NULL;
    winScreenInfo *s_pScreenInfo = NULL;
    HWND hwndScreen = NULL;
    DrawablePtr pDraw = NULL;
    winWMMessageRec wmMsg;
    Bool fWMMsgInitialized = FALSE;
    static Bool s_fTracking = FALSE;
    Bool needRestack = FALSE;
    LRESULT ret;
    static Bool hasEnteredSizeMove = FALSE;

#if CYGDEBUG
    winDebugWin32Message("winTopLevelWindowProc", hwnd, message, wParam,
                         lParam);
#endif

    /*
       If this is WM_CREATE, set up the Windows window properties which point to
       X window information, before we populate local convenience variables...
     */
    if (message == WM_CREATE) {
        SetProp(hwnd,
                WIN_WINDOW_PROP,
                (HANDLE) ((LPCREATESTRUCT) lParam)->lpCreateParams);
        SetProp(hwnd,
                WIN_WID_PROP,
                (HANDLE) (INT_PTR)winGetWindowID(((LPCREATESTRUCT) lParam)->
                                                 lpCreateParams));
    }

    /* Check if the Windows window property for our X window pointer is valid */
    if ((pWin = GetProp(hwnd, WIN_WINDOW_PROP)) != NULL) {
        /* Our X window pointer is valid */

        /* Get pointers to the drawable and the screen */
        pDraw = &pWin->drawable;
        s_pScreen = pWin->drawable.pScreen;

        /* Get a pointer to our window privates */
        pWinPriv = winGetWindowPriv(pWin);

        /* Get pointers to our screen privates and screen info */
        s_pScreenPriv = pWinPriv->pScreenPriv;
        s_pScreenInfo = s_pScreenPriv->pScreenInfo;

        /* Get the handle for our screen-sized window */
        hwndScreen = s_pScreenPriv->hwndScreen;

        /* */
        wmMsg.msg = 0;
        wmMsg.hwndWindow = hwnd;
        wmMsg.iWindow = (Window) (INT_PTR) GetProp(hwnd, WIN_WID_PROP);

        wmMsg.iX = pDraw->x;
        wmMsg.iY = pDraw->y;
        wmMsg.iWidth = pDraw->width;
        wmMsg.iHeight = pDraw->height;

        fWMMsgInitialized = TRUE;

#if 0
        /*
         * Print some debugging information
         */

        ErrorF("hWnd %08X\n", hwnd);
        ErrorF("pWin %08X\n", pWin);
        ErrorF("pDraw %08X\n", pDraw);
        ErrorF("\ttype %08X\n", pWin->drawable.type);
        ErrorF("\tclass %08X\n", pWin->drawable.class);
        ErrorF("\tdepth %08X\n", pWin->drawable.depth);
        ErrorF("\tbitsPerPixel %08X\n", pWin->drawable.bitsPerPixel);
        ErrorF("\tid %08X\n", pWin->drawable.id);
        ErrorF("\tx %08X\n", pWin->drawable.x);
        ErrorF("\ty %08X\n", pWin->drawable.y);
        ErrorF("\twidth %08X\n", pWin->drawable.width);
        ErrorF("\thenght %08X\n", pWin->drawable.height);
        ErrorF("\tpScreen %08X\n", pWin->drawable.pScreen);
        ErrorF("\tserialNumber %08X\n", pWin->drawable.serialNumber);
        ErrorF("g_iWindowPrivateKey %p\n", g_iWindowPrivateKey);
        ErrorF("pWinPriv %08X\n", pWinPriv);
        ErrorF("s_pScreenPriv %08X\n", s_pScreenPriv);
        ErrorF("s_pScreenInfo %08X\n", s_pScreenInfo);
        ErrorF("hwndScreen %08X\n", hwndScreen);
#endif
    }

    /* Branch on message type */
    switch (message) {
    case WM_CREATE:
        /*
         * Make X windows' Z orders sync with Windows windows because
         * there can be AlwaysOnTop windows overlapped on the window
         * currently being created.
         */
        winReorderWindowsMultiWindow();

        /* Fix a 'round title bar corner background should be transparent not black' problem when first painted */
        {
            RECT rWindow;
            HRGN hRgnWindow;

            GetWindowRect(hwnd, &rWindow);
            hRgnWindow = CreateRectRgnIndirect(&rWindow);
            SetWindowRgn(hwnd, hRgnWindow, TRUE);
            DeleteObject(hRgnWindow);
        }

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) XMING_SIGNATURE);

        CheckForAlpha(hwnd, pWin, s_pScreenInfo);

        return 0;

    case WM_INIT_SYS_MENU:
        /*
         * Add whatever the setup file wants to for this window
         */
        SetupSysMenu(hwnd);
        return 0;

    case WM_SYSCOMMAND:
        /*
         * Any window menu items go through here
         */
        if (HandleCustomWM_COMMAND(hwnd, LOWORD(wParam), s_pScreenPriv)) {
            /* Don't pass customized menus to DefWindowProc */
            return 0;
        }
        if (wParam == SC_RESTORE || wParam == SC_MAXIMIZE) {
            WINDOWPLACEMENT wndpl;

            wndpl.length = sizeof(wndpl);
            if (GetWindowPlacement(hwnd, &wndpl) &&
                wndpl.showCmd == SW_SHOWMINIMIZED)
                needRestack = TRUE;
        }
        break;

    case WM_INITMENU:
        /* Checks/Unchecks any menu items before they are displayed */
        HandleCustomWM_INITMENU(hwnd, (HMENU)wParam);
        break;

    case WM_ERASEBKGND:
        /*
         * Pretend that we did erase the background but we don't care,
         * since we repaint the entire region anyhow
         * This avoids some flickering when resizing.
         */
        return TRUE;

    case WM_PAINT:
        /* Only paint if our window handle is valid */
        if (hwnd == NULL)
            break;

#ifdef XWIN_GLX_WINDOWS
        if (pWinPriv->fWglUsed) {
            /*
               For regions which are being drawn by GL, the shadow framebuffer doesn't have the
               correct bits, so don't bitblt from the shadow framebuffer

               XXX: For now, just leave it alone, but ideally we want to send an expose event to
               the window so it really redraws the affected region...
             */
            BeginPaint(hwnd, &ps);
            ValidateRect(hwnd, &(ps.rcPaint));
            EndPaint(hwnd, &ps);
        }
        else
#endif
            /* Call the engine dependent repainter */
            if (*s_pScreenPriv->pwinBltExposedWindowRegion)
                (*s_pScreenPriv->pwinBltExposedWindowRegion) (s_pScreen, pWin);

        return 0;

    case WM_MOUSEMOVE:
        /* Unpack the client area mouse coordinates */
        ptMouse.x = GET_X_LPARAM(lParam);
        ptMouse.y = GET_Y_LPARAM(lParam);

        /* Translate the client area mouse coordinates to screen coordinates */
        ClientToScreen(hwnd, &ptMouse);

        /* Screen Coords from (-X, -Y) -> Root Window (0, 0) */
        ptMouse.x -= GetSystemMetrics(SM_XVIRTUALSCREEN);
        ptMouse.y -= GetSystemMetrics(SM_YVIRTUALSCREEN);

        /* We can't do anything without privates */
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        /* Has the mouse pointer crossed screens? */
        if (s_pScreen != miPointerGetScreen(g_pwinPointer))
            miPointerSetScreen(g_pwinPointer, s_pScreenInfo->dwScreen,
                               ptMouse.x - s_pScreenInfo->dwXOffset,
                               ptMouse.y - s_pScreenInfo->dwYOffset);

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
                ErrorF("winTopLevelWindowProc - TrackMouseEvent failed\n");

            /* Flag that we are tracking now */
            s_fTracking = TRUE;
        }

        /* Hide or show the Windows mouse cursor */
        if (g_fSoftwareCursor && g_fCursor) {
            /* Hide Windows cursor */
            g_fCursor = FALSE;
            ShowCursor(FALSE);
        }

        /* Kill the timer used to poll mouse events */
        if (g_uipMousePollingTimerID != 0) {
            KillTimer(s_pScreenPriv->hwndScreen, WIN_POLLING_MOUSE_TIMER_ID);
            g_uipMousePollingTimerID = 0;
        }

        /* Deliver absolute cursor position to X Server */
        winEnqueueMotion(ptMouse.x - s_pScreenInfo->dwXOffset,
                         ptMouse.y - s_pScreenInfo->dwYOffset);

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

        winStartMousePolling(s_pScreenPriv);

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

        winStartMousePolling(s_pScreenPriv);

        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        g_fButton[0] = TRUE;
        SetCapture(hwnd);
        return winMouseButtonsHandle(s_pScreen, ButtonPress, Button1, wParam);

    case WM_LBUTTONUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        g_fButton[0] = FALSE;
        ReleaseCapture();
        winStartMousePolling(s_pScreenPriv);
        return winMouseButtonsHandle(s_pScreen, ButtonRelease, Button1, wParam);

    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        g_fButton[1] = TRUE;
        SetCapture(hwnd);
        return winMouseButtonsHandle(s_pScreen, ButtonPress, Button2, wParam);

    case WM_MBUTTONUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        g_fButton[1] = FALSE;
        ReleaseCapture();
        winStartMousePolling(s_pScreenPriv);
        return winMouseButtonsHandle(s_pScreen, ButtonRelease, Button2, wParam);

    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        g_fButton[2] = TRUE;
        SetCapture(hwnd);
        return winMouseButtonsHandle(s_pScreen, ButtonPress, Button3, wParam);

    case WM_RBUTTONUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        g_fButton[2] = FALSE;
        ReleaseCapture();
        winStartMousePolling(s_pScreenPriv);
        return winMouseButtonsHandle(s_pScreen, ButtonRelease, Button3, wParam);

    case WM_XBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        SetCapture(hwnd);
        return winMouseButtonsHandle(s_pScreen, ButtonPress, HIWORD(wParam) + 7,
                                     wParam);

    case WM_XBUTTONUP:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;
        ReleaseCapture();
        winStartMousePolling(s_pScreenPriv);
        return winMouseButtonsHandle(s_pScreen, ButtonRelease,
                                     HIWORD(wParam) + 7, wParam);

    case WM_MOUSEWHEEL:
        if (SendMessage
            (hwnd, WM_NCHITTEST, 0,
             MAKELONG(GET_X_LPARAM(lParam),
                      GET_Y_LPARAM(lParam))) == HTCLIENT) {
            /* Pass the message to the root window */
            SendMessage(hwndScreen, message, wParam, lParam);
            return 0;
        }
        else
            break;

    case WM_MOUSEHWHEEL:
        if (SendMessage
            (hwnd, WM_NCHITTEST, 0,
             MAKELONG(GET_X_LPARAM(lParam),
                      GET_Y_LPARAM(lParam))) == HTCLIENT) {
            /* Pass the message to the root window */
            SendMessage(hwndScreen, message, wParam, lParam);
            return 0;
        }
        else
            break;

    case WM_SETFOCUS:
        if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
            break;

        {
            /* Get the parent window for transient handling */
            HWND hParent = GetParent(hwnd);

            if (hParent && IsIconic(hParent))
                ShowWindow(hParent, SW_RESTORE);
        }

        winRestoreModeKeyStates();

        /* Add the keyboard hook if possible */
        if (g_fKeyboardHookLL)
            g_fKeyboardHookLL = winInstallKeyboardHookLL();
        return 0;

    case WM_KILLFOCUS:
        /* Pop any pressed keys since we are losing keyboard focus */
        winKeybdReleaseKeys();

        /* Remove our keyboard hook if it is installed */
        winRemoveKeyboardHookLL();

        /* Revert the X focus as well, but only if the Windows focus is going to another window */
        if (!wParam && pWin)
            DeleteWindowFromAnyEvents(pWin, FALSE);

        return 0;

    case WM_SYSDEADCHAR:
    case WM_DEADCHAR:
        /*
         * NOTE: We do nothing with WM_*CHAR messages,
         * nor does the root window, so we can just toss these messages.
         */
        return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:

        /*
         * Don't pass Alt-F4 key combo to root window,
         * let Windows translate to WM_CLOSE and close this top-level window.
         *
         * NOTE: We purposely don't check the fUseWinKillKey setting because
         * it should only apply to the key handling for the root window,
         * not for top-level window-manager windows.
         *
         * ALSO NOTE: We do pass Ctrl-Alt-Backspace to the root window
         * because that is a key combo that no X app should be expecting to
         * receive, since it has historically been used to shutdown the X server.
         * Passing Ctrl-Alt-Backspace to the root window preserves that
         * behavior, assuming that -unixkill has been passed as a parameter.
         */
        if (wParam == VK_F4 && (GetKeyState(VK_MENU) & 0x8000))
            break;

#if CYGWINDOWING_DEBUG
        if (wParam == VK_ESCAPE) {
            /* Place for debug: put any tests and dumps here */
            WINDOWPLACEMENT windPlace;
            RECT rc;
            LPRECT pRect;

            windPlace.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(hwnd, &windPlace);
            pRect = &windPlace.rcNormalPosition;
            ErrorF("\nCYGWINDOWING Dump:\n"
                   "\tdrawable: (%hd, %hd) - %hdx%hd\n", pDraw->x,
                   pDraw->y, pDraw->width, pDraw->height);
            ErrorF("\twindPlace: (%d, %d) - %dx%d\n", (int)pRect->left,
                   (int)pRect->top, (int)(pRect->right - pRect->left),
                   (int)(pRect->bottom - pRect->top));
            if (GetClientRect(hwnd, &rc)) {
                pRect = &rc;
                ErrorF("\tClientRect: (%d, %d) - %dx%d\n", (int)pRect->left,
                       (int)pRect->top, (int)(pRect->right - pRect->left),
                       (int)(pRect->bottom - pRect->top));
            }
            if (GetWindowRect(hwnd, &rc)) {
                pRect = &rc;
                ErrorF("\tWindowRect: (%d, %d) - %dx%d\n", (int)pRect->left,
                       (int)pRect->top, (int)(pRect->right - pRect->left),
                       (int)(pRect->bottom - pRect->top));
            }
            ErrorF("\n");
        }
#endif

        /* Pass the message to the root window */
        return winWindowProc(hwndScreen, message, wParam, lParam);

    case WM_SYSKEYUP:
    case WM_KEYUP:

        /* Pass the message to the root window */
        return winWindowProc(hwndScreen, message, wParam, lParam);

    case WM_HOTKEY:

        /* Pass the message to the root window */
        SendMessage(hwndScreen, message, wParam, lParam);
        return 0;

    case WM_ACTIVATE:

        /* Pass the message to the root window */
        SendMessage(hwndScreen, message, wParam, lParam);

        if (LOWORD(wParam) != WA_INACTIVE) {
            /* Raise the window to the top in Z order */
            /* ago: Activate does not mean putting it to front! */
            /*
               wmMsg.msg = WM_WM_RAISE;
               if (fWMMsgInitialized)
               winSendMessageToWM (s_pScreenPriv->pWMInfo, &wmMsg);
             */

            /* Tell our Window Manager thread to activate the window */
            wmMsg.msg = WM_WM_ACTIVATE;
            if (fWMMsgInitialized)
                if (!pWin || !pWin->overrideRedirect)   /* for OOo menus */
                    winSendMessageToWM(s_pScreenPriv->pWMInfo, &wmMsg);
        }
        /* Prevent the mouse wheel from stalling when another window is minimized */
        if (HIWORD(wParam) == 0 && LOWORD(wParam) == WA_ACTIVE &&
            (HWND) lParam != NULL && (HWND) lParam != GetParent(hwnd))
            SetFocus(hwnd);
        return 0;

    case WM_ACTIVATEAPP:
        /*
         * This message is also sent to the root window
         * so we do nothing for individual multiwindow windows
         */
        break;

    case WM_CLOSE:
        /* Remove AppUserModelID property */
        winSetAppUserModelID(hwnd, NULL);
        /* Branch on if the window was killed in X already */
        if (pWinPriv->fXKilled) {
            /* Window was killed, go ahead and destroy the window */
            DestroyWindow(hwnd);
        }
        else {
            /* Tell our Window Manager thread to kill the window */
            wmMsg.msg = WM_WM_KILL;
            if (fWMMsgInitialized)
                winSendMessageToWM(s_pScreenPriv->pWMInfo, &wmMsg);
        }
        return 0;

    case WM_DESTROY:

        /* Branch on if the window was killed in X already */
        if (pWinPriv && !pWinPriv->fXKilled) {
            ErrorF("winTopLevelWindowProc - WM_DESTROY - WM_WM_KILL\n");

            /* Tell our Window Manager thread to kill the window */
            wmMsg.msg = WM_WM_KILL;
            if (fWMMsgInitialized)
                winSendMessageToWM(s_pScreenPriv->pWMInfo, &wmMsg);
        }

        RemoveProp(hwnd, WIN_WINDOW_PROP);
        RemoveProp(hwnd, WIN_WID_PROP);
        RemoveProp(hwnd, WIN_NEEDMANAGE_PROP);

        break;

    case WM_MOVE:
        /* Adjust the X Window to the moved Windows window */
        if (!hasEnteredSizeMove)
            winAdjustXWindow(pWin, hwnd);
        /* else: Wait for WM_EXITSIZEMOVE */
        return 0;

    case WM_SHOWWINDOW:
        /* Bail out if the window is being hidden */
        if (!wParam)
            return 0;

        /* */
        if (!pWin->overrideRedirect) {
            HWND zstyle = HWND_NOTOPMOST;

            /* Flag that this window needs to be made active when clicked */
            SetProp(hwnd, WIN_NEEDMANAGE_PROP, (HANDLE) 1);

            /* Set the transient style flags */
            if (GetParent(hwnd))
                SetWindowLongPtr(hwnd, GWL_STYLE,
                                 WS_POPUP | WS_OVERLAPPED | WS_SYSMENU |
                                 WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
            /* Set the window standard style flags */
            else
                SetWindowLongPtr(hwnd, GWL_STYLE,
                                 (WS_POPUP | WS_OVERLAPPEDWINDOW |
                                  WS_CLIPCHILDREN | WS_CLIPSIBLINGS)
                                 & ~WS_CAPTION & ~WS_SIZEBOX);

            winUpdateWindowPosition(hwnd, &zstyle);

            {
                WinXWMHints hints;

                if (winMultiWindowGetWMHints(pWin, &hints)) {
                    /*
                       Give the window focus, unless it has an InputHint
                       which is FALSE (this is used by e.g. glean to
                       avoid every test window grabbing the focus)
                     */
                    if (!((hints.flags & InputHint) && (!hints.input))) {
                        SetForegroundWindow(hwnd);
                    }
                }
            }
            wmMsg.msg = WM_WM_MAP_MANAGED;
        }
        else {                  /* It is an overridden window so make it top of Z stack */

            HWND forHwnd = GetForegroundWindow();

#if CYGWINDOWING_DEBUG
            ErrorF("overridden window is shown\n");
#endif
            if (forHwnd != NULL) {
                if (GetWindowLongPtr(forHwnd, GWLP_USERDATA) & (LONG_PTR)
                    XMING_SIGNATURE) {
                    if (GetWindowLongPtr(forHwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
                        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    else
                        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                }
            }
            wmMsg.msg = WM_WM_MAP_UNMANAGED;
        }

        /* Tell our Window Manager thread to map the window */
        if (fWMMsgInitialized)
            winSendMessageToWM(s_pScreenPriv->pWMInfo, &wmMsg);

        winStartMousePolling(s_pScreenPriv);

        return 0;

    case WM_SIZING:
        /* Need to legalize the size according to WM_NORMAL_HINTS */
        /* for applications like xterm */
        return ValidateSizing(hwnd, pWin, wParam, lParam);

    case WM_WINDOWPOSCHANGED:
    {
        LPWINDOWPOS pWinPos = (LPWINDOWPOS) lParam;

        if (!(pWinPos->flags & SWP_NOZORDER)) {
#if CYGWINDOWING_DEBUG
            winDebug("\twindow z order was changed\n");
#endif
            if (pWinPos->hwndInsertAfter == HWND_TOP
                || pWinPos->hwndInsertAfter == HWND_TOPMOST
                || pWinPos->hwndInsertAfter == HWND_NOTOPMOST) {
#if CYGWINDOWING_DEBUG
                winDebug("\traise to top\n");
#endif
                /* Raise the window to the top in Z order */
                winRaiseWindow(pWin);
            }
            else if (pWinPos->hwndInsertAfter == HWND_BOTTOM) {
            }
            else {
                /* Check if this window is top of X windows. */
                HWND hWndAbove = NULL;
                DWORD dwCurrentProcessID = GetCurrentProcessId();
                DWORD dwWindowProcessID = 0;

                for (hWndAbove = pWinPos->hwndInsertAfter;
                     hWndAbove != NULL;
                     hWndAbove = GetNextWindow(hWndAbove, GW_HWNDPREV)) {
                    /* Ignore other XWin process's window */
                    GetWindowThreadProcessId(hWndAbove, &dwWindowProcessID);

                    if ((dwWindowProcessID == dwCurrentProcessID)
                        && GetProp(hWndAbove, WIN_WINDOW_PROP)
                        && !IsWindowVisible(hWndAbove)
                        && !IsIconic(hWndAbove))        /* ignore minimized windows */
                        break;
                }
                /* If this is top of X windows in Windows stack,
                   raise it in X stack. */
                if (hWndAbove == NULL) {
#if CYGWINDOWING_DEBUG
                    winDebug("\traise to top\n");
#endif
                    winRaiseWindow(pWin);
                }
            }
        }
    }
        /*
         * Pass the message to DefWindowProc to let the function
         * break down WM_WINDOWPOSCHANGED to WM_MOVE and WM_SIZE.
         */
        break;

    case WM_ENTERSIZEMOVE:
        hasEnteredSizeMove = TRUE;
        return 0;

    case WM_EXITSIZEMOVE:
        /* Adjust the X Window to the moved Windows window */
        hasEnteredSizeMove = FALSE;
        winAdjustXWindow(pWin, hwnd);
        return 0;

    case WM_SIZE:
        /* see dix/window.c */
#if CYGWINDOWING_DEBUG
    {
        char buf[64];

        switch (wParam) {
        case SIZE_MINIMIZED:
            strcpy(buf, "SIZE_MINIMIZED");
            break;
        case SIZE_MAXIMIZED:
            strcpy(buf, "SIZE_MAXIMIZED");
            break;
        case SIZE_RESTORED:
            strcpy(buf, "SIZE_RESTORED");
            break;
        default:
            strcpy(buf, "UNKNOWN_FLAG");
        }
        ErrorF("winTopLevelWindowProc - WM_SIZE to %dx%d (%s)\n",
               (int) LOWORD(lParam), (int) HIWORD(lParam), buf);
    }
#endif
        if (!hasEnteredSizeMove) {
            /* Adjust the X Window to the moved Windows window */
            winAdjustXWindow(pWin, hwnd);
        }
        /* else: wait for WM_EXITSIZEMOVE */
        return 0;               /* end of WM_SIZE handler */

    case WM_STYLECHANGING:
        /*
           When the style changes, adjust the Windows window size so the client area remains the same size,
           and adjust the Windows window position so that the client area remains in the same place.
         */
    {
        RECT newWinRect;
        DWORD dwExStyle;
        DWORD dwStyle;
        DWORD newStyle = ((STYLESTRUCT *) lParam)->styleNew;
        WINDOWINFO wi;

        dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);

        winDebug("winTopLevelWindowProc - WM_STYLECHANGING from %08x %08x\n",
                 (unsigned int)dwStyle, (unsigned int)dwExStyle);

        if (wParam == GWL_EXSTYLE)
            dwExStyle = newStyle;

        if (wParam == GWL_STYLE)
            dwStyle = newStyle;

        winDebug("winTopLevelWindowProc - WM_STYLECHANGING to %08x %08x\n",
                 (unsigned int)dwStyle, (unsigned int)dwExStyle);

        /* Get client rect in screen coordinates */
        wi.cbSize = sizeof(WINDOWINFO);
        GetWindowInfo(hwnd, &wi);

        winDebug
            ("winTopLevelWindowProc - WM_STYLECHANGING client area {%d, %d, %d, %d}, {%d x %d}\n",
             (int)wi.rcClient.left, (int)wi.rcClient.top, (int)wi.rcClient.right,
             (int)wi.rcClient.bottom, (int)(wi.rcClient.right - wi.rcClient.left),
             (int)(wi.rcClient.bottom - wi.rcClient.top));

        newWinRect = wi.rcClient;
        if (!AdjustWindowRectEx(&newWinRect, dwStyle, FALSE, dwExStyle))
            winDebug
                ("winTopLevelWindowProc - WM_STYLECHANGING AdjustWindowRectEx failed\n");

        winDebug
            ("winTopLevelWindowProc - WM_STYLECHANGING window area should be {%d, %d, %d, %d}, {%d x %d}\n",
             (int)newWinRect.left, (int)newWinRect.top, (int)newWinRect.right,
             (int)newWinRect.bottom, (int)(newWinRect.right - newWinRect.left),
             (int)(newWinRect.bottom - newWinRect.top));

        /*
           Style change hasn't happened yet, so we can't adjust the window size yet, as the winAdjustXWindow()
           which WM_SIZE does will use the current (unchanged) style.  Instead make a note to change it when
           WM_STYLECHANGED is received...
         */
        pWinPriv->hDwp = BeginDeferWindowPos(1);
        pWinPriv->hDwp =
            DeferWindowPos(pWinPriv->hDwp, hwnd, NULL, newWinRect.left,
                           newWinRect.top, newWinRect.right - newWinRect.left,
                           newWinRect.bottom - newWinRect.top,
                           SWP_NOACTIVATE | SWP_NOZORDER);
    }
        return 0;

    case WM_STYLECHANGED:
    {
        if (pWinPriv->hDwp) {
            EndDeferWindowPos(pWinPriv->hDwp);
            pWinPriv->hDwp = NULL;
        }
        winDebug("winTopLevelWindowProc - WM_STYLECHANGED done\n");
    }
        return 0;

    case WM_MOUSEACTIVATE:

        /* Check if this window needs to be made active when clicked */
        if (!GetProp(pWinPriv->hWnd, WIN_NEEDMANAGE_PROP)) {
#if CYGMULTIWINDOW_DEBUG
            ErrorF("winTopLevelWindowProc - WM_MOUSEACTIVATE - "
                   "MA_NOACTIVATE\n");
#endif

            /* */
            return MA_NOACTIVATE;
        }
        break;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            if (!g_fSoftwareCursor)
                SetCursor(s_pScreenPriv->cursor.handle);
            return TRUE;
        }
        break;


    case WM_DWMCOMPOSITIONCHANGED:
        /* This message is only sent on Vista/W7 */
        CheckForAlpha(hwnd, pWin, s_pScreenInfo);

        return 0;
    default:
        break;
    }

    ret = DefWindowProc(hwnd, message, wParam, lParam);
    /*
     * If the window was minized we get the stack change before the window is restored
     * and so it gets lost. Ensure there stacking order is correct.
     */
    if (needRestack)
        winReorderWindowsMultiWindow();
    return ret;
}
