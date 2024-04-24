#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

/*
 * We have a source image filled with solid color, set NORMAL or PAD repeat,
 * and some transform which results in nearest neighbour scaling.
 *
 * The expected result is either that the destination image filled with this solid
 * color or, if the transformation is such that we can't composite anything at
 * all, that nothing has changed in the destination.
 *
 * The surrounding memory of the source image is a different solid color so that
 * we are sure to get failures if we access it.
 */
static int
run_test (int32_t		dst_width,
	  int32_t		dst_height,
	  int32_t		src_width,
	  int32_t		src_height,
	  int32_t		src_x,
	  int32_t		src_y,
	  int32_t		scale_x,
	  int32_t		scale_y,
	  pixman_filter_t	filter,
	  pixman_repeat_t	repeat)
{
    pixman_image_t *   src_img;
    pixman_image_t *   dst_img;
    pixman_transform_t transform;
    uint32_t *         srcbuf;
    uint32_t *         dstbuf;
    pixman_color_t     color_cc = { 0xcccc, 0xcccc, 0xcccc, 0xcccc };
    pixman_image_t *   solid;
    int result;
    int i;

    static const pixman_fixed_t kernel[] =
    {
#define D(f)	(pixman_double_to_fixed (f) + 0x0001)

	pixman_int_to_fixed (5),
	pixman_int_to_fixed (5),
	D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0),
	D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0),
	D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0),
	D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0),
	D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0), D(1/25.0)
    };

    result = 0;

    srcbuf = (uint32_t *)malloc ((src_width + 10) * (src_height + 10) * 4);
    dstbuf = (uint32_t *)malloc (dst_width * dst_height * 4);

    memset (srcbuf, 0x88, src_width * src_height * 4);
    memset (dstbuf, 0x33, dst_width * dst_height * 4);

    src_img = pixman_image_create_bits (
        PIXMAN_a8r8g8b8, src_width, src_height,
	srcbuf + (src_width + 10) * 5 + 5, (src_width + 10) * 4);

    solid = pixman_image_create_solid_fill (&color_cc);
    pixman_image_composite32 (PIXMAN_OP_SRC, solid, NULL, src_img,
			      0, 0, 0, 0, 0, 0, src_width, src_height);
    pixman_image_unref (solid);

    dst_img = pixman_image_create_bits (
        PIXMAN_a8r8g8b8, dst_width, dst_height, dstbuf, dst_width * 4);

    pixman_transform_init_scale (&transform, scale_x, scale_y);
    pixman_image_set_transform (src_img, &transform);
    pixman_image_set_repeat (src_img, repeat);
    if (filter == PIXMAN_FILTER_CONVOLUTION)
	pixman_image_set_filter (src_img, filter, kernel, 27);
    else
	pixman_image_set_filter (src_img, filter, NULL, 0);

    pixman_image_composite (PIXMAN_OP_SRC, src_img, NULL, dst_img,
                            src_x, src_y, 0, 0, 0, 0, dst_width, dst_height);

    pixman_image_unref (src_img);
    pixman_image_unref (dst_img);

    for (i = 0; i < dst_width * dst_height; i++)
    {
	if (dstbuf[i] != 0xCCCCCCCC && dstbuf[i] != 0x33333333)
	{
	    result = 1;
	    break;
	}
    }

    free (srcbuf);
    free (dstbuf);
    return result;
}

typedef struct filter_info_t filter_info_t;
struct filter_info_t
{
    pixman_filter_t value;
    char name[28];
};

static const filter_info_t filters[] =
{
    { PIXMAN_FILTER_NEAREST, "NEAREST" },
    { PIXMAN_FILTER_BILINEAR, "BILINEAR" },
    { PIXMAN_FILTER_CONVOLUTION, "CONVOLUTION" },
};

typedef struct repeat_info_t repeat_info_t;
struct repeat_info_t
{
    pixman_repeat_t value;
    char name[28];
};


static const repeat_info_t repeats[] =
{
    { PIXMAN_REPEAT_PAD, "PAD" },
    { PIXMAN_REPEAT_REFLECT, "REFLECT" },
    { PIXMAN_REPEAT_NORMAL, "NORMAL" }
};

static int
do_test (int32_t		dst_size,
	 int32_t		src_size,
	 int32_t		src_offs,
	 int32_t		scale_factor)
{
    int i, j;

    for (i = 0; i < ARRAY_LENGTH (filters); ++i)
    {
	for (j = 0; j < ARRAY_LENGTH (repeats); ++j)
	{
	    /* horizontal test */
	    if (run_test (dst_size, 1,
			  src_size, 1,
			  src_offs, 0,
			  scale_factor, 65536,
			  filters[i].value,
			  repeats[j].value) != 0)
	    {
		printf ("Vertical test failed with %s filter and repeat mode %s\n",
			filters[i].name, repeats[j].name);

		return 1;
	    }

	    /* vertical test */
	    if (run_test (1, dst_size,
			  1, src_size,
			  0, src_offs,
			  65536, scale_factor,
			  filters[i].value,
			  repeats[j].value) != 0)
	    {
		printf ("Vertical test failed with %s filter and repeat mode %s\n",
			filters[i].name, repeats[j].name);

		return 1;
	    }
	}
    }

    return 0;
}

int
main (int argc, char *argv[])
{
    int i;

    pixman_disable_out_of_bounds_workaround ();

    /* can potentially crash */
    assert (do_test (
		48000, 32767, 1, 65536 * 128) == 0);

    /* can potentially get into a deadloop */
    assert (do_test (
		16384, 65536, 32, 32768) == 0);

    /* can potentially access memory outside source image buffer */
    assert (do_test (
		10, 10, 0, 1) == 0);
    assert (do_test (
		10, 10, 0, 0) == 0);

    for (i = 0; i < 100; ++i)
    {
	pixman_fixed_t one_seventh =
	    (((pixman_fixed_48_16_t)pixman_fixed_1) << 16) / (7 << 16);

	assert (do_test (
		    1, 7, 3, one_seventh + i - 50) == 0);
    }

    for (i = 0; i < 100; ++i)
    {
	pixman_fixed_t scale =
	    (((pixman_fixed_48_16_t)pixman_fixed_1) << 16) / (32767 << 16);

	assert (do_test (
		    1, 32767, 16383, scale + i - 50) == 0);
    }

    /* can potentially provide invalid results (out of range matrix stuff) */
    assert (do_test (
	48000, 32767, 16384, 65536 * 128) == 0);

    return 0;
}
