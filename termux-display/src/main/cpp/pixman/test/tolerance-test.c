#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "utils.h"

#define MAX_WIDTH  16
#define MAX_HEIGHT 16
#define MAX_STRIDE 4

static const pixman_format_code_t formats[] =
{
    PIXMAN_a2r10g10b10,
    PIXMAN_x2r10g10b10,
    PIXMAN_a8r8g8b8,
    PIXMAN_a4r4g4b4,
    PIXMAN_a2r2g2b2,
    PIXMAN_r5g6b5,
    PIXMAN_r3g3b2,
};

static const pixman_op_t operators[] =
{
    PIXMAN_OP_CLEAR,
    PIXMAN_OP_SRC,
    PIXMAN_OP_DST,
    PIXMAN_OP_OVER,
    PIXMAN_OP_OVER_REVERSE,
    PIXMAN_OP_IN,
    PIXMAN_OP_IN_REVERSE,
    PIXMAN_OP_OUT,
    PIXMAN_OP_OUT_REVERSE,
    PIXMAN_OP_ATOP,
    PIXMAN_OP_ATOP_REVERSE,
    PIXMAN_OP_XOR,
    PIXMAN_OP_ADD,
    PIXMAN_OP_SATURATE,

    PIXMAN_OP_DISJOINT_CLEAR,
    PIXMAN_OP_DISJOINT_SRC,
    PIXMAN_OP_DISJOINT_DST,
    PIXMAN_OP_DISJOINT_OVER,
    PIXMAN_OP_DISJOINT_OVER_REVERSE,
    PIXMAN_OP_DISJOINT_IN,
    PIXMAN_OP_DISJOINT_IN_REVERSE,
    PIXMAN_OP_DISJOINT_OUT,
    PIXMAN_OP_DISJOINT_OUT_REVERSE,
    PIXMAN_OP_DISJOINT_ATOP,
    PIXMAN_OP_DISJOINT_ATOP_REVERSE,
    PIXMAN_OP_DISJOINT_XOR,

    PIXMAN_OP_CONJOINT_CLEAR,
    PIXMAN_OP_CONJOINT_SRC,
    PIXMAN_OP_CONJOINT_DST,
    PIXMAN_OP_CONJOINT_OVER,
    PIXMAN_OP_CONJOINT_OVER_REVERSE,
    PIXMAN_OP_CONJOINT_IN,
    PIXMAN_OP_CONJOINT_IN_REVERSE,
    PIXMAN_OP_CONJOINT_OUT,
    PIXMAN_OP_CONJOINT_OUT_REVERSE,
    PIXMAN_OP_CONJOINT_ATOP,
    PIXMAN_OP_CONJOINT_ATOP_REVERSE,
    PIXMAN_OP_CONJOINT_XOR,

    PIXMAN_OP_MULTIPLY,
    PIXMAN_OP_SCREEN,
    PIXMAN_OP_OVERLAY,
    PIXMAN_OP_DARKEN,
    PIXMAN_OP_LIGHTEN,
    PIXMAN_OP_COLOR_DODGE,
    PIXMAN_OP_COLOR_BURN,
    PIXMAN_OP_HARD_LIGHT,
    PIXMAN_OP_SOFT_LIGHT,
    PIXMAN_OP_DIFFERENCE,
    PIXMAN_OP_EXCLUSION,
};

static const pixman_dither_t dithers[] =
{
    PIXMAN_DITHER_ORDERED_BAYER_8,
    PIXMAN_DITHER_ORDERED_BLUE_NOISE_64,
};

#define RANDOM_ELT(array)                                               \
    (array[prng_rand_n (ARRAY_LENGTH (array))])

static void
free_bits (pixman_image_t *image, void *data)
{
    free (image->bits.bits);
}

static pixman_image_t *
create_image (pixman_image_t **clone)
{
    pixman_format_code_t format = RANDOM_ELT (formats);
    pixman_image_t *image;
    int width = prng_rand_n (MAX_WIDTH);
    int height = prng_rand_n (MAX_HEIGHT);
    int stride = ((width * (PIXMAN_FORMAT_BPP (format) / 8)) + 3) & ~3;
    uint32_t *bytes = malloc (stride * height);

    prng_randmemset (bytes, stride * height, RANDMEMSET_MORE_00_AND_FF);

    image = pixman_image_create_bits (
        format, width, height, bytes, stride);

    pixman_image_set_destroy_function (image, free_bits, NULL);

    assert (image);

    if (clone)
    {
        uint32_t *bytes_dup = malloc (stride * height);

        memcpy (bytes_dup, bytes, stride * height);

        *clone = pixman_image_create_bits (
            format, width, height, bytes_dup, stride);

        pixman_image_set_destroy_function (*clone, free_bits, NULL);
    }
    
    return image;
}

