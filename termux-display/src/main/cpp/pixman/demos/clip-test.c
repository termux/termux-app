#include <stdio.h>
#include <stdlib.h>
#include "pixman.h"
#include "gtk-utils.h"

#define WIDTH 200
#define HEIGHT 200
    
static pixman_image_t *
create_solid_bits (uint32_t pixel)
{
    uint32_t *pixels = malloc (WIDTH * HEIGHT * 4);
    int i;
    
    for (i = 0; i < WIDTH * HEIGHT; ++i)
	pixels[i] = pixel;

    return pixman_image_create_bits (PIXMAN_a8r8g8b8,
				     WIDTH, HEIGHT, 
				     pixels,
				     WIDTH * 4);
}

int
main (int argc, char **argv)
{
    pixman_image_t *gradient_img;
    pixman_image_t *src_img, *dst_img;
    pixman_gradient_stop_t stops[2] =
	{
	    { pixman_int_to_fixed (0), { 0xffff, 0x0000, 0x0000, 0xffff } },
	    { pixman_int_to_fixed (1), { 0xffff, 0xffff, 0x0000, 0xffff } }
	};
#if 0
    pixman_point_fixed_t p1 = { 0, 0 };
    pixman_point_fixed_t p2 = { pixman_int_to_fixed (WIDTH),
				pixman_int_to_fixed (HEIGHT) };
#endif
    pixman_point_fixed_t c_inner;
    pixman_point_fixed_t c_outer;
    pixman_fixed_t r_inner;
    pixman_fixed_t r_outer;
    pixman_region32_t clip_region;
    pixman_transform_t trans = {
	{ { pixman_double_to_fixed (1.3), pixman_double_to_fixed (0), pixman_double_to_fixed (-0.5), },
	  { pixman_double_to_fixed (0), pixman_double_to_fixed (1), pixman_double_to_fixed (-0.5), },
	  { pixman_double_to_fixed (0), pixman_double_to_fixed (0), pixman_double_to_fixed (1.0) } 
	}
    };
    
    src_img = create_solid_bits (0xff0000ff);
    
    c_inner.x = pixman_double_to_fixed (100.0);
    c_inner.y = pixman_double_to_fixed (100.0);
    c_outer.x = pixman_double_to_fixed (100.0);
    c_outer.y = pixman_double_to_fixed (100.0);
    r_inner = 0;
    r_outer = pixman_double_to_fixed (100.0);
    
    gradient_img = pixman_image_create_radial_gradient (&c_inner, &c_outer,
							r_inner, r_outer,
							stops, 2);

#if 0
    gradient_img = pixman_image_create_linear_gradient  (&p1, &p2,
							 stops, 2);
    
#endif

    pixman_image_composite (PIXMAN_OP_OVER, gradient_img, NULL, src_img,
			    0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);
    
    pixman_region32_init_rect (&clip_region, 50, 0, 100, 200);
    pixman_image_set_clip_region32 (src_img, &clip_region);
    pixman_image_set_source_clipping (src_img, TRUE);
    pixman_image_set_has_client_clip (src_img, TRUE);
    pixman_image_set_transform (src_img, &trans);
    pixman_image_set_repeat (src_img, PIXMAN_REPEAT_NORMAL);
    
    dst_img = create_solid_bits (0xffff0000);
    pixman_image_composite (PIXMAN_OP_OVER, src_img, NULL, dst_img,
			    0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);
    

#if 0
    printf ("0, 0: %x\n", src[0]);
    printf ("10, 10: %x\n", src[10 * 10 + 10]);
    printf ("w, h: %x\n", src[(HEIGHT - 1) * 100 + (WIDTH - 1)]);
#endif
    
    show_image (dst_img);
    
    pixman_image_unref (gradient_img);
    pixman_image_unref (src_img);
    
    return 0;
}
