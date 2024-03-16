/*
 * Copyright © 2005 Eric Anholt
 * Copyright © 2009 Chris Wilson
 * Copyright © 2010 Soeren Sandmann
 * Copyright © 2010 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h> /* abort() */
#include <math.h>
#include <time.h>
#include "utils.h"

typedef struct image_t image_t;

static const color_t colors[] =
{
    { 1.0, 1.0, 1.0, 1.0 },
    { 1.0, 1.0, 1.0, 0.0 },
    { 0.0, 0.0, 0.0, 1.0 },
    { 0.0, 0.0, 0.0, 0.0 },
    { 1.0, 0.0, 0.0, 1.0 },
    { 0.0, 1.0, 0.0, 1.0 },
    { 0.0, 0.0, 1.0, 1.0 },
    { 0.5, 0.0, 0.0, 0.5 },
};

static uint16_t
_color_double_to_short (double d)
{
    uint32_t i;

    i = (uint32_t) (d * 65536);
    i -= (i >> 16);

    return i;
}

static void
compute_pixman_color (const color_t *color,
		      pixman_color_t *out)
{
    out->red   = _color_double_to_short (color->r);
    out->green = _color_double_to_short (color->g);
    out->blue  = _color_double_to_short (color->b);
    out->alpha = _color_double_to_short (color->a);
}

#define REPEAT 0x01000000
#define FLAGS  0xff000000

static const int sizes[] =
{
    0,
    1,
    1 | REPEAT,
    10
};

static const pixman_format_code_t formats[] =
{
    /* 32 bpp formats */
    PIXMAN_a8r8g8b8,
    PIXMAN_x8r8g8b8,
    PIXMAN_a8b8g8r8,
    PIXMAN_x8b8g8r8,
    PIXMAN_b8g8r8a8,
    PIXMAN_b8g8r8x8,
    PIXMAN_r8g8b8a8,
    PIXMAN_r8g8b8x8,
    PIXMAN_x2r10g10b10,
    PIXMAN_x2b10g10r10,
    PIXMAN_a2r10g10b10,
    PIXMAN_a2b10g10r10,
    
    /* sRGB formats */
    PIXMAN_a8r8g8b8_sRGB,

    /* 24 bpp formats */
    PIXMAN_r8g8b8,
    PIXMAN_b8g8r8,
    PIXMAN_r5g6b5,
    PIXMAN_b5g6r5,

    /* 16 bpp formats */
    PIXMAN_x1r5g5b5,
    PIXMAN_x1b5g5r5,
    PIXMAN_a1r5g5b5,
    PIXMAN_a1b5g5r5,
    PIXMAN_a4b4g4r4,
    PIXMAN_x4b4g4r4,
    PIXMAN_a4r4g4b4,
    PIXMAN_x4r4g4b4,

    /* 8 bpp formats */
    PIXMAN_a8,
    PIXMAN_r3g3b2,
    PIXMAN_b2g3r3,
    PIXMAN_a2r2g2b2,
    PIXMAN_a2b2g2r2,
    PIXMAN_x4a4,

    /* 4 bpp formats */
    PIXMAN_a4,
    PIXMAN_r1g2b1,
    PIXMAN_b1g2r1,
    PIXMAN_a1r1g1b1,
    PIXMAN_a1b1g1r1,

    /* 1 bpp formats */
    PIXMAN_a1,
};

struct image_t
{
    pixman_image_t *image;
    pixman_format_code_t format;
    const color_t *color;
    pixman_repeat_t repeat;
    int size;
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
};

static uint32_t
get_value (pixman_image_t *image)
{
    uint32_t value = *(uint32_t *)pixman_image_get_data (image);

#ifdef WORDS_BIGENDIAN
    {
	pixman_format_code_t format = pixman_image_get_format (image);
	value >>= 8 * sizeof(value) - PIXMAN_FORMAT_BPP (format);
    }
#endif

    return value;
}

static char *
describe_image (image_t *info, char *buf)
{
    if (info->size)
    {
	sprintf (buf, "%s, %dx%d%s",
		 format_name (info->format),
		 info->size, info->size,
		 info->repeat ? " R" :"");
    }
    else
    {
	sprintf (buf, "solid");
    }

    return buf;
}

static char *
describe_color (const color_t *color, char *buf)
{
    sprintf (buf, "%.3f %.3f %.3f %.3f",
	     color->r, color->g, color->b, color->a);

    return buf;
}

