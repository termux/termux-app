#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pixman.h"
#include "gtk-utils.h"

/* This test demonstrates that clipping is done totally different depending
 * on whether the source is transformed or not.
 */
int
main (int argc, char **argv)
{
#define WIDTH 200
#define HEIGHT 200

#define SMALL 25
    
    uint32_t *sbits = malloc (SMALL * SMALL * 4);
    uint32_t *bits = malloc (WIDTH * HEIGHT * 4);
    pixman_transform_t trans = {
    {
	{ pixman_double_to_fixed (1.0), pixman_double_to_fixed (0), pixman_double_to_fixed (-0.1), },
	{ pixman_double_to_fixed (0), pixman_double_to_fixed (1), pixman_double_to_fixed (-0.1), },
	{ pixman_double_to_fixed (0), pixman_double_to_fixed (0), pixman_double_to_fixed (1.0) }
    } };
	  
    pixman_image_t *src_img = pixman_image_create_bits (PIXMAN_a8r8g8b8, SMALL, SMALL, sbits, 4 * SMALL);
    pixman_image_t *dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, bits, 4 * WIDTH);

    memset (bits, 0xff, WIDTH * HEIGHT * 4);
    memset (sbits, 0x00, SMALL * SMALL * 4);

    pixman_image_composite (PIXMAN_OP_IN,
			    src_img, NULL, dest_img,
			    0, 0, 0, 0, SMALL, SMALL, 200, 200);
    
    pixman_image_set_transform (src_img, &trans);
    
    pixman_image_composite (PIXMAN_OP_IN,
			    src_img, NULL, dest_img,
			    0, 0, 0, 0, SMALL * 2, SMALL * 2, 200, 200);
    
    show_image (dest_img);
    
    pixman_image_unref (src_img);
    pixman_image_unref (dest_img);
    free (bits);
    
    return 0;
}
