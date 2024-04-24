#include <math.h>
#include "pixman.h"
#include "gtk-utils.h"

#define F(x)								\
    pixman_double_to_fixed (x)

#define WIDTH 600
#define HEIGHT 300

static uint16_t
convert_to_srgb (uint16_t in)
{
    double d = in * (1/65535.0);
    double a = 0.055;

    if (d < 0.0031308)
	d = 12.92 * d;
    else
	d = (1 + a) * pow (d, 1 / 2.4) - a;

    return (d * 65535.0) + 0.5;
}

static void
convert_color (pixman_color_t *dest_srgb, pixman_color_t *linear)
{
    dest_srgb->alpha = convert_to_srgb (linear->alpha);
    dest_srgb->red = convert_to_srgb (linear->red);
    dest_srgb->green = convert_to_srgb (linear->green);
    dest_srgb->blue = convert_to_srgb (linear->blue);
}

int
main (int argc, char **argv)
{
    static const pixman_trapezoid_t traps[] =
    {
	{ F(10.10), F(280.0),
	  { { F(20.0), F(10.10) },
	    { F(5.3), F(280.0) } },
	  { { F(20.3), F(10.10) },
	    { F(5.6), F(280.0) } }
	},
	{ F(10.10), F(280.0),
	  { { F(40.0), F(10.10) },
	    { F(15.3), F(280.0) } },
	  { { F(41.0), F(10.10) },
	    { F(16.3), F(280.0) } }
	},
	{ F(10.10), F(280.0),
	  { { F(120.0), F(10.10) },
	    { F(5.3), F(280.0) } },
	  { { F(128.3), F(10.10) },
	    { F(6.6), F(280.0) } }
	},
	{ F(10.10), F(280.0),
	  { { F(60.0), F(10.10) },
	    { F(25.3), F(280.0) } },
	  { { F(61.0), F(10.10) },
	    { F(26.3), F(280.0) } }
	},
	{ F(10.10), F(280.0),
	  { { F(90.0), F(10.10) },
	    { F(55.3), F(280.0) } },
	  { { F(93.0), F(10.10) },
	    { F(58.3), F(280.0) } }
	},
	{ F(130.10), F(150.0),
	  { { F(100.0), F(130.10) },
	    { F(250.3), F(150.0) } },
	  { { F(110.0), F(130.10) },
	    { F(260.3), F(150.0) } }
	},
	{ F(170.10), F(240.0),
	  { { F(100.0), F(170.10) },
	    { F(120.3), F(240.0) } },
	  { { F(250.0), F(170.10) },
	    { F(250.3), F(240.0) } }
	},
    };

    pixman_image_t *src, *dest_srgb, *dest_linear;
    pixman_color_t bg = { 0x0000, 0x0000, 0x0000, 0xffff };
    pixman_color_t fg = { 0xffff, 0xffff, 0xffff, 0xffff };
    pixman_color_t fg_srgb;
    uint32_t *d;

    d = malloc (WIDTH * HEIGHT * 4);
    
    dest_srgb = pixman_image_create_bits (
	PIXMAN_a8r8g8b8_sRGB, WIDTH, HEIGHT, d, WIDTH * 4);
    dest_linear = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, WIDTH, HEIGHT, d, WIDTH * 4);
    
    src = pixman_image_create_solid_fill (&bg);
    pixman_image_composite32 (PIXMAN_OP_SRC,
			      src, NULL, dest_srgb,
			      0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);
    
    src = pixman_image_create_solid_fill (&fg);
    
    pixman_composite_trapezoids (PIXMAN_OP_OVER,
				 src, dest_srgb, PIXMAN_a8,
				 0, 0, 10, 10, G_N_ELEMENTS (traps), traps);

    convert_color (&fg_srgb, &fg);
    src = pixman_image_create_solid_fill (&fg_srgb);
    
    pixman_composite_trapezoids (PIXMAN_OP_OVER,
				 src, dest_linear, PIXMAN_a8,
				 0, 0, 310, 10, G_N_ELEMENTS (traps), traps);

    show_image (dest_linear);
    pixman_image_unref(dest_linear);
    free(d);
    
    return 0;
}
