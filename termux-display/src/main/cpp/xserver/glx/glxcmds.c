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
#include <assert.h>

#include "glxserver.h"
#include <GL/glxtokens.h>
#include <X11/extensions/presenttokens.h>
#include <unpack.h>
#include <pixmapstr.h>
#include <windowstr.h>
#include "glxutil.h"
#include "glxext.h"
#include "indirect_dispatch.h"
#include "indirect_table.h"
#include "indirect_util.h"
#include "protocol-versions.h"
#include "glxvndabi.h"

static char GLXServerVendorName[] = "SGI";

_X_HIDDEN int
validGlxScreen(ClientPtr client, int screen, __GLXscreen ** pGlxScreen,
               int *err)
{
    /*
     ** Check if screen exists.
     */
    if (screen < 0 || screen >= screenInfo.numScreens) {
        client->errorValue = screen;
        *err = BadValue;
        return FALSE;
    }
    *pGlxScreen = glxGetScreen(screenInfo.screens[screen]);

    return TRUE;
}

_X_HIDDEN int
validGlxFBConfig(ClientPtr client, __GLXscreen * pGlxScreen, XID id,
                 __GLXconfig ** config, int *err)
{
    __GLXconfig *m;

    for (m = pGlxScreen->fbconfigs; m != NULL; m = m->next)
        if (m->fbconfigID == id) {
            *config = m;
            return TRUE;
        }

    client->errorValue = id;
    *err = __glXError(GLXBadFBConfig);

    return FALSE;
}

static int
validGlxVisual(ClientPtr client, __GLXscreen * pGlxScreen, XID id,
               __GLXconfig ** config, int *err)
{
    int i;

    for (i = 0; i < pGlxScreen->numVisuals; i++)
        if (pGlxScreen->visuals[i]->visualID == id) {
            *config = pGlxScreen->visuals[i];
            return TRUE;
        }

    client->errorValue = id;
    *err = BadValue;

    return FALSE;
}

static int
validGlxFBConfigForWindow(ClientPtr client, __GLXconfig * config,
                          DrawablePtr pDraw, int *err)
{
    ScreenPtr pScreen = pDraw->pScreen;
    VisualPtr pVisual = NULL;
    XID vid;
    int i;

    vid = wVisual((WindowPtr) pDraw);
    for (i = 0; i < pScreen->numVisuals; i++) {
        if (pScreen->visuals[i].vid == vid) {
            pVisual = &pScreen->visuals[i];
            break;
        }
    }

    /* FIXME: What exactly should we check here... */
    if (pVisual->class != glxConvertToXVisualType(config->visualType) ||
        !(config->drawableType & GLX_WINDOW_BIT)) {
        client->errorValue = pDraw->id;
        *err = BadMatch;
        return FALSE;
    }

    return TRUE;
}

_X_HIDDEN int
validGlxContext(ClientPtr client, XID id, int access_mode,
                __GLXcontext ** context, int *err)
{
    /* no ghost contexts */
    if (id & SERVER_BIT) {
        *err = __glXError(GLXBadContext);
        return FALSE;
    }

    *err = dixLookupResourceByType((void **) context, id,
                                   __glXContextRes, client, access_mode);
    if (*err != Success || (*context)->idExists == GL_FALSE) {
        client->errorValue = id;
        if (*err == BadValue || *err == Success)
            *err = __glXError(GLXBadContext);
        return FALSE;
    }

    return TRUE;
}

int
validGlxDrawable(ClientPtr client, XID id, int type, int access_mode,
                 __GLXdrawable ** drawable, int *err)
{
    int rc;

    rc = dixLookupResourceByType((void **) drawable, id,
                                 __glXDrawableRes, client, access_mode);
    if (rc != Success && rc != BadValue) {
        *err = rc;
        client->errorValue = id;
        return FALSE;
    }

    /* If the ID of the glx drawable we looked up doesn't match the id
     * we looked for, it's because we looked it up under the X
     * drawable ID (see DoCreateGLXDrawable). */
    if (rc == BadValue ||
        (*drawable)->drawId != id ||
        (type != GLX_DRAWABLE_ANY && type != (*drawable)->type)) {
        client->errorValue = id;
        switch (type) {
        case GLX_DRAWABLE_WINDOW:
            *err = __glXError(GLXBadWindow);
            return FALSE;
        case GLX_DRAWABLE_PIXMAP:
            *err = __glXError(GLXBadPixmap);
            return FALSE;
        case GLX_DRAWABLE_PBUFFER:
            *err = __glXError(GLXBadPbuffer);
            return FALSE;
        case GLX_DRAWABLE_ANY:
            *err = __glXError(GLXBadDrawable);
            return FALSE;
        }
    }

    return TRUE;
}

void
__glXContextDestroy(__GLXcontext * context)
{
    lastGLContext = NULL;
}

static void
__glXdirectContextDestroy(__GLXcontext * context)
{
    __glXContextDestroy(context);
    free(context);
}

static int
__glXdirectContextLoseCurrent(__GLXcontext * context)
{
    return GL_TRUE;
}

_X_HIDDEN __GLXcontext *
__glXdirectContextCreate(__GLXscreen * screen,
                         __GLXconfig * modes, __GLXcontext * shareContext)
{
    __GLXcontext *context;

    context = calloc(1, sizeof(__GLXcontext));
    if (context == NULL)
        return NULL;

    context->config = modes;
    context->destroy = __glXdirectContextDestroy;
    context->loseCurrent = __glXdirectContextLoseCurrent;

    return context;
}

/**
 * Create a GL context with the given properties.  This routine is used
 * to implement \c glXCreateContext, \c glXCreateNewContext, and
 * \c glXCreateContextWithConfigSGIX.  This works because of the hack way
 * that GLXFBConfigs are implemented.  Basically, the FBConfigID is the
 * same as the VisualID.
 */

static int
DoCreateContext(__GLXclientState * cl, GLXContextID gcId,
                GLXContextID shareList, __GLXconfig * config,
                __GLXscreen * pGlxScreen, GLboolean isDirect,
                int renderType)
{
    ClientPtr client = cl->client;
    __GLXcontext *glxc, *shareglxc;
    int err;

    /*
     ** Find the display list space that we want to share.
     **
     ** NOTE: In a multithreaded X server, we would need to keep a reference
     ** count for each display list so that if one client destroyed a list that
     ** another client was using, the list would not really be freed until it
     ** was no longer in use.  Since this sample implementation has no support
     ** for multithreaded servers, we don't do this.
     */
    if (shareList == None) {
        shareglxc = 0;
    }
    else {
        if (!validGlxContext(client, shareList, DixReadAccess,
                             &shareglxc, &err))
            return err;

        /* Page 26 (page 32 of the PDF) of the GLX 1.4 spec says:
         *
         *     "The server context state for all sharing contexts must exist
         *     in a single address space or a BadMatch error is generated."
         *
         * If the share context is indirect, force the new context to also be
         * indirect.  If the shard context is direct but the new context
         * cannot be direct, generate BadMatch.
         */
        if (shareglxc->isDirect && !isDirect) {
            client->errorValue = shareList;
            return BadMatch;
        }
        else if (!shareglxc->isDirect) {
            /*
             ** Create an indirect context regardless of what the client asked
             ** for; this way we can share display list space with shareList.
             */
            isDirect = GL_FALSE;
        }

        /* Core GLX doesn't explicitly require this, but GLX_ARB_create_context
         * does (see glx/createcontext.c), and it's assumed by our
         * implementation anyway, so let's be consistent about it.
         */
        if (shareglxc->pGlxScreen != pGlxScreen) {
            client->errorValue = shareglxc->pGlxScreen->pScreen->myNum;
            return BadMatch;
        }
    }

    /*
     ** Allocate memory for the new context
     */
    if (!isDirect) {
        /* Only allow creating indirect GLX contexts if allowed by
         * server command line.  Indirect GLX is of limited use (since
         * it's only GL 1.4), it's slower than direct contexts, and
         * it's a massive attack surface for buffer overflow type
         * errors.
         */
        if (!enableIndirectGLX) {
            client->errorValue = isDirect;
            return BadValue;
        }

        /* Without any attributes, the only error that the driver should be
         * able to generate is BadAlloc.  As result, just drop the error
         * returned from the driver on the floor.
         */
        glxc = pGlxScreen->createContext(pGlxScreen, config, shareglxc,
                                         0, NULL, &err);
    }
    else
        glxc = __glXdirectContextCreate(pGlxScreen, config, shareglxc);
    if (!glxc) {
        return BadAlloc;
    }

    /* Initialize the GLXcontext structure.
     */
    glxc->pGlxScreen = pGlxScreen;
    glxc->config = config;
    glxc->id = gcId;
    glxc->share_id = shareList;
    glxc->idExists = GL_TRUE;
    glxc->isDirect = isDirect;
    glxc->renderMode = GL_RENDER;
    glxc->renderType = renderType;

    /* The GLX_ARB_create_context_robustness spec says:
     *
     *     "The default value for GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB
     *     is GLX_NO_RESET_NOTIFICATION_ARB."
     *
     * Without using glXCreateContextAttribsARB, there is no way to specify a
     * non-default reset notification strategy.
     */
    glxc->resetNotificationStrategy = GLX_NO_RESET_NOTIFICATION_ARB;

#ifdef GLX_CONTEXT_RELEASE_BEHAVIOR_ARB
    /* The GLX_ARB_context_flush_control spec says:
     *
     *     "The default value [for GLX_CONTEXT_RELEASE_BEHAVIOR] is
     *     CONTEXT_RELEASE_BEHAVIOR_FLUSH, and may in some cases be changed
     *     using platform-specific context creation extensions."
     *
     * Without using glXCreateContextAttribsARB, there is no way to specify a
     * non-default release behavior.
     */
    glxc->releaseBehavior = GLX_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB;
#endif

    /* Add the new context to the various global tables of GLX contexts.
     */
    if (!__glXAddContext(glxc)) {
        (*glxc->destroy) (glxc);
        client->errorValue = gcId;
        return BadAlloc;
    }

    return Success;
}

