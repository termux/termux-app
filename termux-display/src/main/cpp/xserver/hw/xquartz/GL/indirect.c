/*
 * GLX implementation that uses Apple's OpenGL.framework
 * (Indirect rendering path -- it's also used for some direct mode code too)
 *
 * Copyright (c) 2007-2012 Apple Inc.
 * Copyright (c) 2004 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002 Greg Parker. All Rights Reserved.
 *
 * Portions of this file are copied from Mesa's xf86glx.c,
 * which contains the following copyright:
 *
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <dlfcn.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>     /* Just to prevent glxserver.h from loading mesa's and colliding with OpenGL.h */

#include <X11/Xproto.h>
#include <GL/glxproto.h>

#include <glxserver.h>
#include <glxutil.h>

#include "x-hash.h"

#include "visualConfigs.h"
#include "dri.h"
#include "extension_string.h"

#include "darwin.h"
#define GLAQUA_DEBUG_MSG(msg, args ...) ASL_LOG(ASL_LEVEL_DEBUG, "GLXAqua", \
                                                msg, \
                                                ## args)

__GLXprovider *
GlxGetDRISWrastProvider(void);

static void
setup_dispatch_table(void);
GLuint
__glFloorLog2(GLuint val);
void
warn_func(void * p1, char *format, ...);

// some prototypes
static __GLXscreen *
__glXAquaScreenProbe(ScreenPtr pScreen);
static __GLXdrawable *
__glXAquaScreenCreateDrawable(ClientPtr client, __GLXscreen *screen,
                              DrawablePtr pDraw, XID drawId, int type,
                              XID glxDrawId,
                              __GLXconfig *conf);

static void
__glXAquaContextDestroy(__GLXcontext *baseContext);
static int
__glXAquaContextMakeCurrent(__GLXcontext *baseContext);
static int
__glXAquaContextLoseCurrent(__GLXcontext *baseContext);
static int
__glXAquaContextCopy(__GLXcontext *baseDst, __GLXcontext *baseSrc,
                     unsigned long mask);

static CGLPixelFormatObj
makeFormat(__GLXconfig *conf);

__GLXprovider __glXDRISWRastProvider = {
    __glXAquaScreenProbe,
    "Core OpenGL",
    NULL
};

typedef struct __GLXAquaScreen __GLXAquaScreen;
typedef struct __GLXAquaContext __GLXAquaContext;
typedef struct __GLXAquaDrawable __GLXAquaDrawable;

/*
 * The following structs must keep the base as the first member.
 * It's used to treat the start of the struct as a different struct
 * in GLX.
 *
 * Note: these structs should be initialized with xcalloc or memset
 * prior to usage, and some of them require initializing
 * the base with function pointers.
 */
struct __GLXAquaScreen {
    __GLXscreen base;
};

struct __GLXAquaContext {
    __GLXcontext base;
    CGLContextObj ctx;
    CGLPixelFormatObj pixelFormat;
    xp_surface_id sid;
    unsigned isAttached : 1;
};

struct __GLXAquaDrawable {
    __GLXdrawable base;
    DrawablePtr pDraw;
    xp_surface_id sid;
    __GLXAquaContext *context;
};

