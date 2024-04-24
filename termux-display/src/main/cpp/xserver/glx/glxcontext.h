#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _GLX_context_h_
#define _GLX_context_h_

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

struct __GLXcontext {
    void (*destroy) (__GLXcontext * context);
    int (*makeCurrent) (__GLXcontext * context);
    int (*loseCurrent) (__GLXcontext * context);
    int (*copy) (__GLXcontext * dst, __GLXcontext * src, unsigned long mask);
    Bool (*wait) (__GLXcontext * context, __GLXclientState * cl, int *error);

    /* EXT_texture_from_pixmap */
    int (*bindTexImage) (__GLXcontext * baseContext,
                         int buffer, __GLXdrawable * pixmap);
    int (*releaseTexImage) (__GLXcontext * baseContext,
                            int buffer, __GLXdrawable * pixmap);

    /*
     ** list of context structs
     */
    __GLXcontext *next;

    /*
     ** config struct for this context
     */
    __GLXconfig *config;

    /*
     ** Pointer to screen info data for this context.  This is set
     ** when the context is created.
     */
    __GLXscreen *pGlxScreen;

    /*
     ** If this context is current for a client, this will be that client
     */
    ClientPtr currentClient;

    /*
     ** The XID of this context.
     */
    XID id;

    /*
     ** The XID of the shareList context.
     */
    XID share_id;

    /*
     ** Whether this context's ID still exists.
     */
    GLboolean idExists;

    /*
     ** Whether this context is a direct rendering context.
     */
    GLboolean isDirect;

    /*
     ** Current rendering mode for this context.
     */
    GLenum renderMode;

    /**
     * Reset notification strategy used when a GPU reset occurs.
     */
    GLenum resetNotificationStrategy;

    /**
     * Context release behavior
     */
    GLenum releaseBehavior;

    /**
     * Context render type
     */
    int renderType;

    /*
     ** Buffers for feedback and selection.
     */
    GLfloat *feedbackBuf;
    GLint feedbackBufSize;      /* number of elements allocated */
    GLuint *selectBuf;
    GLint selectBufSize;        /* number of elements allocated */

    /*
     ** Keep track of large rendering commands, which span multiple requests.
     */
    GLint largeCmdBytesSoFar;   /* bytes received so far        */
    GLint largeCmdBytesTotal;   /* total bytes expected         */
    GLint largeCmdRequestsSoFar;        /* requests received so far     */
    GLint largeCmdRequestsTotal;        /* total requests expected      */
    GLbyte *largeCmdBuf;
    GLint largeCmdBufSize;

    /*
     ** The drawable private this context is bound to
     */
    __GLXdrawable *drawPriv;
    __GLXdrawable *readPriv;
};

void __glXContextDestroy(__GLXcontext * context);

extern int validGlxScreen(ClientPtr client, int screen,
                          __GLXscreen ** pGlxScreen, int *err);

extern int validGlxFBConfig(ClientPtr client, __GLXscreen * pGlxScreen,
                            XID id, __GLXconfig ** config, int *err);

extern int validGlxContext(ClientPtr client, XID id, int access_mode,
                           __GLXcontext ** context, int *err);

extern __GLXcontext *__glXdirectContextCreate(__GLXscreen * screen,
                                              __GLXconfig * modes,
                                              __GLXcontext * shareContext);

#endif                          /* !__GLX_context_h__ */
