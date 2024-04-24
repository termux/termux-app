#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _GLX_screens_h_
#define _GLX_screens_h_

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

#include "extension_string.h"
#include "glxvndabi.h"

typedef struct __GLXconfig __GLXconfig;
struct __GLXconfig {
    /* Management */
    __GLXconfig *next;
#ifdef COMPOSITE
    GLboolean duplicatedForComp;
#endif
    GLuint doubleBufferMode;
    GLuint stereoMode;

    GLint redBits, greenBits, blueBits, alphaBits;      /* bits per comp */
    GLuint redMask, greenMask, blueMask, alphaMask;
    GLint rgbBits;              /* total bits for rgb */
    GLint indexBits;            /* total bits for colorindex */

    GLint accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits;
    GLint depthBits;
    GLint stencilBits;

    GLint numAuxBuffers;

    GLint level;

    /* GLX */
    GLint visualID;
    GLint visualType;     /**< One of the GLX X visual types. (i.e.,
			   * \c GLX_TRUE_COLOR, etc.)
			   */

    /* EXT_visual_rating / GLX 1.2 */
    GLint visualRating;

    /* EXT_visual_info / GLX 1.2 */
    GLint transparentPixel;
    /*    colors are floats scaled to ints */
    GLint transparentRed, transparentGreen, transparentBlue, transparentAlpha;
    GLint transparentIndex;

    /* ARB_multisample / SGIS_multisample */
    GLint sampleBuffers;
    GLint samples;

    /* SGIX_fbconfig / GLX 1.3 */
    GLint drawableType;
    GLint renderType;
    GLint fbconfigID;

    /* SGIX_pbuffer / GLX 1.3 */
    GLint maxPbufferWidth;
    GLint maxPbufferHeight;
    GLint maxPbufferPixels;
    GLint optimalPbufferWidth;  /* Only for SGIX_pbuffer. */
    GLint optimalPbufferHeight; /* Only for SGIX_pbuffer. */

    /* SGIX_visual_select_group */
    GLint visualSelectGroup;

    /* OML_swap_method */
    GLint swapMethod;

    /* EXT_texture_from_pixmap */
    GLint bindToTextureRgb;
    GLint bindToTextureRgba;
    GLint bindToMipmapTexture;
    GLint bindToTextureTargets;
    GLint yInverted;

    /* ARB_framebuffer_sRGB */
    GLint sRGBCapable;
};

GLint glxConvertToXVisualType(int visualType);

/*
** Screen dependent data.  These methods are the interface between the DIX
** and DDX layers of the GLX server extension.  The methods provide an
** interface for context management on a screen.
*/
typedef struct __GLXscreen __GLXscreen;
struct __GLXscreen {
    void (*destroy) (__GLXscreen * screen);

    __GLXcontext *(*createContext) (__GLXscreen * screen,
                                    __GLXconfig * modes,
                                    __GLXcontext * shareContext,
                                    unsigned num_attribs,
                                    const uint32_t *attribs,
                                    int *error);

    __GLXdrawable *(*createDrawable) (ClientPtr client,
                                      __GLXscreen * context,
                                      DrawablePtr pDraw,
                                      XID drawId,
                                      int type,
                                      XID glxDrawId, __GLXconfig * modes);
    int (*swapInterval) (__GLXdrawable * drawable, int interval);

    ScreenPtr pScreen;

    /* Linked list of valid fbconfigs for this screen. */
    __GLXconfig *fbconfigs;
    int numFBConfigs;

    /* Subset of fbconfigs that are exposed as GLX visuals. */
    __GLXconfig **visuals;
    GLint numVisuals;

    char *GLextensions;
    char *GLXextensions;
    char *glvnd;
    unsigned char glx_enable_bits[__GLX_EXT_BYTES];

    Bool (*CloseScreen) (ScreenPtr pScreen);
};

void __glXScreenInit(__GLXscreen * screen, ScreenPtr pScreen);
void __glXScreenDestroy(__GLXscreen * screen);

#endif                          /* !__GLX_screens_h__ */
