/*
 * Minimal implementation of PanoramiX/Xinerama
 *
 * This is used in rootless mode where the underlying window server
 * already provides an abstracted view of multiple screens as one
 * large screen area.
 *
 * This code is largely based on panoramiX.c, which contains the
 * following copyright notice:
 */
/*****************************************************************
   Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software.

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
   BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Digital Equipment Corporation
   shall not be used in advertising or otherwise to promote the sale, use or other
   dealings in this Software without prior written authorization from Digital
   Equipment Corporation.
 ******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "pseudoramiX.h"
#include "extnsionst.h"
#include "nonsdk_extinit.h"
#include "dixstruct.h"
#include "window.h"
#include <X11/extensions/panoramiXproto.h>
#include "globals.h"

#define TRACE PseudoramiXTrace("TRACE " __FILE__ ":%s",__FUNCTION__)
#define DEBUG_LOG PseudoramiXDebug

Bool noPseudoramiXExtension = FALSE;
extern Bool noRRXineramaExtension;

extern int
ProcPanoramiXQueryVersion(ClientPtr client);

static void
PseudoramiXResetProc(ExtensionEntry *extEntry);

static int
ProcPseudoramiXQueryVersion(ClientPtr client);
static int
ProcPseudoramiXGetState(ClientPtr client);
static int
ProcPseudoramiXGetScreenCount(ClientPtr client);
static int
ProcPseudoramiXGetScreenSize(ClientPtr client);
static int
ProcPseudoramiXIsActive(ClientPtr client);
static int
ProcPseudoramiXQueryScreens(ClientPtr client);
static int
ProcPseudoramiXDispatch(ClientPtr client);

static int
SProcPseudoramiXQueryVersion(ClientPtr client);
static int
SProcPseudoramiXGetState(ClientPtr client);
static int
SProcPseudoramiXGetScreenCount(ClientPtr client);
static int
SProcPseudoramiXGetScreenSize(ClientPtr client);
static int
SProcPseudoramiXIsActive(ClientPtr client);
static int
SProcPseudoramiXQueryScreens(ClientPtr client);
static int
SProcPseudoramiXDispatch(ClientPtr client);

typedef struct {
    int x;
    int y;
    int w;
    int h;
} PseudoramiXScreenRec;

static PseudoramiXScreenRec *pseudoramiXScreens = NULL;
static int pseudoramiXScreensAllocated = 0;
static int pseudoramiXNumScreens = 0;
static unsigned long pseudoramiXGeneration = 0;

static void
PseudoramiXTrace(const char *format, ...)
    _X_ATTRIBUTE_PRINTF(1, 2);

static void
PseudoramiXTrace(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(X_NONE, 10, format, ap);
    va_end(ap);
}

static void
PseudoramiXDebug(const char *format, ...)
    _X_ATTRIBUTE_PRINTF(1, 2);

static void
PseudoramiXDebug(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(X_NONE, 3, format, ap);
    va_end(ap);
}

// Add a PseudoramiX screen.
// The rest of the X server will know nothing about this screen.
// Can be called before or after extension init.
// Screens must be re-added once per generation.
void
PseudoramiXAddScreen(int x, int y, int w, int h)
{
    PseudoramiXScreenRec *s;

    if (noPseudoramiXExtension) return;

    if (pseudoramiXNumScreens == pseudoramiXScreensAllocated) {
        pseudoramiXScreensAllocated += pseudoramiXScreensAllocated + 1;
        pseudoramiXScreens = reallocarray(pseudoramiXScreens,
                                          pseudoramiXScreensAllocated,
                                          sizeof(PseudoramiXScreenRec));
    }

    DEBUG_LOG("x: %d, y: %d, w: %d, h: %d\n", x, y, w, h);

    s = &pseudoramiXScreens[pseudoramiXNumScreens++];
    s->x = x;
    s->y = y;
    s->w = w;
    s->h = h;
}

// Initialize PseudoramiX.
// Copied from PanoramiXExtensionInit
void
PseudoramiXExtensionInit(void)
{
    Bool success = FALSE;
    ExtensionEntry      *extEntry;

    if (noPseudoramiXExtension) return;

    TRACE;

    /* Even with only one screen we need to enable PseudoramiX to allow
       dynamic screen configuration changes. */
