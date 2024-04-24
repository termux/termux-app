/***********************************************************

Copyright 1987, 1998  The Open Group

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

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

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
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "windowstr.h"
#include "propertyst.h"
#include "dixstruct.h"
#include "dispatch.h"
#include "swaprep.h"
#include "xace.h"

/*****************************************************************
 * Property Stuff
 *
 *    dixLookupProperty, dixChangeProperty, DeleteProperty
 *
 *   Properties belong to windows.  The list of properties should not be
 *   traversed directly.  Instead, use the three functions listed above.
 *
 *****************************************************************/

#ifdef notdef
static void
PrintPropertys(WindowPtr pWin)
{
    PropertyPtr pProp;
    int j;

    pProp = pWin->userProps;
    while (pProp) {
        ErrorF("[dix] %x %x\n", pProp->propertyName, pProp->type);
        ErrorF("[dix] property format: %d\n", pProp->format);
        ErrorF("[dix] property data: \n");
        for (j = 0; j < (pProp->format / 8) * pProp->size; j++)
            ErrorF("[dix] %c\n", pProp->data[j]);
        pProp = pProp->next;
    }
}
#endif

int
dixLookupProperty(PropertyPtr *result, WindowPtr pWin, Atom propertyName,
                  ClientPtr client, Mask access_mode)
{
    PropertyPtr pProp;
    int rc = BadMatch;

    client->errorValue = propertyName;

    for (pProp = wUserProps(pWin); pProp; pProp = pProp->next)
        if (pProp->propertyName == propertyName)
            break;

    if (pProp)
        rc = XaceHookPropertyAccess(client, pWin, &pProp, access_mode);
    *result = pProp;
    return rc;
}

CallbackListPtr PropertyStateCallback;

static void
deliverPropertyNotifyEvent(WindowPtr pWin, int state, PropertyPtr pProp)
{
    xEvent event;
    PropertyStateRec rec = {
        .win = pWin,
        .prop = pProp,
        .state = state
    };
    UpdateCurrentTimeIf();
    event = (xEvent) {
        .u.property.window = pWin->drawable.id,
        .u.property.state = state,
        .u.property.atom = pProp->propertyName,
        .u.property.time = currentTime.milliseconds,
    };
    event.u.u.type = PropertyNotify;

    CallCallbacks(&PropertyStateCallback, &rec);
    DeliverEvents(pWin, &event, 1, (WindowPtr) NULL);
}

int
ProcRotateProperties(ClientPtr client)
{
    int i, j, delta, rc;

    REQUEST(xRotatePropertiesReq);
    WindowPtr pWin;
    Atom *atoms;
    PropertyPtr *props;         /* array of pointer */
    PropertyPtr pProp, saved;

    REQUEST_FIXED_SIZE(xRotatePropertiesReq, stuff->nAtoms << 2);
    UpdateCurrentTime();
    rc = dixLookupWindow(&pWin, stuff->window, client, DixSetPropAccess);
    if (rc != Success || stuff->nAtoms <= 0)
        return rc;

    atoms = (Atom *) &stuff[1];
    props = xallocarray(stuff->nAtoms, sizeof(PropertyPtr));
    saved = xallocarray(stuff->nAtoms, sizeof(PropertyRec));
    if (!props || !saved) {
        rc = BadAlloc;
        goto out;
    }

    for (i = 0; i < stuff->nAtoms; i++) {
        if (!ValidAtom(atoms[i])) {
            rc = BadAtom;
            client->errorValue = atoms[i];
            goto out;
        }
        for (j = i + 1; j < stuff->nAtoms; j++)
            if (atoms[j] == atoms[i]) {
                rc = BadMatch;
                goto out;
            }

        rc = dixLookupProperty(&pProp, pWin, atoms[i], client,
                               DixReadAccess | DixWriteAccess);
        if (rc != Success)
            goto out;

        props[i] = pProp;
        saved[i] = *pProp;
    }
    delta = stuff->nPositions;

    /* If the rotation is a complete 360 degrees, then moving the properties
       around and generating PropertyNotify events should be skipped. */

    if (abs(delta) % stuff->nAtoms) {
        while (delta < 0)       /* faster if abs value is small */
            delta += stuff->nAtoms;
        for (i = 0; i < stuff->nAtoms; i++) {
            j = (i + delta) % stuff->nAtoms;
            deliverPropertyNotifyEvent(pWin, PropertyNewValue, props[i]);

            /* Preserve name and devPrivates */
            props[j]->type = saved[i].type;
            props[j]->format = saved[i].format;
            props[j]->size = saved[i].size;
            props[j]->data = saved[i].data;
        }
    }
 out:
    free(saved);
    free(props);
    return rc;
}

