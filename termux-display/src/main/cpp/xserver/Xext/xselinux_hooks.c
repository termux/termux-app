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

/*
 * Portions of this code copyright (c) 2005 by Trusted Computer Solutions, Inc.
 * All rights reserved.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <sys/socket.h>
#include <stdio.h>
#include <stdarg.h>

#include <libaudit.h>

#include <X11/Xatom.h>
#include "selection.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "propertyst.h"
#include "extnsionst.h"
#include "xacestr.h"
#include "client.h"
#define _XSELINUX_NEED_FLASK_MAP
#include "xselinuxint.h"

/* structure passed to auditing callback */
typedef struct {
    ClientPtr client;           /* client */
    DeviceIntPtr dev;           /* device */
    char *command;              /* client's executable path */
    unsigned id;                /* resource id, if any */
    int restype;                /* resource type, if any */
    int event;                  /* event type, if any */
    Atom property;              /* property name, if any */
    Atom selection;             /* selection name, if any */
    char *extension;            /* extension name, if any */
} SELinuxAuditRec;

/* private state keys */
DevPrivateKeyRec subjectKeyRec;
DevPrivateKeyRec objectKeyRec;
DevPrivateKeyRec dataKeyRec;

/* audit file descriptor */
static int audit_fd;

/* atoms for window label properties */
static Atom atom_ctx;
static Atom atom_client_ctx;

/* The unlabeled SID */
static security_id_t unlabeled_sid;

/* forward declarations */
static void SELinuxScreen(CallbackListPtr *, void *, void *);

/* "true" pointer value for use as callback data */
static void *truep = (void *) 1;

/*
 * Performs an SELinux permission check.
 */
static int
SELinuxDoCheck(SELinuxSubjectRec * subj, SELinuxObjectRec * obj,
               security_class_t class, Mask mode, SELinuxAuditRec * auditdata)
{
    /* serverClient requests OK */
    if (subj->privileged)
        return Success;

    auditdata->command = subj->command;
    errno = 0;

    if (avc_has_perm(subj->sid, obj->sid, class, mode, &subj->aeref,
                     auditdata) < 0) {
        if (mode == DixUnknownAccess)
            return Success;     /* DixUnknownAccess requests OK ... for now */
        if (errno == EACCES)
            return BadAccess;
        ErrorF("SELinux: avc_has_perm: unexpected error %d\n", errno);
        return BadValue;
    }

    return Success;
}

/*
 * Labels a newly connected client.
 */
static void
SELinuxLabelClient(ClientPtr client)
{
    int fd = XaceGetConnectionNumber(client);
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    char *ctx;

    subj = dixLookupPrivate(&client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&client->devPrivates, objectKey);

    /* Try to get a context from the socket */
    if (fd < 0 || getpeercon_raw(fd, &ctx) < 0) {
        /* Otherwise, fall back to a default context */
        ctx = SELinuxDefaultClientLabel();
    }

    /* For local clients, try and determine the executable name */
    if (XaceIsLocal(client)) {
        /* Get cached command name if CLIENTIDS is enabled. */
        const char *cmdname = GetClientCmdName(client);
        Bool cached = (cmdname != NULL);

        /* If CLIENTIDS is disabled, figure out the command name from
         * scratch. */
        if (!cmdname) {
            pid_t pid = DetermineClientPid(client);

            if (pid != -1)
                DetermineClientCmd(pid, &cmdname, NULL);
        }

        if (!cmdname)
            goto finish;

        strncpy(subj->command, cmdname, COMMAND_LEN - 1);

        if (!cached)
            free((void *) cmdname);     /* const char * */
    }

 finish:
    /* Get a SID from the context */
    if (avc_context_to_sid_raw(ctx, &subj->sid) < 0)
        FatalError("SELinux: client %d: context_to_sid_raw(%s) failed\n",
                   client->index, ctx);

    obj->sid = subj->sid;
    freecon(ctx);
}

/*
 * Labels initial server objects.
 */
