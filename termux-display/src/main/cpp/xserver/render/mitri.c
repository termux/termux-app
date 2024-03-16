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
#include "mi.h"
#include "picturestr.h"
#include "mipict.h"

void
miPointFixedBounds(int npoint, xPointFixed * points, BoxPtr bounds)
{
    bounds->x1 = xFixedToInt(points->x);
    bounds->x2 = xFixedToInt(xFixedCeil(points->x));
    bounds->y1 = xFixedToInt(points->y);
    bounds->y2 = xFixedToInt(xFixedCeil(points->y));
    points++;
    npoint--;
    while (npoint-- > 0) {
        INT16 x1 = xFixedToInt(points->x);
        INT16 x2 = xFixedToInt(xFixedCeil(points->x));
        INT16 y1 = xFixedToInt(points->y);
        INT16 y2 = xFixedToInt(xFixedCeil(points->y));

        if (x1 < bounds->x1)
            bounds->x1 = x1;
        else if (x2 > bounds->x2)
            bounds->x2 = x2;
        if (y1 < bounds->y1)
            bounds->y1 = y1;
        else if (y2 > bounds->y2)
            bounds->y2 = y2;
        points++;
    }
}

void
miTriangleBounds(int ntri, xTriangle * tris, BoxPtr bounds)
{
    miPointFixedBounds(ntri * 3, (xPointFixed *) tris, bounds);
}
