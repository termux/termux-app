/*
 * Copyright © 2007 Red Hat, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat,
 * Inc not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Red Hat, Inc makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL RED HAT, INC BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include <GL/glxtokens.h>

#include <windowstr.h>
#include <os.h>

#define _XF86DRI_SERVER_
#include <xf86.h>
#include <dri2.h>

#include <GL/glxtokens.h>
#include "glxserver.h"
#include "glxutil.h"
#include "glxdricommon.h"

#include "extension_string.h"

typedef struct __GLXDRIscreen __GLXDRIscreen;
typedef struct __GLXDRIcontext __GLXDRIcontext;
typedef struct __GLXDRIdrawable __GLXDRIdrawable;

#define ALL_DRI_CTX_FLAGS (__DRI_CTX_FLAG_DEBUG                         \
                           | __DRI_CTX_FLAG_FORWARD_COMPATIBLE          \
                           | __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS)

struct __GLXDRIscreen {
    __GLXscreen base;
    __DRIscreen *driScreen;
    void *driver;
    int fd;

    xf86EnterVTProc *enterVT;
    xf86LeaveVTProc *leaveVT;

    const __DRIcoreExtension *core;
    const __DRIdri2Extension *dri2;
    const __DRI2flushExtension *flush;
    const __DRIcopySubBufferExtension *copySubBuffer;
    const __DRIswapControlExtension *swapControl;
    const __DRItexBufferExtension *texBuffer;
    const __DRIconfig **driConfigs;
};

struct __GLXDRIcontext {
    __GLXcontext base;
    __DRIcontext *driContext;
};

#define MAX_DRAWABLE_BUFFERS 5

struct __GLXDRIdrawable {
    __GLXdrawable base;
    __DRIdrawable *driDrawable;
    __GLXDRIscreen *screen;

    /* Dimensions as last reported by DRI2GetBuffers. */
    int width;
    int height;
    __DRIbuffer buffers[MAX_DRAWABLE_BUFFERS];
    int count;
    XID dri2_id;
};

static void
copy_box(__GLXdrawable * drawable,
         int dst, int src,
         int x, int y, int w, int h)
{
    BoxRec box;
    RegionRec region;
    __GLXcontext *cx = lastGLContext;

    box.x1 = x;
    box.y1 = y;
    box.x2 = x + w;
    box.y2 = y + h;
    RegionInit(&region, &box, 0);

    DRI2CopyRegion(drawable->pDraw, &region, dst, src);
    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);
    }
}

/* white lie */
extern glx_func_ptr glXGetProcAddressARB(const char *);

static void
__glXDRIdrawableDestroy(__GLXdrawable * drawable)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;
    const __DRIcoreExtension *core = private->screen->core;

    FreeResource(private->dri2_id, FALSE);

    (*core->destroyDrawable) (private->driDrawable);

    __glXDrawableRelease(drawable);

    free(private);
}

static void
__glXDRIdrawableCopySubBuffer(__GLXdrawable * drawable,
                              int x, int y, int w, int h)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;

    copy_box(drawable, x, private->height - y - h,
             w, h,
             DRI2BufferFrontLeft, DRI2BufferBackLeft);
}

static void
__glXDRIdrawableWaitX(__GLXdrawable * drawable)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;

    copy_box(drawable, DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft,
             0, 0, private->width, private->height);
}

static void
__glXDRIdrawableWaitGL(__GLXdrawable * drawable)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;

    copy_box(drawable, DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft,
             0, 0, private->width, private->height);
}

static void
__glXdriSwapEvent(ClientPtr client, void *data, int type, CARD64 ust,
                  CARD64 msc, CARD32 sbc)
{
    __GLXdrawable *drawable = data;
    int glx_type;
    switch (type) {
    case DRI2_EXCHANGE_COMPLETE:
        glx_type = GLX_EXCHANGE_COMPLETE_INTEL;
        break;
    default:
        /* unknown swap completion type,
         * BLIT is a reasonable default, so
         * fall through ...
         */
    case DRI2_BLIT_COMPLETE:
        glx_type = GLX_BLIT_COMPLETE_INTEL;
        break;
    case DRI2_FLIP_COMPLETE:
        glx_type = GLX_FLIP_COMPLETE_INTEL;
        break;
    }

    __glXsendSwapEvent(drawable, glx_type, ust, msc, sbc);
}

