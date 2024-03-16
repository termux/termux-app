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
 * Function to allow frozen events to be routed from extension input devices.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "exglobals.h"

#include "allowev.h"
#include "dixevents.h"

/***********************************************************************
 *
 * This procedure allows frozen events to be routed.
 *
 */

int _X_COLD
SProcXAllowDeviceEvents(ClientPtr client)
{
    REQUEST(xAllowDeviceEventsReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xAllowDeviceEventsReq);
    swapl(&stuff->time);
    return (ProcXAllowDeviceEvents(client));
}

/***********************************************************************
 *
 * This procedure allows frozen events to be routed.
 *
 */

int
ProcXAllowDeviceEvents(ClientPtr client)
{
    TimeStamp time;
    DeviceIntPtr thisdev;
    int rc;

    REQUEST(xAllowDeviceEventsReq);
    REQUEST_SIZE_MATCH(xAllowDeviceEventsReq);

    rc = dixLookupDevice(&thisdev, stuff->deviceid, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    time = ClientTimeToServerTime(stuff->time);

    switch (stuff->mode) {
    case ReplayThisDevice:
        AllowSome(client, time, thisdev, NOT_GRABBED);
        break;
    case SyncThisDevice:
        AllowSome(client, time, thisdev, FREEZE_NEXT_EVENT);
        break;
    case AsyncThisDevice:
        AllowSome(client, time, thisdev, THAWED);
        break;
    case AsyncOtherDevices:
        AllowSome(client, time, thisdev, THAW_OTHERS);
        break;
    case SyncAll:
        AllowSome(client, time, thisdev, FREEZE_BOTH_NEXT_EVENT);
        break;
    case AsyncAll:
        AllowSome(client, time, thisdev, THAWED_BOTH);
        break;
    default:
        client->errorValue = stuff->mode;
        return BadValue;
    }
    return Success;
}