#if 0
    if (pseudoramiXNumScreens == 1) {
        // Only one screen - disable Xinerama extension.
        noPseudoramiXExtension = TRUE;
        return;
    }
#endif

    if (pseudoramiXGeneration != serverGeneration) {
        extEntry = AddExtension(PANORAMIX_PROTOCOL_NAME, 0, 0,
                                ProcPseudoramiXDispatch,
                                SProcPseudoramiXDispatch,
                                PseudoramiXResetProc,
                                StandardMinorOpcode);
        if (!extEntry) {
            ErrorF("PseudoramiXExtensionInit(): AddExtension failed\n");
        }
        else {
            pseudoramiXGeneration = serverGeneration;
            success = TRUE;
        }
    }

    /* Do not allow RRXinerama to initialize if we did */
    noRRXineramaExtension = success;

    if (!success) {
        ErrorF("%s Extension (PseudoramiX) failed to initialize\n",
               PANORAMIX_PROTOCOL_NAME);
        return;
    }
}

void
PseudoramiXResetScreens(void)
{
    TRACE;

    pseudoramiXNumScreens = 0;
}

static void
PseudoramiXResetProc(ExtensionEntry *extEntry)
{
    TRACE;

    PseudoramiXResetScreens();
}

// was PanoramiX
static int
ProcPseudoramiXQueryVersion(ClientPtr client)
{
    TRACE;

    return ProcPanoramiXQueryVersion(client);
}

// was PanoramiX
static int
ProcPseudoramiXGetState(ClientPtr client)
{
    REQUEST(xPanoramiXGetStateReq);
    WindowPtr pWin;
    xPanoramiXGetStateReply rep;
    register int rc;

    TRACE;

    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.state = !noPseudoramiXExtension;
    rep.window = stuff->window;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.window);
    }
    WriteToClient(client, sizeof(xPanoramiXGetStateReply),&rep);
    return Success;
}

// was PanoramiX
static int
ProcPseudoramiXGetScreenCount(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenCountReq);
    WindowPtr pWin;
    xPanoramiXGetScreenCountReply rep;
    register int rc;

    TRACE;

    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.ScreenCount = pseudoramiXNumScreens;
    rep.window = stuff->window;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.window);
    }
    WriteToClient(client, sizeof(xPanoramiXGetScreenCountReply),&rep);
    return Success;
}

// was PanoramiX
static int
ProcPseudoramiXGetScreenSize(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenSizeReq);
    WindowPtr pWin;
    xPanoramiXGetScreenSizeReply rep;
    register int rc;

    TRACE;

    REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);

    if (stuff->screen >= pseudoramiXNumScreens)
      return BadMatch;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    /* screen dimensions */
    rep.width = pseudoramiXScreens[stuff->screen].w;
    // was screenInfo.screens[stuff->screen]->width;
    rep.height = pseudoramiXScreens[stuff->screen].h;
    // was screenInfo.screens[stuff->screen]->height;
    rep.window = stuff->window;
    rep.screen = stuff->screen;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.width);
        swapl(&rep.height);
        swapl(&rep.window);
        swapl(&rep.screen);
    }
    WriteToClient(client, sizeof(xPanoramiXGetScreenSizeReply),&rep);
    return Success;
}

// was Xinerama
static int
ProcPseudoramiXIsActive(ClientPtr client)
{
    /* REQUEST(xXineramaIsActiveReq); */
    xXineramaIsActiveReply rep;

    TRACE;

    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.state = !noPseudoramiXExtension;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.state);
    }
    WriteToClient(client, sizeof(xXineramaIsActiveReply),&rep);
    return Success;
}

