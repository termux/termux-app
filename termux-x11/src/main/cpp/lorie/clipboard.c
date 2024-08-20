/* Copyright 2016-2019 Pierre Ossman for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#pragma clang diagnostic ignored "-Wunknown-pragmas"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <android/log.h>
#include <X11/Xatom.h>
#include <windowstr.h>
#include <selection.h>
#include <propertyst.h>
#include <xacestr.h>

#include "lorie.h"

/* utility functions for text conversion */

static inline void lorieConvertLF(const char* src, char *dst, size_t bytes) {
    size_t i = 0, j = 0;
    for (; i < bytes; i++)
        if (src[i] != '\r')
            dst[j++] = src[i];
}

static inline void lorieLatin1ToUTF8(unsigned char* out, const unsigned char* in) {
    while (*in)
        if (*in < 128)
            *out++ = *in++;
        else
            *out++ = 0xc2 + (*in > 0xbf), *out++ = (*in++ & 0x3f) + 0x80;
}

static inline int lorieCheckUTF8(const unsigned char *utf, size_t size) {
    int ix;
    unsigned char c;

    for (ix = 0; (c = utf[ix]) && ix < size;) {
        if (c & 0x80) {
            if ((utf[ix + 1] & 0xc0) != 0x80)
                return 0;
            if ((c & 0xe0) == 0xe0) {
                if ((utf[ix + 2] & 0xc0) != 0x80)
                    return 0;
                if ((c & 0xf0) == 0xf0) {
                    if ((c & 0xf8) != 0xf0 || (utf[ix + 3] & 0xc0) != 0x80)
                        return 0;
                    ix += 4;
                    /* 4-byte code */
                } else
                    /* 3-byte code */
                    ix += 3;
            } else
                /* 2-byte code */
                ix += 2;
        } else
            /* 1-byte code */
            ix++;
    }
    return 1;
}

static size_t lorieUtf8ToUCS4(const char* src, size_t max, unsigned* dst) {
    size_t count, consumed;

    *dst = 0xfffd;

    if (max == 0)
        return 0;

    consumed = 1;

    if ((*src & 0x80) == 0) {
        *dst = *src;
        count = 0;
    } else if ((*src & 0xe0) == 0xc0) {
        *dst = *src & 0x1f;
        count = 1;
    } else if ((*src & 0xf0) == 0xe0) {
        *dst = *src & 0x0f;
        count = 2;
    } else if ((*src & 0xf8) == 0xf0) {
        *dst = *src & 0x07;
        count = 3;
    } else {
        // Invalid sequence, consume all continuation characters
        src++;
        max--;
        while ((max-- > 0) && ((*src++ & 0xc0) == 0x80))
            consumed++;
        return consumed;
    }

    src++;
    max--;

    while (count--) {
        consumed++;

        // Invalid or truncated sequence?
        if ((max == 0) || ((*src & 0xc0) != 0x80)) {
            *dst = 0xfffd;
            return consumed;
        }

        *dst <<= 6;
        *dst |= *src & 0x3f;

        src++;
        max--;
    }

    // UTF-16 surrogate code point?
    if ((*dst >= 0xd800) && (*dst < 0xe000))
        *dst = 0xfffd;

    return consumed;
}

static const char *lorieUtf8ToLatin1(const char *src) {
    size_t sz;

    const char* in;
    size_t in_len;

    // Compute output size
    sz = 0;
    in = src;
    in_len = -1;
    while ((in_len > 0) && (*in != '\0')) {
        size_t len;
        unsigned ucs;

        len = lorieUtf8ToUCS4(in, in_len, &ucs);
        in += len;
        in_len -= len;
        sz++;
    }

    // Reserve space
    unsigned char out[sz + 1];
    memset(out, 0, sz + 1);
    size_t position = 0;

    // And convert
    in = src;
    in_len = 4.294967295E9;
    while ((in_len > 0) && (*in != '\0')) {
        size_t len;
        unsigned ucs;

        len = lorieUtf8ToUCS4(in, in_len, &ucs);
        in += len;
        in_len -= len;

        if (ucs > 0xff)
            out[position++] = '?';
        else
            out[position++] = (unsigned char)ucs;
    }

    return strdup((const char*) out);
}

/* end utility functions */

#define log(prio, ...) __android_log_print(ANDROID_LOG_ ## prio, "LorieNative", __VA_ARGS__)
extern ScreenPtr pScreenPtr;

static int (*origProcSendEvent)(ClientPtr) = NULL;
static int (*origProcConvertSelection)(ClientPtr) = NULL;
static Atom xaTIMESTAMP = 0, xaTEXT = 0, xaCLIPBOARD = 0, xaTARGETS = 0, xaSTRING = 0, xaUTF8_STRING = 0;
static Bool clipboardEnabled = FALSE;
static const char* cachedData = NULL;