static pixman_bool_t
access (pixman_image_t *image, int x, int y, uint32_t *pixel)
{
    int bytes_per_pixel;
    int stride;
    uint8_t *location;

    if (x < 0 || x >= image->bits.width || y < 0 || y >= image->bits.height)
        return FALSE;
    
    bytes_per_pixel = PIXMAN_FORMAT_BPP (image->bits.format) / 8;
    stride = image->bits.rowstride * 4;
    
    location = (uint8_t *)image->bits.bits + y * stride + x * bytes_per_pixel;

    if (bytes_per_pixel == 4)
        *pixel = *(uint32_t *)location;
    else if (bytes_per_pixel == 2)
        *pixel = *(uint16_t *)location;
    else if (bytes_per_pixel == 1)
        *pixel = *(uint8_t *)location;
    else
	assert (0);

    return TRUE;
}

static void
get_color (pixel_checker_t *checker,
	   pixman_image_t *image,
	   int x, int y,
	   color_t *color,
	   uint32_t *pixel)
{
    if (!access (image, x, y, pixel))
    {
	color->a = 0.0;
	color->r = 0.0;
	color->g = 0.0;
	color->b = 0.0;
    }
    else
    {
	pixel_checker_convert_pixel_to_color (
	    checker, *pixel, color);
    }
}
            
static pixman_bool_t
verify (int test_no,
        pixman_op_t op,
        pixman_image_t *source,
	pixman_image_t *mask,
        pixman_image_t *dest,
        pixman_image_t *orig_dest,
        int x, int y,
        int width, int height,
	pixman_bool_t component_alpha,
	pixman_dither_t dither)
{
    pixel_checker_t dest_checker, src_checker, mask_checker;
    int i, j;

    pixel_checker_init (&src_checker, source->bits.format);
    pixel_checker_init (&dest_checker, dest->bits.format);
    pixel_checker_init (&mask_checker, mask->bits.format);

    if (dest->bits.dither != PIXMAN_DITHER_NONE)
	pixel_checker_allow_dither (&dest_checker);

    assert (dest->bits.format == orig_dest->bits.format);

    for (j = y; j < y + height; ++j)
    {
        for (i = x; i < x + width; ++i)
        {
            color_t src_color, mask_color, orig_dest_color, result;
            uint32_t dest_pixel, orig_dest_pixel, src_pixel, mask_pixel;

            access (dest, i, j, &dest_pixel);

	    get_color (&src_checker,
		       source, i - x, j - y,
		       &src_color, &src_pixel);

	    get_color (&mask_checker,
		       mask, i - x, j - y,
		       &mask_color, &mask_pixel);

	    get_color (&dest_checker, 
		       orig_dest, i, j,
		       &orig_dest_color, &orig_dest_pixel);

	    do_composite (op, 
			  &src_color, &mask_color, &orig_dest_color,
			  &result, component_alpha);

            if (!pixel_checker_check (&dest_checker, dest_pixel, &result))
            {
                int a, r, g, b;

                printf ("--------- Test 0x%x failed ---------\n", test_no);
                
                printf ("   operator:         %s (%s alpha)\n", operator_name (op),
			component_alpha? "component" : "unified");
		printf ("   dither:           %s\n", dither_name (dither));
                printf ("   dest_x, dest_y:   %d %d\n", x, y);
                printf ("   width, height:    %d %d\n", width, height);
                printf ("   source:           format: %-14s  size: %2d x %2d\n",
                        format_name (source->bits.format),
			source->bits.width, source->bits.height);
                printf ("   mask:             format: %-14s  size: %2d x %2d\n",
                        format_name (mask->bits.format),
			mask->bits.width, mask->bits.height);
                printf ("   dest:             format: %-14s  size: %2d x %2d\n",
                        format_name (dest->bits.format),
			dest->bits.width, dest->bits.height);
                printf ("   -- Failed pixel: (%d, %d) --\n", i, j);
                printf ("   source ARGB:      %f  %f  %f  %f   (pixel: %x)\n",
                        src_color.a, src_color.r, src_color.g, src_color.b,
                        src_pixel);
                printf ("   mask ARGB:        %f  %f  %f  %f   (pixel: %x)\n",
                        mask_color.a, mask_color.r, mask_color.g, mask_color.b,
                        mask_pixel);
                printf ("   dest ARGB:        %f  %f  %f  %f   (pixel: %x)\n",
                        orig_dest_color.a, orig_dest_color.r, orig_dest_color.g, orig_dest_color.b,
                        orig_dest_pixel);
                printf ("   expected ARGB:    %f  %f  %f  %f\n",
                        result.a, result.r, result.g, result.b);

                pixel_checker_get_min (&dest_checker, &result, &a, &r, &g, &b);
                printf ("   min acceptable:   %8d  %8d  %8d  %8d\n", a, r, g, b);

                pixel_checker_split_pixel (&dest_checker, dest_pixel, &a, &r, &g, &b);
                printf ("   got:              %8d  %8d  %8d  %8d   (pixel: %x)\n", a, r, g, b, dest_pixel);
                
                pixel_checker_get_max (&dest_checker, &result, &a, &r, &g, &b);
                printf ("   max acceptable:   %8d  %8d  %8d  %8d\n", a, r, g, b);
		printf ("\n");
		printf ("    { %s,\n", operator_name (op));
		printf ("      PIXMAN_%s,\t0x%x,\n", format_name (source->bits.format), src_pixel);
		printf ("      PIXMAN_%s,\t0x%x,\n", format_name (mask->bits.format), mask_pixel);
		printf ("      PIXMAN_%s,\t0x%x\n", format_name (dest->bits.format), orig_dest_pixel);
		printf ("    },\n");
                return FALSE;
            }
        }
    }

    return TRUE;
}

