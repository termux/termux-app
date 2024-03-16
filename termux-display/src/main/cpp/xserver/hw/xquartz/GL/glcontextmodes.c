/*
 * (C) Copyright IBM Corporation 2003
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glcontextmodes.c
 * Utility routines for working with \c __GLcontextModes structures.  At
 * some point most or all of these functions will be moved to the Mesa
 * code base.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#if defined(IN_MINI_GLX)
#include <GL/gl.h>
#else
#if defined(HAVE_DIX_CONFIG_H)
#include <dix-config.h>
#endif
#include <X11/X.h>
#include <GL/glx.h>
#include "GL/glxint.h"
#endif

/* Memory macros */
#if defined(IN_MINI_GLX)
#include <stdlib.h>
#include <string.h>
#define _mesa_malloc(b) malloc(b)
#define _mesa_free(m)   free(m)
#define _mesa_memset memset
#else
#ifdef XFree86Server
#include <os.h>
#include <string.h>
#define _mesa_malloc(b) malloc(b)
#define _mesa_free(m)   free(m)
#define _mesa_memset memset
#else
#include <X11/Xlibint.h>
#define _mesa_memset memset
#define _mesa_malloc(b) Xmalloc(b)
#define _mesa_free(m)   free(m)
#endif  /* XFree86Server */
#endif /* !defined(IN_MINI_GLX) */

#include "glcontextmodes.h"

#if !defined(IN_MINI_GLX)
#define NUM_VISUAL_TYPES 6

/**
 * Convert an X visual type to a GLX visual type.
 *
 * \param visualType X visual type (i.e., \c TrueColor, \c StaticGray, etc.)
 *        to be converted.
 * \return If \c visualType is a valid X visual type, a GLX visual type will
 *         be returned.  Otherwise \c GLX_NONE will be returned.
 */
GLint
_gl_convert_from_x_visual_type(int visualType)
{
    static const int glx_visual_types[NUM_VISUAL_TYPES] = {
        GLX_STATIC_GRAY,  GLX_GRAY_SCALE,
        GLX_STATIC_COLOR, GLX_PSEUDO_COLOR,
        GLX_TRUE_COLOR,   GLX_DIRECT_COLOR
    };

    return ((unsigned)visualType < NUM_VISUAL_TYPES)
           ? glx_visual_types[visualType] : GLX_NONE;
}

/**
 * Convert a GLX visual type to an X visual type.
 *
 * \param visualType GLX visual type (i.e., \c GLX_TRUE_COLOR,
 *                   \c GLX_STATIC_GRAY, etc.) to be converted.
 * \return If \c visualType is a valid GLX visual type, an X visual type will
 *         be returned.  Otherwise -1 will be returned.
 */
GLint
_gl_convert_to_x_visual_type(int visualType)
{
    static const int x_visual_types[NUM_VISUAL_TYPES] = {
        TrueColor,   DirectColor,
        PseudoColor, StaticColor,
        GrayScale,   StaticGray
    };

    return ((unsigned)(visualType - GLX_TRUE_COLOR) < NUM_VISUAL_TYPES)
           ? x_visual_types[visualType - GLX_TRUE_COLOR] : -1;
}

/**
 * Copy a GLX visual config structure to a GL context mode structure.  All
 * of the fields in \c config are copied to \c mode.  Additional fields in
 * \c mode that can be derived from the fields of \c config (i.e.,
 * \c haveDepthBuffer) are also filled in.  The remaining fields in \c mode
 * that cannot be derived are set to default values.
 *
 * \param mode   Destination GL context mode.
 * \param config Source GLX visual config.
 *
 * \note
 * The \c fbconfigID and \c visualID fields of the \c __GLcontextModes
 * structure will be set to the \c vid of the \c __GLXvisualConfig structure.
 */
