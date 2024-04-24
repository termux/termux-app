#include "utils.h"
#include <stdio.h>

int
main ()
{
    static const pixman_point_fixed_t inner = { 0x0000, 0x0000 };
    static const pixman_point_fixed_t outer = { 0x0000, 0x0000 };
    static const pixman_fixed_t r_inner = 0;
    static const pixman_fixed_t r_outer = 64 << 16;
    static const pixman_gradient_stop_t stops[] = {
	{ 0x00000, { 0x6666, 0x6666, 0x6666, 0xffff } },
	{ 0x10000, { 0x0000, 0x0000, 0x0000, 0xffff } }
    };
    static const pixman_transform_t transform = {
	{ { 0x0,        0x26ee, 0x0}, 
	  { 0xffffeeef, 0x0,    0x0}, 
	  { 0x0,        0x0,    0x10000}
	}
    };
    static const pixman_color_t z = { 0x0000, 0x0000, 0x0000, 0x0000 };
    pixman_image_t *dest, *radial, *zero;
    int i;
    double before, after;

    dest = pixman_image_create_bits (
	PIXMAN_x8r8g8b8, 640, 429, NULL, -1);
    zero = pixman_image_create_solid_fill (&z);
    radial = pixman_image_create_radial_gradient (
	&inner, &outer, r_inner, r_outer, stops, ARRAY_LENGTH (stops));
    pixman_image_set_transform (radial, &transform);
    pixman_image_set_repeat (radial, PIXMAN_REPEAT_PAD);

#define N_COMPOSITE	500

    before = gettime();
    for (i = 0; i < N_COMPOSITE; ++i)
    {
	before -= gettime();

	pixman_image_composite (
	    PIXMAN_OP_SRC, zero, NULL, dest,
	    0, 0, 0, 0, 0, 0, 640, 429);

	before += gettime();

	pixman_image_composite32 (
	    PIXMAN_OP_OVER, radial, NULL, dest,
	    - 150, -158, 0, 0, 0, 0, 640, 361);
    }

    after = gettime();

    write_png (dest, "radial.png");

    printf ("Average time to composite: %f\n", (after - before) / N_COMPOSITE);
    return 0;
}
