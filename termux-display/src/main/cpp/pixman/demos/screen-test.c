#include <stdio.h>
#include <stdlib.h>
#include "pixman.h"
#include "gtk-utils.h"

int
main (int argc, char **argv)
{
#define WIDTH 40
#define HEIGHT 40
    
    uint32_t *src1 = malloc (WIDTH * HEIGHT * 4);
    uint32_t *src2 = malloc (WIDTH * HEIGHT * 4);
    uint32_t *src3 = malloc (WIDTH * HEIGHT * 4);
    uint32_t *dest = malloc (3 * WIDTH * 2 * HEIGHT * 4);
    pixman_image_t *simg1, *simg2, *simg3, *dimg;

    int i;

    for (i = 0; i < WIDTH * HEIGHT; ++i)
    {
	src1[i] = 0x7ff00000;
	src2[i] = 0x7f00ff00;
	src3[i] = 0x7f0000ff;
    }

    for (i = 0; i < 3 * WIDTH * 2 * HEIGHT; ++i)
    {
	dest[i] = 0x0;
    }

    simg1 = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, src1, WIDTH * 4);
    simg2 = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, src2, WIDTH * 4);
    simg3 = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, src3, WIDTH * 4);
    dimg  = pixman_image_create_bits (PIXMAN_a8r8g8b8, 3 * WIDTH, 2 * HEIGHT, dest, 3 * WIDTH * 4);

    pixman_image_composite (PIXMAN_OP_SCREEN, simg1, NULL, dimg, 0, 0, 0, 0, WIDTH, HEIGHT / 4, WIDTH, HEIGHT);
    pixman_image_composite (PIXMAN_OP_SCREEN, simg2, NULL, dimg, 0, 0, 0, 0, (WIDTH/2), HEIGHT / 4 + HEIGHT / 2, WIDTH, HEIGHT);
    pixman_image_composite (PIXMAN_OP_SCREEN, simg3, NULL, dimg, 0, 0, 0, 0, (4 * WIDTH) / 3, HEIGHT, WIDTH, HEIGHT);

    show_image (dimg);
    
    return 0;
}
