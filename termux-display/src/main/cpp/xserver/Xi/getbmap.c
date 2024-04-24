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

/***********************************************************************
 *
 * Extension function to return the version of the extension.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exglobals.h"

#include "getbmap.h"

/***********************************************************************
 *
 * This procedure gets the button mapping for the specified device.
 *
 */

int _X_COLD
SProcXGetDeviceButtonMapping(ClientPtr client)
{
    REQUEST(xGetDeviceButtonMappingReq);
    swaps(&stuff->length);
    return (ProcXGetDeviceButtonMapping(client));
}

/***********************************************************************
 *
 * This procedure gets the button mapping for the specified device.
 *
 */

int
ProcXGetDeviceButtonMapping(ClientPtr client)
{
    DeviceIntPtr dev;
    xGetDeviceButtonMappingReply rep;
    ButtonClassPtr b;
    int rc;

    REQUEST(xGetDeviceButtonMappingReq);
    REQUEST_SIZE_MATCH(xGetDeviceButtonMappingReq);

    rep = (xGetDeviceButtonMappingReply) {
        .repType = X_Reply,
        .RepType = X_GetDeviceButtonMapping,
        .sequenceNumber = client->sequence,
        .nElts = 0,
        .length = 0
    };

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    b = dev->button;
    if (b == NULL)
        return BadMatch;

    rep.nElts = b->numButtons;
    rep.length = bytes_to_int32(rep.nElts);
    WriteReplyToClient(client, sizeof(xGetDeviceButtonMappingReply), &rep);
    WriteToClient(client, rep.nElts, &b->map[1]);
    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XGetDeviceButtonMapping function,
 * if the client and server have a different byte ordering.
 *
 */

void _X_COLD
SRepXGetDeviceButtonMapping(ClientPtr client, int size,
                            xGetDeviceButtonMappingReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
}