static void
SELinuxLabelInitial(void)
{
    int i;
    XaceScreenAccessRec srec;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    char *ctx;
    void *unused;

    /* Do the serverClient */
    subj = dixLookupPrivate(&serverClient->devPrivates, subjectKey);
    obj = dixLookupPrivate(&serverClient->devPrivates, objectKey);
    subj->privileged = 1;

    /* Use the context of the X server process for the serverClient */
    if (getcon_raw(&ctx) < 0)
        FatalError("SELinux: couldn't get context of X server process\n");

    /* Get a SID from the context */
    if (avc_context_to_sid_raw(ctx, &subj->sid) < 0)
        FatalError("SELinux: serverClient: context_to_sid(%s) failed\n", ctx);

    obj->sid = subj->sid;
    freecon(ctx);

    srec.client = serverClient;
    srec.access_mode = DixCreateAccess;
    srec.status = Success;

    for (i = 0; i < screenInfo.numScreens; i++) {
        /* Do the screen object */
        srec.screen = screenInfo.screens[i];
        SELinuxScreen(NULL, NULL, &srec);

        /* Do the default colormap */
        dixLookupResourceByType(&unused, screenInfo.screens[i]->defColormap,
                                RT_COLORMAP, serverClient, DixCreateAccess);
    }
}

/*
 * Labels new resource objects.
 */
static int
SELinuxLabelResource(XaceResourceAccessRec * rec, SELinuxSubjectRec * subj,
                     SELinuxObjectRec * obj, security_class_t class)
{
    int offset;
    security_id_t tsid;

    /* Check for a create context */
    if (rec->rtype & RC_DRAWABLE && subj->win_create_sid) {
        obj->sid = subj->win_create_sid;
        return Success;
    }

    if (rec->parent)
        offset = dixLookupPrivateOffset(rec->ptype);

    if (rec->parent && offset >= 0) {
        /* Use the SID of the parent object in the labeling operation */
        PrivateRec **privatePtr = DEVPRIV_AT(rec->parent, offset);
        SELinuxObjectRec *pobj = dixLookupPrivate(privatePtr, objectKey);

        tsid = pobj->sid;
    }
    else {
        /* Use the SID of the subject */
        tsid = subj->sid;
    }

    /* Perform a transition to obtain the final SID */
    if (avc_compute_create(subj->sid, tsid, class, &obj->sid) < 0) {
        ErrorF("SELinux: a compute_create call failed!\n");
        return BadValue;
    }

    return Success;
}

/*
 * Libselinux Callbacks
 */

static int
SELinuxAudit(void *auditdata,
             security_class_t class, char *msgbuf, size_t msgbufsize)
{
    SELinuxAuditRec *audit = auditdata;
    ClientPtr client = audit->client;
    char idNum[16];
    const char *propertyName, *selectionName;
    int major = -1, minor = -1;

    if (client) {
        REQUEST(xReq);
        if (stuff) {
            major = client->majorOp;
            minor = client->minorOp;
        }
    }
    if (audit->id)
        snprintf(idNum, 16, "%x", audit->id);

    propertyName = audit->property ? NameForAtom(audit->property) : NULL;
    selectionName = audit->selection ? NameForAtom(audit->selection) : NULL;

    return snprintf(msgbuf, msgbufsize,
                    "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                    (major >= 0) ? "request=" : "",
                    (major >= 0) ? LookupRequestName(major, minor) : "",
                    audit->command ? " comm=" : "",
                    audit->command ? audit->command : "",
                    audit->dev ? " xdevice=\"" : "",
                    audit->dev ? audit->dev->name : "",
                    audit->dev ? "\"" : "",
                    audit->id ? " resid=" : "",
                    audit->id ? idNum : "",
                    audit->restype ? " restype=" : "",
                    audit->restype ? LookupResourceName(audit->restype) : "",
                    audit->event ? " event=" : "",
                    audit->event ? LookupEventName(audit->event & 127) : "",
                    audit->property ? " property=" : "",
                    audit->property ? propertyName : "",
                    audit->selection ? " selection=" : "",
                    audit->selection ? selectionName : "",
                    audit->extension ? " extension=" : "",
                    audit->extension ? audit->extension : "");
}

static int
SELinuxLog(int type, const char *fmt, ...) _X_ATTRIBUTE_PRINTF(2, 3);

