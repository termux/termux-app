#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

int
main (int argc, char **argv)
{
#define WIDTH 400
#define HEIGHT 200
    
    uint32_t *dest = malloc (WIDTH * HEIGHT * 4);
    pixman_image_t *src_img;
    pixman_image_t *dest_img;
    int i, j, k, p;

    typedef struct
    {
	pixman_point_fixed_t p0;
	pixman_point_fixed_t p1;
    } point_pair_t;
    
    pixman_gradient_stop_t onestop[1] =
	{
	    { pixman_int_to_fixed (1), { 0xffff, 0xeeee, 0xeeee, 0xeeee } },
	};

    pixman_gradient_stop_t subsetstops[2] =
	{
	    { pixman_int_to_fixed (1), { 0xffff, 0xeeee, 0xeeee, 0xeeee } },
	    { pixman_int_to_fixed (1), { 0xffff, 0xeeee, 0xeeee, 0xeeee } },
	};

    pixman_gradient_stop_t stops01[2] =
	{
	    { pixman_int_to_fixed (0), { 0xffff, 0xeeee, 0xeeee, 0xeeee } },
	    { pixman_int_to_fixed (1), { 0xffff, 0x1111, 0x1111, 0x1111 } }
	};

    point_pair_t point_pairs [] =
	{ { { pixman_double_to_fixed (0), 0 },
	    { pixman_double_to_fixed (WIDTH / 8.), pixman_int_to_fixed (0) } },
	  { { pixman_double_to_fixed (WIDTH / 2.0), pixman_double_to_fixed (HEIGHT / 2.0) },
	    { pixman_double_to_fixed (WIDTH / 2.0), pixman_double_to_fixed (HEIGHT / 2.0) } }
	};
    
    pixman_transform_t transformations[] = {
	{
	    { { pixman_double_to_fixed (2), pixman_double_to_fixed (0.5), pixman_double_to_fixed (-100), },
	      { pixman_double_to_fixed (0), pixman_double_to_fixed (3), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (0), pixman_double_to_fixed (0.000), pixman_double_to_fixed (1.0) } 
	    }
	},
	{
	    { { pixman_double_to_fixed (1), pixman_double_to_fixed (0), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (0), pixman_double_to_fixed (1), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (0), pixman_double_to_fixed (0.000), pixman_double_to_fixed (1.0) } 
	    }
	},
	{
	    { { pixman_double_to_fixed (2), pixman_double_to_fixed (1), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (1), pixman_double_to_fixed (1), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (2), pixman_double_to_fixed (1.000), pixman_double_to_fixed (1.0) } 
	    }
	},
	{
	    { { pixman_double_to_fixed (2), pixman_double_to_fixed (1), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (1), pixman_double_to_fixed (1), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (0), pixman_double_to_fixed (0), pixman_double_to_fixed (0) } 
	    }
	},
	{
	    { { pixman_double_to_fixed (2), pixman_double_to_fixed (1), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (1), pixman_double_to_fixed (1), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (2), pixman_double_to_fixed (-1), pixman_double_to_fixed (0) } 
	    }
	},
	{
	    { { pixman_double_to_fixed (2), pixman_double_to_fixed (1), pixman_double_to_fixed (3), },
	      { pixman_double_to_fixed (1), pixman_double_to_fixed (1), pixman_double_to_fixed (0), },
	      { pixman_double_to_fixed (2), pixman_double_to_fixed (-1), pixman_double_to_fixed (0) } 
	    }
	},
    };
    
    pixman_fixed_t r_inner;
    pixman_fixed_t r_outer;

    enable_divbyzero_exceptions();
    
    for (i = 0; i < WIDTH * HEIGHT; ++i)
	dest[i] = 0x4f00004f; /* pale blue */
    
    dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					 WIDTH, HEIGHT, 
					 dest,
					 WIDTH * 4);

    r_inner = 0;
    r_outer = pixman_double_to_fixed (50.0);
    
    for (i = 0; i < 3; ++i)
    {
	pixman_gradient_stop_t *stops;
        int num_stops;

	if (i == 0)
	{
	    stops = onestop;
	    num_stops = ARRAY_LENGTH (onestop);
	}
	else if (i == 1)
	{
	    stops = subsetstops;
	    num_stops = ARRAY_LENGTH (subsetstops);
	}
	else
	{
	    stops = stops01;
	    num_stops = ARRAY_LENGTH (stops01);
	}
	
	for (j = 0; j < 3; ++j)
	{
	    for (p = 0; p < ARRAY_LENGTH (point_pairs); ++p)
	    {
		point_pair_t *pair = &(point_pairs[p]);

		if (j == 0)
		    src_img = pixman_image_create_conical_gradient (&(pair->p0), r_inner,
								    stops, num_stops);
		else if (j == 1)
		    src_img = pixman_image_create_radial_gradient  (&(pair->p0), &(pair->p1),
								    r_inner, r_outer,
								    stops, num_stops);
		else
		    src_img = pixman_image_create_linear_gradient  (&(pair->p0), &(pair->p1),
								    stops, num_stops);
		
		for (k = 0; k < ARRAY_LENGTH (transformations); ++k)
		{
		    pixman_image_set_transform (src_img, &transformations[k]);
		    
		    pixman_image_set_repeat (src_img, PIXMAN_REPEAT_NONE);
		    pixman_image_composite (PIXMAN_OP_OVER, src_img, NULL, dest_img,
					    0, 0, 0, 0, 0, 0, 10 * WIDTH, HEIGHT);
		}

		pixman_image_unref (src_img);
	    }

	}
    }

    pixman_image_unref (dest_img);
    free (dest);
    
    return 0;
}