int
__glXDisp_CreateContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXCreateContextReq *req = (xGLXCreateContextReq *) pc;
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    int err;

    if (!validGlxScreen(cl->client, req->screen, &pGlxScreen, &err))
        return err;
    if (!validGlxVisual(cl->client, pGlxScreen, req->visual, &config, &err))
        return err;

    return DoCreateContext(cl, req->context, req->shareList,
                           config, pGlxScreen, req->isDirect,
                           GLX_RGBA_TYPE);
}

int
__glXDisp_CreateNewContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXCreateNewContextReq *req = (xGLXCreateNewContextReq *) pc;
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    int err;

    if (!validGlxScreen(cl->client, req->screen, &pGlxScreen, &err))
        return err;
    if (!validGlxFBConfig(cl->client, pGlxScreen, req->fbconfig, &config, &err))
        return err;

    return DoCreateContext(cl, req->context, req->shareList,
                           config, pGlxScreen, req->isDirect,
                           req->renderType);
}

int
__glXDisp_CreateContextWithConfigSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreateContextWithConfigSGIXReq *req =
        (xGLXCreateContextWithConfigSGIXReq *) pc;
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    int err;

    REQUEST_SIZE_MATCH(xGLXCreateContextWithConfigSGIXReq);

    if (!validGlxScreen(cl->client, req->screen, &pGlxScreen, &err))
        return err;
    if (!validGlxFBConfig(cl->client, pGlxScreen, req->fbconfig, &config, &err))
        return err;

    return DoCreateContext(cl, req->context, req->shareList,
                           config, pGlxScreen, req->isDirect,
                           req->renderType);
}

int
__glXDisp_DestroyContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXDestroyContextReq *req = (xGLXDestroyContextReq *) pc;
    __GLXcontext *glxc;
    int err;

    if (!validGlxContext(cl->client, req->context, DixDestroyAccess,
                         &glxc, &err))
        return err;

    glxc->idExists = GL_FALSE;
    if (glxc->currentClient) {
        XID ghost = FakeClientID(glxc->currentClient->index);

        if (!AddResource(ghost, __glXContextRes, glxc))
            return BadAlloc;
        ChangeResourceValue(glxc->id, __glXContextRes, NULL);
        glxc->id = ghost;
    }

    FreeResourceByType(req->context, __glXContextRes, FALSE);

    return Success;
}

__GLXcontext *
__glXLookupContextByTag(__GLXclientState * cl, GLXContextTag tag)
{
    return glxServer.getContextTagPrivate(cl->client, tag);
}

static __GLXconfig *
inferConfigForWindow(__GLXscreen *pGlxScreen, WindowPtr pWin)
{
    int i, vid = wVisual(pWin);

    for (i = 0; i < pGlxScreen->numVisuals; i++)
        if (pGlxScreen->visuals[i]->visualID == vid)
            return pGlxScreen->visuals[i];

    return NULL;
}

/**
 * This is a helper function to handle the legacy (pre GLX 1.3) cases
 * where passing an X window to glXMakeCurrent is valid.  Given a
 * resource ID, look up the GLX drawable if available, otherwise, make
 * sure it's an X window and create a GLX drawable one the fly.
 */
static __GLXdrawable *
__glXGetDrawable(__GLXcontext * glxc, GLXDrawable drawId, ClientPtr client,
                 int *error)
{
    DrawablePtr pDraw;
    __GLXdrawable *pGlxDraw;
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    int rc;

    rc = dixLookupResourceByType((void **)&pGlxDraw, drawId,
                                 __glXDrawableRes, client, DixWriteAccess);
    if (rc == Success &&
        /* If pGlxDraw->drawId == drawId, drawId is a valid GLX drawable.
         * Otherwise, if pGlxDraw->type == GLX_DRAWABLE_WINDOW, drawId is
         * an X window, but the client has already created a GLXWindow
         * associated with it, so we don't want to create another one. */
        (pGlxDraw->drawId == drawId ||
         pGlxDraw->type == GLX_DRAWABLE_WINDOW)) {
        if (glxc != NULL &&
            glxc->config != NULL &&
            glxc->config != pGlxDraw->config) {
            client->errorValue = drawId;
            *error = BadMatch;
            return NULL;
        }

        return pGlxDraw;
    }

    /* No active context and an unknown drawable, bail. */
    if (glxc == NULL) {
        client->errorValue = drawId;
        *error = BadMatch;
        return NULL;
    }

    /* The drawId wasn't a GLX drawable.  Make sure it's a window and
     * create a GLXWindow for it.  Check that the drawable screen
     * matches the context screen and that the context fbconfig is
     * compatible with the window visual. */

    rc = dixLookupDrawable(&pDraw, drawId, client, 0, DixGetAttrAccess);
    if (rc != Success || pDraw->type != DRAWABLE_WINDOW) {
        client->errorValue = drawId;
        *error = __glXError(GLXBadDrawable);
        return NULL;
    }

    pGlxScreen = glxc->pGlxScreen;
    if (pDraw->pScreen != pGlxScreen->pScreen) {
        client->errorValue = pDraw->pScreen->myNum;
        *error = BadMatch;
        return NULL;
    }

    config = glxc->config;
    if (!config)
        config = inferConfigForWindow(pGlxScreen, (WindowPtr)pDraw);
    if (!config) {
        /*
         * If we get here, we've tried to bind a no-config context to a
         * window without a corresponding fbconfig, presumably because
         * we don't support GL on it (PseudoColor perhaps). From GLX Section
         * 3.3.7 "Rendering Contexts":
         *
         * "If draw or read are not compatible with ctx a BadMatch error
         * is generated."
         */
        *error = BadMatch;
        return NULL;
    }

    if (!validGlxFBConfigForWindow(client, config, pDraw, error))
        return NULL;

    pGlxDraw = pGlxScreen->createDrawable(client, pGlxScreen, pDraw, drawId,
                                          GLX_DRAWABLE_WINDOW, drawId, config);
    if (!pGlxDraw) {
	*error = BadAlloc;
	return NULL;
    }

    /* since we are creating the drawablePrivate, drawId should be new */
    if (!AddResource(drawId, __glXDrawableRes, pGlxDraw)) {
        *error = BadAlloc;
        return NULL;
    }

    return pGlxDraw;
}

/*****************************************************************************/
/*
** Make an OpenGL context and drawable current.
*/

int
xorgGlxMakeCurrent(ClientPtr client, GLXContextTag tag, XID drawId, XID readId,
                   XID contextId, GLXContextTag newContextTag)
{
    __GLXclientState *cl = glxGetClient(client);
    __GLXcontext *glxc = NULL, *prevglxc = NULL;
    __GLXdrawable *drawPriv = NULL;
    __GLXdrawable *readPriv = NULL;
    int error;

    /* Drawables but no context makes no sense */
    if (!contextId && (drawId || readId))
        return BadMatch;

    /* If either drawable is null, the other must be too */
    if ((drawId == None) != (readId == None))
        return BadMatch;

    /* Look up old context. If we have one, it must be in a usable state. */
    if (tag != 0) {
        prevglxc = glxServer.getContextTagPrivate(client, tag);

        if (prevglxc && prevglxc->renderMode != GL_RENDER) {
            /* Oops.  Not in render mode render. */
            client->errorValue = prevglxc->id;
            return __glXError(GLXBadContextState);
        }
    }

    /* Look up new context. It must not be current for someone else. */
    if (contextId != None) {
        int status;

        if (!validGlxContext(client, contextId, DixUseAccess, &glxc, &error))
            return error;

        if ((glxc != prevglxc) && glxc->currentClient)
            return BadAccess;

        if (drawId) {
            drawPriv = __glXGetDrawable(glxc, drawId, client, &status);
            if (drawPriv == NULL)
                return status;
        }

        if (readId) {
            readPriv = __glXGetDrawable(glxc, readId, client, &status);
            if (readPriv == NULL)
                return status;
        }
    }

    if (prevglxc) {
        /* Flush the previous context if needed. */
        Bool need_flush = !prevglxc->isDirect;
#ifdef GLX_CONTEXT_RELEASE_BEHAVIOR_ARB
        if (prevglxc->releaseBehavior == GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB)
            need_flush = GL_FALSE;
#endif
        if (need_flush) {
            if (!__glXForceCurrent(cl, tag, (int *) &error))
                return error;
            glFlush();
        }

        /* Make the previous context not current. */
        if (!(*prevglxc->loseCurrent) (prevglxc))
            return __glXError(GLXBadContext);

        lastGLContext = NULL;
        if (!prevglxc->isDirect) {
            prevglxc->drawPriv = NULL;
            prevglxc->readPriv = NULL;
        }
    }

    if (glxc && !glxc->isDirect) {
        glxc->drawPriv = drawPriv;
        glxc->readPriv = readPriv;

        /* make the context current */
        lastGLContext = glxc;
        if (!(*glxc->makeCurrent) (glxc)) {
            lastGLContext = NULL;
            glxc->drawPriv = NULL;
            glxc->readPriv = NULL;
            return __glXError(GLXBadContext);
        }
    }

    glxServer.setContextTagPrivate(client, newContextTag, glxc);
    if (glxc)
        glxc->currentClient = client;

    if (prevglxc) {
        prevglxc->currentClient = NULL;
        if (!prevglxc->idExists) {
            FreeResourceByType(prevglxc->id, __glXContextRes, FALSE);
        }
    }

    return Success;
}

int
__glXDisp_MakeCurrent(__GLXclientState * cl, GLbyte * pc)
{
    return BadImplementation;
}

int
__glXDisp_MakeContextCurrent(__GLXclientState * cl, GLbyte * pc)
{
    return BadImplementation;
}

int
__glXDisp_MakeCurrentReadSGI(__GLXclientState * cl, GLbyte * pc)
{
    return BadImplementation;
}