int
ProcChangeProperty(ClientPtr client)
{
    WindowPtr pWin;
    char format, mode;
    unsigned long len;
    int sizeInBytes, err;
    uint64_t totalSize;

    REQUEST(xChangePropertyReq);

    REQUEST_AT_LEAST_SIZE(xChangePropertyReq);
    UpdateCurrentTime();
    format = stuff->format;
    mode = stuff->mode;
    if ((mode != PropModeReplace) && (mode != PropModeAppend) &&
        (mode != PropModePrepend)) {
        client->errorValue = mode;
        return BadValue;
    }
    if ((format != 8) && (format != 16) && (format != 32)) {
        client->errorValue = format;
        return BadValue;
    }
    len = stuff->nUnits;
    if (len > bytes_to_int32(0xffffffff - sizeof(xChangePropertyReq)))
        return BadLength;
    sizeInBytes = format >> 3;
    totalSize = len * sizeInBytes;
    REQUEST_FIXED_SIZE(xChangePropertyReq, totalSize);

    err = dixLookupWindow(&pWin, stuff->window, client, DixSetPropAccess);
    if (err != Success)
        return err;
    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }
    if (!ValidAtom(stuff->type)) {
        client->errorValue = stuff->type;
        return BadAtom;
    }

    err = dixChangeWindowProperty(client, pWin, stuff->property, stuff->type,
                                  (int) format, (int) mode, len, &stuff[1],
                                  TRUE);
    if (err != Success)
        return err;
    else
        return Success;
}

int
dixChangeWindowProperty(ClientPtr pClient, WindowPtr pWin, Atom property,
                        Atom type, int format, int mode, unsigned long len,
                        const void *value, Bool sendevent)
{
    PropertyPtr pProp;
    PropertyRec savedProp;
    int sizeInBytes, totalSize, rc;
    unsigned char *data;
    Mask access_mode;

    sizeInBytes = format >> 3;
    totalSize = len * sizeInBytes;
    access_mode = (mode == PropModeReplace) ? DixWriteAccess : DixBlendAccess;

    /* first see if property already exists */
    rc = dixLookupProperty(&pProp, pWin, property, pClient, access_mode);

    if (rc == BadMatch) {       /* just add to list */
        if (!pWin->optional && !MakeWindowOptional(pWin))
            return BadAlloc;
        pProp = dixAllocateObjectWithPrivates(PropertyRec, PRIVATE_PROPERTY);
        if (!pProp)
            return BadAlloc;
        data = malloc(totalSize);
        if (!data && len) {
            dixFreeObjectWithPrivates(pProp, PRIVATE_PROPERTY);
            return BadAlloc;
        }
        memcpy(data, value, totalSize);
        pProp->propertyName = property;
        pProp->type = type;
        pProp->format = format;
        pProp->data = data;
        pProp->size = len;
        rc = XaceHookPropertyAccess(pClient, pWin, &pProp,
                                    DixCreateAccess | DixWriteAccess);
        if (rc != Success) {
            free(data);
            dixFreeObjectWithPrivates(pProp, PRIVATE_PROPERTY);
            pClient->errorValue = property;
            return rc;
        }
        pProp->next = pWin->optional->userProps;
        pWin->optional->userProps = pProp;
    }
    else if (rc == Success) {
        /* To append or prepend to a property the request format and type
           must match those of the already defined property.  The
           existing format and type are irrelevant when using the mode
           "PropModeReplace" since they will be written over. */

        if ((format != pProp->format) && (mode != PropModeReplace))
            return BadMatch;
        if ((pProp->type != type) && (mode != PropModeReplace))
            return BadMatch;

        /* save the old values for later */
        savedProp = *pProp;

        if (mode == PropModeReplace) {
            data = malloc(totalSize);
            if (!data && len)
                return BadAlloc;
            memcpy(data, value, totalSize);
            pProp->data = data;
            pProp->size = len;
            pProp->type = type;
            pProp->format = format;
        }
        else if (len == 0) {
            /* do nothing */
        }
        else if (mode == PropModeAppend) {
            data = xallocarray(pProp->size + len, sizeInBytes);
            if (!data)
                return BadAlloc;
            memcpy(data, pProp->data, pProp->size * sizeInBytes);
            memcpy(data + pProp->size * sizeInBytes, value, totalSize);
            pProp->data = data;
            pProp->size += len;
        }
        else if (mode == PropModePrepend) {
            data = xallocarray(len + pProp->size, sizeInBytes);
            if (!data)
                return BadAlloc;
            memcpy(data + totalSize, pProp->data, pProp->size * sizeInBytes);
            memcpy(data, value, totalSize);
            pProp->data = data;
            pProp->size += len;
        }

        /* Allow security modules to check the new content */
        access_mode |= DixPostAccess;
        rc = XaceHookPropertyAccess(pClient, pWin, &pProp, access_mode);
        if (rc == Success) {
            if (savedProp.data != pProp->data)
                free(savedProp.data);
        }
        else {
            if (savedProp.data != pProp->data)
                free(pProp->data);
            *pProp = savedProp;
            return rc;
        }
    }
    else
        return rc;

    if (sendevent)
        deliverPropertyNotifyEvent(pWin, PropertyNewValue, pProp);

    return Success;
}

