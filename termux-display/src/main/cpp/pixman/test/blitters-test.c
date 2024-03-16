/*
 * Test program, which stresses the use of different color formats and
 * compositing operations.
 *
 * Script 'fuzzer-find-diff.pl' can be used to narrow down the problem in
 * the case of test failure.
 */
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

static pixman_indexed_t rgb_palette[9];
static pixman_indexed_t y_palette[9];

/* The first eight format in the list are by far the most widely
 * used formats, so we test those more than the others
 */
#define N_MOST_LIKELY_FORMATS 8

/* Create random image for testing purposes */
static pixman_image_t *
create_random_image (pixman_format_code_t *allowed_formats,
		     int                   max_width,
		     int                   max_height,
		     int                   max_extra_stride,
		     pixman_format_code_t *used_fmt)
{
    int n = 0, width, height, stride;
    pixman_format_code_t fmt;
    uint32_t *buf;
    pixman_image_t *img;

    while (allowed_formats[n] != PIXMAN_null)
	n++;

    if (n > N_MOST_LIKELY_FORMATS && prng_rand_n (4) != 0)
	n = N_MOST_LIKELY_FORMATS;
    fmt = allowed_formats[prng_rand_n (n)];

    width = prng_rand_n (max_width) + 1;
    height = prng_rand_n (max_height) + 1;
    stride = (width * PIXMAN_FORMAT_BPP (fmt) + 7) / 8 +
	prng_rand_n (max_extra_stride + 1);
    stride = (stride + 3) & ~3;

    /* do the allocation */
    buf = aligned_malloc (64, stride * height);

    if (prng_rand_n (4) == 0)
    {
	/* uniform distribution */
	prng_randmemset (buf, stride * height, 0);
    }
    else
    {
	/* significantly increased probability for 0x00 and 0xFF */
	prng_randmemset (buf, stride * height, RANDMEMSET_MORE_00_AND_FF);
    }

    /* test negative stride */
    if (prng_rand_n (4) == 0)
    {
	buf += (stride / 4) * (height - 1);
	stride = - stride;
    }
    
    img = pixman_image_create_bits (fmt, width, height, buf, stride);

    if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_COLOR)
    {
	pixman_image_set_indexed (img, &(rgb_palette[PIXMAN_FORMAT_BPP (fmt)]));
    }
    else if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_GRAY)
    {
	pixman_image_set_indexed (img, &(y_palette[PIXMAN_FORMAT_BPP (fmt)]));
    }

    if (prng_rand_n (16) == 0)
	pixman_image_set_filter (img, PIXMAN_FILTER_BILINEAR, NULL, 0);

    image_endian_swap (img);

    if (used_fmt) *used_fmt = fmt;
    return img;
}

/* Free random image, and optionally update crc32 based on its data */
static uint32_t
free_random_image (uint32_t initcrc,
		   pixman_image_t *img,
		   pixman_format_code_t fmt)
{
    uint32_t crc32 = 0;
    uint32_t *data = pixman_image_get_data (img);

    if (fmt != PIXMAN_null)
	crc32 = compute_crc32_for_image (initcrc, img);

    if (img->bits.rowstride < 0)
	data += img->bits.rowstride * (img->bits.height - 1);

    pixman_image_unref (img);
    free (data);

    return crc32;
}

static pixman_op_t op_list[] = {
    PIXMAN_OP_SRC,
    PIXMAN_OP_OVER,
    PIXMAN_OP_ADD,
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
    PIXMAN_OP_MULTIPLY,
    PIXMAN_OP_SCREEN,
    PIXMAN_OP_OVERLAY,
    PIXMAN_OP_DARKEN,
    PIXMAN_OP_LIGHTEN,
    PIXMAN_OP_HARD_LIGHT,
    PIXMAN_OP_DIFFERENCE,
    PIXMAN_OP_EXCLUSION,
#if 0 /* these use floating point math and are not always bitexact on different platforms */
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
    PIXMAN_OP_COLOR_DODGE,
    PIXMAN_OP_COLOR_BURN,
    PIXMAN_OP_SOFT_LIGHT,
    PIXMAN_OP_HSL_HUE,
    PIXMAN_OP_HSL_SATURATION,
    PIXMAN_OP_HSL_COLOR,
    PIXMAN_OP_HSL_LUMINOSITY,
#endif
};