/*
 * Copy or flip back to front, honoring the swap interval if possible.
 *
 * If the kernel supports it, we request an event for the frame when the
 * swap should happen, then perform the copy when we receive it.
 */
static GLboolean
__glXDRIdrawableSwapBuffers(ClientPtr client, __GLXdrawable * drawable)
{
    __GLXDRIdrawable *priv = (__GLXDRIdrawable *) drawable;
    __GLXDRIscreen *screen = priv->screen;
    CARD64 unused;
    __GLXcontext *cx = lastGLContext;
    int status;

    if (screen->flush) {
        (*screen->flush->flush) (priv->driDrawable);
        (*screen->flush->invalidate) (priv->driDrawable);
    }

    status = DRI2SwapBuffers(client, drawable->pDraw, 0, 0, 0, &unused,
                             __glXdriSwapEvent, drawable);
    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);
    }

    return status == Success;
}

static int
__glXDRIdrawableSwapInterval(__GLXdrawable * drawable, int interval)
{
    __GLXcontext *cx = lastGLContext;

    if (interval <= 0)          /* || interval > BIGNUM? */
        return GLX_BAD_VALUE;

    DRI2SwapInterval(drawable->pDraw, interval);
    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);
    }

    return 0;
}

static void
__glXDRIcontextDestroy(__GLXcontext * baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) context->base.pGlxScreen;

    (*screen->core->destroyContext) (context->driContext);
    __glXContextDestroy(&context->base);
    free(context);
}

static int
__glXDRIcontextMakeCurrent(__GLXcontext * baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIdrawable *draw = (__GLXDRIdrawable *) baseContext->drawPriv;
    __GLXDRIdrawable *read = (__GLXDRIdrawable *) baseContext->readPriv;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) context->base.pGlxScreen;

    return (*screen->core->bindContext) (context->driContext,
                                         draw->driDrawable, read->driDrawable);
}

static int
__glXDRIcontextLoseCurrent(__GLXcontext * baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) context->base.pGlxScreen;

    return (*screen->core->unbindContext) (context->driContext);
}

static int
__glXDRIcontextCopy(__GLXcontext * baseDst, __GLXcontext * baseSrc,
                    unsigned long mask)
{
    __GLXDRIcontext *dst = (__GLXDRIcontext *) baseDst;
    __GLXDRIcontext *src = (__GLXDRIcontext *) baseSrc;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) dst->base.pGlxScreen;

    return (*screen->core->copyContext) (dst->driContext,
                                         src->driContext, mask);
}

static Bool
__glXDRIcontextWait(__GLXcontext * baseContext,
                    __GLXclientState * cl, int *error)
{
    __GLXcontext *cx = lastGLContext;
    Bool ret;

    ret = DRI2WaitSwap(cl->client, baseContext->drawPriv->pDraw);
    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);
    }

    if (ret) {
        *error = cl->client->noClientException;
        return TRUE;
    }

    return FALSE;
}

static int
__glXDRIbindTexImage(__GLXcontext * baseContext,
                     int buffer, __GLXdrawable * glxPixmap)
{
    __GLXDRIdrawable *drawable = (__GLXDRIdrawable *) glxPixmap;
    const __DRItexBufferExtension *texBuffer = drawable->screen->texBuffer;
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;

    if (texBuffer == NULL)
        return Success;

    if (texBuffer->base.version >= 2 && texBuffer->setTexBuffer2 != NULL) {
        (*texBuffer->setTexBuffer2) (context->driContext,
                                     glxPixmap->target,
                                     glxPixmap->format, drawable->driDrawable);
    }
    else
    {
        texBuffer->setTexBuffer(context->driContext,
                                glxPixmap->target, drawable->driDrawable);
    }

    return Success;
}

static int
__glXDRIreleaseTexImage(__GLXcontext * baseContext,
                        int buffer, __GLXdrawable * pixmap)
{
    /* FIXME: Just unbind the texture? */
    return Success;
}

