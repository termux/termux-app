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
 *  Get feedback control attributes for an extension device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exglobals.h"

#include "getfctl.h"

/***********************************************************************
 *
 * This procedure gets the control attributes for an extension device,
 * for clients on machines with a different byte ordering than the server.
 *
 */

int _X_COLD
SProcXGetFeedbackControl(ClientPtr client)
{
    REQUEST(xGetFeedbackControlReq);
    swaps(&stuff->length);
    return (ProcXGetFeedbackControl(client));
}

/***********************************************************************
 *
 * This procedure copies KbdFeedbackClass data, swapping if necessary.
 *
 */

static void
CopySwapKbdFeedback(ClientPtr client, KbdFeedbackPtr k, char **buf)
{
    int i;
    xKbdFeedbackState *k2;

    k2 = (xKbdFeedbackState *) * buf;
    k2->class = KbdFeedbackClass;
    k2->length = sizeof(xKbdFeedbackState);
    k2->id = k->ctrl.id;
    k2->click = k->ctrl.click;
    k2->percent = k->ctrl.bell;
    k2->pitch = k->ctrl.bell_pitch;
    k2->duration = k->ctrl.bell_duration;
    k2->led_mask = k->ctrl.leds;
    k2->global_auto_repeat = k->ctrl.autoRepeat;
    for (i = 0; i < 32; i++)
        k2->auto_repeats[i] = k->ctrl.autoRepeats[i];
    if (client->swapped) {
        swaps(&k2->length);
        swaps(&k2->pitch);
        swaps(&k2->duration);
        swapl(&k2->led_mask);
        swapl(&k2->led_values);
    }
    *buf += sizeof(xKbdFeedbackState);
}

/***********************************************************************
 *
 * This procedure copies PtrFeedbackClass data, swapping if necessary.
 *
 */

static void
CopySwapPtrFeedback(ClientPtr client, PtrFeedbackPtr p, char **buf)
{
    xPtrFeedbackState *p2;

    p2 = (xPtrFeedbackState *) * buf;
    p2->class = PtrFeedbackClass;
    p2->length = sizeof(xPtrFeedbackState);
    p2->id = p->ctrl.id;
    p2->accelNum = p->ctrl.num;
    p2->accelDenom = p->ctrl.den;
    p2->threshold = p->ctrl.threshold;
    if (client->swapped) {
        swaps(&p2->length);
        swaps(&p2->accelNum);
        swaps(&p2->accelDenom);
        swaps(&p2->threshold);
    }
    *buf += sizeof(xPtrFeedbackState);
}

/***********************************************************************
 *
 * This procedure copies IntegerFeedbackClass data, swapping if necessary.
 *
 */

static void
CopySwapIntegerFeedback(ClientPtr client, IntegerFeedbackPtr i, char **buf)
{
    xIntegerFeedbackState *i2;

    i2 = (xIntegerFeedbackState *) * buf;
    i2->class = IntegerFeedbackClass;
    i2->length = sizeof(xIntegerFeedbackState);
    i2->id = i->ctrl.id;
    i2->resolution = i->ctrl.resolution;
    i2->min_value = i->ctrl.min_value;
    i2->max_value = i->ctrl.max_value;
    if (client->swapped) {
        swaps(&i2->length);
        swapl(&i2->resolution);
        swapl(&i2->min_value);
        swapl(&i2->max_value);
    }
    *buf += sizeof(xIntegerFeedbackState);
}

/***********************************************************************
 *
 * This procedure copies StringFeedbackClass data, swapping if necessary.
 *
 */

static void
CopySwapStringFeedback(ClientPtr client, StringFeedbackPtr s, char **buf)
{
    int i;
    xStringFeedbackState *s2;
    KeySym *kptr;

    s2 = (xStringFeedbackState *) * buf;
    s2->class = StringFeedbackClass;
    s2->length = sizeof(xStringFeedbackState) +
        s->ctrl.num_symbols_supported * sizeof(KeySym);
    s2->id = s->ctrl.id;
    s2->max_symbols = s->ctrl.max_symbols;
    s2->num_syms_supported = s->ctrl.num_symbols_supported;
    *buf += sizeof(xStringFeedbackState);
    kptr = (KeySym *) (*buf);
    for (i = 0; i < s->ctrl.num_symbols_supported; i++)
        *kptr++ = *(s->ctrl.symbols_supported + i);
    if (client->swapped) {
        swaps(&s2->length);
        swaps(&s2->max_symbols);
        swaps(&s2->num_syms_supported);
        kptr = (KeySym *) (*buf);
        for (i = 0; i < s->ctrl.num_symbols_supported; i++, kptr++) {
            swapl(kptr);
        }
    }
    *buf += (s->ctrl.num_symbols_supported * sizeof(KeySym));
}