int
__glXDisp_IsDirect(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXIsDirectReq *req = (xGLXIsDirectReq *) pc;
    xGLXIsDirectReply reply;
    __GLXcontext *glxc;
    int err;

    if (!validGlxContext(cl->client, req->context, DixReadAccess, &glxc, &err))
        return err;

    reply = (xGLXIsDirectReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .isDirect = glxc->isDirect
    };

    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
    }
    WriteToClient(client, sz_xGLXIsDirectReply, &reply);

    return Success;
}

int
__glXDisp_QueryVersion(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXQueryVersionReq *req = (xGLXQueryVersionReq *) pc;
    xGLXQueryVersionReply reply;
    GLuint major, minor;

    REQUEST_SIZE_MATCH(xGLXQueryVersionReq);

    major = req->majorVersion;
    minor = req->minorVersion;
    (void) major;
    (void) minor;

    /*
     ** Server should take into consideration the version numbers sent by the
     ** client if it wants to work with older clients; however, in this
     ** implementation the server just returns its version number.
     */
    reply = (xGLXQueryVersionReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_GLX_MAJOR_VERSION,
        .minorVersion = SERVER_GLX_MINOR_VERSION
    };

    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.majorVersion);
        __GLX_SWAP_INT(&reply.minorVersion);
    }

    WriteToClient(client, sz_xGLXQueryVersionReply, &reply);
    return Success;
}

int
__glXDisp_WaitGL(__GLXclientState * cl, GLbyte * pc)
{
    xGLXWaitGLReq *req = (xGLXWaitGLReq *) pc;
    GLXContextTag tag;
    __GLXcontext *glxc = NULL;
    int error;

    tag = req->contextTag;
    if (tag) {
        glxc = __glXLookupContextByTag(cl, tag);
        if (!glxc)
            return __glXError(GLXBadContextTag);

        if (!__glXForceCurrent(cl, req->contextTag, &error))
            return error;

        glFinish();
    }

    if (glxc && glxc->drawPriv && glxc->drawPriv->waitGL)
        (*glxc->drawPriv->waitGL) (glxc->drawPriv);

    return Success;
}

int
__glXDisp_WaitX(__GLXclientState * cl, GLbyte * pc)
{
    xGLXWaitXReq *req = (xGLXWaitXReq *) pc;
    GLXContextTag tag;
    __GLXcontext *glxc = NULL;
    int error;

    tag = req->contextTag;
    if (tag) {
        glxc = __glXLookupContextByTag(cl, tag);
        if (!glxc)
            return __glXError(GLXBadContextTag);

        if (!__glXForceCurrent(cl, req->contextTag, &error))
            return error;
    }

    if (glxc && glxc->drawPriv && glxc->drawPriv->waitX)
        (*glxc->drawPriv->waitX) (glxc->drawPriv);

    return Success;
}

int
__glXDisp_CopyContext(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCopyContextReq *req = (xGLXCopyContextReq *) pc;
    GLXContextID source;
    GLXContextID dest;
    GLXContextTag tag;
    unsigned long mask;
    __GLXcontext *src, *dst;
    int error;

    source = req->source;
    dest = req->dest;
    tag = req->contextTag;
    mask = req->mask;
    if (!validGlxContext(cl->client, source, DixReadAccess, &src, &error))
        return error;
    if (!validGlxContext(cl->client, dest, DixWriteAccess, &dst, &error))
        return error;

    /*
     ** They must be in the same address space, and same screen.
     ** NOTE: no support for direct rendering contexts here.
     */
    if (src->isDirect || dst->isDirect || (src->pGlxScreen != dst->pGlxScreen)) {
        client->errorValue = source;
        return BadMatch;
    }

    /*
     ** The destination context must not be current for any client.
     */
    if (dst->currentClient) {
        client->errorValue = dest;
        return BadAccess;
    }

    if (tag) {
        __GLXcontext *tagcx = __glXLookupContextByTag(cl, tag);

        if (!tagcx) {
            return __glXError(GLXBadContextTag);
        }
        if (tagcx != src) {
            /*
             ** This would be caused by a faulty implementation of the client
             ** library.
             */
            return BadMatch;
        }
        /*
         ** In this case, glXCopyContext is in both GL and X streams, in terms
         ** of sequentiality.
         */
        if (__glXForceCurrent(cl, tag, &error)) {
            /*
             ** Do whatever is needed to make sure that all preceding requests
             ** in both streams are completed before the copy is executed.
             */
            glFinish();
        }
        else {
            return error;
        }
    }
    /*
     ** Issue copy.  The only reason for failure is a bad mask.
     */
    if (!(*dst->copy) (dst, src, mask)) {
        client->errorValue = mask;
        return BadValue;
    }
    return Success;
}

enum {
    GLX_VIS_CONFIG_UNPAIRED = 18,
    GLX_VIS_CONFIG_PAIRED = 22
};

enum {
    GLX_VIS_CONFIG_TOTAL = GLX_VIS_CONFIG_UNPAIRED + GLX_VIS_CONFIG_PAIRED
};

int
__glXDisp_GetVisualConfigs(__GLXclientState * cl, GLbyte * pc)
{
    xGLXGetVisualConfigsReq *req = (xGLXGetVisualConfigsReq *) pc;
    ClientPtr client = cl->client;
    xGLXGetVisualConfigsReply reply;
    __GLXscreen *pGlxScreen;
    __GLXconfig *modes;
    CARD32 buf[GLX_VIS_CONFIG_TOTAL];
    int p, i, err;

    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;

    if (!validGlxScreen(cl->client, req->screen, &pGlxScreen, &err))
        return err;

    reply = (xGLXGetVisualConfigsReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = (pGlxScreen->numVisuals *
                   __GLX_SIZE_CARD32 * GLX_VIS_CONFIG_TOTAL) >> 2,
        .numVisuals = pGlxScreen->numVisuals,
        .numProps = GLX_VIS_CONFIG_TOTAL
    };

    if (client->swapped) {
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.numVisuals);
        __GLX_SWAP_INT(&reply.numProps);
    }

    WriteToClient(client, sz_xGLXGetVisualConfigsReply, &reply);

    for (i = 0; i < pGlxScreen->numVisuals; i++) {
        modes = pGlxScreen->visuals[i];

        p = 0;
        buf[p++] = modes->visualID;
        buf[p++] = glxConvertToXVisualType(modes->visualType);
        buf[p++] = (modes->renderType & GLX_RGBA_BIT) ? GL_TRUE : GL_FALSE;

        buf[p++] = modes->redBits;
        buf[p++] = modes->greenBits;
        buf[p++] = modes->blueBits;
        buf[p++] = modes->alphaBits;
        buf[p++] = modes->accumRedBits;
        buf[p++] = modes->accumGreenBits;
        buf[p++] = modes->accumBlueBits;
        buf[p++] = modes->accumAlphaBits;

        buf[p++] = modes->doubleBufferMode;
        buf[p++] = modes->stereoMode;

        buf[p++] = modes->rgbBits;
        buf[p++] = modes->depthBits;
        buf[p++] = modes->stencilBits;
        buf[p++] = modes->numAuxBuffers;
        buf[p++] = modes->level;

        assert(p == GLX_VIS_CONFIG_UNPAIRED);
        /*
         ** Add token/value pairs for extensions.
         */
        buf[p++] = GLX_VISUAL_CAVEAT_EXT;
        buf[p++] = modes->visualRating;
        buf[p++] = GLX_TRANSPARENT_TYPE;
        buf[p++] = modes->transparentPixel;
        buf[p++] = GLX_TRANSPARENT_RED_VALUE;
        buf[p++] = modes->transparentRed;
        buf[p++] = GLX_TRANSPARENT_GREEN_VALUE;
        buf[p++] = modes->transparentGreen;
        buf[p++] = GLX_TRANSPARENT_BLUE_VALUE;
        buf[p++] = modes->transparentBlue;
        buf[p++] = GLX_TRANSPARENT_ALPHA_VALUE;
        buf[p++] = modes->transparentAlpha;
        buf[p++] = GLX_TRANSPARENT_INDEX_VALUE;
        buf[p++] = modes->transparentIndex;
        buf[p++] = GLX_SAMPLES_SGIS;
        buf[p++] = modes->samples;
        buf[p++] = GLX_SAMPLE_BUFFERS_SGIS;
        buf[p++] = modes->sampleBuffers;
        buf[p++] = GLX_VISUAL_SELECT_GROUP_SGIX;
        buf[p++] = modes->visualSelectGroup;
        /* Add attribute only if its value is not default. */
        if (modes->sRGBCapable != GL_FALSE) {
            buf[p++] = GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT;
            buf[p++] = modes->sRGBCapable;
        }
        /* Pad with zeroes, so that attributes count is constant. */
        while (p < GLX_VIS_CONFIG_TOTAL) {
            buf[p++] = 0;
        }

        assert(p == GLX_VIS_CONFIG_TOTAL);
        if (client->swapped) {
            __GLX_SWAP_INT_ARRAY(buf, p);
        }
        WriteToClient(client, __GLX_SIZE_CARD32 * p, buf);
    }
    return Success;
}

#define __GLX_TOTAL_FBCONFIG_ATTRIBS (44)
#define __GLX_FBCONFIG_ATTRIBS_LENGTH (__GLX_TOTAL_FBCONFIG_ATTRIBS * 2)
/**
 * Send the set of GLXFBConfigs to the client.  There is not currently
 * and interface into the driver on the server-side to get GLXFBConfigs,
 * so we "invent" some based on the \c __GLXvisualConfig structures that
 * the driver does supply.
 *
 * The reply format for both \c glXGetFBConfigs and \c glXGetFBConfigsSGIX
 * is the same, so this routine pulls double duty.
 */

