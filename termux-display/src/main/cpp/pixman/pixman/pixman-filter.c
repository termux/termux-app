/*
 * Copyright 2012, Red Hat, Inc.
 * Copyright 2012, Soren Sandmann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Soren Sandmann <soren.sandmann@gmail.com>
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "pixman-private.h"

typedef double (* kernel_func_t) (double x);

typedef struct
{
    pixman_kernel_t	kernel;
    kernel_func_t	func;
    double		width;
} filter_info_t;

static double
impulse_kernel (double x)
{
    return (x == 0.0)? 1.0 : 0.0;
}

static double
box_kernel (double x)
{
    return 1;
}

static double
linear_kernel (double x)
{
    return 1 - fabs (x);
}

static double
gaussian_kernel (double x)
{
#define SQRT2 (1.4142135623730950488016887242096980785696718753769480)
#define SIGMA (SQRT2 / 2.0)
    
    return exp (- x * x / (2 * SIGMA * SIGMA)) / (SIGMA * sqrt (2.0 * M_PI));
}

static double
sinc (double x)
{
    if (x == 0.0)
	return 1.0;
    else
	return sin (M_PI * x) / (M_PI * x);
}

static double
lanczos (double x, int n)
{
    return sinc (x) * sinc (x * (1.0 / n));
}

static double
lanczos2_kernel (double x)
{
    return lanczos (x, 2);
}

static double
lanczos3_kernel (double x)
{
    return lanczos (x, 3);
}

static double
nice_kernel (double x)
{
    return lanczos3_kernel (x * 0.75);
}

static double
general_cubic (double x, double B, double C)
{
    double ax = fabs(x);

    if (ax < 1)
    {
	return (((12 - 9 * B - 6 * C) * ax +
		 (-18 + 12 * B + 6 * C)) * ax * ax +
		(6 - 2 * B)) / 6;
    }
    else if (ax < 2)
    {
	return ((((-B - 6 * C) * ax +
		  (6 * B + 30 * C)) * ax +
		 (-12 * B - 48 * C)) * ax +
		(8 * B + 24 * C)) / 6;
    }
    else
    {
	return 0;
    }
}

static double
cubic_kernel (double x)
{
    /* This is the Mitchell-Netravali filter.
     *
     * (0.0, 0.5) would give us the Catmull-Rom spline,
     * but that one seems to be indistinguishable from Lanczos2.
     */
    return general_cubic (x, 1/3.0, 1/3.0);
}

static const filter_info_t filters[] =
{
    { PIXMAN_KERNEL_IMPULSE,	        impulse_kernel,   0.0 },
    { PIXMAN_KERNEL_BOX,	        box_kernel,       1.0 },
    { PIXMAN_KERNEL_LINEAR,	        linear_kernel,    2.0 },
    { PIXMAN_KERNEL_CUBIC,		cubic_kernel,     4.0 },
    { PIXMAN_KERNEL_GAUSSIAN,	        gaussian_kernel,  5.0 },
    { PIXMAN_KERNEL_LANCZOS2,	        lanczos2_kernel,  4.0 },
    { PIXMAN_KERNEL_LANCZOS3,	        lanczos3_kernel,  6.0 },
    { PIXMAN_KERNEL_LANCZOS3_STRETCHED, nice_kernel,      8.0 },
};

/* This function scales @kernel2 by @scale, then
 * aligns @x1 in @kernel1 with @x2 in @kernel2 and
 * and integrates the product of the kernels across @width.
 *
 * This function assumes that the intervals are within
 * the kernels in question. E.g., the caller must not
 * try to integrate a linear kernel ouside of [-1:1]
 */
