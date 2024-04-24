#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pixman.h"
#include "gtk-utils.h"

int
main (int argc, char **argv)
{
#define WIDTH 200
#define HEIGHT 200

    pixman_image_t *src_img;
    pixman_image_t *mask_img;
    pixman_image_t *dest_img;
    pixman_trap_t trap;
    pixman_color_t white = { 0x0000, 0xffff, 0x0000, 0xffff };
    uint32_t *bits = malloc (WIDTH * HEIGHT * 4);
    uint32_t *mbits = malloc (WIDTH * HEIGHT);

    memset (mbits, 0, WIDTH * HEIGHT);
    memset (bits, 0xff, WIDTH * HEIGHT * 4);
    
    trap.top.l = pixman_int_to_fixed (50) + 0x8000;
    trap.top.r = pixman_int_to_fixed (150) + 0x8000;
    trap.top.y = pixman_int_to_fixed (30);

    trap.bot.l = pixman_int_to_fixed (50) + 0x8000;
    trap.bot.r = pixman_int_to_fixed (150) + 0x8000;
    trap.bot.y = pixman_int_to_fixed (150);

    mask_img = pixman_image_create_bits (PIXMAN_a8, WIDTH, HEIGHT, mbits, WIDTH);
    src_img = pixman_image_create_solid_fill (&white);
    dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, bits, WIDTH * 4);
    
    pixman_add_traps (mask_img, 0, 0, 1, &trap);

    pixman_image_composite (PIXMAN_OP_OVER,
			    src_img, mask_img, dest_img,
			    0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);
    
    show_image (dest_img);
    
    pixman_image_unref (src_img);
    pixman_image_unref (dest_img);
    free (bits);
    
    return 0;
}
