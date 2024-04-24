/************************************************************

Author: Eamon Walsh <ewalsh@tycho.nsa.gov>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "selection.h"
#include "inputstr.h"
#include "windowstr.h"
#include "propertyst.h"
#include "extnsionst.h"
#include "extinit.h"
#include "xselinuxint.h"

#define CTX_DEV offsetof(SELinuxSubjectRec, dev_create_sid)
#define CTX_WIN offsetof(SELinuxSubjectRec, win_create_sid)
#define CTX_PRP offsetof(SELinuxSubjectRec, prp_create_sid)
#define CTX_SEL offsetof(SELinuxSubjectRec, sel_create_sid)
#define USE_PRP offsetof(SELinuxSubjectRec, prp_use_sid)
#define USE_SEL offsetof(SELinuxSubjectRec, sel_use_sid)

typedef struct {
    char *octx;
    char *dctx;
    CARD32 octx_len;
    CARD32 dctx_len;
    CARD32 id;
} SELinuxListItemRec;

/*
 * Extension Dispatch
 */

static char *
SELinuxCopyContext(char *ptr, unsigned len)
{
    char *copy = malloc(len + 1);

    if (!copy)
        return NULL;
    strncpy(copy, ptr, len);
    copy[len] = '\0';
    return copy;
}

static int
ProcSELinuxQueryVersion(ClientPtr client)
{
    SELinuxQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .server_major = SELINUX_MAJOR_VERSION,
        .server_minor = SELINUX_MINOR_VERSION
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.server_major);
        swaps(&rep.server_minor);
    }
    WriteToClient(client, sizeof(rep), &rep);
    return Success;
}

static int
SELinuxSendContextReply(ClientPtr client, security_id_t sid)
{
    SELinuxGetContextReply rep;
    char *ctx = NULL;
    int len = 0;

    if (sid) {
        if (avc_sid_to_context_raw(sid, &ctx) < 0)
            return BadValue;
        len = strlen(ctx) + 1;
    }

    rep = (SELinuxGetContextReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(len),
        .context_len = len
    };

    if (client->swapped) {
        swapl(&rep.length);
        swaps(&rep.sequenceNumber);
        swapl(&rep.context_len);
    }

    WriteToClient(client, sizeof(SELinuxGetContextReply), &rep);
    WriteToClient(client, len, ctx);
    freecon(ctx);
    return Success;
}

static int
ProcSELinuxSetCreateContext(ClientPtr client, unsigned offset)
{
    PrivateRec **privPtr = &client->devPrivates;
    security_id_t *pSid;
    char *ctx = NULL;
    char *ptr;
    int rc;

    REQUEST(SELinuxSetCreateContextReq);
    REQUEST_FIXED_SIZE(SELinuxSetCreateContextReq, stuff->context_len);

    if (stuff->context_len > 0) {
        ctx = SELinuxCopyContext((char *) (stuff + 1), stuff->context_len);
        if (!ctx)
            return BadAlloc;
    }

    ptr = dixLookupPrivate(privPtr, subjectKey);
    pSid = (security_id_t *) (ptr + offset);
    *pSid = NULL;

    rc = Success;
    if (stuff->context_len > 0) {
        if (security_check_context_raw(ctx) < 0 ||
            avc_context_to_sid_raw(ctx, pSid) < 0)
            rc = BadValue;
    }

    free(ctx);
    return rc;
}

static int
ProcSELinuxGetCreateContext(ClientPtr client, unsigned offset)
{
    security_id_t *pSid;
    char *ptr;

    REQUEST_SIZE_MATCH(SELinuxGetCreateContextReq);

    if (offset == CTX_DEV)
        ptr = dixLookupPrivate(&serverClient->devPrivates, subjectKey);
    else
        ptr = dixLookupPrivate(&client->devPrivates, subjectKey);

    pSid = (security_id_t *) (ptr + offset);
    return SELinuxSendContextReply(client, *pSid);
}