void
_gl_copy_visual_to_context_mode(__GLcontextModes * mode,
                                const __GLXvisualConfig * config)
{
    __GLcontextModes * const next = mode->next;

    (void)_mesa_memset(mode, 0, sizeof(__GLcontextModes));
    mode->next = next;

    mode->visualID = config->vid;
    mode->visualType = _gl_convert_from_x_visual_type(config->class);
    mode->fbconfigID = config->vid;
    mode->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT;

    mode->rgbMode = (config->rgba != 0);
    mode->renderType = (mode->rgbMode) ? GLX_RGBA_BIT : GLX_COLOR_INDEX_BIT;

    mode->colorIndexMode = !(mode->rgbMode);
    mode->doubleBufferMode = (config->doubleBuffer != 0);
    mode->stereoMode = (config->stereo != 0);

    mode->haveAccumBuffer = ((config->accumRedSize +
                              config->accumGreenSize +
                              config->accumBlueSize +
                              config->accumAlphaSize) > 0);
    mode->haveDepthBuffer = (config->depthSize > 0);
    mode->haveStencilBuffer = (config->stencilSize > 0);

    mode->redBits = config->redSize;
    mode->greenBits = config->greenSize;
    mode->blueBits = config->blueSize;
    mode->alphaBits = config->alphaSize;
    mode->redMask = config->redMask;
    mode->greenMask = config->greenMask;
    mode->blueMask = config->blueMask;
    mode->alphaMask = config->alphaMask;
    mode->rgbBits = mode->rgbMode ? config->bufferSize : 0;
    mode->indexBits = mode->colorIndexMode ? config->bufferSize : 0;

    mode->accumRedBits = config->accumRedSize;
    mode->accumGreenBits = config->accumGreenSize;
    mode->accumBlueBits = config->accumBlueSize;
    mode->accumAlphaBits = config->accumAlphaSize;
    mode->depthBits = config->depthSize;
    mode->stencilBits = config->stencilSize;

    mode->numAuxBuffers = config->auxBuffers;
    mode->level = config->level;

    mode->visualRating = config->visualRating;
    mode->transparentPixel = config->transparentPixel;
    mode->transparentRed = config->transparentRed;
    mode->transparentGreen = config->transparentGreen;
    mode->transparentBlue = config->transparentBlue;
    mode->transparentAlpha = config->transparentAlpha;
    mode->transparentIndex = config->transparentIndex;
    mode->samples = config->multiSampleSize;
    mode->sampleBuffers = config->nMultiSampleBuffers;
    /* mode->visualSelectGroup = config->visualSelectGroup; ? */

    mode->swapMethod = GLX_SWAP_UNDEFINED_OML;

    mode->bindToTextureRgb = (mode->rgbMode) ? GL_TRUE : GL_FALSE;
    mode->bindToTextureRgba = (mode->rgbMode && mode->alphaBits) ?
                              GL_TRUE : GL_FALSE;
    mode->bindToMipmapTexture = mode->rgbMode ? GL_TRUE : GL_FALSE;
    mode->bindToTextureTargets = mode->rgbMode ?
                                 GLX_TEXTURE_1D_BIT_EXT |
                                 GLX_TEXTURE_2D_BIT_EXT |
                                 GLX_TEXTURE_RECTANGLE_BIT_EXT : 0;
    mode->yInverted = GL_FALSE;
}

/**
 * Get data from a GL context mode.
 *
 * \param mode         GL context mode whose data is to be returned.
 * \param attribute    Attribute of \c mode that is to be returned.
 * \param value_return Location to store the data member of \c mode.
 * \return  If \c attribute is a valid attribute of \c mode, zero is
 *          returned.  Otherwise \c GLX_BAD_ATTRIBUTE is returned.
 */
