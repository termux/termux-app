/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "randrstr.h"
#include "propertyst.h"
#include "swaprep.h"

static int
DeliverPropertyEvent(WindowPtr pWin, void *value)
{
    xRRProviderPropertyNotifyEvent *event = value;
    RREventPtr *pHead, pRREvent;

    dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                            RREventType, serverClient, DixReadAccess);
    if (!pHead)
        return WT_WALKCHILDREN;

    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) {
        if (!(pRREvent->mask & RRProviderPropertyNotifyMask))
            continue;

        event->window = pRREvent->window->drawable.id;
        WriteEventsToClient(pRREvent->client, 1, (xEvent *) event);
    }

    return WT_WALKCHILDREN;
}

static void
RRDeliverPropertyEvent(ScreenPtr pScreen, xEvent *event)
{
    if (!(dispatchException & (DE_RESET | DE_TERMINATE)))
        WalkTree(pScreen, DeliverPropertyEvent, event);
}

static void
RRDestroyProviderProperty(RRPropertyPtr prop)
{
    free(prop->valid_values);
    free(prop->current.data);
    free(prop->pending.data);
    free(prop);
}

static void
RRDeleteProperty(RRProviderRec * provider, RRPropertyRec * prop)
{
    xRRProviderPropertyNotifyEvent event = {
        .type = RREventBase + RRNotify,
        .subCode = RRNotify_ProviderProperty,
        .provider = provider->id,
        .state = PropertyDelete,
        .atom = prop->propertyName,
        .timestamp = currentTime.milliseconds
    };

    RRDeliverPropertyEvent(provider->pScreen, (xEvent *) &event);

    RRDestroyProviderProperty(prop);
}

void
RRDeleteAllProviderProperties(RRProviderPtr provider)
{
    RRPropertyPtr prop, next;

    for (prop = provider->properties; prop; prop = next) {
        next = prop->next;
        RRDeleteProperty(provider, prop);
    }
}

static void
RRInitProviderPropertyValue(RRPropertyValuePtr property_value)
{
    property_value->type = None;
    property_value->format = 0;
    property_value->size = 0;
    property_value->data = NULL;
}

static RRPropertyPtr
RRCreateProviderProperty(Atom property)
{
    RRPropertyPtr prop;

    prop = (RRPropertyPtr) malloc(sizeof(RRPropertyRec));
    if (!prop)
        return NULL;
    prop->next = NULL;
    prop->propertyName = property;
    prop->is_pending = FALSE;
    prop->range = FALSE;
    prop->immutable = FALSE;
    prop->num_valid = 0;
    prop->valid_values = NULL;
    RRInitProviderPropertyValue(&prop->current);
    RRInitProviderPropertyValue(&prop->pending);
    return prop;
}

void
RRDeleteProviderProperty(RRProviderPtr provider, Atom property)
{
    RRPropertyRec *prop, **prev;

    for (prev = &provider->properties; (prop = *prev); prev = &(prop->next))
        if (prop->propertyName == property) {
            *prev = prop->next;
            RRDeleteProperty(provider, prop);
            return;
        }
}

