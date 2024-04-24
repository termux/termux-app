/*
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

#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <GL/gl.h>
#include <GL/glxtokens.h>
#include <GL/internal/dri_interface.h>
#include <os.h>
#include "extinit.h"
#include "glxserver.h"
#include "glxext.h"
#include "glxcontext.h"
#include "glxscreens.h"
#include "glxdricommon.h"

#define __ATTRIB(attrib, field) \
    { attrib, offsetof(__GLXconfig, field) }

static const struct {
    unsigned int attrib, offset;
} attribMap[] = {
__ATTRIB(__DRI_ATTRIB_BUFFER_SIZE, rgbBits),
        __ATTRIB(__DRI_ATTRIB_LEVEL, level),
        __ATTRIB(__DRI_ATTRIB_RED_SIZE, redBits),
        __ATTRIB(__DRI_ATTRIB_GREEN_SIZE, greenBits),
        __ATTRIB(__DRI_ATTRIB_BLUE_SIZE, blueBits),
        __ATTRIB(__DRI_ATTRIB_ALPHA_SIZE, alphaBits),
        __ATTRIB(__DRI_ATTRIB_DEPTH_SIZE, depthBits),
        __ATTRIB(__DRI_ATTRIB_STENCIL_SIZE, stencilBits),
        __ATTRIB(__DRI_ATTRIB_ACCUM_RED_SIZE, accumRedBits),
        __ATTRIB(__DRI_ATTRIB_ACCUM_GREEN_SIZE, accumGreenBits),
        __ATTRIB(__DRI_ATTRIB_ACCUM_BLUE_SIZE, accumBlueBits),
        __ATTRIB(__DRI_ATTRIB_ACCUM_ALPHA_SIZE, accumAlphaBits),
        __ATTRIB(__DRI_ATTRIB_SAMPLE_BUFFERS, sampleBuffers),
        __ATTRIB(__DRI_ATTRIB_SAMPLES, samples),
        __ATTRIB(__DRI_ATTRIB_DOUBLE_BUFFER, doubleBufferMode),
        __ATTRIB(__DRI_ATTRIB_STEREO, stereoMode),
        __ATTRIB(__DRI_ATTRIB_AUX_BUFFERS, numAuxBuffers),
        __ATTRIB(__DRI_ATTRIB_TRANSPARENT_TYPE, transparentPixel),
        __ATTRIB(__DRI_ATTRIB_TRANSPARENT_INDEX_VALUE, transparentPixel),
        __ATTRIB(__DRI_ATTRIB_TRANSPARENT_RED_VALUE, transparentRed),
        __ATTRIB(__DRI_ATTRIB_TRANSPARENT_GREEN_VALUE, transparentGreen),
        __ATTRIB(__DRI_ATTRIB_TRANSPARENT_BLUE_VALUE, transparentBlue),
        __ATTRIB(__DRI_ATTRIB_TRANSPARENT_ALPHA_VALUE, transparentAlpha),
        __ATTRIB(__DRI_ATTRIB_RED_MASK, redMask),
        __ATTRIB(__DRI_ATTRIB_GREEN_MASK, greenMask),
        __ATTRIB(__DRI_ATTRIB_BLUE_MASK, blueMask),
        __ATTRIB(__DRI_ATTRIB_ALPHA_MASK, alphaMask),
        __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_WIDTH, maxPbufferWidth),
        __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_HEIGHT, maxPbufferHeight),
        __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_PIXELS, maxPbufferPixels),
        __ATTRIB(__DRI_ATTRIB_OPTIMAL_PBUFFER_WIDTH, optimalPbufferWidth),
        __ATTRIB(__DRI_ATTRIB_OPTIMAL_PBUFFER_HEIGHT, optimalPbufferHeight),
        __ATTRIB(__DRI_ATTRIB_SWAP_METHOD, swapMethod),
        __ATTRIB(__DRI_ATTRIB_BIND_TO_TEXTURE_RGB, bindToTextureRgb),
        __ATTRIB(__DRI_ATTRIB_BIND_TO_TEXTURE_RGBA, bindToTextureRgba),
        __ATTRIB(__DRI_ATTRIB_BIND_TO_MIPMAP_TEXTURE, bindToMipmapTexture),
        __ATTRIB(__DRI_ATTRIB_YINVERTED, yInverted),
        __ATTRIB(__DRI_ATTRIB_FRAMEBUFFER_SRGB_CAPABLE, sRGBCapable),
        };

static void
setScalar(__GLXconfig * config, unsigned int attrib, unsigned int value)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(attribMap); i++)
        if (attribMap[i].attrib == attrib) {
            *(unsigned int *) ((char *) config + attribMap[i].offset) = value;
            return;
        }
}

static Bool
render_type_is_pbuffer_only(unsigned renderType)
{
    /* The GL_ARB_color_buffer_float spec says:
     *
     *     "Note that floating point rendering is only supported for
     *     GLXPbuffer drawables.  The GLX_DRAWABLE_TYPE attribute of the
     *     GLXFBConfig must have the GLX_PBUFFER_BIT bit set and the
     *     GLX_RENDER_TYPE attribute must have the GLX_RGBA_FLOAT_BIT set."
     */
    return !!(renderType & (__DRI_ATTRIB_UNSIGNED_FLOAT_BIT
                            | __DRI_ATTRIB_FLOAT_BIT));
}

