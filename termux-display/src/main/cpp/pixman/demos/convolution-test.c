#include <stdio.h>
#include <stdlib.h>
#include "pixman.h"
#include "gtk-utils.h"

int
main (int argc, char **argv)
{
#define WIDTH 200
#define HEIGHT 200

#define d2f pixman_double_to_fixed
    
    uint32_t *src = malloc (WIDTH * HEIGHT * 4);
    uint32_t *mask = malloc (WIDTH * HEIGHT * 4);
    uint32_t *dest = malloc (WIDTH * HEIGHT * 4);
    pixman_fixed_t convolution[] =
    {
	d2f (3), d2f (3),
	d2f (0.5), d2f (0.5), d2f (0.5),
	d2f (0.5), d2f (0.5), d2f (0.5),
	d2f (0.5), d2f (0.5), d2f (0.5),
    };
    pixman_image_t *simg, *mimg, *dimg;

    int i;

    for (i = 0; i < WIDTH * HEIGHT; ++i)
    {
	src[i] = 0x7f007f00;
	mask[i] = (i % 256) * 0x01000000;
	dest[i] = 0;
    }

    simg = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, src, WIDTH * 4);
    mimg = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, mask, WIDTH * 4);
    dimg = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, dest, WIDTH * 4);

    pixman_image_set_filter (mimg, PIXMAN_FILTER_CONVOLUTION,
			     convolution, 11);

    pixman_image_composite (PIXMAN_OP_OVER, simg, mimg, dimg, 0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);

    show_image (dimg);
    
    return 0;
}
