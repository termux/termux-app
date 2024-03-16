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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "glxserver.h"
#include "glxutil.h"
#include "glxext.h"
#include "indirect_dispatch.h"
#include "unpack.h"

int
__glXDisp_FeedbackBuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    GLsizei size;
    GLenum type;
    __GLXcontext *cx;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 8);

    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
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
__glXDisp_SelectBuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    GLsizei size;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 4);

    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
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
__glXDisp_RenderMode(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXRenderModeReply reply;
    __GLXcontext *cx;
    GLint nitems = 0, retBytes = 0, retval, newModeCheck;
    GLubyte *retBuffer = NULL;
    GLenum newMode;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 4);

    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
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
    WriteToClient(client, sz_xGLXRenderModeReply, &reply);
    if (retBytes) {
        WriteToClient(client, retBytes, retBuffer);
    }
    return Success;
}

int
__glXDisp_Flush(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    int error;

    REQUEST_SIZE_MATCH(xGLXSingleReq);

    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    glFlush();
    return Success;
}

int
__glXDisp_Finish(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    int error;
    xGLXSingleReply reply = { 0, };

    REQUEST_SIZE_MATCH(xGLXSingleReq);

    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    /* Do a local glFinish */
    glFinish();

    /* Send empty reply packet to indicate finish is finished */
    client = cl->client;
    __GLX_BEGIN_REPLY(0);
    __GLX_SEND_HEADER();
    return Success;
}

#define SEPARATOR " "

static char *
__glXcombine_strings(const char *cext_string, const char *sext_string)
{
    size_t clen, slen;
    char *combo_string, *token, *s1;
    const char *s2, *end;

    /* safeguard to prevent potentially fatal errors in the string functions */
    if (!cext_string)
        cext_string = "";
    if (!sext_string)
        sext_string = "";

    /*
     ** String can't be longer than min(cstring, sstring)
     ** pull tokens out of shortest string
     ** include space in combo_string for final separator and null terminator
     */
    clen = strlen(cext_string);
    slen = strlen(sext_string);
    if (clen > slen) {
        combo_string = (char *) malloc(slen + 2);
        s1 = (char *) malloc(slen + 2);
        if (s1)
            strcpy(s1, sext_string);
        s2 = cext_string;
    }
    else {
        combo_string = (char *) malloc(clen + 2);
        s1 = (char *) malloc(clen + 2);
        if (s1)
            strcpy(s1, cext_string);
        s2 = sext_string;
    }
    if (!combo_string || !s1) {
        free(combo_string);
        free(s1);
        return NULL;
    }
    combo_string[0] = '\0';

    /* Get first extension token */
    token = strtok(s1, SEPARATOR);
    while (token != NULL) {

        /*
         ** if token in second string then save it
         ** beware of extension names which are prefixes of other extension names
         */
        const char *p = s2;

        end = p + strlen(p);
        while (p < end) {
            size_t n = strcspn(p, SEPARATOR);

            if ((strlen(token) == n) && (strncmp(token, p, n) == 0)) {
                combo_string = strcat(combo_string, token);
                combo_string = strcat(combo_string, SEPARATOR);
            }
            p += (n + 1);
        }

        /* Get next extension token */
        token = strtok(NULL, SEPARATOR);
    }
    free(s1);
    return combo_string;
}

int
DoGetString(__GLXclientState * cl, GLbyte * pc, GLboolean need_swap)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    GLenum name;
    const char *string;
    xGLXSingleReply reply = { 0, };

    __GLX_DECLARE_SWAP_VARIABLES;
    int error;
    char *buf = NULL, *buf1 = NULL;
    GLint length = 0;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 4);

    /* If the client has the opposite byte order, swap the contextTag and
     * the name.
     */
    if (need_swap) {
        __GLX_SWAP_INT(pc + 4);
        __GLX_SWAP_INT(pc + __GLX_SINGLE_HDR_SIZE);
    }

    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
    name = *(GLenum *) (pc + 0);
    string = (const char *) glGetString(name);

    if (string == NULL)
        string = "";

    /*
     ** Restrict extensions to those that are supported by both the
     ** implementation and the connection.  That is, return the
     ** intersection of client, server, and core extension strings.
     */
    if (name == GL_EXTENSIONS) {
        buf1 = __glXcombine_strings(string, cl->GLClientextensions);
        buf = __glXcombine_strings(buf1, cx->pGlxScreen->GLextensions);
        free(buf1);
        string = buf;
    }
    else if (name == GL_VERSION) {
        if (atof(string) > atof(GLServerVersion)) {
            if (asprintf(&buf, "%s (%s)", GLServerVersion, string) == -1) {
                string = GLServerVersion;
            }
            else {
                string = buf;
            }
        }
    }
    if (string) {
        length = strlen((const char *) string) + 1;
    }

    __GLX_BEGIN_REPLY(length);
    __GLX_PUT_SIZE(length);

    if (need_swap) {
        __GLX_SWAP_REPLY_SIZE();
        __GLX_SWAP_REPLY_HEADER();
    }

    __GLX_SEND_HEADER();
    WriteToClient(client, length, string);
    free(buf);

    return Success;
}

int
__glXDisp_GetString(__GLXclientState * cl, GLbyte * pc)
{
    return DoGetString(cl, pc, GL_FALSE);
}