static int
server_has_depth(int depth)
{
    int i;
    for (i = 0; i < screenInfo.numPixmapFormats; i++)
        if (screenInfo.formats[i].depth == depth)
            return 1;
    return 0;
}

static __GLXconfig *
createModeFromConfig(const __DRIcoreExtension * core,
                     const __DRIconfig * driConfig,
                     unsigned int visualType,
                     GLboolean duplicateForComp)
{
    __GLXDRIconfig *config;
    GLint renderType = 0;
    unsigned int attrib, value, drawableType = GLX_PBUFFER_BIT;
    int i;


    config = calloc(1, sizeof *config);

    config->driConfig = driConfig;

    i = 0;
    while (core->indexConfigAttrib(driConfig, i++, &attrib, &value)) {
        switch (attrib) {
        case __DRI_ATTRIB_RENDER_TYPE:
            if (value & __DRI_ATTRIB_RGBA_BIT)
                renderType |= GLX_RGBA_BIT;
            if (value & __DRI_ATTRIB_COLOR_INDEX_BIT)
                renderType |= GLX_COLOR_INDEX_BIT;
            if (value & __DRI_ATTRIB_FLOAT_BIT)
                renderType |= GLX_RGBA_FLOAT_BIT_ARB;
            if (value & __DRI_ATTRIB_UNSIGNED_FLOAT_BIT)
                renderType |= GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT;
            break;
        case __DRI_ATTRIB_CONFIG_CAVEAT:
            if (value & __DRI_ATTRIB_NON_CONFORMANT_CONFIG)
                config->config.visualRating = GLX_NON_CONFORMANT_CONFIG;
            else if (value & __DRI_ATTRIB_SLOW_BIT)
                config->config.visualRating = GLX_SLOW_CONFIG;
            else
                config->config.visualRating = GLX_NONE;
            break;
        case __DRI_ATTRIB_BIND_TO_TEXTURE_TARGETS:
            config->config.bindToTextureTargets = 0;
            if (value & __DRI_ATTRIB_TEXTURE_1D_BIT)
                config->config.bindToTextureTargets |= GLX_TEXTURE_1D_BIT_EXT;
            if (value & __DRI_ATTRIB_TEXTURE_2D_BIT)
                config->config.bindToTextureTargets |= GLX_TEXTURE_2D_BIT_EXT;
            if (value & __DRI_ATTRIB_TEXTURE_RECTANGLE_BIT)
                config->config.bindToTextureTargets |=
                    GLX_TEXTURE_RECTANGLE_BIT_EXT;
            break;
        case __DRI_ATTRIB_SWAP_METHOD:
            /* Workaround for broken dri drivers */
            if (value != GLX_SWAP_UNDEFINED_OML &&
                value != GLX_SWAP_COPY_OML &&
                value != GLX_SWAP_EXCHANGE_OML)
                value = GLX_SWAP_UNDEFINED_OML;
            /* Fall through. */
        default:
            setScalar(&config->config, attrib, value);
            break;
        }
    }

    if (!render_type_is_pbuffer_only(renderType))
        drawableType |= GLX_WINDOW_BIT | GLX_PIXMAP_BIT;

    /* Make sure we don't advertise things the server isn't configured for */
    if ((drawableType & (GLX_PBUFFER_BIT | GLX_PIXMAP_BIT)) &&
        !server_has_depth(config->config.rgbBits)) {
        drawableType &= ~(GLX_PBUFFER_BIT | GLX_PIXMAP_BIT);
        if (!drawableType) {
            free(config);
            return NULL;
        }
    }

    config->config.next = NULL;
    config->config.visualType = visualType;
    config->config.renderType = renderType;
    config->config.drawableType = drawableType;
    config->config.yInverted = GL_TRUE;

#ifdef COMPOSITE
    if (!noCompositeExtension) {
        /*
        * Here we decide what fbconfigs will be duplicated for compositing.
        * fgbconfigs marked with duplicatedForConf will be reserved for
        * compositing visuals.
        * It might look strange to do this decision this late when translation
        * from a __DRIConfig is already done, but using the __DRIConfig
        * accessor function becomes worse both with respect to code complexity
        * and CPU usage.
        */
        if (duplicateForComp &&
            (render_type_is_pbuffer_only(renderType) ||
            config->config.rgbBits != 32 ||
            config->config.redBits != 8 ||
            config->config.greenBits != 8 ||
            config->config.blueBits != 8 ||
            config->config.visualRating != GLX_NONE ||
            config->config.sampleBuffers != 0)) {
            free(config);
            return NULL;
        }

        config->config.duplicatedForComp = duplicateForComp;
    }
#endif

    return &config->config;
}

