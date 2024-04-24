/*
 * Copyright (c) 2008-2012 Apple Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define Cursor Mac_Cursor
#define BOOL   Mac_BOOL
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#include <ApplicationServices/ApplicationServices.h>
#undef Cursor
#undef BOOL

#include "capabilities.h"

#include "os.h"

static void
handleBufferModes(struct glCapabilitiesConfig *c, GLint bufferModes)
{
    if (bufferModes & kCGLStereoscopicBit) {
        c->stereo = true;
    }

    if (bufferModes & kCGLDoubleBufferBit) {
        c->buffers = 2;
    }
    else {
        c->buffers = 1;
    }
}

static void
handleStencilModes(struct glCapabilitiesConfig *c, GLint smodes)
{
    int offset = 0;

    if (kCGL0Bit & smodes)
        c->stencil_bit_depths[offset++] = 0;

    if (kCGL1Bit & smodes)
        c->stencil_bit_depths[offset++] = 1;

    if (kCGL2Bit & smodes)
        c->stencil_bit_depths[offset++] = 2;

    if (kCGL3Bit & smodes)
        c->stencil_bit_depths[offset++] = 3;

    if (kCGL4Bit & smodes)
        c->stencil_bit_depths[offset++] = 4;

    if (kCGL5Bit & smodes)
        c->stencil_bit_depths[offset++] = 5;

    if (kCGL6Bit & smodes)
        c->stencil_bit_depths[offset++] = 6;

    if (kCGL8Bit & smodes)
        c->stencil_bit_depths[offset++] = 8;

    if (kCGL10Bit & smodes)
        c->stencil_bit_depths[offset++] = 10;

    if (kCGL12Bit & smodes)
        c->stencil_bit_depths[offset++] = 12;

    if (kCGL16Bit & smodes)
        c->stencil_bit_depths[offset++] = 16;

    if (kCGL24Bit & smodes)
        c->stencil_bit_depths[offset++] = 24;

    if (kCGL32Bit & smodes)
        c->stencil_bit_depths[offset++] = 32;

    if (kCGL48Bit & smodes)
        c->stencil_bit_depths[offset++] = 48;

    if (kCGL64Bit & smodes)
        c->stencil_bit_depths[offset++] = 64;

    if (kCGL96Bit & smodes)
        c->stencil_bit_depths[offset++] = 96;

    if (kCGL128Bit & smodes)
        c->stencil_bit_depths[offset++] = 128;

    assert(offset < GLCAPS_STENCIL_BIT_DEPTH_BUFFERS);

    c->total_stencil_bit_depths = offset;
}

static int
handleColorAndAccumulation(struct glColorBufCapabilities *c,
                           GLint cmodes, int forAccum)
{
    int offset = 0;

    /*1*/
    if (kCGLRGB444Bit & cmodes) {
        c[offset].r = 4;
        c[offset].g = 4;
        c[offset].b = 4;
        ++offset;
    }

    /*2*/
    if (kCGLARGB4444Bit & cmodes) {
        c[offset].a = 4;
        c[offset].r = 4;
        c[offset].g = 4;
        c[offset].b = 4;
        c[offset].is_argb = true;
        ++offset;
    }

    /*3*/
    if (kCGLRGB444A8Bit & cmodes) {
        c[offset].r = 4;
        c[offset].g = 4;
        c[offset].b = 4;
        c[offset].a = 8;
        ++offset;
    }

    /*4*/
    if (kCGLRGB555Bit & cmodes) {
        c[offset].r = 5;
        c[offset].g = 5;
        c[offset].b = 5;
        ++offset;
    }

    /*5*/
    if (kCGLARGB1555Bit & cmodes) {
        c[offset].a = 1;
        c[offset].r = 5;
        c[offset].g = 5;
        c[offset].b = 5;
        c[offset].is_argb = true;
        ++offset;
    }

    /*6*/
    if (kCGLRGB555A8Bit & cmodes) {
        c[offset].r = 5;
        c[offset].g = 5;
        c[offset].b = 5;
        c[offset].a = 8;
        ++offset;
    }

    /*7*/
    if (kCGLRGB565Bit & cmodes) {
        c[offset].r = 5;
        c[offset].g = 6;
        c[offset].b = 5;
        ++offset;
    }

    /*8*/
    if (kCGLRGB565A8Bit & cmodes) {
        c[offset].r = 5;
        c[offset].g = 6;
        c[offset].b = 5;
        c[offset].a = 8;
        ++offset;
    }

    /*9*/
    if (kCGLRGB888Bit & cmodes) {
        c[offset].r = 8;
        c[offset].g = 8;
        c[offset].b = 8;
        ++offset;
    }

    /*10*/
    if (kCGLARGB8888Bit & cmodes) {
        c[offset].a = 8;
        c[offset].r = 8;
        c[offset].g = 8;
        c[offset].b = 8;
        c[offset].is_argb = true;
        ++offset;
    }

    /*11*/
    if (kCGLRGB888A8Bit & cmodes) {
        c[offset].r = 8;
        c[offset].g = 8;
        c[offset].b = 8;
        c[offset].a = 8;
        ++offset;
    }

    if (forAccum) {
        //#if 0
        /* FIXME
         * Disable this path, because some part of libGL, X, or Xplugin
         * doesn't work with sizes greater than 8.
         * When this is enabled and visuals are chosen using depths
         * such as 16, the result is that the windows don't redraw
         * and are often white, until a resize.
         */

        /*12*/
        if (kCGLRGB101010Bit & cmodes) {
            c[offset].r = 10;
            c[offset].g = 10;
            c[offset].b = 10;
            ++offset;
        }

        /*13*/
        if (kCGLARGB2101010Bit & cmodes) {
            c[offset].a = 2;
            c[offset].r = 10;
            c[offset].g = 10;
            c[offset].b = 10;
            c[offset].is_argb = true;
            ++offset;
        }

        /*14*/
        if (kCGLRGB101010_A8Bit & cmodes) {
            c[offset].r = 10;
            c[offset].g = 10;
            c[offset].b = 10;
            c[offset].a = 8;
            ++offset;
        }

        /*15*/
        if (kCGLRGB121212Bit & cmodes) {
            c[offset].r = 12;
            c[offset].g = 12;
            c[offset].b = 12;
            ++offset;
        }

        /*16*/
        if (kCGLARGB12121212Bit & cmodes) {
            c[offset].a = 12;
            c[offset].r = 12;
            c[offset].g = 12;
            c[offset].b = 12;
            c[offset].is_argb = true;
            ++offset;
        }

        /*17*/
        if (kCGLRGB161616Bit & cmodes) {
            c[offset].r = 16;
            c[offset].g = 16;
            c[offset].b = 16;
            ++offset;
        }

        /*18*/
        if (kCGLRGBA16161616Bit & cmodes) {
            c[offset].r = 16;
            c[offset].g = 16;
            c[offset].b = 16;
            c[offset].a = 16;
            ++offset;
        }
    }
    //#endif

    /* FIXME should we handle the floating point color modes, and if so, how? */

    return offset;
}

