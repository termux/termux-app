/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>
#include "glxserver.h"
#include <windowstr.h>
#include <propertyst.h>
#include <registry.h>
#include "privates.h"
#include <os.h>
#include "extinit.h"
#include "glx_extinit.h"
#include "unpack.h"
#include "glxutil.h"
#include "glxext.h"
#include "indirect_table.h"
#include "indirect_util.h"
#include "glxvndabi.h"

/*
** X resources.
*/
static int glxGeneration;
RESTYPE __glXContextRes;
RESTYPE __glXDrawableRes;

static DevPrivateKeyRec glxClientPrivateKeyRec;
static GlxServerVendor *glvnd_vendor = NULL;

#define glxClientPrivateKey (&glxClientPrivateKeyRec)

/*
** Forward declarations.
*/
static int __glXDispatch(ClientPtr);
static GLboolean __glXFreeContext(__GLXcontext * cx);

/*
 * This procedure is called when the client who created the context goes away
 * OR when glXDestroyContext is called. If the context is current for a client
 * the dispatch layer will have moved the context struct to a fake resource ID
 * and cx here will be NULL. Otherwise we really free the context.
 */
static int
ContextGone(__GLXcontext * cx, XID id)
{
    if (!cx)
        return TRUE;

    if (!cx->currentClient)
        __glXFreeContext(cx);

    return TRUE;
}

static __GLXcontext *glxPendingDestroyContexts;
static __GLXcontext *glxAllContexts;
static int glxBlockClients;

/*
** Destroy routine that gets called when a drawable is freed.  A drawable
** contains the ancillary buffers needed for rendering.
*/
static Bool
DrawableGone(__GLXdrawable * glxPriv, XID xid)
{
    __GLXcontext *c, *next;

    if (glxPriv->type == GLX_DRAWABLE_WINDOW) {
        /* If this was created by glXCreateWindow, free the matching resource */
        if (glxPriv->drawId != glxPriv->pDraw->id) {
            if (xid == glxPriv->drawId)
                FreeResourceByType(glxPriv->pDraw->id, __glXDrawableRes, TRUE);
            else
                FreeResourceByType(glxPriv->drawId, __glXDrawableRes, TRUE);
        }
        /* otherwise this window was implicitly created by MakeCurrent */
    }

    for (c = glxAllContexts; c; c = next) {
        next = c->next;
        if (c->currentClient &&
		(c->drawPriv == glxPriv || c->readPriv == glxPriv)) {
            /* flush the context */
            glFlush();
            /* just force a re-bind the next time through */
            (*c->loseCurrent) (c);
            lastGLContext = NULL;
        }
        if (c->drawPriv == glxPriv)
            c->drawPriv = NULL;
        if (c->readPriv == glxPriv)
            c->readPriv = NULL;
    }

    /* drop our reference to any backing pixmap */
    if (glxPriv->type == GLX_DRAWABLE_PIXMAP)
        glxPriv->pDraw->pScreen->DestroyPixmap((PixmapPtr) glxPriv->pDraw);

    glxPriv->destroy(glxPriv);

    return TRUE;
}

Bool
__glXAddContext(__GLXcontext * cx)
{
    /* Register this context as a resource.
     */
    if (!AddResource(cx->id, __glXContextRes, (void *)cx)) {
	return FALSE;
    }

    cx->next = glxAllContexts;
    glxAllContexts = cx;
    return TRUE;
}

static void
__glXRemoveFromContextList(__GLXcontext * cx)
{
    __GLXcontext *c, *prev;

    if (cx == glxAllContexts)
        glxAllContexts = cx->next;
    else {
        prev = glxAllContexts;
        for (c = glxAllContexts; c; c = c->next) {
            if (c == cx)
                prev->next = c->next;
            prev = c;
        }
    }
}

