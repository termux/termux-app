/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xprotostr.h>
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"
#include "mi.h"
#include "misc.h"

/* mi rectangles
   written by newman, with debts to all and sundry
*/

/* MIPOLYFILLRECT -- public entry for PolyFillRect request
 * very straight forward: translate rectangles if necessary
 * then call FillSpans to fill each rectangle.  We let FillSpans worry about
 * clipping to the destination
 */
_X_COLD void
miPolyFillRect(DrawablePtr pDrawable, GCPtr pGC, int nrectFill, /* number of rectangles to fill */
               xRectangle *prectInit    /* Pointer to first rectangle to fill */
    )
{
    int i;
    int height;
    int width;
    xRectangle *prect;
    int xorg;
    int yorg;
    int maxheight;
    DDXPointPtr pptFirst;
    DDXPointPtr ppt;
    int *pwFirst;
    int *pw;

    if (pGC->miTranslate) {
        xorg = pDrawable->x;
        yorg = pDrawable->y;
        prect = prectInit;
        maxheight = 0;
        for (i = 0; i < nrectFill; i++, prect++) {
            prect->x += xorg;
            prect->y += yorg;
            maxheight = max(maxheight, prect->height);
        }
    }
    else {
        prect = prectInit;
        maxheight = 0;
        for (i = 0; i < nrectFill; i++, prect++)
            maxheight = max(maxheight, prect->height);
    }

    pptFirst = xallocarray(maxheight, sizeof(DDXPointRec));
    pwFirst = xallocarray(maxheight, sizeof(int));
    if (!pptFirst || !pwFirst) {
        free(pwFirst);
        free(pptFirst);
        return;
    }

    prect = prectInit;
    while (nrectFill--) {
        ppt = pptFirst;
        pw = pwFirst;
        height = prect->height;
        width = prect->width;
        xorg = prect->x;
        yorg = prect->y;
        while (height--) {
            *pw++ = width;
            ppt->x = xorg;
            ppt->y = yorg;
            ppt++;
            yorg++;
        }
        (*pGC->ops->FillSpans) (pDrawable, pGC,
                                prect->height, pptFirst, pwFirst, 1);
        prect++;
    }
    free(pwFirst);
    free(pptFirst);
}
