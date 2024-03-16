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
#endif

#include <limits.h>
#include <wchar.h>

#include <xcb/xcb.h>
#include <xcb/xfixes.h>

#include "winclipboard.h"
#include "internal.h"

/*
 * Constants
 */

#define CLIP_NUM_SELECTIONS		2
#define CLIP_OWN_NONE			-1
#define CLIP_OWN_PRIMARY		0
#define CLIP_OWN_CLIPBOARD		1

#define CP_ISO_8559_1 28591

/*
 * Global variables
 */

extern int xfixes_event_base;
BOOL fPrimarySelection = TRUE;

/*
 * Local variables
 */

static xcb_window_t s_iOwners[CLIP_NUM_SELECTIONS] = { XCB_NONE, XCB_NONE };
static const char *szSelectionNames[CLIP_NUM_SELECTIONS] =
    { "PRIMARY", "CLIPBOARD" };

static unsigned int lastOwnedSelectionIndex = CLIP_OWN_NONE;

static void
MonitorSelection(xcb_xfixes_selection_notify_event_t * e, unsigned int i)
{
    /* Look for owned -> not owned transition */
    if ((XCB_NONE == e->owner) && (XCB_NONE != s_iOwners[i])) {
        unsigned int other_index;

        winDebug("MonitorSelection - %s - Going from owned to not owned.\n",
                 szSelectionNames[i]);

        /* If this selection is not owned, the other monitored selection must be the most
           recently owned, if it is owned at all */
        if (i == CLIP_OWN_PRIMARY)
            other_index = CLIP_OWN_CLIPBOARD;
        if (i == CLIP_OWN_CLIPBOARD)
            other_index = CLIP_OWN_PRIMARY;
        if (XCB_NONE != s_iOwners[other_index])
            lastOwnedSelectionIndex = other_index;
        else
            lastOwnedSelectionIndex = CLIP_OWN_NONE;
    }

    /* Save last owned selection */
    if (XCB_NONE != e->owner) {
        lastOwnedSelectionIndex = i;
    }

    /* Save new selection owner or None */
    s_iOwners[i] = e->owner;
    winDebug("MonitorSelection - %s - Now owned by XID %x\n",
             szSelectionNames[i], e->owner);
}

xcb_atom_t
winClipboardGetLastOwnedSelectionAtom(ClipboardAtoms *atoms)
{
    if (lastOwnedSelectionIndex == CLIP_OWN_NONE)
        return XCB_NONE;

    if (lastOwnedSelectionIndex == CLIP_OWN_PRIMARY)
        return XCB_ATOM_PRIMARY;

    if (lastOwnedSelectionIndex == CLIP_OWN_CLIPBOARD)
        return atoms->atomClipboard;

    return XCB_NONE;
}


void
winClipboardInitMonitoredSelections(void)
{
    /* Initialize static variables */
    int i;
    for (i = 0; i < CLIP_NUM_SELECTIONS; ++i)
      s_iOwners[i] = XCB_NONE;

    lastOwnedSelectionIndex = CLIP_OWN_NONE;
}

static char *get_atom_name(xcb_connection_t *conn, xcb_atom_t atom)
{
    char *ret;
    xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, atom);
    xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(conn, cookie, NULL);
    if (!reply)
        return NULL;
    ret = malloc(xcb_get_atom_name_name_length(reply) + 1);
    if (ret) {
        memcpy(ret, xcb_get_atom_name_name(reply), xcb_get_atom_name_name_length(reply));
        ret[xcb_get_atom_name_name_length(reply)] = '\0';
    }
    free(reply);
    return ret;
}