/*
** Free a context.
*/
static GLboolean
__glXFreeContext(__GLXcontext * cx)
{
    if (cx->idExists || cx->currentClient)
        return GL_FALSE;

    __glXRemoveFromContextList(cx);

    free(cx->feedbackBuf);
    free(cx->selectBuf);
    free(cx->largeCmdBuf);
    if (cx == lastGLContext) {
        lastGLContext = NULL;
    }

    /* We can get here through both regular dispatching from
     * __glXDispatch() or as a callback from the resource manager.  In
     * the latter case we need to lift the DRI lock manually. */

    if (!glxBlockClients) {
        cx->destroy(cx);
    }
    else {
        cx->next = glxPendingDestroyContexts;
        glxPendingDestroyContexts = cx;
    }

    return GL_TRUE;
}

/************************************************************************/

/*
** These routines can be used to check whether a particular GL command
** has caused an error.  Specifically, we use them to check whether a
** given query has caused an error, in which case a zero-length data
** reply is sent to the client.
*/

static GLboolean errorOccured = GL_FALSE;

/*
** The GL was will call this routine if an error occurs.
*/
void
__glXErrorCallBack(GLenum code)
{
    errorOccured = GL_TRUE;
}

/*
** Clear the error flag before calling the GL command.
*/
void
__glXClearErrorOccured(void)
{
    errorOccured = GL_FALSE;
}

/*
** Check if the GL command caused an error.
*/
GLboolean
__glXErrorOccured(void)
{
    return errorOccured;
}

static int __glXErrorBase;
int __glXEventBase;

int
__glXError(int error)
{
    return __glXErrorBase + error;
}

__GLXclientState *
glxGetClient(ClientPtr pClient)
{
    return dixLookupPrivate(&pClient->devPrivates, glxClientPrivateKey);
}

static void
glxClientCallback(CallbackListPtr *list, void *closure, void *data)
{
    NewClientInfoRec *clientinfo = (NewClientInfoRec *) data;
    ClientPtr pClient = clientinfo->client;
    __GLXclientState *cl = glxGetClient(pClient);

    switch (pClient->clientState) {
    case ClientStateGone:
        free(cl->returnBuf);
        free(cl->GLClientextensions);
        cl->returnBuf = NULL;
        cl->GLClientextensions = NULL;
        break;

    default:
        break;
    }
}

/************************************************************************/

static __GLXprovider *__glXProviderStack = &__glXDRISWRastProvider;

void
GlxPushProvider(__GLXprovider * provider)
{
    provider->next = __glXProviderStack;
    __glXProviderStack = provider;
}

static Bool
checkScreenVisuals(void)
{
    int i, j;

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr screen = screenInfo.screens[i];
        for (j = 0; j < screen->numVisuals; j++) {
            if ((screen->visuals[j].class == TrueColor ||
                 screen->visuals[j].class == DirectColor) &&
                screen->visuals[j].nplanes > 12)
                return TRUE;
        }
    }

    return FALSE;
}

static void
GetGLXDrawableBytes(void *value, XID id, ResourceSizePtr size)
{
    __GLXdrawable *draw = value;

    size->resourceSize = 0;
    size->pixmapRefSize = 0;
    size->refCnt = 1;

    if (draw->type == GLX_DRAWABLE_PIXMAP) {
        SizeType pixmapSizeFunc = GetResourceTypeSizeFunc(RT_PIXMAP);
        ResourceSizeRec pixmapSize = { 0, };
        pixmapSizeFunc((PixmapPtr)draw->pDraw, draw->pDraw->id, &pixmapSize);
        size->pixmapRefSize += pixmapSize.pixmapRefSize;
    }
}

static void
xorgGlxCloseExtension(const ExtensionEntry *extEntry)
{
    if (glvnd_vendor != NULL) {
        glxServer.destroyVendor(glvnd_vendor);
        glvnd_vendor = NULL;
    }
    lastGLContext = NULL;
}

static int
xorgGlxHandleRequest(ClientPtr client)
{
    return __glXDispatch(client);
}

static ScreenPtr
screenNumToScreen(int screen)
{
    if (screen < 0 || screen >= screenInfo.numScreens)
        return NULL;

    return screenInfo.screens[screen];
}