__GLXconfig *
glxConvertConfigs(const __DRIcoreExtension * core,
                  const __DRIconfig ** configs)
{
    __GLXconfig head, *tail;
    int i;

    tail = &head;
    head.next = NULL;

    for (i = 0; configs[i]; i++) {
        tail->next = createModeFromConfig(core, configs[i], GLX_TRUE_COLOR,
                                          GL_FALSE);
        if (tail->next == NULL)
            break;
        tail = tail->next;
    }

    for (i = 0; configs[i]; i++) {
        tail->next = createModeFromConfig(core, configs[i], GLX_DIRECT_COLOR,
                                          GL_FALSE);
        if (tail->next == NULL)
            break;

        tail = tail->next;
    }

#ifdef COMPOSITE
    if (!noCompositeExtension) {
        /* Duplicate fbconfigs for use with compositing visuals */
        for (i = 0; configs[i]; i++) {
            tail->next = createModeFromConfig(core, configs[i], GLX_TRUE_COLOR,
                                            GL_TRUE);
            if (tail->next == NULL)
                continue;

            tail = tail->next;
        }
    }
#endif

    return head.next;
}

static const char dri_driver_path[] = DRI_DRIVER_PATH;

/* Temporary define to allow building without a dri_interface.h from
 * updated Mesa.  Some day when we don't care about Mesa that old any
 * more this can be removed.
 */
#ifndef __DRI_DRIVER_GET_EXTENSIONS
#define __DRI_DRIVER_GET_EXTENSIONS "__driDriverGetExtensions"
#endif

void *
glxProbeDriver(const char *driverName,
               void **coreExt, const char *coreName, int coreVersion,
               void **renderExt, const char *renderName, int renderVersion)
{
    int i;
    void *driver;
    char filename[PATH_MAX];
    char *get_extensions_name;
    const __DRIextension **extensions = NULL;
    const char *path = NULL;

    /* Search in LIBGL_DRIVERS_PATH if we're not setuid. */
    if (!PrivsElevated())
        path = getenv("LIBGL_DRIVERS_PATH");

    if (!path)
        path = dri_driver_path;

    do {
        const char *next;
        int path_len;

        next = strchr(path, ':');
        if (next) {
            path_len = next - path;
            next++;
        } else {
            path_len = strlen(path);
            next = NULL;
        }

        snprintf(filename, sizeof filename, "%.*s/%s_dri.so", path_len, path,
                 driverName);

        driver = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
        if (driver != NULL)
            break;

        LogMessage(X_ERROR, "AIGLX error: dlopen of %s failed (%s)\n",
                   filename, dlerror());

        path = next;
    } while (path);

    if (driver == NULL) {
        LogMessage(X_ERROR, "AIGLX error: unable to load driver %s\n",
                  driverName);
        goto cleanup_failure;
    }

    if (asprintf(&get_extensions_name, "%s_%s",
                 __DRI_DRIVER_GET_EXTENSIONS, driverName) != -1) {
        const __DRIextension **(*get_extensions)(void);

        for (i = 0; i < strlen(get_extensions_name); i++) {
            /* Replace all non-alphanumeric characters with underscore,
             * since they are not allowed in C symbol names. That fixes up
             * symbol name for drivers with '-drm' suffix
             */
            if (!isalnum(get_extensions_name[i]))
                get_extensions_name[i] = '_';
        }

        get_extensions = dlsym(driver, get_extensions_name);
        if (get_extensions)
            extensions = get_extensions();
        free(get_extensions_name);
    }

    if (!extensions)
        extensions = dlsym(driver, __DRI_DRIVER_EXTENSIONS);
    if (extensions == NULL) {
        LogMessage(X_ERROR, "AIGLX error: %s exports no extensions (%s)\n",
                   driverName, dlerror());
        goto cleanup_failure;
    }

    for (i = 0; extensions[i]; i++) {
        if (strcmp(extensions[i]->name, coreName) == 0 &&
            extensions[i]->version >= coreVersion) {
            *coreExt = (void *) extensions[i];
        }

        if (strcmp(extensions[i]->name, renderName) == 0 &&
            extensions[i]->version >= renderVersion) {
            *renderExt = (void *) extensions[i];
        }
    }

    if (*coreExt == NULL || *renderExt == NULL) {
        LogMessage(X_ERROR,
                   "AIGLX error: %s does not export required DRI extension\n",
                   driverName);
        goto cleanup_failure;
    }
    return driver;

 cleanup_failure:
    if (driver)
        dlclose(driver);
    *coreExt = *renderExt = NULL;
    return NULL;
}
