/* Based loosely on scaling-test */

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#define MAX_SRC_WIDTH  48
#define MAX_SRC_HEIGHT 48
#define MAX_DST_WIDTH  48
#define MAX_DST_HEIGHT 48
#define MAX_STRIDE     4

static pixman_format_code_t formats[] =
{
    PIXMAN_a8r8g8b8, PIXMAN_a8, PIXMAN_r5g6b5, PIXMAN_a1, PIXMAN_a4
};

static pixman_format_code_t mask_formats[] =
{
    PIXMAN_a1, PIXMAN_a4, PIXMAN_a8,
};

static pixman_op_t operators[] =
{
    PIXMAN_OP_OVER, PIXMAN_OP_ADD, PIXMAN_OP_SRC, PIXMAN_OP_IN
};

#define RANDOM_ELT(array)						\
    ((array)[prng_rand_n(ARRAY_LENGTH((array)))])

static void
destroy_bits (pixman_image_t *image, void *data)
{
    fence_free (data);
}

static pixman_fixed_t
random_fixed (int n)
{
    return prng_rand_n (n << 16);
}

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
    pixman_region16_t  clip;
    int                dst_width, dst_height;
    int                dst_stride;
    int                dst_x, dst_y;
    int                dst_bpp;
    pixman_op_t        op;
    uint32_t *         dst_bits;
    uint32_t           crc32;
    pixman_format_code_t mask_format, dst_format;
    pixman_trapezoid_t *traps;
    int src_x, src_y;
    int n_traps;

    static pixman_color_t colors[] =
    {
	{ 0xffff, 0xffff, 0xffff, 0xffff },
	{ 0x0000, 0x0000, 0x0000, 0x0000 },
	{ 0xabcd, 0xabcd, 0x0000, 0xabcd },
	{ 0x0000, 0x0000, 0x0000, 0xffff },
	{ 0x0101, 0x0101, 0x0101, 0x0101 },
	{ 0x7777, 0x6666, 0x5555, 0x9999 },
    };
    
    FLOAT_REGS_CORRUPTION_DETECTOR_START ();

    prng_srand (testnum);

    op = RANDOM_ELT (operators);
    mask_format = RANDOM_ELT (mask_formats);

    /* Create source image */
    
    if (prng_rand_n (4) == 0)
    {
	src_img = pixman_image_create_solid_fill (
	    &(colors[prng_rand_n (ARRAY_LENGTH (colors))]));

	src_x = 10;
	src_y = 234;
    }
    else
    {
	pixman_format_code_t src_format = RANDOM_ELT(formats);
	int src_bpp = (PIXMAN_FORMAT_BPP (src_format) + 7) / 8;
	int src_width = prng_rand_n (MAX_SRC_WIDTH) + 1;
	int src_height = prng_rand_n (MAX_SRC_HEIGHT) + 1;
	int src_stride = src_width * src_bpp + prng_rand_n (MAX_STRIDE) * src_bpp;
	uint32_t *bits, *orig;

	src_x = -(src_width / 4) + prng_rand_n (src_width * 3 / 2);
	src_y = -(src_height / 4) + prng_rand_n (src_height * 3 / 2);

	src_stride = (src_stride + 3) & ~3;
	
	orig = bits = (uint32_t *)make_random_bytes (src_stride * src_height);

	if (prng_rand_n (2) == 0)
	{
	    bits += (src_stride / 4) * (src_height - 1);
	    src_stride = - src_stride;
	}
	
	src_img = pixman_image_create_bits (
	    src_format, src_width, src_height, bits, src_stride);

	pixman_image_set_destroy_function (src_img, destroy_bits, orig);

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

	image_endian_swap (src_img);
    }

    /* Create destination image */
    {
	dst_format = RANDOM_ELT(formats);
	dst_bpp = (PIXMAN_FORMAT_BPP (dst_format) + 7) / 8;
	dst_width = prng_rand_n (MAX_DST_WIDTH) + 1;
	dst_height = prng_rand_n (MAX_DST_HEIGHT) + 1;
	dst_stride = dst_width * dst_bpp + prng_rand_n (MAX_STRIDE) * dst_bpp;
	dst_stride = (dst_stride + 3) & ~3;
	
	dst_bits = (uint32_t *)make_random_bytes (dst_stride * dst_height);

	if (prng_rand_n (2) == 0)
	{
	    dst_bits += (dst_stride / 4) * (dst_height - 1);
	    dst_stride = - dst_stride;
	}
	
	dst_x = -(dst_width / 4) + prng_rand_n (dst_width * 3 / 2);
	dst_y = -(dst_height / 4) + prng_rand_n (dst_height * 3 / 2);
	
	dst_img = pixman_image_create_bits (
	    dst_format, dst_width, dst_height, dst_bits, dst_stride);

	image_endian_swap (dst_img);
    }

    /* Create traps */
    {
	int i;

	n_traps = prng_rand_n (25);
	traps = fence_malloc (n_traps * sizeof (pixman_trapezoid_t));

	for (i = 0; i < n_traps; ++i)
	{
	    pixman_trapezoid_t *t = &(traps[i]);
	    
	    t->top = random_fixed (MAX_DST_HEIGHT) - MAX_DST_HEIGHT / 2;
	    t->bottom = t->top + random_fixed (MAX_DST_HEIGHT);
	    t->left.p1.x = random_fixed (MAX_DST_WIDTH) - MAX_DST_WIDTH / 2;
	    t->left.p1.y = t->top - random_fixed (50);
	    t->left.p2.x = random_fixed (MAX_DST_WIDTH) - MAX_DST_WIDTH / 2;
	    t->left.p2.y = t->bottom + random_fixed (50);
	    t->right.p1.x = t->left.p1.x + random_fixed (MAX_DST_WIDTH);
	    t->right.p1.y = t->top - random_fixed (50);
	    t->right.p2.x = t->left.p2.x + random_fixed (MAX_DST_WIDTH);
	    t->right.p2.y = t->bottom - random_fixed (50);
	}
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

    pixman_composite_trapezoids (op, src_img, dst_img, mask_format,
				 src_x, src_y, dst_x, dst_y, n_traps, traps);

    crc32 = compute_crc32_for_image (0, dst_img);

    if (verbose)
	print_image (dst_img);

    if (dst_stride < 0)
	dst_bits += (dst_stride / 4) * (dst_height - 1);
    
    fence_free (dst_bits);
    
    pixman_image_unref (src_img);
    pixman_image_unref (dst_img);
    fence_free (traps);

    FLOAT_REGS_CORRUPTION_DETECTOR_FINISH ();
    return crc32;
}

int
main (int argc, const char *argv[])
{
    return fuzzer_test_main("composite traps", 40000, 0xAF41D210,
			    test_composite, argc, argv);
}