static double
integral (pixman_kernel_t kernel1, double x1,
	  pixman_kernel_t kernel2, double scale, double x2,
	  double width)
{
    if (kernel1 == PIXMAN_KERNEL_BOX && kernel2 == PIXMAN_KERNEL_BOX)
    {
	return width;
    }
    /* The LINEAR filter is not differentiable at 0, so if the
     * integration interval crosses zero, break it into two
     * separate integrals.
     */
    else if (kernel1 == PIXMAN_KERNEL_LINEAR && x1 < 0 && x1 + width > 0)
    {
	return
	    integral (kernel1, x1, kernel2, scale, x2, - x1) +
	    integral (kernel1, 0, kernel2, scale, x2 - x1, width + x1);
    }
    else if (kernel2 == PIXMAN_KERNEL_LINEAR && x2 < 0 && x2 + width > 0)
    {
	return
	    integral (kernel1, x1, kernel2, scale, x2, - x2) +
	    integral (kernel1, x1 - x2, kernel2, scale, 0, width + x2);
    }
    else if (kernel1 == PIXMAN_KERNEL_IMPULSE)
    {
	assert (width == 0.0);
	return filters[kernel2].func (x2 * scale);
    }
    else if (kernel2 == PIXMAN_KERNEL_IMPULSE)
    {
	assert (width == 0.0);
	return filters[kernel1].func (x1);
    }
    else
    {
	/* Integration via Simpson's rule
	 * See http://www.intmath.com/integration/6-simpsons-rule.php
	 * 12 segments (6 cubic approximations) seems to produce best
	 * result for lanczos3.linear, which was the combination that
	 * showed the most errors.  This makes sense as the lanczos3
	 * filter is 6 wide.
	 */
#define N_SEGMENTS 12
#define SAMPLE(a1, a2)							\
	(filters[kernel1].func ((a1)) * filters[kernel2].func ((a2) * scale))
	
	double s = 0.0;
	double h = width / N_SEGMENTS;
	int i;

	s = SAMPLE (x1, x2);

	for (i = 1; i < N_SEGMENTS; i += 2)
	{
	    double a1 = x1 + h * i;
	    double a2 = x2 + h * i;
	    s += 4 * SAMPLE (a1, a2);
	}

	for (i = 2; i < N_SEGMENTS; i += 2)
	{
	    double a1 = x1 + h * i;
	    double a2 = x2 + h * i;
	    s += 2 * SAMPLE (a1, a2);
	}

	s += SAMPLE (x1 + width, x2 + width);
	
	return h * s * (1.0 / 3.0);
    }
}

static void
create_1d_filter (int              width,
		  pixman_kernel_t  reconstruct,
		  pixman_kernel_t  sample,
		  double           scale,
		  int              n_phases,
		  pixman_fixed_t *p)
{
    double step;
    int i;

    step = 1.0 / n_phases;

    for (i = 0; i < n_phases; ++i)
    {
        double frac = step / 2.0 + i * step;
	pixman_fixed_t new_total;
        int x, x1, x2;
	double total, e;

	/* Sample convolution of reconstruction and sampling
	 * filter. See rounding.txt regarding the rounding
	 * and sample positions.
	 */

	x1 = ceil (frac - width / 2.0 - 0.5);
	x2 = x1 + width;

	total = 0;
        for (x = x1; x < x2; ++x)
        {
	    double pos = x + 0.5 - frac;
	    double rlow = - filters[reconstruct].width / 2.0;
	    double rhigh = rlow + filters[reconstruct].width;
	    double slow = pos - scale * filters[sample].width / 2.0;
	    double shigh = slow + scale * filters[sample].width;
	    double c = 0.0;
	    double ilow, ihigh;

	    if (rhigh >= slow && rlow <= shigh)
	    {
		ilow = MAX (slow, rlow);
		ihigh = MIN (shigh, rhigh);

		c = integral (reconstruct, ilow,
			      sample, 1.0 / scale, ilow - pos,
			      ihigh - ilow);
	    }

            *p = (pixman_fixed_t)floor (c * 65536.0 + 0.5);
	    total += *p;
	    p++;
        }

	/* Normalize, with error diffusion */
	p -= width;
        total = 65536.0 / total;
        new_total = 0;
	e = 0.0;
	for (x = x1; x < x2; ++x)
	{
	    double v = (*p) * total + e;
	    pixman_fixed_t t = floor (v + 0.5);

	    e = v - t;
	    new_total += t;
	    *p++ = t;
	}

	/* pixman_fixed_e's worth of error may remain; put it
	 * at the first sample, since that is the only one that
	 * hasn't had any error diffused into it.
	 */
	*(p - width) += pixman_fixed_1 - new_total;
    }
}


static int
filter_width (pixman_kernel_t reconstruct, pixman_kernel_t sample, double size)
{
    return ceil (filters[reconstruct].width + size * filters[sample].width);
}

#ifdef PIXMAN_GNUPLOT

/* If enable-gnuplot is configured, then you can pipe the output of a
 * pixman-using program to gnuplot and get a continuously-updated plot
 * of the horizontal filter. This works well with demos/scale to test
 * the filter generation.
 *
 * The plot is all the different subposition filters shuffled
 * together. This is misleading in a few cases:
 *
 *  IMPULSE.BOX - goes up and down as the subfilters have different
 *		  numbers of non-zero samples
 *  IMPULSE.TRIANGLE - somewhat crooked for the same reason
 *  1-wide filters - looks triangular, but a 1-wide box would be more
 *		     accurate
 */