int
RRChangeProviderProperty(RRProviderPtr provider, Atom property, Atom type,
                       int format, int mode, unsigned long len,
                       void *value, Bool sendevent, Bool pending)
{
    RRPropertyPtr prop;
    rrScrPrivPtr pScrPriv = rrGetScrPriv(provider->pScreen);
    int size_in_bytes;
    int total_size;
    unsigned long total_len;
    RRPropertyValuePtr prop_value;
    RRPropertyValueRec new_value;
    Bool add = FALSE;

    size_in_bytes = format >> 3;

    /* first see if property already exists */
    prop = RRQueryProviderProperty(provider, property);
    if (!prop) {                /* just add to list */
        prop = RRCreateProviderProperty(property);
        if (!prop)
            return BadAlloc;
        add = TRUE;
        mode = PropModeReplace;
    }
    if (pending && prop->is_pending)
        prop_value = &prop->pending;
    else
        prop_value = &prop->current;

    /* To append or prepend to a property the request format and type
       must match those of the already defined property.  The
       existing format and type are irrelevant when using the mode
       "PropModeReplace" since they will be written over. */

    if ((format != prop_value->format) && (mode != PropModeReplace))
        return BadMatch;
    if ((prop_value->type != type) && (mode != PropModeReplace))
        return BadMatch;
    new_value = *prop_value;
    if (mode == PropModeReplace)
        total_len = len;
    else
        total_len = prop_value->size + len;

    if (mode == PropModeReplace || len > 0) {
        void *new_data = NULL, *old_data = NULL;

        total_size = total_len * size_in_bytes;
        new_value.data = (void *) malloc(total_size);
        if (!new_value.data && total_size) {
            if (add)
                RRDestroyProviderProperty(prop);
            return BadAlloc;
        }
        new_value.size = len;
        new_value.type = type;
        new_value.format = format;

        switch (mode) {
        case PropModeReplace:
            new_data = new_value.data;
            old_data = NULL;
            break;
        case PropModeAppend:
            new_data = (void *) (((char *) new_value.data) +
                                  (prop_value->size * size_in_bytes));
            old_data = new_value.data;
            break;
        case PropModePrepend:
            new_data = new_value.data;
            old_data = (void *) (((char *) new_value.data) +
                                  (prop_value->size * size_in_bytes));
            break;
        }
        if (new_data)
            memcpy((char *) new_data, (char *) value, len * size_in_bytes);
        if (old_data)
            memcpy((char *) old_data, (char *) prop_value->data,
                   prop_value->size * size_in_bytes);

        if (pending && pScrPriv->rrProviderSetProperty &&
            !pScrPriv->rrProviderSetProperty(provider->pScreen, provider,
                                           prop->propertyName, &new_value)) {
            if (add)
                RRDestroyProviderProperty(prop);
            free(new_value.data);
            return BadValue;
        }
        free(prop_value->data);
        *prop_value = new_value;
    }

    else if (len == 0) {
        /* do nothing */
    }

    if (add) {
        prop->next = provider->properties;
        provider->properties = prop;
    }

    if (pending && prop->is_pending)
        provider->pendingProperties = TRUE;

    if (sendevent) {
        xRRProviderPropertyNotifyEvent event = {
            .type = RREventBase + RRNotify,
            .subCode = RRNotify_ProviderProperty,
            .provider = provider->id,
            .state = PropertyNewValue,
            .atom = prop->propertyName,
            .timestamp = currentTime.milliseconds
        };
        RRDeliverPropertyEvent(provider->pScreen, (xEvent *) &event);
    }
    return Success;
}

Bool
RRPostProviderPendingProperties(RRProviderPtr provider)
{
    RRPropertyValuePtr pending_value;
    RRPropertyValuePtr current_value;
    RRPropertyPtr property;
    Bool ret = TRUE;

    if (!provider->pendingProperties)
        return TRUE;

    provider->pendingProperties = FALSE;
    for (property = provider->properties; property; property = property->next) {
        /* Skip non-pending properties */
        if (!property->is_pending)
            continue;

        pending_value = &property->pending;
        current_value = &property->current;

        /*
         * If the pending and current values are equal, don't mark it
         * as changed (which would deliver an event)
         */
        if (pending_value->type == current_value->type &&
            pending_value->format == current_value->format &&
            pending_value->size == current_value->size &&
            !memcmp(pending_value->data, current_value->data,
                    pending_value->size * (pending_value->format / 8)))
            continue;

        if (RRChangeProviderProperty(provider, property->propertyName,
                                   pending_value->type, pending_value->format,
                                   PropModeReplace, pending_value->size,
                                   pending_value->data, TRUE, FALSE) != Success)
            ret = FALSE;
    }
    return ret;
}

RRPropertyPtr
RRQueryProviderProperty(RRProviderPtr provider, Atom property)
{
    RRPropertyPtr prop;

    for (prop = provider->properties; prop; prop = prop->next)
        if (prop->propertyName == property)
            return prop;
    return NULL;
}

RRPropertyValuePtr
RRGetProviderProperty(RRProviderPtr provider, Atom property, Bool pending)
{
    RRPropertyPtr prop = RRQueryProviderProperty(provider, property);
    rrScrPrivPtr pScrPriv = rrGetScrPriv(provider->pScreen);

    if (!prop)
        return NULL;
    if (pending && prop->is_pending)
        return &prop->pending;
    else {
#if RANDR_13_INTERFACE
        /* If we can, try to update the property value first */
        if (pScrPriv->rrProviderGetProperty)
            pScrPriv->rrProviderGetProperty(provider->pScreen, provider,
                                          prop->propertyName);
#endif
        return &prop->current;
    }
}