static __GLXcontext *
__glXAquaScreenCreateContext(__GLXscreen *screen,
                             __GLXconfig *conf,
                             __GLXcontext *baseShareContext,
                             unsigned num_attribs,
                             const uint32_t *attribs,
                             int *error)
{
    __GLXAquaContext *context;
    __GLXAquaContext *shareContext = (__GLXAquaContext *)baseShareContext;
    CGLError gl_err;

    /* Unused (for now?) */
    (void)num_attribs;
    (void)attribs;
    (void)error;

    GLAQUA_DEBUG_MSG("glXAquaScreenCreateContext\n");

    context = calloc(1, sizeof(__GLXAquaContext));

    if (context == NULL)
        return NULL;

    memset(context, 0, sizeof *context);

    context->base.pGlxScreen = screen;
    context->base.config = conf;
    context->base.destroy = __glXAquaContextDestroy;
    context->base.makeCurrent = __glXAquaContextMakeCurrent;
    context->base.loseCurrent = __glXAquaContextLoseCurrent;
    context->base.copy = __glXAquaContextCopy;
    /*FIXME verify that the context->base is fully initialized. */

    context->pixelFormat = makeFormat(conf);

    if (!context->pixelFormat) {
        free(context);
        return NULL;
    }

    context->ctx = NULL;
    gl_err = CGLCreateContext(context->pixelFormat,
                              shareContext ? shareContext->ctx : NULL,
                              &context->ctx);

    if (gl_err != 0) {
        ErrorF("CGLCreateContext error: %s\n", CGLErrorString(gl_err));
        CGLDestroyPixelFormat(context->pixelFormat);
        free(context);
        return NULL;
    }

    setup_dispatch_table();
    GLAQUA_DEBUG_MSG("glAquaCreateContext done\n");

    return &context->base;
}

/* maps from surface id -> list of __GLcontext */
static x_hash_table *surface_hash;

static void
__glXAquaContextDestroy(__GLXcontext *baseContext)
{
    x_list *lst;

    __GLXAquaContext *context = (__GLXAquaContext *)baseContext;

    GLAQUA_DEBUG_MSG("glAquaContextDestroy (ctx %p)\n", baseContext);
    if (context != NULL) {
        if (context->sid != 0 && surface_hash != NULL) {
            lst =
                x_hash_table_lookup(surface_hash, x_cvt_uint_to_vptr(
                                        context->sid), NULL);
            lst = x_list_remove(lst, context);
            x_hash_table_insert(surface_hash, x_cvt_uint_to_vptr(
                                    context->sid), lst);
        }

        if (context->ctx != NULL)
            CGLDestroyContext(context->ctx);

        if (context->pixelFormat != NULL)
            CGLDestroyPixelFormat(context->pixelFormat);

        free(context);
    }
}

static int
__glXAquaContextLoseCurrent(__GLXcontext *baseContext)
{
    CGLError gl_err;

    GLAQUA_DEBUG_MSG("glAquaLoseCurrent (ctx 0x%p)\n", baseContext);

    gl_err = CGLSetCurrentContext(NULL);
    if (gl_err != 0)
        ErrorF("CGLSetCurrentContext error: %s\n", CGLErrorString(gl_err));

    /*
     * There should be no need to set __glXLastContext to NULL here, because
     * glxcmds.c does it as part of the context cache flush after calling
     * this.
     */

    return GL_TRUE;
}

/* Called when a surface is destroyed as a side effect of destroying
   the window it's attached to. */
static void
surface_notify(void *_arg, void *data)
{
    DRISurfaceNotifyArg *arg = (DRISurfaceNotifyArg *)_arg;
    __GLXAquaDrawable *draw = (__GLXAquaDrawable *)data;
    __GLXAquaContext *context;
    x_list *lst;
    if (_arg == NULL || data == NULL) {
        ErrorF("surface_notify called with bad params");
        return;
    }

    GLAQUA_DEBUG_MSG("surface_notify(%p, %p)\n", _arg, data);
    switch (arg->kind) {
    case AppleDRISurfaceNotifyDestroyed:
        if (surface_hash != NULL)
            x_hash_table_remove(surface_hash, x_cvt_uint_to_vptr(arg->id));
        draw->pDraw = NULL;
        draw->sid = 0;
        break;

    case AppleDRISurfaceNotifyChanged:
        if (surface_hash != NULL) {
            lst =
                x_hash_table_lookup(surface_hash, x_cvt_uint_to_vptr(
                                        arg->id), NULL);
            for (; lst != NULL; lst = lst->next) {
                context = lst->data;
                xp_update_gl_context(context->ctx);
            }
        }
        break;

    default:
        ErrorF("surface_notify: unknown kind %d\n", arg->kind);
        break;
    }
}

