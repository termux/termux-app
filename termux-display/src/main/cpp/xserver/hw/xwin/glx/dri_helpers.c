/*
 * Copyright © 2014 Jon Turney
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
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <glx/glxserver.h>
#include <glx/glxutil.h>
#include <X11/extensions/windowsdriconst.h>

#include "indirect.h"
#include "winpriv.h"
#include "dri_helpers.h"
#include "win.h"

int
glxWinQueryDrawable(ClientPtr client, XID drawId, unsigned int *type, unsigned int *handle)
{
    __GLXWinDrawable *pDrawable;
    int err;

    if (validGlxDrawable(client, drawId, GLX_DRAWABLE_ANY,
                         DixReadAccess, (__GLXdrawable **)&pDrawable, &err)) {

        switch (pDrawable->base.type)
            {
            case GLX_DRAWABLE_WINDOW:
                {
                    HWND h = winGetWindowInfo((WindowPtr)(pDrawable->base.pDraw));
                    *handle = (uintptr_t)h;
                    *type = WindowsDRIDrawableWindow;
                }
                break;

            case GLX_DRAWABLE_PIXMAP:
                glxWinDeferredCreateDrawable(pDrawable, pDrawable->base.config);
                *handle = pDrawable->base.pDraw->id;
                // The XID is used to create a unique name for a file mapping
                // shared with the requesting process
                //
                // XXX: Alternatively, we could use an anonymous file mapping
                // and use DuplicateHandle to make pDrawable->hSection available
                // to the requesting process... ?
                *type = WindowsDRIDrawablePixmap;
                break;

            case GLX_DRAWABLE_PBUFFER:
                glxWinDeferredCreateDrawable(pDrawable, pDrawable->base.config);
                *handle = (uintptr_t)(pDrawable->hPbuffer);
                *type = WindowsDRIDrawablePbuffer;
                break;

            default:
                assert(FALSE);
                *handle = 0;
            }
    }
    else {
        HWND h;
        /* The drawId XID doesn't identify a GLX drawable.  The only other valid
           alternative is that it is the XID of a window drawable that is being
           used by the pre-GLX 1.3 interface */
        DrawablePtr pDraw;
        int rc = dixLookupDrawable(&pDraw, drawId, client, 0, DixGetAttrAccess);
        if (rc != Success || pDraw->type != DRAWABLE_WINDOW) {
            return err;
        }

        h = winGetWindowInfo((WindowPtr)(pDraw));
        *handle = (uintptr_t)h;
        *type = WindowsDRIDrawableWindow;
    }

    winDebug("glxWinQueryDrawable: type %d, handle %p\n", *type, (void *)(uintptr_t)*handle);
    return Success;
}

int
glxWinFBConfigIDToPixelFormatIndex(int scr, int fbConfigID)
{
    __GLXscreen *screen = glxGetScreen(screenInfo.screens[scr]);
    __GLXconfig *c;

    for (c = screen->fbconfigs;
         c != NULL;
         c = c->next) {
        if (c->fbconfigID == fbConfigID)
            return ((GLXWinConfig *)c)->pixelFormatIndex;
    }

    return 0;
}

Bool
glxWinGetScreenAiglxIsActive(ScreenPtr pScreen)
{
    winPrivScreenPtr pWinScreen = winGetScreenPriv(pScreen);
    return pWinScreen->fNativeGlActive;
}
