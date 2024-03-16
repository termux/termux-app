/*
 * (C) Copyright IBM Corporation 2005, 2006
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glxserver.h"
#include "glxbyteorder.h"
#include "glxext.h"
#include "singlesize.h"
#include "unpack.h"
#include "indirect_size_get.h"
#include "indirect_dispatch.h"

int
__glXDisp_GetCompressedTexImage(struct __GLXclientStateRec *cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx = __glXForceCurrent(cl, req->contextTag, &error);
    ClientPtr client = cl->client;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 8);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum target = *(GLenum *) (pc + 0);
        const GLint level = *(GLint *) (pc + 4);
        GLint compsize = 0;
        char *answer = NULL, answerBuffer[200];
        xGLXSingleReply reply = { 0, };

        glGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
                                 &compsize);

        if (compsize != 0) {
            PFNGLGETCOMPRESSEDTEXIMAGEARBPROC GetCompressedTexImageARB =
                __glGetProcAddress("glGetCompressedTexImageARB");
            __GLX_GET_ANSWER_BUFFER(answer, cl, compsize, 1);
            __glXClearErrorOccured();
            GetCompressedTexImageARB(target, level, answer);
        }

        if (__glXErrorOccured()) {
            __GLX_BEGIN_REPLY(0);
            __GLX_SEND_HEADER();
        }
        else {
            __GLX_BEGIN_REPLY(compsize);
            ((xGLXGetTexImageReply *) &reply)->width = compsize;
            __GLX_SEND_HEADER();
            __GLX_SEND_VOID_ARRAY(compsize);
        }

        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetCompressedTexImage(struct __GLXclientStateRec *cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_32(req->contextTag), &error);
    ClientPtr client = cl->client;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 8);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum target = (GLenum) bswap_32(*(int *) (pc + 0));
        const GLint level = (GLint) bswap_32(*(int *) (pc + 4));
        GLint compsize = 0;
        char *answer = NULL, answerBuffer[200];
        xGLXSingleReply reply = { 0, };

        glGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
                                 &compsize);

        if (compsize != 0) {
            PFNGLGETCOMPRESSEDTEXIMAGEARBPROC GetCompressedTexImageARB =
                __glGetProcAddress("glGetCompressedTexImageARB");
            __GLX_GET_ANSWER_BUFFER(answer, cl, compsize, 1);
            __glXClearErrorOccured();
            GetCompressedTexImageARB(target, level, answer);
        }

        if (__glXErrorOccured()) {
            __GLX_BEGIN_REPLY(0);
            __GLX_SEND_HEADER();
        }
        else {
            __GLX_BEGIN_REPLY(compsize);
            ((xGLXGetTexImageReply *) &reply)->width = compsize;
            __GLX_SEND_HEADER();
            __GLX_SEND_VOID_ARRAY(compsize);
        }

        error = Success;
    }

    return error;
}
