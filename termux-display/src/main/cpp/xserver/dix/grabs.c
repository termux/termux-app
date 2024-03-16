/*

Copyright 1987, 1998  The Open Group

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

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.
DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN action OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "misc.h"
#include <X11/Xproto.h>
#include <X11/extensions/XI2.h>
#include "windowstr.h"
#include "inputstr.h"
#include "cursorstr.h"
#include "dixgrabs.h"
#include "xace.h"
#include "exevents.h"
#include "exglobals.h"
#include "inpututils.h"
#include "client.h"

#define BITMASK(i) (((Mask)1) << ((i) & 31))
#define MASKIDX(i) ((i) >> 5)
#define MASKWORD(buf, i) buf[MASKIDX(i)]
#define BITSET(buf, i) MASKWORD(buf, i) |= BITMASK(i)
#define BITCLEAR(buf, i) MASKWORD(buf, i) &= ~BITMASK(i)
#define GETBIT(buf, i) (MASKWORD(buf, i) & BITMASK(i))

void
PrintDeviceGrabInfo(DeviceIntPtr dev)
{
    ClientPtr client;
    LocalClientCredRec *lcc;
    int i, j;
    GrabInfoPtr devGrab = &dev->deviceGrab;
    GrabPtr grab = devGrab->grab;
    Bool clientIdPrinted = FALSE;

    ErrorF("Active grab 0x%lx (%s) on device '%s' (%d):\n",
           (unsigned long) grab->resource,
           (grab->grabtype == XI2) ? "xi2" :
           ((grab->grabtype == CORE) ? "core" : "xi1"), dev->name, dev->id);

    client = clients[CLIENT_ID(grab->resource)];
    if (client) {
        pid_t clientpid = GetClientPid(client);
        const char *cmdname = GetClientCmdName(client);
        const char *cmdargs = GetClientCmdArgs(client);

        if ((clientpid > 0) && (cmdname != NULL)) {
            ErrorF("      client pid %ld %s %s\n",
                   (long) clientpid, cmdname, cmdargs ? cmdargs : "");
            clientIdPrinted = TRUE;
        }
        else if (GetLocalClientCreds(client, &lcc) != -1) {
            ErrorF("      client pid %ld uid %ld gid %ld\n",
                   (lcc->fieldsSet & LCC_PID_SET) ? (long) lcc->pid : 0,
                   (lcc->fieldsSet & LCC_UID_SET) ? (long) lcc->euid : 0,
                   (lcc->fieldsSet & LCC_GID_SET) ? (long) lcc->egid : 0);
            FreeLocalClientCreds(lcc);
            clientIdPrinted = TRUE;
        }
    }
    if (!clientIdPrinted) {
        ErrorF("      (no client information available for client %d)\n",
               CLIENT_ID(grab->resource));
    }

    /* XXX is this even correct? */
    if (devGrab->sync.other)
        ErrorF("      grab ID 0x%lx from paired device\n",
               (unsigned long) devGrab->sync.other->resource);

    ErrorF("      at %ld (from %s grab)%s (device %s, state %d)\n",
           (unsigned long) devGrab->grabTime.milliseconds,
           devGrab->fromPassiveGrab ? "passive" : "active",
           devGrab->implicitGrab ? " (implicit)" : "",
           devGrab->sync.frozen ? "frozen" : "thawed", devGrab->sync.state);

    if (grab->grabtype == CORE) {
        ErrorF("        core event mask 0x%lx\n",
               (unsigned long) grab->eventMask);
    }
    else if (grab->grabtype == XI) {
        ErrorF("      xi1 event mask 0x%lx\n",
               devGrab->implicitGrab ? (unsigned long) grab->deviceMask :
               (unsigned long) grab->eventMask);
    }
    else if (grab->grabtype == XI2) {
        for (i = 0; i < xi2mask_num_masks(grab->xi2mask); i++) {
            const unsigned char *mask;
            int print;

            print = 0;
            for (j = 0; j < XI2MASKSIZE; j++) {
                mask = xi2mask_get_one_mask(grab->xi2mask, i);
                if (mask[j]) {
                    print = 1;
                    break;
                }
            }
            if (!print)
                continue;
            ErrorF("      xi2 event mask for device %d: 0x", dev->id);
            for (j = 0; j < xi2mask_mask_size(grab->xi2mask); j++)
                ErrorF("%x", mask[j]);
            ErrorF("\n");
        }
    }

    if (devGrab->fromPassiveGrab) {
        ErrorF("      passive grab type %d, detail 0x%x, "
               "activating key %d\n", grab->type, grab->detail.exact,
               devGrab->activatingKey);
    }

    ErrorF("      owner-events %s, kb %d ptr %d, confine %lx, cursor 0x%lx\n",
           grab->ownerEvents ? "true" : "false",
           grab->keyboardMode, grab->pointerMode,
           grab->confineTo ? (unsigned long) grab->confineTo->drawable.id : 0,
           grab->cursor ? (unsigned long) grab->cursor->id : 0);
}