static int
winClipboardSelectionNotifyTargets(HWND hwnd, xcb_window_t iWindow, xcb_connection_t *conn, ClipboardConversionData *data, ClipboardAtoms *atoms)
{
  /* Retrieve the selection data and delete the property */
  xcb_get_property_cookie_t cookie = xcb_get_property(conn,
                                                      TRUE,
                                                      iWindow,
                                                      atoms->atomLocalProperty,
                                                      XCB_GET_PROPERTY_TYPE_ANY,
                                                      0,
                                                      INT_MAX);
  xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
  if (!reply) {
      ErrorF("winClipboardFlushXEvents - SelectionNotify - "
             "XGetWindowProperty () failed\n");
  } else {
      xcb_atom_t *prop = xcb_get_property_value(reply);
      int nitems = xcb_get_property_value_length(reply)/sizeof(xcb_atom_t);
      int i;
      data->targetList = malloc((nitems+1)*sizeof(xcb_atom_t));

      for (i = 0; i < nitems; i++)
          {
              xcb_atom_t atom = prop[i];
              char *pszAtomName = get_atom_name(conn, atom);
              data->targetList[i] = atom;
              winDebug("winClipboardFlushXEvents - SelectionNotify - target[%d] %d = %s\n", i, atom, pszAtomName);
              free(pszAtomName);
      }

    data->targetList[nitems] = 0;

    free(reply);
  }

  return WIN_XEVENTS_NOTIFY_TARGETS;
}

