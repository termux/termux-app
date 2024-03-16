/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *Copyright (C) Colin Harrison 2005-2009
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
 *              Colin Harrison
 */

/* X headers */
#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __CYGWIN__
#include <sys/select.h>
#endif
#include <fcntl.h>
#include <setjmp.h>
#define HANDLE void *
#include <pthread.h>
#undef HANDLE
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_aux.h>
#include <xcb/composite.h>

#include <X11/Xwindows.h>

/* Local headers */
#include "X11/Xdefs.h" // for Bool type
#include "winwindow.h"
#include "winprefs.h"
#include "window.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "winglobals.h"
#include "windisplay.h"
#include "winmultiwindowicons.h"
#include "winauth.h"

/* We need the native HWND atom for intWM, so for consistency use the
   same name as extWM does */
#define WINDOWSWM_NATIVE_HWND "_WINDOWSWM_NATIVE_HWND"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

extern void winDebug(const char *format, ...);
extern void winReshapeMultiWindow(WindowPtr pWin);
extern void winUpdateRgnMultiWindow(WindowPtr pWin);

#ifndef CYGDEBUG
#define CYGDEBUG NO
#endif

/*
 * Constant defines
 */

#define WIN_CONNECT_RETRIES	5
#define WIN_CONNECT_DELAY	5
#ifdef HAS_DEVWINDOWS
#define WIN_MSG_QUEUE_FNAME	"/dev/windows"
#endif

/*
 * Local structures
 */

typedef struct _WMMsgNodeRec {
    winWMMessageRec msg;
    struct _WMMsgNodeRec *pNext;
} WMMsgNodeRec, *WMMsgNodePtr;

typedef struct _WMMsgQueueRec {
    struct _WMMsgNodeRec *pHead;
    struct _WMMsgNodeRec *pTail;
    pthread_mutex_t pmMutex;
    pthread_cond_t pcNotEmpty;
} WMMsgQueueRec, *WMMsgQueuePtr;

typedef struct _WMInfo {
    xcb_connection_t *conn;
    WMMsgQueueRec wmMsgQueue;
    xcb_atom_t atmWmProtos;
    xcb_atom_t atmWmDelete;
    xcb_atom_t atmWmTakeFocus;
    xcb_atom_t atmPrivMap;
    xcb_atom_t atmUtf8String;
    xcb_atom_t atmNetWmName;
    xcb_atom_t atmCurrentDesktop;
    xcb_atom_t atmNumberDesktops;
    xcb_atom_t atmDesktopNames;
    xcb_ewmh_connection_t ewmh;
    Bool fCompositeWM;
} WMInfoRec, *WMInfoPtr;

typedef struct _WMProcArgRec {
    DWORD dwScreen;
    WMInfoPtr pWMInfo;
    pthread_mutex_t *ppmServerStarted;
} WMProcArgRec, *WMProcArgPtr;

typedef struct _XMsgProcArgRec {
    xcb_connection_t *conn;
    DWORD dwScreen;
    WMInfoPtr pWMInfo;
    pthread_mutex_t *ppmServerStarted;
    HWND hwndScreen;
} XMsgProcArgRec, *XMsgProcArgPtr;

/*
 * Prototypes for local functions
 */

static void
 PushMessage(WMMsgQueuePtr pQueue, WMMsgNodePtr pNode);

static WMMsgNodePtr PopMessage(WMMsgQueuePtr pQueue, WMInfoPtr pWMInfo);

static Bool
 InitQueue(WMMsgQueuePtr pQueue);

static void
 GetWindowName(WMInfoPtr pWMInfo, xcb_window_t iWin, char **ppWindowName);

static void
 SendXMessage(xcb_connection_t *conn, xcb_window_t iWin, xcb_atom_t atmType, long nData);

static void
 UpdateName(WMInfoPtr pWMInfo, xcb_window_t iWindow);

static void *winMultiWindowWMProc(void *pArg);

static void *winMultiWindowXMsgProc(void *pArg);

static void
 winInitMultiWindowWM(WMInfoPtr pWMInfo, WMProcArgPtr pProcArg);

#if 0
static void
 PreserveWin32Stack(WMInfoPtr pWMInfo, xcb_window_t iWindow, UINT direction);
#endif

static Bool
CheckAnotherWindowManager(xcb_connection_t *conn, DWORD dwScreen);

static void
 winApplyHints(WMInfoPtr pWMInfo, xcb_window_t iWindow, HWND hWnd, HWND * zstyle);

void
 winUpdateWindowPosition(HWND hWnd, HWND * zstyle);

/*
 * Local globals
 */

static Bool g_shutdown = FALSE;

/*
 * Translate msg id to text, for debug purposes
 */

#if CYGMULTIWINDOW_DEBUG
static const char *
MessageName(winWMMessagePtr msg)
{
  switch (msg->msg)
    {
    case WM_WM_MOVE:
      return "WM_WM_MOVE";
      break;
    case WM_WM_SIZE:
      return "WM_WM_SIZE";
      break;
    case WM_WM_RAISE:
      return "WM_WM_RAISE";
      break;
    case WM_WM_LOWER:
      return "WM_WM_LOWER";
      break;
    case WM_WM_UNMAP:
      return "WM_WM_UNMAP";
      break;
    case WM_WM_KILL:
      return "WM_WM_KILL";
      break;
    case WM_WM_ACTIVATE:
      return "WM_WM_ACTIVATE";
      break;
    case WM_WM_NAME_EVENT:
      return "WM_WM_NAME_EVENT";
      break;
    case WM_WM_ICON_EVENT:
      return "WM_WM_ICON_EVENT";
      break;
    case WM_WM_CHANGE_STATE:
      return "WM_WM_CHANGE_STATE";
      break;
    case WM_WM_MAP_UNMANAGED:
      return "WM_WM_MAP_UNMANAGED";
      break;
    case WM_WM_MAP_MANAGED:
      return "WM_WM_MAP_MANAGED";
      break;
    case WM_WM_HINTS_EVENT:
      return "WM_WM_HINTS_EVENT";
      break;
    default:
      return "Unknown Message";
      break;
    }
}
#endif


/*
 * PushMessage - Push a message onto the queue
 */

static void
PushMessage(WMMsgQueuePtr pQueue, WMMsgNodePtr pNode)
{

    /* Lock the queue mutex */
    pthread_mutex_lock(&pQueue->pmMutex);

    pNode->pNext = NULL;

    if (pQueue->pTail != NULL) {
        pQueue->pTail->pNext = pNode;
    }
    pQueue->pTail = pNode;

    if (pQueue->pHead == NULL) {
        pQueue->pHead = pNode;
    }

    /* Release the queue mutex */
    pthread_mutex_unlock(&pQueue->pmMutex);

    /* Signal that the queue is not empty */
    pthread_cond_signal(&pQueue->pcNotEmpty);
}

/*
 * PopMessage - Pop a message from the queue
 */

static WMMsgNodePtr
PopMessage(WMMsgQueuePtr pQueue, WMInfoPtr pWMInfo)
{
    WMMsgNodePtr pNode;

    /* Lock the queue mutex */
    pthread_mutex_lock(&pQueue->pmMutex);

    /* Wait for --- */
    while (pQueue->pHead == NULL) {
        pthread_cond_wait(&pQueue->pcNotEmpty, &pQueue->pmMutex);
    }

    pNode = pQueue->pHead;
    if (pQueue->pHead != NULL) {
        pQueue->pHead = pQueue->pHead->pNext;
    }

    if (pQueue->pTail == pNode) {
        pQueue->pTail = NULL;
    }

    /* Release the queue mutex */
    pthread_mutex_unlock(&pQueue->pmMutex);

    return pNode;
}

#if 0
/*
 * HaveMessage -
 */

static Bool
HaveMessage(WMMsgQueuePtr pQueue, UINT msg, xcb_window_t iWindow)
{
    WMMsgNodePtr pNode;

    for (pNode = pQueue->pHead; pNode != NULL; pNode = pNode->pNext) {
        if (pNode->msg.msg == msg && pNode->msg.iWindow == iWindow)
            return True;
    }

    return False;
}
#endif

/*
 * InitQueue - Initialize the Window Manager message queue
 */

static
    Bool
InitQueue(WMMsgQueuePtr pQueue)
{
    /* Check if the pQueue pointer is NULL */
    if (pQueue == NULL) {
        ErrorF("InitQueue - pQueue is NULL.  Exiting.\n");
        return FALSE;
    }

    /* Set the head and tail to NULL */
    pQueue->pHead = NULL;
    pQueue->pTail = NULL;

    winDebug("InitQueue - Calling pthread_mutex_init\n");

    /* Create synchronization objects */
    pthread_mutex_init(&pQueue->pmMutex, NULL);

    winDebug("InitQueue - pthread_mutex_init returned\n");
    winDebug("InitQueue - Calling pthread_cond_init\n");

    pthread_cond_init(&pQueue->pcNotEmpty, NULL);

    winDebug("InitQueue - pthread_cond_init returned\n");

    return TRUE;
}