static pixman_bool_t
composite_test (image_t *dst,
		pixman_op_t op,
		image_t *src,
		image_t *mask,
		pixman_bool_t component_alpha,
		int testno)
{
    color_t expected, tdst, tsrc, tmsk;
    pixel_checker_t checker;

    if (mask)
    {
	pixman_image_set_component_alpha (mask->image, component_alpha);

	pixman_image_composite (op, src->image, mask->image, dst->image,
				0, 0, 0, 0, 0, 0, dst->size, dst->size);
    }
    else
    {
	pixman_image_composite (op, src->image, NULL, dst->image,
				0, 0,
				0, 0,
				0, 0,
				dst->size, dst->size);
    }

    tdst = *dst->color;
    tsrc = *src->color;

    if (mask)
    {
	tmsk = *mask->color;
    }

    /* It turns out that by construction all source, mask etc. colors are
     * linear because they are made from fills, and fills are always in linear
     * color space.  However, if they have been converted to bitmaps, we need
     * to simulate the sRGB approximation to pass the test cases.
     */
    if (src->size)
    {
	if (PIXMAN_FORMAT_TYPE (src->format) == PIXMAN_TYPE_ARGB_SRGB)
        {
	    tsrc.r = convert_linear_to_srgb (tsrc.r);
	    tsrc.g = convert_linear_to_srgb (tsrc.g);
	    tsrc.b = convert_linear_to_srgb (tsrc.b);
	    round_color (src->format, &tsrc);
	    tsrc.r = convert_srgb_to_linear (tsrc.r);
	    tsrc.g = convert_srgb_to_linear (tsrc.g);
	    tsrc.b = convert_srgb_to_linear (tsrc.b);
	}
        else
        {
	    round_color (src->format, &tsrc);
	}
    }

    if (mask && mask->size)
    {
	if (PIXMAN_FORMAT_TYPE (mask->format) == PIXMAN_TYPE_ARGB_SRGB)
	{
	    tmsk.r = convert_linear_to_srgb (tmsk.r);
	    tmsk.g = convert_linear_to_srgb (tmsk.g);
	    tmsk.b = convert_linear_to_srgb (tmsk.b);
	    round_color (mask->format, &tmsk);
	    tmsk.r = convert_srgb_to_linear (tmsk.r);
	    tmsk.g = convert_srgb_to_linear (tmsk.g);
	    tmsk.b = convert_srgb_to_linear (tmsk.b);
	}
	else
	{
	    round_color (mask->format, &tmsk);
	}
    }

    if (PIXMAN_FORMAT_TYPE (dst->format) == PIXMAN_TYPE_ARGB_SRGB)
    {
	tdst.r = convert_linear_to_srgb (tdst.r);
	tdst.g = convert_linear_to_srgb (tdst.g);
	tdst.b = convert_linear_to_srgb (tdst.b);
    	round_color (dst->format, &tdst);
	tdst.r = convert_srgb_to_linear (tdst.r);
	tdst.g = convert_srgb_to_linear (tdst.g);
	tdst.b = convert_srgb_to_linear (tdst.b);
    }
    else
    {
    	round_color (dst->format, &tdst);
    }

    do_composite (op,
		  &tsrc,
		  mask? &tmsk : NULL,
		  &tdst,
		  &expected,
		  component_alpha);

    pixel_checker_init (&checker, dst->format);

    if (!pixel_checker_check (&checker, get_value (dst->image), &expected))
    {
	char buf[40], buf2[40];
	int a, r, g, b;
	uint32_t pixel;

	printf ("---- Test %d failed ----\n", testno);
	printf ("Operator:      %s %s\n",
                operator_name (op), component_alpha ? "CA" : "");

	printf ("Source:        %s\n", describe_image (src, buf));
	if (mask != NULL)
	    printf ("Mask:          %s\n", describe_image (mask, buf));

	printf ("Destination:   %s\n\n", describe_image (dst, buf));
	printf ("               R     G     B     A         Rounded\n");
	printf ("Source color:  %s     %s\n",
		describe_color (src->color, buf),
		describe_color (&tsrc, buf2));
	if (mask)
	{
	    printf ("Mask color:    %s     %s\n",
		    describe_color (mask->color, buf),
		    describe_color (&tmsk, buf2));
	}
	printf ("Dest. color:   %s     %s\n",
		describe_color (dst->color, buf),
		describe_color (&tdst, buf2));

	pixel = get_value (dst->image);

	printf ("Expected:      %s\n", describe_color (&expected, buf));

	pixel_checker_split_pixel (&checker, pixel, &a, &r, &g, &b);

	printf ("Got:           %5d %5d %5d %5d  [pixel: 0x%08x]\n", r, g, b, a, pixel);
	pixel_checker_get_min (&checker, &expected, &a, &r, &g, &b);
	printf ("Min accepted:  %5d %5d %5d %5d\n", r, g, b, a);
	pixel_checker_get_max (&checker, &expected, &a, &r, &g, &b);
	printf ("Max accepted:  %5d %5d %5d %5d\n", r, g, b, a);

	return FALSE;
    }
    return TRUE;
}

