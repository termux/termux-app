/*
 * Copyright © 2008 George Sapountzis <gsap7@yahoo.gr>
 * Copyright © 2008 Red Hat, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <dlfcn.h>

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include <GL/glxtokens.h>

#include "scrnintstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "os.h"

#include "glxserver.h"
#include "glxutil.h"
#include "glxdricommon.h"

#include "extension_string.h"

/* RTLD_LOCAL is not defined on Cygwin */
#ifdef __CYGWIN__
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif
#endif

typedef struct __GLXDRIscreen __GLXDRIscreen;
typedef struct __GLXDRIcontext __GLXDRIcontext;
typedef struct __GLXDRIdrawable __GLXDRIdrawable;

struct __GLXDRIscreen {
    __GLXscreen base;
    __DRIscreen *driScreen;
    void *driver;

    const __DRIcoreExtension *core;
    const __DRIswrastExtension *swrast;
    const __DRIcopySubBufferExtension *copySubBuffer;
    const __DRItexBufferExtension *texBuffer;
    const __DRIconfig **driConfigs;
};

struct __GLXDRIcontext {
    __GLXcontext base;
    __DRIcontext *driContext;
};

struct __GLXDRIdrawable {
    __GLXdrawable base;
    __DRIdrawable *driDrawable;
    __GLXDRIscreen *screen;
};

/* white lie */
extern glx_func_ptr glXGetProcAddressARB(const char *);

static void
__glXDRIdrawableDestroy(__GLXdrawable * drawable)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;
    const __DRIcoreExtension *core = private->screen->core;

    (*core->destroyDrawable) (private->driDrawable);

    __glXDrawableRelease(drawable);

    free(private);
}

static GLboolean
__glXDRIdrawableSwapBuffers(ClientPtr client, __GLXdrawable * drawable)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;
    const __DRIcoreExtension *core = private->screen->core;

    (*core->swapBuffers) (private->driDrawable);

    return TRUE;
}

static void
__glXDRIdrawableCopySubBuffer(__GLXdrawable * basePrivate,
                              int x, int y, int w, int h)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) basePrivate;
    const __DRIcopySubBufferExtension *copySubBuffer =
        private->screen->copySubBuffer;

    if (copySubBuffer)
        (*copySubBuffer->copySubBuffer) (private->driDrawable, x, y, w, h);
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

static int
__glXDRIbindTexImage(__GLXcontext * baseContext,
                     int buffer, __GLXdrawable * glxPixmap)
{
    __GLXDRIdrawable *drawable = (__GLXDRIdrawable *) glxPixmap;
    const __DRItexBufferExtension *texBuffer = drawable->screen->texBuffer;
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;

    if (texBuffer == NULL)
        return Success;

#if __DRI_TEX_BUFFER_VERSION >= 2
    if (texBuffer->base.version >= 2 && texBuffer->setTexBuffer2 != NULL) {
        (*texBuffer->setTexBuffer2) (context->driContext,
                                     glxPixmap->target,
                                     glxPixmap->format, drawable->driDrawable);
    }
    else
#endif
        texBuffer->setTexBuffer(context->driContext,
                                glxPixmap->target, drawable->driDrawable);

    return Success;
}

static int
__glXDRIreleaseTexImage(__GLXcontext * baseContext,
                        int buffer, __GLXdrawable * pixmap)
{
    /* FIXME: Just unbind the texture? */
    return Success;
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
    const __DRIconfig *driConfig = config ? config->driConfig : NULL;
    const __DRIcoreExtension *core = screen->core;
    __DRIcontext *driShare;

    /* DRISWRAST won't support createContextAttribs, so these parameters will
     * never be used.
     */
    (void) num_attribs;
    (void) attribs;
    (void) error;

    shareContext = (__GLXDRIcontext *) baseShareContext;
    if (shareContext)
        driShare = shareContext->driContext;
    else
        driShare = NULL;

    context = calloc(1, sizeof *context);
    if (context == NULL)
        return NULL;

    context->base.config = glxConfig;
    context->base.destroy = __glXDRIcontextDestroy;
    context->base.makeCurrent = __glXDRIcontextMakeCurrent;
    context->base.loseCurrent = __glXDRIcontextLoseCurrent;
    context->base.copy = __glXDRIcontextCopy;
    context->base.bindTexImage = __glXDRIbindTexImage;
    context->base.releaseTexImage = __glXDRIreleaseTexImage;

    context->driContext =
        (*core->createNewContext) (screen->driScreen, driConfig, driShare,
                                   context);

    return &context->base;
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

    private->driDrawable =
        (*driScreen->swrast->createNewDrawable) (driScreen->driScreen,
                                                 config->driConfig, private);

    return &private->base;
}

static void
swrastGetDrawableInfo(__DRIdrawable * draw,
                      int *x, int *y, int *w, int *h, void *loaderPrivate)
{
    __GLXDRIdrawable *drawable = loaderPrivate;
    DrawablePtr pDraw = drawable->base.pDraw;

    *x = pDraw->x;
    *y = pDraw->y;
    *w = pDraw->width;
    *h = pDraw->height;
}