static void
gnuplot_filter (int width, int n_phases, const pixman_fixed_t* p)
{
    double step;
    int i, j;
    int first;

    step = 1.0 / n_phases;

    printf ("set style line 1 lc rgb '#0060ad' lt 1 lw 0.5 pt 7 pi 1 ps 0.5\n");
    printf ("plot [x=%g:%g] '-' with linespoints ls 1\n", -width*0.5, width*0.5);
    /* Print a point at the origin so that y==0 line is included: */
    printf ("0 0\n\n");

    /* The position of the first sample of the phase corresponding to
     * frac is given by:
     * 
     *     ceil (frac - width / 2.0 - 0.5) + 0.5 - frac
     * 
     * We have to find the frac that minimizes this expression.
     * 
     * For odd widths, we have
     * 
     *     ceil (frac - width / 2.0 - 0.5) + 0.5 - frac
     *   = ceil (frac) + K - frac
     *   = 1 + K - frac
     * 
     * for some K, so this is minimized when frac is maximized and
     * strictly growing with frac. So for odd widths, we can simply
     * start at the last phase and go backwards.
     * 
     * For even widths, we have
     * 
     *     ceil (frac - width / 2.0 - 0.5) + 0.5 - frac
     *   = ceil (frac - 0.5) + K - frac
     * 
     * The graph for this function (ignoring K) looks like this:
     * 
     *        0.5
     *           |    |\ 
     *           |    | \ 
     *           |    |  \ 
     *         0 |    |   \ 
     *           |\   |
     *           | \  |
     *           |  \ |
     *      -0.5 |   \|
     *   ---------------------------------
     *           0    0.5   1
     * 
     * So in this case we need to start with the phase whose frac is
     * less than, but as close as possible to 0.5, then go backwards
     * until we hit the first phase, then wrap around to the last
     * phase and continue backwards.
     * 
     * Which phase is as close as possible 0.5? The locations of the
     * sampling point corresponding to the kth phase is given by
     * 1/(2 * n_phases) + k / n_phases:
     * 
     *         1/(2 * n_phases) + k / n_phases = 0.5
     *  
     * from which it follows that
     * 
     *         k = (n_phases - 1) / 2
     * 
     * rounded down is the phase in question.
     */
    if (width & 1)
	first = n_phases - 1;
    else
	first = (n_phases - 1) / 2;

    for (j = 0; j < width; ++j)
    {
	for (i = 0; i < n_phases; ++i)
	{
	    int phase = first - i;
	    double frac, pos;

	    if (phase < 0)
		phase = n_phases + phase;

	    frac = step / 2.0 + phase * step;
	    pos = ceil (frac - width / 2.0 - 0.5) + 0.5 - frac + j;

	    printf ("%g %g\n",
		    pos,
		    pixman_fixed_to_double (*(p + phase * width + j)));
	}
    }

    printf ("e\n");
    fflush (stdout);
}

#endif

/* Create the parameter list for a SEPARABLE_CONVOLUTION filter
 * with the given kernels and scale parameters
 */
PIXMAN_EXPORT pixman_fixed_t *
pixman_filter_create_separable_convolution (int             *n_values,
					    pixman_fixed_t   scale_x,
					    pixman_fixed_t   scale_y,
					    pixman_kernel_t  reconstruct_x,
					    pixman_kernel_t  reconstruct_y,
					    pixman_kernel_t  sample_x,
					    pixman_kernel_t  sample_y,
					    int              subsample_bits_x,
					    int	             subsample_bits_y)
{
    double sx = fabs (pixman_fixed_to_double (scale_x));
    double sy = fabs (pixman_fixed_to_double (scale_y));
    pixman_fixed_t *params;
    int subsample_x, subsample_y;
    int width, height;

    width = filter_width (reconstruct_x, sample_x, sx);
    subsample_x = (1 << subsample_bits_x);

    height = filter_width (reconstruct_y, sample_y, sy);
    subsample_y = (1 << subsample_bits_y);

    *n_values = 4 + width * subsample_x + height * subsample_y;
    
    params = malloc (*n_values * sizeof (pixman_fixed_t));
    if (!params)
	return NULL;

    params[0] = pixman_int_to_fixed (width);
    params[1] = pixman_int_to_fixed (height);
    params[2] = pixman_int_to_fixed (subsample_bits_x);
    params[3] = pixman_int_to_fixed (subsample_bits_y);

    create_1d_filter (width, reconstruct_x, sample_x, sx, subsample_x,
		      params + 4);
    create_1d_filter (height, reconstruct_y, sample_y, sy, subsample_y,
		      params + 4 + width * subsample_x);

#ifdef PIXMAN_GNUPLOT
    gnuplot_filter(width, subsample_x, params + 4);
#endif

    return params;
}
