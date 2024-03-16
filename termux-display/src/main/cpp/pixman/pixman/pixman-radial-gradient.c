/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
/*
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 * Copyright © 2000 SuSE, Inc.
 *             2005 Lars Knoll & Zack Rusin, Trolltech
 * Copyright © 2007 Red Hat, Inc.
 *
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

static inline pixman_fixed_32_32_t
dot (pixman_fixed_48_16_t x1,
     pixman_fixed_48_16_t y1,
     pixman_fixed_48_16_t z1,
     pixman_fixed_48_16_t x2,
     pixman_fixed_48_16_t y2,
     pixman_fixed_48_16_t z2)
{
    /*
     * Exact computation, assuming that the input values can
     * be represented as pixman_fixed_16_16_t
     */
    return x1 * x2 + y1 * y2 + z1 * z2;
}

static inline double
fdot (double x1,
      double y1,
      double z1,
      double x2,
      double y2,
      double z2)
{
    /*
     * Error can be unbound in some special cases.
     * Using clever dot product algorithms (for example compensated
     * dot product) would improve this but make the code much less
     * obvious
     */
    return x1 * x2 + y1 * y2 + z1 * z2;
}

static void
radial_write_color (double                         a,
		    double                         b,
		    double                         c,
		    double                         inva,
		    double                         dr,
		    double                         mindr,
		    pixman_gradient_walker_t      *walker,
		    pixman_repeat_t                repeat,
		    int                            Bpp,
		    pixman_gradient_walker_write_t write_pixel,
		    uint32_t                      *buffer)
{
    /*
     * In this function error propagation can lead to bad results:
     *  - discr can have an unbound error (if b*b-a*c is very small),
     *    potentially making it the opposite sign of what it should have been
     *    (thus clearing a pixel that would have been colored or vice-versa)
     *    or propagating the error to sqrtdiscr;
     *    if discr has the wrong sign or b is very small, this can lead to bad
     *    results
     *
     *  - the algorithm used to compute the solutions of the quadratic
     *    equation is not numerically stable (but saves one division compared
     *    to the numerically stable one);
     *    this can be a problem if a*c is much smaller than b*b
     *
     *  - the above problems are worse if a is small (as inva becomes bigger)
     */
    double discr;

    if (a == 0)
    {
	double t;

	if (b == 0)
	{
	    memset (buffer, 0, Bpp);
	    return;
	}

	t = pixman_fixed_1 / 2 * c / b;
	if (repeat == PIXMAN_REPEAT_NONE)
	{
	    if (0 <= t && t <= pixman_fixed_1)
	    {
		write_pixel (walker, t, buffer);
		return;
	    }
	}
	else
	{
	    if (t * dr >= mindr)
	    {
		write_pixel (walker, t, buffer);
		return;
	    }
	}

	memset (buffer, 0, Bpp);
	return;
    }

    discr = fdot (b, a, 0, b, -c, 0);
    if (discr >= 0)
    {
	double sqrtdiscr, t0, t1;

	sqrtdiscr = sqrt (discr);
	t0 = (b + sqrtdiscr) * inva;
	t1 = (b - sqrtdiscr) * inva;

	/*
	 * The root that must be used is the biggest one that belongs
	 * to the valid range ([0,1] for PIXMAN_REPEAT_NONE, any
	 * solution that results in a positive radius otherwise).
	 *
	 * If a > 0, t0 is the biggest solution, so if it is valid, it
	 * is the correct result.
	 *
	 * If a < 0, only one of the solutions can be valid, so the
	 * order in which they are tested is not important.
	 */
	if (repeat == PIXMAN_REPEAT_NONE)
	{
	    if (0 <= t0 && t0 <= pixman_fixed_1)
	    {
		write_pixel (walker, t0, buffer);
		return;
	    }
	    else if (0 <= t1 && t1 <= pixman_fixed_1)
	    {
		write_pixel (walker, t1, buffer);
		return;
           }
	}
	else
	{
	    if (t0 * dr >= mindr)
	    {
		write_pixel (walker, t0, buffer);
		return;
	    }
	    else if (t1 * dr >= mindr)
	    {
		write_pixel (walker, t1, buffer);
		return;
	    }
	}
    }

    memset (buffer, 0, Bpp);
    return;
}

