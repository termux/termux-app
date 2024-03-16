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

#ifndef _RRTRANSFORM_H_
#define _RRTRANSFORM_H_

#include <X11/extensions/randr.h>
#include "picturestr.h"

typedef struct _rrTransform RRTransformRec, *RRTransformPtr;

struct _rrTransform {
    PictTransform transform;
    struct pict_f_transform f_transform;
    struct pict_f_transform f_inverse;
    PictFilterPtr filter;
    xFixed *params;
    int nparams;
    int width;
    int height;
};

extern _X_EXPORT void
 RRTransformInit(RRTransformPtr transform);

extern _X_EXPORT void
 RRTransformFini(RRTransformPtr transform);

extern _X_EXPORT Bool
 RRTransformEqual(RRTransformPtr a, RRTransformPtr b);

extern _X_EXPORT Bool

RRTransformSetFilter(RRTransformPtr dst,
                     PictFilterPtr filter,
                     xFixed * params, int nparams, int width, int height);

extern _X_EXPORT Bool
 RRTransformCopy(RRTransformPtr dst, RRTransformPtr src);

/*
 * Compute the complete transformation matrix including
 * client-specified transform, rotation/reflection values and the crtc
 * offset.
 *
 * Return TRUE if the resulting transform is not a simple translation.
 */
extern _X_EXPORT Bool

RRTransformCompute(int x,
                   int y,
                   int width,
                   int height,
                   Rotation rotation,
                   RRTransformPtr rr_transform,
                   PictTransformPtr transform,
                   struct pict_f_transform *f_transform,
                   struct pict_f_transform *f_inverse);

#endif                          /* _RRTRANSFORM_H_ */
