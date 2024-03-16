/*
 * Copyright (C) Jon TURNEY 2011
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "win.h"

/*
 * This is the messaging window, a hidden top-level window. We never do anything
 * with it, but other programs may send messages to it.
 */

/*
 * winMsgWindowProc - Window procedure for msg window
 */

static
LRESULT CALLBACK
winMsgWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#if CYGDEBUG
    winDebugWin32Message("winMsgWindowProc", hwnd, message, wParam, lParam);
#endif

    switch (message) {
    case WM_ENDSESSION:
        if (!wParam)
            return 0;           /* shutdown is being cancelled */

        /*
           Send a WM_GIVEUP message to the X server thread so it wakes up if
           blocked in select(), performs GiveUp(), and then notices that GiveUp()
           has set the DE_TERMINATE flag so exits the msg dispatch loop.
         */
        {
            ScreenPtr pScreen = screenInfo.screens[0];

            winScreenPriv(pScreen);
            PostMessage(pScreenPriv->hwndScreen, WM_GIVEUP, 0, 0);
        }

        /*
           This process will be terminated by the system almost immediately
           after the last thread with a message queue returns from processing
           WM_ENDSESSION, so we cannot rely on any code executing after this
           message is processed and need to wait here until ddxGiveUp() is called
           and releases the termination mutex to guarantee that the lock file and
           unix domain sockets have been removed

           ofc, Microsoft doesn't document this under WM_ENDSESSION, you are supposed
           to read the source of CRSS to find out how it works :-)

           http://blogs.msdn.com/b/michen/archive/2008/04/04/application-termination-when-user-logs-off.aspx
         */
        {
            int iReturn = pthread_mutex_lock(&g_pmTerminating);

            if (iReturn != 0) {
                ErrorF("winMsgWindowProc - pthread_mutex_lock () failed: %d\n",
                       iReturn);
            }
            winDebug
                ("winMsgWindowProc - WM_ENDSESSION termination lock acquired\n");
        }

        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

static HWND
winCreateMsgWindow(void)
{
    HWND hwndMsg;

    // register window class
    {
        WNDCLASSEX wcx;

        wcx.cbSize = sizeof(WNDCLASSEX);
        wcx.style = CS_HREDRAW | CS_VREDRAW;
        wcx.lpfnWndProc = winMsgWindowProc;
        wcx.cbClsExtra = 0;
        wcx.cbWndExtra = 0;
        wcx.hInstance = g_hInstance;
        wcx.hIcon = NULL;
        wcx.hCursor = 0;
        wcx.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
        wcx.lpszMenuName = NULL;
        wcx.lpszClassName = WINDOW_CLASS_X_MSG;
        wcx.hIconSm = NULL;
        RegisterClassEx(&wcx);
    }

    // Create the msg window.
    hwndMsg = CreateWindowEx(0, // no extended styles
                             WINDOW_CLASS_X_MSG,        // class name
                             "XWin Msg Window", // window name
                             WS_OVERLAPPEDWINDOW,       // overlapped window
                             CW_USEDEFAULT,     // default horizontal position
                             CW_USEDEFAULT,     // default vertical position
                             CW_USEDEFAULT,     // default width
                             CW_USEDEFAULT,     // default height
                             (HWND) NULL,       // no parent or owner window
                             (HMENU) NULL,      // class menu used
                             GetModuleHandle(NULL),     // instance handle
                             NULL);     // no window creation data

    if (!hwndMsg) {
        ErrorF("winCreateMsgWindow - Create msg window failed\n");
        return NULL;
    }

    winDebug("winCreateMsgWindow - Created msg window hwnd 0x%p\n", hwndMsg);

    return hwndMsg;
}

static void *
winMsgWindowThreadProc(void *arg)
{
    HWND hwndMsg;

    winDebug("winMsgWindowThreadProc - Hello\n");

    hwndMsg = winCreateMsgWindow();
    if (hwndMsg) {
        MSG msg;

        /* Pump the msg window message queue */
        while (GetMessage(&msg, hwndMsg, 0, 0) > 0) {
#if CYGDEBUG
            winDebugWin32Message("winMsgWindowThread", msg.hwnd, msg.message,
                                 msg.wParam, msg.lParam);
#endif
            DispatchMessage(&msg);
        }
    }

    winDebug("winMsgWindowThreadProc - Exit\n");

    return NULL;
}

Bool
winCreateMsgWindowThread(void)
{
    pthread_t ptMsgWindowThreadProc;

    /* Spawn a thread for the msg window  */
    if (pthread_create(&ptMsgWindowThreadProc,
                       NULL, winMsgWindowThreadProc, NULL)) {
        /* Bail if thread creation failed */
        ErrorF("winCreateMsgWindow - pthread_create failed.\n");
        return FALSE;
    }

    return TRUE;
}