void
UngrabAllDevices(Bool kill_client)
{
    DeviceIntPtr dev;
    ClientPtr client;

    ErrorF("Ungrabbing all devices%s; grabs listed below:\n",
           kill_client ? " and killing their owners" : "");

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (!dev->deviceGrab.grab)
            continue;
        PrintDeviceGrabInfo(dev);
        client = clients[CLIENT_ID(dev->deviceGrab.grab->resource)];
        if (!kill_client || !client || client->clientGone)
            dev->deviceGrab.DeactivateGrab(dev);
        if (kill_client)
            CloseDownClient(client);
    }

    ErrorF("End list of ungrabbed devices\n");
}

GrabPtr
AllocGrab(const GrabPtr src)
{
    GrabPtr grab = calloc(1, sizeof(GrabRec));

    if (grab) {
        grab->xi2mask = xi2mask_new();
        if (!grab->xi2mask) {
            free(grab);
            grab = NULL;
        }
        else if (src && !CopyGrab(grab, src)) {
            free(grab->xi2mask);
            free(grab);
            grab = NULL;
        }
    }

    return grab;
}

GrabPtr
CreateGrab(int client, DeviceIntPtr device, DeviceIntPtr modDevice,
           WindowPtr window, enum InputLevel grabtype, GrabMask *mask,
           GrabParameters *param, int type,
           KeyCode keybut,        /* key or button */
           WindowPtr confineTo, CursorPtr cursor)
{
    GrabPtr grab;

    grab = AllocGrab(NULL);
    if (!grab)
        return (GrabPtr) NULL;
    grab->resource = FakeClientID(client);
    grab->device = device;
    grab->window = window;
    if (grabtype == CORE || grabtype == XI)
        grab->eventMask = mask->core;       /* same for XI */
    else
        grab->eventMask = 0;
    grab->deviceMask = 0;
    grab->ownerEvents = param->ownerEvents;
    grab->keyboardMode = param->this_device_mode;
    grab->pointerMode = param->other_devices_mode;
    grab->modifiersDetail.exact = param->modifiers;
    grab->modifiersDetail.pMask = NULL;
    grab->modifierDevice = modDevice;
    grab->type = type;
    grab->grabtype = grabtype;
    grab->detail.exact = keybut;
    grab->detail.pMask = NULL;
    grab->confineTo = confineTo;
    grab->cursor = RefCursor(cursor);
    grab->next = NULL;

    if (grabtype == XI2)
        xi2mask_merge(grab->xi2mask, mask->xi2mask);
    return grab;

}

void
FreeGrab(GrabPtr pGrab)
{
    BUG_RETURN(!pGrab);

    free(pGrab->modifiersDetail.pMask);
    free(pGrab->detail.pMask);

    if (pGrab->cursor)
        FreeCursor(pGrab->cursor, (Cursor) 0);

    xi2mask_free(&pGrab->xi2mask);
    free(pGrab);
}

