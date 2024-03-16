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
 *  Change key mapping for an extension device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exevents.h"
#include "exglobals.h"

#include "chgkmap.h"

/***********************************************************************
 *
 * This procedure swaps the request when the client and
 * server have different byte orderings.
 *
 */

int _X_COLD
SProcXChangeDeviceKeyMapping(ClientPtr client)
{
    unsigned int count;

    REQUEST(xChangeDeviceKeyMappingReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xChangeDeviceKeyMappingReq);
    count = stuff->keyCodes * stuff->keySymsPerKeyCode;
    REQUEST_FIXED_SIZE(xChangeDeviceKeyMappingReq, count * sizeof(CARD32));
    SwapLongs((CARD32 *) (&stuff[1]), count);
    return (ProcXChangeDeviceKeyMapping(client));
}

/***********************************************************************
 *
 * Change the device key mapping.
 *
 */

int
ProcXChangeDeviceKeyMapping(ClientPtr client)
{
    int ret;
    unsigned len;
    DeviceIntPtr dev;
    unsigned int count;

    REQUEST(xChangeDeviceKeyMappingReq);
    REQUEST_AT_LEAST_SIZE(xChangeDeviceKeyMappingReq);

    count = stuff->keyCodes * stuff->keySymsPerKeyCode;
    REQUEST_FIXED_SIZE(xChangeDeviceKeyMappingReq, count * sizeof(CARD32));

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixManageAccess);
    if (ret != Success)
        return ret;
    len = stuff->length - bytes_to_int32(sizeof(xChangeDeviceKeyMappingReq));

    ret = ChangeKeyMapping(client, dev, len, DeviceMappingNotify,
                           stuff->firstKeyCode, stuff->keyCodes,
                           stuff->keySymsPerKeyCode, (KeySym *) &stuff[1]);

    return ret;
}
