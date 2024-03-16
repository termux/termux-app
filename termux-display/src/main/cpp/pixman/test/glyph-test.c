#include <stdlib.h>
#include "utils.h"

static const pixman_format_code_t glyph_formats[] =
{
    PIXMAN_a8r8g8b8,
    PIXMAN_a8,
    PIXMAN_a4,
    PIXMAN_a1,
    PIXMAN_x8r8g8b8,
    PIXMAN_r3g3b2,
    PIXMAN_null,
};

static const pixman_format_code_t formats[] =
{
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
#if 0
    /* These use floating point */
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
    PIXMAN_x4a4,
    PIXMAN_a4,
    PIXMAN_r1g2b1,
    PIXMAN_b1g2r1,
    PIXMAN_a1r1g1b1,
    PIXMAN_a1b1g1r1,
    PIXMAN_null,
};

static const pixman_op_t operators[] =
{
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
    PIXMAN_OP_ADD
};

enum
{
    ALLOW_CLIPPED		= (1 << 0),
    ALLOW_ALPHA_MAP		= (1 << 1),
    ALLOW_SOURCE_CLIPPING	= (1 << 2),
    ALLOW_REPEAT		= (1 << 3),
    ALLOW_SOLID			= (1 << 4),
    ALLOW_FENCED_MEMORY		= (1 << 5),
};

static void
destroy_fenced (pixman_image_t *image, void *data)
{
    fence_free (data);
}

static void
destroy_malloced (pixman_image_t *image, void *data)
{
    free (data);
}

static pixman_format_code_t
random_format (const pixman_format_code_t *formats)
{
    int i;
    i = 0;
    while (formats[i] != PIXMAN_null)
	++i;
    return formats[prng_rand_n (i)];
}

static pixman_image_t *
create_image (int max_size, const pixman_format_code_t *formats, uint32_t flags)
{
    int width, height;
    pixman_image_t *image;
    pixman_format_code_t format;
    uint32_t *data;
    int bpp;
    int stride;
    int i;
    pixman_image_destroy_func_t destroy;

    if ((flags & ALLOW_SOLID) && prng_rand_n (4) == 0)
    {
	pixman_color_t color;

	color.alpha = prng_rand();
	color.red = prng_rand();
	color.green = prng_rand();
	color.blue = prng_rand();

	return pixman_image_create_solid_fill (&color);
    }

    width = prng_rand_n (max_size) + 1;
    height = prng_rand_n (max_size) + 1;
    format = random_format (formats);

    bpp = PIXMAN_FORMAT_BPP (format);
    stride = (width * bpp + 7) / 8 + prng_rand_n (17);
    stride = (stride + 3) & ~3;

    if (prng_rand_n (64) == 0)
    {
	if (!(data = (uint32_t *)make_random_bytes (stride * height)))
	{
	    fprintf (stderr, "Out of memory\n");
	    abort ();
	}
	destroy = destroy_fenced;
    }
    else
    {
	data = malloc (stride * height);
	prng_randmemset (data, height * stride, 0);
	destroy = destroy_malloced;
    }

    image = pixman_image_create_bits (format, width, height, data, stride);
    pixman_image_set_destroy_function (image, destroy, data);

    if ((flags & ALLOW_CLIPPED) && prng_rand_n (8) == 0)
    {
	pixman_box16_t clip_boxes[8];
	pixman_region16_t clip;
	int n = prng_rand_n (8) + 1;

	for (i = 0; i < n; i++)
	{
	    clip_boxes[i].x1 = prng_rand_n (width);
	    clip_boxes[i].y1 = prng_rand_n (height);
	    clip_boxes[i].x2 =
		clip_boxes[i].x1 + prng_rand_n (width - clip_boxes[i].x1);
	    clip_boxes[i].y2 =
		clip_boxes[i].y1 + prng_rand_n (height - clip_boxes[i].y1);
	}

	pixman_region_init_rects (&clip, clip_boxes, n);
	pixman_image_set_clip_region (image, &clip);
	pixman_region_fini (&clip);
    }

    if ((flags & ALLOW_SOURCE_CLIPPING) && prng_rand_n (4) == 0)
    {
	pixman_image_set_source_clipping (image, TRUE);
	pixman_image_set_has_client_clip (image, TRUE);
    }

    if ((flags & ALLOW_ALPHA_MAP) && prng_rand_n (16) == 0)
    {
	pixman_image_t *alpha_map;
	int alpha_x, alpha_y;

	alpha_x = prng_rand_n (width);
	alpha_y = prng_rand_n (height);
	alpha_map =
	    create_image (max_size, formats, (flags & ~(ALLOW_ALPHA_MAP | ALLOW_SOLID)));
	pixman_image_set_alpha_map (image, alpha_map, alpha_x, alpha_y);
	pixman_image_unref (alpha_map);
    }

    if ((flags & ALLOW_REPEAT) && prng_rand_n (2) == 0)
	pixman_image_set_repeat (image, prng_rand_n (4));

    image_endian_swap (image);

    return image;
}