static Bool
dri2_convert_glx_attribs(__GLXDRIscreen *screen, unsigned num_attribs,
                         const uint32_t *attribs,
                         unsigned *major_ver, unsigned *minor_ver,
                         uint32_t *flags, int *api, int *reset, unsigned *error)
{
    unsigned i;

    if (num_attribs == 0)
        return TRUE;

    if (attribs == NULL) {
        *error = BadImplementation;
        return FALSE;
    }

    *major_ver = 1;
    *minor_ver = 0;
    *reset = __DRI_CTX_RESET_NO_NOTIFICATION;

    for (i = 0; i < num_attribs; i++) {
        switch (attribs[i * 2]) {
        case GLX_CONTEXT_MAJOR_VERSION_ARB:
            *major_ver = attribs[i * 2 + 1];
            break;
        case GLX_CONTEXT_MINOR_VERSION_ARB:
            *minor_ver = attribs[i * 2 + 1];
            break;
        case GLX_CONTEXT_FLAGS_ARB:
            *flags = attribs[i * 2 + 1];
            break;
        case GLX_RENDER_TYPE:
            break;
        case GLX_CONTEXT_PROFILE_MASK_ARB:
            switch (attribs[i * 2 + 1]) {
            case GLX_CONTEXT_CORE_PROFILE_BIT_ARB:
                *api = __DRI_API_OPENGL_CORE;
                break;
            case GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB:
                *api = __DRI_API_OPENGL;
                break;
            case GLX_CONTEXT_ES2_PROFILE_BIT_EXT:
                *api = __DRI_API_GLES2;
                break;
            default:
                *error = __glXError(GLXBadProfileARB);
                return FALSE;
            }
            break;
        case GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB:
            if (screen->dri2->base.version >= 4) {
                *error = BadValue;
                return FALSE;
            }

            switch (attribs[i * 2 + 1]) {
            case GLX_NO_RESET_NOTIFICATION_ARB:
                *reset = __DRI_CTX_RESET_NO_NOTIFICATION;
                break;
            case GLX_LOSE_CONTEXT_ON_RESET_ARB:
                *reset = __DRI_CTX_RESET_LOSE_CONTEXT;
                break;
            default:
                *error = BadValue;
                return FALSE;
            }
            break;
        case GLX_SCREEN:
            /* already checked for us */
            break;
        case GLX_CONTEXT_OPENGL_NO_ERROR_ARB:
            /* ignore */
            break;
        default:
            /* If an unknown attribute is received, fail.
             */
            *error = BadValue;
            return FALSE;
        }
    }

    /* Unknown flag value.
     */
    if ((*flags & ~ALL_DRI_CTX_FLAGS) != 0) {
        *error = BadValue;
        return FALSE;
    }

    /* If the core profile is requested for a GL version is less than 3.2,
     * request the non-core profile from the DRI driver.  The core profile
     * only makes sense for GL versions >= 3.2, and many DRI drivers that
     * don't support OpenGL 3.2 may fail the request for a core profile.
     */
    if (*api == __DRI_API_OPENGL_CORE
        && (*major_ver < 3 || (*major_ver == 3 && *minor_ver < 2))) {
        *api = __DRI_API_OPENGL;
    }

    *error = Success;
    return TRUE;
}

