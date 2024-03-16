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

#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#include <stdbool.h>

enum { GLCAPS_INVALID_STENCIL_DEPTH = -1 };
enum { GLCAPS_COLOR_BUF_INVALID_VALUE = -1 };
enum { GLCAPS_COLOR_BUFFERS = 20 };
enum { GLCAPS_STENCIL_BIT_DEPTH_BUFFERS = 20 };
enum { GLCAPS_DEPTH_BUFFERS = 20 };
enum { GLCAPS_INVALID_DEPTH_VALUE = 1 };

struct glColorBufCapabilities {
    char r, g, b, a;
    bool is_argb;
};

struct glCapabilitiesConfig {
    bool accelerated;
    bool stereo;
    int aux_buffers;
    int buffers;
    int total_depth_buffer_depths;
    int depth_buffers[GLCAPS_DEPTH_BUFFERS];
    int multisample_buffers;
    int multisample_samples;
    int total_stencil_bit_depths;
    char stencil_bit_depths[GLCAPS_STENCIL_BIT_DEPTH_BUFFERS];
    int total_color_buffers;
    struct glColorBufCapabilities color_buffers[GLCAPS_COLOR_BUFFERS];
    int total_accum_buffers;
    struct glColorBufCapabilities accum_buffers[GLCAPS_COLOR_BUFFERS];
    struct glCapabilitiesConfig *next;
};

struct glCapabilities {
    struct glCapabilitiesConfig *configurations;
    int total_configurations;
};

bool
getGlCapabilities(struct glCapabilities *cap);
void
freeGlCapabilities(struct glCapabilities *cap);

#endif