static
char *
Xutf8TextPropertyToString(WMInfoPtr pWMInfo, xcb_icccm_get_text_property_reply_t *xtp)
{
    char *pszReturnData;

    if ((xtp->encoding == XCB_ATOM_STRING) ||        // Latin1 ISO 8859-1
        (xtp->encoding == pWMInfo->atmUtf8String)) { // UTF-8  ISO 10646
        pszReturnData = strndup(xtp->name, xtp->name_len);
    }
    else {
        // Converting from COMPOUND_TEXT to UTF-8 properly is complex to
        // implement, and not very much use unless you have an old
        // application which isn't UTF-8 aware.
        ErrorF("Xutf8TextPropertyToString: text encoding %d is not implemented\n", xtp->encoding);
        pszReturnData = strdup("");
    }

    return pszReturnData;
}

/*
 * GetWindowName - Retrieve the title of an X Window
 */

static void
GetWindowName(WMInfoPtr pWMInfo, xcb_window_t iWin, char **ppWindowName)
{
    xcb_connection_t *conn = pWMInfo->conn;
    char *pszWindowName = NULL;

#if CYGMULTIWINDOW_DEBUG
    ErrorF("GetWindowName\n");
#endif

    /* Try to get window name from _NET_WM_NAME */
    {
        xcb_get_property_cookie_t cookie;
        xcb_get_property_reply_t *reply;

        cookie = xcb_get_property(pWMInfo->conn, FALSE, iWin,
                                  pWMInfo->atmNetWmName,
                                  XCB_GET_PROPERTY_TYPE_ANY, 0, INT_MAX);
        reply = xcb_get_property_reply(pWMInfo->conn, cookie, NULL);
        if (reply && (reply->type != XCB_NONE)) {
            pszWindowName = strndup(xcb_get_property_value(reply),
                                    xcb_get_property_value_length(reply));
            free(reply);
        }
    }

    /* Otherwise, try to get window name from WM_NAME */
    if (!pszWindowName)
        {
            xcb_get_property_cookie_t cookie;
            xcb_icccm_get_text_property_reply_t reply;

            cookie = xcb_icccm_get_wm_name(conn, iWin);
            if (!xcb_icccm_get_wm_name_reply(conn, cookie, &reply, NULL)) {
                ErrorF("GetWindowName - xcb_icccm_get_wm_name_reply failed.  No name.\n");
                *ppWindowName = NULL;
                return;
            }

            pszWindowName = Xutf8TextPropertyToString(pWMInfo, &reply);
            xcb_icccm_get_text_property_reply_wipe(&reply);
        }

    /* return the window name, unless... */
    *ppWindowName = pszWindowName;

    if (g_fHostInTitle) {
        xcb_get_property_cookie_t cookie;
        xcb_icccm_get_text_property_reply_t reply;

        /* Try to get client machine name */
        cookie = xcb_icccm_get_wm_client_machine(conn, iWin);
        if (xcb_icccm_get_wm_client_machine_reply(conn, cookie, &reply, NULL)) {
            char *pszClientMachine;
            char *pszClientHostname;
            char *dot;
            char hostname[HOST_NAME_MAX + 1];

            pszClientMachine = Xutf8TextPropertyToString(pWMInfo, &reply);
            xcb_icccm_get_text_property_reply_wipe(&reply);

            /* If client machine name looks like a FQDN, find the hostname */
            pszClientHostname = strdup(pszClientMachine);
            dot = strchr(pszClientHostname, '.');
            if (dot)
                *dot = '\0';

            /*
               If we have a client machine hostname
               and it's not the local hostname
               and it's not already in the window title...
             */
            if (strlen(pszClientHostname) &&
                !gethostname(hostname, HOST_NAME_MAX + 1) &&
                strcmp(hostname, pszClientHostname) &&
                (strstr(pszWindowName, pszClientHostname) == 0)) {
                /* ... add '@<clientmachine>' to end of window name */
                *ppWindowName =
                    malloc(strlen(pszWindowName) +
                           strlen(pszClientMachine) + 2);
                strcpy(*ppWindowName, pszWindowName);
                strcat(*ppWindowName, "@");
                strcat(*ppWindowName, pszClientMachine);

                free(pszWindowName);
            }

            free(pszClientMachine);
            free(pszClientHostname);
        }
    }
}

/*
 * Does the client support the specified WM_PROTOCOLS protocol?
 */

static Bool
IsWmProtocolAvailable(WMInfoPtr pWMInfo, xcb_window_t iWindow, xcb_atom_t atmProtocol)
{
  int i, found = 0;
  xcb_get_property_cookie_t cookie;
  xcb_icccm_get_wm_protocols_reply_t reply;
  xcb_connection_t *conn = pWMInfo->conn;

  cookie = xcb_icccm_get_wm_protocols(conn, iWindow, pWMInfo->ewmh.WM_PROTOCOLS);
  if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &reply, NULL)) {
    for (i = 0; i < reply.atoms_len; ++i)
      if (reply.atoms[i] == atmProtocol) {
              ++found;
              break;
      }
    xcb_icccm_get_wm_protocols_reply_wipe(&reply);
  }

  return found > 0;
}

/*
 * Send a message to the X server from the WM thread
 */

static void
SendXMessage(xcb_connection_t *conn, xcb_window_t iWin, xcb_atom_t atmType, long nData)
{
    xcb_client_message_event_t e;

    /* Prepare the X event structure */
    memset(&e, 0, sizeof(e));
    e.response_type = XCB_CLIENT_MESSAGE;
    e.window = iWin;
    e.type = atmType;
    e.format = 32;
    e.data.data32[0] = nData;
    e.data.data32[1] = XCB_CURRENT_TIME;

    /* Send the event to X */
    xcb_send_event(conn, FALSE, iWin, XCB_EVENT_MASK_NO_EVENT, (const char *)&e);
}

/*
 * See if we can get the stored HWND for this window...
 */
static HWND
getHwnd(WMInfoPtr pWMInfo, xcb_window_t iWindow)
{
    HWND hWnd = NULL;
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t *reply;

    cookie = xcb_get_property(pWMInfo->conn, FALSE, iWindow, pWMInfo->atmPrivMap,
                              XCB_ATOM_INTEGER, 0L, sizeof(HWND)/4L);
    reply = xcb_get_property_reply(pWMInfo->conn, cookie, NULL);

    if (reply) {
        int length = xcb_get_property_value_length(reply);
        HWND *value = xcb_get_property_value(reply);

        if (value && (length == sizeof(HWND))) {
            hWnd = *value;
        }
        free(reply);
    }

    /* Some sanity checks */
    if (!hWnd)
        return NULL;
    if (!IsWindow(hWnd))
        return NULL;

    return hWnd;
}

/*
 * Helper function to check for override-redirect
 */
static Bool
IsOverrideRedirect(xcb_connection_t *conn, xcb_window_t iWin)
{
    Bool result = FALSE;
    xcb_get_window_attributes_reply_t *reply;
    xcb_get_window_attributes_cookie_t cookie;

    cookie = xcb_get_window_attributes(conn, iWin);
    reply = xcb_get_window_attributes_reply(conn, cookie, NULL);
    if (reply) {
        result = (reply->override_redirect != 0);
        free(reply);
    }
    else {
        ErrorF("IsOverrideRedirect: Failed to get window attributes\n");
    }

    return result;
}

/*
 * Helper function to get class and window names
*/
static void
GetClassNames(WMInfoPtr pWMInfo, xcb_window_t iWindow, char **res_name,
              char **res_class, char **window_name)
{
    xcb_get_property_cookie_t cookie1;
    xcb_icccm_get_wm_class_reply_t reply1;
    xcb_get_property_cookie_t cookie2;
    xcb_icccm_get_text_property_reply_t reply2;

    cookie1 = xcb_icccm_get_wm_class(pWMInfo->conn, iWindow);
    if (xcb_icccm_get_wm_class_reply(pWMInfo->conn, cookie1, &reply1,
                                     NULL)) {
        *res_name = strdup(reply1.instance_name);
        *res_class = strdup(reply1.class_name);
        xcb_icccm_get_wm_class_reply_wipe(&reply1);
    }
    else {
        *res_name = strdup("");
        *res_class = strdup("");
    }

    cookie2 = xcb_icccm_get_wm_name(pWMInfo->conn, iWindow);
    if (xcb_icccm_get_wm_name_reply(pWMInfo->conn, cookie2, &reply2, NULL)) {
        *window_name = strndup(reply2.name, reply2.name_len);
        xcb_icccm_get_text_property_reply_wipe(&reply2);
    }
    else {
        *window_name = strdup("");
    }
}

