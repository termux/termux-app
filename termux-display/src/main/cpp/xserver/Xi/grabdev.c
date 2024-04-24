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
 * Extension function to grab an extension device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exglobals.h"
#include "dixevents.h"          /* GrabDevice */

#include "grabdev.h"

extern XExtEventInfo EventInfo[];
extern int ExtEventIndex;

/***********************************************************************
 *
 * Swap the request if the requestor has a different byte order than us.
 *
 */

int _X_COLD
SProcXGrabDevice(ClientPtr client)
{
    REQUEST(xGrabDeviceReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xGrabDeviceReq);
    swapl(&stuff->grabWindow);
    swapl(&stuff->time);
    swaps(&stuff->event_count);

    if (stuff->length !=
        bytes_to_int32(sizeof(xGrabDeviceReq)) + stuff->event_count)
        return BadLength;

    SwapLongs((CARD32 *) (&stuff[1]), stuff->event_count);

    return (ProcXGrabDevice(client));
}

/***********************************************************************
 *
 * Grab an extension device.
 *
 */

int
ProcXGrabDevice(ClientPtr client)
{
    int rc;
    xGrabDeviceReply rep;
    DeviceIntPtr dev;
    GrabMask mask;
    struct tmask tmp[EMASKSIZE];

    REQUEST(xGrabDeviceReq);
    REQUEST_AT_LEAST_SIZE(xGrabDeviceReq);

    if (stuff->length !=
        bytes_to_int32(sizeof(xGrabDeviceReq)) + stuff->event_count)
        return BadLength;

    rep = (xGrabDeviceReply) {
        .repType = X_Reply,
        .RepType = X_GrabDevice,
        .sequenceNumber = client->sequence,
        .length = 0,
    };

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixGrabAccess);
    if (rc != Success)
        return rc;

    if ((rc = CreateMaskFromList(client, (XEventClass *) &stuff[1],
                                 stuff->event_count, tmp, dev,
                                 X_GrabDevice)) != Success)
        return rc;

    mask.xi = tmp[stuff->deviceid].mask;

    rc = GrabDevice(client, dev, stuff->other_devices_mode,
                    stuff->this_device_mode, stuff->grabWindow,
                    stuff->ownerEvents, stuff->time,
                    &mask, XI, None, None, &rep.status);

    if (rc != Success)
        return rc;

    WriteReplyToClient(client, sizeof(xGrabDeviceReply), &rep);
    return Success;
}

/***********************************************************************
 *
 * This procedure creates an event mask from a list of XEventClasses.
 *
 * Procedure is as follows:
 * An XEventClass is (deviceid << 8 | eventtype). For each entry in the list,
 * get the device. Then run through all available event indices (those are
 * set when XI starts up) and binary OR's the device's mask to whatever the
 * event mask for the given event type was.
 * If an error occurs, it is sent to the client. Errors are generated if
 *  - if the device given in the event class is invalid
 *  - if the device in the class list is not the device given as parameter (no
 *  error if parameter is NULL)
 *
 * mask has to be size EMASKSIZE and pre-allocated.
 *
 * @param client The client to send the error to (if one occurs)
 * @param list List of event classes as sent from the client.
 * @param count Number of elements in list.
 * @param mask Preallocated mask (size EMASKSIZE).
 * @param dev The device we're creating masks for.
 * @param req The request we're processing. Used to fill in error fields.
 */

int
CreateMaskFromList(ClientPtr client, XEventClass * list, int count,
                   struct tmask *mask, DeviceIntPtr dev, int req)
{
    int rc, i, j;
    int device;
    DeviceIntPtr tdev;

    memset(mask, 0, EMASKSIZE * sizeof(struct tmask));

    for (i = 0; i < count; i++, list++) {
        device = *list >> 8;
        if (device > 255)
            return BadClass;

        rc = dixLookupDevice(&tdev, device, client, DixUseAccess);
        if (rc != BadDevice && rc != Success)
            return rc;
        if (rc == BadDevice || (dev != NULL && tdev != dev))
            return BadClass;

        for (j = 0; j < ExtEventIndex; j++)
            if (EventInfo[j].type == (*list & 0xff)) {
                mask[device].mask |= EventInfo[j].mask;
                mask[device].dev = (void *) tdev;
                break;
            }
    }
    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XGrabDevice function,
 * if the client and server have a different byte ordering.
 *
 */

void _X_COLD
SRepXGrabDevice(ClientPtr client, int size, xGrabDeviceReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
}