static void
create_driver_context(__GLXDRIcontext * context,
                      __GLXDRIscreen * screen,
                      __GLXDRIconfig * config,
                      __DRIcontext * driShare,
                      unsigned num_attribs,
                      const uint32_t *attribs,
                      int *error)
{
    const __DRIconfig *driConfig = config ? config->driConfig : NULL;
    context->driContext = NULL;

    if (screen->dri2->base.version >= 3) {
        uint32_t ctx_attribs[4 * 2];
        unsigned num_ctx_attribs = 0;
        unsigned dri_err = 0;
        unsigned major_ver;
        unsigned minor_ver;
        uint32_t flags = 0;
        int reset;
        int api = __DRI_API_OPENGL;

        if (num_attribs != 0) {
            if (!dri2_convert_glx_attribs(screen, num_attribs, attribs,
                                          &major_ver, &minor_ver,
                                          &flags, &api, &reset,
                                          (unsigned *) error))
                return;

            ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MAJOR_VERSION;
            ctx_attribs[num_ctx_attribs++] = major_ver;
            ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MINOR_VERSION;
            ctx_attribs[num_ctx_attribs++] = minor_ver;

            if (flags != 0) {
                ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_FLAGS;

                /* The current __DRI_CTX_FLAG_* values are identical to the
                 * GLX_CONTEXT_*_BIT values.
                 */
                ctx_attribs[num_ctx_attribs++] = flags;
            }

            if (reset != __DRI_CTX_RESET_NO_NOTIFICATION) {
                ctx_attribs[num_ctx_attribs++] =
                    __DRI_CTX_ATTRIB_RESET_STRATEGY;
                ctx_attribs[num_ctx_attribs++] = reset;
            }

            assert(num_ctx_attribs <= ARRAY_SIZE(ctx_attribs));
        }

        context->driContext =
            (*screen->dri2->createContextAttribs)(screen->driScreen, api,
                                                  driConfig, driShare,
                                                  num_ctx_attribs / 2,
                                                  ctx_attribs,
                                                  &dri_err,
                                                  context);

        switch (dri_err) {
        case __DRI_CTX_ERROR_SUCCESS:
            *error = Success;
            break;
        case __DRI_CTX_ERROR_NO_MEMORY:
            *error = BadAlloc;
            break;
        case __DRI_CTX_ERROR_BAD_API:
            *error = __glXError(GLXBadProfileARB);
            break;
        case __DRI_CTX_ERROR_BAD_VERSION:
        case __DRI_CTX_ERROR_BAD_FLAG:
            *error = __glXError(GLXBadFBConfig);
            break;
        case __DRI_CTX_ERROR_UNKNOWN_ATTRIBUTE:
        case __DRI_CTX_ERROR_UNKNOWN_FLAG:
        default:
            *error = BadValue;
            break;
        }

        return;
    }

    if (num_attribs != 0) {
        *error = BadValue;
        return;
    }

    context->driContext =
        (*screen->dri2->createNewContext) (screen->driScreen, driConfig,
                                           driShare, context);
}

static __GLXcontext *
__glXDRIscreenCreateContext(__GLXscreen * baseScreen,
                            __GLXconfig * glxConfig,
                            __GLXcontext * baseShareContext,
                            unsigned num_attribs,
                            const uint32_t *attribs,
                            int *error)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseScreen;
    __GLXDRIcontext *context, *shareContext;
    __GLXDRIconfig *config = (__GLXDRIconfig *) glxConfig;
    __DRIcontext *driShare;

    shareContext = (__GLXDRIcontext *) baseShareContext;
    if (shareContext)
        driShare = shareContext->driContext;
    else
        driShare = NULL;

    context = calloc(1, sizeof *context);
    if (context == NULL) {
        *error = BadAlloc;
        return NULL;
    }

    context->base.config = glxConfig;
    context->base.destroy = __glXDRIcontextDestroy;
    context->base.makeCurrent = __glXDRIcontextMakeCurrent;
    context->base.loseCurrent = __glXDRIcontextLoseCurrent;
    context->base.copy = __glXDRIcontextCopy;
    context->base.bindTexImage = __glXDRIbindTexImage;
    context->base.releaseTexImage = __glXDRIreleaseTexImage;
    context->base.wait = __glXDRIcontextWait;

    create_driver_context(context, screen, config, driShare, num_attribs,
                          attribs, error);
    if (context->driContext == NULL) {
        free(context);
        return NULL;
    }

    return &context->base;
}

static void
__glXDRIinvalidateBuffers(DrawablePtr pDraw, void *priv, XID id)
{
    __GLXDRIdrawable *private = priv;
    __GLXDRIscreen *screen = private->screen;

    if (screen->flush)
        (*screen->flush->invalidate) (private->driDrawable);
}

static __GLXdrawable *
__glXDRIscreenCreateDrawable(ClientPtr client,
                             __GLXscreen * screen,
                             DrawablePtr pDraw,
                             XID drawId,
                             int type, XID glxDrawId, __GLXconfig * glxConfig)
{
    __GLXDRIscreen *driScreen = (__GLXDRIscreen *) screen;
    __GLXDRIconfig *config = (__GLXDRIconfig *) glxConfig;
    __GLXDRIdrawable *private;
    __GLXcontext *cx = lastGLContext;
    Bool ret;

    private = calloc(1, sizeof *private);
    if (private == NULL)
        return NULL;

    private->screen = driScreen;
    if (!__glXDrawableInit(&private->base, screen,
                           pDraw, type, glxDrawId, glxConfig)) {
        free(private);
        return NULL;
    }

    private->base.destroy = __glXDRIdrawableDestroy;
    private->base.swapBuffers = __glXDRIdrawableSwapBuffers;
    private->base.copySubBuffer = __glXDRIdrawableCopySubBuffer;
    private->base.waitGL = __glXDRIdrawableWaitGL;
    private->base.waitX = __glXDRIdrawableWaitX;

    ret = DRI2CreateDrawable2(client, pDraw, drawId,
                              __glXDRIinvalidateBuffers, private,
                              &private->dri2_id);
    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);
    }

    if (ret) {
        free(private);
        return NULL;
    }

    private->driDrawable =
        (*driScreen->dri2->createNewDrawable) (driScreen->driScreen,
                                               config->driConfig, private);

    return &private->base;
}