/*
 * Updates the name of a HWND according to its X WM_NAME property
 */

static void
UpdateName(WMInfoPtr pWMInfo, xcb_window_t iWindow)
{
    HWND hWnd;

    hWnd = getHwnd(pWMInfo, iWindow);
    if (!hWnd)
        return;

    /* If window isn't override-redirect */
    if (!IsOverrideRedirect(pWMInfo->conn, iWindow)) {
        char *pszWindowName;

        /* Get the X windows window name */
        GetWindowName(pWMInfo, iWindow, &pszWindowName);

        if (pszWindowName) {
            /* Convert from UTF-8 to wide char */
            int iLen =
                MultiByteToWideChar(CP_UTF8, 0, pszWindowName, -1, NULL, 0);
            wchar_t *pwszWideWindowName =
                malloc(sizeof(wchar_t)*(iLen + 1));
            MultiByteToWideChar(CP_UTF8, 0, pszWindowName, -1,
                                pwszWideWindowName, iLen);

            /* Set the Windows window name */
            SetWindowTextW(hWnd, pwszWideWindowName);

            free(pwszWideWindowName);
            free(pszWindowName);
        }
    }
}

/*
 * Updates the icon of a HWND according to its X icon properties
 */

static void
UpdateIcon(WMInfoPtr pWMInfo, xcb_window_t iWindow)
{
    HWND hWnd;
    HICON hIconNew = NULL;

    hWnd = getHwnd(pWMInfo, iWindow);
    if (!hWnd)
        return;

    /* If window isn't override-redirect */
    if (!IsOverrideRedirect(pWMInfo->conn, iWindow)) {
        char *window_name = 0;
        char *res_name = 0;
        char *res_class = 0;

        GetClassNames(pWMInfo, iWindow, &res_name, &res_class, &window_name);

        hIconNew = winOverrideIcon(res_name, res_class, window_name);

        free(res_name);
        free(res_class);
        free(window_name);
        winUpdateIcon(hWnd, pWMInfo->conn, iWindow, hIconNew);
    }
}

/*
 * Updates the style of a HWND according to its X style properties
 */

static void
UpdateStyle(WMInfoPtr pWMInfo, xcb_window_t iWindow)
{
    HWND hWnd;
    HWND zstyle = HWND_NOTOPMOST;
    UINT flags;

    hWnd = getHwnd(pWMInfo, iWindow);
    if (!hWnd)
        return;

    /* Determine the Window style, which determines borders and clipping region... */
    winApplyHints(pWMInfo, iWindow, hWnd, &zstyle);
    winUpdateWindowPosition(hWnd, &zstyle);

    /* Apply the updated window style, without changing its show or activation state */
    flags = SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
    if (zstyle == HWND_NOTOPMOST)
        flags |= SWP_NOZORDER | SWP_NOOWNERZORDER;
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, flags);

    /*
       Use the WS_EX_TOOLWINDOW style to remove window from Alt-Tab window switcher

       According to MSDN, this is supposed to remove the window from the taskbar as well,
       if we SW_HIDE before changing the style followed by SW_SHOW afterwards.

       But that doesn't seem to work reliably, and causes the window to flicker, so use
       the iTaskbarList interface to tell the taskbar to show or hide this window.
     */
    winShowWindowOnTaskbar(hWnd,
                           (GetWindowLongPtr(hWnd, GWL_EXSTYLE) &
                            WS_EX_APPWINDOW) ? TRUE : FALSE);
}

/*
 * Updates the state of a HWND
 * (only minimization supported at the moment)
 */

static void
UpdateState(WMInfoPtr pWMInfo, xcb_window_t iWindow)
{
    HWND hWnd;

    winDebug("UpdateState: iWindow 0x%08x\n", (int)iWindow);

    hWnd = getHwnd(pWMInfo, iWindow);
    if (!hWnd)
        return;

    ShowWindow(hWnd, SW_MINIMIZE);
}

#if 0
/*
 * Fix up any differences between the X11 and Win32 window stacks
 * starting at the window passed in
 */
static void
PreserveWin32Stack(WMInfoPtr pWMInfo, xcb_window_t iWindow, UINT direction)
{
    HWND hWnd;
    DWORD myWinProcID, winProcID;
    xcb_window_t xWindow;
    WINDOWPLACEMENT wndPlace;

    hWnd = getHwnd(pWMInfo, iWindow);
    if (!hWnd)
        return;

    GetWindowThreadProcessId(hWnd, &myWinProcID);
    hWnd = GetNextWindow(hWnd, direction);

    while (hWnd) {
        GetWindowThreadProcessId(hWnd, &winProcID);
        if (winProcID == myWinProcID) {
            wndPlace.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(hWnd, &wndPlace);
            if (!(wndPlace.showCmd == SW_HIDE ||
                  wndPlace.showCmd == SW_MINIMIZE)) {
                xWindow = (Window) GetProp(hWnd, WIN_WID_PROP);
                if (xWindow) {
                    if (direction == GW_HWNDPREV)
                        XRaiseWindow(pWMInfo->pDisplay, xWindow);
                    else
                        XLowerWindow(pWMInfo->pDisplay, xWindow);
                }
            }
        }
        hWnd = GetNextWindow(hWnd, direction);
    }
}
#endif                          /* PreserveWin32Stack */

/*
 * winMultiWindowWMProc
 */

