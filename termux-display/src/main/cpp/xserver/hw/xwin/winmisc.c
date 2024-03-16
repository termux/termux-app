/*
 *Copyright (C) 2001-2004 Harold L Hunt II All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of Harold L Hunt II
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from Harold L Hunt II.
 *
 * Authors:	Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

/*
 * Count the number of one bits in a color mask.
 */

CARD8
winCountBits(DWORD dw)
{
    DWORD dwBits = 0;

    while (dw) {
        dwBits += (dw & 1);
        dw >>= 1;
    }

    return dwBits;
}

/*
 * Modify the screen pixmap to point to the new framebuffer address
 */

Bool
winUpdateFBPointer(ScreenPtr pScreen, void *pbits)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    /* Location of shadow framebuffer has changed */
    pScreenInfo->pfb = pbits;

    /* Update the screen pixmap */
    if (!(*pScreen->ModifyPixmapHeader) (pScreen->devPrivate,
                                         pScreen->width,
                                         pScreen->height,
                                         pScreen->rootDepth,
                                         BitsPerPixel(pScreen->rootDepth),
                                         PixmapBytePad(pScreenInfo->dwStride,
                                                       pScreenInfo->dwBPP),
                                         pScreenInfo->pfb)) {
        FatalError("winUpdateFramebufferPointer - Failed modifying "
                   "screen pixmap\n");
    }

    return TRUE;
}