static BOOL
attach(__GLXAquaContext *context, __GLXAquaDrawable *draw)
{
    DrawablePtr pDraw;

    GLAQUA_DEBUG_MSG("attach(%p, %p)\n", context, draw);

    if (NULL == context || NULL == draw)
        return TRUE;

    pDraw = draw->base.pDraw;

    if (NULL == pDraw) {
        ErrorF("%s:%s() pDraw is NULL!\n", __FILE__, __func__);
        return TRUE;
    }

    if (draw->sid == 0) {
        //if (!quartzProcs->CreateSurface(pDraw->pScreen, pDraw->id, pDraw,
        if (!DRICreateSurface(pDraw->pScreen, pDraw->id, pDraw,
                              0, &draw->sid, NULL,
                              surface_notify, draw))
            return TRUE;
        draw->pDraw = pDraw;
    }

    if (!context->isAttached || context->sid != draw->sid) {
        x_list *lst;

        if (xp_attach_gl_context(context->ctx, draw->sid) != Success) {
            //quartzProcs->DestroySurface(pDraw->pScreen, pDraw->id, pDraw,
            DRIDestroySurface(pDraw->pScreen, pDraw->id, pDraw,
                              surface_notify, draw);
            if (surface_hash != NULL)
                x_hash_table_remove(surface_hash,
                                    x_cvt_uint_to_vptr(draw->sid));

            draw->sid = 0;
            return TRUE;
        }

        context->isAttached = TRUE;
        context->sid = draw->sid;

        if (surface_hash == NULL)
            surface_hash = x_hash_table_new(NULL, NULL, NULL, NULL);

        lst =
            x_hash_table_lookup(surface_hash, x_cvt_uint_to_vptr(
                                    context->sid), NULL);
        if (x_list_find(lst, context) == NULL) {
            lst = x_list_prepend(lst, context);
            x_hash_table_insert(surface_hash, x_cvt_uint_to_vptr(
                                    context->sid), lst);
        }

        GLAQUA_DEBUG_MSG("attached 0x%x to 0x%x\n", (unsigned int)pDraw->id,
                         (unsigned int)draw->sid);
    }

    draw->context = context;

    return FALSE;
}

#if 0     // unused
static void
unattach(__GLXAquaContext *context)
{
    x_list *lst;
    GLAQUA_DEBUG_MSG("unattach\n");
    if (context == NULL) {
        ErrorF("Tried to unattach a null context\n");
        return;
    }
    if (context->isAttached) {
        GLAQUA_DEBUG_MSG("unattaching\n");

        if (surface_hash != NULL) {
            lst = x_hash_table_lookup(surface_hash, (void *)context->sid,
                                      NULL);
            lst = x_list_remove(lst, context);
            x_hash_table_insert(surface_hash, (void *)context->sid, lst);
        }

        CGLClearDrawable(context->ctx);
        context->isAttached = FALSE;
        context->sid = 0;
    }
}
#endif

static int
__glXAquaContextMakeCurrent(__GLXcontext *baseContext)
{
    CGLError gl_err;
    __GLXAquaContext *context = (__GLXAquaContext *)baseContext;
    __GLXAquaDrawable *drawPriv = (__GLXAquaDrawable *)context->base.drawPriv;

    GLAQUA_DEBUG_MSG("glAquaMakeCurrent (ctx 0x%p)\n", baseContext);

    if (context->base.drawPriv != context->base.readPriv)
        return 0;

    if (attach(context, drawPriv))
        return /*error*/ 0;

    gl_err = CGLSetCurrentContext(context->ctx);
    if (gl_err != 0)
        ErrorF("CGLSetCurrentContext error: %s\n", CGLErrorString(gl_err));

    return gl_err == 0;
}

