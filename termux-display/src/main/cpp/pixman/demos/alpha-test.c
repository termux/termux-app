#include <stdio.h>
#include <stdlib.h>
#include "pixman.h"
#include "gtk-utils.h"

int
main (int argc, char **argv)
{
#define WIDTH 400
#define HEIGHT 200
    
    uint32_t *alpha = malloc (WIDTH * HEIGHT * 4);
    uint32_t *dest = malloc (WIDTH * HEIGHT * 4);
    uint32_t *src = malloc (WIDTH * HEIGHT * 4);
    pixman_image_t *grad_img;
    pixman_image_t *alpha_img;
    pixman_image_t *dest_img;
    pixman_image_t *src_img;
    int i;
    pixman_gradient_stop_t stops[2] =
	{
	    { pixman_int_to_fixed (0), { 0x0000, 0x0000, 0x0000, 0x0000 } },
	    { pixman_int_to_fixed (1), { 0xffff, 0x0000, 0x1111, 0xffff } }
	};
    pixman_point_fixed_t p1 = { pixman_double_to_fixed (0), 0 };
    pixman_point_fixed_t p2 = { pixman_double_to_fixed (WIDTH),
				pixman_int_to_fixed (0) };
#if 0
    pixman_transform_t trans = {
	{ { pixman_double_to_fixed (2), pixman_double_to_fixed (0.5), pixman_double_to_fixed (-100), },
	  { pixman_double_to_fixed (0), pixman_double_to_fixed (3), pixman_double_to_fixed (0), },
	  { pixman_double_to_fixed (0), pixman_double_to_fixed (0.000), pixman_double_to_fixed (1.0) } 
	}
    };
#else
    pixman_transform_t trans = {
	{ { pixman_fixed_1, 0, 0 },
	  { 0, pixman_fixed_1, 0 },
	  { 0, 0, pixman_fixed_1 } }
    };
#endif

#if 0
    pixman_point_fixed_t c_inner;
    pixman_point_fixed_t c_outer;
    pixman_fixed_t r_inner;
    pixman_fixed_t r_outer;
#endif
    
    for (i = 0; i < WIDTH * HEIGHT; ++i)
	alpha[i] = 0x4f00004f; /* pale blue */
    
    alpha_img = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					 WIDTH, HEIGHT, 
					  alpha,
					 WIDTH * 4);

    for (i = 0; i < WIDTH * HEIGHT; ++i)
	dest[i] = 0xffffff00;		/* yellow */
    
    dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					 WIDTH, HEIGHT, 
					 dest,
					 WIDTH * 4);

    for (i = 0; i < WIDTH * HEIGHT; ++i)
	src[i] = 0xffff0000;

    src_img = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					WIDTH, HEIGHT,
					src,
					WIDTH * 4);
    
#if 0
    c_inner.x = pixman_double_to_fixed (50.0);
    c_inner.y = pixman_double_to_fixed (50.0);
    c_outer.x = pixman_double_to_fixed (50.0);
    c_outer.y = pixman_double_to_fixed (50.0);
    r_inner = 0;
    r_outer = pixman_double_to_fixed (50.0);
    
    grad_img = pixman_image_create_conical_gradient (&c_inner, r_inner,
						    stops, 2);
#endif
#if 0
    grad_img = pixman_image_create_conical_gradient (&c_inner, r_inner,
						    stops, 2);
    grad_img = pixman_image_create_linear_gradient (&c_inner, &c_outer,
						   r_inner, r_outer,
						   stops, 2);
#endif
    
    grad_img = pixman_image_create_linear_gradient  (&p1, &p2,
						    stops, 2);

    pixman_image_set_transform (grad_img, &trans);
    pixman_image_set_repeat (grad_img, PIXMAN_REPEAT_PAD);
    
    pixman_image_composite (PIXMAN_OP_OVER, grad_img, NULL, alpha_img,
			    0, 0, 0, 0, 0, 0, 10 * WIDTH, HEIGHT);

    pixman_image_set_alpha_map (src_img, alpha_img, 10, 10);
    
    pixman_image_composite (PIXMAN_OP_OVER, src_img, NULL, dest_img,
			    0, 0, 0, 0, 0, 0, 10 * WIDTH, HEIGHT);
    
    printf ("0, 0: %x\n", dest[0]);
    printf ("10, 10: %x\n", dest[10 * 10 + 10]);
    printf ("w, h: %x\n", dest[(HEIGHT - 1) * 100 + (WIDTH - 1)]);
    
    show_image (dest_img);

    pixman_image_unref (src_img);
    pixman_image_unref (grad_img);
    pixman_image_unref (alpha_img);
    free (dest);
    
    return 0;
}
