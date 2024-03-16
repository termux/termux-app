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
#include "glxutil.h"
#include "glxext.h"
#include "indirect_dispatch.h"
#include "unpack.h"

int
__glXDispSwap_FeedbackBuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    GLsizei size;
    GLenum type;

    __GLX_DECLARE_SWAP_VARIABLES;
    __GLXcontext *cx;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 8);

    __GLX_SWAP_INT(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
    __GLX_SWAP_INT(pc + 0);
    __GLX_SWAP_INT(pc + 4);
    size = *(GLsizei *) (pc + 0);
    type = *(GLenum *) (pc + 4);
    if (cx->feedbackBufSize < size) {
        cx->feedbackBuf = reallocarray(cx->feedbackBuf,
                                       (size_t) size, __GLX_SIZE_FLOAT32);
        if (!cx->feedbackBuf) {
            cl->client->errorValue = size;
            return BadAlloc;
        }
        cx->feedbackBufSize = size;
    }
    glFeedbackBuffer(size, type, cx->feedbackBuf);
    return Success;
}

int
__glXDispSwap_SelectBuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    GLsizei size;

    __GLX_DECLARE_SWAP_VARIABLES;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 4);

    __GLX_SWAP_INT(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
    __GLX_SWAP_INT(pc + 0);
    size = *(GLsizei *) (pc + 0);
    if (cx->selectBufSize < size) {
        cx->selectBuf = reallocarray(cx->selectBuf,
                                     (size_t) size, __GLX_SIZE_CARD32);
        if (!cx->selectBuf) {
            cl->client->errorValue = size;
            return BadAlloc;
        }
        cx->selectBufSize = size;
    }
    glSelectBuffer(size, cx->selectBuf);
    return Success;
}

int
__glXDispSwap_RenderMode(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    xGLXRenderModeReply reply;
    GLint nitems = 0, retBytes = 0, retval, newModeCheck;
    GLubyte *retBuffer = NULL;
    GLenum newMode;

    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 4);

    __GLX_SWAP_INT(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
    __GLX_SWAP_INT(pc);
    newMode = *(GLenum *) pc;
    retval = glRenderMode(newMode);

    /* Check that render mode worked */
    glGetIntegerv(GL_RENDER_MODE, &newModeCheck);
    if (newModeCheck != newMode) {
        /* Render mode change failed.  Bail */
        newMode = newModeCheck;
        goto noChangeAllowed;
    }

    /*
     ** Render mode might have still failed if we get here.  But in this
     ** case we can't really tell, nor does it matter.  If it did fail, it
     ** will return 0, and thus we won't send any data across the wire.
     */

    switch (cx->renderMode) {
    case GL_RENDER:
        cx->renderMode = newMode;
        break;
    case GL_FEEDBACK:
        if (retval < 0) {
            /* Overflow happened. Copy the entire buffer */
            nitems = cx->feedbackBufSize;
        }
        else {
            nitems = retval;
        }
        retBytes = nitems * __GLX_SIZE_FLOAT32;
        retBuffer = (GLubyte *) cx->feedbackBuf;
        __GLX_SWAP_FLOAT_ARRAY((GLbyte *) retBuffer, nitems);
        cx->renderMode = newMode;
        break;
    case GL_SELECT:
        if (retval < 0) {
            /* Overflow happened.  Copy the entire buffer */
            nitems = cx->selectBufSize;
        }
        else {
            GLuint *bp = cx->selectBuf;
            GLint i;

            /*
             ** Figure out how many bytes of data need to be sent.  Parse
             ** the selection buffer to determine this fact as the
             ** return value is the number of hits, not the number of
             ** items in the buffer.
             */
            nitems = 0;
            i = retval;
            while (--i >= 0) {
                GLuint n;

                /* Parse select data for this hit */
                n = *bp;
                bp += 3 + n;
            }
            nitems = bp - cx->selectBuf;
        }
        retBytes = nitems * __GLX_SIZE_CARD32;
        retBuffer = (GLubyte *) cx->selectBuf;
        __GLX_SWAP_INT_ARRAY((GLbyte *) retBuffer, nitems);
        cx->renderMode = newMode;
        break;
    }

    /*
     ** First reply is the number of elements returned in the feedback or
     ** selection array, as per the API for glRenderMode itself.
     */
 noChangeAllowed:;
    reply = (xGLXRenderModeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = nitems,
        .retval = retval,
        .size = nitems,
        .newMode = newMode
    };
    __GLX_SWAP_SHORT(&reply.sequenceNumber);
    __GLX_SWAP_INT(&reply.length);
    __GLX_SWAP_INT(&reply.retval);
    __GLX_SWAP_INT(&reply.size);
    __GLX_SWAP_INT(&reply.newMode);
    WriteToClient(client, sz_xGLXRenderModeReply, &reply);
    if (retBytes) {
        WriteToClient(client, retBytes, retBuffer);
    }
    return Success;
}

int
__glXDispSwap_Flush(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    int error;

    __GLX_DECLARE_SWAP_VARIABLES;

    REQUEST_SIZE_MATCH(xGLXSingleReq);

    __GLX_SWAP_INT(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    glFlush();
    return Success;
}

int
__glXDispSwap_Finish(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    int error;
    xGLXSingleReply reply = { 0, };

    __GLX_DECLARE_SWAP_VARIABLES;

    REQUEST_SIZE_MATCH(xGLXSingleReq);

    __GLX_SWAP_INT(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    /* Do a local glFinish */
    glFinish();

    /* Send empty reply packet to indicate finish is finished */
    __GLX_BEGIN_REPLY(0);
    __GLX_PUT_RETVAL(0);
    __GLX_SWAP_REPLY_HEADER();
    __GLX_SEND_HEADER();

    return Success;
}

int
__glXDispSwap_GetString(__GLXclientState * cl, GLbyte * pc)
{
    return DoGetString(cl, pc, GL_TRUE);
}