static uint32_t *
radial_get_scanline (pixman_iter_t                 *iter,
		     const uint32_t                *mask,
		     int                            Bpp,
		     pixman_gradient_walker_write_t write_pixel)
{
    /*
     * Implementation of radial gradients following the PDF specification.
     * See section 8.7.4.5.4 Type 3 (Radial) Shadings of the PDF Reference
     * Manual (PDF 32000-1:2008 at the time of this writing).
     *
     * In the radial gradient problem we are given two circles (c₁,r₁) and
     * (c₂,r₂) that define the gradient itself.
     *
     * Mathematically the gradient can be defined as the family of circles
     *
     *     ((1-t)·c₁ + t·(c₂), (1-t)·r₁ + t·r₂)
     *
     * excluding those circles whose radius would be < 0. When a point
     * belongs to more than one circle, the one with a bigger t is the only
     * one that contributes to its color. When a point does not belong
     * to any of the circles, it is transparent black, i.e. RGBA (0, 0, 0, 0).
     * Further limitations on the range of values for t are imposed when
     * the gradient is not repeated, namely t must belong to [0,1].
     *
     * The graphical result is the same as drawing the valid (radius > 0)
     * circles with increasing t in [-inf, +inf] (or in [0,1] if the gradient
     * is not repeated) using SOURCE operator composition.
     *
     * It looks like a cone pointing towards the viewer if the ending circle
     * is smaller than the starting one, a cone pointing inside the page if
     * the starting circle is the smaller one and like a cylinder if they
     * have the same radius.
     *
     * What we actually do is, given the point whose color we are interested
     * in, compute the t values for that point, solving for t in:
     *
     *     length((1-t)·c₁ + t·(c₂) - p) = (1-t)·r₁ + t·r₂
     *
     * Let's rewrite it in a simpler way, by defining some auxiliary
     * variables:
     *
     *     cd = c₂ - c₁
     *     pd = p - c₁
     *     dr = r₂ - r₁
     *     length(t·cd - pd) = r₁ + t·dr
     *
     * which actually means
     *
     *     hypot(t·cdx - pdx, t·cdy - pdy) = r₁ + t·dr
     *
     * or
     *
     *     ⎷((t·cdx - pdx)² + (t·cdy - pdy)²) = r₁ + t·dr.
     *
     * If we impose (as stated earlier) that r₁ + t·dr >= 0, it becomes:
     *
     *     (t·cdx - pdx)² + (t·cdy - pdy)² = (r₁ + t·dr)²
     *
     * where we can actually expand the squares and solve for t:
     *
     *     t²cdx² - 2t·cdx·pdx + pdx² + t²cdy² - 2t·cdy·pdy + pdy² =
     *       = r₁² + 2·r₁·t·dr + t²·dr²
     *
     *     (cdx² + cdy² - dr²)t² - 2(cdx·pdx + cdy·pdy + r₁·dr)t +
     *         (pdx² + pdy² - r₁²) = 0
     *
     *     A = cdx² + cdy² - dr²
     *     B = pdx·cdx + pdy·cdy + r₁·dr
     *     C = pdx² + pdy² - r₁²
     *     At² - 2Bt + C = 0
     *
     * The solutions (unless the equation degenerates because of A = 0) are:
     *
     *     t = (B ± ⎷(B² - A·C)) / A
     *
     * The solution we are going to prefer is the bigger one, unless the
     * radius associated to it is negative (or it falls outside the valid t
     * range).
     *
     * Additional observations (useful for optimizations):
     * A does not depend on p
     *
     * A < 0 <=> one of the two circles completely contains the other one
     *   <=> for every p, the radiuses associated with the two t solutions
     *       have opposite sign
     */
    pixman_image_t *image = iter->image;
    int x = iter->x;
    int y = iter->y;
    int width = iter->width;
    uint32_t *buffer = iter->buffer;

    gradient_t *gradient = (gradient_t *)image;
    radial_gradient_t *radial = (radial_gradient_t *)image;
    uint32_t *end = buffer + width * (Bpp / 4);
    pixman_gradient_walker_t walker;
    pixman_vector_t v, unit;

    /* reference point is the center of the pixel */
    v.vector[0] = pixman_int_to_fixed (x) + pixman_fixed_1 / 2;
    v.vector[1] = pixman_int_to_fixed (y) + pixman_fixed_1 / 2;
    v.vector[2] = pixman_fixed_1;

    _pixman_gradient_walker_init (&walker, gradient, image->common.repeat);

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

    if (unit.vector[2] == 0 && v.vector[2] == pixman_fixed_1)
    {
	/*
	 * Given:
	 *
	 * t = (B ± ⎷(B² - A·C)) / A
	 *
	 * where
	 *
	 * A = cdx² + cdy² - dr²
	 * B = pdx·cdx + pdy·cdy + r₁·dr
	 * C = pdx² + pdy² - r₁²
	 * det = B² - A·C
	 *
	 * Since we have an affine transformation, we know that (pdx, pdy)
	 * increase linearly with each pixel,
	 *
	 * pdx = pdx₀ + n·ux,
	 * pdy = pdy₀ + n·uy,
	 *
	 * we can then express B, C and det through multiple differentiation.
	 */
	pixman_fixed_32_32_t b, db, c, dc, ddc;

	/* warning: this computation may overflow */
	v.vector[0] -= radial->c1.x;
	v.vector[1] -= radial->c1.y;

	/*
	 * B and C are computed and updated exactly.
	 * If fdot was used instead of dot, in the worst case it would
	 * lose 11 bits of precision in each of the multiplication and
	 * summing up would zero out all the bit that were preserved,
	 * thus making the result 0 instead of the correct one.
	 * This would mean a worst case of unbound relative error or
	 * about 2^10 absolute error
	 */
	b = dot (v.vector[0], v.vector[1], radial->c1.radius,
		 radial->delta.x, radial->delta.y, radial->delta.radius);
	db = dot (unit.vector[0], unit.vector[1], 0,
		  radial->delta.x, radial->delta.y, 0);

	c = dot (v.vector[0], v.vector[1],
		 -((pixman_fixed_48_16_t) radial->c1.radius),
		 v.vector[0], v.vector[1], radial->c1.radius);
	dc = dot (2 * (pixman_fixed_48_16_t) v.vector[0] + unit.vector[0],
		  2 * (pixman_fixed_48_16_t) v.vector[1] + unit.vector[1],
		  0,
		  unit.vector[0], unit.vector[1], 0);
	ddc = 2 * dot (unit.vector[0], unit.vector[1], 0,
		       unit.vector[0], unit.vector[1], 0);

	while (buffer < end)
	{
	    if (!mask || *mask++)
	    {
		radial_write_color (radial->a, b, c,
				    radial->inva,
				    radial->delta.radius,
				    radial->mindr,
				    &walker,
				    image->common.repeat,
				    Bpp,
				    write_pixel,
				    buffer);
	    }

	    b += db;
	    c += dc;
	    dc += ddc;
	    buffer += (Bpp / 4);
	}
    }
    else
    {
	/* projective */
	/* Warning:
	 * error propagation guarantees are much looser than in the affine case
	 */
	while (buffer < end)
	{
	    if (!mask || *mask++)
	    {
		if (v.vector[2] != 0)
		{
		    double pdx, pdy, invv2, b, c;

		    invv2 = 1. * pixman_fixed_1 / v.vector[2];

		    pdx = v.vector[0] * invv2 - radial->c1.x;
		    /*    / pixman_fixed_1 */

		    pdy = v.vector[1] * invv2 - radial->c1.y;
		    /*    / pixman_fixed_1 */

		    b = fdot (pdx, pdy, radial->c1.radius,
			      radial->delta.x, radial->delta.y,
			      radial->delta.radius);
		    /*  / pixman_fixed_1 / pixman_fixed_1 */

		    c = fdot (pdx, pdy, -radial->c1.radius,
			      pdx, pdy, radial->c1.radius);
		    /*  / pixman_fixed_1 / pixman_fixed_1 */

		    radial_write_color (radial->a, b, c,
					radial->inva,
					radial->delta.radius,
					radial->mindr,
					&walker,
					image->common.repeat,
					Bpp,
					write_pixel,
					buffer);
		}
		else
		{
		    memset (buffer, 0, Bpp);
		}
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
radial_get_scanline_narrow (pixman_iter_t *iter, const uint32_t *mask)
{
    return radial_get_scanline (iter, mask, 4,
				_pixman_gradient_walker_write_narrow);
}

static uint32_t *
radial_get_scanline_wide (pixman_iter_t *iter, const uint32_t *mask)
{
    return radial_get_scanline (iter, NULL, 16,
				_pixman_gradient_walker_write_wide);
}

void
_pixman_radial_gradient_iter_init (pixman_image_t *image, pixman_iter_t *iter)
{
    if (iter->iter_flags & ITER_NARROW)
	iter->get_scanline = radial_get_scanline_narrow;
    else
	iter->get_scanline = radial_get_scanline_wide;
}

PIXMAN_EXPORT pixman_image_t *
pixman_image_create_radial_gradient (const pixman_point_fixed_t *  inner,
				     const pixman_point_fixed_t *  outer,
				     pixman_fixed_t                inner_radius,
				     pixman_fixed_t                outer_radius,
				     const pixman_gradient_stop_t *stops,
				     int                           n_stops)
{
    pixman_image_t *image;
    radial_gradient_t *radial;

    image = _pixman_image_allocate ();

    if (!image)
	return NULL;

    radial = &image->radial;

    if (!_pixman_init_gradient (&radial->common, stops, n_stops))
    {
	free (image);
	return NULL;
    }

    image->type = RADIAL;

    radial->c1.x = inner->x;
    radial->c1.y = inner->y;
    radial->c1.radius = inner_radius;
    radial->c2.x = outer->x;
    radial->c2.y = outer->y;
    radial->c2.radius = outer_radius;

    /* warning: this computations may overflow */
    radial->delta.x = radial->c2.x - radial->c1.x;
    radial->delta.y = radial->c2.y - radial->c1.y;
    radial->delta.radius = radial->c2.radius - radial->c1.radius;

    /* computed exactly, then cast to double -> every bit of the double
       representation is correct (53 bits) */
    radial->a = dot (radial->delta.x, radial->delta.y, -radial->delta.radius,
		     radial->delta.x, radial->delta.y, radial->delta.radius);
    if (radial->a != 0)
	radial->inva = 1. * pixman_fixed_1 / radial->a;

    radial->mindr = -1. * pixman_fixed_1 * radial->c1.radius;

    return image;
}