static int
SELinuxLog(int type, const char *fmt, ...)
{
    va_list ap;
    char buf[MAX_AUDIT_MESSAGE_LENGTH];
    int rc, aut;

    switch (type) {
    case SELINUX_INFO:
        aut = AUDIT_USER_MAC_POLICY_LOAD;
        break;
    case SELINUX_AVC:
        aut = AUDIT_USER_AVC;
        break;
    default:
        aut = AUDIT_USER_SELINUX_ERR;
        break;
    }

    va_start(ap, fmt);
    vsnprintf(buf, MAX_AUDIT_MESSAGE_LENGTH, fmt, ap);
    rc = audit_log_user_avc_message(audit_fd, aut, buf, NULL, NULL, NULL, 0);
    (void) rc;
    va_end(ap);
    LogMessageVerb(X_WARNING, 0, "%s", buf);
    return 0;
}

/*
 * XACE Callbacks
 */

static void
SELinuxDevice(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceDeviceAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    SELinuxAuditRec auditdata = {.client = rec->client,.dev = rec->dev };
    security_class_t cls;
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&rec->dev->devPrivates, objectKey);

    /* If this is a new object that needs labeling, do it now */
    if (rec->access_mode & DixCreateAccess) {
        SELinuxSubjectRec *dsubj;

        dsubj = dixLookupPrivate(&rec->dev->devPrivates, subjectKey);

        if (subj->dev_create_sid) {
            /* Label the device with the create context */
            obj->sid = subj->dev_create_sid;
            dsubj->sid = subj->dev_create_sid;
        }
        else {
            /* Label the device directly with the process SID */
            obj->sid = subj->sid;
            dsubj->sid = subj->sid;
        }
    }

    cls = IsPointerDevice(rec->dev) ? SECCLASS_X_POINTER : SECCLASS_X_KEYBOARD;
    rc = SELinuxDoCheck(subj, obj, cls, rec->access_mode, &auditdata);
    if (rc != Success)
        rec->status = rc;
}

static void
SELinuxSend(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceSendAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj, ev_sid;
    SELinuxAuditRec auditdata = {.client = rec->client,.dev = rec->dev };
    security_class_t class;
    int rc, i, type;

    if (rec->dev)
        subj = dixLookupPrivate(&rec->dev->devPrivates, subjectKey);
    else
        subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);

    obj = dixLookupPrivate(&rec->pWin->devPrivates, objectKey);

    /* Check send permission on window */
    rc = SELinuxDoCheck(subj, obj, SECCLASS_X_DRAWABLE, DixSendAccess,
                        &auditdata);
    if (rc != Success)
        goto err;

    /* Check send permission on specific event types */
    for (i = 0; i < rec->count; i++) {
        type = rec->events[i].u.u.type;
        class = (type & 128) ? SECCLASS_X_FAKEEVENT : SECCLASS_X_EVENT;

        rc = SELinuxEventToSID(type, obj->sid, &ev_sid);
        if (rc != Success)
            goto err;

        auditdata.event = type;
        rc = SELinuxDoCheck(subj, &ev_sid, class, DixSendAccess, &auditdata);
        if (rc != Success)
            goto err;
    }
    return;
 err:
    rec->status = rc;
}

static void
SELinuxReceive(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceReceiveAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj, ev_sid;
    SELinuxAuditRec auditdata = {.client = NULL };
    security_class_t class;
    int rc, i, type;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&rec->pWin->devPrivates, objectKey);

    /* Check receive permission on window */
    rc = SELinuxDoCheck(subj, obj, SECCLASS_X_DRAWABLE, DixReceiveAccess,
                        &auditdata);
    if (rc != Success)
        goto err;

    /* Check receive permission on specific event types */
    for (i = 0; i < rec->count; i++) {
        type = rec->events[i].u.u.type;
        class = (type & 128) ? SECCLASS_X_FAKEEVENT : SECCLASS_X_EVENT;

        rc = SELinuxEventToSID(type, obj->sid, &ev_sid);
        if (rc != Success)
            goto err;

        auditdata.event = type;
        rc = SELinuxDoCheck(subj, &ev_sid, class, DixReceiveAccess, &auditdata);
        if (rc != Success)
            goto err;
    }
    return;
 err:
    rec->status = rc;
}