static int
winClipboardSelectionNotifyData(HWND hwnd, xcb_window_t iWindow, xcb_connection_t *conn, ClipboardConversionData *data, ClipboardAtoms *atoms)
{
    xcb_atom_t encoding;
    int format;
    unsigned long int nitems;
    unsigned long int after;
    unsigned char *value;

    unsigned char *xtpText_value;
    xcb_atom_t xtpText_encoding;
    int xtpText_nitems;

    BOOL fSetClipboardData = TRUE;
    char *pszReturnData = NULL;
    UINT codepage;
    wchar_t *pwszUnicodeStr = NULL;
    HGLOBAL hGlobal = NULL;
    char *pszGlobalData = NULL;

    /* Retrieve the selection data and delete the property */
    xcb_get_property_cookie_t cookie = xcb_get_property(conn,
                                                        TRUE,
                                                        iWindow,
                                                        atoms->atomLocalProperty,
                                                        XCB_GET_PROPERTY_TYPE_ANY,
                                                        0,
                                                        INT_MAX);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
    if (!reply) {
        ErrorF("winClipboardFlushXEvents - SelectionNotify - "
               "XGetWindowProperty () failed\n");
        goto winClipboardFlushXEvents_SelectionNotify_Done;
    } else {
        nitems = xcb_get_property_value_length(reply);
        value =  xcb_get_property_value(reply);
        after = reply->bytes_after;
        encoding = reply->type;
        format = reply->format;
        // We assume format == 8 (i.e. data is a sequence of bytes).  It's not
        // clear how anything else should be handled.
        if (format != 8)
            ErrorF("SelectionNotify: format is %d, proceeding as if it was 8\n", format);
    }

    {
        char *pszAtomName;
        winDebug("SelectionNotify - returned data %lu left %lu\n", nitems, after);
        pszAtomName = get_atom_name(conn, encoding);
        winDebug("Notify atom name %s\n", pszAtomName);
        free(pszAtomName);
    }

    /* INCR reply indicates the start of a incremental transfer */
    if (encoding == atoms->atomIncr) {
        winDebug("winClipboardSelectionNotifyData: starting INCR, anticipated size %d\n", *(int *)value);
        data->incrsize = 0;
        data->incr = malloc(*(int *)value);
        // XXX: if malloc failed, we have an error
        return WIN_XEVENTS_SUCCESS;
    }
    else if (data->incr) {
        /* If an INCR transfer is in progress ... */
        if (nitems == 0) {
            winDebug("winClipboardSelectionNotifyData: ending INCR, actual size %ld\n", data->incrsize);
            /* a zero-length property indicates the end of the data */
            xtpText_value = data->incr;
            xtpText_encoding = encoding;
            // XXX: The type of the converted selection is the type of the first
            // partial property. The remaining partial properties must have the
            // same type.
            xtpText_nitems = data->incrsize;
        }
        else {
            /* Otherwise, continue appending the INCR data */
            winDebug("winClipboardSelectionNotifyData: INCR, %ld bytes\n", nitems);
            data->incr = realloc(data->incr, data->incrsize + nitems);
            memcpy(data->incr + data->incrsize, value, nitems);
            data->incrsize = data->incrsize + nitems;
            return WIN_XEVENTS_SUCCESS;
        }
    }
    else {
        /* Otherwise, the data is just contained in the property */
        winDebug("winClipboardSelectionNotifyData: non-INCR, %ld bytes\n", nitems);
        xtpText_value = value;
        xtpText_encoding = encoding;
        xtpText_nitems = nitems;
    }

    if (xtpText_encoding == atoms->atomUTF8String) {
        pszReturnData = malloc(xtpText_nitems + 1);
        memcpy(pszReturnData, xtpText_value, xtpText_nitems);
        pszReturnData[xtpText_nitems] = 0;
        codepage = CP_UTF8; // code page identifier for utf8
    } else if (xtpText_encoding == XCB_ATOM_STRING) {
        // STRING encoding is Latin1 (ISO8859-1) plus tab and newline
        pszReturnData = malloc(xtpText_nitems + 1);
        memcpy(pszReturnData, xtpText_value, xtpText_nitems);
        pszReturnData[xtpText_nitems] = 0;
        codepage = CP_ISO_8559_1; // code page identifier for iso-8559-1
    } else if (xtpText_encoding == atoms->atomCompoundText) {
        // COMPOUND_TEXT is complex, based on ISO 2022
        ErrorF("SelectionNotify: data in COMPOUND_TEXT encoding which is not implemented, discarding\n");
        pszReturnData = malloc(1);
        pszReturnData[0] = '\0';
    } else { // shouldn't happen as we accept no other encodings
        pszReturnData = malloc(1);
        pszReturnData[0] = '\0';
    }

    /* Free the data returned from xcb_get_property */
    free(reply);

    /* Free any INCR data */
    if (data->incr) {
        free(data->incr);
        data->incr = NULL;
        data->incrsize = 0;
    }

    /* Convert the X clipboard string to DOS format */
    winClipboardUNIXtoDOS(&pszReturnData, strlen(pszReturnData));

    /* Find out how much space needed when converted to UTF-16 */
    int iUnicodeLen = MultiByteToWideChar(codepage, 0,
                                          pszReturnData, -1, NULL, 0);

    /* NOTE: iUnicodeLen includes space for null terminator */
    pwszUnicodeStr = malloc(sizeof(wchar_t) * iUnicodeLen);
    if (!pwszUnicodeStr) {
        ErrorF("winClipboardFlushXEvents - SelectionNotify "
               "malloc failed for pwszUnicodeStr, aborting.\n");

        /* Abort */
        goto winClipboardFlushXEvents_SelectionNotify_Done;
    }

    /* Do the actual conversion */
    MultiByteToWideChar(codepage, 0,
                        pszReturnData, -1, pwszUnicodeStr, iUnicodeLen);

    /* Allocate global memory for the X clipboard data */
    hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * iUnicodeLen);

    free(pszReturnData);

    /* Check that global memory was allocated */
    if (!hGlobal) {
        ErrorF("winClipboardFlushXEvents - SelectionNotify "
               "GlobalAlloc failed, aborting: %08x\n", (unsigned int)GetLastError());

        /* Abort */
        goto winClipboardFlushXEvents_SelectionNotify_Done;
    }

    /* Obtain a pointer to the global memory */
    pszGlobalData = GlobalLock(hGlobal);
    if (pszGlobalData == NULL) {
        ErrorF("winClipboardFlushXEvents - Could not lock global "
               "memory for clipboard transfer\n");

        /* Abort */
        goto winClipboardFlushXEvents_SelectionNotify_Done;
    }

    /* Copy the returned string into the global memory */
    wcscpy((wchar_t *)pszGlobalData, pwszUnicodeStr);
    free(pwszUnicodeStr);
    pwszUnicodeStr = NULL;

    /* Release the pointer to the global memory */
    GlobalUnlock(hGlobal);
    pszGlobalData = NULL;

    /* Push the selection data to the Windows clipboard */
    SetClipboardData(CF_UNICODETEXT, hGlobal);

    /* Flag that SetClipboardData has been called */
    fSetClipboardData = FALSE;

    /*
     * NOTE: Do not try to free pszGlobalData, it is owned by
     * Windows after the call to SetClipboardData ().
     */

 winClipboardFlushXEvents_SelectionNotify_Done:
    /* Free allocated resources */
    free(pwszUnicodeStr);
    if (hGlobal && pszGlobalData)
        GlobalUnlock(hGlobal);
    if (fSetClipboardData) {
        SetClipboardData(CF_UNICODETEXT, NULL);
        SetClipboardData(CF_TEXT, NULL);
    }
    return WIN_XEVENTS_NOTIFY_DATA;
}