static int
DoGetFBConfigs(__GLXclientState * cl, unsigned screen)
{
    ClientPtr client = cl->client;
    xGLXGetFBConfigsReply reply;
    __GLXscreen *pGlxScreen;
    CARD32 buf[__GLX_FBCONFIG_ATTRIBS_LENGTH];
    int p, err;
    __GLXconfig *modes;

    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;

    if (!validGlxScreen(cl->client, screen, &pGlxScreen, &err))
        return err;

    reply = (xGLXGetFBConfigsReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = __GLX_FBCONFIG_ATTRIBS_LENGTH * pGlxScreen->numFBConfigs,
        .numFBConfigs = pGlxScreen->numFBConfigs,
        .numAttribs = __GLX_TOTAL_FBCONFIG_ATTRIBS
    };

    if (client->swapped) {
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.numFBConfigs);
        __GLX_SWAP_INT(&reply.numAttribs);
    }

    WriteToClient(client, sz_xGLXGetFBConfigsReply, &reply);

    for (modes = pGlxScreen->fbconfigs; modes != NULL; modes = modes->next) {
        p = 0;

#define WRITE_PAIR(tag,value) \
    do { buf[p++] = tag ; buf[p++] = value ; } while( 0 )

        WRITE_PAIR(GLX_VISUAL_ID, modes->visualID);
        WRITE_PAIR(GLX_FBCONFIG_ID, modes->fbconfigID);
        WRITE_PAIR(GLX_X_RENDERABLE,
                   (modes->drawableType & (GLX_WINDOW_BIT | GLX_PIXMAP_BIT)
                    ? GL_TRUE
                    : GL_FALSE));

        WRITE_PAIR(GLX_RGBA,
                   (modes->renderType & GLX_RGBA_BIT) ? GL_TRUE : GL_FALSE);
        WRITE_PAIR(GLX_RENDER_TYPE, modes->renderType);
        WRITE_PAIR(GLX_DOUBLEBUFFER, modes->doubleBufferMode);
        WRITE_PAIR(GLX_STEREO, modes->stereoMode);

        WRITE_PAIR(GLX_BUFFER_SIZE, modes->rgbBits);
        WRITE_PAIR(GLX_LEVEL, modes->level);
        WRITE_PAIR(GLX_AUX_BUFFERS, modes->numAuxBuffers);
        WRITE_PAIR(GLX_RED_SIZE, modes->redBits);
        WRITE_PAIR(GLX_GREEN_SIZE, modes->greenBits);
        WRITE_PAIR(GLX_BLUE_SIZE, modes->blueBits);
        WRITE_PAIR(GLX_ALPHA_SIZE, modes->alphaBits);
        WRITE_PAIR(GLX_ACCUM_RED_SIZE, modes->accumRedBits);
        WRITE_PAIR(GLX_ACCUM_GREEN_SIZE, modes->accumGreenBits);
        WRITE_PAIR(GLX_ACCUM_BLUE_SIZE, modes->accumBlueBits);
        WRITE_PAIR(GLX_ACCUM_ALPHA_SIZE, modes->accumAlphaBits);
        WRITE_PAIR(GLX_DEPTH_SIZE, modes->depthBits);
        WRITE_PAIR(GLX_STENCIL_SIZE, modes->stencilBits);
        WRITE_PAIR(GLX_X_VISUAL_TYPE, modes->visualType);
        WRITE_PAIR(GLX_CONFIG_CAVEAT, modes->visualRating);
        WRITE_PAIR(GLX_TRANSPARENT_TYPE, modes->transparentPixel);
        WRITE_PAIR(GLX_TRANSPARENT_RED_VALUE, modes->transparentRed);
        WRITE_PAIR(GLX_TRANSPARENT_GREEN_VALUE, modes->transparentGreen);
        WRITE_PAIR(GLX_TRANSPARENT_BLUE_VALUE, modes->transparentBlue);
        WRITE_PAIR(GLX_TRANSPARENT_ALPHA_VALUE, modes->transparentAlpha);
        WRITE_PAIR(GLX_TRANSPARENT_INDEX_VALUE, modes->transparentIndex);
        WRITE_PAIR(GLX_SWAP_METHOD_OML, modes->swapMethod);
        WRITE_PAIR(GLX_SAMPLES_SGIS, modes->samples);
        WRITE_PAIR(GLX_SAMPLE_BUFFERS_SGIS, modes->sampleBuffers);
        WRITE_PAIR(GLX_VISUAL_SELECT_GROUP_SGIX, modes->visualSelectGroup);
        WRITE_PAIR(GLX_DRAWABLE_TYPE, modes->drawableType);
        WRITE_PAIR(GLX_BIND_TO_TEXTURE_RGB_EXT, modes->bindToTextureRgb);
        WRITE_PAIR(GLX_BIND_TO_TEXTURE_RGBA_EXT, modes->bindToTextureRgba);
        WRITE_PAIR(GLX_BIND_TO_MIPMAP_TEXTURE_EXT, modes->bindToMipmapTexture);
        WRITE_PAIR(GLX_BIND_TO_TEXTURE_TARGETS_EXT,
                   modes->bindToTextureTargets);
	/* can't report honestly until mesa is fixed */
	WRITE_PAIR(GLX_Y_INVERTED_EXT, GLX_DONT_CARE);
	if (modes->drawableType & GLX_PBUFFER_BIT) {
	    WRITE_PAIR(GLX_MAX_PBUFFER_WIDTH, modes->maxPbufferWidth);
	    WRITE_PAIR(GLX_MAX_PBUFFER_HEIGHT, modes->maxPbufferHeight);
	    WRITE_PAIR(GLX_MAX_PBUFFER_PIXELS, modes->maxPbufferPixels);
	    WRITE_PAIR(GLX_OPTIMAL_PBUFFER_WIDTH_SGIX,
		       modes->optimalPbufferWidth);
	    WRITE_PAIR(GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX,
		       modes->optimalPbufferHeight);
	}
        /* Add attribute only if its value is not default. */
        if (modes->sRGBCapable != GL_FALSE) {
            WRITE_PAIR(GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, modes->sRGBCapable);
        }
        /* Pad the remaining place with zeroes, so that attributes count is constant. */
        while (p < __GLX_FBCONFIG_ATTRIBS_LENGTH) {
            WRITE_PAIR(0, 0);
        }
        assert(p == __GLX_FBCONFIG_ATTRIBS_LENGTH);

        if (client->swapped) {
            __GLX_SWAP_INT_ARRAY(buf, __GLX_FBCONFIG_ATTRIBS_LENGTH);
        }
        WriteToClient(client, __GLX_SIZE_CARD32 * __GLX_FBCONFIG_ATTRIBS_LENGTH,
                      (char *) buf);
    }
    return Success;
}

int
__glXDisp_GetFBConfigs(__GLXclientState * cl, GLbyte * pc)
{
    xGLXGetFBConfigsReq *req = (xGLXGetFBConfigsReq *) pc;

    return DoGetFBConfigs(cl, req->screen);
}

int
__glXDisp_GetFBConfigsSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXGetFBConfigsSGIXReq *req = (xGLXGetFBConfigsSGIXReq *) pc;

    /* work around mesa bug, don't use REQUEST_SIZE_MATCH */
    REQUEST_AT_LEAST_SIZE(xGLXGetFBConfigsSGIXReq);
    return DoGetFBConfigs(cl, req->screen);
}

GLboolean
__glXDrawableInit(__GLXdrawable * drawable,
                  __GLXscreen * screen, DrawablePtr pDraw, int type,
                  XID drawId, __GLXconfig * config)
{
    drawable->pDraw = pDraw;
    drawable->type = type;
    drawable->drawId = drawId;
    drawable->config = config;
    drawable->eventMask = 0;

    return GL_TRUE;
}

void
__glXDrawableRelease(__GLXdrawable * drawable)
{
}

static int
DoCreateGLXDrawable(ClientPtr client, __GLXscreen * pGlxScreen,
                    __GLXconfig * config, DrawablePtr pDraw, XID drawableId,
                    XID glxDrawableId, int type)
{
    __GLXdrawable *pGlxDraw;

    if (pGlxScreen->pScreen != pDraw->pScreen)
        return BadMatch;

    pGlxDraw = pGlxScreen->createDrawable(client, pGlxScreen, pDraw,
                                          drawableId, type,
                                          glxDrawableId, config);
    if (pGlxDraw == NULL)
        return BadAlloc;

    if (!AddResource(glxDrawableId, __glXDrawableRes, pGlxDraw))
        return BadAlloc;

    /*
     * Windows aren't refcounted, so track both the X and the GLX window
     * so we get called regardless of destruction order.
     */
    if (drawableId != glxDrawableId && type == GLX_DRAWABLE_WINDOW &&
        !AddResource(pDraw->id, __glXDrawableRes, pGlxDraw))
        return BadAlloc;

    return Success;
}

static int
DoCreateGLXPixmap(ClientPtr client, __GLXscreen * pGlxScreen,
                  __GLXconfig * config, XID drawableId, XID glxDrawableId)
{
    DrawablePtr pDraw;
    int err;

    err = dixLookupDrawable(&pDraw, drawableId, client, 0, DixAddAccess);
    if (err != Success) {
        client->errorValue = drawableId;
        return err;
    }
    if (pDraw->type != DRAWABLE_PIXMAP) {
        client->errorValue = drawableId;
        return BadPixmap;
    }

    err = DoCreateGLXDrawable(client, pGlxScreen, config, pDraw, drawableId,
                              glxDrawableId, GLX_DRAWABLE_PIXMAP);

    if (err == Success)
        ((PixmapPtr) pDraw)->refcnt++;

    return err;
}

