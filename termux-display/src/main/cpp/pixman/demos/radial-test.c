#include "../test/utils.h"
#include "gtk-utils.h"

#define NUM_GRADIENTS 9
#define NUM_STOPS 3
#define NUM_REPEAT 4
#define SIZE 128
#define WIDTH (SIZE * NUM_GRADIENTS)
#define HEIGHT (SIZE * NUM_REPEAT)

/*
 * We want to test all the possible relative positions of the start
 * and end circle:
 *
 *  - The start circle can be smaller/equal/bigger than the end
 *    circle. A radial gradient can be classified in one of these
 *    three cases depending on the sign of dr.
 *
 *  - The smaller circle can be completely inside/internally
 *    tangent/outside (at least in part) of the bigger circle. This
 *    classification is the same as the one which can be computed by
 *    examining the sign of a = (dx^2 + dy^2 - dr^2).
 *
 *  - If the two circles have the same size, neither can be inside or
 *    internally tangent
 *
 * This test draws radial gradients whose circles always have the same
 * centers (0, 0) and (1, 0), but with different radiuses. From left
 * to right:
 *
 * - Degenerate start circle completely inside the end circle
 *     0.00 -> 1.75; dr = 1.75 > 0; a = 1 - 1.75^2 < 0
 *
 * - Small start circle completely inside the end circle
 *     0.25 -> 1.75; dr =  1.5 > 0; a = 1 - 1.50^2 < 0
 *
 * - Small start circle internally tangent to the end circle
 *     0.50 -> 1.50; dr =  1.0 > 0; a = 1 - 1.00^2 = 0
 *
 * - Small start circle outside of the end circle
 *     0.50 -> 1.00; dr =  0.5 > 0; a = 1 - 0.50^2 > 0
 *
 * - Start circle with the same size as the end circle
 *     1.00 -> 1.00; dr =  0.0 = 0; a = 1 - 0.00^2 > 0
 *
 * - Small end circle outside of the start circle
 *     1.00 -> 0.50; dr = -0.5 > 0; a = 1 - 0.50^2 > 0
 *
 * - Small end circle internally tangent to the start circle
 *     1.50 -> 0.50; dr = -1.0 > 0; a = 1 - 1.00^2 = 0
 *
 * - Small end circle completely inside the start circle
 *     1.75 -> 0.25; dr = -1.5 > 0; a = 1 - 1.50^2 < 0
 *
 * - Degenerate end circle completely inside the start circle
 *     0.00 -> 1.75; dr = 1.75 > 0; a = 1 - 1.75^2 < 0
 *
 */

const static double radiuses[NUM_GRADIENTS] = {
    0.00,
    0.25,
    0.50,
    0.50,
    1.00,
    1.00,
    1.50,
    1.75,
    1.75
};

#define double_to_color(x)					\
    (((uint32_t) ((x)*65536)) - (((uint32_t) ((x)*65536)) >> 16))

#define PIXMAN_STOP(offset,r,g,b,a)		\
    { pixman_double_to_fixed (offset),		\
	{					\
	double_to_color (r),			\
	double_to_color (g),			\
	double_to_color (b),			\
	double_to_color (a)			\
	}					\
    }

static const pixman_gradient_stop_t stops[NUM_STOPS] = {
    PIXMAN_STOP (0.0,        1, 0, 0, 0.75),
    PIXMAN_STOP (0.70710678, 0, 1, 0, 0),
    PIXMAN_STOP (1.0,        0, 0, 1, 1)
};

static pixman_image_t *
create_radial (int index)
{
    pixman_point_fixed_t p0, p1;
    pixman_fixed_t r0, r1;
    double x0, x1, radius0, radius1, left, right, center;

    x0 = 0;
    x1 = 1;
    radius0 = radiuses[index];
    radius1 = radiuses[NUM_GRADIENTS - index - 1];

    /* center the gradient */
    left = MIN (x0 - radius0, x1 - radius1);
    right = MAX (x0 + radius0, x1 + radius1);
    center = (left + right) * 0.5;
    x0 -= center;
    x1 -= center;

    /* scale to make it fit within a 1x1 rect centered in (0,0) */
    x0 *= 0.25;
    x1 *= 0.25;
    radius0 *= 0.25;
    radius1 *= 0.25;

    p0.x = pixman_double_to_fixed (x0);
    p0.y = pixman_double_to_fixed (0);

    p1.x = pixman_double_to_fixed (x1);
    p1.y = pixman_double_to_fixed (0);

    r0 = pixman_double_to_fixed (radius0);
    r1 = pixman_double_to_fixed (radius1);

    return pixman_image_create_radial_gradient (&p0, &p1,
						r0, r1,
						stops, NUM_STOPS);
}

static const pixman_repeat_t repeat[NUM_REPEAT] = {
    PIXMAN_REPEAT_NONE,
    PIXMAN_REPEAT_NORMAL,
    PIXMAN_REPEAT_REFLECT,
    PIXMAN_REPEAT_PAD
};

int
main (int argc, char **argv)
{
    pixman_transform_t transform;
    pixman_image_t *src_img, *dest_img;
    int i, j;

    enable_divbyzero_exceptions ();

    dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					 WIDTH, HEIGHT,
					 NULL, 0);

    draw_checkerboard (dest_img, 25, 0xffaaaaaa, 0xffbbbbbb);
    
    pixman_transform_init_identity (&transform);

    /*
     * The create_radial() function returns gradients centered in the
     * origin and whose interesting part fits a 1x1 square. We want to
     * paint these gradients on a SIZExSIZE square and to make things
     * easier we want the origin in the top-left corner of the square
     * we want to see.
     */
    pixman_transform_translate (NULL, &transform,
				pixman_double_to_fixed (0.5),
				pixman_double_to_fixed (0.5));

    pixman_transform_scale (NULL, &transform,
			    pixman_double_to_fixed (SIZE),
			    pixman_double_to_fixed (SIZE));

    /*
     * Gradients are evaluated at the center of each pixel, so we need
     * to translate by half a pixel to trigger some interesting
     * cornercases. In particular, the original implementation of PDF
     * radial gradients tried to divide by 0 when using this transform
     * on the "tangent circles" cases.
     */
    pixman_transform_translate (NULL, &transform,
				pixman_double_to_fixed (0.5),
				pixman_double_to_fixed (0.5));

    for (i = 0; i < NUM_GRADIENTS; i++)
    {
	src_img = create_radial (i);
	pixman_image_set_transform (src_img, &transform);

	for (j = 0; j < NUM_REPEAT; j++)
	{
	    pixman_image_set_repeat (src_img, repeat[j]);

	    pixman_image_composite32 (PIXMAN_OP_OVER,
				      src_img,
				      NULL,
				      dest_img,
				      0, 0,
				      0, 0,
				      i * SIZE, j * SIZE,
				      SIZE, SIZE);

	}

	pixman_image_unref (src_img);
    }

    show_image (dest_img);

    pixman_image_unref (dest_img);

    return 0;
}