/*
 * Process any pending X events
 */

int
winClipboardFlushXEvents(HWND hwnd,
                         xcb_window_t iWindow, xcb_connection_t *conn,
                         ClipboardConversionData *data, ClipboardAtoms *atoms)
{
    xcb_atom_t atomClipboard = atoms->atomClipboard;
    xcb_atom_t atomUTF8String = atoms->atomUTF8String;
    xcb_atom_t atomCompoundText = atoms->atomCompoundText;
    xcb_atom_t atomTargets = atoms->atomTargets;

    /* Process all pending events */
    xcb_generic_event_t *event;
    while ((event = xcb_poll_for_event(conn))) {
        const char *pszGlobalData = NULL;
        HGLOBAL hGlobal = NULL;
        char *pszConvertData = NULL;
        BOOL fAbort = FALSE;
        BOOL fCloseClipboard = FALSE;

        /* Branch on the event type */
        switch (event->response_type & ~0x80) {
        case XCB_SELECTION_REQUEST:
        {
            char *xtpText_value = NULL;
            int xtpText_nitems;
            UINT codepage;

            xcb_selection_request_event_t *selection_request =  (xcb_selection_request_event_t *)event;
        {
            char *pszAtomName = NULL;

            winDebug("SelectionRequest - target %d\n", selection_request->target);

            pszAtomName = get_atom_name(conn, selection_request->target);
            winDebug("SelectionRequest - Target atom name %s\n", pszAtomName);
            free(pszAtomName);
        }

            /* Abort if invalid target type */
            if (selection_request->target != XCB_ATOM_STRING
                && selection_request->target != atomUTF8String
                && selection_request->target != atomCompoundText
                && selection_request->target != atomTargets) {
                /* Abort */
                fAbort = TRUE;
                goto winClipboardFlushXEvents_SelectionRequest_Done;
            }

            /* Handle targets type of request */
            if (selection_request->target == atomTargets) {
                xcb_atom_t atomTargetArr[] =
                    {
                     atomTargets,
                     atomUTF8String,
                     XCB_ATOM_STRING,
                     // atomCompoundText, not implemented (yet?)
                    };

                /* Try to change the property */
                xcb_void_cookie_t cookie = xcb_change_property_checked(conn,
                                          XCB_PROP_MODE_REPLACE,
                                          selection_request->requestor,
                                          selection_request->property,
                                          XCB_ATOM_ATOM,
                                          32,
                                          ARRAY_SIZE(atomTargetArr),
                                          (unsigned char *) atomTargetArr);
                xcb_generic_error_t *error;
                if ((error = xcb_request_check(conn, cookie))) {
                    ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                           "xcb_change_property failed");
                    free(error);
                }

                /* Setup selection notify xevent */
                xcb_selection_notify_event_t eventSelection;
                eventSelection.response_type = XCB_SELECTION_NOTIFY;
                eventSelection.requestor = selection_request->requestor;
                eventSelection.selection = selection_request->selection;
                eventSelection.target = selection_request->target;
                eventSelection.property = selection_request->property;
                eventSelection.time = selection_request->time;

                /*
                 * Notify the requesting window that
                 * the operation has completed
                 */
                cookie = xcb_send_event_checked(conn, FALSE,
                                                eventSelection.requestor,
                                                0, (char *) &eventSelection);
                if ((error = xcb_request_check(conn, cookie))) {
                    ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                           "xcb_send_event() failed\n");
                }
                break;
            }

            /* Close clipboard if we have it open already */
            if (GetOpenClipboardWindow() == hwnd) {
                CloseClipboard();
            }

            /* Access the clipboard */
            if (!OpenClipboard(hwnd)) {
                ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                       "OpenClipboard () failed: %08x\n", (unsigned int)GetLastError());

                /* Abort */
                fAbort = TRUE;
                goto winClipboardFlushXEvents_SelectionRequest_Done;
            }

            /* Indicate that clipboard was opened */
            fCloseClipboard = TRUE;

            /* Check that clipboard format is available */
            if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                static int count;       /* Hack to stop acroread spamming the log */
                static HWND lasthwnd;   /* I've not seen any other client get here repeatedly? */

                if (hwnd != lasthwnd)
                    count = 0;
                count++;
                if (count < 6)
                    ErrorF("winClipboardFlushXEvents - CF_UNICODETEXT is not "
                           "available from Win32 clipboard.  Aborting %d.\n",
                           count);
                lasthwnd = hwnd;

                /* Abort */
                fAbort = TRUE;
                goto winClipboardFlushXEvents_SelectionRequest_Done;
            }

            /* Get a pointer to the clipboard text, in desired format */
            /* Retrieve clipboard data */
            hGlobal = GetClipboardData(CF_UNICODETEXT);

            if (!hGlobal) {
                ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                       "GetClipboardData () failed: %08x\n", (unsigned int)GetLastError());

                /* Abort */
                fAbort = TRUE;
                goto winClipboardFlushXEvents_SelectionRequest_Done;
            }
            pszGlobalData = (char *) GlobalLock(hGlobal);

            /* Convert to target string style */
            if (selection_request->target == XCB_ATOM_STRING) {
                codepage = CP_ISO_8559_1; // code page identifier for iso-8559-1
            } else if (selection_request->target == atomUTF8String) {
                codepage = CP_UTF8; // code page identifier for utf8
            } else if (selection_request->target == atomCompoundText) {
                // COMPOUND_TEXT is complex, not (yet) implemented
                pszGlobalData = "COMPOUND_TEXT not implemented";
                codepage = CP_UTF8; // code page identifier for utf8
            }

            /* Convert the UTF16 string to required encoding */
            int iConvertDataLen = WideCharToMultiByte(codepage, 0,
                                                      (LPCWSTR) pszGlobalData, -1,
                                                      NULL, 0, NULL, NULL);
            /* NOTE: iConvertDataLen includes space for null terminator */
            pszConvertData = malloc(iConvertDataLen);
            WideCharToMultiByte(codepage, 0,
                                (LPCWSTR) pszGlobalData, -1,
                                pszConvertData, iConvertDataLen, NULL, NULL);

            /* Convert DOS string to UNIX string */
            winClipboardDOStoUNIX(pszConvertData, strlen(pszConvertData));

            xtpText_value = strdup(pszConvertData);
            xtpText_nitems = strlen(pszConvertData);

            /* data will fit into a single X request? (INCR not yet supported) */
            {
                uint32_t maxreqsize = xcb_get_maximum_request_length(conn);

                /* covert to bytes and allow for allow for X_ChangeProperty request */
                maxreqsize = maxreqsize*4 - 24;

                if (xtpText_nitems > maxreqsize) {
                    ErrorF("winClipboardFlushXEvents - clipboard data size %d greater than maximum %u\n", xtpText_nitems, maxreqsize);

                    /* Abort */
                    fAbort = TRUE;
                    goto winClipboardFlushXEvents_SelectionRequest_Done;
                }
            }

            /* Copy the clipboard text to the requesting window */
            xcb_void_cookie_t cookie = xcb_change_property_checked(conn,
                                      XCB_PROP_MODE_REPLACE,
                                      selection_request->requestor,
                                      selection_request->property,
                                      selection_request->target,
                                      8,
                                      xtpText_nitems, xtpText_value);
            xcb_generic_error_t *error;
            if ((error = xcb_request_check(conn, cookie))) {
                ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                       "xcb_change_property failed\n");

                /* Abort */
                fAbort = TRUE;
                goto winClipboardFlushXEvents_SelectionRequest_Done;
            }

            /* Free the converted string */
            free(pszConvertData);
            pszConvertData = NULL;

            /* Release the clipboard data */
            GlobalUnlock(hGlobal);
            pszGlobalData = NULL;
            fCloseClipboard = FALSE;
            CloseClipboard();

            /* Clean up */
            free(xtpText_value);
            xtpText_value = NULL;

            /* Setup selection notify event */
            xcb_selection_notify_event_t eventSelection;
            eventSelection.response_type = XCB_SELECTION_NOTIFY;
            eventSelection.requestor = selection_request->requestor;
            eventSelection.selection = selection_request->selection;
            eventSelection.target = selection_request->target;
            eventSelection.property = selection_request->property;
            eventSelection.time = selection_request->time;

            /* Notify the requesting window that the operation has completed */
            cookie = xcb_send_event_checked(conn, FALSE,
                                            eventSelection.requestor,
                                            0, (char *) &eventSelection);
            if ((error = xcb_request_check(conn, cookie))) {
                ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                       "xcb_send_event() failed\n");

                /* Abort */
                fAbort = TRUE;
                goto winClipboardFlushXEvents_SelectionRequest_Done;
            }

 winClipboardFlushXEvents_SelectionRequest_Done:
            /* Free allocated resources */
            if (xtpText_value) {
                free(xtpText_value);
            }
            if (pszConvertData)
                free(pszConvertData);
            if (hGlobal && pszGlobalData)
                GlobalUnlock(hGlobal);

            /*
             * Send a SelectionNotify event to the requesting
             * client when we abort.
             */
            if (fAbort) {
                /* Setup selection notify event */
                eventSelection.response_type = XCB_SELECTION_NOTIFY;
                eventSelection.requestor = selection_request->requestor;
                eventSelection.selection = selection_request->selection;
                eventSelection.target = selection_request->target;
                eventSelection.property = XCB_NONE;
                eventSelection.time = selection_request->time;

                /* Notify the requesting window that the operation is complete */
                cookie = xcb_send_event_checked(conn, FALSE,
                                                eventSelection.requestor,
                                                0, (char *) &eventSelection);
                if ((error = xcb_request_check(conn, cookie))) {
                    /*
                     * Should not be a problem if XSendEvent fails because
                     * the client may simply have exited.
                     */
                    ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                           "xcb_send_event() failed for abort event.\n");
                }
            }

            /* Close clipboard if it was opened */
            if (fCloseClipboard) {
                fCloseClipboard = FALSE;
                CloseClipboard();
            }
            break;
        }

        case XCB_SELECTION_NOTIFY:
        {
            xcb_selection_notify_event_t *selection_notify =  (xcb_selection_notify_event_t *)event;
            winDebug("winClipboardFlushXEvents - SelectionNotify\n");
            {
                char *pszAtomName;
                pszAtomName = get_atom_name(conn, selection_notify->selection);
                winDebug("winClipboardFlushXEvents - SelectionNotify - ATOM: %s\n", pszAtomName);
                free(pszAtomName);
            }

            /*
              SelectionNotify with property of XCB_NONE indicates either:

              (i) Generated by the X server if no owner for the specified selection exists
                  (perhaps it's disappeared on us mid-transaction), or
              (ii) Sent by the selection owner when the requested selection conversion could
                   not be performed or server errors prevented the conversion data being returned
            */
            if (selection_notify->property == XCB_NONE) {
                    ErrorF("winClipboardFlushXEvents - SelectionNotify - "
                           "Conversion to format %d refused.\n",
                           selection_notify->target);
                    return WIN_XEVENTS_FAILED;
                }

            if (selection_notify->target == atomTargets) {
              return winClipboardSelectionNotifyTargets(hwnd, iWindow, conn, data, atoms);
            }

            return winClipboardSelectionNotifyData(hwnd, iWindow, conn, data, atoms);
        }

        case XCB_SELECTION_CLEAR:
            winDebug("SelectionClear - doing nothing\n");
            break;

        case XCB_PROPERTY_NOTIFY:
        {
            xcb_property_notify_event_t *property_notify = (xcb_property_notify_event_t *)event;

            /* If INCR is in progress, collect the data */
            if (data->incr &&
                (property_notify->atom == atoms->atomLocalProperty) &&
                (property_notify->state == XCB_PROPERTY_NEW_VALUE))
                return winClipboardSelectionNotifyData(hwnd, iWindow, conn, data, atoms);

            break;
        }

        case XCB_MAPPING_NOTIFY:
            break;

        case 0:
            /* This is just laziness rather than making sure we used _checked everywhere */
            {
                xcb_generic_error_t *err = (xcb_generic_error_t *)event;
                ErrorF("winClipboardFlushXEvents - Error code: %i, ID: 0x%08x, "
                       "Major opcode: %i, Minor opcode: %i\n",
                       err->error_code, err->resource_id,
                       err->major_code, err->minor_code);
            }
            break;

        default:
            if ((event->response_type & ~0x80) == XCB_XFIXES_SELECTION_EVENT_SET_SELECTION_OWNER + xfixes_event_base) {
                xcb_xfixes_selection_notify_event_t *e = (xcb_xfixes_selection_notify_event_t *)event;
                winDebug("winClipboardFlushXEvents - XFixesSetSelectionOwnerNotify\n");

                /* Save selection owners for monitored selections, ignore other selections */
                if ((e->selection == XCB_ATOM_PRIMARY) && fPrimarySelection) {
                    MonitorSelection(e, CLIP_OWN_PRIMARY);
                }
                else if (e->selection == atomClipboard) {
                    MonitorSelection(e, CLIP_OWN_CLIPBOARD);
                }
                else
                    break;

                /* Selection is being disowned */
                if (e->owner == XCB_NONE) {
                    winDebug("winClipboardFlushXEvents - No window, returning.\n");
                    break;
                }

                /*
                   XXX: there are all kinds of wacky edge cases we might need here:
                   - we own windows clipboard, but neither PRIMARY nor CLIPBOARD have an owner, so we should disown it?
                   - root window is taking ownership?
                 */

                /* If we are the owner of the most recently owned selection, don't go all recursive :) */
                if ((lastOwnedSelectionIndex != CLIP_OWN_NONE) &&
                    (s_iOwners[lastOwnedSelectionIndex] == iWindow)) {
                    winDebug("winClipboardFlushXEvents - Ownership changed to us, aborting.\n");
                    break;
                }

                /* Close clipboard if we have it open already (possible? correct??) */
                if (GetOpenClipboardWindow() == hwnd) {
                    CloseClipboard();
                }

                /* Access the Windows clipboard */
                if (!OpenClipboard(hwnd)) {
                    ErrorF("winClipboardFlushXEvents - OpenClipboard () failed: %08x\n",
                           (int) GetLastError());
                    break;
                }

                /* Take ownership of the Windows clipboard */
                if (!EmptyClipboard()) {
                    ErrorF("winClipboardFlushXEvents - EmptyClipboard () failed: %08x\n",
                           (int) GetLastError());
                    break;
                }

                /* Advertise regular text and unicode */
                SetClipboardData(CF_UNICODETEXT, NULL);
                SetClipboardData(CF_TEXT, NULL);

                /* Release the clipboard */
                if (!CloseClipboard()) {
                    ErrorF("winClipboardFlushXEvents - CloseClipboard () failed: %08x\n",
                           (int) GetLastError());
                    break;
                }
            }
            /* XCB_XFIXES_SELECTION_EVENT_SELECTION_WINDOW_DESTROY */
            /* XCB_XFIXES_SELECTION_EVENT_SELECTION_CLIENT_CLOSE */
            else {
                ErrorF("winClipboardFlushXEvents - unexpected event type %d\n",
                       event->response_type);
            }
            break;
        }

        /* I/O errors etc. */
        {
            int e = xcb_connection_has_error(conn);
            if (e) {
                ErrorF("winClipboardFlushXEvents - Fatal error %d on xcb connection\n", e);
                break;
            }
        }
    }

    return WIN_XEVENTS_SUCCESS;

}
