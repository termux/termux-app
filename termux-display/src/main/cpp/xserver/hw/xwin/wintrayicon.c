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
 * Authors:	Early Ehlinger
 *		Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "win.h"
#include <shellapi.h>
#include "winprefs.h"
#include "winclipboard/winclipboard.h"

/*
 * Initialize the tray icon
 */

void
winInitNotifyIcon(winPrivScreenPtr pScreenPriv)
{
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    NOTIFYICONDATA nid = { 0 };

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = pScreenPriv->hwndScreen;
    nid.uID = pScreenInfo->dwScreen;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = winTaskbarIcon();

    /* Save handle to the icon so it can be freed later */
    pScreenPriv->hiconNotifyIcon = nid.hIcon;

    /* Set display and screen-specific tooltip text */
    snprintf(nid.szTip,
             sizeof(nid.szTip),
             PROJECT_NAME " Server:%s.%d",
             display, (int) pScreenInfo->dwScreen);

    /* Add the tray icon */
    if (!Shell_NotifyIcon(NIM_ADD, &nid))
        ErrorF("winInitNotifyIcon - Shell_NotifyIcon Failed\n");
}

/*
 * Delete the tray icon
 */

void
winDeleteNotifyIcon(winPrivScreenPtr pScreenPriv)
{
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    NOTIFYICONDATA nid = { 0 };

#if 0
    ErrorF("winDeleteNotifyIcon\n");
#endif

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = pScreenPriv->hwndScreen;
    nid.uID = pScreenInfo->dwScreen;

    /* Delete the tray icon */
    if (!Shell_NotifyIcon(NIM_DELETE, &nid)) {
        ErrorF("winDeleteNotifyIcon - Shell_NotifyIcon failed\n");
        return;
    }

    /* Free the icon that was loaded */
    if (pScreenPriv->hiconNotifyIcon != NULL
        && DestroyIcon(pScreenPriv->hiconNotifyIcon) == 0) {
        ErrorF("winDeleteNotifyIcon - DestroyIcon failed\n");
    }
    pScreenPriv->hiconNotifyIcon = NULL;
}

/*
 * Process messages intended for the tray icon
 */

LRESULT
winHandleIconMessage(HWND hwnd, UINT message,
                     WPARAM wParam, LPARAM lParam, winPrivScreenPtr pScreenPriv)
{
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    switch (lParam) {
    case WM_LBUTTONUP:
        /* Restack and bring all windows to top */
        SetForegroundWindow (pScreenPriv->hwndScreen);
        break;

    case WM_LBUTTONDBLCLK:
        /* Display Exit dialog box */
        winDisplayExitDialog(pScreenPriv);
        break;

    case WM_RBUTTONUP:
    {
        POINT ptCursor;
        HMENU hmenuPopup;
        HMENU hmenuTray;

        /* Get cursor position */
        GetCursorPos(&ptCursor);

        /* Load tray icon menu resource */
        hmenuPopup = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDM_TRAYICON_MENU));
        if (!hmenuPopup)
            ErrorF("winHandleIconMessage - LoadMenu failed\n");

        /* Get actual tray icon menu */
        hmenuTray = GetSubMenu(hmenuPopup, 0);

        /* Check for MultiWindow mode */
        if (pScreenInfo->fMultiWindow) {
            MENUITEMINFO mii = { 0 };

            /* Root is shown, remove the check box */

            /* Setup menu item info structure */
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STATE;
            mii.fState = MFS_CHECKED;

            /* Unheck box if root is shown */
            if (pScreenPriv->fRootWindowShown)
                mii.fState = MFS_UNCHECKED;

            /* Set menu state */
            SetMenuItemInfo(hmenuTray, ID_APP_HIDE_ROOT, FALSE, &mii);
        }
        else
        {
            /* Remove Hide Root Window button */
            RemoveMenu(hmenuTray, ID_APP_HIDE_ROOT, MF_BYCOMMAND);
        }

        if (g_fClipboard) {
            /* Set menu state to indicate if 'Monitor Primary' is enabled or not */
            MENUITEMINFO mii = { 0 };
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STATE;
            mii.fState = fPrimarySelection ? MFS_CHECKED : MFS_UNCHECKED;
            SetMenuItemInfo(hmenuTray, ID_APP_MONITOR_PRIMARY, FALSE, &mii);
        }
        else {
            /* Remove 'Monitor Primary' menu item */
            RemoveMenu(hmenuTray, ID_APP_MONITOR_PRIMARY, MF_BYCOMMAND);
        }

        SetupRootMenu(hmenuTray);

        /*
         * NOTE: This three-step procedure is required for
         * proper popup menu operation.  Without the
         * call to SetForegroundWindow the
         * popup menu will often not disappear when you click
         * outside of it.  Without the PostMessage the second
         * time you display the popup menu it might immediately
         * disappear.
         */
        SetForegroundWindow(hwnd);
        TrackPopupMenuEx(hmenuTray,
                         TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                         ptCursor.x, ptCursor.y, hwnd, NULL);
        PostMessage(hwnd, WM_NULL, 0, 0);

        /* Free menu */
        DestroyMenu(hmenuPopup);
    }
        break;
    }

    return 0;
}
