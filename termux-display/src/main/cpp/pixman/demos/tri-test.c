#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../test/utils.h"
#include "gtk-utils.h"

int
main (int argc, char **argv)
{
#define WIDTH 200
#define HEIGHT 200

#define POINT(x,y)							\
    { pixman_double_to_fixed ((x)), pixman_double_to_fixed ((y)) }
    
    pixman_image_t *src_img, *dest_img;
    pixman_triangle_t tris[4] =
    {
	{ POINT (100, 100), POINT (10, 50), POINT (110, 10) },
	{ POINT (100, 100), POINT (150, 10), POINT (200, 50) },
	{ POINT (100, 100), POINT (10, 170), POINT (90, 175) },
	{ POINT (100, 100), POINT (170, 150), POINT (120, 190) },
    };
    pixman_color_t color = { 0x4444, 0x4444, 0xffff, 0xffff };
    uint32_t *bits = malloc (WIDTH * HEIGHT * 4);
    int i;

    for (i = 0; i < WIDTH * HEIGHT; ++i)
	bits[i] = (i / HEIGHT) * 0x01010000;
    
    src_img = pixman_image_create_solid_fill (&color);
    dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, bits, WIDTH * 4);
    
    pixman_composite_triangles (PIXMAN_OP_ATOP_REVERSE,
				src_img,
				dest_img,
				PIXMAN_a8,
				200, 200,
				-5, 5,
				ARRAY_LENGTH (tris), tris);
    show_image (dest_img);
    
    pixman_image_unref (src_img);
    pixman_image_unref (dest_img);
    free (bits);
    
    return 0;
}
