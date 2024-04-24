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

#include <GL/gl.h>
#include "glxserver.h"
#include "GL/glxproto.h"
#include "unpack.h"
#include "indirect_size.h"
#include "indirect_reqsize.h"

#define SWAPL(a) \
  (((a & 0xff000000U)>>24) | ((a & 0xff0000U)>>8) | \
   ((a & 0xff00U)<<8) | ((a & 0xffU)<<24))

int
__glXMap1dReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum target;
    GLint order;

    target = *(GLenum *) (pc + 16);
    order = *(GLint *) (pc + 20);
    if (swap) {
        target = SWAPL(target);
        order = SWAPL(order);
    }
    if (order < 1)
        return -1;
    return safe_mul(8, safe_mul(__glMap1d_size(target), order));
}

int
__glXMap1fReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum target;
    GLint order;

    target = *(GLenum *) (pc + 0);
    order = *(GLint *) (pc + 12);
    if (swap) {
        target = SWAPL(target);
        order = SWAPL(order);
    }
    if (order < 1)
        return -1;
    return safe_mul(4, safe_mul(__glMap1f_size(target), order));
}

static int
Map2Size(int k, int majorOrder, int minorOrder)
{
    if (majorOrder < 1 || minorOrder < 1)
        return -1;
    return safe_mul(k, safe_mul(majorOrder, minorOrder));
}

int
__glXMap2dReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum target;
    GLint uorder, vorder;

    target = *(GLenum *) (pc + 32);
    uorder = *(GLint *) (pc + 36);
    vorder = *(GLint *) (pc + 40);
    if (swap) {
        target = SWAPL(target);
        uorder = SWAPL(uorder);
        vorder = SWAPL(vorder);
    }
    return safe_mul(8, Map2Size(__glMap2d_size(target), uorder, vorder));
}

int
__glXMap2fReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    GLenum target;
    GLint uorder, vorder;

    target = *(GLenum *) (pc + 0);
    uorder = *(GLint *) (pc + 12);
    vorder = *(GLint *) (pc + 24);
    if (swap) {
        target = SWAPL(target);
        uorder = SWAPL(uorder);
        vorder = SWAPL(vorder);
    }
    return safe_mul(4, Map2Size(__glMap2f_size(target), uorder, vorder));
}

/**
 * Calculate the size of an image.
 *
 * The size of an image sent to the server from the client or sent from the
 * server to the client is calculated.  The size is based on the dimensions
 * of the image, the type of pixel data, padding in the image, and the
 * alignment requirements of the image.
 *
 * \param format       Format of the pixels.  Same as the \c format parameter
 *                     to \c glTexImage1D
 * \param type         Type of the pixel data.  Same as the \c type parameter
 *                     to \c glTexImage1D
 * \param target       Typically the texture target of the image.  If the
 *                     target is one of \c GL_PROXY_*, the size returned is
 *                     always zero. For uses that do not have a texture target
 *                     (e.g, glDrawPixels), zero should be specified.
 * \param w            Width of the image data.  Must be >= 1.
 * \param h            Height of the image data.  Must be >= 1, even for 1D
 *                     images.
 * \param d            Depth of the image data.  Must be >= 1, even for 1D or
 *                     2D images.
 * \param imageHeight  If non-zero, defines the true height of a volumetric
 *                     image.  This value will be used instead of \c h for
 *                     calculating the size of the image.
 * \param rowLength    If non-zero, defines the true width of an image.  This
 *                     value will be used instead of \c w for calculating the
 *                     size of the image.
 * \param skipImages   Number of extra layers of image data in a volumtric
 *                     image that are to be skipped before the real data.
 * \param skipRows     Number of extra rows of image data in an image that are
 *                     to be skipped before the real data.
 * \param alignment    Specifies the alignment for the start of each pixel row
 *                     in memory.  This value must be one of 1, 2, 4, or 8.
 *
 * \returns
 * The size of the image is returned.  If the specified \c format and \c type
 * are invalid, -1 is returned.  If \c target is one of \c GL_PROXY_*, zero
 * is returned.
 */