static int
maybe_swap32(ClientPtr client, int x)
{
    return client->swapped ? bswap_32(x) : x;
}

static GlxServerVendor *
vendorForScreen(ClientPtr client, int screen)
{
    screen = maybe_swap32(client, screen);

    return glxServer.getVendorForScreen(client, screenNumToScreen(screen));
}

/* this ought to be generated */
static int
xorgGlxThunkRequest(ClientPtr client)
{
    REQUEST(xGLXVendorPrivateReq);
    CARD32 vendorCode = maybe_swap32(client, stuff->vendorCode);
    GlxServerVendor *vendor = NULL;
    XID resource = 0;
    int ret;

    switch (vendorCode) {
    case X_GLXvop_QueryContextInfoEXT: {
        xGLXQueryContextInfoEXTReq *req = (void *)stuff;
        REQUEST_AT_LEAST_SIZE(*req);
        if (!(vendor = glxServer.getXIDMap(maybe_swap32(client, req->context))))
            return __glXError(GLXBadContext);
        break;
        }

    case X_GLXvop_GetFBConfigsSGIX: {
        xGLXGetFBConfigsSGIXReq *req = (void *)stuff;
        REQUEST_AT_LEAST_SIZE(*req);
        if (!(vendor = vendorForScreen(client, req->screen)))
            return BadValue;
        break;
        }

    case X_GLXvop_CreateContextWithConfigSGIX: {
        xGLXCreateContextWithConfigSGIXReq *req = (void *)stuff;
        REQUEST_AT_LEAST_SIZE(*req);
        resource = maybe_swap32(client, req->context);
        if (!(vendor = vendorForScreen(client, req->screen)))
            return BadValue;
        break;
        }

    case X_GLXvop_CreateGLXPixmapWithConfigSGIX: {
        xGLXCreateGLXPixmapWithConfigSGIXReq *req = (void *)stuff;
        REQUEST_AT_LEAST_SIZE(*req);
        resource = maybe_swap32(client, req->glxpixmap);
        if (!(vendor = vendorForScreen(client, req->screen)))
            return BadValue;
        break;
        }

    case X_GLXvop_CreateGLXPbufferSGIX: {
        xGLXCreateGLXPbufferSGIXReq *req = (void *)stuff;
        REQUEST_AT_LEAST_SIZE(*req);
        resource = maybe_swap32(client, req->pbuffer);
        if (!(vendor = vendorForScreen(client, req->screen)))
            return BadValue;
        break;
        }

    /* same offset for the drawable for these three */
    case X_GLXvop_DestroyGLXPbufferSGIX:
    case X_GLXvop_ChangeDrawableAttributesSGIX:
    case X_GLXvop_GetDrawableAttributesSGIX: {
        xGLXGetDrawableAttributesSGIXReq *req = (void *)stuff;
        REQUEST_AT_LEAST_SIZE(*req);
        if (!(vendor = glxServer.getXIDMap(maybe_swap32(client,
                                                        req->drawable))))
            return __glXError(GLXBadDrawable);
        break;
        }

    /* most things just use the standard context tag */
    default: {
        /* size checked by vnd layer already */
        GLXContextTag tag = maybe_swap32(client, stuff->contextTag);
        vendor = glxServer.getContextTag(client, tag);
        if (!vendor)
            return __glXError(GLXBadContextTag);
        break;
        }
    }

    /* If we're creating a resource, add the map now */
    if (resource) {
        LEGAL_NEW_RESOURCE(resource, client);
        if (!glxServer.addXIDMap(resource, vendor))
            return BadAlloc;
    }

    ret = glxServer.forwardRequest(vendor, client);

    if (ret == Success && vendorCode == X_GLXvop_DestroyGLXPbufferSGIX) {
        xGLXDestroyGLXPbufferSGIXReq *req = (void *)stuff;
        glxServer.removeXIDMap(maybe_swap32(client, req->pbuffer));
    }

    if (ret != Success)
        glxServer.removeXIDMap(resource);

    return ret;
}

