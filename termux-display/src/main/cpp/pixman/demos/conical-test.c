#include "../test/utils.h"
#include "gtk-utils.h"

#define SIZE 128
#define GRADIENTS_PER_ROW 7
#define NUM_ROWS ((NUM_GRADIENTS + GRADIENTS_PER_ROW - 1) / GRADIENTS_PER_ROW)
#define WIDTH (SIZE * GRADIENTS_PER_ROW)
#define HEIGHT (SIZE * NUM_ROWS)
#define NUM_GRADIENTS 35

#define double_to_color(x)					\
    (((uint32_t) ((x)*65536)) - (((uint32_t) ((x)*65536)) >> 16))

#define PIXMAN_STOP(offset,r,g,b,a)		\
    { pixman_double_to_fixed (offset),		\
	{					\
	    double_to_color (r),		\
		double_to_color (g),		\
		double_to_color (b),		\
		double_to_color (a)		\
	}					\
    }


static const pixman_gradient_stop_t stops[] = {
    PIXMAN_STOP (0.25,       1, 0, 0, 0.7),
    PIXMAN_STOP (0.5,        1, 1, 0, 0.7),
    PIXMAN_STOP (0.75,       0, 1, 0, 0.7),
    PIXMAN_STOP (1.0,        0, 0, 1, 0.7)
};

#define NUM_STOPS (sizeof (stops) / sizeof (stops[0]))

static pixman_image_t *
create_conical (int index)
{
    pixman_point_fixed_t c;
    double angle;

    c.x = pixman_double_to_fixed (0);
    c.y = pixman_double_to_fixed (0);

    angle = (0.5 / NUM_GRADIENTS + index / (double)NUM_GRADIENTS) * 720 - 180;

    return pixman_image_create_conical_gradient (
	&c, pixman_double_to_fixed (angle), stops, NUM_STOPS);
}

int
main (int argc, char **argv)
{
    pixman_transform_t transform;
    pixman_image_t *src_img, *dest_img;
    int i;

    enable_divbyzero_exceptions ();

    dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					 WIDTH, HEIGHT,
					 NULL, 0);
 
    draw_checkerboard (dest_img, 25, 0xffaaaaaa, 0xff888888);

    pixman_transform_init_identity (&transform);

    pixman_transform_translate (NULL, &transform,
				pixman_double_to_fixed (0.5),
				pixman_double_to_fixed (0.5));

    pixman_transform_scale (NULL, &transform,
			    pixman_double_to_fixed (SIZE),
			    pixman_double_to_fixed (SIZE));
    pixman_transform_translate (NULL, &transform,
				pixman_double_to_fixed (0.5),
				pixman_double_to_fixed (0.5));

    for (i = 0; i < NUM_GRADIENTS; i++)
    {
	int column = i % GRADIENTS_PER_ROW;
	int row = i / GRADIENTS_PER_ROW;

	src_img = create_conical (i); 
	pixman_image_set_repeat (src_img, PIXMAN_REPEAT_NORMAL);
   
	pixman_image_set_transform (src_img, &transform);
	
	pixman_image_composite32 (
	    PIXMAN_OP_OVER, src_img, NULL,dest_img,
	    0, 0, 0, 0, column * SIZE, row * SIZE,
	    SIZE, SIZE);
	
	pixman_image_unref (src_img);
    }

    show_image (dest_img);

    pixman_image_unref (dest_img);

    return 0;
}