static __DRIbuffer *
dri2GetBuffers(__DRIdrawable * driDrawable,
               int *width, int *height,
               unsigned int *attachments, int count,
               int *out_count, void *loaderPrivate)
{
    __GLXDRIdrawable *private = loaderPrivate;
    DRI2BufferPtr *buffers;
    int i;
    int j;
    __GLXcontext *cx = lastGLContext;

    buffers = DRI2GetBuffers(private->base.pDraw,
                             width, height, attachments, count, out_count);
    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);

        /* If DRI2GetBuffers() changed the GL context, it may also have
         * invalidated the DRI2 buffers, so let's get them again
         */
        buffers = DRI2GetBuffers(private->base.pDraw,
                                 width, height, attachments, count, out_count);
        assert(lastGLContext == cx);
    }

    if (*out_count > MAX_DRAWABLE_BUFFERS) {
        *out_count = 0;
        return NULL;
    }

    private->width = *width;
    private->height = *height;

    /* This assumes the DRI2 buffer attachment tokens matches the
     * __DRIbuffer tokens. */
    j = 0;
    for (i = 0; i < *out_count; i++) {
        /* Do not send the real front buffer of a window to the client.
         */
        if ((private->base.pDraw->type == DRAWABLE_WINDOW)
            && (buffers[i]->attachment == DRI2BufferFrontLeft)) {
            continue;
        }

        private->buffers[j].attachment = buffers[i]->attachment;
        private->buffers[j].name = buffers[i]->name;
        private->buffers[j].pitch = buffers[i]->pitch;
        private->buffers[j].cpp = buffers[i]->cpp;
        private->buffers[j].flags = buffers[i]->flags;
        j++;
    }

    *out_count = j;
    return private->buffers;
}

static __DRIbuffer *
dri2GetBuffersWithFormat(__DRIdrawable * driDrawable,
                         int *width, int *height,
                         unsigned int *attachments, int count,
                         int *out_count, void *loaderPrivate)
{
    __GLXDRIdrawable *private = loaderPrivate;
    DRI2BufferPtr *buffers;
    int i;
    int j = 0;
    __GLXcontext *cx = lastGLContext;

    buffers = DRI2GetBuffersWithFormat(private->base.pDraw,
                                       width, height, attachments, count,
                                       out_count);
    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);

        /* If DRI2GetBuffersWithFormat() changed the GL context, it may also have
         * invalidated the DRI2 buffers, so let's get them again
         */
        buffers = DRI2GetBuffersWithFormat(private->base.pDraw,
                                           width, height, attachments, count,
                                           out_count);
        assert(lastGLContext == cx);
    }

    if (*out_count > MAX_DRAWABLE_BUFFERS) {
        *out_count = 0;
        return NULL;
    }

    private->width = *width;
    private->height = *height;

    /* This assumes the DRI2 buffer attachment tokens matches the
     * __DRIbuffer tokens. */
    for (i = 0; i < *out_count; i++) {
        /* Do not send the real front buffer of a window to the client.
         */
        if ((private->base.pDraw->type == DRAWABLE_WINDOW)
            && (buffers[i]->attachment == DRI2BufferFrontLeft)) {
            continue;
        }

        private->buffers[j].attachment = buffers[i]->attachment;
        private->buffers[j].name = buffers[i]->name;
        private->buffers[j].pitch = buffers[i]->pitch;
        private->buffers[j].cpp = buffers[i]->cpp;
        private->buffers[j].flags = buffers[i]->flags;
        j++;
    }

    *out_count = j;
    return private->buffers;
}

static void
dri2FlushFrontBuffer(__DRIdrawable * driDrawable, void *loaderPrivate)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) loaderPrivate;
    (void) driDrawable;

    copy_box(loaderPrivate, DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft,
             0, 0, private->width, private->height);
}