static GlxServerDispatchProc
xorgGlxGetDispatchAddress(CARD8 minorOpcode, CARD32 vendorCode)
{
    /* we don't support any other GLX opcodes */
    if (minorOpcode != X_GLXVendorPrivate &&
        minorOpcode != X_GLXVendorPrivateWithReply)
        return NULL;

    /* we only support some vendor private requests */
    if (!__glXGetProtocolDecodeFunction(&VendorPriv_dispatch_info, vendorCode,
                                        FALSE))
        return NULL;

    return xorgGlxThunkRequest;
}

static Bool
xorgGlxServerPreInit(const ExtensionEntry *extEntry)
{
    if (glxGeneration != serverGeneration) {
        /* Mesa requires at least one True/DirectColor visual */
        if (!checkScreenVisuals())
            return FALSE;

        __glXContextRes = CreateNewResourceType((DeleteType) ContextGone,
                                                "GLXContext");
        __glXDrawableRes = CreateNewResourceType((DeleteType) DrawableGone,
                                                 "GLXDrawable");
        if (!__glXContextRes || !__glXDrawableRes)
            return FALSE;

        if (!dixRegisterPrivateKey
            (&glxClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(__GLXclientState)))
            return FALSE;
        if (!AddCallback(&ClientStateCallback, glxClientCallback, 0))
            return FALSE;

        __glXErrorBase = extEntry->errorBase;
        __glXEventBase = extEntry->eventBase;

        SetResourceTypeSizeFunc(__glXDrawableRes, GetGLXDrawableBytes);
#if PRESENT
        __glXregisterPresentCompleteNotify();
#endif

        glxGeneration = serverGeneration;
    }

    return glxGeneration == serverGeneration;
}

static void
xorgGlxInitGLVNDVendor(void)
{
    if (glvnd_vendor == NULL) {
        GlxServerImports *imports = NULL;
        imports = glxServer.allocateServerImports();

        if (imports != NULL) {
            imports->extensionCloseDown = xorgGlxCloseExtension;
            imports->handleRequest = xorgGlxHandleRequest;
            imports->getDispatchAddress = xorgGlxGetDispatchAddress;
            imports->makeCurrent = xorgGlxMakeCurrent;
            glvnd_vendor = glxServer.createVendor(imports);
            glxServer.freeServerImports(imports);
        }
    }
}

static void
xorgGlxServerInit(CallbackListPtr *pcbl, void *param, void *ext)
{
    const ExtensionEntry *extEntry = ext;
    int i;

    if (!xorgGlxServerPreInit(extEntry)) {
        return;
    }

    xorgGlxInitGLVNDVendor();
    if (!glvnd_vendor) {
        return;
    }

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];
        __GLXprovider *p;

        if (glxServer.getVendorForScreen(NULL, pScreen) != NULL) {
            // There's already a vendor registered.
            LogMessage(X_INFO, "GLX: Another vendor is already registered for screen %d\n", i);
            continue;
        }

        for (p = __glXProviderStack; p != NULL; p = p->next) {
            __GLXscreen *glxScreen = p->screenProbe(pScreen);
            if (glxScreen != NULL) {
                LogMessage(X_INFO,
                           "GLX: Initialized %s GL provider for screen %d\n",
                           p->name, i);
                break;
            }

        }

        if (p) {
            glxServer.setScreenVendor(pScreen, glvnd_vendor);
        } else {
            LogMessage(X_INFO,
                       "GLX: no usable GL providers found for screen %d\n", i);
        }
    }
}

Bool
xorgGlxCreateVendor(void)
{
    return AddCallback(glxServer.extensionInitCallback, xorgGlxServerInit, NULL);
}

/************************************************************************/