static void *
winMultiWindowWMProc(void *pArg)
{
    WMProcArgPtr pProcArg = (WMProcArgPtr) pArg;
    WMInfoPtr pWMInfo = pProcArg->pWMInfo;

    /* Initialize the Window Manager */
    winInitMultiWindowWM(pWMInfo, pProcArg);

#if CYGMULTIWINDOW_DEBUG
    ErrorF("winMultiWindowWMProc ()\n");
#endif

    /* Loop until we explicitly break out */
    for (;;) {
        WMMsgNodePtr pNode;

        /* Pop a message off of our queue */
        pNode = PopMessage(&pWMInfo->wmMsgQueue, pWMInfo);
        if (pNode == NULL) {
            /* Bail if PopMessage returns without a message */
            /* NOTE: Remember that PopMessage is a blocking function. */
            ErrorF("winMultiWindowWMProc - Queue is Empty?  Exiting.\n");
            pthread_exit(NULL);
        }

#if CYGMULTIWINDOW_DEBUG
        ErrorF("winMultiWindowWMProc - MSG: %s (%d) ID: %d\n",
               MessageName(&(pNode->msg)), (int)pNode->msg.msg, (int)pNode->msg.dwID);
#endif

        /* Branch on the message type */
        switch (pNode->msg.msg) {
#if 0
        case WM_WM_MOVE:
            break;

        case WM_WM_SIZE:
            break;
#endif

        case WM_WM_RAISE:
            /* Raise the window */
            {
                const static uint32_t values[] = { XCB_STACK_MODE_ABOVE };
                xcb_configure_window(pWMInfo->conn, pNode->msg.iWindow,
                                     XCB_CONFIG_WINDOW_STACK_MODE, values);
            }

#if 0
            PreserveWin32Stack(pWMInfo, pNode->msg.iWindow, GW_HWNDPREV);
#endif
            break;

        case WM_WM_LOWER:
            /* Lower the window */
            {
                const static uint32_t values[] = { XCB_STACK_MODE_BELOW };
                xcb_configure_window(pWMInfo->conn, pNode->msg.iWindow,
                                     XCB_CONFIG_WINDOW_STACK_MODE, values);
            }
            break;

        case WM_WM_MAP_UNMANAGED:
            /* Put a note as to the HWND associated with this Window */
            xcb_change_property(pWMInfo->conn, XCB_PROP_MODE_REPLACE,
                                pNode->msg.iWindow, pWMInfo->atmPrivMap,
                                XCB_ATOM_INTEGER, 32,
                                sizeof(HWND)/4, &(pNode->msg.hwndWindow));

            break;

        case WM_WM_MAP_MANAGED:
            /* Put a note as to the HWND associated with this Window */
            xcb_change_property(pWMInfo->conn, XCB_PROP_MODE_REPLACE,
                                pNode->msg.iWindow, pWMInfo->atmPrivMap,
                                XCB_ATOM_INTEGER, 32,
                                sizeof(HWND)/4, &(pNode->msg.hwndWindow));

            UpdateName(pWMInfo, pNode->msg.iWindow);
            UpdateIcon(pWMInfo, pNode->msg.iWindow);
            UpdateStyle(pWMInfo, pNode->msg.iWindow);


            /* Reshape */
            {
                WindowPtr pWin =
                    GetProp(pNode->msg.hwndWindow, WIN_WINDOW_PROP);
                if (pWin) {
                    winReshapeMultiWindow(pWin);
                    winUpdateRgnMultiWindow(pWin);
                }
            }

            break;

        case WM_WM_UNMAP:

            /* Unmap the window */
            xcb_unmap_window(pWMInfo->conn, pNode->msg.iWindow);
            break;

        case WM_WM_KILL:
            {
                /* --- */
                if (IsWmProtocolAvailable(pWMInfo,
                                          pNode->msg.iWindow,
                                          pWMInfo->atmWmDelete))
                    SendXMessage(pWMInfo->conn,
                                 pNode->msg.iWindow,
                                 pWMInfo->atmWmProtos, pWMInfo->atmWmDelete);
                else
                    xcb_kill_client(pWMInfo->conn, pNode->msg.iWindow);
            }
            break;

        case WM_WM_ACTIVATE:
            /* Set the input focus */

            /*
               ICCCM 4.1.7 is pretty opaque, but it appears that the rules are
               actually quite simple:
               -- the WM_HINTS input field determines whether the WM should call
               XSetInputFocus()
               -- independently, the WM_TAKE_FOCUS protocol determines whether
               the WM should send a WM_TAKE_FOCUS ClientMessage.
            */
            {
              Bool neverFocus = FALSE;
              xcb_get_property_cookie_t cookie;
              xcb_icccm_wm_hints_t hints;

              cookie = xcb_icccm_get_wm_hints(pWMInfo->conn, pNode->msg.iWindow);
              if (xcb_icccm_get_wm_hints_reply(pWMInfo->conn, cookie, &hints,
                                               NULL)) {
                if (hints.flags & XCB_ICCCM_WM_HINT_INPUT)
                  neverFocus = !hints.input;
              }

              if (!neverFocus)
                xcb_set_input_focus(pWMInfo->conn, XCB_INPUT_FOCUS_POINTER_ROOT,
                                    pNode->msg.iWindow, XCB_CURRENT_TIME);

              if (IsWmProtocolAvailable(pWMInfo,
                                        pNode->msg.iWindow,
                                        pWMInfo->atmWmTakeFocus))
                SendXMessage(pWMInfo->conn,
                             pNode->msg.iWindow,
                             pWMInfo->atmWmProtos, pWMInfo->atmWmTakeFocus);

            }
            break;

        case WM_WM_NAME_EVENT:
            UpdateName(pWMInfo, pNode->msg.iWindow);
            break;

        case WM_WM_ICON_EVENT:
            UpdateIcon(pWMInfo, pNode->msg.iWindow);
            break;

        case WM_WM_HINTS_EVENT:
            {
            /* Don't do anything if this is an override-redirect window */
            if (IsOverrideRedirect(pWMInfo->conn, pNode->msg.iWindow))
              break;

            UpdateStyle(pWMInfo, pNode->msg.iWindow);
            }
            break;

        case WM_WM_CHANGE_STATE:
            UpdateState(pWMInfo, pNode->msg.iWindow);
            break;

        default:
            ErrorF("winMultiWindowWMProc - Unknown Message.  Exiting.\n");
            pthread_exit(NULL);
            break;
        }

        /* Free the retrieved message */
        free(pNode);

        /* Flush any pending events on our display */
        xcb_flush(pWMInfo->conn);

        /* This is just laziness rather than making sure we used _checked everywhere */
        {
            xcb_generic_event_t *event = xcb_poll_for_event(pWMInfo->conn);
            if (event) {
                if ((event->response_type & ~0x80) == 0) {
                    xcb_generic_error_t *err = (xcb_generic_error_t *)event;
                    ErrorF("winMultiWindowWMProc - Error code: %i, ID: 0x%08x, "
                           "Major opcode: %i, Minor opcode: %i\n",
                           err->error_code, err->resource_id,
                           err->major_code, err->minor_code);
                }
            }
        }

        /* I/O errors etc. */
        {
            int e = xcb_connection_has_error(pWMInfo->conn);
            if (e) {
                ErrorF("winMultiWindowWMProc - Fatal error %d on xcb connection\n", e);
                break;
            }
        }
    }

    /* Free the condition variable */
    pthread_cond_destroy(&pWMInfo->wmMsgQueue.pcNotEmpty);

    /* Free the mutex variable */
    pthread_mutex_destroy(&pWMInfo->wmMsgQueue.pmMutex);

    /* Free the passed-in argument */
    free(pProcArg);

#if CYGMULTIWINDOW_DEBUG
    ErrorF("-winMultiWindowWMProc ()\n");
#endif
    return NULL;
}

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
 * X message procedure
 */