static void
SELinuxExtension(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceExtAccessRec *rec = calldata;
    SELinuxSubjectRec *subj, *serv;
    SELinuxObjectRec *obj;
    SELinuxAuditRec auditdata = {.client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&rec->ext->devPrivates, objectKey);

    /* If this is a new object that needs labeling, do it now */
    /* XXX there should be a separate callback for this */
    if (obj->sid == NULL) {
        security_id_t sid;

        serv = dixLookupPrivate(&serverClient->devPrivates, subjectKey);
        rc = SELinuxExtensionToSID(rec->ext->name, &sid);
        if (rc != Success) {
            rec->status = rc;
            return;
        }

        /* Perform a transition to obtain the final SID */
        if (avc_compute_create(serv->sid, sid, SECCLASS_X_EXTENSION,
                               &obj->sid) < 0) {
            ErrorF("SELinux: a SID transition call failed!\n");
            rec->status = BadValue;
            return;
        }
    }

    /* Perform the security check */
    auditdata.extension = (char *) rec->ext->name;
    rc = SELinuxDoCheck(subj, obj, SECCLASS_X_EXTENSION, rec->access_mode,
                        &auditdata);
    if (rc != Success)
        rec->status = rc;
}

static void
SELinuxSelection(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceSelectionAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj, *data;
    Selection *pSel = *rec->ppSel;
    Atom name = pSel->selection;
    Mask access_mode = rec->access_mode;
    SELinuxAuditRec auditdata = {.client = rec->client,.selection = name };
    security_id_t tsid;
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&pSel->devPrivates, objectKey);

    /* If this is a new object that needs labeling, do it now */
    if (access_mode & DixCreateAccess) {
        rc = SELinuxSelectionToSID(name, subj, &obj->sid, &obj->poly);
        if (rc != Success)
            obj->sid = unlabeled_sid;
        access_mode = DixSetAttrAccess;
    }
    /* If this is a polyinstantiated object, find the right instance */
    else if (obj->poly) {
        rc = SELinuxSelectionToSID(name, subj, &tsid, NULL);
        if (rc != Success) {
            rec->status = rc;
            return;
        }
        while (pSel->selection != name || obj->sid != tsid) {
            if ((pSel = pSel->next) == NULL)
                break;
            obj = dixLookupPrivate(&pSel->devPrivates, objectKey);
        }

        if (pSel)
            *rec->ppSel = pSel;
        else {
            rec->status = BadMatch;
            return;
        }
    }

    /* Perform the security check */
    rc = SELinuxDoCheck(subj, obj, SECCLASS_X_SELECTION, access_mode,
                        &auditdata);
    if (rc != Success)
        rec->status = rc;

    /* Label the content (advisory only) */
    if (access_mode & DixSetAttrAccess) {
        data = dixLookupPrivate(&pSel->devPrivates, dataKey);
        if (subj->sel_create_sid)
            data->sid = subj->sel_create_sid;
        else
            data->sid = obj->sid;
    }
}

static void
SELinuxProperty(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XacePropertyAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj, *data;
    PropertyPtr pProp = *rec->ppProp;
    Atom name = pProp->propertyName;
    SELinuxAuditRec auditdata = {.client = rec->client,.property = name };
    security_id_t tsid;
    int rc;

    /* Don't care about the new content check */
    if (rec->access_mode & DixPostAccess)
        return;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&pProp->devPrivates, objectKey);

    /* If this is a new object that needs labeling, do it now */
    if (rec->access_mode & DixCreateAccess) {
        rc = SELinuxPropertyToSID(name, subj, &obj->sid, &obj->poly);
        if (rc != Success) {
            rec->status = rc;
            return;
        }
    }
    /* If this is a polyinstantiated object, find the right instance */
    else if (obj->poly) {
        rc = SELinuxPropertyToSID(name, subj, &tsid, NULL);
        if (rc != Success) {
            rec->status = rc;
            return;
        }
        while (pProp->propertyName != name || obj->sid != tsid) {
            if ((pProp = pProp->next) == NULL)
                break;
            obj = dixLookupPrivate(&pProp->devPrivates, objectKey);
        }

        if (pProp)
            *rec->ppProp = pProp;
        else {
            rec->status = BadMatch;
            return;
        }
    }

    /* Perform the security check */
    rc = SELinuxDoCheck(subj, obj, SECCLASS_X_PROPERTY, rec->access_mode,
                        &auditdata);
    if (rc != Success)
        rec->status = rc;

    /* Label the content (advisory only) */
    if (rec->access_mode & DixWriteAccess) {
        data = dixLookupPrivate(&pProp->devPrivates, dataKey);
        if (subj->prp_create_sid)
            data->sid = subj->prp_create_sid;
        else
            data->sid = obj->sid;
    }
}