/*
** Make a context the current one for the GL (in this implementation, there
** is only one instance of the GL, and we use it to serve all GL clients by
** switching it between different contexts).  While we are at it, look up
** a context by its tag and return its (__GLXcontext *).
*/
__GLXcontext *
__glXForceCurrent(__GLXclientState * cl, GLXContextTag tag, int *error)
{
    ClientPtr client = cl->client;
    REQUEST(xGLXSingleReq);

    __GLXcontext *cx;

    /*
     ** See if the context tag is legal; it is managed by the extension,
     ** so if it's invalid, we have an implementation error.
     */
    cx = __glXLookupContextByTag(cl, tag);
    if (!cx) {
        cl->client->errorValue = tag;
        *error = __glXError(GLXBadContextTag);
        return 0;
    }

    /* If we're expecting a glXRenderLarge request, this better be one. */
    if (cx->largeCmdRequestsSoFar != 0 && stuff->glxCode != X_GLXRenderLarge) {
        client->errorValue = stuff->glxCode;
        *error = __glXError(GLXBadLargeRequest);
        return 0;
    }

    if (!cx->isDirect) {
        if (cx->drawPriv == NULL) {
            /*
             ** The drawable has vanished.  It must be a window, because only
             ** windows can be destroyed from under us; GLX pixmaps are
             ** refcounted and don't go away until no one is using them.
             */
            *error = __glXError(GLXBadCurrentWindow);
            return 0;
        }
    }

    if (cx->wait && (*cx->wait) (cx, cl, error))
        return NULL;

    if (cx == lastGLContext) {
        /* No need to re-bind */
        return cx;
    }

    /* Make this context the current one for the GL. */
    if (!cx->isDirect) {
        /*
         * If it is being forced, it means that this context was already made
         * current. So it cannot just be made current again without decrementing
         * refcount's
         */
        (*cx->loseCurrent) (cx);
        lastGLContext = cx;
        if (!(*cx->makeCurrent) (cx)) {
            /* Bind failed, and set the error code.  Bummer */
            lastGLContext = NULL;
            cl->client->errorValue = cx->id;
            *error = __glXError(GLXBadContextState);
            return 0;
        }
    }
    return cx;
}

/************************************************************************/

void
glxSuspendClients(void)
{
    int i;

    for (i = 1; i < currentMaxClients; i++) {
        if (clients[i] && glxGetClient(clients[i])->client)
            IgnoreClient(clients[i]);
    }

    glxBlockClients = TRUE;
}

void
glxResumeClients(void)
{
    __GLXcontext *cx, *next;
    int i;

    glxBlockClients = FALSE;

    for (i = 1; i < currentMaxClients; i++) {
        if (clients[i] && glxGetClient(clients[i])->client)
            AttendClient(clients[i]);
    }

    for (cx = glxPendingDestroyContexts; cx != NULL; cx = next) {
        next = cx->next;

        cx->destroy(cx);
    }
    glxPendingDestroyContexts = NULL;
}

static glx_gpa_proc _get_proc_address;

void
__glXsetGetProcAddress(glx_gpa_proc get_proc_address)
{
    _get_proc_address = get_proc_address;
}

void *__glGetProcAddress(const char *proc)
{
    void *ret = (void *) _get_proc_address(proc);

    return ret ? ret : (void *) NoopDDA;
}

/*
** Top level dispatcher; all commands are executed from here down.
*/
static int
__glXDispatch(ClientPtr client)
{
    REQUEST(xGLXSingleReq);
    CARD8 opcode;
    __GLXdispatchSingleProcPtr proc;
    __GLXclientState *cl;
    int retval = BadRequest;

    opcode = stuff->glxCode;
    cl = glxGetClient(client);


    if (!cl->client)
        cl->client = client;

    /* If we're currently blocking GLX clients, just put this guy to
     * sleep, reset the request and return. */
    if (glxBlockClients) {
        ResetCurrentRequest(client);
        client->sequence--;
        IgnoreClient(client);
        return Success;
    }

    /*
     ** Use the opcode to index into the procedure table.
     */
    proc = __glXGetProtocolDecodeFunction(&Single_dispatch_info, opcode,
                                          client->swapped);
    if (proc != NULL)
        retval = (*proc) (cl, (GLbyte *) stuff);

    return retval;
}
