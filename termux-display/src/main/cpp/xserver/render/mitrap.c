/*
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
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

#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mi.h"
#include "picturestr.h"
#include "mipict.h"

static xFixed
miLineFixedX(xLineFixed * l, xFixed y, Bool ceil)
{
    xFixed dx = l->p2.x - l->p1.x;
    xFixed_32_32 ex = (xFixed_32_32) (y - l->p1.y) * dx;
    xFixed dy = l->p2.y - l->p1.y;

    if (ceil)
        ex += (dy - 1);
    return l->p1.x + (xFixed) (ex / dy);
}

void
miTrapezoidBounds(int ntrap, xTrapezoid * traps, BoxPtr box)
{
    box->y1 = MAXSHORT;
    box->y2 = MINSHORT;
    box->x1 = MAXSHORT;
    box->x2 = MINSHORT;
    for (; ntrap; ntrap--, traps++) {
        INT16 x1, y1, x2, y2;

        if (!xTrapezoidValid(traps))
            continue;
        y1 = xFixedToInt(traps->top);
        if (y1 < box->y1)
            box->y1 = y1;

        y2 = xFixedToInt(xFixedCeil(traps->bottom));
        if (y2 > box->y2)
            box->y2 = y2;

        x1 = xFixedToInt(min(miLineFixedX(&traps->left, traps->top, FALSE),
                             miLineFixedX(&traps->left, traps->bottom, FALSE)));
        if (x1 < box->x1)
            box->x1 = x1;

        x2 = xFixedToInt(xFixedCeil
                         (max
                          (miLineFixedX(&traps->right, traps->top, TRUE),
                           miLineFixedX(&traps->right, traps->bottom, TRUE))));
        if (x2 > box->x2)
            box->x2 = x2;
    }
}