static void
SELinuxResource(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceResourceAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    SELinuxAuditRec auditdata = {.client = rec->client };
    Mask access_mode = rec->access_mode;
    PrivateRec **privatePtr;
    security_class_t class;
    int rc, offset;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);

    /* Determine if the resource object has a devPrivates field */
    offset = dixLookupPrivateOffset(rec->rtype);
    if (offset < 0) {
        /* No: use the SID of the owning client */
        class = SECCLASS_X_RESOURCE;
        privatePtr = &clients[CLIENT_ID(rec->id)]->devPrivates;
        obj = dixLookupPrivate(privatePtr, objectKey);
    }
    else {
        /* Yes: use the SID from the resource object itself */
        class = SELinuxTypeToClass(rec->rtype);
        privatePtr = DEVPRIV_AT(rec->res, offset);
        obj = dixLookupPrivate(privatePtr, objectKey);
    }

    /* If this is a new object that needs labeling, do it now */
    if (access_mode & DixCreateAccess && offset >= 0) {
        rc = SELinuxLabelResource(rec, subj, obj, class);
        if (rc != Success) {
            rec->status = rc;
            return;
        }
    }

    /* Collapse generic resource permissions down to read/write */
    if (class == SECCLASS_X_RESOURCE) {
        access_mode = ! !(rec->access_mode & SELinuxReadMask);  /* rd */
        access_mode |= ! !(rec->access_mode & ~SELinuxReadMask) << 1;   /* wr */
    }

    /* Perform the security check */
    auditdata.restype = rec->rtype;
    auditdata.id = rec->id;
    rc = SELinuxDoCheck(subj, obj, class, access_mode, &auditdata);
    if (rc != Success)
        rec->status = rc;

    /* Perform the background none check on windows */
    if (access_mode & DixCreateAccess && rec->rtype == RT_WINDOW) {
        rc = SELinuxDoCheck(subj, obj, class, DixBlendAccess, &auditdata);
        if (rc != Success)
            ((WindowPtr) rec->res)->forcedBG = TRUE;
    }
}

static void
SELinuxScreen(CallbackListPtr *pcbl, void *is_saver, void *calldata)
{
    XaceScreenAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    SELinuxAuditRec auditdata = {.client = rec->client };
    Mask access_mode = rec->access_mode;
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&rec->screen->devPrivates, objectKey);

    /* If this is a new object that needs labeling, do it now */
    if (access_mode & DixCreateAccess) {
        /* Perform a transition to obtain the final SID */
        if (avc_compute_create(subj->sid, subj->sid, SECCLASS_X_SCREEN,
                               &obj->sid) < 0) {
            ErrorF("SELinux: a compute_create call failed!\n");
            rec->status = BadValue;
            return;
        }
    }

    if (is_saver)
        access_mode <<= 2;

    rc = SELinuxDoCheck(subj, obj, SECCLASS_X_SCREEN, access_mode, &auditdata);
    if (rc != Success)
        rec->status = rc;
}

static void
SELinuxClient(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceClientAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    SELinuxAuditRec auditdata = {.client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&rec->target->devPrivates, objectKey);

    rc = SELinuxDoCheck(subj, obj, SECCLASS_X_CLIENT, rec->access_mode,
                        &auditdata);
    if (rc != Success)
        rec->status = rc;
}

static void
SELinuxServer(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceServerAccessRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    SELinuxAuditRec auditdata = {.client = rec->client };
    int rc;

    subj = dixLookupPrivate(&rec->client->devPrivates, subjectKey);
    obj = dixLookupPrivate(&serverClient->devPrivates, objectKey);

    rc = SELinuxDoCheck(subj, obj, SECCLASS_X_SERVER, rec->access_mode,
                        &auditdata);
    if (rc != Success)
        rec->status = rc;
}

/*
 * DIX Callbacks
 */

