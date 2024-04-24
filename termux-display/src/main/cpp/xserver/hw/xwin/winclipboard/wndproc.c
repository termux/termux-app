/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
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
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the copyright holder(s)
 *and author(s) shall not be used in advertising or otherwise to promote
 *the sale, use or other dealings in this Software without prior written
 *authorization from the copyright holder(s) and author(s).
 *
 * Authors:	Harold L Hunt II
 *              Colin Harrison
 */

#define WINVER 0x0600

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <limits.h>

#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>

#include "internal.h"
#include "winclipboard.h"

/*
 * Constants
 */

#define WIN_POLL_TIMEOUT	1

/*
 * Process X events up to specified timeout
 */

static int
winProcessXEventsTimeout(HWND hwnd, xcb_window_t iWindow, xcb_connection_t *conn,
                         ClipboardConversionData *data, ClipboardAtoms *atoms, int iTimeoutSec)
{
    int iConnNumber;
    struct timeval tv;
    int iReturn;

    winDebug("winProcessXEventsTimeout () - pumping X events, timeout %d seconds\n",
             iTimeoutSec);

    /* Get our connection number */
    iConnNumber = xcb_get_file_descriptor(conn);

    /* Loop for X events */
    while (1) {
        fd_set fdsRead;
        long remainingTime;

        /* Process X events */
        iReturn = winClipboardFlushXEvents(hwnd, iWindow, conn, data, atoms);

        winDebug("winProcessXEventsTimeout () - winClipboardFlushXEvents returned %d\n", iReturn);

        if ((WIN_XEVENTS_NOTIFY_DATA == iReturn) || (WIN_XEVENTS_NOTIFY_TARGETS == iReturn) || (WIN_XEVENTS_FAILED == iReturn)) {
          /* Bail out */
          return iReturn;
        }

        /* We need to ensure that all pending requests are sent */
        xcb_flush(conn);

        /* Setup the file descriptor set */
        FD_ZERO(&fdsRead);
        FD_SET(iConnNumber, &fdsRead);

        /* Adjust timeout */
        remainingTime = iTimeoutSec * 1000;
        tv.tv_sec = remainingTime / 1000;
        tv.tv_usec = (remainingTime % 1000) * 1000;

        /* Break out if no time left */
        if (remainingTime <= 0)
            return WIN_XEVENTS_SUCCESS;

        /* Wait for an X event */
        iReturn = select(iConnNumber + 1,       /* Highest fds number */
                         &fdsRead,      /* Read mask */
                         NULL,  /* No write mask */
                         NULL,  /* No exception mask */
                         &tv);  /* Timeout */
        if (iReturn < 0) {
            ErrorF("winProcessXEventsTimeout - Call to select () failed: %d.  "
                   "Bailing.\n", iReturn);
            break;
        }

        if (!FD_ISSET(iConnNumber, &fdsRead)) {
            winDebug("winProcessXEventsTimeout - Spurious wake, select() returned %d\n", iReturn);
        }
    }

    return WIN_XEVENTS_SUCCESS;
}

/*
 * Process a given Windows message
 */

