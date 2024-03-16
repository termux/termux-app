/*
 * Copyright © 2007 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "misc.h"
#include "scrnintstr.h"
#include "os.h"
#include "regionstr.h"
#include "validate.h"
#include "windowstr.h"
#include "input.h"
#include "resource.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"
#include "picturestr.h"

void
PictTransform_from_xRenderTransform(PictTransformPtr pict,
                                    xRenderTransform * render)
{
    pict->matrix[0][0] = render->matrix11;
    pict->matrix[0][1] = render->matrix12;
    pict->matrix[0][2] = render->matrix13;

    pict->matrix[1][0] = render->matrix21;
    pict->matrix[1][1] = render->matrix22;
    pict->matrix[1][2] = render->matrix23;

    pict->matrix[2][0] = render->matrix31;
    pict->matrix[2][1] = render->matrix32;
    pict->matrix[2][2] = render->matrix33;
}

void
xRenderTransform_from_PictTransform(xRenderTransform * render,
                                    PictTransformPtr pict)
{
    render->matrix11 = pict->matrix[0][0];
    render->matrix12 = pict->matrix[0][1];
    render->matrix13 = pict->matrix[0][2];

    render->matrix21 = pict->matrix[1][0];
    render->matrix22 = pict->matrix[1][1];
    render->matrix23 = pict->matrix[1][2];

    render->matrix31 = pict->matrix[2][0];
    render->matrix32 = pict->matrix[2][1];
    render->matrix33 = pict->matrix[2][2];
}

Bool
PictureTransformPoint(PictTransformPtr transform, PictVectorPtr vector)
{
    return pixman_transform_point(transform, vector);
}

Bool
PictureTransformPoint3d(PictTransformPtr transform, PictVectorPtr vector)
{
    return pixman_transform_point_3d(transform, vector);
}
