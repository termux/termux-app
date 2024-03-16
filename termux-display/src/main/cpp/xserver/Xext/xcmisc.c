/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "swaprep.h"
#include <X11/extensions/xcmiscproto.h>
#include "extinit.h"

#include <stdint.h>

static int
ProcXCMiscGetVersion(ClientPtr client)
{
    xXCMiscGetVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = XCMiscMajorVersion,
        .minorVersion = XCMiscMinorVersion
    };

    REQUEST_SIZE_MATCH(xXCMiscGetVersionReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xXCMiscGetVersionReply), &rep);
    return Success;
}

static int
ProcXCMiscGetXIDRange(ClientPtr client)
{
    xXCMiscGetXIDRangeReply rep;
    XID min_id, max_id;

    REQUEST_SIZE_MATCH(xXCMiscGetXIDRangeReq);
    GetXIDRange(client->index, FALSE, &min_id, &max_id);
    rep = (xXCMiscGetXIDRangeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .start_id = min_id,
        .count = max_id - min_id + 1
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.start_id);
        swapl(&rep.count);
    }
    WriteToClient(client, sizeof(xXCMiscGetXIDRangeReply), &rep);
    return Success;
}

static int
ProcXCMiscGetXIDList(ClientPtr client)
{
    REQUEST(xXCMiscGetXIDListReq);
    xXCMiscGetXIDListReply rep;
    XID *pids;
    unsigned int count;

    REQUEST_SIZE_MATCH(xXCMiscGetXIDListReq);

    if (stuff->count > UINT32_MAX / sizeof(XID))
        return BadAlloc;

    pids = xallocarray(stuff->count, sizeof(XID));
    if (!pids) {
        return BadAlloc;
    }
    count = GetXIDList(client, stuff->count, pids);
    rep = (xXCMiscGetXIDListReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = count,
        .count = count
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.count);
    }
    WriteToClient(client, sizeof(xXCMiscGetXIDListReply), &rep);
    if (count) {
        client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
        WriteSwappedDataToClient(client, count * sizeof(XID), pids);
    }
    free(pids);
    return Success;
}

static int
ProcXCMiscDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XCMiscGetVersion:
        return ProcXCMiscGetVersion(client);
    case X_XCMiscGetXIDRange:
        return ProcXCMiscGetXIDRange(client);
    case X_XCMiscGetXIDList:
        return ProcXCMiscGetXIDList(client);
    default:
        return BadRequest;
    }
}

static int _X_COLD
SProcXCMiscGetVersion(ClientPtr client)
{
    REQUEST(xXCMiscGetVersionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXCMiscGetVersionReq);
    swaps(&stuff->majorVersion);
    swaps(&stuff->minorVersion);
    return ProcXCMiscGetVersion(client);
}

static int _X_COLD
SProcXCMiscGetXIDRange(ClientPtr client)
{
    REQUEST(xReq);

    swaps(&stuff->length);
    return ProcXCMiscGetXIDRange(client);
}

static int _X_COLD
SProcXCMiscGetXIDList(ClientPtr client)
{
    REQUEST(xXCMiscGetXIDListReq);
    REQUEST_SIZE_MATCH(xXCMiscGetXIDListReq);

    swaps(&stuff->length);
    swapl(&stuff->count);
    return ProcXCMiscGetXIDList(client);
}

static int _X_COLD
SProcXCMiscDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XCMiscGetVersion:
        return SProcXCMiscGetVersion(client);
    case X_XCMiscGetXIDRange:
        return SProcXCMiscGetXIDRange(client);
    case X_XCMiscGetXIDList:
        return SProcXCMiscGetXIDList(client);
    default:
        return BadRequest;
    }
}

void
XCMiscExtensionInit(void)
{
    AddExtension(XCMiscExtensionName, 0, 0,
                 ProcXCMiscDispatch, SProcXCMiscDispatch,
                 NULL, StandardMinorOpcode);
}