static int
__glXAquaContextCopy(__GLXcontext *baseDst, __GLXcontext *baseSrc,
                     unsigned long mask)
{
    CGLError gl_err;

    __GLXAquaContext *dst = (__GLXAquaContext *)baseDst;
    __GLXAquaContext *src = (__GLXAquaContext *)baseSrc;

    GLAQUA_DEBUG_MSG("GLXAquaContextCopy\n");

    gl_err = CGLCopyContext(src->ctx, dst->ctx, mask);
    if (gl_err != 0)
        ErrorF("CGLCopyContext error: %s\n", CGLErrorString(gl_err));

    return gl_err == 0;
}

/* Drawing surface notification callbacks */
static GLboolean
__glXAquaDrawableSwapBuffers(ClientPtr client, __GLXdrawable *base)
{
    CGLError err;
    __GLXAquaDrawable *drawable;

    //    GLAQUA_DEBUG_MSG("glAquaDrawableSwapBuffers(%p)\n",base);

    if (!base) {
        ErrorF("%s passed NULL\n", __func__);
        return GL_FALSE;
    }

    drawable = (__GLXAquaDrawable *)base;

    if (NULL == drawable->context) {
        ErrorF("%s called with a NULL->context for drawable %p!\n",
               __func__, (void *)drawable);
        return GL_FALSE;
    }

    err = CGLFlushDrawable(drawable->context->ctx);

    if (kCGLNoError != err) {
        ErrorF("CGLFlushDrawable error: %s in %s\n", CGLErrorString(err),
               __func__);
        return GL_FALSE;
    }

    return GL_TRUE;
}

static CGLPixelFormatObj
makeFormat(__GLXconfig *conf)
{
    CGLPixelFormatAttribute attr[64];
    CGLPixelFormatObj fobj;
    GLint formats;
    CGLError error;
    int i = 0;

    if (conf->doubleBufferMode)
        attr[i++] = kCGLPFADoubleBuffer;

    if (conf->stereoMode)
        attr[i++] = kCGLPFAStereo;

    attr[i++] = kCGLPFAColorSize;
    attr[i++] = conf->redBits + conf->greenBits + conf->blueBits;
    attr[i++] = kCGLPFAAlphaSize;
    attr[i++] = conf->alphaBits;

    if ((conf->accumRedBits + conf->accumGreenBits + conf->accumBlueBits +
         conf->accumAlphaBits) > 0) {

        attr[i++] = kCGLPFAAccumSize;
        attr[i++] = conf->accumRedBits + conf->accumGreenBits
                    + conf->accumBlueBits + conf->accumAlphaBits;
    }

    attr[i++] = kCGLPFADepthSize;
    attr[i++] = conf->depthBits;

    if (conf->stencilBits) {
        attr[i++] = kCGLPFAStencilSize;
        attr[i++] = conf->stencilBits;
    }

    if (conf->numAuxBuffers > 0) {
        attr[i++] = kCGLPFAAuxBuffers;
        attr[i++] = conf->numAuxBuffers;
    }

    if (conf->sampleBuffers > 0) {
        attr[i++] = kCGLPFASampleBuffers;
        attr[i++] = conf->sampleBuffers;
        attr[i++] = kCGLPFASamples;
        attr[i++] = conf->samples;
    }

    attr[i] = 0;

    error = CGLChoosePixelFormat(attr, &fobj, &formats);
    if (error) {
        ErrorF("error: creating pixel format %s\n", CGLErrorString(error));
        return NULL;
    }

    return fobj;
}

static void
__glXAquaScreenDestroy(__GLXscreen *screen)
{

    GLAQUA_DEBUG_MSG("glXAquaScreenDestroy(%p)\n", screen);
    __glXScreenDestroy(screen);

    free(screen);
}

/* This is called by __glXInitScreens(). */
static __GLXscreen *
__glXAquaScreenProbe(ScreenPtr pScreen)
{
    __GLXAquaScreen *screen;

    GLAQUA_DEBUG_MSG("glXAquaScreenProbe\n");

    if (pScreen == NULL)
        return NULL;

    screen = calloc(1, sizeof *screen);

    if (NULL == screen)
        return NULL;

    screen->base.destroy = __glXAquaScreenDestroy;
    screen->base.createContext = __glXAquaScreenCreateContext;
    screen->base.createDrawable = __glXAquaScreenCreateDrawable;
    screen->base.swapInterval = /*FIXME*/ NULL;
    screen->base.pScreen = pScreen;

    screen->base.fbconfigs = __glXAquaCreateVisualConfigs(
        &screen->base.numFBConfigs, pScreen->myNum);

    __glXInitExtensionEnableBits(screen->base.glx_enable_bits);
    __glXScreenInit(&screen->base, pScreen);

    return &screen->base;
}