Bool
CopyGrab(GrabPtr dst, const GrabPtr src)
{
    Mask *mdetails_mask = NULL;
    Mask *details_mask = NULL;
    XI2Mask *xi2mask;

    if (src->modifiersDetail.pMask) {
        int len = MasksPerDetailMask * sizeof(Mask);

        mdetails_mask = malloc(len);
        if (!mdetails_mask)
            return FALSE;
        memcpy(mdetails_mask, src->modifiersDetail.pMask, len);
    }

    if (src->detail.pMask) {
        int len = MasksPerDetailMask * sizeof(Mask);

        details_mask = malloc(len);
        if (!details_mask) {
            free(mdetails_mask);
            return FALSE;
        }
        memcpy(details_mask, src->detail.pMask, len);
    }

    if (!dst->xi2mask) {
        xi2mask = xi2mask_new();
        if (!xi2mask) {
            free(mdetails_mask);
            free(details_mask);
            return FALSE;
        }
    }
    else {
        xi2mask = dst->xi2mask;
        xi2mask_zero(xi2mask, -1);
    }

    *dst = *src;
    dst->modifiersDetail.pMask = mdetails_mask;
    dst->detail.pMask = details_mask;
    dst->xi2mask = xi2mask;
    dst->cursor = RefCursor(src->cursor);

    xi2mask_merge(dst->xi2mask, src->xi2mask);

    return TRUE;
}

int
DeletePassiveGrab(void *value, XID id)
{
    GrabPtr g, prev;
    GrabPtr pGrab = (GrabPtr) value;

    /* it is OK if the grab isn't found */
    prev = 0;
    for (g = (wPassiveGrabs(pGrab->window)); g; g = g->next) {
        if (pGrab == g) {
            if (prev)
                prev->next = g->next;
            else if (!(pGrab->window->optional->passiveGrabs = g->next))
                CheckWindowOptionalNeed(pGrab->window);
            break;
        }
        prev = g;
    }
    FreeGrab(pGrab);
    return Success;
}

static Mask *
DeleteDetailFromMask(Mask *pDetailMask, unsigned int detail)
{
    Mask *mask;
    int i;

    mask = malloc(sizeof(Mask) * MasksPerDetailMask);
    if (mask) {
        if (pDetailMask)
            for (i = 0; i < MasksPerDetailMask; i++)
                mask[i] = pDetailMask[i];
        else
            for (i = 0; i < MasksPerDetailMask; i++)
                mask[i] = ~0L;
        BITCLEAR(mask, detail);
    }
    return mask;
}

static Bool
IsInGrabMask(DetailRec firstDetail,
             DetailRec secondDetail, unsigned int exception)
{
    if (firstDetail.exact == exception) {
        if (firstDetail.pMask == NULL)
            return TRUE;

        /* (at present) never called with two non-null pMasks */
        if (secondDetail.exact == exception)
            return FALSE;

        if (GETBIT(firstDetail.pMask, secondDetail.exact))
            return TRUE;
    }

    return FALSE;
}

static Bool
IdenticalExactDetails(unsigned int firstExact,
                      unsigned int secondExact, unsigned int exception)
{
    if ((firstExact == exception) || (secondExact == exception))
        return FALSE;

    if (firstExact == secondExact)
        return TRUE;

    return FALSE;
}

static Bool
DetailSupersedesSecond(DetailRec firstDetail,
                       DetailRec secondDetail, unsigned int exception)
{
    if (IsInGrabMask(firstDetail, secondDetail, exception))
        return TRUE;

    if (IdenticalExactDetails(firstDetail.exact, secondDetail.exact, exception))
        return TRUE;

    return FALSE;
}

