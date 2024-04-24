/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*

(c)Copyright 1988,1991 Adobe Systems Incorporated. All rights reserved.

Permission to use, copy, modify, distribute, and sublicense this software and its
documentation for any purpose and without fee is hereby granted, provided that
the above copyright notices appear in all copies and that both those copyright
notices and this permission notice appear in supporting documentation and that
the name of Adobe Systems Incorporated not be used in advertising or publicity
pertaining to distribution of the software without specific, written prior
permission.  No trademark license to use the Adobe trademarks is hereby
granted.  If the Adobe trademark "Display PostScript"(tm) is used to describe
this software, its functionality or for any other purpose, such use shall be
limited to a statement that this software works in conjunction with the Display
PostScript system.  Proper trademark attribution to reflect Adobe's ownership
of the trademark shall be given whenever any such reference to the Display
PostScript system is made.

ADOBE MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THE SOFTWARE FOR ANY
PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.  ADOBE
DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-
INFRINGEMENT OF THIRD PARTY RIGHTS.  IN NO EVENT SHALL ADOBE BE LIABLE TO YOU
OR ANY OTHER PARTY FOR ANY SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER WHETHER IN AN ACTION OF CONTRACT,NEGLIGENCE, STRICT
LIABILITY OR ANY OTHER ACTION ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.  ADOBE WILL NOT PROVIDE ANY TRAINING OR OTHER
SUPPORT FOR THE SOFTWARE.

Adobe, PostScript, and Display PostScript are trademarks of Adobe Systems
Incorporated which may be registered in certain jurisdictions.

Author:  Adobe Systems Incorporated

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include "misc.h"
#include "windowstr.h"
#include "dixstruct.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#define  XK_LATIN1
#include <X11/keysymdef.h>
#include "xace.h"

/*
 * CompareTimeStamps returns -1, 0, or +1 depending on if the first
 * argument is less than, equal to or greater than the second argument.
 */

int
CompareTimeStamps(TimeStamp a, TimeStamp b)
{
    if (a.months < b.months)
        return EARLIER;
    if (a.months > b.months)
        return LATER;
    if (a.milliseconds < b.milliseconds)
        return EARLIER;
    if (a.milliseconds > b.milliseconds)
        return LATER;
    return SAMETIME;
}

/*
 * convert client times to server TimeStamps
 */

#define HALFMONTH ((unsigned long) 1<<31)
TimeStamp
ClientTimeToServerTime(CARD32 c)
{
    TimeStamp ts;

    if (c == CurrentTime)
        return currentTime;
    ts.months = currentTime.months;
    ts.milliseconds = c;
    if (c > currentTime.milliseconds) {
        if (((unsigned long) c - currentTime.milliseconds) > HALFMONTH)
            ts.months -= 1;
    }
    else if (c < currentTime.milliseconds) {
        if (((unsigned long) currentTime.milliseconds - c) > HALFMONTH)
            ts.months += 1;
    }
    return ts;
}

/*
 * ISO Latin-1 case conversion routine
 *
 * this routine always null-terminates the result, so
 * beware of too-small buffers
 */

static unsigned char
ISOLatin1ToLower(unsigned char source)
{
    unsigned char dest;

    if ((source >= XK_A) && (source <= XK_Z))
        dest = source + (XK_a - XK_A);
    else if ((source >= XK_Agrave) && (source <= XK_Odiaeresis))
        dest = source + (XK_agrave - XK_Agrave);
    else if ((source >= XK_Ooblique) && (source <= XK_Thorn))
        dest = source + (XK_oslash - XK_Ooblique);
    else
        dest = source;
    return dest;
}

int
CompareISOLatin1Lowered(const unsigned char *s1, int s1len,
                        const unsigned char *s2, int s2len)
{
    unsigned char c1, c2;

    for (;;) {
        /* note -- compare against zero so that -1 ignores len */
        c1 = s1len-- ? *s1++ : '\0';
        c2 = s2len-- ? *s2++ : '\0';
        if (!c1 ||
            (c1 != c2 &&
             (c1 = ISOLatin1ToLower(c1)) != (c2 = ISOLatin1ToLower(c2))))
            break;
    }
    return (int) c1 - (int) c2;
}

