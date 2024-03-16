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

#include "randrstr.h"
#include "rrtransform.h"

void
RRTransformInit(RRTransformPtr transform)
{
    pixman_transform_init_identity(&transform->transform);
    pixman_f_transform_init_identity(&transform->f_transform);
    pixman_f_transform_init_identity(&transform->f_inverse);
    transform->filter = NULL;
    transform->params = NULL;
    transform->nparams = 0;
}

void
RRTransformFini(RRTransformPtr transform)
{
    free(transform->params);
}

Bool
RRTransformEqual(RRTransformPtr a, RRTransformPtr b)
{
    if (a && pixman_transform_is_identity(&a->transform))
        a = NULL;
    if (b && pixman_transform_is_identity(&b->transform))
        b = NULL;
    if (a == NULL && b == NULL)
        return TRUE;
    if (a == NULL || b == NULL)
        return FALSE;
    if (memcmp(&a->transform, &b->transform, sizeof(a->transform)) != 0)
        return FALSE;
    if (a->filter != b->filter)
        return FALSE;
    if (a->nparams != b->nparams)
        return FALSE;
    if (memcmp(a->params, b->params, a->nparams * sizeof(xFixed)) != 0)
        return FALSE;
    return TRUE;
}

Bool
RRTransformSetFilter(RRTransformPtr dst,
                     PictFilterPtr filter,
                     xFixed * params, int nparams, int width, int height)
{
    xFixed *new_params;

    if (nparams) {
        new_params = xallocarray(nparams, sizeof(xFixed));
        if (!new_params)
            return FALSE;
        memcpy(new_params, params, nparams * sizeof(xFixed));
    }
    else
        new_params = NULL;
    free(dst->params);
    dst->filter = filter;
    dst->params = new_params;
    dst->nparams = nparams;
    dst->width = width;
    dst->height = height;
    return TRUE;
}

Bool
RRTransformCopy(RRTransformPtr dst, RRTransformPtr src)
{
    if (src && pixman_transform_is_identity(&src->transform))
        src = NULL;

    if (src) {
        if (!RRTransformSetFilter(dst, src->filter,
                                  src->params, src->nparams, src->width,
                                  src->height))
            return FALSE;
        dst->transform = src->transform;
        dst->f_transform = src->f_transform;
        dst->f_inverse = src->f_inverse;
    }
    else {
        if (!RRTransformSetFilter(dst, NULL, NULL, 0, 0, 0))
            return FALSE;
        pixman_transform_init_identity(&dst->transform);
        pixman_f_transform_init_identity(&dst->f_transform);
        pixman_f_transform_init_identity(&dst->f_inverse);
    }
    return TRUE;
}

#define F(x)	IntToxFixed(x)

static void
RRTransformRescale(struct pixman_f_transform *f_transform, double limit)
{
    double max = 0, v, scale;
    int i, j;

    for (j = 0; j < 3; j++)
        for (i = 0; i < 3; i++)
            if ((v = fabs(f_transform->m[j][i])) > max)
                max = v;
    scale = limit / max;
    for (j = 0; j < 3; j++)
        for (i = 0; i < 3; i++)
            f_transform->m[j][i] *= scale;
}

/*
 * Compute the complete transformation matrix including
 * client-specified transform, rotation/reflection values and the crtc
 * offset.
 *
 * Return TRUE if the resulting transform is not a simple translation.
 */
Bool
RRTransformCompute(int x,
                   int y,
                   int width,
                   int height,
                   Rotation rotation,
                   RRTransformPtr rr_transform,
                   PictTransformPtr transform,
                   struct pixman_f_transform *f_transform,
                   struct pixman_f_transform *f_inverse)
{
    PictTransform t_transform, inverse;
    struct pixman_f_transform tf_transform, tf_inverse;
    Bool overflow = FALSE;

    if (!transform)
        transform = &t_transform;
    if (!f_transform)
        f_transform = &tf_transform;
    if (!f_inverse)
        f_inverse = &tf_inverse;

