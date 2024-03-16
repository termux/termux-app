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
 * Extension function to change the keyboard device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exglobals.h"

#include "devbell.h"

/***********************************************************************
 *
 * This procedure is invoked to swap the request bytes if the server and
 * client have a different byte order.
 *
 */

int _X_COLD
SProcXDeviceBell(ClientPtr client)
{
    REQUEST(xDeviceBellReq);
    swaps(&stuff->length);
    return (ProcXDeviceBell(client));
}

/***********************************************************************
 *
 * This procedure rings a bell on an extension device.
 *
 */

int
ProcXDeviceBell(ClientPtr client)
{
    DeviceIntPtr dev;
    KbdFeedbackPtr k;
    BellFeedbackPtr b;
    int rc, base;
    int newpercent;
    CARD8 class;
    void *ctrl;
    BellProcPtr proc;

    REQUEST(xDeviceBellReq);
    REQUEST_SIZE_MATCH(xDeviceBellReq);

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixBellAccess);
    if (rc != Success) {
        client->errorValue = stuff->deviceid;
        return rc;
    }

    if (stuff->percent < -100 || stuff->percent > 100) {
        client->errorValue = stuff->percent;
        return BadValue;
    }
    if (stuff->feedbackclass == KbdFeedbackClass) {
        for (k = dev->kbdfeed; k; k = k->next)
            if (k->ctrl.id == stuff->feedbackid)
                break;
        if (!k) {
            client->errorValue = stuff->feedbackid;
            return BadValue;
        }
        base = k->ctrl.bell;
        proc = k->BellProc;
        ctrl = (void *) &(k->ctrl);
        class = KbdFeedbackClass;
    }
    else if (stuff->feedbackclass == BellFeedbackClass) {
        for (b = dev->bell; b; b = b->next)
            if (b->ctrl.id == stuff->feedbackid)
                break;
        if (!b) {
            client->errorValue = stuff->feedbackid;
            return BadValue;
        }
        base = b->ctrl.percent;
        proc = b->BellProc;
        ctrl = (void *) &(b->ctrl);
        class = BellFeedbackClass;
    }
    else {
        client->errorValue = stuff->feedbackclass;
        return BadValue;
    }
    newpercent = (base * stuff->percent) / 100;
    if (stuff->percent < 0)
        newpercent = base + newpercent;
    else
        newpercent = base - newpercent + stuff->percent;
    if (proc == NULL)
        return BadValue;
    (*proc) (newpercent, dev, ctrl, class);
    return Success;
}
