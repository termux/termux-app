#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"
#include "pixman-inlines.h"

/* A trivial reference implementation for
 * 'bilinear_pad_repeat_get_scanline_bounds'
 */
static void
bilinear_pad_repeat_get_scanline_bounds_ref (int32_t        source_image_width,
					     pixman_fixed_t vx_,
					     pixman_fixed_t unit_x,
					     int32_t *      left_pad,
					     int32_t *      left_tz,
					     int32_t *      width,
					     int32_t *      right_tz,
					     int32_t *      right_pad)
{
    int w = *width;
    int64_t vx = vx_;
    *left_pad = 0;
    *left_tz = 0;
    *width = 0;
    *right_tz = 0;
    *right_pad = 0;
    while (--w >= 0)
    {
	if (vx < 0)
	{
	    if (vx + pixman_fixed_1 < 0)
		*left_pad += 1;
	    else
		*left_tz += 1;
	}
	else if (vx + pixman_fixed_1 >= pixman_int_to_fixed (source_image_width))
	{
	    if (vx >= pixman_int_to_fixed (source_image_width))
		*right_pad += 1;
	    else
		*right_tz += 1;
	}
	else
	{
	    *width += 1;
	}
	vx += unit_x;
    }
}

int
main (void)
{
    int i;
    prng_srand (0);
    for (i = 0; i < 10000; i++)
    {
	int32_t left_pad1, left_tz1, width1, right_tz1, right_pad1;
	int32_t left_pad2, left_tz2, width2, right_tz2, right_pad2;
	pixman_fixed_t vx = prng_rand_n(10000 << 16) - (3000 << 16);
	int32_t width = prng_rand_n(10000);
	int32_t source_image_width = prng_rand_n(10000) + 1;
	pixman_fixed_t unit_x = prng_rand_n(10 << 16) + 1;
	width1 = width2 = width;

	bilinear_pad_repeat_get_scanline_bounds_ref (source_image_width,
						     vx,
						     unit_x,
						     &left_pad1,
						     &left_tz1,
						     &width1,
						     &right_tz1,
						     &right_pad1);

	bilinear_pad_repeat_get_scanline_bounds (source_image_width,
						 vx,
						 unit_x,
						 &left_pad2,
						 &left_tz2,
						 &width2,
						 &right_tz2,
						 &right_pad2);

	assert (left_pad1 == left_pad2);
	assert (left_tz1 == left_tz2);
	assert (width1 == width2);
	assert (right_tz1 == right_tz2);
	assert (right_pad1 == right_pad2);
    }

    return 0;
}