static int
ProcSELinuxSetDeviceContext(ClientPtr client)
{
    char *ctx;
    security_id_t sid;
    DeviceIntPtr dev;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    int rc;

    REQUEST(SELinuxSetContextReq);
    REQUEST_FIXED_SIZE(SELinuxSetContextReq, stuff->context_len);

    if (stuff->context_len < 1)
        return BadLength;
    ctx = SELinuxCopyContext((char *) (stuff + 1), stuff->context_len);
    if (!ctx)
        return BadAlloc;

    rc = dixLookupDevice(&dev, stuff->id, client, DixManageAccess);
    if (rc != Success)
        goto out;

    if (security_check_context_raw(ctx) < 0 ||
        avc_context_to_sid_raw(ctx, &sid) < 0) {
        rc = BadValue;
        goto out;
    }

    subj = dixLookupPrivate(&dev->devPrivates, subjectKey);
    subj->sid = sid;
    obj = dixLookupPrivate(&dev->devPrivates, objectKey);
    obj->sid = sid;

    rc = Success;
 out:
    free(ctx);
    return rc;
}

static int
ProcSELinuxGetDeviceContext(ClientPtr client)
{
    DeviceIntPtr dev;
    SELinuxSubjectRec *subj;
    int rc;

    REQUEST(SELinuxGetContextReq);
    REQUEST_SIZE_MATCH(SELinuxGetContextReq);

    rc = dixLookupDevice(&dev, stuff->id, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    subj = dixLookupPrivate(&dev->devPrivates, subjectKey);
    return SELinuxSendContextReply(client, subj->sid);
}

static int
ProcSELinuxGetDrawableContext(ClientPtr client)
{
    DrawablePtr pDraw;
    PrivateRec **privatePtr;
    SELinuxObjectRec *obj;
    int rc;

    REQUEST(SELinuxGetContextReq);
    REQUEST_SIZE_MATCH(SELinuxGetContextReq);

    rc = dixLookupDrawable(&pDraw, stuff->id, client, 0, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    if (pDraw->type == DRAWABLE_PIXMAP)
        privatePtr = &((PixmapPtr) pDraw)->devPrivates;
    else
        privatePtr = &((WindowPtr) pDraw)->devPrivates;

    obj = dixLookupPrivate(privatePtr, objectKey);
    return SELinuxSendContextReply(client, obj->sid);
}

static int
ProcSELinuxGetPropertyContext(ClientPtr client, void *privKey)
{
    WindowPtr pWin;
    PropertyPtr pProp;
    SELinuxObjectRec *obj;
    int rc;

    REQUEST(SELinuxGetPropertyContextReq);
    REQUEST_SIZE_MATCH(SELinuxGetPropertyContextReq);

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetPropAccess);
    if (rc != Success)
        return rc;

    rc = dixLookupProperty(&pProp, pWin, stuff->property, client,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;

    obj = dixLookupPrivate(&pProp->devPrivates, privKey);
    return SELinuxSendContextReply(client, obj->sid);
}

static int
ProcSELinuxGetSelectionContext(ClientPtr client, void *privKey)
{
    Selection *pSel;
    SELinuxObjectRec *obj;
    int rc;

    REQUEST(SELinuxGetContextReq);
    REQUEST_SIZE_MATCH(SELinuxGetContextReq);

    rc = dixLookupSelection(&pSel, stuff->id, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    obj = dixLookupPrivate(&pSel->devPrivates, privKey);
    return SELinuxSendContextReply(client, obj->sid);
}

static int
ProcSELinuxGetClientContext(ClientPtr client)
{
    ClientPtr target;
    SELinuxSubjectRec *subj;
    int rc;

    REQUEST(SELinuxGetContextReq);
    REQUEST_SIZE_MATCH(SELinuxGetContextReq);

    rc = dixLookupClient(&target, stuff->id, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    subj = dixLookupPrivate(&target->devPrivates, subjectKey);
    return SELinuxSendContextReply(client, subj->sid);
}

static int
SELinuxPopulateItem(SELinuxListItemRec * i, PrivateRec ** privPtr, CARD32 id,
                    int *size)
{
    SELinuxObjectRec *obj = dixLookupPrivate(privPtr, objectKey);
    SELinuxObjectRec *data = dixLookupPrivate(privPtr, dataKey);

    if (avc_sid_to_context_raw(obj->sid, &i->octx) < 0)
        return BadValue;
    if (avc_sid_to_context_raw(data->sid, &i->dctx) < 0)
        return BadValue;

    i->id = id;
    i->octx_len = bytes_to_int32(strlen(i->octx) + 1);
    i->dctx_len = bytes_to_int32(strlen(i->dctx) + 1);

    *size += i->octx_len + i->dctx_len + 3;
    return Success;
}

static void
SELinuxFreeItems(SELinuxListItemRec * items, int count)
{
    int k;

    for (k = 0; k < count; k++) {
        freecon(items[k].octx);
        freecon(items[k].dctx);
    }
    free(items);
}

static int
SELinuxSendItemsToClient(ClientPtr client, SELinuxListItemRec * items,
                         int size, int count)
{
    int rc, k, pos = 0;
    SELinuxListItemsReply rep;
    CARD32 *buf;

    buf = calloc(size, sizeof(CARD32));
    if (size && !buf) {
        rc = BadAlloc;
        goto out;
    }

    /* Fill in the buffer */
    for (k = 0; k < count; k++) {
        buf[pos] = items[k].id;
        if (client->swapped)
            swapl(buf + pos);
        pos++;

        buf[pos] = items[k].octx_len * 4;
        if (client->swapped)
            swapl(buf + pos);
        pos++;

        buf[pos] = items[k].dctx_len * 4;
        if (client->swapped)
            swapl(buf + pos);
        pos++;

        memcpy((char *) (buf + pos), items[k].octx, strlen(items[k].octx) + 1);
        pos += items[k].octx_len;
        memcpy((char *) (buf + pos), items[k].dctx, strlen(items[k].dctx) + 1);
        pos += items[k].dctx_len;
    }

    /* Send reply to client */
    rep = (SELinuxListItemsReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = size,
        .count = count
    };

    if (client->swapped) {
        swapl(&rep.length);
        swaps(&rep.sequenceNumber);
        swapl(&rep.count);
    }

    WriteToClient(client, sizeof(SELinuxListItemsReply), &rep);
    WriteToClient(client, size * 4, buf);

    /* Free stuff and return */
    rc = Success;
    free(buf);
 out:
    SELinuxFreeItems(items, count);
    return rc;
}

static int
ProcSELinuxListProperties(ClientPtr client)
{
    WindowPtr pWin;
    PropertyPtr pProp;
    SELinuxListItemRec *items;
    int rc, count, size, i;
    CARD32 id;

    REQUEST(SELinuxGetContextReq);
    REQUEST_SIZE_MATCH(SELinuxGetContextReq);

    rc = dixLookupWindow(&pWin, stuff->id, client, DixListPropAccess);
    if (rc != Success)
        return rc;

    /* Count the number of properties and allocate items */
    count = 0;
    for (pProp = wUserProps(pWin); pProp; pProp = pProp->next)
        count++;
    items = calloc(count, sizeof(SELinuxListItemRec));
    if (count && !items)
        return BadAlloc;

    /* Fill in the items and calculate size */
    i = 0;
    size = 0;
    for (pProp = wUserProps(pWin); pProp; pProp = pProp->next) {
        id = pProp->propertyName;
        rc = SELinuxPopulateItem(items + i, &pProp->devPrivates, id, &size);
        if (rc != Success) {
            SELinuxFreeItems(items, count);
            return rc;
        }
        i++;
    }

    return SELinuxSendItemsToClient(client, items, size, count);
}

static int
ProcSELinuxListSelections(ClientPtr client)
{
    Selection *pSel;
    SELinuxListItemRec *items;
    int rc, count, size, i;
    CARD32 id;

    REQUEST_SIZE_MATCH(SELinuxGetCreateContextReq);

    /* Count the number of selections and allocate items */
    count = 0;
    for (pSel = CurrentSelections; pSel; pSel = pSel->next)
        count++;
    items = calloc(count, sizeof(SELinuxListItemRec));
    if (count && !items)
        return BadAlloc;

    /* Fill in the items and calculate size */
    i = 0;
    size = 0;
    for (pSel = CurrentSelections; pSel; pSel = pSel->next) {
        id = pSel->selection;
        rc = SELinuxPopulateItem(items + i, &pSel->devPrivates, id, &size);
        if (rc != Success) {
            SELinuxFreeItems(items, count);
            return rc;
        }
        i++;
    }

    return SELinuxSendItemsToClient(client, items, size, count);
}

static int
ProcSELinuxDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_SELinuxQueryVersion:
        return ProcSELinuxQueryVersion(client);
    case X_SELinuxSetDeviceCreateContext:
        return ProcSELinuxSetCreateContext(client, CTX_DEV);
    case X_SELinuxGetDeviceCreateContext:
        return ProcSELinuxGetCreateContext(client, CTX_DEV);
    case X_SELinuxSetDeviceContext:
        return ProcSELinuxSetDeviceContext(client);
    case X_SELinuxGetDeviceContext:
        return ProcSELinuxGetDeviceContext(client);
    case X_SELinuxSetDrawableCreateContext:
        return ProcSELinuxSetCreateContext(client, CTX_WIN);
    case X_SELinuxGetDrawableCreateContext:
        return ProcSELinuxGetCreateContext(client, CTX_WIN);
    case X_SELinuxGetDrawableContext:
        return ProcSELinuxGetDrawableContext(client);
    case X_SELinuxSetPropertyCreateContext:
        return ProcSELinuxSetCreateContext(client, CTX_PRP);
    case X_SELinuxGetPropertyCreateContext:
        return ProcSELinuxGetCreateContext(client, CTX_PRP);
    case X_SELinuxSetPropertyUseContext:
        return ProcSELinuxSetCreateContext(client, USE_PRP);
    case X_SELinuxGetPropertyUseContext:
        return ProcSELinuxGetCreateContext(client, USE_PRP);
    case X_SELinuxGetPropertyContext:
        return ProcSELinuxGetPropertyContext(client, objectKey);
    case X_SELinuxGetPropertyDataContext:
        return ProcSELinuxGetPropertyContext(client, dataKey);
    case X_SELinuxListProperties:
        return ProcSELinuxListProperties(client);
    case X_SELinuxSetSelectionCreateContext:
        return ProcSELinuxSetCreateContext(client, CTX_SEL);
    case X_SELinuxGetSelectionCreateContext:
        return ProcSELinuxGetCreateContext(client, CTX_SEL);
    case X_SELinuxSetSelectionUseContext:
        return ProcSELinuxSetCreateContext(client, USE_SEL);
    case X_SELinuxGetSelectionUseContext:
        return ProcSELinuxGetCreateContext(client, USE_SEL);
    case X_SELinuxGetSelectionContext:
        return ProcSELinuxGetSelectionContext(client, objectKey);
    case X_SELinuxGetSelectionDataContext:
        return ProcSELinuxGetSelectionContext(client, dataKey);
    case X_SELinuxListSelections:
        return ProcSELinuxListSelections(client);
    case X_SELinuxGetClientContext:
        return ProcSELinuxGetClientContext(client);
    default:
        return BadRequest;
    }
}

static int _X_COLD
SProcSELinuxQueryVersion(ClientPtr client)
{
    return ProcSELinuxQueryVersion(client);
}

static int _X_COLD
SProcSELinuxSetCreateContext(ClientPtr client, unsigned offset)
{
    REQUEST(SELinuxSetCreateContextReq);

    REQUEST_AT_LEAST_SIZE(SELinuxSetCreateContextReq);
    swapl(&stuff->context_len);
    return ProcSELinuxSetCreateContext(client, offset);
}

static int _X_COLD
SProcSELinuxSetDeviceContext(ClientPtr client)
{
    REQUEST(SELinuxSetContextReq);

    REQUEST_AT_LEAST_SIZE(SELinuxSetContextReq);
    swapl(&stuff->id);
    swapl(&stuff->context_len);
    return ProcSELinuxSetDeviceContext(client);
}

static int _X_COLD
SProcSELinuxGetDeviceContext(ClientPtr client)
{
    REQUEST(SELinuxGetContextReq);

    REQUEST_SIZE_MATCH(SELinuxGetContextReq);
    swapl(&stuff->id);
    return ProcSELinuxGetDeviceContext(client);
}

static int _X_COLD
SProcSELinuxGetDrawableContext(ClientPtr client)
{
    REQUEST(SELinuxGetContextReq);

    REQUEST_SIZE_MATCH(SELinuxGetContextReq);
    swapl(&stuff->id);
    return ProcSELinuxGetDrawableContext(client);
}

static int _X_COLD
SProcSELinuxGetPropertyContext(ClientPtr client, void *privKey)
{
    REQUEST(SELinuxGetPropertyContextReq);

    REQUEST_SIZE_MATCH(SELinuxGetPropertyContextReq);
    swapl(&stuff->window);
    swapl(&stuff->property);
    return ProcSELinuxGetPropertyContext(client, privKey);
}

static int _X_COLD
SProcSELinuxGetSelectionContext(ClientPtr client, void *privKey)
{
    REQUEST(SELinuxGetContextReq);

    REQUEST_SIZE_MATCH(SELinuxGetContextReq);
    swapl(&stuff->id);
    return ProcSELinuxGetSelectionContext(client, privKey);
}

static int _X_COLD
SProcSELinuxListProperties(ClientPtr client)
{
    REQUEST(SELinuxGetContextReq);

    REQUEST_SIZE_MATCH(SELinuxGetContextReq);
    swapl(&stuff->id);
    return ProcSELinuxListProperties(client);
}

static int _X_COLD
SProcSELinuxGetClientContext(ClientPtr client)
{
    REQUEST(SELinuxGetContextReq);

    REQUEST_SIZE_MATCH(SELinuxGetContextReq);
    swapl(&stuff->id);
    return ProcSELinuxGetClientContext(client);
}

static int _X_COLD
SProcSELinuxDispatch(ClientPtr client)
{
    REQUEST(xReq);

    swaps(&stuff->length);

    switch (stuff->data) {
    case X_SELinuxQueryVersion:
        return SProcSELinuxQueryVersion(client);
    case X_SELinuxSetDeviceCreateContext:
        return SProcSELinuxSetCreateContext(client, CTX_DEV);
    case X_SELinuxGetDeviceCreateContext:
        return ProcSELinuxGetCreateContext(client, CTX_DEV);
    case X_SELinuxSetDeviceContext:
        return SProcSELinuxSetDeviceContext(client);
    case X_SELinuxGetDeviceContext:
        return SProcSELinuxGetDeviceContext(client);
    case X_SELinuxSetDrawableCreateContext:
        return SProcSELinuxSetCreateContext(client, CTX_WIN);
    case X_SELinuxGetDrawableCreateContext:
        return ProcSELinuxGetCreateContext(client, CTX_WIN);
    case X_SELinuxGetDrawableContext:
        return SProcSELinuxGetDrawableContext(client);
    case X_SELinuxSetPropertyCreateContext:
        return SProcSELinuxSetCreateContext(client, CTX_PRP);
    case X_SELinuxGetPropertyCreateContext:
        return ProcSELinuxGetCreateContext(client, CTX_PRP);
    case X_SELinuxSetPropertyUseContext:
        return SProcSELinuxSetCreateContext(client, USE_PRP);
    case X_SELinuxGetPropertyUseContext:
        return ProcSELinuxGetCreateContext(client, USE_PRP);
    case X_SELinuxGetPropertyContext:
        return SProcSELinuxGetPropertyContext(client, objectKey);
    case X_SELinuxGetPropertyDataContext:
        return SProcSELinuxGetPropertyContext(client, dataKey);
    case X_SELinuxListProperties:
        return SProcSELinuxListProperties(client);
    case X_SELinuxSetSelectionCreateContext:
        return SProcSELinuxSetCreateContext(client, CTX_SEL);
    case X_SELinuxGetSelectionCreateContext:
        return ProcSELinuxGetCreateContext(client, CTX_SEL);
    case X_SELinuxSetSelectionUseContext:
        return SProcSELinuxSetCreateContext(client, USE_SEL);
    case X_SELinuxGetSelectionUseContext:
        return ProcSELinuxGetCreateContext(client, USE_SEL);
    case X_SELinuxGetSelectionContext:
        return SProcSELinuxGetSelectionContext(client, objectKey);
    case X_SELinuxGetSelectionDataContext:
        return SProcSELinuxGetSelectionContext(client, dataKey);
    case X_SELinuxListSelections:
        return ProcSELinuxListSelections(client);
    case X_SELinuxGetClientContext:
        return SProcSELinuxGetClientContext(client);
    default:
        return BadRequest;
    }
}

/*
 * Extension Setup / Teardown
 */

static void
SELinuxResetProc(ExtensionEntry * extEntry)
{
    SELinuxFlaskReset();
    SELinuxLabelReset();
}

void
SELinuxExtensionInit(void)
{
    /* Check SELinux mode on system, configuration file, and boolean */
    if (!is_selinux_enabled()) {
        LogMessage(X_INFO, "SELinux: Disabled on system\n");
        return;
    }
    if (selinuxEnforcingState == SELINUX_MODE_DISABLED) {
        LogMessage(X_INFO, "SELinux: Disabled in configuration file\n");
        return;
    }
    if (!security_get_boolean_active("xserver_object_manager")) {
        LogMessage(X_INFO, "SELinux: Disabled by boolean\n");
        return;
    }

    /* Set up XACE hooks */
    SELinuxLabelInit();
    SELinuxFlaskInit();

    /* Add extension to server */
    AddExtension(SELINUX_EXTENSION_NAME, SELinuxNumberEvents,
                 SELinuxNumberErrors, ProcSELinuxDispatch,
                 SProcSELinuxDispatch, SELinuxResetProc, StandardMinorOpcode);
}