int
DeleteProperty(ClientPtr client, WindowPtr pWin, Atom propName)
{
    PropertyPtr pProp, prevProp;
    int rc;

    rc = dixLookupProperty(&pProp, pWin, propName, client, DixDestroyAccess);
    if (rc == BadMatch)
        return Success;         /* Succeed if property does not exist */

    if (rc == Success) {
        if (pWin->optional->userProps == pProp) {
            /* Takes care of head */
            if (!(pWin->optional->userProps = pProp->next))
                CheckWindowOptionalNeed(pWin);
        }
        else {
            /* Need to traverse to find the previous element */
            prevProp = pWin->optional->userProps;
            while (prevProp->next != pProp)
                prevProp = prevProp->next;
            prevProp->next = pProp->next;
        }

        deliverPropertyNotifyEvent(pWin, PropertyDelete, pProp);
        free(pProp->data);
        dixFreeObjectWithPrivates(pProp, PRIVATE_PROPERTY);
    }
    return rc;
}

void
DeleteAllWindowProperties(WindowPtr pWin)
{
    PropertyPtr pProp, pNextProp;

    pProp = wUserProps(pWin);
    while (pProp) {
        deliverPropertyNotifyEvent(pWin, PropertyDelete, pProp);
        pNextProp = pProp->next;
        free(pProp->data);
        dixFreeObjectWithPrivates(pProp, PRIVATE_PROPERTY);
        pProp = pNextProp;
    }

    if (pWin->optional)
        pWin->optional->userProps = NULL;
}

static int
NullPropertyReply(ClientPtr client, ATOM propertyType, int format)
{
    xGetPropertyReply reply = {
        .type = X_Reply,
        .format = format,
        .sequenceNumber = client->sequence,
        .length = 0,
        .propertyType = propertyType,
        .bytesAfter = 0,
        .nItems = 0
    };
    WriteReplyToClient(client, sizeof(xGenericReply), &reply);
    return Success;
}

/*****************
 * GetProperty
 *    If type Any is specified, returns the property from the specified
 *    window regardless of its type.  If a type is specified, returns the
 *    property only if its type equals the specified type.
 *    If delete is True and a property is returned, the property is also
 *    deleted from the window and a PropertyNotify event is generated on the
 *    window.
 *****************/

int
ProcGetProperty(ClientPtr client)
{
    PropertyPtr pProp, prevProp;
    unsigned long n, len, ind;
    int rc;
    WindowPtr pWin;
    xGetPropertyReply reply;
    Mask win_mode = DixGetPropAccess, prop_mode = DixReadAccess;

    REQUEST(xGetPropertyReq);

    REQUEST_SIZE_MATCH(xGetPropertyReq);
    if (stuff->delete) {
        UpdateCurrentTime();
        win_mode |= DixSetPropAccess;
        prop_mode |= DixDestroyAccess;
    }
    rc = dixLookupWindow(&pWin, stuff->window, client, win_mode);
    if (rc != Success)
        return rc;

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }
    if ((stuff->delete != xTrue) && (stuff->delete != xFalse)) {
        client->errorValue = stuff->delete;
        return BadValue;
    }
    if ((stuff->type != AnyPropertyType) && !ValidAtom(stuff->type)) {
        client->errorValue = stuff->type;
        return BadAtom;
    }

    rc = dixLookupProperty(&pProp, pWin, stuff->property, client, prop_mode);
    if (rc == BadMatch)
        return NullPropertyReply(client, None, 0);
    else if (rc != Success)
        return rc;

    /* If the request type and actual type don't match. Return the
       property information, but not the data. */

    if (((stuff->type != pProp->type) && (stuff->type != AnyPropertyType))
        ) {
        reply = (xGetPropertyReply) {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .bytesAfter = pProp->size,
            .format = pProp->format,
            .length = 0,
            .nItems = 0,
            .propertyType = pProp->type
        };
        WriteReplyToClient(client, sizeof(xGenericReply), &reply);
        return Success;
    }