static void
determineTextureTarget(ClientPtr client, XID glxDrawableID,
                       CARD32 *attribs, CARD32 numAttribs)
{
    GLenum target = 0;
    GLenum format = 0;
    int i, err;
    __GLXdrawable *pGlxDraw;

    if (!validGlxDrawable(client, glxDrawableID, GLX_DRAWABLE_PIXMAP,
                          DixWriteAccess, &pGlxDraw, &err))
        /* We just added it in CreatePixmap, so we should never get here. */
        return;

    for (i = 0; i < numAttribs; i++) {
        if (attribs[2 * i] == GLX_TEXTURE_TARGET_EXT) {
            switch (attribs[2 * i + 1]) {
            case GLX_TEXTURE_2D_EXT:
                target = GL_TEXTURE_2D;
                break;
            case GLX_TEXTURE_RECTANGLE_EXT:
                target = GL_TEXTURE_RECTANGLE_ARB;
                break;
            }
        }

        if (attribs[2 * i] == GLX_TEXTURE_FORMAT_EXT)
            format = attribs[2 * i + 1];
    }

    if (!target) {
        int w = pGlxDraw->pDraw->width, h = pGlxDraw->pDraw->height;

        if (h & (h - 1) || w & (w - 1))
            target = GL_TEXTURE_RECTANGLE_ARB;
        else
            target = GL_TEXTURE_2D;
    }

    pGlxDraw->target = target;
    pGlxDraw->format = format;
}

int
__glXDisp_CreateGLXPixmap(__GLXclientState * cl, GLbyte * pc)
{
    xGLXCreateGLXPixmapReq *req = (xGLXCreateGLXPixmapReq *) pc;
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    int err;

    if (!validGlxScreen(cl->client, req->screen, &pGlxScreen, &err))
        return err;
    if (!validGlxVisual(cl->client, pGlxScreen, req->visual, &config, &err))
        return err;

    return DoCreateGLXPixmap(cl->client, pGlxScreen, config,
                             req->pixmap, req->glxpixmap);
}

int
__glXDisp_CreatePixmap(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreatePixmapReq *req = (xGLXCreatePixmapReq *) pc;
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    int err;

    REQUEST_AT_LEAST_SIZE(xGLXCreatePixmapReq);
    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXCreatePixmapReq, req->numAttribs << 3);

    if (!validGlxScreen(cl->client, req->screen, &pGlxScreen, &err))
        return err;
    if (!validGlxFBConfig(cl->client, pGlxScreen, req->fbconfig, &config, &err))
        return err;

    err = DoCreateGLXPixmap(cl->client, pGlxScreen, config,
                            req->pixmap, req->glxpixmap);
    if (err != Success)
        return err;

    determineTextureTarget(cl->client, req->glxpixmap,
                           (CARD32 *) (req + 1), req->numAttribs);

    return Success;
}

int
__glXDisp_CreateGLXPixmapWithConfigSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreateGLXPixmapWithConfigSGIXReq *req =
        (xGLXCreateGLXPixmapWithConfigSGIXReq *) pc;
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    int err;

    REQUEST_SIZE_MATCH(xGLXCreateGLXPixmapWithConfigSGIXReq);

    if (!validGlxScreen(cl->client, req->screen, &pGlxScreen, &err))
        return err;
    if (!validGlxFBConfig(cl->client, pGlxScreen, req->fbconfig, &config, &err))
        return err;

    return DoCreateGLXPixmap(cl->client, pGlxScreen,
                             config, req->pixmap, req->glxpixmap);
}

static int
DoDestroyDrawable(__GLXclientState * cl, XID glxdrawable, int type)
{
    __GLXdrawable *pGlxDraw;
    int err;

    if (!validGlxDrawable(cl->client, glxdrawable, type,
                          DixDestroyAccess, &pGlxDraw, &err))
        return err;

    FreeResource(glxdrawable, FALSE);

    return Success;
}

int
__glXDisp_DestroyGLXPixmap(__GLXclientState * cl, GLbyte * pc)
{
    xGLXDestroyGLXPixmapReq *req = (xGLXDestroyGLXPixmapReq *) pc;

    return DoDestroyDrawable(cl, req->glxpixmap, GLX_DRAWABLE_PIXMAP);
}

int
__glXDisp_DestroyPixmap(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyPixmapReq *req = (xGLXDestroyPixmapReq *) pc;

    /* should be REQUEST_SIZE_MATCH, but mesa's glXDestroyPixmap used to set
     * length to 3 instead of 2 */
    REQUEST_AT_LEAST_SIZE(xGLXDestroyPixmapReq);

    return DoDestroyDrawable(cl, req->glxpixmap, GLX_DRAWABLE_PIXMAP);
}

static int
DoCreatePbuffer(ClientPtr client, int screenNum, XID fbconfigId,
                int width, int height, XID glxDrawableId)
{
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    PixmapPtr pPixmap;
    int err;

    if (!validGlxScreen(client, screenNum, &pGlxScreen, &err))
        return err;
    if (!validGlxFBConfig(client, pGlxScreen, fbconfigId, &config, &err))
        return err;

    pPixmap = (*pGlxScreen->pScreen->CreatePixmap) (pGlxScreen->pScreen,
                                                    width, height,
                                                    config->rgbBits, 0);
    if (!pPixmap)
        return BadAlloc;

    /* Assign the pixmap the same id as the pbuffer and add it as a
     * resource so it and the DRI2 drawable will be reclaimed when the
     * pbuffer is destroyed. */
    pPixmap->drawable.id = glxDrawableId;
    if (!AddResource(pPixmap->drawable.id, RT_PIXMAP, pPixmap))
        return BadAlloc;

    return DoCreateGLXDrawable(client, pGlxScreen, config, &pPixmap->drawable,
                               glxDrawableId, glxDrawableId,
                               GLX_DRAWABLE_PBUFFER);
}

int
__glXDisp_CreatePbuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreatePbufferReq *req = (xGLXCreatePbufferReq *) pc;
    CARD32 *attrs;
    int width, height, i;

    REQUEST_AT_LEAST_SIZE(xGLXCreatePbufferReq);
    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXCreatePbufferReq, req->numAttribs << 3);

    attrs = (CARD32 *) (req + 1);
    width = 0;
    height = 0;

    for (i = 0; i < req->numAttribs; i++) {
        switch (attrs[i * 2]) {
        case GLX_PBUFFER_WIDTH:
            width = attrs[i * 2 + 1];
            break;
        case GLX_PBUFFER_HEIGHT:
            height = attrs[i * 2 + 1];
            break;
        case GLX_LARGEST_PBUFFER:
            /* FIXME: huh... */
            break;
        }
    }

    return DoCreatePbuffer(cl->client, req->screen, req->fbconfig,
                           width, height, req->pbuffer);
}

int
__glXDisp_CreateGLXPbufferSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreateGLXPbufferSGIXReq *req = (xGLXCreateGLXPbufferSGIXReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXCreateGLXPbufferSGIXReq);

    /*
     * We should really handle attributes correctly, but this extension
     * is so rare I have difficulty caring.
     */
    return DoCreatePbuffer(cl->client, req->screen, req->fbconfig,
                           req->width, req->height, req->pbuffer);
}

int
__glXDisp_DestroyPbuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyPbufferReq *req = (xGLXDestroyPbufferReq *) pc;

    REQUEST_SIZE_MATCH(xGLXDestroyPbufferReq);

    return DoDestroyDrawable(cl, req->pbuffer, GLX_DRAWABLE_PBUFFER);
}

int
__glXDisp_DestroyGLXPbufferSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyGLXPbufferSGIXReq *req = (xGLXDestroyGLXPbufferSGIXReq *) pc;

    REQUEST_SIZE_MATCH(xGLXDestroyGLXPbufferSGIXReq);

    return DoDestroyDrawable(cl, req->pbuffer, GLX_DRAWABLE_PBUFFER);
}

static int
DoChangeDrawableAttributes(ClientPtr client, XID glxdrawable,
                           int numAttribs, CARD32 *attribs)
{
    __GLXdrawable *pGlxDraw;
    int i, err;

    if (!validGlxDrawable(client, glxdrawable, GLX_DRAWABLE_ANY,
                          DixSetAttrAccess, &pGlxDraw, &err))
        return err;

    for (i = 0; i < numAttribs; i++) {
        switch (attribs[i * 2]) {
        case GLX_EVENT_MASK:
            /* All we do is to record the event mask so we can send it
             * back when queried.  We never actually clobber the
             * pbuffers, so we never need to send out the event. */
            pGlxDraw->eventMask = attribs[i * 2 + 1];
            break;
        }
    }

    return Success;
}

int
__glXDisp_ChangeDrawableAttributes(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXChangeDrawableAttributesReq *req =
        (xGLXChangeDrawableAttributesReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXChangeDrawableAttributesReq);
    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
#if 0
    /* mesa sends an additional 8 bytes */
    REQUEST_FIXED_SIZE(xGLXChangeDrawableAttributesReq, req->numAttribs << 3);
#else
    if (((sizeof(xGLXChangeDrawableAttributesReq) +
          (req->numAttribs << 3)) >> 2) < client->req_len)
        return BadLength;
#endif

    return DoChangeDrawableAttributes(cl->client, req->drawable,
                                      req->numAttribs, (CARD32 *) (req + 1));
}

int
__glXDisp_ChangeDrawableAttributesSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXChangeDrawableAttributesSGIXReq *req =
        (xGLXChangeDrawableAttributesSGIXReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXChangeDrawableAttributesSGIXReq);
    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXChangeDrawableAttributesSGIXReq,
                       req->numAttribs << 3);

    return DoChangeDrawableAttributes(cl->client, req->drawable,
                                      req->numAttribs, (CARD32 *) (req + 1));
}

int
__glXDisp_CreateWindow(__GLXclientState * cl, GLbyte * pc)
{
    xGLXCreateWindowReq *req = (xGLXCreateWindowReq *) pc;
    __GLXconfig *config;
    __GLXscreen *pGlxScreen;
    ClientPtr client = cl->client;
    DrawablePtr pDraw;
    int err;

    REQUEST_AT_LEAST_SIZE(xGLXCreateWindowReq);
    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXCreateWindowReq, req->numAttribs << 3);

    if (!validGlxScreen(client, req->screen, &pGlxScreen, &err))
        return err;
    if (!validGlxFBConfig(client, pGlxScreen, req->fbconfig, &config, &err))
        return err;

    err = dixLookupDrawable(&pDraw, req->window, client, 0, DixAddAccess);
    if (err != Success || pDraw->type != DRAWABLE_WINDOW) {
        client->errorValue = req->window;
        return BadWindow;
    }

    if (!validGlxFBConfigForWindow(client, config, pDraw, &err))
        return err;

    return DoCreateGLXDrawable(client, pGlxScreen, config,
                               pDraw, req->window,
                               req->glxwindow, GLX_DRAWABLE_WINDOW);
}