int
__glXImageSize(GLenum format, GLenum type, GLenum target,
               GLsizei w, GLsizei h, GLsizei d,
               GLint imageHeight, GLint rowLength,
               GLint skipImages, GLint skipRows, GLint alignment)
{
    GLint bytesPerElement, elementsPerGroup, groupsPerRow;
    GLint groupSize, rowSize, padding, imageSize;

    if (w == 0 || h == 0 || d == 0)
        return 0;

    if (w < 0 || h < 0 || d < 0 ||
        (type == GL_BITMAP &&
         (format != GL_COLOR_INDEX && format != GL_STENCIL_INDEX))) {
        return -1;
    }

    /* proxy targets have no data */
    switch (target) {
    case GL_PROXY_TEXTURE_1D:
    case GL_PROXY_TEXTURE_2D:
    case GL_PROXY_TEXTURE_3D:
    case GL_PROXY_TEXTURE_4D_SGIS:
    case GL_PROXY_TEXTURE_CUBE_MAP:
    case GL_PROXY_TEXTURE_RECTANGLE_ARB:
    case GL_PROXY_HISTOGRAM:
    case GL_PROXY_COLOR_TABLE:
    case GL_PROXY_TEXTURE_COLOR_TABLE_SGI:
    case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
    case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
    case GL_PROXY_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP:
        return 0;
    }

    /* real data has to have real sizes */
    if (imageHeight < 0 || rowLength < 0 || skipImages < 0 || skipRows < 0)
        return -1;
    if (alignment != 1 && alignment != 2 && alignment != 4 && alignment != 8)
        return -1;

    if (type == GL_BITMAP) {
        if (rowLength > 0) {
            groupsPerRow = rowLength;
        }
        else {
            groupsPerRow = w;
        }
        rowSize = bits_to_bytes(groupsPerRow);
        if (rowSize < 0)
            return -1;
        padding = (rowSize % alignment);
        if (padding) {
            rowSize += alignment - padding;
        }

        return safe_mul(safe_add(h, skipRows), rowSize);
    }
    else {
        switch (format) {
        case GL_COLOR_INDEX:
        case GL_STENCIL_INDEX:
        case GL_DEPTH_COMPONENT:
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_INTENSITY:
        case GL_RED_INTEGER_EXT:
        case GL_GREEN_INTEGER_EXT:
        case GL_BLUE_INTEGER_EXT:
        case GL_ALPHA_INTEGER_EXT:
        case GL_LUMINANCE_INTEGER_EXT:
            elementsPerGroup = 1;
            break;
        case GL_422_EXT:
        case GL_422_REV_EXT:
        case GL_422_AVERAGE_EXT:
        case GL_422_REV_AVERAGE_EXT:
        case GL_DEPTH_STENCIL_NV:
        case GL_DEPTH_STENCIL_MESA:
        case GL_YCBCR_422_APPLE:
        case GL_YCBCR_MESA:
        case GL_LUMINANCE_ALPHA:
        case GL_LUMINANCE_ALPHA_INTEGER_EXT:
            elementsPerGroup = 2;
            break;
        case GL_RGB:
        case GL_BGR:
        case GL_RGB_INTEGER_EXT:
        case GL_BGR_INTEGER_EXT:
            elementsPerGroup = 3;
            break;
        case GL_RGBA:
        case GL_BGRA:
        case GL_RGBA_INTEGER_EXT:
        case GL_BGRA_INTEGER_EXT:
        case GL_ABGR_EXT:
            elementsPerGroup = 4;
            break;
        default:
            return -1;
        }
        switch (type) {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
            bytesPerElement = 1;
            break;
        case GL_UNSIGNED_BYTE_3_3_2:
        case GL_UNSIGNED_BYTE_2_3_3_REV:
            bytesPerElement = 1;
            elementsPerGroup = 1;
            break;
        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
            bytesPerElement = 2;
            break;
        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_UNSIGNED_SHORT_5_6_5_REV:
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_4_4_4_4_REV:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_UNSIGNED_SHORT_1_5_5_5_REV:
        case GL_UNSIGNED_SHORT_8_8_APPLE:
        case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
        case GL_UNSIGNED_SHORT_15_1_MESA:
        case GL_UNSIGNED_SHORT_1_15_REV_MESA:
            bytesPerElement = 2;
            elementsPerGroup = 1;
            break;
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
            bytesPerElement = 4;
            break;
        case GL_UNSIGNED_INT_8_8_8_8:
        case GL_UNSIGNED_INT_8_8_8_8_REV:
        case GL_UNSIGNED_INT_10_10_10_2:
        case GL_UNSIGNED_INT_2_10_10_10_REV:
        case GL_UNSIGNED_INT_24_8_NV:
        case GL_UNSIGNED_INT_24_8_MESA:
        case GL_UNSIGNED_INT_8_24_REV_MESA:
            bytesPerElement = 4;
            elementsPerGroup = 1;
            break;
        default:
            return -1;
        }
        /* known safe by the switches above, not checked */
        groupSize = bytesPerElement * elementsPerGroup;
        if (rowLength > 0) {
            groupsPerRow = rowLength;
        }
        else {
            groupsPerRow = w;
        }

        if ((rowSize = safe_mul(groupsPerRow, groupSize)) < 0)
            return -1;
        padding = (rowSize % alignment);
        if (padding) {
            rowSize += alignment - padding;
        }

        if (imageHeight > 0)
            h = imageHeight;
        h = safe_add(h, skipRows);

        imageSize = safe_mul(h, rowSize);

        return safe_mul(safe_add(d, skipImages), imageSize);
    }
}