static void
SELinuxClientState(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    NewClientInfoRec *pci = calldata;

    switch (pci->client->clientState) {
    case ClientStateInitial:
        SELinuxLabelClient(pci->client);
        break;

    default:
        break;
    }
}

static void
SELinuxResourceState(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    ResourceStateInfoRec *rec = calldata;
    SELinuxSubjectRec *subj;
    SELinuxObjectRec *obj;
    WindowPtr pWin;

    if (rec->type != RT_WINDOW)
        return;
    if (rec->state != ResourceStateAdding)
        return;

    pWin = (WindowPtr) rec->value;
    subj = dixLookupPrivate(&wClient(pWin)->devPrivates, subjectKey);

    if (subj->sid) {
        char *ctx;
        int rc = avc_sid_to_context_raw(subj->sid, &ctx);

        if (rc < 0)
            FatalError("SELinux: Failed to get security context!\n");
        rc = dixChangeWindowProperty(serverClient,
                                     pWin, atom_client_ctx, XA_STRING, 8,
                                     PropModeReplace, strlen(ctx), ctx, FALSE);
        if (rc != Success)
            FatalError("SELinux: Failed to set label property on window!\n");
        freecon(ctx);
    }
    else
        FatalError("SELinux: Unexpected unlabeled client found\n");

    obj = dixLookupPrivate(&pWin->devPrivates, objectKey);

    if (obj->sid) {
        char *ctx;
        int rc = avc_sid_to_context_raw(obj->sid, &ctx);

        if (rc < 0)
            FatalError("SELinux: Failed to get security context!\n");
        rc = dixChangeWindowProperty(serverClient,
                                     pWin, atom_ctx, XA_STRING, 8,
                                     PropModeReplace, strlen(ctx), ctx, FALSE);
        if (rc != Success)
            FatalError("SELinux: Failed to set label property on window!\n");
        freecon(ctx);
    }
    else
        FatalError("SELinux: Unexpected unlabeled window found\n");
}

static int netlink_fd;

static void
SELinuxNetlinkNotify(int fd, int ready, void *data)
{
    avc_netlink_check_nb();
}

void
SELinuxFlaskReset(void)
{
    /* Unregister callbacks */
    DeleteCallback(&ClientStateCallback, SELinuxClientState, NULL);
    DeleteCallback(&ResourceStateCallback, SELinuxResourceState, NULL);

    XaceDeleteCallback(XACE_EXT_DISPATCH, SELinuxExtension, NULL);
    XaceDeleteCallback(XACE_RESOURCE_ACCESS, SELinuxResource, NULL);
    XaceDeleteCallback(XACE_DEVICE_ACCESS, SELinuxDevice, NULL);
    XaceDeleteCallback(XACE_PROPERTY_ACCESS, SELinuxProperty, NULL);
    XaceDeleteCallback(XACE_SEND_ACCESS, SELinuxSend, NULL);
    XaceDeleteCallback(XACE_RECEIVE_ACCESS, SELinuxReceive, NULL);
    XaceDeleteCallback(XACE_CLIENT_ACCESS, SELinuxClient, NULL);
    XaceDeleteCallback(XACE_EXT_ACCESS, SELinuxExtension, NULL);
    XaceDeleteCallback(XACE_SERVER_ACCESS, SELinuxServer, NULL);
    XaceDeleteCallback(XACE_SELECTION_ACCESS, SELinuxSelection, NULL);
    XaceDeleteCallback(XACE_SCREEN_ACCESS, SELinuxScreen, NULL);
    XaceDeleteCallback(XACE_SCREENSAVER_ACCESS, SELinuxScreen, truep);

    /* Tear down SELinux stuff */
    audit_close(audit_fd);
    avc_netlink_release_fd();
    RemoveNotifyFd(netlink_fd);

    avc_destroy();
}