int
RRConfigureProviderProperty(RRProviderPtr provider, Atom property,
                          Bool pending, Bool range, Bool immutable,
                          int num_values, INT32 *values)
{
    RRPropertyPtr prop = RRQueryProviderProperty(provider, property);
    Bool add = FALSE;
    INT32 *new_values;

    if (!prop) {
        prop = RRCreateProviderProperty(property);
        if (!prop)
            return BadAlloc;
        add = TRUE;
    }
    else if (prop->immutable && !immutable)
        return BadAccess;

    /*
     * ranges must have even number of values
     */
    if (range && (num_values & 1)) {
        if (add)
            RRDestroyProviderProperty(prop);
        return BadMatch;
    }

    new_values = xallocarray(num_values, sizeof(INT32));
    if (!new_values && num_values) {
        if (add)
            RRDestroyProviderProperty(prop);
        return BadAlloc;
    }
    if (num_values)
        memcpy(new_values, values, num_values * sizeof(INT32));

    /*
     * Property moving from pending to non-pending
     * loses any pending values
     */
    if (prop->is_pending && !pending) {
        free(prop->pending.data);
        RRInitProviderPropertyValue(&prop->pending);
    }

    prop->is_pending = pending;
    prop->range = range;
    prop->immutable = immutable;
    prop->num_valid = num_values;
    free(prop->valid_values);
    prop->valid_values = new_values;

    if (add) {
        prop->next = provider->properties;
        provider->properties = prop;
    }

    return Success;
}

int
ProcRRListProviderProperties(ClientPtr client)
{
    REQUEST(xRRListProviderPropertiesReq);
    Atom *pAtoms = NULL, *temppAtoms;
    xRRListProviderPropertiesReply rep;
    int numProps = 0;
    RRProviderPtr provider;
    RRPropertyPtr prop;

    REQUEST_SIZE_MATCH(xRRListProviderPropertiesReq);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    for (prop = provider->properties; prop; prop = prop->next)
        numProps++;
    if (numProps)
        if (!(pAtoms = xallocarray(numProps, sizeof(Atom))))
            return BadAlloc;

    rep = (xRRListProviderPropertiesReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(numProps * sizeof(Atom)),
        .nAtoms = numProps
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.nAtoms);
    }
    temppAtoms = pAtoms;
    for (prop = provider->properties; prop; prop = prop->next)
        *temppAtoms++ = prop->propertyName;

    WriteToClient(client, sizeof(xRRListProviderPropertiesReply), (char *) &rep);
    if (numProps) {
        client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
        WriteSwappedDataToClient(client, numProps * sizeof(Atom), pAtoms);
        free(pAtoms);
    }
    return Success;
}

int
ProcRRQueryProviderProperty(ClientPtr client)
{
    REQUEST(xRRQueryProviderPropertyReq);
    xRRQueryProviderPropertyReply rep;
    RRProviderPtr provider;
    RRPropertyPtr prop;
    char *extra = NULL;

    REQUEST_SIZE_MATCH(xRRQueryProviderPropertyReq);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    prop = RRQueryProviderProperty(provider, stuff->property);
    if (!prop)
        return BadName;

    if (prop->num_valid) {
        extra = xallocarray(prop->num_valid, sizeof(INT32));
        if (!extra)
            return BadAlloc;
    }
    rep = (xRRQueryProviderPropertyReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = prop->num_valid,
        .pending = prop->is_pending,
        .range = prop->range,
        .immutable = prop->immutable
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }
    WriteToClient(client, sizeof(xRRQueryProviderPropertyReply), (char *) &rep);
    if (prop->num_valid) {
        memcpy(extra, prop->valid_values, prop->num_valid * sizeof(INT32));
        client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
        WriteSwappedDataToClient(client, prop->num_valid * sizeof(INT32),
                                 extra);
        free(extra);
    }
    return Success;
}

int
ProcRRConfigureProviderProperty(ClientPtr client)
{
    REQUEST(xRRConfigureProviderPropertyReq);
    RRProviderPtr provider;
    int num_valid;

    REQUEST_AT_LEAST_SIZE(xRRConfigureProviderPropertyReq);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    num_valid =
        stuff->length - bytes_to_int32(sizeof(xRRConfigureProviderPropertyReq));
    return RRConfigureProviderProperty(provider, stuff->property, stuff->pending,
                                     stuff->range, FALSE, num_valid,
                                     (INT32 *) (stuff + 1));
}

int
ProcRRChangeProviderProperty(ClientPtr client)
{
    REQUEST(xRRChangeProviderPropertyReq);
    RRProviderPtr provider;
    char format, mode;
    unsigned long len;
    int sizeInBytes;
    int totalSize;
    int err;

    REQUEST_AT_LEAST_SIZE(xRRChangeProviderPropertyReq);
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
    if (len > bytes_to_int32((0xffffffff - sizeof(xChangePropertyReq))))
        return BadLength;
    sizeInBytes = format >> 3;
    totalSize = len * sizeInBytes;
    REQUEST_FIXED_SIZE(xRRChangeProviderPropertyReq, totalSize);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }
    if (!ValidAtom(stuff->type)) {
        client->errorValue = stuff->type;
        return BadAtom;
    }

    err = RRChangeProviderProperty(provider, stuff->property,
                                 stuff->type, (int) format,
                                 (int) mode, len, (void *) &stuff[1], TRUE,
                                 TRUE);
    if (err != Success)
        return err;
    else
        return Success;
}