/***********************************************************************
 *
 * This procedure copies LedFeedbackClass data, swapping if necessary.
 *
 */

static void
CopySwapLedFeedback(ClientPtr client, LedFeedbackPtr l, char **buf)
{
    xLedFeedbackState *l2;

    l2 = (xLedFeedbackState *) * buf;
    l2->class = LedFeedbackClass;
    l2->length = sizeof(xLedFeedbackState);
    l2->id = l->ctrl.id;
    l2->led_values = l->ctrl.led_values;
    l2->led_mask = l->ctrl.led_mask;
    if (client->swapped) {
        swaps(&l2->length);
        swapl(&l2->led_values);
        swapl(&l2->led_mask);
    }
    *buf += sizeof(xLedFeedbackState);
}

/***********************************************************************
 *
 * This procedure copies BellFeedbackClass data, swapping if necessary.
 *
 */

static void
CopySwapBellFeedback(ClientPtr client, BellFeedbackPtr b, char **buf)
{
    xBellFeedbackState *b2;

    b2 = (xBellFeedbackState *) * buf;
    b2->class = BellFeedbackClass;
    b2->length = sizeof(xBellFeedbackState);
    b2->id = b->ctrl.id;
    b2->percent = b->ctrl.percent;
    b2->pitch = b->ctrl.pitch;
    b2->duration = b->ctrl.duration;
    if (client->swapped) {
        swaps(&b2->length);
        swaps(&b2->pitch);
        swaps(&b2->duration);
    }
    *buf += sizeof(xBellFeedbackState);
}

/***********************************************************************
 *
 * This procedure writes the reply for the xGetFeedbackControl function,
 * if the client and server have a different byte ordering.
 *
 */

void _X_COLD
SRepXGetFeedbackControl(ClientPtr client, int size,
                        xGetFeedbackControlReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swaps(&rep->num_feedbacks);
    WriteToClient(client, size, rep);
}

/***********************************************************************
 *
 * Get the feedback control state.
 *
 */

int
ProcXGetFeedbackControl(ClientPtr client)
{
    int rc, total_length = 0;
    char *buf, *savbuf;
    DeviceIntPtr dev;
    KbdFeedbackPtr k;
    PtrFeedbackPtr p;
    IntegerFeedbackPtr i;
    StringFeedbackPtr s;
    BellFeedbackPtr b;
    LedFeedbackPtr l;
    xGetFeedbackControlReply rep;

    REQUEST(xGetFeedbackControlReq);
    REQUEST_SIZE_MATCH(xGetFeedbackControlReq);

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep = (xGetFeedbackControlReply) {
        .repType = X_Reply,
        .RepType = X_GetFeedbackControl,
        .sequenceNumber = client->sequence,
        .length = 0,
        .num_feedbacks = 0
    };

    for (k = dev->kbdfeed; k; k = k->next) {
        rep.num_feedbacks++;
        total_length += sizeof(xKbdFeedbackState);
    }
    for (p = dev->ptrfeed; p; p = p->next) {
        rep.num_feedbacks++;
        total_length += sizeof(xPtrFeedbackState);
    }
    for (s = dev->stringfeed; s; s = s->next) {
        rep.num_feedbacks++;
        total_length += sizeof(xStringFeedbackState) +
            (s->ctrl.num_symbols_supported * sizeof(KeySym));
    }
    for (i = dev->intfeed; i; i = i->next) {
        rep.num_feedbacks++;
        total_length += sizeof(xIntegerFeedbackState);
    }
    for (l = dev->leds; l; l = l->next) {
        rep.num_feedbacks++;
        total_length += sizeof(xLedFeedbackState);
    }
    for (b = dev->bell; b; b = b->next) {
        rep.num_feedbacks++;
        total_length += sizeof(xBellFeedbackState);
    }

    if (total_length == 0)
        return BadMatch;

    buf = (char *) malloc(total_length);
    if (!buf)
        return BadAlloc;
    savbuf = buf;

    for (k = dev->kbdfeed; k; k = k->next)
        CopySwapKbdFeedback(client, k, &buf);
    for (p = dev->ptrfeed; p; p = p->next)
        CopySwapPtrFeedback(client, p, &buf);
    for (s = dev->stringfeed; s; s = s->next)
        CopySwapStringFeedback(client, s, &buf);
    for (i = dev->intfeed; i; i = i->next)
        CopySwapIntegerFeedback(client, i, &buf);
    for (l = dev->leds; l; l = l->next)
        CopySwapLedFeedback(client, l, &buf);
    for (b = dev->bell; b; b = b->next)
        CopySwapBellFeedback(client, b, &buf);

    rep.length = bytes_to_int32(total_length);
    WriteReplyToClient(client, sizeof(xGetFeedbackControlReply), &rep);
    WriteToClient(client, total_length, savbuf);
    free(savbuf);
    return Success;
}