/*
 * dixLookupWindow and dixLookupDrawable:
 * Look up the window/drawable taking into account the client doing the
 * lookup, the type of drawable desired, and the type of access desired.
 * Return Success with *pDraw set if the window/drawable exists and the client
 * is allowed access, else return an error code with *pDraw set to NULL.  The
 * access mask values are defined in resource.h.  The type mask values are
 * defined in pixmap.h, with zero equivalent to M_DRAWABLE.
 */
int
dixLookupDrawable(DrawablePtr *pDraw, XID id, ClientPtr client,
                  Mask type, Mask access)
{
    DrawablePtr pTmp;
    int rc;

    *pDraw = NULL;

    rc = dixLookupResourceByClass((void **) &pTmp, id, RC_DRAWABLE, client,
                                  access);

    if (rc != Success)
        client->errorValue = id;

    if (rc == BadValue)
        return BadDrawable;
    if (rc != Success)
        return rc;
    if (!((1 << pTmp->type) & (type ? type : M_DRAWABLE)))
        return BadMatch;

    *pDraw = pTmp;
    return Success;
}

int
dixLookupWindow(WindowPtr *pWin, XID id, ClientPtr client, Mask access)
{
    int rc;

    rc = dixLookupDrawable((DrawablePtr *) pWin, id, client, M_WINDOW, access);
    /* dixLookupDrawable returns BadMatch iff id is a valid Drawable
       but is not a Window. Users of dixLookupWindow expect a BadWindow
       error in this case; they don't care that it's a valid non-Window XID */
    if (rc == BadMatch)
        rc = BadWindow;
    /* Similarly, users of dixLookupWindow don't want BadDrawable. */
    if (rc == BadDrawable)
        rc = BadWindow;
    return rc;
}

int
dixLookupGC(GCPtr *pGC, XID id, ClientPtr client, Mask access)
{
    return dixLookupResourceByType((void **) pGC, id, RT_GC, client, access);
}

int
dixLookupFontable(FontPtr *pFont, XID id, ClientPtr client, Mask access)
{
    int rc;
    GC *pGC;

    client->errorValue = id;    /* EITHER font or gc */
    rc = dixLookupResourceByType((void **) pFont, id, RT_FONT, client,
                                 access);
    if (rc != BadFont)
        return rc;
    rc = dixLookupResourceByType((void **) &pGC, id, RT_GC, client, access);
    if (rc == BadGC)
        return BadFont;
    if (rc == Success)
        *pFont = pGC->font;
    return rc;
}

int
dixLookupClient(ClientPtr *pClient, XID rid, ClientPtr client, Mask access)
{
    void *pRes;
    int rc = BadValue, clientIndex = CLIENT_ID(rid);

    if (!clientIndex || !clients[clientIndex] || (rid & SERVER_BIT))
        goto bad;

    rc = dixLookupResourceByClass(&pRes, rid, RC_ANY, client, DixGetAttrAccess);
    if (rc != Success)
        goto bad;

    rc = XaceHook(XACE_CLIENT_ACCESS, client, clients[clientIndex], access);
    if (rc != Success)
        goto bad;

    *pClient = clients[clientIndex];
    return Success;
 bad:
    if (client)
        client->errorValue = rid;
    *pClient = NULL;
    return rc;
}

int
AlterSaveSetForClient(ClientPtr client, WindowPtr pWin, unsigned mode,
                      Bool toRoot, Bool map)
{
    int numnow;
    SaveSetElt *pTmp = NULL;
    int j;

    numnow = client->numSaved;
    j = 0;
    if (numnow) {
        pTmp = client->saveSet;
        while ((j < numnow) && (SaveSetWindow(pTmp[j]) != (void *) pWin))
            j++;
    }
    if (mode == SetModeInsert) {
        if (j < numnow)         /* duplicate */
            return Success;
        numnow++;
        pTmp = (SaveSetElt *) realloc(client->saveSet, sizeof(*pTmp) * numnow);
        if (!pTmp)
            return BadAlloc;
        client->saveSet = pTmp;
        client->numSaved = numnow;
        SaveSetAssignWindow(client->saveSet[numnow - 1], pWin);
        SaveSetAssignToRoot(client->saveSet[numnow - 1], toRoot);
        SaveSetAssignMap(client->saveSet[numnow - 1], map);
        return Success;
    }
    else if ((mode == SetModeDelete) && (j < numnow)) {
        while (j < numnow - 1) {
            pTmp[j] = pTmp[j + 1];
            j++;
        }
        numnow--;
        if (numnow) {
            pTmp =
                (SaveSetElt *) realloc(client->saveSet, sizeof(*pTmp) * numnow);
            if (pTmp)
                client->saveSet = pTmp;
        }
        else {
            free(client->saveSet);
            client->saveSet = (SaveSetElt *) NULL;
        }
        client->numSaved = numnow;
        return Success;
    }
    return Success;
}