    pixman_transform_init_identity(transform);
    pixman_transform_init_identity(&inverse);
    pixman_f_transform_init_identity(f_transform);
    pixman_f_transform_init_identity(f_inverse);
    if (rotation != RR_Rotate_0) {
        double f_rot_cos, f_rot_sin, f_rot_dx, f_rot_dy;
        double f_scale_x, f_scale_y, f_scale_dx, f_scale_dy;
        xFixed rot_cos, rot_sin, rot_dx, rot_dy;
        xFixed scale_x, scale_y, scale_dx, scale_dy;

        /* rotation */
        switch (rotation & 0xf) {
        default:
        case RR_Rotate_0:
            f_rot_cos = 1;
            f_rot_sin = 0;
            f_rot_dx = 0;
            f_rot_dy = 0;
            rot_cos = F(1);
            rot_sin = F(0);
            rot_dx = F(0);
            rot_dy = F(0);
            break;
        case RR_Rotate_90:
            f_rot_cos = 0;
            f_rot_sin = 1;
            f_rot_dx = height;
            f_rot_dy = 0;
            rot_cos = F(0);
            rot_sin = F(1);
            rot_dx = F(height);
            rot_dy = F(0);
            break;
        case RR_Rotate_180:
            f_rot_cos = -1;
            f_rot_sin = 0;
            f_rot_dx = width;
            f_rot_dy = height;
            rot_cos = F(~0u);
            rot_sin = F(0);
            rot_dx = F(width);
            rot_dy = F(height);
            break;
        case RR_Rotate_270:
            f_rot_cos = 0;
            f_rot_sin = -1;
            f_rot_dx = 0;
            f_rot_dy = width;
            rot_cos = F(0);
            rot_sin = F(~0u);
            rot_dx = F(0);
            rot_dy = F(width);
            break;
        }

        pixman_transform_rotate(transform, &inverse, rot_cos, rot_sin);
        pixman_transform_translate(transform, &inverse, rot_dx, rot_dy);
        pixman_f_transform_rotate(f_transform, f_inverse, f_rot_cos, f_rot_sin);
        pixman_f_transform_translate(f_transform, f_inverse, f_rot_dx,
                                     f_rot_dy);

        /* reflection */
        f_scale_x = 1;
        f_scale_dx = 0;
        f_scale_y = 1;
        f_scale_dy = 0;
        scale_x = F(1);
        scale_dx = 0;
        scale_y = F(1);
        scale_dy = 0;
        if (rotation & RR_Reflect_X) {
            f_scale_x = -1;
            scale_x = F(~0u);
            if (rotation & (RR_Rotate_0 | RR_Rotate_180)) {
                f_scale_dx = width;
                scale_dx = F(width);
            }
            else {
                f_scale_dx = height;
                scale_dx = F(height);
            }
        }
        if (rotation & RR_Reflect_Y) {
            f_scale_y = -1;
            scale_y = F(~0u);
            if (rotation & (RR_Rotate_0 | RR_Rotate_180)) {
                f_scale_dy = height;
                scale_dy = F(height);
            }
            else {
                f_scale_dy = width;
                scale_dy = F(width);
            }
        }

        pixman_transform_scale(transform, &inverse, scale_x, scale_y);
        pixman_f_transform_scale(f_transform, f_inverse, f_scale_x, f_scale_y);
        pixman_transform_translate(transform, &inverse, scale_dx, scale_dy);
        pixman_f_transform_translate(f_transform, f_inverse, f_scale_dx,
                                     f_scale_dy);
    }

#ifdef RANDR_12_INTERFACE
    if (rr_transform) {
        if (!pixman_transform_multiply
            (transform, &rr_transform->transform, transform))
            overflow = TRUE;
        pixman_f_transform_multiply(f_transform, &rr_transform->f_transform,
                                    f_transform);
        pixman_f_transform_multiply(f_inverse, f_inverse,
                                    &rr_transform->f_inverse);
    }
#endif
    /*
     * Compute the class of the resulting transform
     */
    if (!overflow && pixman_transform_is_identity(transform)) {
        pixman_transform_init_translate(transform, F(x), F(y));

        pixman_f_transform_init_translate(f_transform, x, y);
        pixman_f_transform_init_translate(f_inverse, -x, -y);
        return FALSE;
    }
    else {
        pixman_f_transform_translate(f_transform, f_inverse, x, y);
        if (!pixman_transform_translate(transform, &inverse, F(x), F(y)))
            overflow = TRUE;
        if (overflow) {
            struct pixman_f_transform f_scaled;

            f_scaled = *f_transform;
            RRTransformRescale(&f_scaled, 16384.0);
            pixman_transform_from_pixman_f_transform(transform, &f_scaled);
        }
        return TRUE;
    }
}
