/*
 * (C) Copyright IBM Corporation 2006
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glxserver.h"
#include "glxutil.h"
#include "glxext.h"
#include "singlesize.h"
#include "unpack.h"
#include "indirect_size_get.h"
#include "indirect_dispatch.h"
#include "glxbyteorder.h"

static int DoSwapInterval(__GLXclientState * cl, GLbyte * pc, int do_swap);

int
DoSwapInterval(__GLXclientState * cl, GLbyte * pc, int do_swap)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    ClientPtr client = cl->client;
    const GLXContextTag tag = req->contextTag;
    __GLXcontext *cx;
    GLint interval;

    REQUEST_FIXED_SIZE(xGLXVendorPrivateReq, 4);

    cx = __glXLookupContextByTag(cl, tag);

    if ((cx == NULL) || (cx->pGlxScreen == NULL)) {
        client->errorValue = tag;
        return __glXError(GLXBadContext);
    }

    if (cx->pGlxScreen->swapInterval == NULL) {
        LogMessage(X_ERROR, "AIGLX: cx->pGlxScreen->swapInterval == NULL\n");
        client->errorValue = tag;
        return __glXError(GLXUnsupportedPrivateRequest);
    }

    if (cx->drawPriv == NULL) {
        client->errorValue = tag;
        return BadValue;
    }

    pc += __GLX_VENDPRIV_HDR_SIZE;
    interval = (do_swap)
        ? bswap_32(*(int *) (pc + 0))
        : *(int *) (pc + 0);

    if (interval <= 0)
        return BadValue;

    (void) (*cx->pGlxScreen->swapInterval) (cx->drawPriv, interval);
    return Success;
}

int
__glXDisp_SwapIntervalSGI(__GLXclientState * cl, GLbyte * pc)
{
    return DoSwapInterval(cl, pc, 0);
}

int
__glXDispSwap_SwapIntervalSGI(__GLXclientState * cl, GLbyte * pc)
{
    return DoSwapInterval(cl, pc, 1);
}