void
DeleteWindowFromAnySaveSet(WindowPtr pWin)
{
    int i;
    ClientPtr client;

    for (i = 0; i < currentMaxClients; i++) {
        client = clients[i];
        if (client && client->numSaved)
            (void) AlterSaveSetForClient(client, pWin, SetModeDelete, FALSE,
                                         TRUE);
    }
}

/* No-op Don't Do Anything : sometimes we need to be able to call a procedure
 * that doesn't do anything.  For example, on screen with only static
 * colormaps, if someone calls install colormap, it's easier to have a dummy
 * procedure to call than to check if there's a procedure
 */
void
NoopDDA(void)
{
}

typedef struct _BlockHandler {
    ServerBlockHandlerProcPtr BlockHandler;
    ServerWakeupHandlerProcPtr WakeupHandler;
    void *blockData;
    Bool deleted;
} BlockHandlerRec, *BlockHandlerPtr;

static BlockHandlerPtr handlers;
static int numHandlers;
static int sizeHandlers;
static Bool inHandler;
static Bool handlerDeleted;

/**
 *
 *  \param pTimeout   DIX doesn't want to know how OS represents time
 *  \param pReadMask  nor how it represents the det of descriptors
 */
void
BlockHandler(void *pTimeout)
{
    int i, j;

    ++inHandler;
    for (i = 0; i < numHandlers; i++)
        if (!handlers[i].deleted)
            (*handlers[i].BlockHandler) (handlers[i].blockData, pTimeout);

    for (i = 0; i < screenInfo.numGPUScreens; i++)
        (*screenInfo.gpuscreens[i]->BlockHandler) (screenInfo.gpuscreens[i], pTimeout);

    for (i = 0; i < screenInfo.numScreens; i++)
        (*screenInfo.screens[i]->BlockHandler) (screenInfo.screens[i], pTimeout);

    if (handlerDeleted) {
        for (i = 0; i < numHandlers;)
            if (handlers[i].deleted) {
                for (j = i; j < numHandlers - 1; j++)
                    handlers[j] = handlers[j + 1];
                numHandlers--;
            }
            else
                i++;
        handlerDeleted = FALSE;
    }
    --inHandler;
}

/**
 *
 *  \param result    32 bits of undefined result from the wait
 *  \param pReadmask the resulting descriptor mask
 */
void
WakeupHandler(int result)
{
    int i, j;

    ++inHandler;
    for (i = 0; i < screenInfo.numScreens; i++)
        (*screenInfo.screens[i]->WakeupHandler) (screenInfo.screens[i], result);
    for (i = 0; i < screenInfo.numGPUScreens; i++)
        (*screenInfo.gpuscreens[i]->WakeupHandler) (screenInfo.gpuscreens[i], result);
    for (i = numHandlers - 1; i >= 0; i--)
        if (!handlers[i].deleted)
            (*handlers[i].WakeupHandler) (handlers[i].blockData, result);
    if (handlerDeleted) {
        for (i = 0; i < numHandlers;)
            if (handlers[i].deleted) {
                for (j = i; j < numHandlers - 1; j++)
                    handlers[j] = handlers[j + 1];
                numHandlers--;
            }
            else
                i++;
        handlerDeleted = FALSE;
    }
    --inHandler;
}

/**
 * Reentrant with BlockHandler and WakeupHandler, except wakeup won't
 * get called until next time
 */
