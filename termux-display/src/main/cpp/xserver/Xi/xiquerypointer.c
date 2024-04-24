/*
 * Copyright 2007-2008 Peter Hutterer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Peter Hutterer, University of South Australia, NICTA
 */

/***********************************************************************
 *
 * Request to query the pointer location of an extension input device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>              /* for inputstr.h    */
#include <X11/Xproto.h>         /* Request macro     */
#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2proto.h>
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"
#include "eventconvert.h"
#include "scrnintstr.h"
#include "xkbsrv.h"

#ifdef PANORAMIX
#include "panoramiXsrv.h"
#endif

#include "inpututils.h"
#include "xiquerypointer.h"

/***********************************************************************
 *
 * This procedure allows a client to query the pointer of a device.
 *
 */

int _X_COLD
SProcXIQueryPointer(ClientPtr client)
{
    REQUEST(xXIQueryPointerReq);
    REQUEST_SIZE_MATCH(xXIQueryPointerReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->win);
    return (ProcXIQueryPointer(client));
}

int
ProcXIQueryPointer(ClientPtr client)
{
    int rc;
    xXIQueryPointerReply rep;
    DeviceIntPtr pDev, kbd;
    WindowPtr pWin, t;
    SpritePtr pSprite;
    XkbStatePtr state;
    char *buttons = NULL;
    int buttons_size = 0;       /* size of buttons array */
    XIClientPtr xi_client;
    Bool have_xi22 = FALSE;

    REQUEST(xXIQueryPointerReq);
    REQUEST_SIZE_MATCH(xXIQueryPointerReq);

    /* Check if client is compliant with XInput 2.2 or later. Earlier clients
     * do not know about touches, so we must report emulated button presses. 2.2
     * and later clients are aware of touches, so we don't include emulated
     * button presses in the reply. */
    xi_client = dixLookupPrivate(&client->devPrivates, XIClientPrivateKey);
    if (version_compare(xi_client->major_version,
                        xi_client->minor_version, 2, 2) >= 0)
        have_xi22 = TRUE;

    rc = dixLookupDevice(&pDev, stuff->deviceid, client, DixReadAccess);
    if (rc != Success) {
        client->errorValue = stuff->deviceid;
        return rc;
    }

    if (pDev->valuator == NULL || IsKeyboardDevice(pDev) || (!IsMaster(pDev) && !IsFloating(pDev))) {   /* no attached devices */
        client->errorValue = stuff->deviceid;
        return BadDevice;
    }

    rc = dixLookupWindow(&pWin, stuff->win, client, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->win;
        return rc;
    }

    if (pDev->valuator->motionHintWindow)
        MaybeStopHint(pDev, client);

    if (IsMaster(pDev))
        kbd = GetMaster(pDev, MASTER_KEYBOARD);
    else
        kbd = (pDev->key) ? pDev : NULL;

    pSprite = pDev->spriteInfo->sprite;

    rep = (xXIQueryPointerReply) {
        .repType = X_Reply,
        .RepType = X_XIQueryPointer,
        .sequenceNumber = client->sequence,
        .length = 6,
        .root = (GetCurrentRootWindow(pDev))->drawable.id,
        .root_x = double_to_fp1616(pSprite->hot.x),
        .root_y = double_to_fp1616(pSprite->hot.y),
        .child = None
    };

    if (kbd) {
        state = &kbd->key->xkbInfo->state;
        rep.mods.base_mods = state->base_mods;
        rep.mods.latched_mods = state->latched_mods;
        rep.mods.locked_mods = state->locked_mods;

        rep.group.base_group = state->base_group;
        rep.group.latched_group = state->latched_group;
        rep.group.locked_group = state->locked_group;
    }

    if (pDev->button) {
        int i;

        rep.buttons_len =
            bytes_to_int32(bits_to_bytes(pDev->button->numButtons));
        rep.length += rep.buttons_len;
        buttons = calloc(rep.buttons_len, 4);
        if (!buttons)
            return BadAlloc;
        buttons_size = rep.buttons_len * 4;

        for (i = 1; i < pDev->button->numButtons; i++)
            if (BitIsOn(pDev->button->down, i))
                SetBit(buttons, pDev->button->map[i]);

        if (!have_xi22 && pDev->touch && pDev->touch->buttonsDown > 0)
            SetBit(buttons, pDev->button->map[1]);
    }
    else
        rep.buttons_len = 0;

    if (pSprite->hot.pScreen == pWin->drawable.pScreen) {
        rep.same_screen = xTrue;
        rep.win_x = double_to_fp1616(pSprite->hot.x - pWin->drawable.x);
        rep.win_y = double_to_fp1616(pSprite->hot.y - pWin->drawable.y);
        for (t = pSprite->win; t; t = t->parent)
            if (t->parent == pWin) {
                rep.child = t->drawable.id;
                break;
            }
    }
    else {
        rep.same_screen = xFalse;
        rep.win_x = 0;
        rep.win_y = 0;
    }

#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        rep.root_x += double_to_fp1616(screenInfo.screens[0]->x);
        rep.root_y += double_to_fp1616(screenInfo.screens[0]->y);
        if (stuff->win == rep.root) {
            rep.win_x += double_to_fp1616(screenInfo.screens[0]->x);
            rep.win_y += double_to_fp1616(screenInfo.screens[0]->y);
        }
    }
#endif

    WriteReplyToClient(client, sizeof(xXIQueryPointerReply), &rep);
    if (buttons)
        WriteToClient(client, buttons_size, buttons);

    free(buttons);

    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XIQueryPointer function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepXIQueryPointer(ClientPtr client, int size, xXIQueryPointerReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swapl(&rep->root);
    swapl(&rep->child);
    swapl(&rep->root_x);
    swapl(&rep->root_y);
    swapl(&rep->win_x);
    swapl(&rep->win_y);
    swaps(&rep->buttons_len);

    WriteToClient(client, size, rep);
}
