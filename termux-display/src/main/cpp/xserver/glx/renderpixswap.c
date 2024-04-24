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

#include "glxserver.h"
#include "unpack.h"
#include "indirect_dispatch.h"

void
__glXDispSwap_SeparableFilter2D(GLbyte * pc)
{
    __GLXdispatchConvolutionFilterHeader *hdr =
        (__GLXdispatchConvolutionFilterHeader *) pc;
    GLint hdrlen, image1len;

    __GLX_DECLARE_SWAP_VARIABLES;

    hdrlen = __GLX_PAD(__GLX_CONV_FILT_CMD_HDR_SIZE);

    __GLX_SWAP_INT((GLbyte *) &hdr->rowLength);
    __GLX_SWAP_INT((GLbyte *) &hdr->skipRows);
    __GLX_SWAP_INT((GLbyte *) &hdr->skipPixels);
    __GLX_SWAP_INT((GLbyte *) &hdr->alignment);

    __GLX_SWAP_INT((GLbyte *) &hdr->target);
    __GLX_SWAP_INT((GLbyte *) &hdr->internalformat);
    __GLX_SWAP_INT((GLbyte *) &hdr->width);
    __GLX_SWAP_INT((GLbyte *) &hdr->height);
    __GLX_SWAP_INT((GLbyte *) &hdr->format);
    __GLX_SWAP_INT((GLbyte *) &hdr->type);

    /*
     ** Just invert swapBytes flag; the GL will figure out if it needs to swap
     ** the pixel data.
     */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, !hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, hdr->rowLength);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, hdr->skipRows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, hdr->skipPixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, hdr->alignment);

    /* XXX check this usage - internal code called
     ** a version without the packing parameters
     */
    image1len = __glXImageSize(hdr->format, hdr->type, 0, hdr->width, 1, 1,
                               0, hdr->rowLength, 0, hdr->skipRows,
                               hdr->alignment);
    image1len = __GLX_PAD(image1len);

    glSeparableFilter2D(hdr->target, hdr->internalformat, hdr->width,
                        hdr->height, hdr->format, hdr->type,
                        ((GLubyte *) hdr + hdrlen),
                        ((GLubyte *) hdr + hdrlen + image1len));
}