#if 0 // unused
static void
__glXAquaDrawableCopySubBuffer(__GLXdrawable *drawable,
                               int x, int y, int w, int h)
{
    /*TODO finish me*/
}
#endif

static void
__glXAquaDrawableDestroy(__GLXdrawable *base)
{
    /* gstaplin: base is the head of the structure, so it's at the same
     * offset in memory.
     * Is this safe with strict aliasing?   I noticed that the other dri code
     * does this too...
     */
    __GLXAquaDrawable *glxPriv = (__GLXAquaDrawable *)base;

    GLAQUA_DEBUG_MSG("TRACE");

    /* It doesn't work to call DRIDestroySurface here, the drawable's
       already gone.. But dri.c notices the window destruction and
       frees the surface itself. */

    /*gstaplin: verify the statement above.  The surface destroy
       *messages weren't making it through, and may still not be.
       *We need a good test case for surface creation and destruction.
       *We also need a good way to enable introspection on the server
       *to validate the test, beyond using gdb with print.
     */

    free(glxPriv);
}

static __GLXdrawable *
__glXAquaScreenCreateDrawable(ClientPtr client,
                              __GLXscreen *screen,
                              DrawablePtr pDraw,
                              XID drawId,
                              int type,
                              XID glxDrawId,
                              __GLXconfig *conf)
{
    __GLXAquaDrawable *glxPriv;

    glxPriv = malloc(sizeof *glxPriv);

    if (glxPriv == NULL)
        return NULL;

    memset(glxPriv, 0, sizeof *glxPriv);

    if (!__glXDrawableInit(&glxPriv->base, screen, pDraw, type, glxDrawId,
                           conf)) {
        free(glxPriv);
        return NULL;
    }

    glxPriv->base.destroy = __glXAquaDrawableDestroy;
    glxPriv->base.swapBuffers = __glXAquaDrawableSwapBuffers;
    glxPriv->base.copySubBuffer = NULL; /* __glXAquaDrawableCopySubBuffer; */

    glxPriv->pDraw = pDraw;
    glxPriv->sid = 0;
    glxPriv->context = NULL;

    return &glxPriv->base;
}

// Extra goodies for glx

GLuint
__glFloorLog2(GLuint val)
{
    int c = 0;

    while (val > 1) {
        c++;
        val >>= 1;
    }
    return c;
}

#ifndef OPENGL_FRAMEWORK_PATH
#define OPENGL_FRAMEWORK_PATH \
    "/System/Library/Frameworks/OpenGL.framework/OpenGL"
#endif

static void *opengl_framework_handle;

static glx_func_ptr
get_proc_address(const char *sym)
{
    return (glx_func_ptr) dlsym(opengl_framework_handle, sym);
}

static void
setup_dispatch_table(void)
{
    const char *opengl_framework_path;

    if (opengl_framework_handle) {
        return;
    }

    opengl_framework_path = getenv("OPENGL_FRAMEWORK_PATH");
    if (!opengl_framework_path) {
        opengl_framework_path = OPENGL_FRAMEWORK_PATH;
    }

    (void)dlerror();             /*drain dlerror */
    opengl_framework_handle = dlopen(opengl_framework_path, RTLD_LOCAL);

    if (!opengl_framework_handle) {
        ErrorF("unable to dlopen %s : %s, using RTLD_DEFAULT\n",
               opengl_framework_path, dlerror());
        opengl_framework_handle = RTLD_DEFAULT;
    }

    __glXsetGetProcAddress(get_proc_address);
}