struct LorieDataTarget {
    ClientPtr client;
    Atom selection;
    Atom target;
    Atom property;
    Window requestor;
    CARD32 time;
    struct LorieDataTarget* next;
} *lorieDataTargetHead;

void lorieEnableClipboardSync(Bool enable) {
    clipboardEnabled = enable;
}

/* functions related to clipboard receiving */

static void lorieSelectionRequest(Atom selection, Atom target) {
    Selection *pSel;

    if (clipboardEnabled && dixLookupSelection(&pSel, selection, serverClient, DixGetAttrAccess) == Success) {
        xEvent event = {0};
        event.u.u.type = SelectionRequest;
        event.u.selectionRequest.owner = pSel->window;
        event.u.selectionRequest.time = currentTime.milliseconds;
        event.u.selectionRequest.requestor = pScreenPtr->root->drawable.id;
        event.u.selectionRequest.selection = selection;
        event.u.selectionRequest.target = target;
        event.u.selectionRequest.property = target;
        WriteEventsToClient(pSel->client, 1, &event);
    }
}

static Bool lorieHasAtom(Atom atom, const Atom list[], size_t size) {
    for (size_t i = 0; i < size; i++)
        if (list[i] == atom)
            return TRUE;

    return FALSE;
}

static void lorieHandleSelection(Atom target) {
    PropertyPtr prop;
    if (target != xaTARGETS && target != xaSTRING && target != xaUTF8_STRING)
        return;

    if (dixLookupProperty(&prop, pScreenPtr->root, target, serverClient, DixReadAccess) != Success)
        return;

    log(DEBUG, "Selection notification for CLIPBOARD (target %s, type %s)\n", NameForAtom(target), NameForAtom(prop->type));

    if (target == xaTARGETS && prop->type == XA_ATOM && prop->format == 32) {
        if (lorieHasAtom(xaUTF8_STRING, (const Atom*)prop->data, prop->size))
            lorieSelectionRequest(xaCLIPBOARD, xaUTF8_STRING);
        else if (lorieHasAtom(xaSTRING, (const Atom*)prop->data, prop->size))
            lorieSelectionRequest(xaCLIPBOARD, xaSTRING);
    } else if (target == xaSTRING && prop->type == xaSTRING && prop->format == 8) {
        if (prop->format != 8 || prop->type != xaSTRING)
            return;

        char filtered[prop->size + 1], utf8[(prop->size + 1) * 2];
        memset(filtered, 0, sizeof(filtered));
        memset(utf8, 0, sizeof(utf8));

        lorieConvertLF(prop->data,  filtered, prop->size);
        lorieLatin1ToUTF8((unsigned char*) utf8, (unsigned char*) filtered);
        log(DEBUG, "Sending clipboard to clients (%zu bytes)\n", strlen(utf8));
        lorieSendClipboardData(utf8);
    } else if (target == xaUTF8_STRING && prop->type == xaUTF8_STRING && prop->format == 8) {
        char filtered[prop->size + 1];

        if (!lorieCheckUTF8(prop->data, prop->size)) {
            dprintf(2, "Invalid UTF-8 sequence in clipboard\n");
            return;
        }

        memset(filtered, 0, prop->size + 1);
        lorieConvertLF(prop->data, filtered, prop->size);

        log(DEBUG, "Sending clipboard to clients (%zu bytes)\n", strlen(filtered));
        lorieSendClipboardData(filtered);
    }
}

static int lorieProcSendEvent(ClientPtr client) {
    REQUEST(xSendEventReq)
    REQUEST_SIZE_MATCH(xSendEventReq);
    __typeof__(stuff->event.u.selectionNotify)* e = &stuff->event.u.selectionNotify;

    if (clipboardEnabled && e->requestor == pScreenPtr->root->drawable.id &&
        stuff->event.u.u.type == SelectionNotify && e->selection == xaCLIPBOARD && e->target == e->property)
        lorieHandleSelection(e->target);

    return origProcSendEvent(client);
}

static void lorieSelectionCallback(__unused CallbackListPtr *callbacks, __unused void * data, void * args) {
    SelectionInfoRec *info = (SelectionInfoRec *) args;

    if (clipboardEnabled && info->selection->selection == xaCLIPBOARD && info->kind == SelectionSetOwner)
        lorieSelectionRequest(xaCLIPBOARD, xaTARGETS);
}

/* end functions related to clipboard receiving */

/* functions related to clipboard announcing and sending */