static pixman_format_code_t img_fmt_list[] = {
    PIXMAN_a8r8g8b8,
    PIXMAN_a8b8g8r8,
    PIXMAN_x8r8g8b8,
    PIXMAN_x8b8g8r8,
    PIXMAN_r5g6b5,
    PIXMAN_b5g6r5,
    PIXMAN_a8,
    PIXMAN_a1,
    PIXMAN_r3g3b2,
    PIXMAN_b8g8r8a8,
    PIXMAN_b8g8r8x8,
    PIXMAN_r8g8b8a8,
    PIXMAN_r8g8b8x8,
    PIXMAN_x14r6g6b6,
    PIXMAN_r8g8b8,
    PIXMAN_b8g8r8,
#if 0 /* These are going to use floating point in the near future */
    PIXMAN_x2r10g10b10,
    PIXMAN_a2r10g10b10,
    PIXMAN_x2b10g10r10,
    PIXMAN_a2b10g10r10,
#endif
    PIXMAN_a1r5g5b5,
    PIXMAN_x1r5g5b5,
    PIXMAN_a1b5g5r5,
    PIXMAN_x1b5g5r5,
    PIXMAN_a4r4g4b4,
    PIXMAN_x4r4g4b4,
    PIXMAN_a4b4g4r4,
    PIXMAN_x4b4g4r4,
    PIXMAN_r3g3b2,
    PIXMAN_b2g3r3,
    PIXMAN_a2r2g2b2,
    PIXMAN_a2b2g2r2,
    PIXMAN_c8,
    PIXMAN_g8,
    PIXMAN_x4c4,
    PIXMAN_x4g4,
    PIXMAN_c4,
    PIXMAN_g4,
    PIXMAN_g1,
    PIXMAN_x4a4,
    PIXMAN_a4,
    PIXMAN_r1g2b1,
    PIXMAN_b1g2r1,
    PIXMAN_a1r1g1b1,
    PIXMAN_a1b1g1r1,
    PIXMAN_null
};

static pixman_format_code_t mask_fmt_list[] = {
    PIXMAN_a8r8g8b8,
    PIXMAN_a8,
    PIXMAN_a4,
    PIXMAN_a1,
    PIXMAN_null
};


/*
 * Composite operation with pseudorandom images
 */