static Bool
GrabSupersedesSecond(GrabPtr pFirstGrab, GrabPtr pSecondGrab)
{
    unsigned int any_modifier = (pFirstGrab->grabtype == XI2) ?
        (unsigned int) XIAnyModifier : (unsigned int) AnyModifier;
    if (!DetailSupersedesSecond(pFirstGrab->modifiersDetail,
                                pSecondGrab->modifiersDetail, any_modifier))
        return FALSE;

    if (DetailSupersedesSecond(pFirstGrab->detail,
                               pSecondGrab->detail, (unsigned int) AnyKey))
        return TRUE;

    return FALSE;
}

/**
 * Compares two grabs and returns TRUE if the first grab matches the second
 * grab.
 *
 * A match is when
 *  - the devices set for the grab are equal (this is optional).
 *  - the event types for both grabs are equal.
 *  - XXX
 *
 * @param ignoreDevice TRUE if the device settings on the grabs are to be
 * ignored.
 * @return TRUE if the grabs match or FALSE otherwise.
 */
Bool
GrabMatchesSecond(GrabPtr pFirstGrab, GrabPtr pSecondGrab, Bool ignoreDevice)
{
    unsigned int any_modifier = (pFirstGrab->grabtype == XI2) ?
        (unsigned int) XIAnyModifier : (unsigned int) AnyModifier;

    if (pFirstGrab->grabtype != pSecondGrab->grabtype)
        return FALSE;

    if (pFirstGrab->grabtype == XI2) {
        if (pFirstGrab->device == inputInfo.all_devices ||
            pSecondGrab->device == inputInfo.all_devices) {
            /* do nothing */
        }
        else if (pFirstGrab->device == inputInfo.all_master_devices) {
            if (pSecondGrab->device != inputInfo.all_master_devices &&
                !IsMaster(pSecondGrab->device))
                return FALSE;
        }
        else if (pSecondGrab->device == inputInfo.all_master_devices) {
            if (pFirstGrab->device != inputInfo.all_master_devices &&
                !IsMaster(pFirstGrab->device))
                return FALSE;
        }
        else if (pSecondGrab->device != pFirstGrab->device)
            return FALSE;
    }
    else if (!ignoreDevice &&
             ((pFirstGrab->device != pSecondGrab->device) ||
              (pFirstGrab->modifierDevice != pSecondGrab->modifierDevice)))
        return FALSE;

    if (pFirstGrab->type != pSecondGrab->type)
        return FALSE;

    if (GrabSupersedesSecond(pFirstGrab, pSecondGrab) ||
        GrabSupersedesSecond(pSecondGrab, pFirstGrab))
        return TRUE;

    if (DetailSupersedesSecond(pSecondGrab->detail, pFirstGrab->detail,
                               (unsigned int) AnyKey)
        &&
        DetailSupersedesSecond(pFirstGrab->modifiersDetail,
                               pSecondGrab->modifiersDetail, any_modifier))
        return TRUE;

    if (DetailSupersedesSecond(pFirstGrab->detail, pSecondGrab->detail,
                               (unsigned int) AnyKey)
        &&
        DetailSupersedesSecond(pSecondGrab->modifiersDetail,
                               pFirstGrab->modifiersDetail, any_modifier))
        return TRUE;

    return FALSE;
}

static Bool
GrabsAreIdentical(GrabPtr pFirstGrab, GrabPtr pSecondGrab)
{
    unsigned int any_modifier = (pFirstGrab->grabtype == XI2) ?
        (unsigned int) XIAnyModifier : (unsigned int) AnyModifier;

    if (pFirstGrab->grabtype != pSecondGrab->grabtype)
        return FALSE;

    if (pFirstGrab->device != pSecondGrab->device ||
        (pFirstGrab->modifierDevice != pSecondGrab->modifierDevice) ||
        (pFirstGrab->type != pSecondGrab->type))
        return FALSE;

    if (!(DetailSupersedesSecond(pFirstGrab->detail,
                                 pSecondGrab->detail,
                                 (unsigned int) AnyKey) &&
          DetailSupersedesSecond(pSecondGrab->detail,
                                 pFirstGrab->detail, (unsigned int) AnyKey)))
        return FALSE;

    if (!(DetailSupersedesSecond(pFirstGrab->modifiersDetail,
                                 pSecondGrab->modifiersDetail,
                                 any_modifier) &&
          DetailSupersedesSecond(pSecondGrab->modifiersDetail,
                                 pFirstGrab->modifiersDetail, any_modifier)))
        return FALSE;

    return TRUE;
}

