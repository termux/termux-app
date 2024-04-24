/*
 * Test program, which can detect some problems with nearest neighbour
 * and bilinear scaling in pixman. Testing is done by running lots
 * of random SRC and OVER compositing operations a8r8g8b8, x8a8r8g8b8
 * and r5g6b5 color formats.
 *
 * Script 'fuzzer-find-diff.pl' can be used to narrow down the problem in
 * the case of test failure.
 */
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#define MAX_SRC_WIDTH  48
#define MAX_SRC_HEIGHT 8
#define MAX_DST_WIDTH  48
#define MAX_DST_HEIGHT 8
#define MAX_STRIDE     4

/*
 * Composite operation with pseudorandom images
 */

static pixman_format_code_t
get_format (int bpp)
{
    if (bpp == 4)
    {
	switch (prng_rand_n (4))
	{
	default:
	case 0:
	    return PIXMAN_a8r8g8b8;
	case 1:
	    return PIXMAN_x8r8g8b8;
	case 2:
	    return PIXMAN_a8b8g8r8;
	case 3:
	    return PIXMAN_x8b8g8r8;
	}
    }
    else
    {
	return PIXMAN_r5g6b5;
    }
}

uint32_t
test_composite (int      testnum,
		int      verbose)
{
    int                i;
    pixman_image_t *   src_img;
    pixman_image_t *   mask_img;
    pixman_image_t *   dst_img;
    pixman_transform_t transform;
    pixman_region16_t  clip;
    int                src_width, src_height;
    int                mask_width, mask_height;
    int                dst_width, dst_height;
    int                src_stride, mask_stride, dst_stride;
    int                src_x, src_y;
    int                mask_x, mask_y;
    int                dst_x, dst_y;
    int                src_bpp;
    int                mask_bpp = 1;
    int                dst_bpp;
    int                w, h;
    pixman_fixed_t     scale_x = 65536, scale_y = 65536;
    pixman_fixed_t     translate_x = 0, translate_y = 0;
    pixman_fixed_t     mask_scale_x = 65536, mask_scale_y = 65536;
    pixman_fixed_t     mask_translate_x = 0, mask_translate_y = 0;
    pixman_op_t        op;
    pixman_repeat_t    repeat = PIXMAN_REPEAT_NONE;
    pixman_repeat_t    mask_repeat = PIXMAN_REPEAT_NONE;
    pixman_format_code_t src_fmt, mask_fmt, dst_fmt;
    uint32_t *         srcbuf;
    uint32_t *         dstbuf;
    uint32_t *         maskbuf;
    uint32_t           crc32;
    FLOAT_REGS_CORRUPTION_DETECTOR_START ();

    prng_srand (testnum);

    src_bpp = (prng_rand_n (2) == 0) ? 2 : 4;
    dst_bpp = (prng_rand_n (2) == 0) ? 2 : 4;
    switch (prng_rand_n (3))
    {
    case 0:
	op = PIXMAN_OP_SRC;
	break;
    case 1:
	op = PIXMAN_OP_OVER;
	break;
    default:
	op = PIXMAN_OP_ADD;
	break;
    }

    src_width = prng_rand_n (MAX_SRC_WIDTH) + 1;
    src_height = prng_rand_n (MAX_SRC_HEIGHT) + 1;

    if (prng_rand_n (2))
    {
	mask_width = prng_rand_n (MAX_SRC_WIDTH) + 1;
	mask_height = prng_rand_n (MAX_SRC_HEIGHT) + 1;
    }
    else
    {
	mask_width = mask_height = 1;
    }

    dst_width = prng_rand_n (MAX_DST_WIDTH) + 1;
    dst_height = prng_rand_n (MAX_DST_HEIGHT) + 1;
    src_stride = src_width * src_bpp + prng_rand_n (MAX_STRIDE) * src_bpp;
    mask_stride = mask_width * mask_bpp + prng_rand_n (MAX_STRIDE) * mask_bpp;
    dst_stride = dst_width * dst_bpp + prng_rand_n (MAX_STRIDE) * dst_bpp;

    if (src_stride & 3)
	src_stride += 2;

    if (mask_stride & 1)
	mask_stride += 1;
    if (mask_stride & 2)
	mask_stride += 2;

    if (dst_stride & 3)
	dst_stride += 2;

    src_x = -(src_width / 4) + prng_rand_n (src_width * 3 / 2);
    src_y = -(src_height / 4) + prng_rand_n (src_height * 3 / 2);
    mask_x = -(mask_width / 4) + prng_rand_n (mask_width * 3 / 2);
    mask_y = -(mask_height / 4) + prng_rand_n (mask_height * 3 / 2);
    dst_x = -(dst_width / 4) + prng_rand_n (dst_width * 3 / 2);
    dst_y = -(dst_height / 4) + prng_rand_n (dst_height * 3 / 2);
    w = prng_rand_n (dst_width * 3 / 2 - dst_x);
    h = prng_rand_n (dst_height * 3 / 2 - dst_y);

    srcbuf = (uint32_t *)malloc (src_stride * src_height);
    maskbuf = (uint32_t *)malloc (mask_stride * mask_height);
    dstbuf = (uint32_t *)malloc (dst_stride * dst_height);

    prng_randmemset (srcbuf, src_stride * src_height, 0);
    prng_randmemset (maskbuf, mask_stride * mask_height, 0);
    prng_randmemset (dstbuf, dst_stride * dst_height, 0);

    src_fmt = get_format (src_bpp);
    mask_fmt = PIXMAN_a8;
    dst_fmt = get_format (dst_bpp);

    if (prng_rand_n (2))
    {
	srcbuf += (src_stride / 4) * (src_height - 1);
	src_stride = - src_stride;
    }

    if (prng_rand_n (2))
    {
	maskbuf += (mask_stride / 4) * (mask_height - 1);
	mask_stride = - mask_stride;
    }

    if (prng_rand_n (2))
    {
	dstbuf += (dst_stride / 4) * (dst_height - 1);
	dst_stride = - dst_stride;
    }

    src_img = pixman_image_create_bits (
        src_fmt, src_width, src_height, srcbuf, src_stride);

    mask_img = pixman_image_create_bits (
        mask_fmt, mask_width, mask_height, maskbuf, mask_stride);

    dst_img = pixman_image_create_bits (
        dst_fmt, dst_width, dst_height, dstbuf, dst_stride);

    image_endian_swap (src_img);
    image_endian_swap (dst_img);

    if (prng_rand_n (4) > 0)
    {
	scale_x = -32768 * 3 + prng_rand_n (65536 * 5);
	scale_y = -32768 * 3 + prng_rand_n (65536 * 5);
	translate_x = prng_rand_n (65536);
	translate_y = prng_rand_n (65536);
	pixman_transform_init_scale (&transform, scale_x, scale_y);
	pixman_transform_translate (&transform, NULL, translate_x, translate_y);
	pixman_image_set_transform (src_img, &transform);
    }

    if (prng_rand_n (2) > 0)
    {
	mask_scale_x = -32768 * 3 + prng_rand_n (65536 * 5);
	mask_scale_y = -32768 * 3 + prng_rand_n (65536 * 5);
	mask_translate_x = prng_rand_n (65536);
	mask_translate_y = prng_rand_n (65536);
	pixman_transform_init_scale (&transform, mask_scale_x, mask_scale_y);
	pixman_transform_translate (&transform, NULL, mask_translate_x, mask_translate_y);
	pixman_image_set_transform (mask_img, &transform);
    }

    switch (prng_rand_n (4))
    {
    case 0:
	mask_repeat = PIXMAN_REPEAT_NONE;
	break;

    case 1:
	mask_repeat = PIXMAN_REPEAT_NORMAL;
	break;

    case 2:
	mask_repeat = PIXMAN_REPEAT_PAD;
	break;

    case 3:
	mask_repeat = PIXMAN_REPEAT_REFLECT;
	break;

    default:
        break;
    }
    pixman_image_set_repeat (mask_img, mask_repeat);

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

    if (prng_rand_n (2))
	pixman_image_set_filter (mask_img, PIXMAN_FILTER_NEAREST, NULL, 0);
    else
	pixman_image_set_filter (mask_img, PIXMAN_FILTER_BILINEAR, NULL, 0);

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
	    clip_boxes[i].x1 = prng_rand_n (mask_width);
	    clip_boxes[i].y1 = prng_rand_n (mask_height);
	    clip_boxes[i].x2 =
		clip_boxes[i].x1 + prng_rand_n (mask_width - clip_boxes[i].x1);
	    clip_boxes[i].y2 =
		clip_boxes[i].y1 + prng_rand_n (mask_height - clip_boxes[i].y1);

	    if (verbose)
	    {
		printf ("mask clip box: [%d,%d-%d,%d]\n",
		        clip_boxes[i].x1, clip_boxes[i].y1,
		        clip_boxes[i].x2, clip_boxes[i].y2);
	    }
	}

	pixman_region_init_rects (&clip, clip_boxes, n);
	pixman_image_set_clip_region (mask_img, &clip);
	pixman_image_set_source_clipping (mask_img, 1);
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

    if (prng_rand_n (2) == 0)
    {
	mask_fmt = PIXMAN_null;
	pixman_image_unref (mask_img);
	mask_img = NULL;
	mask_x = 0;
	mask_y = 0;
    }

    if (verbose)
    {
	printf ("op=%s, src_fmt=%s, mask_fmt=%s, dst_fmt=%s\n",
	        operator_name (op), format_name (src_fmt),
	        format_name (mask_fmt), format_name (dst_fmt));
	printf ("scale_x=%d, scale_y=%d, repeat=%d, filter=%d\n",
	        scale_x, scale_y, repeat, src_img->common.filter);
	printf ("translate_x=%d, translate_y=%d\n",
	        translate_x, translate_y);
	if (mask_fmt != PIXMAN_null)
	{
	    printf ("mask_scale_x=%d, mask_scale_y=%d, "
	            "mask_repeat=%d, mask_filter=%d\n",
	            mask_scale_x, mask_scale_y, mask_repeat,
	            mask_img->common.filter);
	    printf ("mask_translate_x=%d, mask_translate_y=%d\n",
	            mask_translate_x, mask_translate_y);
	}
	printf ("src_width=%d, src_height=%d, src_x=%d, src_y=%d\n",
	        src_width, src_height, src_x, src_y);
	if (mask_fmt != PIXMAN_null)
	{
	    printf ("mask_width=%d, mask_height=%d, mask_x=%d, mask_y=%d\n",
	            mask_width, mask_height, mask_x, mask_y);
	}
	printf ("dst_width=%d, dst_height=%d, dst_x=%d, dst_y=%d\n",
	        dst_width, dst_height, dst_x, dst_y);
	printf ("w=%d, h=%d\n", w, h);
    }

    pixman_image_composite (op, src_img, mask_img, dst_img,
                            src_x, src_y, mask_x, mask_y, dst_x, dst_y, w, h);

    crc32 = compute_crc32_for_image (0, dst_img);
    
    if (verbose)
	print_image (dst_img);

    pixman_image_unref (src_img);
    if (mask_img != NULL)
	pixman_image_unref (mask_img);
    pixman_image_unref (dst_img);

    if (src_stride < 0)
	srcbuf += (src_stride / 4) * (src_height - 1);

    if (mask_stride < 0)
	maskbuf += (mask_stride / 4) * (mask_height - 1);

    if (dst_stride < 0)
	dstbuf += (dst_stride / 4) * (dst_height - 1);

    free (srcbuf);
    free (maskbuf);
    free (dstbuf);

    FLOAT_REGS_CORRUPTION_DETECTOR_FINISH ();
    return crc32;
}

#if BILINEAR_INTERPOLATION_BITS == 7
#define CHECKSUM 0x92E0F068
#elif BILINEAR_INTERPOLATION_BITS == 4
#define CHECKSUM 0x8EFFA1E5
#else
#define CHECKSUM 0x00000000
#endif

int
main (int argc, const char *argv[])
{
    pixman_disable_out_of_bounds_workaround ();

    return fuzzer_test_main("scaling", 8000000, CHECKSUM,
			    test_composite, argc, argv);
}
