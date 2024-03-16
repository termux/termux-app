/*
 * Copyright (c) 2007, 2008 Apple Inc.
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

#include "dri.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/CGLContext.h>

#include <GL/glxproto.h>
#include <windowstr.h>
#include <resource.h>
#include <GL/glxint.h>
#include <GL/glxtokens.h>
#include <scrnintstr.h>
#include <glxserver.h>
#include <glxscreens.h>
#include <glxdrawable.h>
#include <glxcontext.h>
#include <glxext.h>
#include <glxutil.h>

#include "capabilities.h"
#include "visualConfigs.h"
#include "darwinfb.h"

/* Based originally on code from indirect.c which was based on code from i830_dri.c. */
__GLXconfig *__glXAquaCreateVisualConfigs(int *numConfigsPtr, int screenNumber) {
    int numConfigs = 0;
    __GLXconfig *visualConfigs, *c, *l;
    struct glCapabilities caps;
    struct glCapabilitiesConfig *conf;
    int stereo, depth, aux, buffers, stencil, accum, color, msample;

    if(getGlCapabilities(&caps)) {
        ErrorF("error from getGlCapabilities()!\n");
        return NULL;
    }

    /*
     conf->stereo is 0 or 1, but we need at least 1 iteration of the loop,
     so we treat a true conf->stereo as 2.

     The depth size is 0 or 24.  Thus we do 2 iterations for that.

     conf->aux_buffers (when available/non-zero) result in 2 iterations instead of 1.

     conf->buffers indicates whether we have single or double buffering.

     conf->total_stencil_bit_depths

     conf->total_color_buffers indicates the RGB/RGBA color depths.

     conf->total_accum_buffers iterations for accum (with at least 1 if equal to 0)

     conf->total_depth_buffer_depths

     conf->multisample_buffers iterations (with at least 1 if equal to 0).  We add 1
     for the 0 multisampling config.

     */

    assert(NULL != caps.configurations);

    numConfigs = 0;

    for(conf = caps.configurations; conf; conf = conf->next) {
        if(conf->total_color_buffers <= 0)
            continue;

        numConfigs += (conf->stereo ? 2 : 1)
	    * (conf->aux_buffers ? 2 : 1)
	    * conf->buffers
	    * ((conf->total_stencil_bit_depths > 0) ? conf->total_stencil_bit_depths : 1)
	    * conf->total_color_buffers
	    * ((conf->total_accum_buffers > 0) ? conf->total_accum_buffers : 1)
	    * conf->total_depth_buffer_depths
	    * (conf->multisample_buffers + 1);
    }

    if(numConfigsPtr)
        *numConfigsPtr = numConfigs;

    /* Note that as of 1.20.0, we cannot allocate all the configs at once.
     * __glXScreenDestroy now walks all the fbconfigs and frees them one at a time.
     * See 4b0a3cbab131eb453e2b3fc0337121969258a7be.
     */
    visualConfigs = calloc(sizeof(*visualConfigs), 1);

    l = NULL;
    c = visualConfigs; /* current buffer */
    for(conf = caps.configurations; conf; conf = conf->next) {
        for(stereo = 0; stereo < (conf->stereo ? 2 : 1); ++stereo) {
            for(aux = 0; aux < (conf->aux_buffers ? 2 : 1); ++aux) {
                for(buffers = 0; buffers < conf->buffers; ++buffers) {
                    for(stencil = 0; stencil < ((conf->total_stencil_bit_depths > 0) ?
                                                conf->total_stencil_bit_depths : 1); ++stencil) {
                        for(color = 0; color < conf->total_color_buffers; ++color) {
                            for(accum = 0; accum < ((conf->total_accum_buffers > 0) ?
                                                    conf->total_accum_buffers : 1); ++accum) {
                                for(depth = 0; depth < conf->total_depth_buffer_depths; ++depth) {
                                    for(msample = 0; msample < (conf->multisample_buffers + 1); ++msample) {

                                        // Global
                                        c->visualID = -1;
                                        c->visualType = GLX_TRUE_COLOR;
                                        c->next = calloc(sizeof(*visualConfigs), 1);
                                        assert(c->next);

                                        c->level = 0;
                                        c->indexBits = 0;

                                        if(conf->accelerated) {
                                            c->visualRating = GLX_NONE;
                                        } else {
                                            c->visualRating = GLX_SLOW_VISUAL_EXT;
                                        }

                                        c->transparentPixel = GLX_NONE;
                                        c->transparentRed = GLX_NONE;
                                        c->transparentGreen = GLX_NONE;
                                        c->transparentBlue = GLX_NONE;
                                        c->transparentAlpha = GLX_NONE;
                                        c->transparentIndex = GLX_NONE;

                                        c->visualSelectGroup = 0;

                                        c->swapMethod = GLX_SWAP_UNDEFINED_OML;

                                        // Stereo
                                        c->stereoMode = stereo ? TRUE : FALSE;

                                        // Aux buffers
                                        c->numAuxBuffers = aux ? conf->aux_buffers : 0;

                                        // Double Buffered
                                        c->doubleBufferMode = buffers ? TRUE : FALSE;

                                        // Stencil Buffer
                                        if(conf->total_stencil_bit_depths > 0) {
                                            c->stencilBits = conf->stencil_bit_depths[stencil];
                                        } else {
                                            c->stencilBits = 0;
                                        }

                                        // Color
                                        if(GLCAPS_COLOR_BUF_INVALID_VALUE != conf->color_buffers[color].a) {
                                            c->alphaBits = conf->color_buffers[color].a;
                                        } else {
                                            c->alphaBits = 0;
                                        }
                                        c->redBits   = conf->color_buffers[color].r;
                                        c->greenBits = conf->color_buffers[color].g;
                                        c->blueBits  = conf->color_buffers[color].b;

                                        c->rgbBits = c->alphaBits + c->redBits + c->greenBits + c->blueBits;

                                        c->alphaMask = AM_ARGB(c->alphaBits, c->redBits, c->greenBits, c->blueBits);
                                        c->redMask   = RM_ARGB(c->alphaBits, c->redBits, c->greenBits, c->blueBits);
                                        c->greenMask = GM_ARGB(c->alphaBits, c->redBits, c->greenBits, c->blueBits);
                                        c->blueMask  = BM_ARGB(c->alphaBits, c->redBits, c->greenBits, c->blueBits);

                                        // Accumulation Buffers
                                        if(conf->total_accum_buffers > 0) {
                                            c->accumRedBits = conf->accum_buffers[accum].r;
                                            c->accumGreenBits = conf->accum_buffers[accum].g;
                                            c->accumBlueBits = conf->accum_buffers[accum].b;
                                            if(GLCAPS_COLOR_BUF_INVALID_VALUE != conf->accum_buffers[accum].a) {
                                                c->accumAlphaBits = conf->accum_buffers[accum].a;
                                            } else {
                                                c->accumAlphaBits = 0;
                                            }
                                        } else {
                                            c->accumRedBits = 0;
                                            c->accumGreenBits = 0;
                                            c->accumBlueBits = 0;
                                            c->accumAlphaBits = 0;
                                        }

                                        // Depth
                                        c->depthBits = conf->depth_buffers[depth];

                                        // MultiSample
                                        if(msample > 0) {
                                            c->samples = conf->multisample_samples;
                                            c->sampleBuffers = conf->multisample_buffers;
                                        } else {
                                            c->samples = 0;
                                            c->sampleBuffers = 0;
                                        }

                                        /*
                                         * The Apple libGL supports GLXPixmaps and
                                         * GLXPbuffers in direct mode.
                                         */
                                        /* SGIX_fbconfig / GLX 1.3 */
                                        c->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;
                                        c->renderType = GLX_RGBA_BIT;
                                        c->fbconfigID = -1;

                                        /* SGIX_pbuffer / GLX 1.3 */

                                        /*
                                         * The CGL layer provides a way of retrieving
                                         * the maximum pbuffer width/height, but only
                                         * if we create a context and call glGetIntegerv.
                                         *
                                         * The following values are from a test program
                                         * that does so.
                                         */
                                        c->maxPbufferWidth = 8192;
                                        c->maxPbufferHeight = 8192;
                                        c->maxPbufferPixels = /*Do we need this?*/ 0;
                                        /*
                                         * There is no introspection for this sort of thing
                                         * with CGL.  What should we do realistically?
                                         */
                                        c->optimalPbufferWidth = 0;
                                        c->optimalPbufferHeight = 0;

                                        /* EXT_texture_from_pixmap */
                                        c->bindToTextureRgb = 0;
                                        c->bindToTextureRgba = 0;
                                        c->bindToMipmapTexture = 0;
                                        c->bindToTextureTargets = 0;
                                        c->yInverted = 0;

                                        /* EXT_framebuffer_sRGB */
                                        c->sRGBCapable = GL_FALSE;

                                        l = c;
                                        c = c->next;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    free(c);
    l->next = NULL;

    freeGlCapabilities(&caps);
    return visualConfigs;
}
