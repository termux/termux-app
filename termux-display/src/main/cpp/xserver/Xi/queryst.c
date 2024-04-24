/*

Copyright 1998, 1998  The Open Group

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

/***********************************************************************
 *
 * Request to query the state of an extension input device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exevents.h"
#include "exglobals.h"
#include "xkbsrv.h"
#include "xkbstr.h"

#include "queryst.h"

/***********************************************************************
 *
 * This procedure allows a client to query the state of a device.
 *
 */

int _X_COLD
SProcXQueryDeviceState(ClientPtr client)
{
    REQUEST(xQueryDeviceStateReq);
    swaps(&stuff->length);
    return (ProcXQueryDeviceState(client));
}

/***********************************************************************
 *
 * This procedure allows frozen events to be routed.
 *
 */

int
ProcXQueryDeviceState(ClientPtr client)
{
    int rc, i;
    int num_classes = 0;
    int total_length = 0;
    char *buf, *savbuf;
    KeyClassPtr k;
    xKeyState *tk;
    ButtonClassPtr b;
    xButtonState *tb;
    ValuatorClassPtr v;
    xValuatorState *tv;
    xQueryDeviceStateReply rep;
    DeviceIntPtr dev;
    double *values;

    REQUEST(xQueryDeviceStateReq);
    REQUEST_SIZE_MATCH(xQueryDeviceStateReq);

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixReadAccess);
    if (rc != Success && rc != BadAccess)
        return rc;

    v = dev->valuator;
    if (v != NULL && v->motionHintWindow != NULL)
        MaybeStopDeviceHint(dev, client);

    k = dev->key;
    if (k != NULL) {
        total_length += sizeof(xKeyState);
        num_classes++;
    }

    b = dev->button;
    if (b != NULL) {
        total_length += sizeof(xButtonState);
        num_classes++;
    }

    if (v != NULL) {
        total_length += (sizeof(xValuatorState) + (v->numAxes * sizeof(int)));
        num_classes++;
    }
    buf = (char *) calloc(total_length, 1);
    if (!buf)
        return BadAlloc;
    savbuf = buf;

    if (k != NULL) {
        tk = (xKeyState *) buf;
        tk->class = KeyClass;
        tk->length = sizeof(xKeyState);
        tk->num_keys = k->xkbInfo->desc->max_key_code -
            k->xkbInfo->desc->min_key_code + 1;
        if (rc != BadAccess)
            for (i = 0; i < 32; i++)
                tk->keys[i] = k->down[i];
        buf += sizeof(xKeyState);
    }

    if (b != NULL) {
        tb = (xButtonState *) buf;
        tb->class = ButtonClass;
        tb->length = sizeof(xButtonState);
        tb->num_buttons = b->numButtons;
        if (rc != BadAccess)
            memcpy(tb->buttons, b->down, sizeof(b->down));
        buf += sizeof(xButtonState);
    }

    if (v != NULL) {
        tv = (xValuatorState *) buf;
        tv->class = ValuatorClass;
        tv->length = sizeof(xValuatorState) + v->numAxes * 4;
        tv->num_valuators = v->numAxes;
        tv->mode = valuator_get_mode(dev, 0);
        tv->mode |= (dev->proximity &&
                     !dev->proximity->in_proximity) ? OutOfProximity : 0;
        buf += sizeof(xValuatorState);
        for (i = 0, values = v->axisVal; i < v->numAxes; i++) {
            if (rc != BadAccess)
                *((int *) buf) = *values;
            values++;
            if (client->swapped) {
                swapl((int *) buf);
            }
            buf += sizeof(int);
        }
    }

    rep = (xQueryDeviceStateReply) {
        .repType = X_Reply,
        .RepType = X_QueryDeviceState,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(total_length),
        .num_classes = num_classes
    };
    WriteReplyToClient(client, sizeof(xQueryDeviceStateReply), &rep);
    if (total_length > 0)
        WriteToClient(client, total_length, savbuf);
    free(savbuf);
    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XQueryDeviceState function,
 * if the client and server have a different byte ordering.
 *
 */

void _X_COLD
SRepXQueryDeviceState(ClientPtr client, int size, xQueryDeviceStateReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
}
