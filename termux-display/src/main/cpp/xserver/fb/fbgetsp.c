/*
 * Copyright © 1998 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "fb.h"

void
fbGetSpans(DrawablePtr pDrawable,
           int wMax,
           DDXPointPtr ppt, int *pwidth, int nspans, char *pchardstStart)
{
    FbBits *src, *dst;
    FbStride srcStride;
    int srcBpp;
    int srcXoff, srcYoff;
    int xoff;

    /*
     * XFree86 DDX empties the root borderClip when the VT is
     * switched away; this checks for that case
     */
    if (!fbDrawableEnabled(pDrawable))
        return;

    fbGetDrawable(pDrawable, src, srcStride, srcBpp, srcXoff, srcYoff);

    while (nspans--) {
        xoff = (int) (((long) pchardstStart) & (FB_MASK >> 3));
        dst = (FbBits *) (pchardstStart - xoff);
        xoff <<= 3;
        fbBlt(src + (ppt->y + srcYoff) * srcStride, srcStride,
              (ppt->x + srcXoff) * srcBpp,
              dst,
              1,
              xoff,
              *pwidth * srcBpp, 1, GXcopy, FB_ALLONES, srcBpp, FALSE, FALSE);
        pchardstStart += PixmapBytePad(*pwidth, pDrawable->depth);
        ppt++;
        pwidth++;
    }

    fbFinishAccess(pDrawable);
}
