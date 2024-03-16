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
 *  Get the modifier mapping for an extension device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>     /* Request macro     */
#include "exglobals.h"

#include "getmmap.h"

/***********************************************************************
 *
 * This procedure gets the modifier mapping for an extension device,
 * for clients on machines with a different byte ordering than the server.
 *
 */

int _X_COLD
SProcXGetDeviceModifierMapping(ClientPtr client)
{
    REQUEST(xGetDeviceModifierMappingReq);
    swaps(&stuff->length);
    return (ProcXGetDeviceModifierMapping(client));
}

/***********************************************************************
 *
 * Get the device Modifier mapping.
 *
 */

int
ProcXGetDeviceModifierMapping(ClientPtr client)
{
    DeviceIntPtr dev;
    xGetDeviceModifierMappingReply rep;
    KeyCode *modkeymap = NULL;
    int ret, max_keys_per_mod;

    REQUEST(xGetDeviceModifierMappingReq);
    REQUEST_SIZE_MATCH(xGetDeviceModifierMappingReq);

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
    if (ret != Success)
        return ret;

    ret = generate_modkeymap(client, dev, &modkeymap, &max_keys_per_mod);
    if (ret != Success)
        return ret;

    rep = (xGetDeviceModifierMappingReply) {
        .repType = X_Reply,
        .RepType = X_GetDeviceModifierMapping,
        .sequenceNumber = client->sequence,
        .numKeyPerModifier = max_keys_per_mod,
    /* length counts 4 byte quantities - there are 8 modifiers 1 byte big */
        .length = max_keys_per_mod << 1
    };

    WriteReplyToClient(client, sizeof(xGetDeviceModifierMappingReply), &rep);
    WriteToClient(client, max_keys_per_mod * 8, modkeymap);

    free(modkeymap);

    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XGetDeviceModifierMapping function,
 * if the client and server have a different byte ordering.
 *
 */

void _X_COLD
SRepXGetDeviceModifierMapping(ClientPtr client, int size,
                              xGetDeviceModifierMappingReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
}