static void *
winMultiWindowXMsgProc(void *pArg)
{
    winWMMessageRec msg;
    XMsgProcArgPtr pProcArg = (XMsgProcArgPtr) pArg;
    char pszDisplay[512];
    int iRetries;
    xcb_atom_t atmWmName;
    xcb_atom_t atmNetWmName;
    xcb_atom_t atmWmHints;
    xcb_atom_t atmWmChange;
    xcb_atom_t atmNetWmIcon;
    xcb_atom_t atmWindowState, atmMotifWmHints, atmWindowType, atmNormalHints;
    int iReturn;
    xcb_auth_info_t *auth_info;
    xcb_screen_t *root_screen;
    xcb_window_t root_window_id;

    winDebug("winMultiWindowXMsgProc - Hello\n");

    /* Check that argument pointer is not invalid */
    if (pProcArg == NULL) {
        ErrorF("winMultiWindowXMsgProc - pProcArg is NULL.  Exiting.\n");
        pthread_exit(NULL);
    }

    winDebug("winMultiWindowXMsgProc - Calling pthread_mutex_lock ()\n");

    /* Grab the server started mutex - pause until we get it */
    iReturn = pthread_mutex_lock(pProcArg->ppmServerStarted);
    if (iReturn != 0) {
        ErrorF("winMultiWindowXMsgProc - pthread_mutex_lock () failed: %d.  "
               "Exiting.\n", iReturn);
        pthread_exit(NULL);
    }

    winDebug("winMultiWindowXMsgProc - pthread_mutex_lock () returned.\n");

    /* Release the server started mutex */
    pthread_mutex_unlock(pProcArg->ppmServerStarted);

    winDebug("winMultiWindowXMsgProc - pthread_mutex_unlock () returned.\n");

    /* Setup the display connection string x */
    winGetDisplayName(pszDisplay, (int) pProcArg->dwScreen);

    /* Print the display connection string */
    ErrorF("winMultiWindowXMsgProc - DISPLAY=%s\n", pszDisplay);

    /* Use our generated cookie for authentication */
    auth_info = winGetXcbAuthInfo();

    /* Initialize retry count */
    iRetries = 0;

    /* Open the X display */
    do {
        /* Try to open the display */
        pProcArg->conn = xcb_connect_to_display_with_auth_info(pszDisplay,
                                                               auth_info, NULL);
        if (xcb_connection_has_error(pProcArg->conn)) {
            ErrorF("winMultiWindowXMsgProc - Could not open display, try: %d, "
                   "sleeping: %d\n", iRetries + 1, WIN_CONNECT_DELAY);
            ++iRetries;
            sleep(WIN_CONNECT_DELAY);
            continue;
        }
        else
            break;
    }
    while (xcb_connection_has_error(pProcArg->conn) && iRetries < WIN_CONNECT_RETRIES);

    /* Make sure that the display opened */
    if (xcb_connection_has_error(pProcArg->conn)) {
        ErrorF("winMultiWindowXMsgProc - Failed opening the display.  "
               "Exiting.\n");
        pthread_exit(NULL);
    }

    ErrorF("winMultiWindowXMsgProc - xcb_connect() returned and "
           "successfully opened the display.\n");

    /* Check if another window manager is already running */
    if (CheckAnotherWindowManager(pProcArg->conn, pProcArg->dwScreen)) {
        ErrorF("winMultiWindowXMsgProc - "
               "another window manager is running.  Exiting.\n");
        pthread_exit(NULL);
    }

    /* Get root window id */
    root_screen = xcb_aux_get_screen(pProcArg->conn, pProcArg->dwScreen);
    root_window_id = root_screen->root;

    {
        /* Set WM_ICON_SIZE property indicating desired icon sizes */
        typedef struct {
            uint32_t min_width, min_height;
            uint32_t max_width, max_height;
            int32_t width_inc, height_inc;
        } xcb_wm_icon_size_hints_hints_t;

        xcb_wm_icon_size_hints_hints_t xis;
        xis.min_width = xis.min_height = 16;
        xis.max_width = xis.max_height = 48;
        xis.width_inc = xis.height_inc = 16;

        xcb_change_property(pProcArg->conn, XCB_PROP_MODE_REPLACE, root_window_id,
                            XCB_ATOM_WM_ICON_SIZE, XCB_ATOM_WM_ICON_SIZE, 32,
                            sizeof(xis)/4, &xis);
    }

    atmWmName = intern_atom(pProcArg->conn, "WM_NAME");
    atmNetWmName = intern_atom(pProcArg->conn, "_NET_WM_NAME");
    atmWmHints = intern_atom(pProcArg->conn, "WM_HINTS");
    atmWmChange = intern_atom(pProcArg->conn, "WM_CHANGE_STATE");
    atmNetWmIcon = intern_atom(pProcArg->conn, "_NET_WM_ICON");
    atmWindowState = intern_atom(pProcArg->conn, "_NET_WM_STATE");
    atmMotifWmHints = intern_atom(pProcArg->conn, "_MOTIF_WM_HINTS");
    atmWindowType = intern_atom(pProcArg->conn, "_NET_WM_WINDOW_TYPE");
    atmNormalHints = intern_atom(pProcArg->conn, "WM_NORMAL_HINTS");

    /*
       iiimxcf had a bug until 2009-04-27, assuming that the
       WM_STATE atom exists, causing clients to fail with
       a BadAtom X error if it doesn't.

       Since this is on in the default Solaris 10 install,
       workaround this by making sure it does exist...
     */
    intern_atom(pProcArg->conn, "WM_STATE");

    /*
      Enable Composite extension and redirect subwindows of the root window
     */
    if (pProcArg->pWMInfo->fCompositeWM) {
        const char *extension_name = "Composite";
        xcb_query_extension_cookie_t cookie;
        xcb_query_extension_reply_t *reply;

        cookie = xcb_query_extension(pProcArg->conn, strlen(extension_name), extension_name);
        reply = xcb_query_extension_reply(pProcArg->conn, cookie, NULL);

        if (reply && (reply->present)) {
            xcb_composite_redirect_subwindows(pProcArg->conn,
                                              root_window_id,
                                              XCB_COMPOSITE_REDIRECT_AUTOMATIC);

            /*
              We use automatic updating of the root window for two
              reasons:

              1) redirected window contents are mirrored to the root
              window so that the root window draws correctly when shown.

              2) updating the root window causes damage against the
              shadow framebuffer, which ultimately causes WM_PAINT to be
              sent to the affected window(s) to cause the damage regions
              to be redrawn.
            */

            ErrorF("Using Composite redirection\n");

            free(reply);
        }
    }

    /* Loop until we explicitly break out */
    while (1) {
        xcb_generic_event_t *event;
        uint8_t type;
        Bool send_event;

        if (g_shutdown)
            break;

        /* Fetch next event */
        event = xcb_wait_for_event(pProcArg->conn);
        if (!event) { // returns NULL on I/O error
            int e = xcb_connection_has_error(pProcArg->conn);
            ErrorF("winMultiWindowXMsgProc - Fatal error %d on xcb connection\n", e);
            break;
        }

        type = event->response_type & ~0x80;
        send_event = event->response_type & 0x80;

        winDebug("winMultiWindowXMsgProc - event %d\n", type);

        /* Branch on event type */
        if (type == 0) {
            xcb_generic_error_t *err = (xcb_generic_error_t *)event;
            ErrorF("winMultiWindowXMsgProc - Error code: %i, ID: 0x%08x, "
                   "Major opcode: %i, Minor opcode: %i\n",
                   err->error_code, err->resource_id,
                   err->major_code, err->minor_code);
            }
        else if (type == XCB_CREATE_NOTIFY) {
            xcb_create_notify_event_t *notify = (xcb_create_notify_event_t *)event;

            /* Request property change events */
            const static uint32_t mask_value[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
            xcb_change_window_attributes (pProcArg->conn, notify->window,
                                          XCB_CW_EVENT_MASK, mask_value);

            /* If it's not override-redirect, set the border-width to 0 */
            if (!IsOverrideRedirect(pProcArg->conn, notify->window)) {
                const static uint32_t width_value[] = { 0 };
                xcb_configure_window(pProcArg->conn, notify->window,
                                     XCB_CONFIG_WINDOW_BORDER_WIDTH, width_value);
            }
        }
        else if (type == XCB_MAP_NOTIFY) {
            /* Fake a reparentNotify event as SWT/Motif expects a
               Window Manager to reparent a top-level window when
               it is mapped and waits until they do.

               We don't actually need to reparent, as the frame is
               a native window, not an X window

               We do this on MapNotify, not MapRequest like a real
               Window Manager would, so we don't have do get involved
               in actually mapping the window via it's (non-existent)
               parent...

               See sourceware bugzilla #9848
             */

            xcb_map_notify_event_t *notify = (xcb_map_notify_event_t *)event;

            xcb_get_geometry_cookie_t cookie;
            xcb_get_geometry_reply_t *reply;
            xcb_query_tree_cookie_t cookie_qt;
            xcb_query_tree_reply_t *reply_qt;

            cookie = xcb_get_geometry(pProcArg->conn, notify->window);
            cookie_qt = xcb_query_tree(pProcArg->conn, notify->window);
            reply = xcb_get_geometry_reply(pProcArg->conn, cookie, NULL);
            reply_qt = xcb_query_tree_reply(pProcArg->conn, cookie_qt, NULL);

            if (reply && reply_qt) {
                /*
                   It's a top-level window if the parent window is a root window
                   Only non-override_redirect windows can get reparented
                 */
                if ((reply->root == reply_qt->parent) && !notify->override_redirect) {
                    xcb_reparent_notify_event_t event_send;

                    event_send.response_type = XCB_REPARENT_NOTIFY;
                    event_send.event = notify->window;
                    event_send.window = notify->window;
                    event_send.parent = reply_qt->parent;
                    event_send.x = reply->x;
                    event_send.y = reply->y;

                    xcb_send_event (pProcArg->conn, TRUE, notify->window,
                                    XCB_EVENT_MASK_STRUCTURE_NOTIFY,
                                    (const char *)&event_send);

                    free(reply_qt);
                    free(reply);
                }
            }
        }
        else if (type == XCB_CONFIGURE_NOTIFY) {
            if (!send_event) {
                /*
                   Java applications using AWT on JRE 1.6.0 break with non-reparenting WMs AWT
                   doesn't explicitly know about (See sun bug #6434227)

                   XDecoratedPeer.handleConfigureNotifyEvent() only processes non-synthetic
                   ConfigureNotify events to update window location if it's identified the
                   WM as a non-reparenting WM it knows about (compiz or lookingglass)

                   Rather than tell all sorts of lies to get XWM to recognize us as one of
                   those, simply send a synthetic ConfigureNotify for every non-synthetic one
                 */
                xcb_configure_notify_event_t *notify = (xcb_configure_notify_event_t *)event;
                xcb_configure_notify_event_t event_send = *notify;

                event_send.event = notify->window;

                xcb_send_event(pProcArg->conn, TRUE, notify->window,
                               XCB_EVENT_MASK_STRUCTURE_NOTIFY,
                               (const char *)&event_send);
            }
        }
        else if (type ==  XCB_PROPERTY_NOTIFY) {
            xcb_property_notify_event_t *notify = (xcb_property_notify_event_t *)event;

            if ((notify->atom == atmWmName) ||
                (notify->atom == atmNetWmName)) {
                memset(&msg, 0, sizeof(msg));

                msg.msg = WM_WM_NAME_EVENT;
                msg.iWindow = notify->window;

                /* Other fields ignored */
                winSendMessageToWM(pProcArg->pWMInfo, &msg);
            }
            else {
                /*
                   Several properties are considered for WM hints, check if this property change affects any of them...
                   (this list needs to be kept in sync with winApplyHints())
                 */
                if ((notify->atom == atmWmHints) ||
                    (notify->atom == atmWindowState) ||
                    (notify->atom == atmMotifWmHints) ||
                    (notify->atom == atmWindowType) ||
                    (notify->atom == atmNormalHints)) {
                    memset(&msg, 0, sizeof(msg));
                    msg.msg = WM_WM_HINTS_EVENT;
                    msg.iWindow = notify->window;

                    /* Other fields ignored */
                    winSendMessageToWM(pProcArg->pWMInfo, &msg);
                }

                /* Not an else as WM_HINTS affects both style and icon */
                if ((notify->atom == atmWmHints) ||
                    (notify->atom == atmNetWmIcon)) {
                    memset(&msg, 0, sizeof(msg));
                    msg.msg = WM_WM_ICON_EVENT;
                    msg.iWindow = notify->window;

                    /* Other fields ignored */
                    winSendMessageToWM(pProcArg->pWMInfo, &msg);
                }
            }
        }
        else if (type == XCB_CLIENT_MESSAGE) {
            xcb_client_message_event_t *client_msg = (xcb_client_message_event_t *)event;

            if (client_msg->type == atmWmChange
                 && client_msg->data.data32[0] == XCB_ICCCM_WM_STATE_ICONIC) {
                ErrorF("winMultiWindowXMsgProc - WM_CHANGE_STATE - IconicState\n");

                memset(&msg, 0, sizeof(msg));

                msg.msg = WM_WM_CHANGE_STATE;
                msg.iWindow = client_msg->window;

                winSendMessageToWM(pProcArg->pWMInfo, &msg);
            }
        }

        /* Free the event */
        free(event);
    }

    xcb_disconnect(pProcArg->conn);
    pthread_exit(NULL);
    return NULL;
}

/*
 * winInitWM - Entry point for the X server to spawn
 * the Window Manager thread.  Called from
 * winscrinit.c/winFinishScreenInitFB ().
 */

Bool
winInitWM(void **ppWMInfo,
          pthread_t * ptWMProc,
          pthread_t * ptXMsgProc,
          pthread_mutex_t * ppmServerStarted,
          int dwScreen, HWND hwndScreen, Bool compositeWM)
{
    WMProcArgPtr pArg = malloc(sizeof(WMProcArgRec));
    WMInfoPtr pWMInfo = malloc(sizeof(WMInfoRec));
    XMsgProcArgPtr pXMsgArg = malloc(sizeof(XMsgProcArgRec));

    /* Bail if the input parameters are bad */
    if (pArg == NULL || pWMInfo == NULL || pXMsgArg == NULL) {
        ErrorF("winInitWM - malloc failed.\n");
        free(pArg);
        free(pWMInfo);
        free(pXMsgArg);
        return FALSE;
    }

    /* Zero the allocated memory */
    ZeroMemory(pArg, sizeof(WMProcArgRec));
    ZeroMemory(pWMInfo, sizeof(WMInfoRec));
    ZeroMemory(pXMsgArg, sizeof(XMsgProcArgRec));

    /* Set a return pointer to the Window Manager info structure */
    *ppWMInfo = pWMInfo;
    pWMInfo->fCompositeWM = compositeWM;

    /* Setup the argument structure for the thread function */
    pArg->dwScreen = dwScreen;
    pArg->pWMInfo = pWMInfo;
    pArg->ppmServerStarted = ppmServerStarted;

    /* Initialize the message queue */
    if (!InitQueue(&pWMInfo->wmMsgQueue)) {
        ErrorF("winInitWM - InitQueue () failed.\n");
        return FALSE;
    }

    /* Spawn a thread for the Window Manager */
    if (pthread_create(ptWMProc, NULL, winMultiWindowWMProc, pArg)) {
        /* Bail if thread creation failed */
        ErrorF("winInitWM - pthread_create failed for Window Manager.\n");
        return FALSE;
    }

    /* Spawn the XNextEvent thread, will send messages to WM */
    pXMsgArg->dwScreen = dwScreen;
    pXMsgArg->pWMInfo = pWMInfo;
    pXMsgArg->ppmServerStarted = ppmServerStarted;
    pXMsgArg->hwndScreen = hwndScreen;
    if (pthread_create(ptXMsgProc, NULL, winMultiWindowXMsgProc, pXMsgArg)) {
        /* Bail if thread creation failed */
        ErrorF("winInitWM - pthread_create failed on XMSG.\n");
        return FALSE;
    }

#if CYGDEBUG || YES
    winDebug("winInitWM - Returning.\n");
#endif

    return TRUE;
}

/*
 * Window manager thread - setup
 */

static void
winInitMultiWindowWM(WMInfoPtr pWMInfo, WMProcArgPtr pProcArg)
{
    int iRetries = 0;
    char pszDisplay[512];
    int iReturn;
    xcb_auth_info_t *auth_info;
    xcb_screen_t *root_screen;
    xcb_window_t root_window_id;

    winDebug("winInitMultiWindowWM - Hello\n");

    /* Check that argument pointer is not invalid */
    if (pProcArg == NULL) {
        ErrorF("winInitMultiWindowWM - pProcArg is NULL.  Exiting.\n");
        pthread_exit(NULL);
    }

    winDebug("winInitMultiWindowWM - Calling pthread_mutex_lock ()\n");

    /* Grab our garbage mutex to satisfy pthread_cond_wait */
    iReturn = pthread_mutex_lock(pProcArg->ppmServerStarted);
    if (iReturn != 0) {
        ErrorF("winInitMultiWindowWM - pthread_mutex_lock () failed: %d.  "
               "Exiting.\n", iReturn);
        pthread_exit(NULL);
    }

    winDebug("winInitMultiWindowWM - pthread_mutex_lock () returned.\n");

    /* Release the server started mutex */
    pthread_mutex_unlock(pProcArg->ppmServerStarted);

    winDebug("winInitMultiWindowWM - pthread_mutex_unlock () returned.\n");

    /* Setup the display connection string x */
    winGetDisplayName(pszDisplay, (int) pProcArg->dwScreen);

    /* Print the display connection string */
    ErrorF("winInitMultiWindowWM - DISPLAY=%s\n", pszDisplay);

    /* Use our generated cookie for authentication */
    auth_info = winGetXcbAuthInfo();

    /* Open the X display */
    do {
        /* Try to open the display */
        pWMInfo->conn = xcb_connect_to_display_with_auth_info(pszDisplay,
                                                              auth_info, NULL);
        if (xcb_connection_has_error(pWMInfo->conn)) {
            ErrorF("winInitMultiWindowWM - Could not open display, try: %d, "
                   "sleeping: %d\n", iRetries + 1, WIN_CONNECT_DELAY);
            ++iRetries;
            sleep(WIN_CONNECT_DELAY);
            continue;
        }
        else
            break;
    }
    while (xcb_connection_has_error(pWMInfo->conn) && iRetries < WIN_CONNECT_RETRIES);

    /* Make sure that the display opened */
    if (xcb_connection_has_error(pWMInfo->conn)) {
        ErrorF("winInitMultiWindowWM - Failed opening the display.  "
               "Exiting.\n");
        pthread_exit(NULL);
    }

    ErrorF("winInitMultiWindowWM - xcb_connect () returned and "
           "successfully opened the display.\n");

    /* Create some atoms */
    pWMInfo->atmWmProtos = intern_atom(pWMInfo->conn, "WM_PROTOCOLS");
    pWMInfo->atmWmDelete = intern_atom(pWMInfo->conn, "WM_DELETE_WINDOW");
    pWMInfo->atmWmTakeFocus = intern_atom(pWMInfo->conn, "WM_TAKE_FOCUS");
    pWMInfo->atmPrivMap = intern_atom(pWMInfo->conn, WINDOWSWM_NATIVE_HWND);
    pWMInfo->atmUtf8String = intern_atom(pWMInfo->conn, "UTF8_STRING");
    pWMInfo->atmNetWmName = intern_atom(pWMInfo->conn, "_NET_WM_NAME");
    pWMInfo->atmCurrentDesktop = intern_atom(pWMInfo->conn, "_NET_CURRENT_DESKTOP");
    pWMInfo->atmNumberDesktops = intern_atom(pWMInfo->conn, "_NET_NUMBER_OF_DESKTOPS");
    pWMInfo->atmDesktopNames = intern_atom(pWMInfo->conn, "__NET_DESKTOP_NAMES");

    /* Initialization for the xcb_ewmh and EWMH atoms */
    {
        xcb_intern_atom_cookie_t *atoms_cookie;
        atoms_cookie = xcb_ewmh_init_atoms(pWMInfo->conn, &pWMInfo->ewmh);
        if (xcb_ewmh_init_atoms_replies(&pWMInfo->ewmh, atoms_cookie, NULL)) {
            /* Set the _NET_SUPPORTED atom for this context.

               TODO: Audit to ensure we implement everything defined as MUSTs
               for window managers in the EWMH standard.*/
            xcb_atom_t supported[] =
                {
                    pWMInfo->ewmh.WM_PROTOCOLS,
                    pWMInfo->ewmh._NET_SUPPORTED,
                    pWMInfo->ewmh._NET_SUPPORTING_WM_CHECK,
                    pWMInfo->ewmh._NET_CLOSE_WINDOW,
                    pWMInfo->ewmh._NET_WM_WINDOW_TYPE,
                    pWMInfo->ewmh._NET_WM_WINDOW_TYPE_DOCK,
                    pWMInfo->ewmh._NET_WM_WINDOW_TYPE_SPLASH,
                    pWMInfo->ewmh._NET_WM_STATE,
                    pWMInfo->ewmh._NET_WM_STATE_HIDDEN,
                    pWMInfo->ewmh._NET_WM_STATE_ABOVE,
                    pWMInfo->ewmh._NET_WM_STATE_BELOW,
                    pWMInfo->ewmh._NET_WM_STATE_SKIP_TASKBAR,
                };

            xcb_ewmh_set_supported(&pWMInfo->ewmh, pProcArg->dwScreen,
                                   ARRAY_SIZE(supported), supported);
        }
        else {
            ErrorF("winInitMultiWindowWM - xcb_ewmh_init_atoms() failed\n");
        }
    }

    /* Get root window id */
    root_screen = xcb_aux_get_screen(pWMInfo->conn, pProcArg->dwScreen);
    root_window_id = root_screen->root;

    /*
      Set root window properties for describing multiple desktops to describe
      the one desktop we have
    */
    {
        int data;
        const char buf[] = "Desktop";

        data = 0;
        xcb_change_property(pWMInfo->conn, XCB_PROP_MODE_REPLACE, root_window_id,
                            pWMInfo->atmCurrentDesktop, XCB_ATOM_CARDINAL, 32,
                            1, &data);
        data = 1;
        xcb_change_property(pWMInfo->conn, XCB_PROP_MODE_REPLACE, root_window_id,
                            pWMInfo->atmNumberDesktops, XCB_ATOM_CARDINAL, 32,
                            1, &data);

        xcb_change_property(pWMInfo->conn, XCB_PROP_MODE_REPLACE, root_window_id,
                            pWMInfo->atmDesktopNames, pWMInfo->atmUtf8String, 8,
                            strlen(buf), (unsigned char *) buf);
    }

    /*
      Set the root window cursor to left_ptr (this controls the cursor an
      application gets over its windows when it doesn't set one)
    */
    {
#define XC_left_ptr 68
        xcb_cursor_t cursor = xcb_generate_id(pWMInfo->conn);
        xcb_font_t font = xcb_generate_id(pWMInfo->conn);
        xcb_font_t *mask_font = &font; /* An alias to clarify */
        int shape = XC_left_ptr;
        uint32_t mask = XCB_CW_CURSOR;
        uint32_t value_list = cursor;

        static const uint16_t fgred = 0, fggreen = 0, fgblue = 0;
        static const uint16_t bgred = 0xFFFF, bggreen = 0xFFFF, bgblue = 0xFFFF;

        xcb_open_font(pWMInfo->conn, font, sizeof("cursor"), "cursor");

        xcb_create_glyph_cursor(pWMInfo->conn, cursor, font, *mask_font,
                                shape, shape + 1,
                                fgred, fggreen, fgblue, bgred, bggreen, bgblue);

        xcb_change_window_attributes(pWMInfo->conn, root_window_id, mask, &value_list);

        xcb_free_cursor(pWMInfo->conn, cursor);
        xcb_close_font(pWMInfo->conn, font);
    }
}

/*
 * winSendMessageToWM - Send a message from the X thread to the WM thread
 */

void
winSendMessageToWM(void *pWMInfo, winWMMessagePtr pMsg)
{
    WMMsgNodePtr pNode;

#if CYGMULTIWINDOW_DEBUG
    ErrorF("winSendMessageToWM %s\n", MessageName(pMsg));
#endif

    pNode = malloc(sizeof(WMMsgNodeRec));
    if (pNode != NULL) {
        memcpy(&pNode->msg, pMsg, sizeof(winWMMessageRec));
        PushMessage(&((WMInfoPtr) pWMInfo)->wmMsgQueue, pNode);
    }
}

/*
 * Check if another window manager is running
 */

static Bool
CheckAnotherWindowManager(xcb_connection_t *conn, DWORD dwScreen)
{
    Bool redirectError = FALSE;

    /* Get root window id */
    xcb_screen_t *root_screen = xcb_aux_get_screen(conn, dwScreen);
    xcb_window_t root_window_id = root_screen->root;

    /*
       Try to select the events which only one client at a time is allowed to select.
       If this causes an error, another window manager is already running...
     */
    const static uint32_t test_mask[] = { XCB_EVENT_MASK_RESIZE_REDIRECT |
                                       XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                                       XCB_EVENT_MASK_BUTTON_PRESS };

    xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(conn,
                                                                    root_window_id,
                                                                    XCB_CW_EVENT_MASK,
                                                                    test_mask);
    xcb_generic_error_t *error;
    if ((error = xcb_request_check(conn, cookie)))
        {
            redirectError = TRUE;
            free(error);
        }

    /*
       Side effect: select the events we are actually interested in...

       Other WMs are not allowed, also select one of the events which only one client
       at a time is allowed to select, so other window managers won't start...
     */
    {
        const uint32_t mask[] = { XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                                  XCB_EVENT_MASK_BUTTON_PRESS };

        xcb_change_window_attributes(conn, root_window_id, XCB_CW_EVENT_MASK, mask);
    }

    return redirectError;
}

/*
 * Notify the MWM thread we're exiting and not to reconnect
 */

void
winDeinitMultiWindowWM(void)
{
    ErrorF("winDeinitMultiWindowWM - Noting shutdown in progress\n");
    g_shutdown = TRUE;
}

/* Windows window styles */
#define HINT_NOFRAME	(1L<<0)
#define HINT_BORDER	(1L<<1)
#define HINT_SIZEBOX	(1L<<2)
#define HINT_CAPTION	(1L<<3)
#define HINT_NOMAXIMIZE (1L<<4)
#define HINT_NOMINIMIZE (1L<<5)
#define HINT_NOSYSMENU  (1L<<6)
#define HINT_SKIPTASKBAR (1L<<7)
/* These two are used on their own */
#define HINT_MAX	(1L<<0)
#define HINT_MIN	(1L<<1)

static void
winApplyHints(WMInfoPtr pWMInfo, xcb_window_t iWindow, HWND hWnd, HWND * zstyle)
{

    xcb_connection_t *conn = pWMInfo->conn;
    static xcb_atom_t windowState, motif_wm_hints;
    static xcb_atom_t hiddenState, fullscreenState, belowState, aboveState,
        skiptaskbarState;
    static xcb_atom_t splashType;
    static int generation;

    unsigned long hint = 0, maxmin = 0;
    unsigned long style, exStyle;

    if (!hWnd)
        return;
    if (!IsWindow(hWnd))
        return;

    if (generation != serverGeneration) {
        generation = serverGeneration;
        windowState = intern_atom(conn, "_NET_WM_STATE");
        motif_wm_hints = intern_atom(conn, "_MOTIF_WM_HINTS");
        hiddenState = intern_atom(conn, "_NET_WM_STATE_HIDDEN");
        fullscreenState = intern_atom(conn, "_NET_WM_STATE_FULLSCREEN");
        belowState = intern_atom(conn, "_NET_WM_STATE_BELOW");
        aboveState = intern_atom(conn, "_NET_WM_STATE_ABOVE");
        skiptaskbarState = intern_atom(conn, "_NET_WM_STATE_SKIP_TASKBAR");
        splashType = intern_atom(conn, "_NET_WM_WINDOW_TYPE_SPLASHSCREEN");
    }

    {
      xcb_get_property_cookie_t cookie_wm_state = xcb_get_property(conn, FALSE, iWindow, windowState, XCB_ATOM_ATOM, 0L, INT_MAX);
      xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie_wm_state, NULL);
      if (reply) {
        int i;
        int nitems = xcb_get_property_value_length(reply)/sizeof(xcb_atom_t);
        xcb_atom_t *pAtom = xcb_get_property_value(reply);

            for (i = 0; i < nitems; i++) {
                if (pAtom[i] == skiptaskbarState)
                    hint |= HINT_SKIPTASKBAR;
                if (pAtom[i] == hiddenState)
                    maxmin |= HINT_MIN;
                else if (pAtom[i] == fullscreenState)
                    maxmin |= HINT_MAX;
                if (pAtom[i] == belowState)
                    *zstyle = HWND_BOTTOM;
                else if (pAtom[i] == aboveState)
                    *zstyle = HWND_TOPMOST;
            }

            free(reply);
      }
    }

    {
      xcb_get_property_cookie_t cookie_mwm_hint = xcb_get_property(conn, FALSE, iWindow, motif_wm_hints, motif_wm_hints, 0L, sizeof(MwmHints));
      xcb_get_property_reply_t *reply =  xcb_get_property_reply(conn, cookie_mwm_hint, NULL);
      if (reply) {
        int nitems = xcb_get_property_value_length(reply)/4;
        MwmHints *mwm_hint = xcb_get_property_value(reply);
        if (mwm_hint && (nitems >= PropMwmHintsElements) &&
            (mwm_hint->flags & MwmHintsDecorations)) {
            if (!mwm_hint->decorations)
                hint |= (HINT_NOFRAME | HINT_NOSYSMENU | HINT_NOMINIMIZE | HINT_NOMAXIMIZE);
            else if (!(mwm_hint->decorations & MwmDecorAll)) {
                if (mwm_hint->decorations & MwmDecorBorder)
                    hint |= HINT_BORDER;
                if (mwm_hint->decorations & MwmDecorHandle)
                    hint |= HINT_SIZEBOX;
                if (mwm_hint->decorations & MwmDecorTitle)
                    hint |= HINT_CAPTION;
                if (!(mwm_hint->decorations & MwmDecorMenu))
                    hint |= HINT_NOSYSMENU;
                if (!(mwm_hint->decorations & MwmDecorMinimize))
                    hint |= HINT_NOMINIMIZE;
                if (!(mwm_hint->decorations & MwmDecorMaximize))
                    hint |= HINT_NOMAXIMIZE;
            }
            else {
                /*
                   MwmDecorAll means all decorations *except* those specified by other flag
                   bits that are set.  Not yet implemented.
                 */
            }
        }
        free(reply);
      }
    }

    {
      int i;
      xcb_ewmh_get_atoms_reply_t type;
      xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_window_type(&pWMInfo->ewmh, iWindow);
      if (xcb_ewmh_get_wm_window_type_reply(&pWMInfo->ewmh, cookie, &type, NULL)) {
        for (i = 0; i < type.atoms_len; i++) {
            if (type.atoms[i] ==  pWMInfo->ewmh._NET_WM_WINDOW_TYPE_DOCK) {
                hint = (hint & ~HINT_NOFRAME) | HINT_SKIPTASKBAR | HINT_SIZEBOX;
                *zstyle = HWND_TOPMOST;
            }
            else if ((type.atoms[i] == pWMInfo->ewmh._NET_WM_WINDOW_TYPE_SPLASH)
                     || (type.atoms[i] == splashType)) {
                hint |= (HINT_SKIPTASKBAR | HINT_NOSYSMENU | HINT_NOMINIMIZE | HINT_NOMAXIMIZE);
                *zstyle = HWND_TOPMOST;
            }
        }
      }
    }

    {
        xcb_size_hints_t size_hints;
        xcb_get_property_cookie_t cookie;

        cookie = xcb_icccm_get_wm_normal_hints(conn, iWindow);
        if (xcb_icccm_get_wm_normal_hints_reply(conn, cookie, &size_hints, NULL)) {
            if (size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) {

                /* Not maximizable if a maximum size is specified, and that size
                   is smaller (in either dimension) than the screen size */
                if ((size_hints.max_width < GetSystemMetrics(SM_CXVIRTUALSCREEN))
                    || (size_hints.max_height < GetSystemMetrics(SM_CYVIRTUALSCREEN)))
                    hint |= HINT_NOMAXIMIZE;

                if (size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
                    /*
                       If both minimum size and maximum size are specified and are the same,
                       don't bother with a resizing frame
                     */
                    if ((size_hints.min_width == size_hints.max_width)
                        && (size_hints.min_height == size_hints.max_height))
                        hint = (hint & ~HINT_SIZEBOX);
                }
            }
        }
    }

    /*
       Override hint settings from above with settings from config file and set
       application id for grouping.
     */
    {
        char *application_id = 0;
        char *window_name = 0;
        char *res_name = 0;
        char *res_class = 0;

        GetClassNames(pWMInfo, iWindow, &res_name, &res_class, &window_name);

        style = STYLE_NONE;
        style = winOverrideStyle(res_name, res_class, window_name);

#define APPLICATION_ID_FORMAT	"%s.xwin.%s"
#define APPLICATION_ID_UNKNOWN "unknown"
        if (res_class) {
            asprintf(&application_id, APPLICATION_ID_FORMAT, XVENDORNAME,
                     res_class);
        }
        else {
            asprintf(&application_id, APPLICATION_ID_FORMAT, XVENDORNAME,
                     APPLICATION_ID_UNKNOWN);
        }
        winSetAppUserModelID(hWnd, application_id);

        free(application_id);
        free(res_name);
        free(res_class);
        free(window_name);
    }

    if (style & STYLE_TOPMOST)
        *zstyle = HWND_TOPMOST;
    else if (style & STYLE_MAXIMIZE)
        maxmin = (hint & ~HINT_MIN) | HINT_MAX;
    else if (style & STYLE_MINIMIZE)
        maxmin = (hint & ~HINT_MAX) | HINT_MIN;
    else if (style & STYLE_BOTTOM)
        *zstyle = HWND_BOTTOM;

    if (maxmin & HINT_MAX)
        SendMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    else if (maxmin & HINT_MIN)
        SendMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);

    if (style & STYLE_NOTITLE)
        hint =
            (hint & ~HINT_NOFRAME & ~HINT_BORDER & ~HINT_CAPTION) |
            HINT_SIZEBOX;
    else if (style & STYLE_OUTLINE)
        hint =
            (hint & ~HINT_NOFRAME & ~HINT_SIZEBOX & ~HINT_CAPTION) |
            HINT_BORDER;
    else if (style & STYLE_NOFRAME)
        hint =
            (hint & ~HINT_BORDER & ~HINT_CAPTION & ~HINT_SIZEBOX) |
            HINT_NOFRAME;

    /* Now apply styles to window */
    style = GetWindowLongPtr(hWnd, GWL_STYLE);
    if (!style)
        return;                 /* GetWindowLongPointer returns 0 on failure, we hope this isn't a valid style */

    style &= ~WS_CAPTION & ~WS_SIZEBOX; /* Just in case */

    if (!(hint & ~HINT_SKIPTASKBAR))    /* No hints, default */
        style = style | WS_CAPTION | WS_SIZEBOX;
    else if (hint & HINT_NOFRAME)       /* No frame, no decorations */
        style = style & ~WS_CAPTION & ~WS_SIZEBOX;
    else
        style = style | ((hint & HINT_BORDER) ? WS_BORDER : 0) |
            ((hint & HINT_SIZEBOX) ? WS_SIZEBOX : 0) |
            ((hint & HINT_CAPTION) ? WS_CAPTION : 0);

    if (hint & HINT_NOMAXIMIZE)
        style = style & ~WS_MAXIMIZEBOX;

    if (hint & HINT_NOMINIMIZE)
        style = style & ~WS_MINIMIZEBOX;

    if (hint & HINT_NOSYSMENU)
        style = style & ~WS_SYSMENU;

    if (hint & HINT_SKIPTASKBAR)
        style = style & ~WS_MINIMIZEBOX;        /* window will become lost if minimized */

    SetWindowLongPtr(hWnd, GWL_STYLE, style);

    exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    if (hint & HINT_SKIPTASKBAR)
        exStyle = (exStyle & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW;
    else
        exStyle = (exStyle & ~WS_EX_TOOLWINDOW) | WS_EX_APPWINDOW;
    SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle);

    winDebug
        ("winApplyHints: iWindow 0x%08x hints 0x%08x style 0x%08x exstyle 0x%08x\n",
         iWindow, hint, style, exStyle);
}