int
__glXDisp_DestroyWindow(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyWindowReq *req = (xGLXDestroyWindowReq *) pc;

    /* mesa's glXDestroyWindow used to set length to 3 instead of 2 */
    REQUEST_AT_LEAST_SIZE(xGLXDestroyWindowReq);

    return DoDestroyDrawable(cl, req->glxwindow, GLX_DRAWABLE_WINDOW);
}

/*****************************************************************************/

/*
** NOTE: There is no portable implementation for swap buffers as of
** this time that is of value.  Consequently, this code must be
** implemented by somebody other than SGI.
*/
int
__glXDisp_SwapBuffers(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXSwapBuffersReq *req = (xGLXSwapBuffersReq *) pc;
    GLXContextTag tag;
    XID drawId;
    __GLXcontext *glxc = NULL;
    __GLXdrawable *pGlxDraw;
    int error;

    tag = req->contextTag;
    drawId = req->drawable;
    if (tag) {
        glxc = __glXLookupContextByTag(cl, tag);
        if (!glxc) {
            return __glXError(GLXBadContextTag);
        }
        /*
         ** The calling thread is swapping its current drawable.  In this case,
         ** glxSwapBuffers is in both GL and X streams, in terms of
         ** sequentiality.
         */
        if (__glXForceCurrent(cl, tag, &error)) {
            /*
             ** Do whatever is needed to make sure that all preceding requests
             ** in both streams are completed before the swap is executed.
             */
            glFinish();
        }
        else {
            return error;
        }
    }

    pGlxDraw = __glXGetDrawable(glxc, drawId, client, &error);
    if (pGlxDraw == NULL)
        return error;

    if (pGlxDraw->type == DRAWABLE_WINDOW &&
        (*pGlxDraw->swapBuffers) (cl->client, pGlxDraw) == GL_FALSE)
        return __glXError(GLXBadDrawable);

    return Success;
}

static int
DoQueryContext(__GLXclientState * cl, GLXContextID gcId)
{
    ClientPtr client = cl->client;
    __GLXcontext *ctx;
    xGLXQueryContextInfoEXTReply reply;
    int nProps = 5;
    int sendBuf[nProps * 2];
    int nReplyBytes;
    int err;

    if (!validGlxContext(cl->client, gcId, DixReadAccess, &ctx, &err))
        return err;

    reply = (xGLXQueryContextInfoEXTReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = nProps << 1,
        .n = nProps
    };

    nReplyBytes = reply.length << 2;
    sendBuf[0] = GLX_SHARE_CONTEXT_EXT;
    sendBuf[1] = (int) (ctx->share_id);
    sendBuf[2] = GLX_VISUAL_ID_EXT;
    sendBuf[3] = (int) (ctx->config ? ctx->config->visualID : 0);
    sendBuf[4] = GLX_SCREEN_EXT;
    sendBuf[5] = (int) (ctx->pGlxScreen->pScreen->myNum);
    sendBuf[6] = GLX_FBCONFIG_ID;
    sendBuf[7] = (int) (ctx->config ? ctx->config->fbconfigID : 0);
    sendBuf[8] = GLX_RENDER_TYPE;
    sendBuf[9] = (int) (ctx->renderType);

    if (client->swapped) {
        int length = reply.length;

        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_DECLARE_SWAP_ARRAY_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.n);
        WriteToClient(client, sz_xGLXQueryContextInfoEXTReply, &reply);
        __GLX_SWAP_INT_ARRAY((int *) sendBuf, length);
        WriteToClient(client, length << 2, sendBuf);
    }
    else {
        WriteToClient(client, sz_xGLXQueryContextInfoEXTReply, &reply);
        WriteToClient(client, nReplyBytes, sendBuf);
    }

    return Success;
}

int
__glXDisp_QueryContextInfoEXT(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXQueryContextInfoEXTReq *req = (xGLXQueryContextInfoEXTReq *) pc;

    REQUEST_SIZE_MATCH(xGLXQueryContextInfoEXTReq);

    return DoQueryContext(cl, req->context);
}

int
__glXDisp_QueryContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXQueryContextReq *req = (xGLXQueryContextReq *) pc;

    return DoQueryContext(cl, req->context);
}

int
__glXDisp_BindTexImageEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    ClientPtr client = cl->client;
    __GLXcontext *context;
    __GLXdrawable *pGlxDraw;
    GLXDrawable drawId;
    int buffer;
    int error;
    CARD32 num_attribs;

    if ((sizeof(xGLXVendorPrivateReq) + 12) >> 2 > client->req_len)
        return BadLength;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = *((CARD32 *) (pc));
    buffer = *((INT32 *) (pc + 4));
    num_attribs = *((CARD32 *) (pc + 8));
    if (num_attribs > (UINT32_MAX >> 3)) {
        client->errorValue = num_attribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXVendorPrivateReq, 12 + (num_attribs << 3));

    if (buffer != GLX_FRONT_LEFT_EXT)
        return __glXError(GLXBadPixmap);

    context = __glXForceCurrent(cl, req->contextTag, &error);
    if (!context)
        return error;

    if (!validGlxDrawable(client, drawId, GLX_DRAWABLE_PIXMAP,
                          DixReadAccess, &pGlxDraw, &error))
        return error;

    if (!context->bindTexImage)
        return __glXError(GLXUnsupportedPrivateRequest);

    return context->bindTexImage(context, buffer, pGlxDraw);
}

int
__glXDisp_ReleaseTexImageEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    ClientPtr client = cl->client;
    __GLXdrawable *pGlxDraw;
    __GLXcontext *context;
    GLXDrawable drawId;
    int buffer;
    int error;

    REQUEST_FIXED_SIZE(xGLXVendorPrivateReq, 8);

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = *((CARD32 *) (pc));
    buffer = *((INT32 *) (pc + 4));

    context = __glXForceCurrent(cl, req->contextTag, &error);
    if (!context)
        return error;

    if (!validGlxDrawable(client, drawId, GLX_DRAWABLE_PIXMAP,
                          DixReadAccess, &pGlxDraw, &error))
        return error;

    if (!context->releaseTexImage)
        return __glXError(GLXUnsupportedPrivateRequest);

    return context->releaseTexImage(context, buffer, pGlxDraw);
}

int
__glXDisp_CopySubBufferMESA(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLXContextTag tag = req->contextTag;
    __GLXcontext *glxc = NULL;
    __GLXdrawable *pGlxDraw;
    ClientPtr client = cl->client;
    GLXDrawable drawId;
    int error;
    int x, y, width, height;

    (void) client;
    (void) req;

    REQUEST_FIXED_SIZE(xGLXVendorPrivateReq, 20);

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = *((CARD32 *) (pc));
    x = *((INT32 *) (pc + 4));
    y = *((INT32 *) (pc + 8));
    width = *((INT32 *) (pc + 12));
    height = *((INT32 *) (pc + 16));

    if (tag) {
        glxc = __glXLookupContextByTag(cl, tag);
        if (!glxc) {
            return __glXError(GLXBadContextTag);
        }
        /*
         ** The calling thread is swapping its current drawable.  In this case,
         ** glxSwapBuffers is in both GL and X streams, in terms of
         ** sequentiality.
         */
        if (__glXForceCurrent(cl, tag, &error)) {
            /*
             ** Do whatever is needed to make sure that all preceding requests
             ** in both streams are completed before the swap is executed.
             */
            glFinish();
        }
        else {
            return error;
        }
    }

    pGlxDraw = __glXGetDrawable(glxc, drawId, client, &error);
    if (!pGlxDraw)
        return error;

    if (pGlxDraw == NULL ||
        pGlxDraw->type != GLX_DRAWABLE_WINDOW ||
        pGlxDraw->copySubBuffer == NULL)
        return __glXError(GLXBadDrawable);

    (*pGlxDraw->copySubBuffer) (pGlxDraw, x, y, width, height);

    return Success;
}

/* hack for old glxext.h */
#ifndef GLX_STEREO_TREE_EXT
#define GLX_STEREO_TREE_EXT                 0x20F5
#endif

/*
** Get drawable attributes
*/
static int
DoGetDrawableAttributes(__GLXclientState * cl, XID drawId)
{
    ClientPtr client = cl->client;
    xGLXGetDrawableAttributesReply reply;
    __GLXdrawable *pGlxDraw = NULL;
    DrawablePtr pDraw;
    CARD32 attributes[20];
    int num = 0, error;

    if (!validGlxDrawable(client, drawId, GLX_DRAWABLE_ANY,
                          DixGetAttrAccess, &pGlxDraw, &error)) {
        /* hack for GLX 1.2 naked windows */
        int err = dixLookupWindow((WindowPtr *)&pDraw, drawId, client,
                                  DixGetAttrAccess);
        if (err != Success)
            return __glXError(GLXBadDrawable);
    }
    if (pGlxDraw)
        pDraw = pGlxDraw->pDraw;

#define ATTRIB(a, v) do { \
    attributes[2*num] = (a); \
    attributes[2*num+1] = (v); \
    num++; \
    } while (0)

    ATTRIB(GLX_Y_INVERTED_EXT, GL_FALSE);
    ATTRIB(GLX_WIDTH, pDraw->width);
    ATTRIB(GLX_HEIGHT, pDraw->height);
    ATTRIB(GLX_SCREEN, pDraw->pScreen->myNum);
    if (pGlxDraw) {
        ATTRIB(GLX_TEXTURE_TARGET_EXT,
               pGlxDraw->target == GL_TEXTURE_2D ?
                GLX_TEXTURE_2D_EXT : GLX_TEXTURE_RECTANGLE_EXT);
        ATTRIB(GLX_EVENT_MASK, pGlxDraw->eventMask);
        ATTRIB(GLX_FBCONFIG_ID, pGlxDraw->config->fbconfigID);
        if (pGlxDraw->type == GLX_DRAWABLE_PBUFFER) {
            ATTRIB(GLX_PRESERVED_CONTENTS, GL_TRUE);
        }
        if (pGlxDraw->type == GLX_DRAWABLE_WINDOW) {
            ATTRIB(GLX_STEREO_TREE_EXT, 0);
        }
    }

    /* GLX_EXT_get_drawable_type */
    if (!pGlxDraw || pGlxDraw->type == GLX_DRAWABLE_WINDOW)
        ATTRIB(GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT);
    else if (pGlxDraw->type == GLX_DRAWABLE_PIXMAP)
        ATTRIB(GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT);
    else if (pGlxDraw->type == GLX_DRAWABLE_PBUFFER)
        ATTRIB(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT);