static void
swrastPutImage(__DRIdrawable * draw, int op,
               int x, int y, int w, int h, char *data, void *loaderPrivate)
{
    __GLXDRIdrawable *drawable = loaderPrivate;
    DrawablePtr pDraw = drawable->base.pDraw;
    GCPtr gc;
    __GLXcontext *cx = lastGLContext;

    if ((gc = GetScratchGC(pDraw->depth, pDraw->pScreen))) {
        ValidateGC(pDraw, gc);
        gc->ops->PutImage(pDraw, gc, pDraw->depth, x, y, w, h, 0, ZPixmap,
                          data);
        FreeScratchGC(gc);
    }

    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);
    }
}

static void
swrastGetImage(__DRIdrawable * draw,
               int x, int y, int w, int h, char *data, void *loaderPrivate)
{
    __GLXDRIdrawable *drawable = loaderPrivate;
    DrawablePtr pDraw = drawable->base.pDraw;
    ScreenPtr pScreen = pDraw->pScreen;
    __GLXcontext *cx = lastGLContext;

    pScreen->SourceValidate(pDraw, x, y, w, h, IncludeInferiors);
    pScreen->GetImage(pDraw, x, y, w, h, ZPixmap, ~0L, data);
    if (cx != lastGLContext) {
        lastGLContext = cx;
        cx->makeCurrent(cx);
    }
}

static const __DRIswrastLoaderExtension swrastLoaderExtension = {
    {__DRI_SWRAST_LOADER, 1},
    swrastGetDrawableInfo,
    swrastPutImage,
    swrastGetImage
};

static const __DRIextension *loader_extensions[] = {
    &swrastLoaderExtension.base,
    NULL
};

static void
initializeExtensions(__GLXscreen * screen)
{
    const __DRIextension **extensions;
    __GLXDRIscreen *dri = (__GLXDRIscreen *)screen;
    int i;

    __glXEnableExtension(screen->glx_enable_bits, "GLX_MESA_copy_sub_buffer");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_no_config_context");

    if (dri->swrast->base.version >= 3) {
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

    /* these are harmless to enable unconditionally */
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_framebuffer_sRGB");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_ARB_fbconfig_float");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_fbconfig_packed_float");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_texture_from_pixmap");

    extensions = dri->core->getExtensions(dri->driScreen);

    for (i = 0; extensions[i]; i++) {
        if (strcmp(extensions[i]->name, __DRI_COPY_SUB_BUFFER) == 0) {
            dri->copySubBuffer =
                (const __DRIcopySubBufferExtension *) extensions[i];
        }

        if (strcmp(extensions[i]->name, __DRI_TEX_BUFFER) == 0) {
            dri->texBuffer = (const __DRItexBufferExtension *) extensions[i];
        }

#ifdef __DRI2_FLUSH_CONTROL
        if (strcmp(extensions[i]->name, __DRI2_FLUSH_CONTROL) == 0) {
            __glXEnableExtension(screen->glx_enable_bits,
                                 "GLX_ARB_context_flush_control");
        }
#endif

    }
}

static void
__glXDRIscreenDestroy(__GLXscreen * baseScreen)
{
    int i;

    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseScreen;

    (*screen->core->destroyScreen) (screen->driScreen);

    dlclose(screen->driver);

    __glXScreenDestroy(baseScreen);

    if (screen->driConfigs) {
        for (i = 0; screen->driConfigs[i] != NULL; i++)
            free((__DRIconfig **) screen->driConfigs[i]);
        free(screen->driConfigs);
    }

    free(screen);
}

static __GLXscreen *
__glXDRIscreenProbe(ScreenPtr pScreen)
{
    const char *driverName = "swrast";
    __GLXDRIscreen *screen;

    screen = calloc(1, sizeof *screen);
    if (screen == NULL)
        return NULL;

    screen->base.destroy = __glXDRIscreenDestroy;
    screen->base.createContext = __glXDRIscreenCreateContext;
    screen->base.createDrawable = __glXDRIscreenCreateDrawable;
    screen->base.swapInterval = NULL;
    screen->base.pScreen = pScreen;

    __glXInitExtensionEnableBits(screen->base.glx_enable_bits);

    screen->driver = glxProbeDriver(driverName,
                                    (void **) &screen->core,
                                    __DRI_CORE, 1,
                                    (void **) &screen->swrast,
                                    __DRI_SWRAST, 1);
    if (screen->driver == NULL) {
        goto handle_error;
    }

    screen->driScreen =
        (*screen->swrast->createNewScreen) (pScreen->myNum,
                                            loader_extensions,
                                            &screen->driConfigs, screen);

    if (screen->driScreen == NULL) {
        LogMessage(X_ERROR, "IGLX error: Calling driver entry point failed\n");
        goto handle_error;
    }

    initializeExtensions(&screen->base);

    screen->base.fbconfigs = glxConvertConfigs(screen->core,
                                               screen->driConfigs);

#if !defined(XQUARTZ) && !defined(WIN32)
    screen->base.glvnd = strdup("mesa");
#endif
    __glXScreenInit(&screen->base, pScreen);

    __glXsetGetProcAddress(glXGetProcAddressARB);

    LogMessage(X_INFO, "IGLX: Loaded and initialized %s\n", driverName);

    return &screen->base;

 handle_error:
    if (screen->driver)
        dlclose(screen->driver);

    free(screen);

    LogMessage(X_ERROR, "GLX: could not load software renderer\n");

    return NULL;
}

__GLXprovider __glXDRISWRastProvider = {
    __glXDRIscreenProbe,
    "DRISWRAST",
    NULL
};