/**
 * Prepend the new grab to the list of passive grabs on the window.
 * Any previously existing grab that matches the new grab will be removed.
 * Adding a new grab that would override another client's grab will result in
 * a BadAccess.
 *
 * @return Success or X error code on failure.
 */
int
AddPassiveGrabToList(ClientPtr client, GrabPtr pGrab)
{
    GrabPtr grab;
    Mask access_mode = DixGrabAccess;
    int rc;

    for (grab = wPassiveGrabs(pGrab->window); grab; grab = grab->next) {
        if (GrabMatchesSecond(pGrab, grab, (pGrab->grabtype == CORE))) {
            if (CLIENT_BITS(pGrab->resource) != CLIENT_BITS(grab->resource)) {
                FreeGrab(pGrab);
                return BadAccess;
            }
        }
    }

    if (pGrab->keyboardMode == GrabModeSync ||
        pGrab->pointerMode == GrabModeSync)
        access_mode |= DixFreezeAccess;
    rc = XaceHook(XACE_DEVICE_ACCESS, client, pGrab->device, access_mode);
    if (rc != Success)
        return rc;

    /* Remove all grabs that match the new one exactly */
    for (grab = wPassiveGrabs(pGrab->window); grab; grab = grab->next) {
        if (GrabsAreIdentical(pGrab, grab)) {
            DeletePassiveGrabFromList(grab);
            break;
        }
    }

    if (!pGrab->window->optional && !MakeWindowOptional(pGrab->window)) {
        FreeGrab(pGrab);
        return BadAlloc;
    }

    pGrab->next = pGrab->window->optional->passiveGrabs;
    pGrab->window->optional->passiveGrabs = pGrab;
    if (AddResource(pGrab->resource, RT_PASSIVEGRAB, (void *) pGrab))
        return Success;
    return BadAlloc;
}

/* the following is kinda complicated, because we need to be able to back out
 * if any allocation fails
 */