static int lorieConvertSelection(ClientPtr client, Atom selection, Atom target, Atom property, Window requestor, CARD32 time, const char* data) {
    Selection *pSel;
    WindowPtr pWin;
    int rc;

    Atom realProperty;

    xEvent event;

    if (data == NULL) {
        log(DEBUG, "Selection request for %s (type %s)",
            NameForAtom(selection), NameForAtom(target));
    } else {
        log(DEBUG, "Sending data for selection request for %s (type %s)",
            NameForAtom(selection), NameForAtom(target));
    }

    rc = dixLookupSelection(&pSel, selection, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    /* We do not validate the time argument because neither does
     * dix/selection.c and some clients (e.g. Qt) relies on this */

    rc = dixLookupWindow(&pWin, requestor, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    if (property != None)
        realProperty = property;
    else
        realProperty = target;

    /* FIXME: MULTIPLE target */

    if (target == xaTARGETS) {
        Atom targets[] = { xaTARGETS, xaTIMESTAMP,
                           xaSTRING, xaTEXT, xaUTF8_STRING };

        rc = dixChangeWindowProperty(serverClient, pWin, realProperty,
                                     XA_ATOM, 32, PropModeReplace,
                                     sizeof(targets)/sizeof(targets[0]),
                                     targets, TRUE);
        if (rc != Success)
            return rc;
    } else if (target == xaTIMESTAMP) {
        rc = dixChangeWindowProperty(serverClient, pWin, realProperty,
                                     XA_INTEGER, 32, PropModeReplace, 1,
                                     &pSel->lastTimeChanged.milliseconds,
                                     TRUE);
        if (rc != Success)
            return rc;
    } else {
        if (data == NULL) {
            struct LorieDataTarget* ldt;

            if ((target != xaSTRING) && (target != xaTEXT) &&
                (target != xaUTF8_STRING))
                return BadMatch;

            ldt = calloc(1, sizeof(struct LorieDataTarget));
            if (ldt == NULL)
                return BadAlloc;

            ldt->client = client;
            ldt->selection = selection;
            ldt->target = target;
            ldt->property = property;
            ldt->requestor = requestor;
            ldt->time = time;

            ldt->next = lorieDataTargetHead;
            lorieDataTargetHead = ldt;

            log(DEBUG, "Requesting clipboard data from client");
            lorieRequestClipboard();

            return Success;
        } else {
            if ((target == xaSTRING) || (target == xaTEXT)) {
                const char* latin1 = lorieUtf8ToLatin1(data);
                if (latin1 == NULL)
                    return BadAlloc;

                rc = dixChangeWindowProperty(serverClient, pWin, realProperty,
                                             XA_STRING, 8, PropModeReplace,
                                             strlen(latin1), latin1, TRUE);

                free((void*) latin1);

                if (rc != Success)
                    return rc;
            } else if (target == xaUTF8_STRING) {
                rc = dixChangeWindowProperty(serverClient, pWin, realProperty,
                                             xaUTF8_STRING, 8, PropModeReplace,
                                             strlen(data), data, TRUE);
                if (rc != Success)
                    return rc;
            } else {
                return BadMatch;
            }
        }
    }

    event.u.u.type = SelectionNotify;
    event.u.selectionNotify.time = time;
    event.u.selectionNotify.requestor = requestor;
    event.u.selectionNotify.selection = selection;
    event.u.selectionNotify.target = target;
    event.u.selectionNotify.property = property;
    WriteEventsToClient(client, 1, &event);
    return Success;
}

static int lorieProcConvertSelection(ClientPtr client) {
    Bool paramsOkay;
    WindowPtr pWin;
    Selection *pSel;
    int rc;

    REQUEST(xConvertSelectionReq)
    REQUEST_SIZE_MATCH(xConvertSelectionReq);

    rc = dixLookupWindow(&pWin, stuff->requestor, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    paramsOkay = ValidAtom(stuff->selection) && ValidAtom(stuff->target);
    paramsOkay &= (stuff->property == None) || ValidAtom(stuff->property);
    if (!paramsOkay) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    /* Do we own this selection? */
    rc = dixLookupSelection(&pSel, stuff->selection, client, DixReadAccess);
    if (rc == Success && pSel->client == serverClient && pSel->window == pScreenPtr->root->drawable.id) {
        /* cachedData will be NULL for the first request, but can then be
         * reused once we've gotten the data once from the client */
        rc = lorieConvertSelection(client, stuff->selection,
                                   stuff->target, stuff->property,
                                   stuff->requestor, stuff->time,
                                   cachedData);
        if (rc != Success) {
            xEvent event;

            memset(&event, 0, sizeof(xEvent));
            event.u.u.type = SelectionNotify;
            event.u.selectionNotify.time = stuff->time;
            event.u.selectionNotify.requestor = stuff->requestor;
            event.u.selectionNotify.selection = stuff->selection;
            event.u.selectionNotify.target = stuff->target;
            event.u.selectionNotify.property = None;
            WriteEventsToClient(client, 1, &event);
        }

        return Success;
    }

    return origProcConvertSelection(client);
}

static int lorieOwnSelection(Atom selection) {
    Selection *pSel;
    int rc;

    SelectionInfoRec info;

    rc = dixLookupSelection(&pSel, selection, serverClient, DixSetAttrAccess);
    if (rc == Success) {
        if (pSel->client && (pSel->client != serverClient)) {
            xEvent event = {
                    .u.selectionClear.time = currentTime.milliseconds,
                    .u.selectionClear.window = pSel->window,
                    .u.selectionClear.atom = pSel->selection
            };
            event.u.u.type = SelectionClear;
            WriteEventsToClient(pSel->client, 1, &event);
        }
    } else if (rc == BadMatch) {
        pSel = dixAllocateObjectWithPrivates(Selection, PRIVATE_SELECTION);
        if (!pSel)
            return BadAlloc;

        pSel->selection = selection;

        rc = XaceHookSelectionAccess(serverClient, &pSel, DixCreateAccess | DixSetAttrAccess);
        if (rc != Success) {
            free(pSel);
            return rc;
        }

        pSel->next = CurrentSelections;
        CurrentSelections = pSel;
    }
    else
        return rc;

    pSel->lastTimeChanged = currentTime;
    pSel->window = pScreenPtr->root->drawable.id;
    pSel->pWin = pScreenPtr->root;
    pSel->client = serverClient;

    log(DEBUG, "Grabbed %s selection", NameForAtom(selection));

    info.selection = pSel;
    info.client = serverClient;
    info.kind = SelectionSetOwner;
    CallCallbacks(&SelectionCallback, &info);

    return Success;
}

void lorieHandleClipboardAnnounce(void) {
    // The data has changed in some way, so whatever is in our cache is now stale
    free((void*) cachedData);
    cachedData = NULL;

    int rc;

    log(DEBUG, "Remote clipboard announced, grabbing local ownership");

    rc = lorieOwnSelection(xaCLIPBOARD);
    if (rc != Success)
        log(ERROR, "Could not set CLIPBOARD selection");
}

void lorieHandleClipboardData(const char* data) {
    struct LorieDataTarget* next;

    log(DEBUG, "Got remote clipboard data, sending to X11 clients");

    free((void*) cachedData);
    cachedData = data;

    while (lorieDataTargetHead != NULL) {
        int rc;
        xEvent event;

        rc = lorieConvertSelection(lorieDataTargetHead->client,
                                   lorieDataTargetHead->selection,
                                   lorieDataTargetHead->target,
                                   lorieDataTargetHead->property,
                                   lorieDataTargetHead->requestor,
                                   lorieDataTargetHead->time,
                                 cachedData);
        if (rc != Success) {
            event.u.u.type = SelectionNotify;
            event.u.selectionNotify.time = lorieDataTargetHead->time;
            event.u.selectionNotify.requestor = lorieDataTargetHead->requestor;
            event.u.selectionNotify.selection = lorieDataTargetHead->selection;
            event.u.selectionNotify.target = lorieDataTargetHead->target;
            event.u.selectionNotify.property = None;
            WriteEventsToClient(lorieDataTargetHead->client, 1, &event);
        }

        next = lorieDataTargetHead->next;
        free(lorieDataTargetHead);
        lorieDataTargetHead = next;
    }
}

/* end functions related to clipboard announcing and sending */

void lorieInitClipboard(void) {
#define ATOM(name) xa##name = MakeAtom(#name, strlen(#name), TRUE)
    ATOM(TIMESTAMP); ATOM(TEXT); ATOM(CLIPBOARD); ATOM(TARGETS); ATOM(STRING); ATOM(UTF8_STRING);

    if (!origProcConvertSelection) {
        origProcConvertSelection = ProcVector[X_ConvertSelection];
        ProcVector[X_ConvertSelection] = lorieProcConvertSelection;
    }

    if (!origProcSendEvent) {
        origProcSendEvent = ProcVector[X_SendEvent];
        ProcVector[X_SendEvent] = lorieProcSendEvent;
    }

    if (!AddCallback(&SelectionCallback, lorieSelectionCallback, NULL))
        FatalError("Adding SelectionCallback failed\n");
}