#undef ATTRIB

    reply = (xGLXGetDrawableAttributesReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = num << 1,
        .numAttribs = num
    };

    if (client->swapped) {
        int length = reply.length;

        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_DECLARE_SWAP_ARRAY_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.numAttribs);
        WriteToClient(client, sz_xGLXGetDrawableAttributesReply, &reply);
        __GLX_SWAP_INT_ARRAY((int *) attributes, length);
        WriteToClient(client, length << 2, attributes);
    }
    else {
        WriteToClient(client, sz_xGLXGetDrawableAttributesReply, &reply);
        WriteToClient(client, reply.length * sizeof(CARD32), attributes);
    }

    return Success;
}

int
__glXDisp_GetDrawableAttributes(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXGetDrawableAttributesReq *req = (xGLXGetDrawableAttributesReq *) pc;

    /* this should be REQUEST_SIZE_MATCH, but mesa sends an additional 4 bytes */
    REQUEST_AT_LEAST_SIZE(xGLXGetDrawableAttributesReq);

    return DoGetDrawableAttributes(cl, req->drawable);
}

int
__glXDisp_GetDrawableAttributesSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXGetDrawableAttributesSGIXReq *req =
        (xGLXGetDrawableAttributesSGIXReq *) pc;

    REQUEST_SIZE_MATCH(xGLXGetDrawableAttributesSGIXReq);

    return DoGetDrawableAttributes(cl, req->drawable);
}

/************************************************************************/

/*
** Render and Renderlarge are not in the GLX API.  They are used by the GLX
** client library to send batches of GL rendering commands.
*/

/*
** Reset state used to keep track of large (multi-request) commands.
*/
static void
ResetLargeCommandStatus(__GLXcontext *cx)
{
    cx->largeCmdBytesSoFar = 0;
    cx->largeCmdBytesTotal = 0;
    cx->largeCmdRequestsSoFar = 0;
    cx->largeCmdRequestsTotal = 0;
}

/*
** Execute all the drawing commands in a request.
*/
int
__glXDisp_Render(__GLXclientState * cl, GLbyte * pc)
{
    xGLXRenderReq *req;
    ClientPtr client = cl->client;
    int left, cmdlen, error;
    int commandsDone;
    CARD16 opcode;
    __GLXrenderHeader *hdr;
    __GLXcontext *glxc;

    __GLX_DECLARE_SWAP_VARIABLES;

    REQUEST_AT_LEAST_SIZE(xGLXRenderReq);

    req = (xGLXRenderReq *) pc;
    if (client->swapped) {
        __GLX_SWAP_SHORT(&req->length);
        __GLX_SWAP_INT(&req->contextTag);
    }

    glxc = __glXForceCurrent(cl, req->contextTag, &error);
    if (!glxc) {
        return error;
    }

    commandsDone = 0;
    pc += sz_xGLXRenderReq;
    left = (req->length << 2) - sz_xGLXRenderReq;
    while (left > 0) {
        __GLXrenderSizeData entry;
        int extra = 0;
        __GLXdispatchRenderProcPtr proc;
        int err;

        if (left < sizeof(__GLXrenderHeader))
            return BadLength;

        /*
         ** Verify that the header length and the overall length agree.
         ** Also, each command must be word aligned.
         */
        hdr = (__GLXrenderHeader *) pc;
        if (client->swapped) {
            __GLX_SWAP_SHORT(&hdr->length);
            __GLX_SWAP_SHORT(&hdr->opcode);
        }
        cmdlen = hdr->length;
        opcode = hdr->opcode;

        if (left < cmdlen)
            return BadLength;

        /*
         ** Check for core opcodes and grab entry data.
         */
        err = __glXGetProtocolSizeData(&Render_dispatch_info, opcode, &entry);
        proc = (__GLXdispatchRenderProcPtr)
            __glXGetProtocolDecodeFunction(&Render_dispatch_info,
                                           opcode, client->swapped);

        if ((err < 0) || (proc == NULL)) {
            client->errorValue = commandsDone;
            return __glXError(GLXBadRenderRequest);
        }

        if (cmdlen < entry.bytes) {
            return BadLength;
        }

        if (entry.varsize) {
            /* variable size command */
            extra = (*entry.varsize) (pc + __GLX_RENDER_HDR_SIZE,
                                      client->swapped,
                                      left - __GLX_RENDER_HDR_SIZE);
            if (extra < 0) {
                return BadLength;
            }
        }

        if (cmdlen != safe_pad(safe_add(entry.bytes, extra))) {
            return BadLength;
        }

        /*
         ** Skip over the header and execute the command.  We allow the
         ** caller to trash the command memory.  This is useful especially
         ** for things that require double alignment - they can just shift
         ** the data towards lower memory (trashing the header) by 4 bytes
         ** and achieve the required alignment.
         */
        (*proc) (pc + __GLX_RENDER_HDR_SIZE);
        pc += cmdlen;
        left -= cmdlen;
        commandsDone++;
    }
    return Success;
}

/*
** Execute a large rendering request (one that spans multiple X requests).
*/
int
__glXDisp_RenderLarge(__GLXclientState * cl, GLbyte * pc)
{
    xGLXRenderLargeReq *req;
    ClientPtr client = cl->client;
    size_t dataBytes;
    __GLXrenderLargeHeader *hdr;
    __GLXcontext *glxc;
    int error;
    CARD16 opcode;

    __GLX_DECLARE_SWAP_VARIABLES;

    REQUEST_AT_LEAST_SIZE(xGLXRenderLargeReq);

    req = (xGLXRenderLargeReq *) pc;
    if (client->swapped) {
        __GLX_SWAP_SHORT(&req->length);
        __GLX_SWAP_INT(&req->contextTag);
        __GLX_SWAP_INT(&req->dataBytes);
        __GLX_SWAP_SHORT(&req->requestNumber);
        __GLX_SWAP_SHORT(&req->requestTotal);
    }

    glxc = __glXForceCurrent(cl, req->contextTag, &error);
    if (!glxc) {
        return error;
    }
    if (safe_pad(req->dataBytes) < 0)
        return BadLength;
    dataBytes = req->dataBytes;

    /*
     ** Check the request length.
     */
    if ((req->length << 2) != safe_pad(dataBytes) + sz_xGLXRenderLargeReq) {
        client->errorValue = req->length;
        /* Reset in case this isn't 1st request. */
        ResetLargeCommandStatus(glxc);
        return BadLength;
    }
    pc += sz_xGLXRenderLargeReq;

    if (glxc->largeCmdRequestsSoFar == 0) {
        __GLXrenderSizeData entry;
        int extra = 0;
        int left = (req->length << 2) - sz_xGLXRenderLargeReq;
        int cmdlen;
        int err;

        /*
         ** This is the first request of a multi request command.
         ** Make enough space in the buffer, then copy the entire request.
         */
        if (req->requestNumber != 1) {
            client->errorValue = req->requestNumber;
            return __glXError(GLXBadLargeRequest);
        }

        if (dataBytes < __GLX_RENDER_LARGE_HDR_SIZE)
            return BadLength;

        hdr = (__GLXrenderLargeHeader *) pc;
        if (client->swapped) {
            __GLX_SWAP_INT(&hdr->length);
            __GLX_SWAP_INT(&hdr->opcode);
        }
        opcode = hdr->opcode;
        if ((cmdlen = safe_pad(hdr->length)) < 0)
            return BadLength;

        /*
         ** Check for core opcodes and grab entry data.
         */
        err = __glXGetProtocolSizeData(&Render_dispatch_info, opcode, &entry);
        if (err < 0) {
            client->errorValue = opcode;
            return __glXError(GLXBadLargeRequest);
        }

        if (entry.varsize) {
            /*
             ** If it's a variable-size command (a command whose length must
             ** be computed from its parameters), all the parameters needed
             ** will be in the 1st request, so it's okay to do this.
             */
            extra = (*entry.varsize) (pc + __GLX_RENDER_LARGE_HDR_SIZE,
                                      client->swapped,
                                      left - __GLX_RENDER_LARGE_HDR_SIZE);
            if (extra < 0) {
                return BadLength;
            }
        }

        /* the +4 is safe because we know entry.bytes is small */
        if (cmdlen != safe_pad(safe_add(entry.bytes + 4, extra))) {
            return BadLength;
        }

        /*
         ** Make enough space in the buffer, then copy the entire request.
         */
        if (glxc->largeCmdBufSize < cmdlen) {
	    GLbyte *newbuf = glxc->largeCmdBuf;

	    if (!(newbuf = realloc(newbuf, cmdlen)))
		return BadAlloc;

	    glxc->largeCmdBuf = newbuf;
            glxc->largeCmdBufSize = cmdlen;
        }
        memcpy(glxc->largeCmdBuf, pc, dataBytes);

        glxc->largeCmdBytesSoFar = dataBytes;
        glxc->largeCmdBytesTotal = cmdlen;
        glxc->largeCmdRequestsSoFar = 1;
        glxc->largeCmdRequestsTotal = req->requestTotal;
        return Success;

    }
    else {
        /*
         ** We are receiving subsequent (i.e. not the first) requests of a
         ** multi request command.
         */
        int bytesSoFar; /* including this packet */

        /*
         ** Check the request number and the total request count.
         */
        if (req->requestNumber != glxc->largeCmdRequestsSoFar + 1) {
            client->errorValue = req->requestNumber;
            ResetLargeCommandStatus(glxc);
            return __glXError(GLXBadLargeRequest);
        }
        if (req->requestTotal != glxc->largeCmdRequestsTotal) {
            client->errorValue = req->requestTotal;
            ResetLargeCommandStatus(glxc);
            return __glXError(GLXBadLargeRequest);
        }

        /*
         ** Check that we didn't get too much data.
         */
        if ((bytesSoFar = safe_add(glxc->largeCmdBytesSoFar, dataBytes)) < 0) {
            client->errorValue = dataBytes;
            ResetLargeCommandStatus(glxc);
            return __glXError(GLXBadLargeRequest);
        }

        if (bytesSoFar > glxc->largeCmdBytesTotal) {
            client->errorValue = dataBytes;
            ResetLargeCommandStatus(glxc);
            return __glXError(GLXBadLargeRequest);
        }

        memcpy(glxc->largeCmdBuf + glxc->largeCmdBytesSoFar, pc, dataBytes);
        glxc->largeCmdBytesSoFar += dataBytes;
        glxc->largeCmdRequestsSoFar++;

        if (req->requestNumber == glxc->largeCmdRequestsTotal) {
            __GLXdispatchRenderProcPtr proc;

            /*
             ** This is the last request; it must have enough bytes to complete
             ** the command.
             */
            /* NOTE: the pad macro below is needed because the client library
             ** pads the total byte count, but not the per-request byte counts.
             ** The Protocol Encoding says the total byte count should not be
             ** padded, so a proposal will be made to the ARB to relax the
             ** padding constraint on the total byte count, thus preserving
             ** backward compatibility.  Meanwhile, the padding done below
             ** fixes a bug that did not allow large commands of odd sizes to
             ** be accepted by the server.
             */
            if (safe_pad(glxc->largeCmdBytesSoFar) != glxc->largeCmdBytesTotal) {
                client->errorValue = dataBytes;
                ResetLargeCommandStatus(glxc);
                return __glXError(GLXBadLargeRequest);
            }
            hdr = (__GLXrenderLargeHeader *) glxc->largeCmdBuf;
            /*
             ** The opcode and length field in the header had already been
             ** swapped when the first request was received.
             **
             ** Use the opcode to index into the procedure table.
             */
            opcode = hdr->opcode;

            proc = (__GLXdispatchRenderProcPtr)
                __glXGetProtocolDecodeFunction(&Render_dispatch_info, opcode,
                                               client->swapped);
            if (proc == NULL) {
                client->errorValue = opcode;
                return __glXError(GLXBadLargeRequest);
            }

            /*
             ** Skip over the header and execute the command.
             */
            (*proc) (glxc->largeCmdBuf + __GLX_RENDER_LARGE_HDR_SIZE);

            /*
             ** Reset for the next RenderLarge series.
             */
            ResetLargeCommandStatus(glxc);
        }
        else {
            /*
             ** This is neither the first nor the last request.
             */
        }
        return Success;
    }
}

