/*
 *Copyright (C) 2004 Harold L Hunt II All Rights Reserved.
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

static HHOOK g_hhookKeyboardLL = NULL;

/*
 * Function prototypes
 */

static LRESULT CALLBACK
winKeyboardMessageHookLL(int iCode, WPARAM wParam, LPARAM lParam);

#ifndef LLKHF_EXTENDED
#define LLKHF_EXTENDED  0x00000001
#endif
#ifndef LLKHF_UP
#define LLKHF_UP  0x00000080
#endif

/*
 * KeyboardMessageHook
 */

static LRESULT CALLBACK
winKeyboardMessageHookLL(int iCode, WPARAM wParam, LPARAM lParam)
{
    BOOL fPassKeystroke = FALSE;
    BOOL fPassAltTab = TRUE;
    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT) lParam;
    HWND hwnd = GetActiveWindow();

    WindowPtr pWin = NULL;
    winPrivWinPtr pWinPriv = NULL;
    winPrivScreenPtr pScreenPriv = NULL;
    winScreenInfo *pScreenInfo = NULL;

    /* Check if the Windows window property for our X window pointer is valid */
    if ((pWin = GetProp(hwnd, WIN_WINDOW_PROP)) != NULL) {
        /* Get a pointer to our window privates */
        pWinPriv = winGetWindowPriv(pWin);

        /* Get pointers to our screen privates and screen info */
        pScreenPriv = pWinPriv->pScreenPriv;
        pScreenInfo = pScreenPriv->pScreenInfo;

        if (pScreenInfo->fMultiWindow)
            fPassAltTab = FALSE;
    }

    /* Pass keystrokes on to our main message loop */
    if (iCode == HC_ACTION) {
        winDebug("winKeyboardMessageHook: vkCode: %08x scanCode: %08x\n",
                 (unsigned int)p->vkCode, (unsigned int)p->scanCode);

        switch (wParam) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            fPassKeystroke =
                (fPassAltTab &&
                 (p->vkCode == VK_TAB) && ((p->flags & LLKHF_ALTDOWN) != 0))
                || (p->vkCode == VK_LWIN) || (p->vkCode == VK_RWIN);
            break;
        }
    }

    /*
     * Pass message on to our main message loop.
     * We process this immediately with SendMessage so that the keystroke
     * appears in, hopefully, the correct order.
     */
    if (fPassKeystroke) {
        LPARAM lParamKey = 0x0;

        /* Construct the lParam from KBDLLHOOKSTRUCT */
        lParamKey = lParamKey | (0x0000FFFF & 0x00000001);      /* Repeat count */
        lParamKey = lParamKey | (0x00FF0000 & (p->scanCode << 16));
        lParamKey = lParamKey
            | (0x01000000 & ((p->flags & LLKHF_EXTENDED) << 23));
        lParamKey = lParamKey
            | (0x20000000 & ((p->flags & LLKHF_ALTDOWN) << 24));
        lParamKey = lParamKey | (0x80000000 & ((p->flags & LLKHF_UP) << 24));

        /* Send message to our main window that has the keyboard focus */
        PostMessage(hwnd, (UINT) wParam, (WPARAM) p->vkCode, lParamKey);

        return 1;
    }

    /* Call next hook */
    return CallNextHookEx(NULL, iCode, wParam, lParam);
}

/*
 * Attempt to install the keyboard hook, return FALSE if it was not installed
 */

Bool
winInstallKeyboardHookLL(void)
{
    /* Install the hook only once */
    if (!g_hhookKeyboardLL)
        g_hhookKeyboardLL = SetWindowsHookEx(WH_KEYBOARD_LL,
                                             winKeyboardMessageHookLL,
                                             g_hInstance, 0);

    return TRUE;
}

/*
 * Remove the keyboard hook if it is installed
 */

void
winRemoveKeyboardHookLL(void)
{
    if (g_hhookKeyboardLL)
        UnhookWindowsHookEx(g_hhookKeyboardLL);
    g_hhookKeyboardLL = NULL;
}
