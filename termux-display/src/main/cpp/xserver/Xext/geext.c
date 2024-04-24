/*
 * Copyright 2007-2008 Peter Hutterer
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
 * Author: Peter Hutterer, University of South Australia, NICTA
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include "windowstr.h"
#include <X11/extensions/ge.h>

#include "geint.h"
#include "geext.h"
#include "protocol-versions.h"
#include "extinit.h"

DevPrivateKeyRec GEClientPrivateKeyRec;

GEExtension GEExtensions[MAXEXTENSIONS];

/* Major available requests */
static const int version_requests[] = {
    X_GEQueryVersion,           /* before client sends QueryVersion */
    X_GEQueryVersion,           /* must be set to last request in version 1 */
};

/* Forward declarations */
static void SGEGenericEvent(xEvent *from, xEvent *to);

#define EXT_MASK(ext) ((ext) & 0x7F)

/************************************************************/
/*                request handlers                          */
/************************************************************/

static int
ProcGEQueryVersion(ClientPtr client)
{
    GEClientInfoPtr pGEClient = GEGetClient(client);
    xGEQueryVersionReply rep;

    REQUEST(xGEQueryVersionReq);

    REQUEST_SIZE_MATCH(xGEQueryVersionReq);

    rep = (xGEQueryVersionReply) {
        .repType = X_Reply,
        .RepType = X_GEQueryVersion,
        .sequenceNumber = client->sequence,
        .length = 0,

        /* return the supported version by the server */
        .majorVersion = SERVER_GE_MAJOR_VERSION,
        .minorVersion = SERVER_GE_MINOR_VERSION
    };

    /* Remember version the client requested */
    pGEClient->major_version = stuff->majorVersion;
    pGEClient->minor_version = stuff->minorVersion;

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
    }

    WriteToClient(client, sizeof(xGEQueryVersionReply), &rep);
    return Success;
}

static int (*ProcGEVector[GENumberRequests]) (ClientPtr) = {
    /* Version 1.0 */
    ProcGEQueryVersion,
};

/************************************************************/
/*                swapped request handlers                  */
/************************************************************/
static int _X_COLD
SProcGEQueryVersion(ClientPtr client)
{
    REQUEST(xGEQueryVersionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGEQueryVersionReq);
    swaps(&stuff->majorVersion);
    swaps(&stuff->minorVersion);
    return (*ProcGEVector[stuff->ReqType]) (client);
}

static int (*SProcGEVector[GENumberRequests]) (ClientPtr) = {
    /* Version 1.0 */
    SProcGEQueryVersion
};

/************************************************************/
/*                callbacks                                 */
/************************************************************/

/* dispatch requests */
static int
ProcGEDispatch(ClientPtr client)
{
    GEClientInfoPtr pGEClient = GEGetClient(client);

    REQUEST(xGEReq);

    if (pGEClient->major_version >= ARRAY_SIZE(version_requests))
        return BadRequest;
    if (stuff->ReqType > version_requests[pGEClient->major_version])
        return BadRequest;

    return (ProcGEVector[stuff->ReqType]) (client);
}

/* dispatch swapped requests */
static int _X_COLD
SProcGEDispatch(ClientPtr client)
{
    GEClientInfoPtr pGEClient = GEGetClient(client);

    REQUEST(xGEReq);

    if (pGEClient->major_version >= ARRAY_SIZE(version_requests))
        return BadRequest;
    if (stuff->ReqType > version_requests[pGEClient->major_version])
        return BadRequest;

    return (*SProcGEVector[stuff->ReqType]) (client);
}

/* Reset extension. Called on server shutdown. */
static void
GEResetProc(ExtensionEntry * extEntry)
{
    EventSwapVector[GenericEvent] = NotImplemented;
}

/*  Calls the registered event swap function for the extension.
 *
 *  Each extension can register a swap function to handle GenericEvents being
 *  swapped properly. The server calls SGEGenericEvent() before the event is
 *  written on the wire, this one calls the registered swap function to do the
 *  work.
 */
static void _X_COLD
SGEGenericEvent(xEvent *from, xEvent *to)
{
    xGenericEvent *gefrom = (xGenericEvent *) from;
    xGenericEvent *geto = (xGenericEvent *) to;

    if ((gefrom->extension & 0x7f) > MAXEXTENSIONS) {
        ErrorF("GE: Invalid extension offset for event.\n");
        return;
    }

    if (GEExtensions[EXT_MASK(gefrom->extension)].evswap)
        GEExtensions[EXT_MASK(gefrom->extension)].evswap(gefrom, geto);
}

/* Init extension, register at server.
 * Since other extensions may rely on XGE (XInput does already), it is a good
 * idea to init XGE first, before any other extension.
 */
void
GEExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (!dixRegisterPrivateKey
        (&GEClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(GEClientInfoRec)))
        FatalError("GEExtensionInit: GE private request failed.\n");

    if ((extEntry = AddExtension(GE_NAME,
                                 0, GENumberErrors,
                                 ProcGEDispatch, SProcGEDispatch,
                                 GEResetProc, StandardMinorOpcode)) != 0) {
        memset(GEExtensions, 0, sizeof(GEExtensions));

        EventSwapVector[GenericEvent] = (EventSwapPtr) SGEGenericEvent;
    }
    else {
        FatalError("GEInit: AddExtensions failed.\n");
    }

}

/************************************************************/
/*                interface for extensions                  */
/************************************************************/

/* Register an extension with GE. The given swap function will be called each
 * time an event is sent to a client with different byte order.
 * @param extension The extensions major opcode
 * @param ev_swap The event swap function.
 * @param ev_fill Called for an event before delivery. The extension now has
 * the chance to fill in necessary fields for the event.
 */
void
GERegisterExtension(int extension,
                    void (*ev_swap) (xGenericEvent *from, xGenericEvent *to))
{
    if (EXT_MASK(extension) >= MAXEXTENSIONS)
        FatalError("GE: extension > MAXEXTENSIONS. This should not happen.\n");

    /* extension opcodes are > 128, might as well save some space here */
    GEExtensions[EXT_MASK(extension)].evswap = ev_swap;
}

/* Sets type and extension field for a generic event. This is just an
 * auxiliary function, extensions could do it manually too.
 */
void
GEInitEvent(xGenericEvent *ev, int extension)
{
    ev->type = GenericEvent;
    ev->extension = extension;
    ev->length = 0;
}