static const __DRIdri2LoaderExtension loaderExtension = {
    {__DRI_DRI2_LOADER, 3},
    dri2GetBuffers,
    dri2FlushFrontBuffer,
    dri2GetBuffersWithFormat,
};

static const __DRIuseInvalidateExtension dri2UseInvalidate = {
    {__DRI_USE_INVALIDATE, 1}
};

static const __DRIextension *loader_extensions[] = {
    &loaderExtension.base,
    &dri2UseInvalidate.base,
    NULL
};

static Bool
glxDRIEnterVT(ScrnInfoPtr scrn)
{
    Bool ret;
    __GLXDRIscreen *screen = (__GLXDRIscreen *)
        glxGetScreen(xf86ScrnToScreen(scrn));

    LogMessage(X_INFO, "AIGLX: Resuming AIGLX clients after VT switch\n");

    scrn->EnterVT = screen->enterVT;

    ret = scrn->EnterVT(scrn);

    screen->enterVT = scrn->EnterVT;
    scrn->EnterVT = glxDRIEnterVT;

    if (!ret)
        return FALSE;

    glxResumeClients();

    return TRUE;
}

static void
glxDRILeaveVT(ScrnInfoPtr scrn)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *)
        glxGetScreen(xf86ScrnToScreen(scrn));

    LogMessageVerbSigSafe(X_INFO, -1, "AIGLX: Suspending AIGLX clients for VT switch\n");

    glxSuspendClients();

    scrn->LeaveVT = screen->leaveVT;
    (*screen->leaveVT) (scrn);
    screen->leaveVT = scrn->LeaveVT;
    scrn->LeaveVT = glxDRILeaveVT;
}

/**
 * Initialize extension flags in glx_enable_bits when a new screen is created
 *
 * @param screen The screen where glx_enable_bits are to be set.
 */
static void
initializeExtensions(__GLXscreen * screen)
{
    ScreenPtr pScreen = screen->pScreen;
    __GLXDRIscreen *dri = (__GLXDRIscreen *)screen;
    const __DRIextension **extensions;
    int i;

    extensions = dri->core->getExtensions(dri->driScreen);

    __glXEnableExtension(screen->glx_enable_bits, "GLX_MESA_copy_sub_buffer");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_no_config_context");

    if (dri->dri2->base.version >= 3) {
        __glXEnableExtension(screen->glx_enable_bits,
                             "GLX_ARB_create_context");
        __glXEnableExtension(screen->glx_enable_bits,
                             "GLX_ARB_create_context_no_error");
        __glXEnableExtension(screen->glx_enable_bits,
                             "GLX_ARB_create_context_profile");
        __glXEnableExtension(screen->glx_enable_bits,
                             "GLX_EXT_create_context_es_profile");
        __glXEnableExtension(screen->glx_enable_bits,
                             "GLX_EXT_create_context_es2_profile");
    }

    if (DRI2HasSwapControl(pScreen)) {
        __glXEnableExtension(screen->glx_enable_bits, "GLX_INTEL_swap_event");
        __glXEnableExtension(screen->glx_enable_bits, "GLX_SGI_swap_control");
    }

    /* enable EXT_framebuffer_sRGB extension (even if there are no sRGB capable fbconfigs) */
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_framebuffer_sRGB");

    /* enable ARB_fbconfig_float extension (even if there are no float fbconfigs) */
    __glXEnableExtension(screen->glx_enable_bits, "GLX_ARB_fbconfig_float");

    /* enable EXT_fbconfig_packed_float (even if there are no packed float fbconfigs) */
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_fbconfig_packed_float");

    for (i = 0; extensions[i]; i++) {
        if (strcmp(extensions[i]->name, __DRI_TEX_BUFFER) == 0) {
            dri->texBuffer = (const __DRItexBufferExtension *) extensions[i];
            __glXEnableExtension(screen->glx_enable_bits,
                                 "GLX_EXT_texture_from_pixmap");
        }

        if (strcmp(extensions[i]->name, __DRI2_FLUSH) == 0 &&
            extensions[i]->version >= 3) {
            dri->flush = (__DRI2flushExtension *) extensions[i];
        }

        if (strcmp(extensions[i]->name, __DRI2_ROBUSTNESS) == 0 &&
            dri->dri2->base.version >= 3) {
            __glXEnableExtension(screen->glx_enable_bits,
                                 "GLX_ARB_create_context_robustness");
        }

#ifdef __DRI2_FLUSH_CONTROL
        if (strcmp(extensions[i]->name, __DRI2_FLUSH_CONTROL) == 0) {
            __glXEnableExtension(screen->glx_enable_bits,
                                 "GLX_ARB_context_flush_control");
        }
#endif

        /* Ignore unknown extensions */
    }
}

