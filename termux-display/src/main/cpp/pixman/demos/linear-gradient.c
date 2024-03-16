#include "../test/utils.h"
#include "gtk-utils.h"

#define WIDTH 1024
#define HEIGHT 640

int
main (int argc, char **argv)
{
    pixman_image_t *src_img, *dest_img;
    pixman_gradient_stop_t stops[] = {
        { 0x00000, { 0x0000, 0x0000, 0x4444, 0xdddd } },
        { 0x10000, { 0xeeee, 0xeeee, 0x8888, 0xdddd } },
#if 0
        /* These colors make it very obvious that dithering
         * is useful even for 8-bit gradients
         */
	{ 0x00000, { 0x6666, 0x3333, 0x3333, 0xffff } },
	{ 0x10000, { 0x3333, 0x6666, 0x6666, 0xffff } },
#endif
    };
    pixman_point_fixed_t p1, p2;

    enable_divbyzero_exceptions ();

    dest_img = pixman_image_create_bits (PIXMAN_x8r8g8b8,
					 WIDTH, HEIGHT,
					 NULL, 0);

    p1.x = p1.y = 0x0000;
    p2.x = WIDTH << 16;
    p2.y = HEIGHT << 16;
    
    src_img = pixman_image_create_linear_gradient (&p1, &p2, stops, ARRAY_LENGTH (stops));

    pixman_image_composite32 (PIXMAN_OP_OVER,
			      src_img,
			      NULL,
			      dest_img,
			      0, 0,
			      0, 0,
			      0, 0,
			      WIDTH, HEIGHT);

    show_image (dest_img);

    pixman_image_unref (dest_img);

    return 0;
}
