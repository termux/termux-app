#include <stdio.h>
#include <stdlib.h>
#include "pixman.h"
#include "gtk-utils.h"

int
main (int argc, char **argv)
{
#define WIDTH 400
#define HEIGHT 400
#define TILE_SIZE 25

    pixman_image_t *checkerboard;
    pixman_image_t *destination;
#define D2F(d) (pixman_double_to_fixed(d))
    pixman_transform_t trans = { {
	    { D2F (-1.96830), D2F (-1.82250), D2F (512.12250)},
	    { D2F (0.00000), D2F (-7.29000), D2F (1458.00000)},
	    { D2F (0.00000), D2F (-0.00911), D2F (0.59231)},
	}};
    int i, j;

    checkerboard = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					     WIDTH, HEIGHT,
					     NULL, 0);

    destination = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					    WIDTH, HEIGHT,
					    NULL, 0);

    for (i = 0; i < HEIGHT / TILE_SIZE; ++i)
    {
	for (j = 0; j < WIDTH / TILE_SIZE; ++j)
	{
	    double u = (double)(j + 1) / (WIDTH / TILE_SIZE);
	    double v = (double)(i + 1) / (HEIGHT / TILE_SIZE);
	    pixman_color_t black = { 0, 0, 0, 0xffff };
	    pixman_color_t white = {
		v * 0xffff,
		u * 0xffff,
		(1 - (double)u) * 0xffff,
		0xffff };
	    pixman_color_t *c;
	    pixman_image_t *fill;

	    if ((j & 1) != (i & 1))
		c = &black;
	    else
		c = &white;

	    fill = pixman_image_create_solid_fill (c);

	    pixman_image_composite (PIXMAN_OP_SRC, fill, NULL, checkerboard,
				    0, 0, 0, 0, j * TILE_SIZE, i * TILE_SIZE,
				    TILE_SIZE, TILE_SIZE);
	}
    }

    pixman_image_set_transform (checkerboard, &trans);
    pixman_image_set_filter (checkerboard, PIXMAN_FILTER_BEST, NULL, 0);
    pixman_image_set_repeat (checkerboard, PIXMAN_REPEAT_NONE);

    pixman_image_composite (PIXMAN_OP_SRC,
			    checkerboard, NULL, destination,
			    0, 0, 0, 0, 0, 0,
			    WIDTH, HEIGHT);

    show_image (destination);

    return 0;
}