Bool
DeletePassiveGrabFromList(GrabPtr pMinuendGrab)
{
    GrabPtr grab;
    GrabPtr *deletes, *adds;
    Mask ***updates, **details;
    int i, ndels, nadds, nups;
    Bool ok;
    unsigned int any_modifier;
    unsigned int any_key;

#define UPDATE(mask,exact) \
	if (!(details[nups] = DeleteDetailFromMask(mask, exact))) \
	  ok = FALSE; \
	else \
	  updates[nups++] = &(mask)

    i = 0;
    for (grab = wPassiveGrabs(pMinuendGrab->window); grab; grab = grab->next)
        i++;
    if (!i)
        return TRUE;
    deletes = xallocarray(i, sizeof(GrabPtr));
    adds = xallocarray(i, sizeof(GrabPtr));
    updates = xallocarray(i, sizeof(Mask **));
    details = xallocarray(i, sizeof(Mask *));
    if (!deletes || !adds || !updates || !details) {
        free(details);
        free(updates);
        free(adds);
        free(deletes);
        return FALSE;
    }

    any_modifier = (pMinuendGrab->grabtype == XI2) ?
        (unsigned int) XIAnyModifier : (unsigned int) AnyModifier;
    any_key = (pMinuendGrab->grabtype == XI2) ?
        (unsigned int) XIAnyKeycode : (unsigned int) AnyKey;
    ndels = nadds = nups = 0;
    ok = TRUE;
    for (grab = wPassiveGrabs(pMinuendGrab->window);
         grab && ok; grab = grab->next) {
        if ((CLIENT_BITS(grab->resource) != CLIENT_BITS(pMinuendGrab->resource))
            || !GrabMatchesSecond(grab, pMinuendGrab, (grab->grabtype == CORE)))
            continue;
        if (GrabSupersedesSecond(pMinuendGrab, grab)) {
            deletes[ndels++] = grab;
        }
        else if ((grab->detail.exact == any_key)
                 && (grab->modifiersDetail.exact != any_modifier)) {
            UPDATE(grab->detail.pMask, pMinuendGrab->detail.exact);
        }
        else if ((grab->modifiersDetail.exact == any_modifier)
                 && (grab->detail.exact != any_key)) {
            UPDATE(grab->modifiersDetail.pMask,
                   pMinuendGrab->modifiersDetail.exact);
        }
        else if ((pMinuendGrab->detail.exact != any_key)
                 && (pMinuendGrab->modifiersDetail.exact != any_modifier)) {
            GrabPtr pNewGrab;
            GrabParameters param;

            UPDATE(grab->detail.pMask, pMinuendGrab->detail.exact);

            memset(&param, 0, sizeof(param));
            param.ownerEvents = grab->ownerEvents;
            param.this_device_mode = grab->keyboardMode;
            param.other_devices_mode = grab->pointerMode;
            param.modifiers = any_modifier;

            pNewGrab = CreateGrab(CLIENT_ID(grab->resource), grab->device,
                                  grab->modifierDevice, grab->window,
                                  grab->grabtype,
                                  (GrabMask *) &grab->eventMask,
                                  &param, (int) grab->type,
                                  pMinuendGrab->detail.exact,
                                  grab->confineTo, grab->cursor);
            if (!pNewGrab)
                ok = FALSE;
            else if (!(pNewGrab->modifiersDetail.pMask =
                       DeleteDetailFromMask(grab->modifiersDetail.pMask,
                                            pMinuendGrab->modifiersDetail.
                                            exact))
                     || (!pNewGrab->window->optional &&
                         !MakeWindowOptional(pNewGrab->window))) {
                FreeGrab(pNewGrab);
                ok = FALSE;
            }
            else if (!AddResource(pNewGrab->resource, RT_PASSIVEGRAB,
                                  (void *) pNewGrab))
                ok = FALSE;
            else
                adds[nadds++] = pNewGrab;
        }
        else if (pMinuendGrab->detail.exact == any_key) {
            UPDATE(grab->modifiersDetail.pMask,
                   pMinuendGrab->modifiersDetail.exact);
        }
        else {
            UPDATE(grab->detail.pMask, pMinuendGrab->detail.exact);
        }
    }

    if (!ok) {
        for (i = 0; i < nadds; i++)
            FreeResource(adds[i]->resource, RT_NONE);
        for (i = 0; i < nups; i++)
            free(details[i]);
    }
    else {
        for (i = 0; i < ndels; i++)
            FreeResource(deletes[i]->resource, RT_NONE);
        for (i = 0; i < nadds; i++) {
            grab = adds[i];
            grab->next = grab->window->optional->passiveGrabs;
            grab->window->optional->passiveGrabs = grab;
        }
        for (i = 0; i < nups; i++) {
            free(*updates[i]);
            *updates[i] = details[i];
        }
    }
    free(details);
    free(updates);
    free(adds);
    free(deletes);
    return ok;

#undef UPDATE
}

Bool
GrabIsPointerGrab(GrabPtr grab)
{
    return (grab->type == ButtonPress ||
            grab->type == DeviceButtonPress || grab->type == XI_ButtonPress);
}

Bool
GrabIsKeyboardGrab(GrabPtr grab)
{
    return (grab->type == KeyPress ||
            grab->type == DeviceKeyPress || grab->type == XI_KeyPress);
}

Bool
GrabIsGestureGrab(GrabPtr grab)
{
    return (grab->type == XI_GesturePinchBegin ||
            grab->type == XI_GestureSwipeBegin);
}
