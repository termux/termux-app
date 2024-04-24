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

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#else
#define HAS_WINSOCK 1
#endif

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/param.h> // for MAX() macro

#ifdef HAS_WINSOCK
#include <X11/Xwinsock.h>
#else
#include <errno.h>
#endif

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xfixes.h>

#include "winclipboard.h"
#include "internal.h"

#define WIN_CONNECT_RETRIES			40
#define WIN_CONNECT_DELAY			4

#define WIN_CLIPBOARD_WINDOW_CLASS		"xwinclip"
#define WIN_CLIPBOARD_WINDOW_TITLE		"xwinclip"
#ifdef HAS_DEVWINDOWS
#define WIN_MSG_QUEUE_FNAME "/dev/windows"
#endif

/*
 * Global variables
 */

static HWND g_hwndClipboard = NULL;

int xfixes_event_base;
int xfixes_error_base;

/*
 * Local function prototypes
 */

static HWND
winClipboardCreateMessagingWindow(xcb_connection_t *conn, xcb_window_t iWindow, ClipboardAtoms *atoms);

static xcb_atom_t
intern_atom(xcb_connection_t *conn, const char *atomName)
{
  xcb_intern_atom_reply_t *atom_reply;
  xcb_intern_atom_cookie_t atom_cookie;
  xcb_atom_t atom = XCB_ATOM_NONE;

  atom_cookie = xcb_intern_atom(conn, 0, strlen(atomName), atomName);
  atom_reply = xcb_intern_atom_reply(conn, atom_cookie, NULL);
  if (atom_reply) {
    atom = atom_reply->atom;
    free(atom_reply);
  }
  return atom;
}

/*
 * Create X11 and Win32 messaging windows, and run message processing loop
 *
 * returns TRUE if shutdown was signalled to loop, FALSE if some error occurred
 */