/* XXX this is used elsewhere - should it be exported from glxserver.h? */
int
__glXTypeSize(GLenum enm)
{
    switch (enm) {
    case GL_BYTE:
        return sizeof(GLbyte);
    case GL_UNSIGNED_BYTE:
        return sizeof(GLubyte);
    case GL_SHORT:
        return sizeof(GLshort);
    case GL_UNSIGNED_SHORT:
        return sizeof(GLushort);
    case GL_INT:
        return sizeof(GLint);
    case GL_UNSIGNED_INT:
        return sizeof(GLint);
    case GL_FLOAT:
        return sizeof(GLfloat);
    case GL_DOUBLE:
        return sizeof(GLdouble);
    default:
        return -1;
    }
}

int
__glXDrawArraysReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    __GLXdispatchDrawArraysHeader *hdr = (__GLXdispatchDrawArraysHeader *) pc;
    __GLXdispatchDrawArraysComponentHeader *compHeader;
    GLint numVertexes = hdr->numVertexes;
    GLint numComponents = hdr->numComponents;
    GLint arrayElementSize = 0;
    GLint x, size;
    int i;

    if (swap) {
        numVertexes = SWAPL(numVertexes);
        numComponents = SWAPL(numComponents);
    }

    pc += sizeof(__GLXdispatchDrawArraysHeader);
    reqlen -= sizeof(__GLXdispatchDrawArraysHeader);

    size = safe_mul(sizeof(__GLXdispatchDrawArraysComponentHeader),
                    numComponents);
    if (size < 0 || reqlen < 0 || reqlen < size)
        return -1;

    compHeader = (__GLXdispatchDrawArraysComponentHeader *) pc;

    for (i = 0; i < numComponents; i++) {
        GLenum datatype = compHeader[i].datatype;
        GLint numVals = compHeader[i].numVals;
        GLint component = compHeader[i].component;

        if (swap) {
            datatype = SWAPL(datatype);
            numVals = SWAPL(numVals);
            component = SWAPL(component);
        }

        switch (component) {
        case GL_VERTEX_ARRAY:
        case GL_COLOR_ARRAY:
        case GL_TEXTURE_COORD_ARRAY:
            break;
        case GL_SECONDARY_COLOR_ARRAY:
        case GL_NORMAL_ARRAY:
            if (numVals != 3) {
                /* bad size */
                return -1;
            }
            break;
        case GL_FOG_COORD_ARRAY:
        case GL_INDEX_ARRAY:
            if (numVals != 1) {
                /* bad size */
                return -1;
            }
            break;
        case GL_EDGE_FLAG_ARRAY:
            if ((numVals != 1) && (datatype != GL_UNSIGNED_BYTE)) {
                /* bad size or bad type */
                return -1;
            }
            break;
        default:
            /* unknown component type */
            return -1;
        }

        x = safe_pad(safe_mul(numVals, __glXTypeSize(datatype)));
        if ((arrayElementSize = safe_add(arrayElementSize, x)) < 0)
            return -1;

        pc += sizeof(__GLXdispatchDrawArraysComponentHeader);
    }

    return safe_add(size, safe_mul(numVertexes, arrayElementSize));
}

int
__glXSeparableFilter2DReqSize(const GLbyte * pc, Bool swap, int reqlen)
{
    __GLXdispatchConvolutionFilterHeader *hdr =
        (__GLXdispatchConvolutionFilterHeader *) pc;

    GLint image1size, image2size;
    GLenum format = hdr->format;
    GLenum type = hdr->type;
    GLint w = hdr->width;
    GLint h = hdr->height;
    GLint rowLength = hdr->rowLength;
    GLint alignment = hdr->alignment;

    if (swap) {
        format = SWAPL(format);
        type = SWAPL(type);
        w = SWAPL(w);
        h = SWAPL(h);
        rowLength = SWAPL(rowLength);
        alignment = SWAPL(alignment);
    }

    /* XXX Should rowLength be used for either or both image? */
    image1size = __glXImageSize(format, type, 0, w, 1, 1,
                                0, rowLength, 0, 0, alignment);
    image2size = __glXImageSize(format, type, 0, h, 1, 1,
                                0, rowLength, 0, 0, alignment);
    return safe_add(safe_pad(image1size), image2size);
}