static void
handleColorModes(struct glCapabilitiesConfig *c, GLint cmodes)
{
    c->total_color_buffers = handleColorAndAccumulation(c->color_buffers,
                                                        cmodes, 0);

    assert(c->total_color_buffers < GLCAPS_COLOR_BUFFERS);
}

static void
handleAccumulationModes(struct glCapabilitiesConfig *c, GLint cmodes)
{
    c->total_accum_buffers = handleColorAndAccumulation(c->accum_buffers,
                                                        cmodes, 1);
    assert(c->total_accum_buffers < GLCAPS_COLOR_BUFFERS);
}

static void
handleDepthModes(struct glCapabilitiesConfig *c, GLint dmodes)
{
    int offset = 0;
#define DEPTH(flag, value) do { \
        if (dmodes & flag) { \
            c->depth_buffers[offset++] = value; \
        } \
} while (0)

    /*1*/
    DEPTH(kCGL0Bit, 0);
    /*2*/
    DEPTH(kCGL1Bit, 1);
    /*3*/
    DEPTH(kCGL2Bit, 2);
    /*4*/
    DEPTH(kCGL3Bit, 3);
    /*5*/
    DEPTH(kCGL4Bit, 4);
    /*6*/
    DEPTH(kCGL5Bit, 5);
    /*7*/
    DEPTH(kCGL6Bit, 6);
    /*8*/
    DEPTH(kCGL8Bit, 8);
    /*9*/
    DEPTH(kCGL10Bit, 10);
    /*10*/
    DEPTH(kCGL12Bit, 12);
    /*11*/
    DEPTH(kCGL16Bit, 16);
    /*12*/
    DEPTH(kCGL24Bit, 24);
    /*13*/
    DEPTH(kCGL32Bit, 32);
    /*14*/
    DEPTH(kCGL48Bit, 48);
    /*15*/
    DEPTH(kCGL64Bit, 64);
    /*16*/
    DEPTH(kCGL96Bit, 96);
    /*17*/
    DEPTH(kCGL128Bit, 128);

#undef DEPTH

    c->total_depth_buffer_depths = offset;
    assert(c->total_depth_buffer_depths < GLCAPS_DEPTH_BUFFERS);
}

/* Return non-zero if an error occurred. */
static CGLError
handleRendererDescriptions(CGLRendererInfoObj info, GLint r,
                           struct glCapabilitiesConfig *c)
{
    CGLError err;
    GLint accelerated = 0, flags = 0, aux = 0, samplebufs = 0, samples = 0;

    err = CGLDescribeRenderer(info, r, kCGLRPAccelerated, &accelerated);

    if (err)
        return err;

    c->accelerated = accelerated;