void
SELinuxFlaskInit(void)
{
    struct selinux_opt avc_option = { AVC_OPT_SETENFORCE, (char *) 0 };
    char *ctx;
    int ret = TRUE;

    switch (selinuxEnforcingState) {
    case SELINUX_MODE_ENFORCING:
        LogMessage(X_INFO, "SELinux: Configured in enforcing mode\n");
        avc_option.value = (char *) 1;
        break;
    case SELINUX_MODE_PERMISSIVE:
        LogMessage(X_INFO, "SELinux: Configured in permissive mode\n");
        avc_option.value = (char *) 0;
        break;
    default:
        avc_option.type = AVC_OPT_UNUSED;
        break;
    }

    /* Set up SELinux stuff */
    selinux_set_callback(SELINUX_CB_LOG, (union selinux_callback) SELinuxLog);
    selinux_set_callback(SELINUX_CB_AUDIT,
                         (union selinux_callback) SELinuxAudit);

    if (selinux_set_mapping(map) < 0) {
        if (errno == EINVAL) {
            ErrorF
                ("SELinux: Invalid object class mapping, disabling SELinux support.\n");
            return;
        }
        FatalError("SELinux: Failed to set up security class mapping\n");
    }

    if (avc_open(&avc_option, 1) < 0)
        FatalError("SELinux: Couldn't initialize SELinux userspace AVC\n");

    if (security_get_initial_context_raw("unlabeled", &ctx) < 0)
        FatalError("SELinux: Failed to look up unlabeled context\n");
    if (avc_context_to_sid_raw(ctx, &unlabeled_sid) < 0)
        FatalError("SELinux: a context_to_SID call failed!\n");
    freecon(ctx);

    /* Prepare for auditing */
    audit_fd = audit_open();
    if (audit_fd < 0)
        FatalError("SELinux: Failed to open the system audit log\n");

    /* Allocate private storage */
    if (!dixRegisterPrivateKey
        (subjectKey, PRIVATE_XSELINUX, sizeof(SELinuxSubjectRec)) ||
        !dixRegisterPrivateKey(objectKey, PRIVATE_XSELINUX,
                               sizeof(SELinuxObjectRec)) ||
        !dixRegisterPrivateKey(dataKey, PRIVATE_XSELINUX,
                               sizeof(SELinuxObjectRec)))
        FatalError("SELinux: Failed to allocate private storage.\n");

    /* Create atoms for doing window labeling */
    atom_ctx = MakeAtom("_SELINUX_CONTEXT", 16, TRUE);
    if (atom_ctx == BAD_RESOURCE)
        FatalError("SELinux: Failed to create atom\n");
    atom_client_ctx = MakeAtom("_SELINUX_CLIENT_CONTEXT", 23, TRUE);
    if (atom_client_ctx == BAD_RESOURCE)
        FatalError("SELinux: Failed to create atom\n");

    netlink_fd = avc_netlink_acquire_fd();
    SetNotifyFd(netlink_fd, SELinuxNetlinkNotify, X_NOTIFY_READ, NULL);

    /* Register callbacks */
    ret &= AddCallback(&ClientStateCallback, SELinuxClientState, NULL);
    ret &= AddCallback(&ResourceStateCallback, SELinuxResourceState, NULL);

    ret &= XaceRegisterCallback(XACE_EXT_DISPATCH, SELinuxExtension, NULL);
    ret &= XaceRegisterCallback(XACE_RESOURCE_ACCESS, SELinuxResource, NULL);
    ret &= XaceRegisterCallback(XACE_DEVICE_ACCESS, SELinuxDevice, NULL);
    ret &= XaceRegisterCallback(XACE_PROPERTY_ACCESS, SELinuxProperty, NULL);
    ret &= XaceRegisterCallback(XACE_SEND_ACCESS, SELinuxSend, NULL);
    ret &= XaceRegisterCallback(XACE_RECEIVE_ACCESS, SELinuxReceive, NULL);
    ret &= XaceRegisterCallback(XACE_CLIENT_ACCESS, SELinuxClient, NULL);
    ret &= XaceRegisterCallback(XACE_EXT_ACCESS, SELinuxExtension, NULL);
    ret &= XaceRegisterCallback(XACE_SERVER_ACCESS, SELinuxServer, NULL);
    ret &= XaceRegisterCallback(XACE_SELECTION_ACCESS, SELinuxSelection, NULL);
    ret &= XaceRegisterCallback(XACE_SCREEN_ACCESS, SELinuxScreen, NULL);
    ret &= XaceRegisterCallback(XACE_SCREENSAVER_ACCESS, SELinuxScreen, truep);
    if (!ret)
        FatalError("SELinux: Failed to register one or more callbacks\n");

    /* Label objects that were created before we could register ourself */
    SELinuxLabelInitial();
}