BOOL
winClipboardProc(char *szDisplay, xcb_auth_info_t *auth_info)
{
    ClipboardAtoms atoms;
    int iReturn;
    HWND hwnd = NULL;
    int iConnectionNumber = 0;
#ifdef HAS_DEVWINDOWS
    int fdMessageQueue = 0;
#else
    struct timeval tvTimeout;
#endif
    fd_set fdsRead;
    int iMaxDescriptor;
    xcb_connection_t *conn;
    xcb_window_t iWindow = XCB_NONE;
    int iSelectError;
    BOOL fShutdown = FALSE;
    ClipboardConversionData data;
    int screen;

    winDebug("winClipboardProc - Hello\n");

    /* Make sure that the display opened */
    conn = xcb_connect_to_display_with_auth_info(szDisplay, auth_info, &screen);
    if (xcb_connection_has_error(conn)) {
        ErrorF("winClipboardProc - Failed opening the display, giving up\n");
        goto winClipboardProc_Done;
    }

    ErrorF("winClipboardProc - xcb_connect () returned and "
           "successfully opened the display.\n");

    /* Get our connection number */
    iConnectionNumber = xcb_get_file_descriptor(conn);

#ifdef HAS_DEVWINDOWS
    /* Open a file descriptor for the windows message queue */
    fdMessageQueue = open(WIN_MSG_QUEUE_FNAME, O_RDONLY);
    if (fdMessageQueue == -1) {
        ErrorF("winClipboardProc - Failed opening %s\n", WIN_MSG_QUEUE_FNAME);
        goto winClipboardProc_Done;
    }

    /* Find max of our file descriptors */
    iMaxDescriptor = MAX(fdMessageQueue, iConnectionNumber) + 1;
#else
    iMaxDescriptor = iConnectionNumber + 1;
#endif

    const xcb_query_extension_reply_t *xfixes_query;
    xfixes_query = xcb_get_extension_data(conn, &xcb_xfixes_id);
    if (!xfixes_query->present)
      ErrorF ("winClipboardProc - XFixes extension not present\n");
    xfixes_event_base = xfixes_query->first_event;
    xfixes_error_base = xfixes_query->first_error;
    /* Must advise server of XFIXES version we require */
    xcb_xfixes_query_version_unchecked(conn, 1, 0);

    /* Create atoms */
    atoms.atomClipboard = intern_atom(conn, "CLIPBOARD");
    atoms.atomLocalProperty = intern_atom(conn, "CYGX_CUT_BUFFER");
    atoms.atomUTF8String = intern_atom(conn, "UTF8_STRING");
    atoms.atomCompoundText = intern_atom(conn, "COMPOUND_TEXT");
    atoms.atomTargets = intern_atom(conn, "TARGETS");
    atoms.atomIncr = intern_atom(conn, "INCR");

    xcb_screen_t *root_screen = xcb_aux_get_screen(conn, screen);
    xcb_window_t root_window_id = root_screen->root;

    /* Create a messaging window */
    iWindow = xcb_generate_id(conn);
    xcb_void_cookie_t cookie = xcb_create_window_checked(conn,
                                                         XCB_COPY_FROM_PARENT,
                                                         iWindow,
                                                         root_window_id,
                                                         1, 1,
                                                         500, 500,
                                                         0,
                                                         XCB_WINDOW_CLASS_INPUT_ONLY,
                                                         XCB_COPY_FROM_PARENT,
                                                         0,
                                                         NULL);

    xcb_generic_error_t *error;
    if ((error = xcb_request_check(conn, cookie))) {
        ErrorF("winClipboardProc - Could not create an X window.\n");
        free(error);
        goto winClipboardProc_Done;
    }

    xcb_icccm_set_wm_name(conn, iWindow, XCB_ATOM_STRING, 8, strlen("xwinclip"), "xwinclip");

    /* Select event types to watch */
    const static uint32_t values[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
    cookie = xcb_change_window_attributes_checked(conn, iWindow, XCB_CW_EVENT_MASK, values);
    if ((error = xcb_request_check(conn, cookie))) {
        ErrorF("winClipboardProc - Could not set event mask on messaging window\n");
        free(error);
    }

    xcb_xfixes_select_selection_input(conn,
                                      iWindow,
                                      XCB_ATOM_PRIMARY,
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE);

    xcb_xfixes_select_selection_input(conn,
                                      iWindow,
                                      atoms.atomClipboard,
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE);

    /* Initialize monitored selection state */
    winClipboardInitMonitoredSelections();
    /* Create Windows messaging window */
    hwnd = winClipboardCreateMessagingWindow(conn, iWindow, &atoms);

    /* Save copy of HWND */
    g_hwndClipboard = hwnd;

    /* Assert ownership of selections if Win32 clipboard is owned */
    if (NULL != GetClipboardOwner()) {
        /* PRIMARY */
        cookie = xcb_set_selection_owner_checked(conn, iWindow, XCB_ATOM_PRIMARY, XCB_CURRENT_TIME);
        if ((error = xcb_request_check(conn, cookie))) {
            ErrorF("winClipboardProc - Could not set PRIMARY owner\n");
            free(error);
            goto winClipboardProc_Done;
        }

        /* CLIPBOARD */
        cookie = xcb_set_selection_owner_checked(conn, iWindow, atoms.atomClipboard, XCB_CURRENT_TIME);
        if ((error = xcb_request_check(conn, cookie))) {
            ErrorF("winClipboardProc - Could not set CLIPBOARD owner\n");
            free(error);
            goto winClipboardProc_Done;
        }
    }

    data.incr = NULL;
    data.incrsize = 0;

    /* Loop for events */
    while (1) {

        /* Process X events */
        winClipboardFlushXEvents(hwnd, iWindow, conn, &data, &atoms);

        /* Process Windows messages */
        if (!winClipboardFlushWindowsMessageQueue(hwnd)) {
          ErrorF("winClipboardProc - winClipboardFlushWindowsMessageQueue trapped "
                       "WM_QUIT message, exiting main loop.\n");
          break;
        }

        /* We need to ensure that all pending requests are sent */
        xcb_flush(conn);

        /* Setup the file descriptor set */
        /*
         * NOTE: You have to do this before every call to select
         *       because select modifies the mask to indicate
         *       which descriptors are ready.
         */
        FD_ZERO(&fdsRead);
        FD_SET(iConnectionNumber, &fdsRead);
#ifdef HAS_DEVWINDOWS
        FD_SET(fdMessageQueue, &fdsRead);
#else
        tvTimeout.tv_sec = 0;
        tvTimeout.tv_usec = 100;
#endif

        /* Wait for a Windows event or an X event */
        iReturn = select(iMaxDescriptor,        /* Highest fds number */
                         &fdsRead,      /* Read mask */
                         NULL,  /* No write mask */
                         NULL,  /* No exception mask */
#ifdef HAS_DEVWINDOWS
                         NULL   /* No timeout */
#else
                         &tvTimeout     /* Set timeout */
#endif
            );

#ifndef HAS_WINSOCK
        iSelectError = errno;
#else
        iSelectError = WSAGetLastError();
#endif

        if (iReturn < 0) {
#ifndef HAS_WINSOCK
            if (iSelectError == EINTR)
#else
            if (iSelectError == WSAEINTR)
#endif
                continue;

            ErrorF("winClipboardProc - Call to select () failed: %d.  "
                   "Bailing.\n", iReturn);
            break;
        }

        if (FD_ISSET(iConnectionNumber, &fdsRead)) {
            winDebug
                ("winClipboardProc - X connection ready, pumping X event queue\n");
        }

#ifdef HAS_DEVWINDOWS
        /* Check for Windows event ready */
        if (FD_ISSET(fdMessageQueue, &fdsRead))
#else
        if (1)
#endif
        {
            winDebug
                ("winClipboardProc - /dev/windows ready, pumping Windows message queue\n");
        }

#ifdef HAS_DEVWINDOWS
        if (!(FD_ISSET(iConnectionNumber, &fdsRead)) &&
            !(FD_ISSET(fdMessageQueue, &fdsRead))) {
            winDebug("winClipboardProc - Spurious wake, select() returned %d\n", iReturn);
        }
#endif
    }

    /* broke out of while loop on a shutdown message */
    fShutdown = TRUE;

 winClipboardProc_Done:
    /* Close our Windows window */
    if (g_hwndClipboard) {
        DestroyWindow(g_hwndClipboard);
    }

    /* Close our X window */
    if (!xcb_connection_has_error(conn) && iWindow) {
        cookie = xcb_destroy_window_checked(conn, iWindow);
        if ((error = xcb_request_check(conn, cookie)))
            ErrorF("winClipboardProc - XDestroyWindow failed.\n");
        else
            ErrorF("winClipboardProc - XDestroyWindow succeeded.\n");
        free(error);
    }

#ifdef HAS_DEVWINDOWS
    /* Close our Win32 message handle */
    if (fdMessageQueue)
        close(fdMessageQueue);
#endif

    /*
     * xcb_disconnect() does not sync, so is safe to call even when we are built
     * into the server.  Unlike XCloseDisplay() there will be no deadlock if the
     * server is in the process of exiting and waiting for this thread to exit.
     */
    if (!xcb_connection_has_error(conn)) {
        /* Close our X display */
        xcb_disconnect(conn);
    }

    /* global clipboard variable reset */
    g_hwndClipboard = NULL;

    return fShutdown;
}

/*
 * Create the Windows window that we use to receive Windows messages
 */

static HWND
winClipboardCreateMessagingWindow(xcb_connection_t *conn, xcb_window_t iWindow, ClipboardAtoms *atoms)
{
    WNDCLASSEX wc;
    ClipboardWindowCreationParams cwcp;
    HWND hwnd;

    /* Setup our window class */
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = winClipboardWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WIN_CLIPBOARD_WINDOW_CLASS;
    wc.hIconSm = 0;
    RegisterClassEx(&wc);

    /* Information to be passed to WM_CREATE */
    cwcp.pClipboardDisplay = conn;
    cwcp.iClipboardWindow = iWindow;
    cwcp.atoms = atoms;

    /* Create the window */
    hwnd = CreateWindowExA(0,   /* Extended styles */
                           WIN_CLIPBOARD_WINDOW_CLASS,  /* Class name */
                           WIN_CLIPBOARD_WINDOW_TITLE,  /* Window name */
                           WS_OVERLAPPED,       /* Not visible anyway */
                           CW_USEDEFAULT,       /* Horizontal position */
                           CW_USEDEFAULT,       /* Vertical position */
                           CW_USEDEFAULT,       /* Right edge */
                           CW_USEDEFAULT,       /* Bottom edge */
                           (HWND) NULL, /* No parent or owner window */
                           (HMENU) NULL,        /* No menu */
                           GetModuleHandle(NULL),       /* Instance handle */
                           &cwcp);       /* Creation data */
    assert(hwnd != NULL);

    /* I'm not sure, but we may need to call this to start message processing */
    ShowWindow(hwnd, SW_HIDE);

    /* Similarly, we may need a call to this even though we don't paint */
    UpdateWindow(hwnd);

    return hwnd;
}

void
winClipboardWindowDestroy(void)
{
  if (g_hwndClipboard) {
    SendMessage(g_hwndClipboard, WM_WM_QUIT, 0, 0);
  }
}
