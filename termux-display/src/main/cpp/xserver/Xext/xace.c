/************************************************************

Author: Eamon Walsh <ewalsh@tycho.nsa.gov>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdarg.h>
#include "scrnintstr.h"
#include "extnsionst.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "xacestr.h"

_X_EXPORT CallbackListPtr XaceHooks[XACE_NUM_HOOKS] = { 0 };

/* Special-cased hook functions.  Called by Xserver.
 */
#undef XaceHookDispatch
int
XaceHookDispatch(ClientPtr client, int major)
{
    /* Call the extension dispatch hook */
    ExtensionEntry *ext = GetExtensionEntry(major);
    XaceExtAccessRec erec = { client, ext, DixUseAccess, Success };
    if (ext)
        CallCallbacks(&XaceHooks[XACE_EXT_DISPATCH], &erec);
    /* On error, pretend extension doesn't exist */
    return (erec.status == Success) ? Success : BadRequest;
}

int
XaceHookPropertyAccess(ClientPtr client, WindowPtr pWin,
                       PropertyPtr *ppProp, Mask access_mode)
{
    XacePropertyAccessRec rec = { client, pWin, ppProp, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_PROPERTY_ACCESS], &rec);
    return rec.status;
}

int
XaceHookSelectionAccess(ClientPtr client, Selection ** ppSel, Mask access_mode)
{
    XaceSelectionAccessRec rec = { client, ppSel, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_SELECTION_ACCESS], &rec);
    return rec.status;
}

/* Entry point for hook functions.  Called by Xserver.
 */
int
XaceHook(int hook, ...)
{
    union {
        XaceResourceAccessRec res;
        XaceDeviceAccessRec dev;
        XaceSendAccessRec send;
        XaceReceiveAccessRec recv;
        XaceClientAccessRec client;
        XaceExtAccessRec ext;
        XaceServerAccessRec server;
        XaceScreenAccessRec screen;
        XaceAuthAvailRec auth;
        XaceKeyAvailRec key;
    } u;
    int *prv = NULL;            /* points to return value from callback */
    va_list ap;                 /* argument list */

    if (!XaceHooks[hook])
        return Success;

    va_start(ap, hook);

    /* Marshal arguments for passing to callback.
     * Each callback has its own case, which sets up a structure to hold
     * the arguments and integer return parameter, or in some cases just
     * sets calldata directly to a single argument (with no return result)
     */
    switch (hook) {
    case XACE_RESOURCE_ACCESS:
        u.res.client = va_arg(ap, ClientPtr);
        u.res.id = va_arg(ap, XID);
        u.res.rtype = va_arg(ap, RESTYPE);
        u.res.res = va_arg(ap, void *);
        u.res.ptype = va_arg(ap, RESTYPE);
        u.res.parent = va_arg(ap, void *);
        u.res.access_mode = va_arg(ap, Mask);

        u.res.status = Success; /* default allow */
        prv = &u.res.status;
        break;
    case XACE_DEVICE_ACCESS:
        u.dev.client = va_arg(ap, ClientPtr);
        u.dev.dev = va_arg(ap, DeviceIntPtr);
        u.dev.access_mode = va_arg(ap, Mask);

        u.dev.status = Success; /* default allow */
        prv = &u.dev.status;
        break;
    case XACE_SEND_ACCESS:
        u.send.client = va_arg(ap, ClientPtr);
        u.send.dev = va_arg(ap, DeviceIntPtr);
        u.send.pWin = va_arg(ap, WindowPtr);

        u.send.events = va_arg(ap, xEventPtr);
        u.send.count = va_arg(ap, int);

        u.send.status = Success;        /* default allow */
        prv = &u.send.status;
        break;
    case XACE_RECEIVE_ACCESS:
        u.recv.client = va_arg(ap, ClientPtr);
        u.recv.pWin = va_arg(ap, WindowPtr);

        u.recv.events = va_arg(ap, xEventPtr);
        u.recv.count = va_arg(ap, int);

        u.recv.status = Success;        /* default allow */
        prv = &u.recv.status;
        break;
    case XACE_CLIENT_ACCESS:
        u.client.client = va_arg(ap, ClientPtr);
        u.client.target = va_arg(ap, ClientPtr);
        u.client.access_mode = va_arg(ap, Mask);

        u.client.status = Success;      /* default allow */
        prv = &u.client.status;
        break;
    case XACE_EXT_ACCESS:
        u.ext.client = va_arg(ap, ClientPtr);

        u.ext.ext = va_arg(ap, ExtensionEntry *);
        u.ext.access_mode = DixGetAttrAccess;
        u.ext.status = Success; /* default allow */
        prv = &u.ext.status;
        break;
    case XACE_SERVER_ACCESS:
        u.server.client = va_arg(ap, ClientPtr);
        u.server.access_mode = va_arg(ap, Mask);

        u.server.status = Success;      /* default allow */
        prv = &u.server.status;
        break;
    case XACE_SCREEN_ACCESS:
    case XACE_SCREENSAVER_ACCESS:
        u.screen.client = va_arg(ap, ClientPtr);
        u.screen.screen = va_arg(ap, ScreenPtr);
        u.screen.access_mode = va_arg(ap, Mask);

        u.screen.status = Success;      /* default allow */
        prv = &u.screen.status;
        break;
    case XACE_AUTH_AVAIL:
        u.auth.client = va_arg(ap, ClientPtr);
        u.auth.authId = va_arg(ap, XID);

        break;
    case XACE_KEY_AVAIL:
        u.key.event = va_arg(ap, xEventPtr);
        u.key.keybd = va_arg(ap, DeviceIntPtr);
        u.key.count = va_arg(ap, int);

        break;
    default:
        va_end(ap);
        return 0;               /* unimplemented hook number */
    }
    va_end(ap);

    /* call callbacks and return result, if any. */
    CallCallbacks(&XaceHooks[hook], &u);
    return prv ? *prv : Success;
}

