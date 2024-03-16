/*
 * Test program, which can detect some problems with affine transformations
 * in pixman. Testing is done by running lots of random SRC and OVER
 * compositing operations a8r8g8b8, x8a8r8g8b8, r5g6b5 and a8 color formats
 * with random scaled, rotated and translated transforms.
 *
 * Script 'fuzzer-find-diff.pl' can be used to narrow down the problem in
 * the case of test failure.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#define MAX_SRC_WIDTH  16
#define MAX_SRC_HEIGHT 16
#define MAX_DST_WIDTH  16
#define MAX_DST_HEIGHT 16
#define MAX_STRIDE     4

/*
 * Composite operation with pseudorandom images
 */
uint32_t
test_composite (int      testnum,
		int      verbose)
{
    int                i;
    pixman_image_t *   src_img;
    pixman_image_t *   dst_img;
    pixman_transform_t transform;
    pixman_region16_t  clip;
    int                src_width, src_height;
    int                dst_width, dst_height;
    int                src_stride, dst_stride;
    int                src_x, src_y;
    int                dst_x, dst_y;
    int                src_bpp;
    int                dst_bpp;
    int                w, h;
    pixman_fixed_t     scale_x = 65536, scale_y = 65536;
    pixman_fixed_t     translate_x = 0, translate_y = 0;
    pixman_op_t        op;
    pixman_repeat_t    repeat = PIXMAN_REPEAT_NONE;
    pixman_format_code_t src_fmt, dst_fmt;
    uint32_t *         srcbuf;
    uint32_t *         dstbuf;
    uint32_t           crc32;
    FLOAT_REGS_CORRUPTION_DETECTOR_START ();

    prng_srand (testnum);

    src_bpp = (prng_rand_n (2) == 0) ? 2 : 4;
    dst_bpp = (prng_rand_n (2) == 0) ? 2 : 4;
    op = (prng_rand_n (2) == 0) ? PIXMAN_OP_SRC : PIXMAN_OP_OVER;

    src_width = prng_rand_n (MAX_SRC_WIDTH) + 1;
    src_height = prng_rand_n (MAX_SRC_HEIGHT) + 1;
    dst_width = prng_rand_n (MAX_DST_WIDTH) + 1;
    dst_height = prng_rand_n (MAX_DST_HEIGHT) + 1;
    src_stride = src_width * src_bpp + prng_rand_n (MAX_STRIDE) * src_bpp;
    dst_stride = dst_width * dst_bpp + prng_rand_n (MAX_STRIDE) * dst_bpp;

    if (src_stride & 3)
	src_stride += 2;

    if (dst_stride & 3)
	dst_stride += 2;

    src_x = -(src_width / 4) + prng_rand_n (src_width * 3 / 2);
    src_y = -(src_height / 4) + prng_rand_n (src_height * 3 / 2);
    dst_x = -(dst_width / 4) + prng_rand_n (dst_width * 3 / 2);
    dst_y = -(dst_height / 4) + prng_rand_n (dst_height * 3 / 2);
    w = prng_rand_n (dst_width * 3 / 2 - dst_x);
    h = prng_rand_n (dst_height * 3 / 2 - dst_y);

    srcbuf = (uint32_t *)malloc (src_stride * src_height);
    dstbuf = (uint32_t *)malloc (dst_stride * dst_height);

    prng_randmemset (srcbuf, src_stride * src_height, 0);
    prng_randmemset (dstbuf, dst_stride * dst_height, 0);

    if (prng_rand_n (2) == 0)
    {
	srcbuf += (src_stride / 4) * (src_height - 1);
	src_stride = - src_stride;
    }

    if (prng_rand_n (2) == 0)
    {
	dstbuf += (dst_stride / 4) * (dst_height - 1);
	dst_stride = - dst_stride;
    }
    
    src_fmt = src_bpp == 4 ? (prng_rand_n (2) == 0 ?
                              PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8) : PIXMAN_r5g6b5;

    dst_fmt = dst_bpp == 4 ? (prng_rand_n (2) == 0 ?
                              PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8) : PIXMAN_r5g6b5;

    src_img = pixman_image_create_bits (
        src_fmt, src_width, src_height, srcbuf, src_stride);

    dst_img = pixman_image_create_bits (
        dst_fmt, dst_width, dst_height, dstbuf, dst_stride);

    image_endian_swap (src_img);
    image_endian_swap (dst_img);

    pixman_transform_init_identity (&transform);

    if (prng_rand_n (3) > 0)
    {
	scale_x = -65536 * 3 + prng_rand_n (65536 * 6);
	if (prng_rand_n (2))
	    scale_y = -65536 * 3 + prng_rand_n (65536 * 6);
	else
	    scale_y = scale_x;
	pixman_transform_init_scale (&transform, scale_x, scale_y);
    }
    if (prng_rand_n (3) > 0)
    {
	translate_x = -65536 * 3 + prng_rand_n (6 * 65536);
	if (prng_rand_n (2))
	    translate_y = -65536 * 3 + prng_rand_n (6 * 65536);
	else
	    translate_y = translate_x;
	pixman_transform_translate (&transform, NULL, translate_x, translate_y);
    }

    if (prng_rand_n (4) > 0)
    {
	int c, s, tx = 0, ty = 0;
	switch (prng_rand_n (4))
	{
	case 0:
	    /* 90 degrees */
	    c = 0;
	    s = pixman_fixed_1;
	    tx = pixman_int_to_fixed (MAX_SRC_HEIGHT);
	    break;
	case 1:
	    /* 180 degrees */
	    c = -pixman_fixed_1;
	    s = 0;
	    tx = pixman_int_to_fixed (MAX_SRC_WIDTH);
	    ty = pixman_int_to_fixed (MAX_SRC_HEIGHT);
	    break;
	case 2:
	    /* 270 degrees */
	    c = 0;
	    s = -pixman_fixed_1;
	    ty = pixman_int_to_fixed (MAX_SRC_WIDTH);
	    break;
	default:
	    /* arbitrary rotation */
	    c = prng_rand_n (2 * 65536) - 65536;
	    s = prng_rand_n (2 * 65536) - 65536;
	    break;
	}
	pixman_transform_rotate (&transform, NULL, c, s);
	pixman_transform_translate (&transform, NULL, tx, ty);
    }

    if (prng_rand_n (8) == 0)
    {
	/* Flip random bits */
	int maxflipcount = 8;
	while (maxflipcount--)
	{
	    int i = prng_rand_n (2);
	    int j = prng_rand_n (3);
	    int bitnum = prng_rand_n (32);
	    transform.matrix[i][j] ^= 1U << bitnum;
	    if (prng_rand_n (2))
		break;
	}
    }

    pixman_image_set_transform (src_img, &transform);

    switch (prng_rand_n (4))
    {
    case 0:
	repeat = PIXMAN_REPEAT_NONE;
	break;

    case 1:
	repeat = PIXMAN_REPEAT_NORMAL;
	break;

    case 2:
	repeat = PIXMAN_REPEAT_PAD;
	break;

    case 3:
	repeat = PIXMAN_REPEAT_REFLECT;
	break;

    default:
        break;
    }
    pixman_image_set_repeat (src_img, repeat);

    if (prng_rand_n (2))
	pixman_image_set_filter (src_img, PIXMAN_FILTER_NEAREST, NULL, 0);
    else
	pixman_image_set_filter (src_img, PIXMAN_FILTER_BILINEAR, NULL, 0);

    if (verbose)
    {
#define M(r,c)								\
	transform.matrix[r][c]

	printf ("src_fmt=%s, dst_fmt=%s\n", format_name (src_fmt), format_name (dst_fmt));
	printf ("op=%s, repeat=%d, transform=\n",
	        operator_name (op), repeat);
	printf (" { { { 0x%08x, 0x%08x, 0x%08x },\n"
		"     { 0x%08x, 0x%08x, 0x%08x },\n"
		"     { 0x%08x, 0x%08x, 0x%08x },\n"
		" } };\n",
		M(0,0), M(0,1), M(0,2),
		M(1,0), M(1,1), M(1,2),
		M(2,0), M(2,1), M(2,2));
	printf ("src_width=%d, src_height=%d, dst_width=%d, dst_height=%d\n",
	        src_width, src_height, dst_width, dst_height);
	printf ("src_x=%d, src_y=%d, dst_x=%d, dst_y=%d\n",
	        src_x, src_y, dst_x, dst_y);
	printf ("w=%d, h=%d\n", w, h);
    }

    if (prng_rand_n (8) == 0)
    {
	pixman_box16_t clip_boxes[2];
	int            n = prng_rand_n (2) + 1;

	for (i = 0; i < n; i++)
	{
	    clip_boxes[i].x1 = prng_rand_n (src_width);
	    clip_boxes[i].y1 = prng_rand_n (src_height);
	    clip_boxes[i].x2 =
		clip_boxes[i].x1 + prng_rand_n (src_width - clip_boxes[i].x1);
	    clip_boxes[i].y2 =
		clip_boxes[i].y1 + prng_rand_n (src_height - clip_boxes[i].y1);

	    if (verbose)
	    {
		printf ("source clip box: [%d,%d-%d,%d]\n",
		        clip_boxes[i].x1, clip_boxes[i].y1,
		        clip_boxes[i].x2, clip_boxes[i].y2);
	    }
	}

	pixman_region_init_rects (&clip, clip_boxes, n);
	pixman_image_set_clip_region (src_img, &clip);
	pixman_image_set_source_clipping (src_img, 1);
	pixman_region_fini (&clip);
    }

    if (prng_rand_n (8) == 0)
    {
	pixman_box16_t clip_boxes[2];
	int            n = prng_rand_n (2) + 1;
	for (i = 0; i < n; i++)
	{
	    clip_boxes[i].x1 = prng_rand_n (dst_width);
	    clip_boxes[i].y1 = prng_rand_n (dst_height);
	    clip_boxes[i].x2 =
		clip_boxes[i].x1 + prng_rand_n (dst_width - clip_boxes[i].x1);
	    clip_boxes[i].y2 =
		clip_boxes[i].y1 + prng_rand_n (dst_height - clip_boxes[i].y1);

	    if (verbose)
	    {
		printf ("destination clip box: [%d,%d-%d,%d]\n",
		        clip_boxes[i].x1, clip_boxes[i].y1,
		        clip_boxes[i].x2, clip_boxes[i].y2);
	    }
	}
	pixman_region_init_rects (&clip, clip_boxes, n);
	pixman_image_set_clip_region (dst_img, &clip);
	pixman_region_fini (&clip);
    }

    pixman_image_composite (op, src_img, NULL, dst_img,
                            src_x, src_y, 0, 0, dst_x, dst_y, w, h);

    crc32 = compute_crc32_for_image (0, dst_img);
    
    if (verbose)
	print_image (dst_img);

    pixman_image_unref (src_img);
    pixman_image_unref (dst_img);

    if (src_stride < 0)
	srcbuf += (src_stride / 4) * (src_height - 1);

    if (dst_stride < 0)
	dstbuf += (dst_stride / 4) * (dst_height - 1);
    
    free (srcbuf);
    free (dstbuf);

    FLOAT_REGS_CORRUPTION_DETECTOR_FINISH ();
    return crc32;
}

#if BILINEAR_INTERPOLATION_BITS == 7
#define CHECKSUM 0xBE724CFE
#elif BILINEAR_INTERPOLATION_BITS == 4
#define CHECKSUM 0x79BBE501
#else
#define CHECKSUM 0x00000000
#endif

int
main (int argc, const char *argv[])
{
    pixman_disable_out_of_bounds_workaround ();

    return fuzzer_test_main ("affine", 8000000, CHECKSUM,
			     test_composite, argc, argv);
}