    /* Buffering modes: single/double, stereo */
    err = CGLDescribeRenderer(info, r, kCGLRPBufferModes, &flags);

    if (err)
        return err;

    handleBufferModes(c, flags);

    /* AUX buffers */
    err = CGLDescribeRenderer(info, r, kCGLRPMaxAuxBuffers, &aux);

    if (err)
        return err;

    c->aux_buffers = aux;

    /* Depth buffer size */
    err = CGLDescribeRenderer(info, r, kCGLRPDepthModes, &flags);

    if (err)
        return err;

    handleDepthModes(c, flags);

    /* Multisample buffers */
    err = CGLDescribeRenderer(info, r, kCGLRPMaxSampleBuffers, &samplebufs);

    if (err)
        return err;

    c->multisample_buffers = samplebufs;

    /* Multisample samples per multisample buffer */
    err = CGLDescribeRenderer(info, r, kCGLRPMaxSamples, &samples);

    if (err)
        return err;

    c->multisample_samples = samples;

    /* Stencil bit depths */
    err = CGLDescribeRenderer(info, r, kCGLRPStencilModes, &flags);

    if (err)
        return err;

    handleStencilModes(c, flags);

    /* Color modes (RGB/RGBA depths supported */
    err = CGLDescribeRenderer(info, r, kCGLRPColorModes, &flags);

    if (err)
        return err;

    handleColorModes(c, flags);

    err = CGLDescribeRenderer(info, r, kCGLRPAccumModes, &flags);

    if (err)
        return err;

    handleAccumulationModes(c, flags);

    return kCGLNoError;
}

static void
initCapabilities(struct glCapabilities *cap)
{
    cap->configurations = NULL;
    cap->total_configurations = 0;
}

static void
initConfig(struct glCapabilitiesConfig *c)
{
    int i;

    c->accelerated = false;
    c->stereo = false;
    c->aux_buffers = 0;
    c->buffers = 0;

    c->total_depth_buffer_depths = 0;

    for (i = 0; i < GLCAPS_DEPTH_BUFFERS; ++i) {
        c->depth_buffers[i] = GLCAPS_INVALID_DEPTH_VALUE;
    }

    c->multisample_buffers = 0;
    c->multisample_samples = 0;

    c->total_stencil_bit_depths = 0;

    for (i = 0; i < GLCAPS_STENCIL_BIT_DEPTH_BUFFERS; ++i) {
        c->stencil_bit_depths[i] = GLCAPS_INVALID_STENCIL_DEPTH;
    }

    c->total_color_buffers = 0;

    for (i = 0; i < GLCAPS_COLOR_BUFFERS; ++i) {
        c->color_buffers[i].r = c->color_buffers[i].g =
                                    c->color_buffers[i].b =
                                        c->color_buffers[i].a =
                                            GLCAPS_COLOR_BUF_INVALID_VALUE;
        c->color_buffers[i].is_argb = false;
    }

    c->total_accum_buffers = 0;

    for (i = 0; i < GLCAPS_COLOR_BUFFERS; ++i) {
        c->accum_buffers[i].r = c->accum_buffers[i].g =
                                    c->accum_buffers[i].b =
                                        c->accum_buffers[i].a =
                                            GLCAPS_COLOR_BUF_INVALID_VALUE;
        c->accum_buffers[i].is_argb = false;
    }

    c->next = NULL;
}

void
freeGlCapabilities(struct glCapabilities *cap)
{
    struct glCapabilitiesConfig *conf, *next;

    conf = cap->configurations;

    while (conf) {
        next = conf->next;
        free(conf);
        conf = next;
    }

    cap->configurations = NULL;
}

/* Return true if an error occurred. */
bool
getGlCapabilities(struct glCapabilities *cap)
{
    CGLRendererInfoObj info;
    CGLError err;
    GLint numRenderers = 0, r;

    initCapabilities(cap);

    err = CGLQueryRendererInfo((GLuint) - 1, &info, &numRenderers);
    if (err) {
        ErrorF("CGLQueryRendererInfo error: %s\n", CGLErrorString(err));
        return err;
    }

    for (r = 0; r < numRenderers; r++) {
        struct glCapabilitiesConfig tmpconf, *conf;

        initConfig(&tmpconf);

        err = handleRendererDescriptions(info, r, &tmpconf);
        if (err) {
            ErrorF("handleRendererDescriptions returned error: %s\n",
                   CGLErrorString(
                       err));
            ErrorF("trying to continue...\n");
            continue;
        }

        conf = malloc(sizeof(*conf));
        if (NULL == conf) {
            FatalError("Unable to allocate memory for OpenGL capabilities\n");
        }

        /* Copy the struct. */
        *conf = tmpconf;

        /* Now link the configuration into the list. */
        conf->next = cap->configurations;
        cap->configurations = conf;
    }

    CGLDestroyRendererInfo(info);

    /* No error occurred.  We are done. */
    return kCGLNoError;
}