/* XaceHookIsSet
 *
 * Utility function to determine whether there are any callbacks listening on a
 * particular XACE hook.
 *
 * Returns non-zero if there is a callback, zero otherwise.
 */
int
XaceHookIsSet(int hook)
{
    if (hook < 0 || hook >= XACE_NUM_HOOKS)
        return 0;
    return XaceHooks[hook] != NULL;
}

/* XaceCensorImage
 *
 * Called after pScreen->GetImage to prevent pieces or trusted windows from
 * being returned in image data from an untrusted window.
 *
 * Arguments:
 *	client is the client doing the GetImage.
 *      pVisibleRegion is the visible region of the window.
 *	widthBytesLine is the width in bytes of one horizontal line in pBuf.
 *	pDraw is the source window.
 *	x, y, w, h is the rectangle of image data from pDraw in pBuf.
 *	format is the format of the image data in pBuf: ZPixmap or XYPixmap.
 *	pBuf is the image data.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Any part of the rectangle (x, y, w, h) that is outside the visible
 *	region of the window will be destroyed (overwritten) in pBuf.
 */
void
XaceCensorImage(ClientPtr client,
                RegionPtr pVisibleRegion,
                long widthBytesLine,
                DrawablePtr pDraw,
                int x, int y, int w, int h, unsigned int format, char *pBuf)
{
    RegionRec imageRegion;      /* region representing x,y,w,h */
    RegionRec censorRegion;     /* region to obliterate */
    BoxRec imageBox;
    int nRects;

    imageBox.x1 = pDraw->x + x;
    imageBox.y1 = pDraw->y + y;
    imageBox.x2 = pDraw->x + x + w;
    imageBox.y2 = pDraw->y + y + h;
    RegionInit(&imageRegion, &imageBox, 1);
    RegionNull(&censorRegion);

    /* censorRegion = imageRegion - visibleRegion */
    RegionSubtract(&censorRegion, &imageRegion, pVisibleRegion);
    nRects = RegionNumRects(&censorRegion);
    if (nRects > 0) {           /* we have something to censor */
        GCPtr pScratchGC = NULL;
        PixmapPtr pPix = NULL;
        xRectangle *pRects = NULL;
        Bool failed = FALSE;
        int depth = 1;
        int bitsPerPixel = 1;
        int i;
        BoxPtr pBox;

        /* convert region to list-of-rectangles for PolyFillRect */

        pRects = malloc(nRects * sizeof(xRectangle));
        if (!pRects) {
            failed = TRUE;
            goto failSafe;
        }
        for (pBox = RegionRects(&censorRegion), i = 0; i < nRects; i++, pBox++) {
            pRects[i].x = pBox->x1 - imageBox.x1;
            pRects[i].y = pBox->y1 - imageBox.y1;
            pRects[i].width = pBox->x2 - pBox->x1;
            pRects[i].height = pBox->y2 - pBox->y1;
        }

        /* use pBuf as a fake pixmap */

        if (format == ZPixmap) {
            depth = pDraw->depth;
            bitsPerPixel = pDraw->bitsPerPixel;
        }

        pPix = GetScratchPixmapHeader(pDraw->pScreen, w, h,
                                      depth, bitsPerPixel,
                                      widthBytesLine, (void *) pBuf);
        if (!pPix) {
            failed = TRUE;
            goto failSafe;
        }

        pScratchGC = GetScratchGC(depth, pPix->drawable.pScreen);
        if (!pScratchGC) {
            failed = TRUE;
            goto failSafe;
        }

        ValidateGC(&pPix->drawable, pScratchGC);
        (*pScratchGC->ops->PolyFillRect) (&pPix->drawable,
                                          pScratchGC, nRects, pRects);

 failSafe:
        if (failed) {
            /* Censoring was not completed above.  To be safe, wipe out
             * all the image data so that nothing trusted gets out.
             */
            memset(pBuf, 0, (int) (widthBytesLine * h));
        }
        free(pRects);
        if (pScratchGC)
            FreeScratchGC(pScratchGC);
        if (pPix)
            FreeScratchPixmapHeader(pPix);
    }
    RegionUninit(&imageRegion);
    RegionUninit(&censorRegion);
}                               /* XaceCensorImage */

/*
 * Xtrans wrappers for use by modules
 */
int
XaceGetConnectionNumber(ClientPtr client)
{
    return GetClientFd(client);
}

int
XaceIsLocal(ClientPtr client)
{
    return ClientIsLocal(client);
}