uint32_t
test_composite (int testnum, int verbose)
{
    pixman_image_t *src_img = NULL;
    pixman_image_t *dst_img = NULL;
    pixman_image_t *mask_img = NULL;
    int src_width, src_height;
    int dst_width, dst_height;
    int src_stride, dst_stride;
    int src_x, src_y;
    int dst_x, dst_y;
    int mask_x, mask_y;
    int w, h;
    pixman_op_t op;
    pixman_format_code_t src_fmt, dst_fmt, mask_fmt;
    uint32_t *srcbuf, *maskbuf;
    uint32_t crc32;
    int max_width, max_height, max_extra_stride;
    FLOAT_REGS_CORRUPTION_DETECTOR_START ();

    max_width = max_height = 24 + testnum / 10000;
    max_extra_stride = 4 + testnum / 1000000;

    if (max_width > 256)
	max_width = 256;

    if (max_height > 16)
	max_height = 16;

    if (max_extra_stride > 8)
	max_extra_stride = 8;

    prng_srand (testnum);

    op = op_list[prng_rand_n (ARRAY_LENGTH (op_list))];

    if (prng_rand_n (8))
    {
	/* normal image */
	src_img = create_random_image (img_fmt_list, max_width, max_height,
				       max_extra_stride, &src_fmt);
    }
    else
    {
	/* solid case */
	src_img = create_random_image (img_fmt_list, 1, 1,
				       max_extra_stride, &src_fmt);

	pixman_image_set_repeat (src_img, PIXMAN_REPEAT_NORMAL);
    }

    dst_img = create_random_image (img_fmt_list, max_width, max_height,
				   max_extra_stride, &dst_fmt);

    src_width = pixman_image_get_width (src_img);
    src_height = pixman_image_get_height (src_img);
    src_stride = pixman_image_get_stride (src_img);

    dst_width = pixman_image_get_width (dst_img);
    dst_height = pixman_image_get_height (dst_img);
    dst_stride = pixman_image_get_stride (dst_img);

    srcbuf = pixman_image_get_data (src_img);

    src_x = prng_rand_n (src_width);
    src_y = prng_rand_n (src_height);
    dst_x = prng_rand_n (dst_width);
    dst_y = prng_rand_n (dst_height);

    mask_img = NULL;
    mask_fmt = PIXMAN_null;
    mask_x = 0;
    mask_y = 0;
    maskbuf = NULL;

    if ((src_fmt == PIXMAN_x8r8g8b8 || src_fmt == PIXMAN_x8b8g8r8) &&
	(prng_rand_n (4) == 0))
    {
	/* PIXBUF */
	mask_fmt = prng_rand_n (2) ? PIXMAN_a8r8g8b8 : PIXMAN_a8b8g8r8;
	mask_img = pixman_image_create_bits (mask_fmt,
	                                     src_width,
	                                     src_height,
	                                     srcbuf,
	                                     src_stride);
	mask_x = src_x;
	mask_y = src_y;
	maskbuf = srcbuf;
    }
    else if (prng_rand_n (2))
    {
	if (prng_rand_n (2))
	{
	    mask_img = create_random_image (mask_fmt_list, max_width, max_height,
					   max_extra_stride, &mask_fmt);
	}
	else
	{
	    /* solid case */
	    mask_img = create_random_image (mask_fmt_list, 1, 1,
					   max_extra_stride, &mask_fmt);
	    pixman_image_set_repeat (mask_img, PIXMAN_REPEAT_NORMAL);
	}

	if (prng_rand_n (2))
	    pixman_image_set_component_alpha (mask_img, 1);

	mask_x = prng_rand_n (pixman_image_get_width (mask_img));
	mask_y = prng_rand_n (pixman_image_get_height (mask_img));
    }


    w = prng_rand_n (dst_width - dst_x + 1);
    h = prng_rand_n (dst_height - dst_y + 1);

    if (verbose)
    {
        printf ("op=%s\n", operator_name (op));
	printf ("src_fmt=%s, dst_fmt=%s, mask_fmt=%s\n",
	    format_name (src_fmt), format_name (dst_fmt),
	    format_name (mask_fmt));
	printf ("src_width=%d, src_height=%d, dst_width=%d, dst_height=%d\n",
	    src_width, src_height, dst_width, dst_height);
	printf ("src_x=%d, src_y=%d, dst_x=%d, dst_y=%d\n",
	    src_x, src_y, dst_x, dst_y);
	printf ("src_stride=%d, dst_stride=%d\n",
	    src_stride, dst_stride);
	printf ("w=%d, h=%d\n", w, h);
    }

    pixman_image_composite (op, src_img, mask_img, dst_img,
			    src_x, src_y, mask_x, mask_y, dst_x, dst_y, w, h);

    if (verbose)
	print_image (dst_img);

    free_random_image (0, src_img, PIXMAN_null);
    crc32 = free_random_image (0, dst_img, dst_fmt);

    if (mask_img)
    {
	if (srcbuf == maskbuf)
	    pixman_image_unref(mask_img);
	else
	    free_random_image (0, mask_img, PIXMAN_null);
    }

    FLOAT_REGS_CORRUPTION_DETECTOR_FINISH ();
    return crc32;
}

int
main (int argc, const char *argv[])
{
    int i;

    prng_srand (0);

    for (i = 1; i <= 8; i++)
    {
	initialize_palette (&(rgb_palette[i]), i, TRUE);
	initialize_palette (&(y_palette[i]), i, FALSE);
    }

    return fuzzer_test_main("blitters", 2000000,
			    0xCC21DDF0,
			    test_composite, argc, argv);
}