/************************************************************************/

/*
** No support is provided for the vendor-private requests other than
** allocating the entry points in the dispatch table.
*/

int
__glXDisp_VendorPrivate(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLint vendorcode = req->vendorCode;
    __GLXdispatchVendorPrivProcPtr proc;

    REQUEST_AT_LEAST_SIZE(xGLXVendorPrivateReq);

    proc = (__GLXdispatchVendorPrivProcPtr)
        __glXGetProtocolDecodeFunction(&VendorPriv_dispatch_info,
                                       vendorcode, 0);
    if (proc != NULL) {
        return (*proc) (cl, (GLbyte *) req);
    }

    cl->client->errorValue = req->vendorCode;
    return __glXError(GLXUnsupportedPrivateRequest);
}

int
__glXDisp_VendorPrivateWithReply(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLint vendorcode = req->vendorCode;
    __GLXdispatchVendorPrivProcPtr proc;

    REQUEST_AT_LEAST_SIZE(xGLXVendorPrivateReq);

    proc = (__GLXdispatchVendorPrivProcPtr)
        __glXGetProtocolDecodeFunction(&VendorPriv_dispatch_info,
                                       vendorcode, 0);
    if (proc != NULL) {
        return (*proc) (cl, (GLbyte *) req);
    }

    cl->client->errorValue = vendorcode;
    return __glXError(GLXUnsupportedPrivateRequest);
}

int
__glXDisp_QueryExtensionsString(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXQueryExtensionsStringReq *req = (xGLXQueryExtensionsStringReq *) pc;
    xGLXQueryExtensionsStringReply reply;
    __GLXscreen *pGlxScreen;
    size_t n, length;
    char *buf;
    int err;

    if (!validGlxScreen(client, req->screen, &pGlxScreen, &err))
        return err;

    n = strlen(pGlxScreen->GLXextensions) + 1;
    length = __GLX_PAD(n) >> 2;
    reply = (xGLXQueryExtensionsStringReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = length,
        .n = n
    };

    /* Allocate buffer to make sure it's a multiple of 4 bytes big. */
    buf = calloc(length, 4);
    if (buf == NULL)
        return BadAlloc;
    memcpy(buf, pGlxScreen->GLXextensions, n);

    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_DECLARE_SWAP_ARRAY_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.n);
        WriteToClient(client, sz_xGLXQueryExtensionsStringReply, &reply);
        __GLX_SWAP_INT_ARRAY((int *) buf, length);
        WriteToClient(client, length << 2, buf);
    }
    else {
        WriteToClient(client, sz_xGLXQueryExtensionsStringReply, &reply);
        WriteToClient(client, (int) (length << 2), buf);
    }

    free(buf);
    return Success;
}

#ifndef GLX_VENDOR_NAMES_EXT
#define GLX_VENDOR_NAMES_EXT 0x20F6
#endif

int
__glXDisp_QueryServerString(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXQueryServerStringReq *req = (xGLXQueryServerStringReq *) pc;
    xGLXQueryServerStringReply reply;
    size_t n, length;
    const char *ptr;
    char *buf;
    __GLXscreen *pGlxScreen;
    int err;

    if (!validGlxScreen(client, req->screen, &pGlxScreen, &err))
        return err;

    switch (req->name) {
    case GLX_VENDOR:
        ptr = GLXServerVendorName;
        break;
    case GLX_VERSION:
        ptr = "1.4";
        break;
    case GLX_EXTENSIONS:
        ptr = pGlxScreen->GLXextensions;
        break;
    case GLX_VENDOR_NAMES_EXT:
        if (pGlxScreen->glvnd) {
            ptr = pGlxScreen->glvnd;
            break;
        }
        /* else fall through */
    default:
        return BadValue;
    }

    n = strlen(ptr) + 1;
    length = __GLX_PAD(n) >> 2;
    reply = (xGLXQueryServerStringReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = length,
        .n = n
    };

    buf = calloc(length, 4);
    if (buf == NULL) {
        return BadAlloc;
    }
    memcpy(buf, ptr, n);

    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.n);
        WriteToClient(client, sz_xGLXQueryServerStringReply, &reply);
        /** no swap is needed for an array of chars **/
        /* __GLX_SWAP_INT_ARRAY((int *)buf, length); */
        WriteToClient(client, length << 2, buf);
    }
    else {
        WriteToClient(client, sz_xGLXQueryServerStringReply, &reply);
        WriteToClient(client, (int) (length << 2), buf);
    }

    free(buf);
    return Success;
}

int
__glXDisp_ClientInfo(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXClientInfoReq *req = (xGLXClientInfoReq *) pc;
    const char *buf;

    REQUEST_AT_LEAST_SIZE(xGLXClientInfoReq);

    buf = (const char *) (req + 1);
    if (!memchr(buf, 0, (client->req_len << 2) - sizeof(xGLXClientInfoReq)))
        return BadLength;

    free(cl->GLClientextensions);
    cl->GLClientextensions = strdup(buf);

    return Success;
}

#include <GL/glxtokens.h>

void
__glXsendSwapEvent(__GLXdrawable *drawable, int type, CARD64 ust,
                   CARD64 msc, CARD32 sbc)
{
    ClientPtr client = clients[CLIENT_ID(drawable->drawId)];

    xGLXBufferSwapComplete2 wire =  {
        .type = __glXEventBase + GLX_BufferSwapComplete
    };

    if (!client)
        return;

    if (!(drawable->eventMask & GLX_BUFFER_SWAP_COMPLETE_INTEL_MASK))
        return;

    wire.event_type = type;
    wire.drawable = drawable->drawId;
    wire.ust_hi = ust >> 32;
    wire.ust_lo = ust & 0xffffffff;
    wire.msc_hi = msc >> 32;
    wire.msc_lo = msc & 0xffffffff;
    wire.sbc = sbc;

    WriteEventsToClient(client, 1, (xEvent *) &wire);
}

#if PRESENT
static void
__glXpresentCompleteNotify(WindowPtr window, CARD8 present_kind, CARD8 present_mode,
                           CARD32 serial, uint64_t ust, uint64_t msc)
{
    __GLXdrawable *drawable;
    int glx_type;
    int rc;

    if (present_kind != PresentCompleteKindPixmap)
        return;

    rc = dixLookupResourceByType((void **) &drawable, window->drawable.id,
                                 __glXDrawableRes, serverClient, DixGetAttrAccess);

    if (rc != Success)
        return;

    if (present_mode == PresentCompleteModeFlip)
        glx_type = GLX_FLIP_COMPLETE_INTEL;
    else
        glx_type = GLX_BLIT_COMPLETE_INTEL;

    __glXsendSwapEvent(drawable, glx_type, ust, msc, serial);
}

#include <present.h>

void
__glXregisterPresentCompleteNotify(void)
{
    present_register_complete_notify(__glXpresentCompleteNotify);
}
#endif
