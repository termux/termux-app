#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define WIDTH 48
#define HEIGHT 48

static const pixman_format_code_t formats[] =
{
    PIXMAN_a8r8g8b8,
    PIXMAN_a2r10g10b10,
    PIXMAN_a4r4g4b4,
    PIXMAN_a8,
    PIXMAN_rgba_float,
};

static const pixman_format_code_t alpha_formats[] =
{
    PIXMAN_null,
    PIXMAN_a8,
    PIXMAN_a2r10g10b10,
    PIXMAN_a4r4g4b4,
    PIXMAN_rgba_float,
};

static const int origins[] =
{
    0, 10, -100
};

static void
on_destroy (pixman_image_t *image, void *data)
{
    uint32_t *bits = pixman_image_get_data (image);

    fence_free (bits);
}

static pixman_image_t *
make_image (pixman_format_code_t format)
{
    uint32_t *bits;
    uint8_t bpp = PIXMAN_FORMAT_BPP (format) / 8;
    pixman_image_t *image;

    if (format != PIXMAN_rgba_float)
	bits = (uint32_t *)make_random_bytes (WIDTH * HEIGHT * bpp);
    else
	bits = (uint32_t *)make_random_floats (WIDTH * HEIGHT * bpp);

    image = pixman_image_create_bits (format, WIDTH, HEIGHT, bits, WIDTH * bpp);

    if (image && bits)
	pixman_image_set_destroy_function (image, on_destroy, NULL);

    return image;
}

static float
get_alpha (pixman_image_t *image, int x, int y, int orig_x, int orig_y)
{
    uint8_t *bits;
    uint32_t r;

    if (image->common.alpha_map)
    {
	if (x - orig_x >= 0 && x - orig_x < WIDTH &&
	    y - orig_y >= 0 && y - orig_y < HEIGHT)
	{
	    image = (pixman_image_t *)image->common.alpha_map;

	    x -= orig_x;
	    y -= orig_y;
	}
	else
	{
	    return 0.f;
	}
    }

    bits = (uint8_t *)image->bits.bits;

    if (image->bits.format == PIXMAN_a8)
    {
	r = bits[y * WIDTH + x];
	return r / 255.f;
    }
    else if (image->bits.format == PIXMAN_a2r10g10b10)
    {
	r = ((uint32_t *)bits)[y * WIDTH + x] >> 30;
	return r / 3.f;
    }
    else if (image->bits.format == PIXMAN_a8r8g8b8)
    {
	r = ((uint32_t *)bits)[y * WIDTH + x] >> 24;
	return r / 255.f;
    }
    else if (image->bits.format == PIXMAN_a4r4g4b4)
    {
	r = ((uint16_t *)bits)[y * WIDTH + x] >> 12;
	return r / 15.f;
    }
    else if (image->bits.format == PIXMAN_rgba_float)
    {
	return ((float *)bits)[y * WIDTH * 4 + x * 4 + 3];
    }
    else
    {
	assert (0);
	return 0.f;
    }
}

static uint16_t
get_red (pixman_image_t *image, int x, int y, int orig_x, int orig_y)
{
    uint8_t *bits;
    uint16_t r;

    bits = (uint8_t *)image->bits.bits;

    if (image->bits.format == PIXMAN_a8)
    {
	r = 0x00;
    }
    else if (image->bits.format == PIXMAN_a2r10g10b10)
    {
	r = ((uint32_t *)bits)[y * WIDTH + x] >> 14;
	r &= 0xffc0;
	r |= (r >> 10);
    }
    else if (image->bits.format == PIXMAN_a8r8g8b8)
    {
	r = ((uint32_t *)bits)[y * WIDTH + x] >> 16;
	r &= 0xff;
	r |= r << 8;
    }
    else if (image->bits.format == PIXMAN_a4r4g4b4)
    {
	r = ((uint16_t *)bits)[y * WIDTH + x] >> 8;
	r &= 0xf;
	r |= r << 4;
	r |= r << 8;
    }
    else if (image->bits.format == PIXMAN_rgba_float)
    {
	double tmp = ((float *)bits)[y * WIDTH * 4 + x * 4];
	return tmp * 65535.;
    }
    else
    {
	assert (0);
    }

    return r;
}

static float get_alpha_err(pixman_format_code_t sf, pixman_format_code_t saf,
			   pixman_format_code_t df, pixman_format_code_t daf)
{
	pixman_format_code_t s = saf != PIXMAN_null ? saf : sf;
	pixman_format_code_t d = daf != PIXMAN_null ? daf : df;

	/* There are cases where we go through the 8 bit compositing
	 * path even with 10bpc and higher formats.
	 */
	if (PIXMAN_FORMAT_A(s) == PIXMAN_FORMAT_A(d))
		return 1.f / 255.f;
	else if (PIXMAN_FORMAT_A(s) > PIXMAN_FORMAT_A(d))
		return 1.f / ((1 << PIXMAN_FORMAT_A(d)) - 1);
	else
		return 1.f / ((1 << PIXMAN_FORMAT_A(s)) - 1);
}

