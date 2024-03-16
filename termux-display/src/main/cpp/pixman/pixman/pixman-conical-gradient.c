/*
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007 Red Hat, Inc.
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *             2005 Lars Knoll & Zack Rusin, Trolltech
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
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <math.h>
#include "pixman-private.h"

static force_inline double
coordinates_to_parameter (double x, double y, double angle)
{
    double t;

    t = atan2 (y, x) + angle;

    while (t < 0)
	t += 2 * M_PI;

    while (t >= 2 * M_PI)
	t -= 2 * M_PI;

    return 1 - t * (1 / (2 * M_PI)); /* Scale t to [0, 1] and
				      * make rotation CCW
				      */
}

static uint32_t *
conical_get_scanline (pixman_iter_t                 *iter,
		      const uint32_t                *mask,
		      int                            Bpp,
		      pixman_gradient_walker_write_t write_pixel)
{
    pixman_image_t *image = iter->image;
    int x = iter->x;
    int y = iter->y;
    int width = iter->width;
    uint32_t *buffer = iter->buffer;

    gradient_t *gradient = (gradient_t *)image;
    conical_gradient_t *conical = (conical_gradient_t *)image;
    uint32_t       *end = buffer + width * (Bpp / 4);
    pixman_gradient_walker_t walker;
    pixman_bool_t affine = TRUE;
    double cx = 1.;
    double cy = 0.;
    double cz = 0.;
    double rx = x + 0.5;
    double ry = y + 0.5;
    double rz = 1.;

    _pixman_gradient_walker_init (&walker, gradient, image->common.repeat);

    if (image->common.transform)
    {
	pixman_vector_t v;

	/* reference point is the center of the pixel */
	v.vector[0] = pixman_int_to_fixed (x) + pixman_fixed_1 / 2;
	v.vector[1] = pixman_int_to_fixed (y) + pixman_fixed_1 / 2;
	v.vector[2] = pixman_fixed_1;

	if (!pixman_transform_point_3d (image->common.transform, &v))
	    return iter->buffer;

	cx = image->common.transform->matrix[0][0] / 65536.;
	cy = image->common.transform->matrix[1][0] / 65536.;
	cz = image->common.transform->matrix[2][0] / 65536.;

	rx = v.vector[0] / 65536.;
	ry = v.vector[1] / 65536.;
	rz = v.vector[2] / 65536.;

	affine =
	    image->common.transform->matrix[2][0] == 0 &&
	    v.vector[2] == pixman_fixed_1;
    }

    if (affine)
    {
	rx -= conical->center.x / 65536.;
	ry -= conical->center.y / 65536.;

	while (buffer < end)
	{
	    if (!mask || *mask++)
	    {
		double t = coordinates_to_parameter (rx, ry, conical->angle);

		write_pixel (&walker,
			     (pixman_fixed_48_16_t)pixman_double_to_fixed (t),
			     buffer);
	    }

	    buffer += (Bpp / 4);

	    rx += cx;
	    ry += cy;
	}
    }
    else
    {
	while (buffer < end)
	{
	    double x, y;

	    if (!mask || *mask++)
	    {
		double t;

		if (rz != 0)
		{
		    x = rx / rz;
		    y = ry / rz;
		}
		else
		{
		    x = y = 0.;
		}

		x -= conical->center.x / 65536.;
		y -= conical->center.y / 65536.;

		t = coordinates_to_parameter (x, y, conical->angle);

		write_pixel (&walker,
			     (pixman_fixed_48_16_t)pixman_double_to_fixed (t),
			     buffer);
	    }

	    buffer += (Bpp / 4);

	    rx += cx;
	    ry += cy;
	    rz += cz;
	}
    }

    iter->y++;
    return iter->buffer;
}

static uint32_t *
conical_get_scanline_narrow (pixman_iter_t *iter, const uint32_t *mask)
{
    return conical_get_scanline (iter, mask, 4,
				 _pixman_gradient_walker_write_narrow);
}

static uint32_t *
conical_get_scanline_wide (pixman_iter_t *iter, const uint32_t *mask)
{
    return conical_get_scanline (iter, NULL, 16,
				 _pixman_gradient_walker_write_wide);
}

void
_pixman_conical_gradient_iter_init (pixman_image_t *image, pixman_iter_t *iter)
{
    if (iter->iter_flags & ITER_NARROW)
	iter->get_scanline = conical_get_scanline_narrow;
    else
	iter->get_scanline = conical_get_scanline_wide;
}

PIXMAN_EXPORT pixman_image_t *
pixman_image_create_conical_gradient (const pixman_point_fixed_t *  center,
                                      pixman_fixed_t                angle,
                                      const pixman_gradient_stop_t *stops,
                                      int                           n_stops)
{
    pixman_image_t *image = _pixman_image_allocate ();
    conical_gradient_t *conical;

    if (!image)
	return NULL;

    conical = &image->conical;

    if (!_pixman_init_gradient (&conical->common, stops, n_stops))
    {
	free (image);
	return NULL;
    }

    angle = MOD (angle, pixman_int_to_fixed (360));

    image->type = CONICAL;

    conical->center = *center;
    conical->angle = (pixman_fixed_to_double (angle) / 180.0) * M_PI;

    return image;
}