// was Xinerama
static int
ProcPseudoramiXQueryScreens(ClientPtr client)
{
    /* REQUEST(xXineramaQueryScreensReq); */
    xXineramaQueryScreensReply rep;

    DEBUG_LOG("noPseudoramiXExtension=%d, pseudoramiXNumScreens=%d\n",
              noPseudoramiXExtension,
              pseudoramiXNumScreens);

    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.number = noPseudoramiXExtension ? 0 : pseudoramiXNumScreens;
    rep.length = bytes_to_int32(rep.number * sz_XineramaScreenInfo);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.number);
    }
    WriteToClient(client, sizeof(xXineramaQueryScreensReply),&rep);

    if (!noPseudoramiXExtension) {
        xXineramaScreenInfo scratch;
        int i;

        for (i = 0; i < pseudoramiXNumScreens; i++) {
            scratch.x_org = pseudoramiXScreens[i].x;
            scratch.y_org = pseudoramiXScreens[i].y;
            scratch.width = pseudoramiXScreens[i].w;
            scratch.height = pseudoramiXScreens[i].h;

            if (client->swapped) {
                swaps(&scratch.x_org);
                swaps(&scratch.y_org);
                swaps(&scratch.width);
                swaps(&scratch.height);
            }
            WriteToClient(client, sz_XineramaScreenInfo,&scratch);
        }
    }

    return Success;
}

// was PanoramiX
static int
ProcPseudoramiXDispatch(ClientPtr client)
{
    REQUEST(xReq);
    TRACE;
    switch (stuff->data) {
    case X_PanoramiXQueryVersion:
        return ProcPseudoramiXQueryVersion(client);

    case X_PanoramiXGetState:
        return ProcPseudoramiXGetState(client);

    case X_PanoramiXGetScreenCount:
        return ProcPseudoramiXGetScreenCount(client);

    case X_PanoramiXGetScreenSize:
        return ProcPseudoramiXGetScreenSize(client);

    case X_XineramaIsActive:
        return ProcPseudoramiXIsActive(client);

    case X_XineramaQueryScreens:
        return ProcPseudoramiXQueryScreens(client);
    }
    return BadRequest;
}

static int
SProcPseudoramiXQueryVersion(ClientPtr client)
{
    REQUEST(xPanoramiXQueryVersionReq);

    TRACE;

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xPanoramiXQueryVersionReq);
    return ProcPseudoramiXQueryVersion(client);
}

static int
SProcPseudoramiXGetState(ClientPtr client)
{
    REQUEST(xPanoramiXGetStateReq);

    TRACE;

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
    return ProcPseudoramiXGetState(client);
}

static int
SProcPseudoramiXGetScreenCount(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenCountReq);

    TRACE;

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
    return ProcPseudoramiXGetScreenCount(client);
}

static int
SProcPseudoramiXGetScreenSize(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenSizeReq);

    TRACE;

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
    return ProcPseudoramiXGetScreenSize(client);
}

static int
SProcPseudoramiXIsActive(ClientPtr client)
{
    REQUEST(xXineramaIsActiveReq);

    TRACE;

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);
    return ProcPseudoramiXIsActive(client);
}

static int
SProcPseudoramiXQueryScreens(ClientPtr client)
{
    REQUEST(xXineramaQueryScreensReq);

    TRACE;

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);
    return ProcPseudoramiXQueryScreens(client);
}

static int
SProcPseudoramiXDispatch(ClientPtr client)
{
    REQUEST(xReq);

    TRACE;

    switch (stuff->data) {
    case X_PanoramiXQueryVersion:
        return SProcPseudoramiXQueryVersion(client);

    case X_PanoramiXGetState:
        return SProcPseudoramiXGetState(client);

    case X_PanoramiXGetScreenCount:
        return SProcPseudoramiXGetScreenCount(client);

    case X_PanoramiXGetScreenSize:
        return SProcPseudoramiXGetScreenSize(client);

    case X_XineramaIsActive:
        return SProcPseudoramiXIsActive(client);

    case X_XineramaQueryScreens:
        return SProcPseudoramiXQueryScreens(client);
    }
    return BadRequest;
}