static void
image_init (image_t *info,
	    int color,
	    int format,
	    int size)
{
    pixman_color_t fill;

    info->color = &colors[color];
    compute_pixman_color (info->color, &fill);

    info->format = formats[format];
    info->size = sizes[size] & ~FLAGS;
    info->repeat = PIXMAN_REPEAT_NONE;

    if (info->size)
    {
	pixman_image_t *solid;

	info->image = pixman_image_create_bits (info->format,
						info->size, info->size,
						NULL, 0);

	solid = pixman_image_create_solid_fill (&fill);
	pixman_image_composite32 (PIXMAN_OP_SRC, solid, NULL, info->image,
				  0, 0, 0, 0, 0, 0, info->size, info->size);
	pixman_image_unref (solid);

	if (sizes[size] & REPEAT)
	{
	    pixman_image_set_repeat (info->image, PIXMAN_REPEAT_NORMAL);
	    info->repeat = PIXMAN_REPEAT_NORMAL;
	}
    }
    else
    {
	info->image = pixman_image_create_solid_fill (&fill);
    }
}

static void
image_fini (image_t *info)
{
    pixman_image_unref (info->image);
}

static int
random_size (void)
{
    return prng_rand_n (ARRAY_LENGTH (sizes));
}

static int
random_color (void)
{
    return prng_rand_n (ARRAY_LENGTH (colors));
}

static int
random_format (void)
{
    return prng_rand_n (ARRAY_LENGTH (formats));
}

static pixman_bool_t
run_test (uint32_t seed)
{
    image_t src, mask, dst;
    pixman_op_t op;
    int ca;
    int ok;

    prng_srand (seed);

    image_init (&dst, random_color(), random_format(), 1);
    image_init (&src, random_color(), random_format(), random_size());
    image_init (&mask, random_color(), random_format(), random_size());

    op = operators [prng_rand_n (ARRAY_LENGTH (operators))];

    ca = prng_rand_n (3);

    switch (ca)
    {
    case 0:
	ok = composite_test (&dst, op, &src, NULL, FALSE, seed);
	break;
    case 1:
	ok = composite_test (&dst, op, &src, &mask, FALSE, seed);
	break;
    case 2:
	ok = composite_test (&dst, op, &src, &mask,
			     mask.size? TRUE : FALSE, seed);
	break;
    default:
	ok = FALSE;
	break;
    }

    image_fini (&src);
    image_fini (&mask);
    image_fini (&dst);

    return ok;
}

int
main (int argc, char **argv)
{
#define N_TESTS (8 * 1024 * 1024)
    int result = 0;
    uint32_t seed;
    int32_t i;

    if (argc > 1)
    {
	char *end;

	i = strtol (argv[1], &end, 0);

	if (end != argv[1])
	{
	    if (!run_test (i))
		return 1;
	    else
		return 0;
	}
	else
	{
	    printf ("Usage:\n\n   %s <number>\n\n", argv[0]);
	    return -1;
	}
    }

    if (getenv ("PIXMAN_RANDOMIZE_TESTS"))
	seed = get_random_seed();
    else
	seed = 1;

#ifdef USE_OPENMP
#   pragma omp parallel for default(none) shared(result, argv, seed)
#endif
    for (i = 0; i <= N_TESTS; ++i)
    {
	if (!result && !run_test (i + seed))
	{
	    printf ("Test 0x%08X failed.\n", seed + i);

	    result = seed + i;
	}
    }

    return result;
}
