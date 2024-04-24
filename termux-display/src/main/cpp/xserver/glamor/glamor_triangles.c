/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1998 Keith Packard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Zhigang Gong <zhigang.gong@gmail.com>
 *
 */

#include "glamor_priv.h"

void
glamor_triangles(CARD8 op,
                 PicturePtr pSrc,
                 PicturePtr pDst,
                 PictFormatPtr maskFormat,
                 INT16 xSrc, INT16 ySrc, int ntris, xTriangle * tris)
{
    if (glamor_prepare_access_picture(pDst, GLAMOR_ACCESS_RW) &&
        glamor_prepare_access_picture(pSrc, GLAMOR_ACCESS_RO)) {
        fbTriangles(op, pSrc, pDst, maskFormat, xSrc, ySrc, ntris, tris);
    }
    glamor_finish_access_picture(pSrc);
    glamor_finish_access_picture(pDst);
}
