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
#include "singlesize.h"
#include "indirect_size_get.h"

/*
** These routines compute the size of variable-size returned parameters.
** Unlike the similar routines that do the same thing for variable-size
** incoming parameters, the samplegl library itself doesn't use these routines.
** Hence, they are located here, in the GLX extension library.
*/

GLint
__glReadPixels_size(GLenum format, GLenum type, GLint w, GLint h)
{
    return __glXImageSize(format, type, 0, w, h, 1, 0, 0, 0, 0, 4);
}

GLint
__glGetMap_size(GLenum target, GLenum query)
{
    GLint k, order = 0, majorMinor[2];

    /*
     ** Assume target and query are both valid.
     */
    switch (target) {
    case GL_MAP1_COLOR_4:
    case GL_MAP1_NORMAL:
    case GL_MAP1_INDEX:
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_VERTEX_4:
        switch (query) {
        case GL_COEFF:
            k = __glMap1d_size(target);
            glGetMapiv(target, GL_ORDER, &order);
            /*
             ** The query above might fail, but then order will be zero anyway.
             */
            return order * k;
        case GL_DOMAIN:
            return 2;
        case GL_ORDER:
            return 1;
        }
        break;
    case GL_MAP2_COLOR_4:
    case GL_MAP2_NORMAL:
    case GL_MAP2_INDEX:
    case GL_MAP2_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_VERTEX_4:
        switch (query) {
        case GL_COEFF:
            k = __glMap2d_size(target);
            majorMinor[0] = majorMinor[1] = 0;
            glGetMapiv(target, GL_ORDER, majorMinor);
            /*
             ** The query above might fail, but then majorMinor will be zeroes
             */
            return majorMinor[0] * majorMinor[1] * k;
        case GL_DOMAIN:
            return 4;
        case GL_ORDER:
            return 2;
        }
        break;
    }
    return -1;
}

GLint
__glGetMapdv_size(GLenum target, GLenum query)
{
    return __glGetMap_size(target, query);
}

GLint
__glGetMapfv_size(GLenum target, GLenum query)
{
    return __glGetMap_size(target, query);
}

GLint
__glGetMapiv_size(GLenum target, GLenum query)
{
    return __glGetMap_size(target, query);
}

GLint
__glGetPixelMap_size(GLenum map)
{
    GLint size;
    GLenum query;

    switch (map) {
    case GL_PIXEL_MAP_I_TO_I:
        query = GL_PIXEL_MAP_I_TO_I_SIZE;
        break;
    case GL_PIXEL_MAP_S_TO_S:
        query = GL_PIXEL_MAP_S_TO_S_SIZE;
        break;
    case GL_PIXEL_MAP_I_TO_R:
        query = GL_PIXEL_MAP_I_TO_R_SIZE;
        break;
    case GL_PIXEL_MAP_I_TO_G:
        query = GL_PIXEL_MAP_I_TO_G_SIZE;
        break;
    case GL_PIXEL_MAP_I_TO_B:
        query = GL_PIXEL_MAP_I_TO_B_SIZE;
        break;
    case GL_PIXEL_MAP_I_TO_A:
        query = GL_PIXEL_MAP_I_TO_A_SIZE;
        break;
    case GL_PIXEL_MAP_R_TO_R:
        query = GL_PIXEL_MAP_R_TO_R_SIZE;
        break;
    case GL_PIXEL_MAP_G_TO_G:
        query = GL_PIXEL_MAP_G_TO_G_SIZE;
        break;
    case GL_PIXEL_MAP_B_TO_B:
        query = GL_PIXEL_MAP_B_TO_B_SIZE;
        break;
    case GL_PIXEL_MAP_A_TO_A:
        query = GL_PIXEL_MAP_A_TO_A_SIZE;
        break;
    default:
        return -1;
    }
    glGetIntegerv(query, &size);
    return size;
}

GLint
__glGetPixelMapfv_size(GLenum map)
{
    return __glGetPixelMap_size(map);
}

GLint
__glGetPixelMapuiv_size(GLenum map)
{
    return __glGetPixelMap_size(map);
}

GLint
__glGetPixelMapusv_size(GLenum map)
{
    return __glGetPixelMap_size(map);
}

GLint
__glGetTexImage_size(GLenum target, GLint level, GLenum format,
                     GLenum type, GLint width, GLint height, GLint depth)
{
    return __glXImageSize(format, type, target, width, height, depth,
                          0, 0, 0, 0, 4);
}