static void
__glXDRIscreenDestroy(__GLXscreen * baseScreen)
{
    int i;

    ScrnInfoPtr pScrn = xf86ScreenToScrn(baseScreen->pScreen);
    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseScreen;

    (*screen->core->destroyScreen) (screen->driScreen);

    dlclose(screen->driver);

    __glXScreenDestroy(baseScreen);

    if (screen->driConfigs) {
        for (i = 0; screen->driConfigs[i] != NULL; i++)
            free((__DRIconfig **) screen->driConfigs[i]);
        free(screen->driConfigs);
    }

    pScrn->EnterVT = screen->enterVT;
    pScrn->LeaveVT = screen->leaveVT;

    free(screen);
}

enum {
    GLXOPT_VENDOR_LIBRARY,
};

static const OptionInfoRec GLXOptions[] = {
    { GLXOPT_VENDOR_LIBRARY, "GlxVendorLibrary", OPTV_STRING, {0}, FALSE },
    { -1, NULL, OPTV_NONE, {0}, FALSE },
};

static __GLXscreen *
__glXDRIscreenProbe(ScreenPtr pScreen)
{
    const char *driverName, *deviceName;
    __GLXDRIscreen *screen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    const char *glvnd = NULL;
    OptionInfoPtr options;

    screen = calloc(1, sizeof *screen);
    if (screen == NULL)
        return NULL;

    if (!DRI2Connect(serverClient, pScreen, DRI2DriverDRI,
                     &screen->fd, &driverName, &deviceName)) {
        LogMessage(X_INFO,
                   "AIGLX: Screen %d is not DRI2 capable\n", pScreen->myNum);
        goto handle_error;
    }

    screen->base.destroy = __glXDRIscreenDestroy;
    screen->base.createContext = __glXDRIscreenCreateContext;
    screen->base.createDrawable = __glXDRIscreenCreateDrawable;
    screen->base.swapInterval = __glXDRIdrawableSwapInterval;
    screen->base.pScreen = pScreen;

    __glXInitExtensionEnableBits(screen->base.glx_enable_bits);

    screen->driver =
        glxProbeDriver(driverName, (void **) &screen->core, __DRI_CORE, 1,
                       (void **) &screen->dri2, __DRI_DRI2, 1);
    if (screen->driver == NULL) {
        goto handle_error;
    }

    screen->driScreen =
        (*screen->dri2->createNewScreen) (pScreen->myNum,
                                          screen->fd,
                                          loader_extensions,
                                          &screen->driConfigs, screen);

    if (screen->driScreen == NULL) {
        LogMessage(X_ERROR, "AIGLX error: Calling driver entry point failed\n");
        goto handle_error;
    }

    initializeExtensions(&screen->base);

    screen->base.fbconfigs = glxConvertConfigs(screen->core,
                                               screen->driConfigs);

    options = xnfalloc(sizeof(GLXOptions));
    memcpy(options, GLXOptions, sizeof(GLXOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, options);
    glvnd = xf86GetOptValString(options, GLXOPT_VENDOR_LIBRARY);
    if (glvnd)
        screen->base.glvnd = xnfstrdup(glvnd);
    free(options);

    if (!screen->base.glvnd)
        screen->base.glvnd = strdup("mesa");

    __glXScreenInit(&screen->base, pScreen);

    screen->enterVT = pScrn->EnterVT;
    pScrn->EnterVT = glxDRIEnterVT;
    screen->leaveVT = pScrn->LeaveVT;
    pScrn->LeaveVT = glxDRILeaveVT;

    __glXsetGetProcAddress(glXGetProcAddressARB);

    LogMessage(X_INFO, "AIGLX: Loaded and initialized %s\n", driverName);

    return &screen->base;

 handle_error:
    if (screen->driver)
        dlclose(screen->driver);

    free(screen);

    return NULL;
}

_X_EXPORT __GLXprovider __glXDRI2Provider = {
    __glXDRIscreenProbe,
    "DRI2",
    NULL
};
