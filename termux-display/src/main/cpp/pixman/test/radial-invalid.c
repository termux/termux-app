#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define WIDTH 100
#define HEIGHT 100

int
main ()
{
    pixman_image_t *radial;
    pixman_image_t *dest = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, WIDTH, HEIGHT, NULL, -1);

    static const pixman_transform_t xform =
	{
	    { { 0x346f7, 0x0, 0x0 },
	      { 0x0, 0x346f7, 0x0 },
	      { 0x0, 0x0, 0x10000 }
	    },
	};

    static const pixman_gradient_stop_t stops[] = 
    {
	{ 0xde61, { 0x4481, 0x96e8, 0x1e6a, 0x29e1 } },
	{ 0xfdd5, { 0xfa10, 0xcc26, 0xbc43, 0x1eb7 } },
	{ 0xfe1e, { 0xd257, 0x5bac, 0x6fc2, 0xa33b } },
    };

    static const pixman_point_fixed_t inner = { 0x320000, 0x320000 };
    static const pixman_point_fixed_t outer = { 0x320000, 0x3cb074 };

    enable_divbyzero_exceptions ();
    enable_invalid_exceptions ();

    radial = pixman_image_create_radial_gradient (
	&inner,
	&outer,
	0xab074, /* inner radius */
	0x0,	 /* outer radius */
	stops, sizeof (stops) / sizeof (stops[0]));

    pixman_image_set_repeat (radial, PIXMAN_REPEAT_REFLECT);
    pixman_image_set_transform (radial, &xform);

    pixman_image_composite (
	PIXMAN_OP_OVER,
	radial, NULL, dest,
	0, 0, 0, 0,
	0, 0, WIDTH, HEIGHT);

    return 0;
}
