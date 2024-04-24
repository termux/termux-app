/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
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
#include <pixman-config.h>
#endif
#include <stdlib.h>
#include "pixman-private.h"

static pixman_bool_t
linear_gradient_is_horizontal (pixman_image_t *image,
			       int             x,
			       int             y,
			       int             width,
			       int             height)
{
    linear_gradient_t *linear = (linear_gradient_t *)image;
    pixman_vector_t v;
    pixman_fixed_32_32_t l;
    pixman_fixed_48_16_t dx, dy;
    double inc;

    if (image->common.transform)
    {
	/* projective transformation */
	if (image->common.transform->matrix[2][0] != 0 ||
	    image->common.transform->matrix[2][1] != 0 ||
	    image->common.transform->matrix[2][2] == 0)
	{
	    return FALSE;
	}

	v.vector[0] = image->common.transform->matrix[0][1];
	v.vector[1] = image->common.transform->matrix[1][1];
	v.vector[2] = image->common.transform->matrix[2][2];
    }
    else
    {
	v.vector[0] = 0;
	v.vector[1] = pixman_fixed_1;
	v.vector[2] = pixman_fixed_1;
    }

    dx = linear->p2.x - linear->p1.x;
    dy = linear->p2.y - linear->p1.y;

    l = dx * dx + dy * dy;

    if (l == 0)
	return FALSE;

    /*
     * compute how much the input of the gradient walked changes
     * when moving vertically through the whole image
     */
    inc = height * (double) pixman_fixed_1 * pixman_fixed_1 *
	(dx * v.vector[0] + dy * v.vector[1]) /
	(v.vector[2] * (double) l);

    /* check that casting to integer would result in 0 */
    if (-1 < inc && inc < 1)
	return TRUE;

    return FALSE;
}

static uint32_t *
linear_get_scanline (pixman_iter_t                 *iter,
		     const uint32_t                *mask,
		     int                            Bpp,
		     pixman_gradient_walker_write_t write_pixel,
		     pixman_gradient_walker_fill_t  fill_pixel)
{
    pixman_image_t *image  = iter->image;
    int             x      = iter->x;
    int             y      = iter->y;
    int             width  = iter->width;
    uint32_t *      buffer = iter->buffer;

    pixman_vector_t v, unit;
    pixman_fixed_32_32_t l;
    pixman_fixed_48_16_t dx, dy;
    gradient_t *gradient = (gradient_t *)image;
    linear_gradient_t *linear = (linear_gradient_t *)image;
    uint32_t *end = buffer + width * (Bpp / 4);
    pixman_gradient_walker_t walker;

    _pixman_gradient_walker_init (&walker, gradient, image->common.repeat);

    /* reference point is the center of the pixel */
    v.vector[0] = pixman_int_to_fixed (x) + pixman_fixed_1 / 2;
    v.vector[1] = pixman_int_to_fixed (y) + pixman_fixed_1 / 2;
    v.vector[2] = pixman_fixed_1;

    if (image->common.transform)
    {
	if (!pixman_transform_point_3d (image->common.transform, &v))
	    return iter->buffer;

	unit.vector[0] = image->common.transform->matrix[0][0];
	unit.vector[1] = image->common.transform->matrix[1][0];
	unit.vector[2] = image->common.transform->matrix[2][0];
    }
    else
    {
	unit.vector[0] = pixman_fixed_1;
	unit.vector[1] = 0;
	unit.vector[2] = 0;
    }

    dx = linear->p2.x - linear->p1.x;
    dy = linear->p2.y - linear->p1.y;

    l = dx * dx + dy * dy;

    if (l == 0 || unit.vector[2] == 0)
    {
	/* affine transformation only */
	pixman_fixed_32_32_t t, next_inc;
	double inc;

	if (l == 0 || v.vector[2] == 0)
	{
	    t = 0;
	    inc = 0;
	}
	else
	{
	    double invden, v2;

	    invden = pixman_fixed_1 * (double) pixman_fixed_1 /
		(l * (double) v.vector[2]);
	    v2 = v.vector[2] * (1. / pixman_fixed_1);
	    t = ((dx * v.vector[0] + dy * v.vector[1]) -
		 (dx * linear->p1.x + dy * linear->p1.y) * v2) * invden;
	    inc = (dx * unit.vector[0] + dy * unit.vector[1]) * invden;
	}
	next_inc = 0;

	if (((pixman_fixed_32_32_t )(inc * width)) == 0)
	{
	    fill_pixel (&walker, t, buffer, end);
	}
	else
	{
	    int i;

	    i = 0;
	    while (buffer < end)
	    {
		if (!mask || *mask++)
		{
		    write_pixel (&walker, t + next_inc, buffer);
		}
		i++;
		next_inc = inc * i;
		buffer += (Bpp / 4);
	    }
	}
    }
    else
    {
	/* projective transformation */
        double t;

	t = 0;

	while (buffer < end)
	{
	    if (!mask || *mask++)
	    {
	        if (v.vector[2] != 0)
		{
		    double invden, v2;

		    invden = pixman_fixed_1 * (double) pixman_fixed_1 /
			(l * (double) v.vector[2]);
		    v2 = v.vector[2] * (1. / pixman_fixed_1);
		    t = ((dx * v.vector[0] + dy * v.vector[1]) -
			 (dx * linear->p1.x + dy * linear->p1.y) * v2) * invden;
		}

		write_pixel (&walker, t, buffer);
	    }

	    buffer += (Bpp / 4);

	    v.vector[0] += unit.vector[0];
	    v.vector[1] += unit.vector[1];
	    v.vector[2] += unit.vector[2];
	}
    }

    iter->y++;

    return iter->buffer;
}

static uint32_t *
linear_get_scanline_narrow (pixman_iter_t  *iter,
			    const uint32_t *mask)
{
    return linear_get_scanline (iter, mask, 4,
				_pixman_gradient_walker_write_narrow,
				_pixman_gradient_walker_fill_narrow);
}


static uint32_t *
linear_get_scanline_wide (pixman_iter_t *iter, const uint32_t *mask)
{
    return linear_get_scanline (iter, NULL, 16,
				_pixman_gradient_walker_write_wide,
				_pixman_gradient_walker_fill_wide);
}

void
_pixman_linear_gradient_iter_init (pixman_image_t *image, pixman_iter_t  *iter)
{
    if (linear_gradient_is_horizontal (
	    iter->image, iter->x, iter->y, iter->width, iter->height))
    {
	if (iter->iter_flags & ITER_NARROW)
	    linear_get_scanline_narrow (iter, NULL);
	else
	    linear_get_scanline_wide (iter, NULL);

	iter->get_scanline = _pixman_iter_get_scanline_noop;
    }
    else
    {
	if (iter->iter_flags & ITER_NARROW)
	    iter->get_scanline = linear_get_scanline_narrow;
	else
	    iter->get_scanline = linear_get_scanline_wide;
    }
}

PIXMAN_EXPORT pixman_image_t *
pixman_image_create_linear_gradient (const pixman_point_fixed_t *  p1,
                                     const pixman_point_fixed_t *  p2,
                                     const pixman_gradient_stop_t *stops,
                                     int                           n_stops)
{
    pixman_image_t *image;
    linear_gradient_t *linear;

    image = _pixman_image_allocate ();

    if (!image)
	return NULL;

    linear = &image->linear;

    if (!_pixman_init_gradient (&linear->common, stops, n_stops))
    {
	free (image);
	return NULL;
    }

    linear->p1 = *p1;
    linear->p2 = *p2;

    image->type = LINEAR;

    return image;
}