static int
run_test (int s, int d, int sa, int da, int soff, int doff)
{
    pixman_format_code_t sf = formats[s];
    pixman_format_code_t df = formats[d];
    pixman_format_code_t saf = alpha_formats[sa];
    pixman_format_code_t daf = alpha_formats[da];
    pixman_image_t *src, *dst, *orig_dst, *alpha, *orig_alpha;
    pixman_transform_t t1;
    int j, k;
    int n_red_bits;

    soff = origins[soff];
    doff = origins[doff];

    n_red_bits = PIXMAN_FORMAT_R (df);

    /* Source */
    src = make_image (sf);
    if (saf != PIXMAN_null)
    {
	alpha = make_image (saf);
	pixman_image_set_alpha_map (src, alpha, soff, soff);
	pixman_image_unref (alpha);
    }

    /* Destination */
    orig_dst = make_image (df);
    dst = make_image (df);
    pixman_image_composite (PIXMAN_OP_SRC, orig_dst, NULL, dst,
			    0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);

    if (daf != PIXMAN_null)
    {
	orig_alpha = make_image (daf);
	alpha = make_image (daf);

	pixman_image_composite (PIXMAN_OP_SRC, orig_alpha, NULL, alpha,
				0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);

	pixman_image_set_alpha_map (orig_dst, orig_alpha, doff, doff);
	pixman_image_set_alpha_map (dst, alpha, doff, doff);

	pixman_image_unref (orig_alpha);
	pixman_image_unref (alpha);
    }

    /* Transformations, repeats and filters on destinations should be ignored,
     * so just set some random ones.
     */
    pixman_transform_init_identity (&t1);
    pixman_transform_scale (&t1, NULL, pixman_int_to_fixed (100), pixman_int_to_fixed (11));
    pixman_transform_rotate (&t1, NULL, pixman_double_to_fixed (0.5), pixman_double_to_fixed (0.11));
    pixman_transform_translate (&t1, NULL, pixman_int_to_fixed (11), pixman_int_to_fixed (17));

    pixman_image_set_transform (dst, &t1);
    pixman_image_set_filter (dst, PIXMAN_FILTER_BILINEAR, NULL, 0);
    pixman_image_set_repeat (dst, PIXMAN_REPEAT_REFLECT);

    pixman_image_composite (PIXMAN_OP_ADD, src, NULL, dst,
			    0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);

    for (j = MAX (doff, 0); j < MIN (HEIGHT, HEIGHT + doff); ++j)
    {
	for (k = MAX (doff, 0); k < MIN (WIDTH, WIDTH + doff); ++k)
	{
	    float sa, da, oda, refa;
	    uint16_t sr, dr, odr, refr;
	    float err;

	    err = get_alpha_err(sf, saf, df, daf);

	    sa = get_alpha (src, k, j, soff, soff);
	    da = get_alpha (dst, k, j, doff, doff);
	    oda = get_alpha (orig_dst, k, j, doff, doff);

	    if (sa + oda > 1.f)
		refa = 1.f;
	    else
		refa = sa + oda;

	    if (da - err > refa ||
	        da + err < refa)
	    {
		printf ("\nWrong alpha value at (%d, %d). Should be %g; got %g. Source was %g, original dest was %g\n",
			k, j, refa, da, sa, oda);

		printf ("src: %s, alpha: %s, origin %d %d\ndst: %s, alpha: %s, origin: %d %d\n\n",
			format_name (sf),
			format_name (saf),
			soff, soff,
			format_name (df),
			format_name (daf),
			doff, doff);
		return 1;
	    }

	    /* There are cases where we go through the 8 bit compositing
	     * path even with 10bpc formats. This results in incorrect
	     * results here, so only do the red check for narrow formats
	     */
	    if (n_red_bits <= 8)
	    {
		sr = get_red (src, k, j, soff, soff);
		dr = get_red (dst, k, j, doff, doff);
		odr = get_red (orig_dst, k, j, doff, doff);

		if (sr + odr > 0xffff)
		    refr = 0xffff;
		else
		    refr = sr + odr;

		if (abs ((dr >> (16 - n_red_bits)) - (refr >> (16 - n_red_bits))) > 1)
		{
		    printf ("%d red bits\n", n_red_bits);
		    printf ("\nWrong red value at (%d, %d). Should be 0x%x; got 0x%x. Source was 0x%x, original dest was 0x%x\n",
			    k, j, refr, dr, sr, odr);

		    printf ("src: %s, alpha: %s, origin %d %d\ndst: %s, alpha: %s, origin: %d %d\n\n",
			    format_name (sf),
			    format_name (saf),
			    soff, soff,
			    format_name (df),
			    format_name (daf),
			    doff, doff);
		    return 1;
		}
	    }
	}
    }

    pixman_image_set_alpha_map (src, NULL, 0, 0);
    pixman_image_set_alpha_map (dst, NULL, 0, 0);
    pixman_image_set_alpha_map (orig_dst, NULL, 0, 0);

    pixman_image_unref (src);
    pixman_image_unref (dst);
    pixman_image_unref (orig_dst);

    return 0;
}

int
main (int argc, char **argv)
{
    int i, j, a, b, x, y;

    prng_srand (0);

    for (i = 0; i < ARRAY_LENGTH (formats); ++i)
    {
	for (j = 0; j < ARRAY_LENGTH (formats); ++j)
	{
	    for (a = 0; a < ARRAY_LENGTH (alpha_formats); ++a)
	    {
		for (b = 0; b < ARRAY_LENGTH (alpha_formats); ++b)
		{
		    for (x = 0; x < ARRAY_LENGTH (origins); ++x)
		    {
			for (y = 0; y < ARRAY_LENGTH (origins); ++y)
			{
			    if (run_test (i, j, a, b, x, y) != 0)
				return 1;
			}
		    }
		}
	    }
	}
    }

    return 0;
}