/*
 *  Return type, format, value to client
 */
    n = (pProp->format / 8) * pProp->size;      /* size (bytes) of prop */
    ind = stuff->longOffset << 2;

    /* If longOffset is invalid such that it causes "len" to
       be negative, it's a value error. */

    if (n < ind) {
        client->errorValue = stuff->longOffset;
        return BadValue;
    }

    len = min(n - ind, 4 * stuff->longLength);

    reply = (xGetPropertyReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .bytesAfter = n - (ind + len),
        .format = pProp->format,
        .length = bytes_to_int32(len),
        .nItems = len / (pProp->format / 8),
        .propertyType = pProp->type
    };

    if (stuff->delete && (reply.bytesAfter == 0))
        deliverPropertyNotifyEvent(pWin, PropertyDelete, pProp);

    WriteReplyToClient(client, sizeof(xGenericReply), &reply);
    if (len) {
        switch (reply.format) {
        case 32:
            client->pSwapReplyFunc = (ReplySwapPtr) CopySwap32Write;
            break;
        case 16:
            client->pSwapReplyFunc = (ReplySwapPtr) CopySwap16Write;
            break;
        default:
            client->pSwapReplyFunc = (ReplySwapPtr) WriteToClient;
            break;
        }
        WriteSwappedDataToClient(client, len, (char *) pProp->data + ind);
    }

    if (stuff->delete && (reply.bytesAfter == 0)) {
        /* Delete the Property */
        if (pWin->optional->userProps == pProp) {
            /* Takes care of head */
            if (!(pWin->optional->userProps = pProp->next))
                CheckWindowOptionalNeed(pWin);
        }
        else {
            /* Need to traverse to find the previous element */
            prevProp = pWin->optional->userProps;
            while (prevProp->next != pProp)
                prevProp = prevProp->next;
            prevProp->next = pProp->next;
        }

        free(pProp->data);
        dixFreeObjectWithPrivates(pProp, PRIVATE_PROPERTY);
    }
    return Success;
}

int
ProcListProperties(ClientPtr client)
{
    Atom *pAtoms = NULL, *temppAtoms;
    xListPropertiesReply xlpr;
    int rc, numProps = 0;
    WindowPtr pWin;
    PropertyPtr pProp, realProp;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixListPropAccess);
    if (rc != Success)
        return rc;

    for (pProp = wUserProps(pWin); pProp; pProp = pProp->next)
        numProps++;

    if (numProps && !(pAtoms = xallocarray(numProps, sizeof(Atom))))
        return BadAlloc;

    numProps = 0;
    temppAtoms = pAtoms;
    for (pProp = wUserProps(pWin); pProp; pProp = pProp->next) {
        realProp = pProp;
        rc = XaceHookPropertyAccess(client, pWin, &realProp, DixGetAttrAccess);
        if (rc == Success && realProp == pProp) {
            *temppAtoms++ = pProp->propertyName;
            numProps++;
        }
    }

    xlpr = (xListPropertiesReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(numProps * sizeof(Atom)),
        .nProperties = numProps
    };
    WriteReplyToClient(client, sizeof(xGenericReply), &xlpr);
    if (numProps) {
        client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
        WriteSwappedDataToClient(client, numProps * sizeof(Atom), pAtoms);
    }
    free(pAtoms);
    return Success;
}

int
ProcDeleteProperty(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xDeletePropertyReq);
    int result;

    REQUEST_SIZE_MATCH(xDeletePropertyReq);
    UpdateCurrentTime();
    result = dixLookupWindow(&pWin, stuff->window, client, DixSetPropAccess);
    if (result != Success)
        return result;
    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    return DeleteProperty(client, pWin, stuff->property);
}