void
winUpdateWindowPosition(HWND hWnd, HWND * zstyle)
{
    int iX, iY, iWidth, iHeight;
    int iDx, iDy;
    RECT rcNew;
    WindowPtr pWin = GetProp(hWnd, WIN_WINDOW_PROP);
    DrawablePtr pDraw = NULL;

    if (!pWin)
        return;
    pDraw = &pWin->drawable;
    if (!pDraw)
        return;

    /* Get the X and Y location of the X window */
    iX = pWin->drawable.x + GetSystemMetrics(SM_XVIRTUALSCREEN);
    iY = pWin->drawable.y + GetSystemMetrics(SM_YVIRTUALSCREEN);

    /* Get the height and width of the X window */
    iWidth = pWin->drawable.width;
    iHeight = pWin->drawable.height;

    /* Setup a rectangle with the X window position and size */
    SetRect(&rcNew, iX, iY, iX + iWidth, iY + iHeight);

    winDebug("winUpdateWindowPosition - drawable extent (%d, %d)-(%d, %d)\n",
             rcNew.left, rcNew.top, rcNew.right, rcNew.bottom);

    AdjustWindowRectEx(&rcNew, GetWindowLongPtr(hWnd, GWL_STYLE), FALSE,
                       GetWindowLongPtr(hWnd, GWL_EXSTYLE));

    /* Don't allow window decoration to disappear off to top-left as a result of this adjustment */
    if (rcNew.left < GetSystemMetrics(SM_XVIRTUALSCREEN)) {
        iDx = GetSystemMetrics(SM_XVIRTUALSCREEN) - rcNew.left;
        rcNew.left += iDx;
        rcNew.right += iDx;
    }

    if (rcNew.top < GetSystemMetrics(SM_YVIRTUALSCREEN)) {
        iDy = GetSystemMetrics(SM_YVIRTUALSCREEN) - rcNew.top;
        rcNew.top += iDy;
        rcNew.bottom += iDy;
    }

    winDebug("winUpdateWindowPosition - Window extent (%d, %d)-(%d, %d)\n",
             rcNew.left, rcNew.top, rcNew.right, rcNew.bottom);

    /* Position the Windows window */
    SetWindowPos(hWnd, *zstyle, rcNew.left, rcNew.top,
                 rcNew.right - rcNew.left, rcNew.bottom - rcNew.top, 0);

}
