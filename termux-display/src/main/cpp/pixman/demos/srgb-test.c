#include <math.h>

#include "pixman.h"
#include "gtk-utils.h"

static uint32_t
linear_argb_to_premult_argb (float a,
			     float r,
			     float g,
			     float b)
{
    r *= a;
    g *= a;
    b *= a;
    return (uint32_t) (a * 255.0f + 0.5f) << 24
	 | (uint32_t) (r * 255.0f + 0.5f) << 16
	 | (uint32_t) (g * 255.0f + 0.5f) <<  8
	 | (uint32_t) (b * 255.0f + 0.5f) <<  0;
}

static float
lin2srgb (float linear)
{
    if (linear < 0.0031308f)
	return linear * 12.92f;
    else
	return 1.055f * powf (linear, 1.0f/2.4f) - 0.055f;
}

static uint32_t
linear_argb_to_premult_srgb_argb (float a,
				  float r,
				  float g,
				  float b)
{
    r = lin2srgb (r * a);
    g = lin2srgb (g * a);
    b = lin2srgb (b * a);
    return (uint32_t) (a * 255.0f + 0.5f) << 24
	 | (uint32_t) (r * 255.0f + 0.5f) << 16
	 | (uint32_t) (g * 255.0f + 0.5f) <<  8
	 | (uint32_t) (b * 255.0f + 0.5f) <<  0;
}

int
main (int argc, char **argv)
{
#define WIDTH 400
#define HEIGHT 200
    int y, x, p;
    float alpha;
 
    uint32_t *dest = malloc (WIDTH * HEIGHT * 4);
    uint32_t *src1 = malloc (WIDTH * HEIGHT * 4);
    pixman_image_t *dest_img, *src1_img;
   
    dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8_sRGB,
					 WIDTH, HEIGHT,
					 dest,
					 WIDTH * 4);
    src1_img = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					 WIDTH, HEIGHT,
					 src1,
					 WIDTH * 4);

    for (y = 0; y < HEIGHT; y ++)
    {
	p = WIDTH * y;
	for (x = 0; x < WIDTH; x ++)
	{
	     alpha = (float) x / WIDTH;
	     src1[p + x] = linear_argb_to_premult_argb (alpha, 1, 0, 1);
	     dest[p + x] = linear_argb_to_premult_srgb_argb (1-alpha, 0, 1, 0);
	}
    }
    
    pixman_image_composite (PIXMAN_OP_ADD, src1_img, NULL, dest_img,
			    0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);
    pixman_image_unref (src1_img);
    free (src1);

    show_image (dest_img);
    pixman_image_unref (dest_img);
    free (dest);
    
    return 0;
}