Bool
RegisterBlockAndWakeupHandlers(ServerBlockHandlerProcPtr blockHandler,
                               ServerWakeupHandlerProcPtr wakeupHandler,
                               void *blockData)
{
    BlockHandlerPtr new;

    if (numHandlers >= sizeHandlers) {
        new = (BlockHandlerPtr) realloc(handlers, (numHandlers + 1) *
                                        sizeof(BlockHandlerRec));
        if (!new)
            return FALSE;
        handlers = new;
        sizeHandlers = numHandlers + 1;
    }
    handlers[numHandlers].BlockHandler = blockHandler;
    handlers[numHandlers].WakeupHandler = wakeupHandler;
    handlers[numHandlers].blockData = blockData;
    handlers[numHandlers].deleted = FALSE;
    numHandlers = numHandlers + 1;
    return TRUE;
}

void
RemoveBlockAndWakeupHandlers(ServerBlockHandlerProcPtr blockHandler,
                             ServerWakeupHandlerProcPtr wakeupHandler,
                             void *blockData)
{
    int i;

    for (i = 0; i < numHandlers; i++)
        if (handlers[i].BlockHandler == blockHandler &&
            handlers[i].WakeupHandler == wakeupHandler &&
            handlers[i].blockData == blockData) {
            if (inHandler) {
                handlerDeleted = TRUE;
                handlers[i].deleted = TRUE;
            }
            else {
                for (; i < numHandlers - 1; i++)
                    handlers[i] = handlers[i + 1];
                numHandlers--;
            }
            break;
        }
}

void
InitBlockAndWakeupHandlers(void)
{
    free(handlers);
    handlers = (BlockHandlerPtr) 0;
    numHandlers = 0;
    sizeHandlers = 0;
}

/*
 * A general work queue.  Perform some task before the server
 * sleeps for input.
 */

WorkQueuePtr workQueue;
static WorkQueuePtr *workQueueLast = &workQueue;

void
ClearWorkQueue(void)
{
    WorkQueuePtr q, *p;

    p = &workQueue;
    while ((q = *p)) {
        *p = q->next;
        free(q);
    }
    workQueueLast = p;
}

void
ProcessWorkQueue(void)
{
    WorkQueuePtr q, *p;

    p = &workQueue;
    /*
     * Scan the work queue once, calling each function.  Those
     * which return TRUE are removed from the queue, otherwise
     * they will be called again.  This must be reentrant with
     * QueueWorkProc.
     */
    while ((q = *p)) {
        if ((*q->function) (q->client, q->closure)) {
            /* remove q from the list */
            *p = q->next;       /* don't fetch until after func called */
            free(q);
        }
        else {
            p = &q->next;       /* don't fetch until after func called */
        }
    }
    workQueueLast = p;
}

void
ProcessWorkQueueZombies(void)
{
    WorkQueuePtr q, *p;

    p = &workQueue;
    while ((q = *p)) {
        if (q->client && q->client->clientGone) {
            (void) (*q->function) (q->client, q->closure);
            /* remove q from the list */
            *p = q->next;       /* don't fetch until after func called */
            free(q);
        }
        else {
            p = &q->next;       /* don't fetch until after func called */
        }
    }
    workQueueLast = p;
}

Bool
QueueWorkProc(Bool (*function) (ClientPtr pClient, void *closure),
              ClientPtr client, void *closure)
{
    WorkQueuePtr q;

    q = malloc(sizeof *q);
    if (!q)
        return FALSE;
    q->function = function;
    q->client = client;
    q->closure = closure;
    q->next = NULL;
    *workQueueLast = q;
    workQueueLast = &q->next;
    return TRUE;
}

/*
 * Manage a queue of sleeping clients, awakening them
 * when requested, by using the OS functions IgnoreClient
 * and AttendClient.  Note that this *ignores* the troubles
 * with request data interleaving itself with events, but
 * we'll leave that until a later time.
 */

typedef struct _SleepQueue {
    struct _SleepQueue *next;
    ClientPtr client;
    ClientSleepProcPtr function;
    void *closure;
} SleepQueueRec, *SleepQueuePtr;

static SleepQueuePtr sleepQueue = NULL;