int
_gl_get_context_mode_data(const __GLcontextModes *mode, int attribute,
                          int *value_return)
{
    switch (attribute) {
    case GLX_USE_GL:
        *value_return = GL_TRUE;
        return 0;

    case GLX_BUFFER_SIZE:
        *value_return = mode->rgbBits;
        return 0;

    case GLX_RGBA:
        *value_return = mode->rgbMode;
        return 0;

    case GLX_RED_SIZE:
        *value_return = mode->redBits;
        return 0;

    case GLX_GREEN_SIZE:
        *value_return = mode->greenBits;
        return 0;

    case GLX_BLUE_SIZE:
        *value_return = mode->blueBits;
        return 0;

    case GLX_ALPHA_SIZE:
        *value_return = mode->alphaBits;
        return 0;

    case GLX_DOUBLEBUFFER:
        *value_return = mode->doubleBufferMode;
        return 0;

    case GLX_STEREO:
        *value_return = mode->stereoMode;
        return 0;

    case GLX_AUX_BUFFERS:
        *value_return = mode->numAuxBuffers;
        return 0;

    case GLX_DEPTH_SIZE:
        *value_return = mode->depthBits;
        return 0;

    case GLX_STENCIL_SIZE:
        *value_return = mode->stencilBits;
        return 0;

    case GLX_ACCUM_RED_SIZE:
        *value_return = mode->accumRedBits;
        return 0;

    case GLX_ACCUM_GREEN_SIZE:
        *value_return = mode->accumGreenBits;
        return 0;

    case GLX_ACCUM_BLUE_SIZE:
        *value_return = mode->accumBlueBits;
        return 0;

    case GLX_ACCUM_ALPHA_SIZE:
        *value_return = mode->accumAlphaBits;
        return 0;

    case GLX_LEVEL:
        *value_return = mode->level;
        return 0;

    case GLX_TRANSPARENT_TYPE_EXT:
        *value_return = mode->transparentPixel;
        return 0;

    case GLX_TRANSPARENT_RED_VALUE:
        *value_return = mode->transparentRed;
        return 0;

    case GLX_TRANSPARENT_GREEN_VALUE:
        *value_return = mode->transparentGreen;
        return 0;

    case GLX_TRANSPARENT_BLUE_VALUE:
        *value_return = mode->transparentBlue;
        return 0;

    case GLX_TRANSPARENT_ALPHA_VALUE:
        *value_return = mode->transparentAlpha;
        return 0;

    case GLX_TRANSPARENT_INDEX_VALUE:
        *value_return = mode->transparentIndex;
        return 0;

    case GLX_X_VISUAL_TYPE:
        *value_return = mode->visualType;
        return 0;

    case GLX_CONFIG_CAVEAT:
        *value_return = mode->visualRating;
        return 0;

    case GLX_VISUAL_ID:
        *value_return = mode->visualID;
        return 0;

    case GLX_DRAWABLE_TYPE:
        *value_return = mode->drawableType;
        return 0;

    case GLX_RENDER_TYPE:
        *value_return = mode->renderType;
        return 0;

    case GLX_X_RENDERABLE:
        *value_return = mode->xRenderable;
        return 0;

    case GLX_FBCONFIG_ID:
        *value_return = mode->fbconfigID;
        return 0;

    case GLX_MAX_PBUFFER_WIDTH:
        *value_return = mode->maxPbufferWidth;
        return 0;

    case GLX_MAX_PBUFFER_HEIGHT:
        *value_return = mode->maxPbufferHeight;
        return 0;

    case GLX_MAX_PBUFFER_PIXELS:
        *value_return = mode->maxPbufferPixels;
        return 0;

    case GLX_OPTIMAL_PBUFFER_WIDTH_SGIX:
        *value_return = mode->optimalPbufferWidth;
        return 0;

    case GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX:
        *value_return = mode->optimalPbufferHeight;
        return 0;

    case GLX_SWAP_METHOD_OML:
        *value_return = mode->swapMethod;
        return 0;

    case GLX_SAMPLE_BUFFERS_SGIS:
        *value_return = mode->sampleBuffers;
        return 0;

    case GLX_SAMPLES_SGIS:
        *value_return = mode->samples;
        return 0;

    case GLX_BIND_TO_TEXTURE_RGB_EXT:
        *value_return = mode->bindToTextureRgb;
        return 0;

    case GLX_BIND_TO_TEXTURE_RGBA_EXT:
        *value_return = mode->bindToTextureRgba;
        return 0;

    case GLX_BIND_TO_MIPMAP_TEXTURE_EXT:
        *value_return = mode->bindToMipmapTexture == GL_TRUE ? GL_TRUE :
                        GL_FALSE;
        return 0;

    case GLX_BIND_TO_TEXTURE_TARGETS_EXT:
        *value_return = mode->bindToTextureTargets;
        return 0;

    case GLX_Y_INVERTED_EXT:
        *value_return = mode->yInverted;
        return 0;

    /* Applications are NOT allowed to query GLX_VISUAL_SELECT_GROUP_SGIX.
     * It is ONLY for communication between the GLX client and the GLX
     * server.
     */
    case GLX_VISUAL_SELECT_GROUP_SGIX:
    default:
        return GLX_BAD_ATTRIBUTE;
    }
}
#endif /* !defined(IN_MINI_GLX) */