LRESULT CALLBACK
winClipboardWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static xcb_connection_t *conn;
    static xcb_window_t iWindow;
    static ClipboardAtoms *atoms;
    static BOOL fRunning;

    /* Branch on message type */
    switch (message) {
    case WM_DESTROY:
    {
        winDebug("winClipboardWindowProc - WM_DESTROY\n");

        /* Remove clipboard listener */
        RemoveClipboardFormatListener(hwnd);
    }
        return 0;

    case WM_WM_QUIT:
    {
        winDebug("winClipboardWindowProc - WM_WM_QUIT\n");
        fRunning = FALSE;
        PostQuitMessage(0);
    }
        return 0;

    case WM_CREATE:
    {
        ClipboardWindowCreationParams *cwcp = (ClipboardWindowCreationParams *)((CREATESTRUCT *)lParam)->lpCreateParams;

        winDebug("winClipboardWindowProc - WM_CREATE\n");

        conn = cwcp->pClipboardDisplay;
        iWindow = cwcp->iClipboardWindow;
        atoms = cwcp->atoms;
        fRunning = TRUE;

        AddClipboardFormatListener(hwnd);
    }
        return 0;

    case WM_CLIPBOARDUPDATE:
    {
        xcb_generic_error_t *error;
        xcb_void_cookie_t cookie_set;

        winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE: Enter\n");

        /*
         * NOTE: We cannot bail out when NULL == GetClipboardOwner ()
         * because some applications deal with the clipboard in a manner
         * that causes the clipboard owner to be NULL when they are in
         * fact taking ownership.  One example of this is the Win32
         * native compile of emacs.
         */

        /* Bail when we still own the clipboard */
        if (hwnd == GetClipboardOwner()) {

            winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE - "
                     "We own the clipboard, returning.\n");
            winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE: Exit\n");

            return 0;
        }

        /* Bail when shutting down */
        if (!fRunning)
            return 0;

        /*
         * Do not take ownership of the X11 selections when something
         * other than CF_TEXT or CF_UNICODETEXT has been copied
         * into the Win32 clipboard.
         */
        if (!IsClipboardFormatAvailable(CF_TEXT)
            && !IsClipboardFormatAvailable(CF_UNICODETEXT)) {

            xcb_get_selection_owner_cookie_t cookie_get;
            xcb_get_selection_owner_reply_t *reply;

            winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE - "
                     "Clipboard does not contain CF_TEXT nor "
                     "CF_UNICODETEXT.\n");

            /*
             * We need to make sure that the X Server has processed
             * previous XSetSelectionOwner messages.
             */
            xcb_aux_sync(conn);

            winDebug("winClipboardWindowProc - XSync done.\n");

            /* Release PRIMARY selection if owned */
            cookie_get = xcb_get_selection_owner(conn, XCB_ATOM_PRIMARY);
            reply = xcb_get_selection_owner_reply(conn, cookie_get, NULL);
            if (reply) {
                if (reply->owner == iWindow) {
                    winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE - "
                             "PRIMARY selection is owned by us, releasing.\n");
                    xcb_set_selection_owner(conn, XCB_NONE, XCB_ATOM_PRIMARY, XCB_CURRENT_TIME);
                }
                free(reply);
            }

            /* Release CLIPBOARD selection if owned */
            cookie_get = xcb_get_selection_owner(conn, atoms->atomClipboard);
            reply = xcb_get_selection_owner_reply(conn, cookie_get, NULL);
            if (reply) {
                if (reply->owner == iWindow) {
                    winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE - "
                             "CLIPBOARD selection is owned by us, releasing\n");
                    xcb_set_selection_owner(conn, XCB_NONE, atoms->atomClipboard, XCB_CURRENT_TIME);
                }
                free(reply);
            }

            winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE: Exit\n");

            return 0;
        }

        /* Reassert ownership of PRIMARY */
        cookie_set = xcb_set_selection_owner_checked(conn, iWindow, XCB_ATOM_PRIMARY, XCB_CURRENT_TIME);
        error = xcb_request_check(conn, cookie_set);
        if (error) {
            ErrorF("winClipboardWindowProc - WM_CLIPBOARDUPDATE - "
                   "Could not reassert ownership of PRIMARY\n");
            free(error);
        } else {
            winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE - "
                     "Reasserted ownership of PRIMARY\n");
        }

        /* Reassert ownership of the CLIPBOARD */
        cookie_set = xcb_set_selection_owner_checked(conn, iWindow, atoms->atomClipboard, XCB_CURRENT_TIME);
        error = xcb_request_check(conn, cookie_set);
        if (error) {
            ErrorF("winClipboardWindowProc - WM_CLIPBOARDUPDATE - "
                    "Could not reassert ownership of CLIPBOARD\n");
            free(error);
        }
        else {
            winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE - "
                     "Reasserted ownership of CLIPBOARD\n");
        }

        /* Flush the pending SetSelectionOwner event now */
        xcb_flush(conn);
    }
        winDebug("winClipboardWindowProc - WM_CLIPBOARDUPDATE: Exit\n");
        return 0;

    case WM_DESTROYCLIPBOARD:
        /*
         * NOTE: Intentionally do nothing.
         * Changes in the Win32 clipboard are handled by WM_CLIPBOARDUPDATE
         * above.  We only process this message to conform to the specs
         * for delayed clipboard rendering in Win32.  You might think
         * that we need to release ownership of the X11 selections, but
         * we do not, because a WM_CLIPBOARDUPDATE message will closely
         * follow this message and reassert ownership of the X11
         * selections, handling the issue for us.
         */
        winDebug("winClipboardWindowProc - WM_DESTROYCLIPBOARD - Ignored.\n");
        return 0;

    case WM_RENDERALLFORMATS:
        winDebug("winClipboardWindowProc - WM_RENDERALLFORMATS - Hello.\n");

        /*
          WM_RENDERALLFORMATS is sent as we are shutting down, to render the
          clipboard so its contents remains available to other applications.

          Unfortunately, this can't work without major changes. The server is
          already waiting for us to stop, so we can't ask for the rendering of
          clipboard text now.
        */

        return 0;

    case WM_RENDERFORMAT:
    {
        int iReturn;
        BOOL pasted = FALSE;
        xcb_atom_t selection;
        ClipboardConversionData data;
        int best_target = 0;

        winDebug("winClipboardWindowProc - WM_RENDERFORMAT %d - Hello.\n",
                 (int)wParam);

        selection = winClipboardGetLastOwnedSelectionAtom(atoms);
        if (selection == XCB_NONE) {
            ErrorF("winClipboardWindowProc - no monitored selection is owned\n");
            goto fake_paste;
        }

        winDebug("winClipboardWindowProc - requesting targets for selection from owner\n");

        /* Request the selection's supported conversion targets */
        xcb_convert_selection(conn, iWindow, selection, atoms->atomTargets,
                              atoms->atomLocalProperty, XCB_CURRENT_TIME);

        /* Process X events */
        data.incr = NULL;
        data.incrsize = 0;

        iReturn = winProcessXEventsTimeout(hwnd,
                                           iWindow,
                                           conn,
                                           &data,
                                           atoms,
                                           WIN_POLL_TIMEOUT);

        if (WIN_XEVENTS_NOTIFY_TARGETS != iReturn) {
            ErrorF
                ("winClipboardWindowProc - timed out waiting for WIN_XEVENTS_NOTIFY_TARGETS\n");
            goto fake_paste;
        }

        /* Choose the most preferred target */
        {
            struct target_priority
            {
                xcb_atom_t target;
                unsigned int priority;
            };

            struct target_priority target_priority_table[] =
                {
                    { atoms->atomUTF8String,   0 },
                    // { atoms->atomCompoundText, 1 }, not implemented (yet?)
                    { XCB_ATOM_STRING,         2 },
                };

            int best_priority = INT_MAX;

            int i,j;
            for (i = 0 ; data.targetList[i] != 0; i++)
                {
                    for (j = 0; j < ARRAY_SIZE(target_priority_table); j ++)
                        {
                            if ((data.targetList[i] == target_priority_table[j].target) &&
                                (target_priority_table[j].priority < best_priority))
                                {
                                    best_target = target_priority_table[j].target;
                                    best_priority = target_priority_table[j].priority;
                                }
                        }
                }
        }

        free(data.targetList);
        data.targetList = 0;

        winDebug("winClipboardWindowProc - best target is %d\n", best_target);

        /* No useful targets found */
        if (best_target == 0)
          goto fake_paste;

        winDebug("winClipboardWindowProc - requesting selection from owner\n");

        /* Request the selection contents */
        xcb_convert_selection(conn, iWindow, selection, best_target,
                              atoms->atomLocalProperty, XCB_CURRENT_TIME);

        /* Process X events */
        iReturn = winProcessXEventsTimeout(hwnd,
                                           iWindow,
                                           conn,
                                           &data,
                                           atoms,
                                           WIN_POLL_TIMEOUT);

        /*
         * winProcessXEventsTimeout had better have seen a notify event,
         * or else we are dealing with a buggy or old X11 app.
         */
        if (WIN_XEVENTS_NOTIFY_DATA != iReturn) {
            ErrorF
                ("winClipboardWindowProc - timed out waiting for WIN_XEVENTS_NOTIFY_DATA\n");
        }
        else {
            pasted = TRUE;
        }

         /*
          * If we couldn't get the data from the X clipboard, we
          * have to paste some fake data to the Win32 clipboard to
          * satisfy the requirement that we write something to it.
          */
    fake_paste:
        if (!pasted)
          {
            /* Paste no data, to satisfy required call to SetClipboardData */
            SetClipboardData(CF_UNICODETEXT, NULL);
            SetClipboardData(CF_TEXT, NULL);
          }

        winDebug("winClipboardWindowProc - WM_RENDERFORMAT - Returning.\n");
        return 0;
    }
    }

    /* Let Windows perform default processing for unhandled messages */
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/*
 * Process any pending Windows messages
 */

BOOL
winClipboardFlushWindowsMessageQueue(HWND hwnd)
{
    MSG msg;

    /* Flush the messaging window queue */
    /* NOTE: Do not pass the hwnd of our messaging window to PeekMessage,
     * as this will filter out many non-window-specific messages that
     * are sent to our thread, such as WM_QUIT.
     */
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        /* Dispatch the message if not WM_QUIT */
        if (msg.message == WM_QUIT)
            return FALSE;
        else
            DispatchMessage(&msg);
    }

    return TRUE;
}