Bool
ClientSleep(ClientPtr client, ClientSleepProcPtr function, void *closure)
{
    SleepQueuePtr q;

    q = malloc(sizeof *q);
    if (!q)
        return FALSE;

    IgnoreClient(client);
    q->next = sleepQueue;
    q->client = client;
    q->function = function;
    q->closure = closure;
    sleepQueue = q;
    return TRUE;
}

Bool
ClientSignal(ClientPtr client)
{
    SleepQueuePtr q;

    for (q = sleepQueue; q; q = q->next)
        if (q->client == client) {
            return QueueWorkProc(q->function, q->client, q->closure);
        }
    return FALSE;
}

int
ClientSignalAll(ClientPtr client, ClientSleepProcPtr function, void *closure)
{
    SleepQueuePtr q;
    int count = 0;

    for (q = sleepQueue; q; q = q->next) {
        if (!(client == CLIENT_SIGNAL_ANY || q->client == client))
            continue;

        if (!(function == CLIENT_SIGNAL_ANY || q->function == function))
            continue;

        if (!(closure == CLIENT_SIGNAL_ANY || q->closure == closure))
            continue;

        count += QueueWorkProc(q->function, q->client, q->closure);
    }

    return count;
}

void
ClientWakeup(ClientPtr client)
{
    SleepQueuePtr q, *prev;

    prev = &sleepQueue;
    while ((q = *prev)) {
        if (q->client == client) {
            *prev = q->next;
            free(q);
            AttendClient(client);
            break;
        }
        prev = &q->next;
    }
}

Bool
ClientIsAsleep(ClientPtr client)
{
    SleepQueuePtr q;

    for (q = sleepQueue; q; q = q->next)
        if (q->client == client)
            return TRUE;
    return FALSE;
}

/*
 *  Generic Callback Manager
 */

/* ===== Private Procedures ===== */

static int numCallbackListsToCleanup = 0;
static CallbackListPtr **listsToCleanup = NULL;

static Bool
_AddCallback(CallbackListPtr *pcbl, CallbackProcPtr callback, void *data)
{
    CallbackPtr cbr;

    cbr = malloc(sizeof(CallbackRec));
    if (!cbr)
        return FALSE;
    cbr->proc = callback;
    cbr->data = data;
    cbr->next = (*pcbl)->list;
    cbr->deleted = FALSE;
    (*pcbl)->list = cbr;
    return TRUE;
}

static Bool
_DeleteCallback(CallbackListPtr *pcbl, CallbackProcPtr callback, void *data)
{
    CallbackListPtr cbl = *pcbl;
    CallbackPtr cbr, pcbr;

    for (pcbr = NULL, cbr = cbl->list; cbr != NULL; pcbr = cbr, cbr = cbr->next) {
        if ((cbr->proc == callback) && (cbr->data == data))
            break;
    }
    if (cbr != NULL) {
        if (cbl->inCallback) {
            ++(cbl->numDeleted);
            cbr->deleted = TRUE;
        }
        else {
            if (pcbr == NULL)
                cbl->list = cbr->next;
            else
                pcbr->next = cbr->next;
            free(cbr);
        }
        return TRUE;
    }
    return FALSE;
}

void
_CallCallbacks(CallbackListPtr *pcbl, void *call_data)
{
    CallbackListPtr cbl = *pcbl;
    CallbackPtr cbr, pcbr;

    ++(cbl->inCallback);
    for (cbr = cbl->list; cbr != NULL; cbr = cbr->next) {
        (*(cbr->proc)) (pcbl, cbr->data, call_data);
    }
    --(cbl->inCallback);

    if (cbl->inCallback)
        return;

    /* Was the entire list marked for deletion? */

    if (cbl->deleted) {
        DeleteCallbackList(pcbl);
        return;
    }

    /* Were some individual callbacks on the list marked for deletion?
     * If so, do the deletions.
     */

    if (cbl->numDeleted) {
        for (pcbr = NULL, cbr = cbl->list; (cbr != NULL) && cbl->numDeleted;) {
            if (cbr->deleted) {
                if (pcbr) {
                    cbr = cbr->next;
                    free(pcbr->next);
                    pcbr->next = cbr;
                }
                else {
                    cbr = cbr->next;
                    free(cbl->list);
                    cbl->list = cbr;
                }
                cbl->numDeleted--;
            }
            else {              /* this one wasn't deleted */

                pcbr = cbr;
                cbr = cbr->next;
            }
        }
    }
}

