/************************************************************

Copyright 1989, 1998  The Open Group

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

Copyright 1989 by Hewlett-Packard Company, Palo Alto, California.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Hewlett-Packard not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/********************************************************************
 *
 *  Get the key mapping for an extension device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exglobals.h"
#include "swaprep.h"
#include "xkbsrv.h"
#include "xkbstr.h"

#include "getkmap.h"

/***********************************************************************
 *
 * This procedure gets the key mapping for an extension device,
 * for clients on machines with a different byte ordering than the server.
 *
 */

int _X_COLD
SProcXGetDeviceKeyMapping(ClientPtr client)
{
    REQUEST(xGetDeviceKeyMappingReq);
    swaps(&stuff->length);
    return (ProcXGetDeviceKeyMapping(client));
}

/***********************************************************************
 *
 * Get the device key mapping.
 *
 */

int
ProcXGetDeviceKeyMapping(ClientPtr client)
{
    xGetDeviceKeyMappingReply rep;
    DeviceIntPtr dev;
    XkbDescPtr xkb;
    KeySymsPtr syms;
    int rc;

    REQUEST(xGetDeviceKeyMappingReq);
    REQUEST_SIZE_MATCH(xGetDeviceKeyMappingReq);

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    if (dev->key == NULL)
        return BadMatch;
    xkb = dev->key->xkbInfo->desc;

    if (stuff->firstKeyCode < xkb->min_key_code ||
        stuff->firstKeyCode > xkb->max_key_code) {
        client->errorValue = stuff->firstKeyCode;
        return BadValue;
    }

    if (stuff->firstKeyCode + stuff->count > xkb->max_key_code + 1) {
        client->errorValue = stuff->count;
        return BadValue;
    }

    syms = XkbGetCoreMap(dev);
    if (!syms)
        return BadAlloc;

    rep = (xGetDeviceKeyMappingReply) {
        .repType = X_Reply,
        .RepType = X_GetDeviceKeyMapping,
        .sequenceNumber = client->sequence,
        .keySymsPerKeyCode = syms->mapWidth,
        .length = (syms->mapWidth * stuff->count) /* KeySyms are 4 bytes */
    };
    WriteReplyToClient(client, sizeof(xGetDeviceKeyMappingReply), &rep);

    client->pSwapReplyFunc = (ReplySwapPtr) CopySwap32Write;
    WriteSwappedDataToClient(client,
                             syms->mapWidth * stuff->count * sizeof(KeySym),
                             &syms->map[syms->mapWidth * (stuff->firstKeyCode -
                                                          syms->minKeyCode)]);
    free(syms->map);
    free(syms);

    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XGetDeviceKeyMapping function,
 * if the client and server have a different byte ordering.
 *
 */

void _X_COLD
SRepXGetDeviceKeyMapping(ClientPtr client, int size,
                         xGetDeviceKeyMappingReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
}