#define KEY1(p) ((void *)(((uintptr_t)p) ^ (0xa7e23dfaUL)))
#define KEY2(p) ((void *)(((uintptr_t)p) ^ (0xabcd9876UL)))

#define MAX_GLYPHS 32

uint32_t
test_glyphs (int testnum, int verbose)
{
    pixman_image_t *glyph_images[MAX_GLYPHS];
    pixman_glyph_t glyphs[4 * MAX_GLYPHS];
    uint32_t crc32 = 0;
    pixman_image_t *source, *dest;
    int n_glyphs, i;
    pixman_glyph_cache_t *cache;

    prng_srand (testnum);

    cache = pixman_glyph_cache_create ();

    source = create_image (300, formats,
			   ALLOW_CLIPPED | ALLOW_ALPHA_MAP |
			   ALLOW_SOURCE_CLIPPING |
			   ALLOW_REPEAT | ALLOW_SOLID);

    dest = create_image (128, formats,
			 ALLOW_CLIPPED | ALLOW_ALPHA_MAP |
			 ALLOW_SOURCE_CLIPPING);

    pixman_glyph_cache_freeze (cache);

    n_glyphs = prng_rand_n (MAX_GLYPHS);
    for (i = 0; i < n_glyphs; ++i)
	glyph_images[i] = create_image (32, glyph_formats, 0);

    for (i = 0; i < 4 * n_glyphs; ++i)
    {
	int g = prng_rand_n (n_glyphs);
	pixman_image_t *glyph_img = glyph_images[g];
	void *key1 = KEY1 (glyph_img);
	void *key2 = KEY2 (glyph_img);
	const void *glyph;

	if (!(glyph = pixman_glyph_cache_lookup (cache, key1, key2)))
	{
	    glyph =
		pixman_glyph_cache_insert (cache, key1, key2, 5, 8, glyph_img);
	}

	glyphs[i].glyph = glyph;
	glyphs[i].x = prng_rand_n (128);
	glyphs[i].y = prng_rand_n (128);
    }

    if (prng_rand_n (2) == 0)
    {
	int src_x = prng_rand_n (300) - 150;
	int src_y = prng_rand_n (300) - 150;
	int mask_x = prng_rand_n (64) - 32;
	int mask_y = prng_rand_n (64) - 32;
	int dest_x = prng_rand_n (64) - 32;
	int dest_y = prng_rand_n (64) - 32;
	int width = prng_rand_n (64);
	int height = prng_rand_n (64);
	pixman_op_t op = operators[prng_rand_n (ARRAY_LENGTH (operators))];
	pixman_format_code_t format = random_format (glyph_formats);

	pixman_composite_glyphs (
	    op,
	    source, dest, format,
	    src_x, src_y,
	    mask_x, mask_y,
	    dest_x, dest_y,
	    width, height,
	    cache, 4 * n_glyphs, glyphs);
    }
    else
    {
	pixman_op_t op = operators[prng_rand_n (ARRAY_LENGTH (operators))];
	int src_x = prng_rand_n (300) - 150;
	int src_y = prng_rand_n (300) - 150;
	int dest_x = prng_rand_n (64) - 32;
	int dest_y = prng_rand_n (64) - 32;

	pixman_composite_glyphs_no_mask (
	    op, source, dest,
	    src_x, src_y,
	    dest_x, dest_y,
	    cache, 4 * n_glyphs, glyphs);
    }

    pixman_glyph_cache_thaw (cache);

    for (i = 0; i < n_glyphs; ++i)
    {
	pixman_image_t *img = glyph_images[i];
	void *key1, *key2;

	key1 = KEY1 (img);
	key2 = KEY2 (img);

	pixman_glyph_cache_remove (cache, key1, key2);
	pixman_image_unref (glyph_images[i]);
    }

    crc32 = compute_crc32_for_image (0, dest);

    pixman_image_unref (source);
    pixman_image_unref (dest);

    pixman_glyph_cache_destroy (cache);

    return crc32;
}

int
main (int argc, const char *argv[])
{
    return fuzzer_test_main ("glyph", 30000,	
			     0xFA478A79,
			     test_glyphs, argc, argv);
}
