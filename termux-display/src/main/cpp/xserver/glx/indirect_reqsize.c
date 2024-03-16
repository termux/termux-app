/* DO NOT EDIT - This file generated automatically by glX_proto_size.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <GL/gl.h>
#include "glxserver.h"
#include "glxbyteorder.h"
#include "indirect_size.h"
#include "indirect_reqsize.h"

#if defined(__CYGWIN__) || defined(__MINGW32__)
#undef HAVE_ALIAS
#endif
#ifdef HAVE_ALIAS
#define ALIAS2(from,to) \
    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap, int reqlen ) \
        __attribute__ ((alias( # to )));
#define ALIAS(from,to) ALIAS2( from, __glX ## to ## ReqSize )
#else
#define ALIAS(from,to) \
    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap, int reqlen ) \
    { return __glX ## to ## ReqSize( pc, swap, reqlen ); }
#endif

int
__glXCallListsReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 0);
    GLenum type = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        n = bswap_32(n);
        type = bswap_32(type);
    }

    compsize = __glCallLists_size(type);
    return safe_pad(safe_mul(compsize, n));
}

int
__glXBitmapReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLsizei width = *(GLsizei *) (pc + 20);
    GLsizei height = *(GLsizei *) (pc + 24);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        width = bswap_32(width);
        height = bswap_32(height);
    }

    return __glXImageSize(GL_COLOR_INDEX, GL_BITMAP, 0, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXFogfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 0);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glFogfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXLightfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glLightfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXLightModelfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 0);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glLightModelfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXMaterialfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glMaterialfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXPolygonStippleReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
    }

    return __glXImageSize(GL_COLOR_INDEX, GL_BITMAP, 0, 32, 32, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexParameterfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glTexParameterfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXTexImage1DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLenum target = *(GLenum *) (pc + 20);
    GLsizei width = *(GLsizei *) (pc + 32);
    GLenum format = *(GLenum *) (pc + 44);
    GLenum type = *(GLenum *) (pc + 48);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, width, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexImage2DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLenum target = *(GLenum *) (pc + 20);
    GLsizei width = *(GLsizei *) (pc + 32);
    GLsizei height = *(GLsizei *) (pc + 36);
    GLenum format = *(GLenum *) (pc + 44);
    GLenum type = *(GLenum *) (pc + 48);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        height = bswap_32(height);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexEnvfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glTexEnvfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXTexGendvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glTexGendv_size(pname);
    return safe_pad(safe_mul(compsize, 8));
}

int
__glXTexGenfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glTexGenfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXPixelMapfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei mapsize = *(GLsizei *) (pc + 4);

    if (swap) {
        mapsize = bswap_32(mapsize);
    }

    return safe_pad(safe_mul(mapsize, 4));
}

int
__glXPixelMapusvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei mapsize = *(GLsizei *) (pc + 4);

    if (swap) {
        mapsize = bswap_32(mapsize);
    }

    return safe_pad(safe_mul(mapsize, 2));
}

int
__glXDrawPixelsReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLsizei width = *(GLsizei *) (pc + 20);
    GLsizei height = *(GLsizei *) (pc + 24);
    GLenum format = *(GLenum *) (pc + 28);
    GLenum type = *(GLenum *) (pc + 32);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        width = bswap_32(width);
        height = bswap_32(height);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, 0, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXPrioritizeTexturesReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 0);

    if (swap) {
        n = bswap_32(n);
    }

    return safe_pad(safe_add(safe_mul(n, 4), safe_mul(n, 4)));
}

int
__glXTexSubImage1DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLenum target = *(GLenum *) (pc + 20);
    GLsizei width = *(GLsizei *) (pc + 36);
    GLenum format = *(GLenum *) (pc + 44);
    GLenum type = *(GLenum *) (pc + 48);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, width, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexSubImage2DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLenum target = *(GLenum *) (pc + 20);
    GLsizei width = *(GLsizei *) (pc + 36);
    GLsizei height = *(GLsizei *) (pc + 40);
    GLenum format = *(GLenum *) (pc + 44);
    GLenum type = *(GLenum *) (pc + 48);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        height = bswap_32(height);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXColorTableReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLenum target = *(GLenum *) (pc + 20);
    GLsizei width = *(GLsizei *) (pc + 28);
    GLenum format = *(GLenum *) (pc + 32);
    GLenum type = *(GLenum *) (pc + 36);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, width, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXColorTableParameterfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glColorTableParameterfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXColorSubTableReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLenum target = *(GLenum *) (pc + 20);
    GLsizei count = *(GLsizei *) (pc + 28);
    GLenum format = *(GLenum *) (pc + 32);
    GLenum type = *(GLenum *) (pc + 36);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        count = bswap_32(count);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, count, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXConvolutionFilter1DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLenum target = *(GLenum *) (pc + 20);
    GLsizei width = *(GLsizei *) (pc + 28);
    GLenum format = *(GLenum *) (pc + 36);
    GLenum type = *(GLenum *) (pc + 40);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, width, 1, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXConvolutionFilter2DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = 0;
    GLint skip_images = 0;
    GLint skip_rows = *(GLint *) (pc + 8);
    GLint alignment = *(GLint *) (pc + 16);
    GLenum target = *(GLenum *) (pc + 20);
    GLsizei width = *(GLsizei *) (pc + 28);
    GLsizei height = *(GLsizei *) (pc + 32);
    GLenum format = *(GLenum *) (pc + 36);
    GLenum type = *(GLenum *) (pc + 40);

    if (swap) {
        row_length = bswap_32(row_length);
        skip_rows = bswap_32(skip_rows);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        height = bswap_32(height);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, width, height, 1,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXConvolutionParameterfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 4);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glConvolutionParameterfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXTexImage3DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = *(GLint *) (pc + 8);
    GLint skip_rows = *(GLint *) (pc + 16);
    GLint skip_images = *(GLint *) (pc + 20);
    GLint alignment = *(GLint *) (pc + 32);
    GLenum target = *(GLenum *) (pc + 36);
    GLsizei width = *(GLsizei *) (pc + 48);
    GLsizei height = *(GLsizei *) (pc + 52);
    GLsizei depth = *(GLsizei *) (pc + 56);
    GLenum format = *(GLenum *) (pc + 68);
    GLenum type = *(GLenum *) (pc + 72);

    if (swap) {
        row_length = bswap_32(row_length);
        image_height = bswap_32(image_height);
        skip_rows = bswap_32(skip_rows);
        skip_images = bswap_32(skip_images);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        height = bswap_32(height);
        depth = bswap_32(depth);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    if (*(CARD32 *) (pc + 76))
        return 0;

    return __glXImageSize(format, type, target, width, height, depth,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXTexSubImage3DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLint row_length = *(GLint *) (pc + 4);
    GLint image_height = *(GLint *) (pc + 8);
    GLint skip_rows = *(GLint *) (pc + 16);
    GLint skip_images = *(GLint *) (pc + 20);
    GLint alignment = *(GLint *) (pc + 32);
    GLenum target = *(GLenum *) (pc + 36);
    GLsizei width = *(GLsizei *) (pc + 60);
    GLsizei height = *(GLsizei *) (pc + 64);
    GLsizei depth = *(GLsizei *) (pc + 68);
    GLenum format = *(GLenum *) (pc + 76);
    GLenum type = *(GLenum *) (pc + 80);

    if (swap) {
        row_length = bswap_32(row_length);
        image_height = bswap_32(image_height);
        skip_rows = bswap_32(skip_rows);
        skip_images = bswap_32(skip_images);
        alignment = bswap_32(alignment);
        target = bswap_32(target);
        width = bswap_32(width);
        height = bswap_32(height);
        depth = bswap_32(depth);
        format = bswap_32(format);
        type = bswap_32(type);
    }

    return __glXImageSize(format, type, target, width, height, depth,
                          image_height, row_length, skip_images,
                          skip_rows, alignment);
}

int
__glXCompressedTexImage1DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei imageSize = *(GLsizei *) (pc + 20);

    if (swap) {
        imageSize = bswap_32(imageSize);
    }

    return safe_pad(imageSize);
}

int
__glXCompressedTexImage2DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei imageSize = *(GLsizei *) (pc + 24);

    if (swap) {
        imageSize = bswap_32(imageSize);
    }

    return safe_pad(imageSize);
}

int
__glXCompressedTexImage3DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei imageSize = *(GLsizei *) (pc + 28);

    if (swap) {
        imageSize = bswap_32(imageSize);
    }

    return safe_pad(imageSize);
}

int
__glXCompressedTexSubImage3DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei imageSize = *(GLsizei *) (pc + 36);

    if (swap) {
        imageSize = bswap_32(imageSize);
    }

    return safe_pad(imageSize);
}

int
__glXPointParameterfvReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum pname = *(GLenum *) (pc + 0);
    GLsizei compsize;

    if (swap) {
        pname = bswap_32(pname);
    }

    compsize = __glPointParameterfv_size(pname);
    return safe_pad(safe_mul(compsize, 4));
}

int
__glXDrawBuffersReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 0);

    if (swap) {
        n = bswap_32(n);
    }

    return safe_pad(safe_mul(n, 4));
}

int
__glXProgramStringARBReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei len = *(GLsizei *) (pc + 8);

    if (swap) {
        len = bswap_32(len);
    }

    return safe_pad(len);
}

int
__glXVertexAttribs1dvNVReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 4);

    if (swap) {
        n = bswap_32(n);
    }

    return safe_pad(safe_mul(n, 8));
}

int
__glXVertexAttribs2dvNVReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 4);

    if (swap) {
        n = bswap_32(n);
    }

    return safe_pad(safe_mul(n, 16));
}

int
__glXVertexAttribs3dvNVReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 4);

    if (swap) {
        n = bswap_32(n);
    }

    return safe_pad(safe_mul(n, 24));
}

int
__glXVertexAttribs3fvNVReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 4);

    if (swap) {
        n = bswap_32(n);
    }

    return safe_pad(safe_mul(n, 12));
}

int
__glXVertexAttribs3svNVReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 4);

    if (swap) {
        n = bswap_32(n);
    }

    return safe_pad(safe_mul(n, 6));
}

int
__glXVertexAttribs4dvNVReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLsizei n = *(GLsizei *) (pc + 4);

    if (swap) {
        n = bswap_32(n);
    }

    return safe_pad(safe_mul(n, 32));
}

ALIAS(Fogiv, Fogfv)
    ALIAS(Lightiv, Lightfv)
    ALIAS(LightModeliv, LightModelfv)
    ALIAS(Materialiv, Materialfv)
    ALIAS(TexParameteriv, TexParameterfv)
    ALIAS(TexEnviv, TexEnvfv)
    ALIAS(TexGeniv, TexGenfv)
    ALIAS(PixelMapuiv, PixelMapfv)
    ALIAS(ColorTableParameteriv, ColorTableParameterfv)
    ALIAS(ConvolutionParameteriv, ConvolutionParameterfv)
    ALIAS(CompressedTexSubImage1D, CompressedTexImage1D)
    ALIAS(CompressedTexSubImage2D, CompressedTexImage3D)
    ALIAS(PointParameteriv, PointParameterfv)
    ALIAS(DeleteFramebuffers, DrawBuffers)
    ALIAS(DeleteRenderbuffers, DrawBuffers)
    ALIAS(VertexAttribs1fvNV, PixelMapfv)
    ALIAS(VertexAttribs1svNV, PixelMapusv)
    ALIAS(VertexAttribs2fvNV, VertexAttribs1dvNV)
    ALIAS(VertexAttribs2svNV, PixelMapfv)
    ALIAS(VertexAttribs4fvNV, VertexAttribs2dvNV)
    ALIAS(VertexAttribs4svNV, VertexAttribs1dvNV)
    ALIAS(VertexAttribs4ubvNV, PixelMapfv)