static pixman_bool_t
do_check (int i)
{
    pixman_image_t *source, *dest, *mask;
    pixman_op_t op;
    int x, y, width, height;
    pixman_image_t *dest_copy;
    pixman_bool_t result = TRUE;
    pixman_bool_t component_alpha;
    pixman_dither_t dither = PIXMAN_DITHER_NONE;

    prng_srand (i);
    op = RANDOM_ELT (operators);
    x = prng_rand_n (MAX_WIDTH);
    y = prng_rand_n (MAX_HEIGHT);
    width = prng_rand_n (MAX_WIDTH) + 4;
    height = prng_rand_n (MAX_HEIGHT) + 4;

    source = create_image (NULL);
    mask = create_image (NULL);
    dest = create_image (&dest_copy);

    if (x >= dest->bits.width)
        x = dest->bits.width / 2;
    if (y >= dest->bits.height)
        y = dest->bits.height / 2;
    if (x + width > dest->bits.width)
        width = dest->bits.width - x;
    if (y + height > dest->bits.height)
        height = dest->bits.height - y;

    if (prng_rand_n (2))
    {
	dither = RANDOM_ELT (dithers);
	pixman_image_set_dither (dest, dither);
    }

    component_alpha = prng_rand_n (2);

    pixman_image_set_component_alpha (mask, component_alpha);
    
    pixman_image_composite32 (op, source, mask, dest,
                              0, 0, 0, 0,
                              x, y, width, height);

    if (!verify (i, op, source, mask, dest, dest_copy,
		 x, y, width, height, component_alpha,
	         dither))
    {
	result = FALSE;
    }
    
    pixman_image_unref (source);
    pixman_image_unref (mask);
    pixman_image_unref (dest);
    pixman_image_unref (dest_copy);

    return result;
}

#define N_TESTS    10000000

int
main (int argc, const char *argv[])
{
    int i;
    int result = 0;

    if (argc == 2)
    {
	if (strcmp (argv[1], "--forever") == 0)
	{
	    uint32_t n;

	    prng_srand (time (0));

	    n = prng_rand();

	    for (;;)
		do_check (n++);
	}
        else
	{
	    do_check (strtol (argv[1], NULL, 0));
	}
    }
    else
    {
#ifdef USE_OPENMP
#       pragma omp parallel for default(none) reduction(|:result)
#endif
        for (i = 0; i < N_TESTS; ++i)
	{
	    if (!do_check (i))
		result |= 1;
	}
    }
    
    return result;
}