int
ProcRRDeleteProviderProperty(ClientPtr client)
{
    REQUEST(xRRDeleteProviderPropertyReq);
    RRProviderPtr provider;
    RRPropertyPtr prop;

    REQUEST_SIZE_MATCH(xRRDeleteProviderPropertyReq);
    UpdateCurrentTime();
    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    prop = RRQueryProviderProperty(provider, stuff->property);
    if (!prop) {
        client->errorValue = stuff->property;
        return BadName;
    }

    if (prop->immutable) {
        client->errorValue = stuff->property;
        return BadAccess;
    }

    RRDeleteProviderProperty(provider, stuff->property);
    return Success;
}

int
ProcRRGetProviderProperty(ClientPtr client)
{
    REQUEST(xRRGetProviderPropertyReq);
    RRPropertyPtr prop, *prev;
    RRPropertyValuePtr prop_value;
    unsigned long n, len, ind;
    RRProviderPtr provider;
    xRRGetProviderPropertyReply reply = {
        .type = X_Reply,
        .sequenceNumber = client->sequence
    };
    char *extra = NULL;

    REQUEST_SIZE_MATCH(xRRGetProviderPropertyReq);
    if (stuff->delete)
        UpdateCurrentTime();
    VERIFY_RR_PROVIDER(stuff->provider, provider,
                     stuff->delete ? DixWriteAccess : DixReadAccess);

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

    for (prev = &provider->properties; (prop = *prev); prev = &prop->next)
        if (prop->propertyName == stuff->property)
            break;

    if (!prop) {
        reply.nItems = 0;
        reply.length = 0;
        reply.bytesAfter = 0;
        reply.propertyType = None;
        reply.format = 0;
        if (client->swapped) {
            swaps(&reply.sequenceNumber);
            swapl(&reply.length);
            swapl(&reply.propertyType);
            swapl(&reply.bytesAfter);
            swapl(&reply.nItems);
        }
        WriteToClient(client, sizeof(xRRGetProviderPropertyReply), &reply);
        return Success;
    }

    if (prop->immutable && stuff->delete)
        return BadAccess;

    prop_value = RRGetProviderProperty(provider, stuff->property, stuff->pending);
    if (!prop_value)
        return BadAtom;

    /* If the request type and actual type don't match. Return the
       property information, but not the data. */

    if (((stuff->type != prop_value->type) && (stuff->type != AnyPropertyType))
        ) {
        reply.bytesAfter = prop_value->size;
        reply.format = prop_value->format;
        reply.length = 0;
        reply.nItems = 0;
        reply.propertyType = prop_value->type;
        if (client->swapped) {
            swaps(&reply.sequenceNumber);
            swapl(&reply.length);
            swapl(&reply.propertyType);
            swapl(&reply.bytesAfter);
            swapl(&reply.nItems);
        }
        WriteToClient(client, sizeof(xRRGetProviderPropertyReply), &reply);
        return Success;
    }

/*
 *  Return type, format, value to client
 */
    n = (prop_value->format / 8) * prop_value->size;    /* size (bytes) of prop */
    ind = stuff->longOffset << 2;

    /* If longOffset is invalid such that it causes "len" to
       be negative, it's a value error. */

    if (n < ind) {
        client->errorValue = stuff->longOffset;
        return BadValue;
    }

    len = min(n - ind, 4 * stuff->longLength);

    if (len) {
        extra = malloc(len);
        if (!extra)
            return BadAlloc;
    }
    reply.bytesAfter = n - (ind + len);
    reply.format = prop_value->format;
    reply.length = bytes_to_int32(len);
    if (prop_value->format)
        reply.nItems = len / (prop_value->format / 8);
    else
        reply.nItems = 0;
    reply.propertyType = prop_value->type;

    if (stuff->delete && (reply.bytesAfter == 0)) {
        xRRProviderPropertyNotifyEvent event = {
            .type = RREventBase + RRNotify,
            .subCode = RRNotify_ProviderProperty,
            .provider = provider->id,
            .state = PropertyDelete,
            .atom = prop->propertyName,
            .timestamp = currentTime.milliseconds
        };
        RRDeliverPropertyEvent(provider->pScreen, (xEvent *) &event);
    }

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.propertyType);
        swapl(&reply.bytesAfter);
        swapl(&reply.nItems);
    }
    WriteToClient(client, sizeof(xGenericReply), &reply);
    if (len) {
        memcpy(extra, (char *) prop_value->data + ind, len);
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
        WriteSwappedDataToClient(client, len, extra);
        free(extra);
    }

    if (stuff->delete && (reply.bytesAfter == 0)) {     /* delete the Property */
        *prev = prop->next;
        RRDestroyProviderProperty(prop);
    }
    return Success;
}