/**
 * Allocate a linked list of \c __GLcontextModes structures.  The fields of
 * each structure will be initialized to "reasonable" default values.  In
 * most cases this is the default value defined by table 3.4 of the GLX
 * 1.3 specification.  This means that most values are either initialized to
 * zero or \c GLX_DONT_CARE (which is -1).  As support for additional
 * extensions is added, the new values will be initialized to appropriate
 * values from the extension specification.
 *
 * \param count         Number of structures to allocate.
 * \param minimum_size  Minimum size of a structure to allocate.  This allows
 *                      for differences in the version of the
 *                      \c __GLcontextModes structure used in libGL and in a
 *                      DRI-based driver.
 * \returns A pointer to the first element in a linked list of \c count
 *          structures on success, or \c NULL on failure.
 *
 * \warning Use of \c minimum_size does \b not guarantee binary compatibility.
 *          The fundamental assumption is that if the \c minimum_size
 *          specified by the driver and the size of the \c __GLcontextModes
 *          structure in libGL is the same, then the meaning of each byte in
 *          the structure is the same in both places.  \b Be \b careful!
 *          Basically this means that fields have to be added in libGL and
 *          then propagated to drivers.  Drivers should \b never arbitrarilly
 *          extend the \c __GLcontextModes data-structure.
 */
__GLcontextModes *
_gl_context_modes_create(unsigned count, size_t minimum_size)
{
    const size_t size = (minimum_size > sizeof(__GLcontextModes))
                        ? minimum_size : sizeof(__GLcontextModes);
    __GLcontextModes * base = NULL;
    __GLcontextModes ** next;
    unsigned i;

    next = &base;
    for (i = 0; i < count; i++) {
        *next = (__GLcontextModes *)_mesa_malloc(size);
        if (*next == NULL) {
            _gl_context_modes_destroy(base);
            base = NULL;
            break;
        }

        (void)_mesa_memset(*next, 0, size);
        (*next)->visualID = GLX_DONT_CARE;
        (*next)->visualType = GLX_DONT_CARE;
        (*next)->visualRating = GLX_NONE;
        (*next)->transparentPixel = GLX_NONE;
        (*next)->transparentRed = GLX_DONT_CARE;
        (*next)->transparentGreen = GLX_DONT_CARE;
        (*next)->transparentBlue = GLX_DONT_CARE;
        (*next)->transparentAlpha = GLX_DONT_CARE;
        (*next)->transparentIndex = GLX_DONT_CARE;
        (*next)->xRenderable = GLX_DONT_CARE;
        (*next)->fbconfigID = GLX_DONT_CARE;
        (*next)->swapMethod = GLX_SWAP_UNDEFINED_OML;
        (*next)->bindToTextureRgb = GLX_DONT_CARE;
        (*next)->bindToTextureRgba = GLX_DONT_CARE;
        (*next)->bindToMipmapTexture = GLX_DONT_CARE;
        (*next)->bindToTextureTargets = GLX_DONT_CARE;
        (*next)->yInverted = GLX_DONT_CARE;

        next = &((*next)->next);
    }

    return base;
}

/**
 * Destroy a linked list of \c __GLcontextModes structures created by
 * \c _gl_context_modes_create.
 *
 * \param modes  Linked list of structures to be destroyed.  All structures
 *               in the list will be freed.
 */
void
_gl_context_modes_destroy(__GLcontextModes * modes)
{
    while (modes != NULL) {
        __GLcontextModes * const next = modes->next;

        _mesa_free(modes);
        modes = next;
    }
}