static void
_DeleteCallbackList(CallbackListPtr *pcbl)
{
    CallbackListPtr cbl = *pcbl;
    CallbackPtr cbr, nextcbr;
    int i;

    if (cbl->inCallback) {
        cbl->deleted = TRUE;
        return;
    }

    for (i = 0; i < numCallbackListsToCleanup; i++) {
        if (listsToCleanup[i] == pcbl) {
            listsToCleanup[i] = NULL;
            break;
        }
    }

    for (cbr = cbl->list; cbr != NULL; cbr = nextcbr) {
        nextcbr = cbr->next;
        free(cbr);
    }
    free(cbl);
    *pcbl = NULL;
}

static Bool
CreateCallbackList(CallbackListPtr *pcbl)
{
    CallbackListPtr cbl;
    int i;

    if (!pcbl)
        return FALSE;
    cbl = malloc(sizeof(CallbackListRec));
    if (!cbl)
        return FALSE;
    cbl->inCallback = 0;
    cbl->deleted = FALSE;
    cbl->numDeleted = 0;
    cbl->list = NULL;
    *pcbl = cbl;

    for (i = 0; i < numCallbackListsToCleanup; i++) {
        if (!listsToCleanup[i]) {
            listsToCleanup[i] = pcbl;
            return TRUE;
        }
    }

    listsToCleanup = (CallbackListPtr **) xnfrealloc(listsToCleanup,
                                                     sizeof(CallbackListPtr *) *
                                                     (numCallbackListsToCleanup
                                                      + 1));
    listsToCleanup[numCallbackListsToCleanup] = pcbl;
    numCallbackListsToCleanup++;
    return TRUE;
}

/* ===== Public Procedures ===== */

Bool
AddCallback(CallbackListPtr *pcbl, CallbackProcPtr callback, void *data)
{
    if (!pcbl)
        return FALSE;
    if (!*pcbl) {               /* list hasn't been created yet; go create it */
        if (!CreateCallbackList(pcbl))
            return FALSE;
    }
    return _AddCallback(pcbl, callback, data);
}

Bool
DeleteCallback(CallbackListPtr *pcbl, CallbackProcPtr callback, void *data)
{
    if (!pcbl || !*pcbl)
        return FALSE;
    return _DeleteCallback(pcbl, callback, data);
}

void
DeleteCallbackList(CallbackListPtr *pcbl)
{
    if (!pcbl || !*pcbl)
        return;
    _DeleteCallbackList(pcbl);
}

void
DeleteCallbackManager(void)
{
    int i;

    for (i = 0; i < numCallbackListsToCleanup; i++) {
        DeleteCallbackList(listsToCleanup[i]);
    }
    free(listsToCleanup);

    numCallbackListsToCleanup = 0;
    listsToCleanup = NULL;
}

void
InitCallbackManager(void)
{
    DeleteCallbackManager();
}

/**
 * Coordinates the global GL context used by modules in the X Server
 * doing rendering with OpenGL.
 *
 * When setting a GL context (glXMakeCurrent() or eglMakeCurrent()),
 * there is an expensive implied glFlush() required by the GLX and EGL
 * APIs, so modules don't want to have to do it on every request.  But
 * the individual modules using GL also don't know about each other,
 * so they have to coordinate who owns the current context.
 *
 * When you're about to do a MakeCurrent, you should set this variable
 * to your context's address, and you can skip MakeCurrent if it's
 * already set to yours.
 *
 * When you're about to do a DestroyContext, you should set this to
 * NULL if it's set to your context.
 *
 * When you're about to do an unbindContext on a DRI driver, you
 * should set this to NULL.  Despite the unbindContext interface
 * sounding like it only unbinds the passed in context, it actually
 * unconditionally clears the dispatch table even if the given
 * context wasn't current.
 */
void *lastGLContext = NULL;