/**
 * Find a context mode matching a Visual ID.
 *
 * \param modes  List list of context-mode structures to be searched.
 * \param vid    Visual ID to be found.
 * \returns A pointer to a context-mode in \c modes if \c vid was found in
 *          the list, or \c NULL if it was not.
 */

__GLcontextModes *
_gl_context_modes_find_visual(__GLcontextModes *modes, int vid)
{
    __GLcontextModes *m;

    for (m = modes; m != NULL; m = m->next)
        if (m->visualID == vid)
            return m;

    return NULL;
}

__GLcontextModes *
_gl_context_modes_find_fbconfig(__GLcontextModes *modes, int fbid)
{
    __GLcontextModes *m;

    for (m = modes; m != NULL; m = m->next)
        if (m->fbconfigID == fbid)
            return m;

    return NULL;
}

/**
 * Determine if two context-modes are the same.  This is intended to be used
 * by libGL implementations to compare to sets of driver generated FBconfigs.
 *
 * \param a  Context-mode to be compared.
 * \param b  Context-mode to be compared.
 * \returns \c GL_TRUE if the two context-modes are the same.  \c GL_FALSE is
 *          returned otherwise.
 */
GLboolean
_gl_context_modes_are_same(const __GLcontextModes * a,
                           const __GLcontextModes * b)
{
    return ((a->rgbMode == b->rgbMode) &&
            (a->floatMode == b->floatMode) &&
            (a->colorIndexMode == b->colorIndexMode) &&
            (a->doubleBufferMode == b->doubleBufferMode) &&
            (a->stereoMode == b->stereoMode) &&
            (a->redBits == b->redBits) &&
            (a->greenBits == b->greenBits) &&
            (a->blueBits == b->blueBits) &&
            (a->alphaBits == b->alphaBits) &&
#if 0 /* For some reason these don't get set on the client-side in libGL. */
            (a->redMask == b->redMask) &&
            (a->greenMask == b->greenMask) &&
            (a->blueMask == b->blueMask) &&
            (a->alphaMask == b->alphaMask) &&
#endif
            (a->rgbBits == b->rgbBits) &&
            (a->indexBits == b->indexBits) &&
            (a->accumRedBits == b->accumRedBits) &&
            (a->accumGreenBits == b->accumGreenBits) &&
            (a->accumBlueBits == b->accumBlueBits) &&
            (a->accumAlphaBits == b->accumAlphaBits) &&
            (a->depthBits == b->depthBits) &&
            (a->stencilBits == b->stencilBits) &&
            (a->numAuxBuffers == b->numAuxBuffers) &&
            (a->level == b->level) &&
            (a->visualRating == b->visualRating) &&

            (a->transparentPixel == b->transparentPixel) &&

            ((a->transparentPixel != GLX_TRANSPARENT_RGB) ||
             ((a->transparentRed == b->transparentRed) &&
              (a->transparentGreen == b->transparentGreen) &&
              (a->transparentBlue == b->transparentBlue) &&
              (a->transparentAlpha == b->transparentAlpha))) &&

            ((a->transparentPixel != GLX_TRANSPARENT_INDEX) ||
             (a->transparentIndex == b->transparentIndex)) &&

            (a->sampleBuffers == b->sampleBuffers) &&
            (a->samples == b->samples) &&
            ((a->drawableType & b->drawableType) != 0) &&
            (a->renderType == b->renderType) &&
            (a->maxPbufferWidth == b->maxPbufferWidth) &&
            (a->maxPbufferHeight == b->maxPbufferHeight) &&
            (a->maxPbufferPixels == b->maxPbufferPixels) &&
            (a->optimalPbufferWidth == b->optimalPbufferWidth) &&
            (a->optimalPbufferHeight == b->optimalPbufferHeight) &&
            (a->swapMethod == b->swapMethod) &&
            (a->bindToTextureRgb == b->bindToTextureRgb) &&
            (a->bindToTextureRgba == b->bindToTextureRgba) &&
            (a->bindToMipmapTexture == b->bindToMipmapTexture) &&
            (a->bindToTextureTargets == b->bindToTextureTargets) &&
            (a->yInverted == b->yInverted));
}
