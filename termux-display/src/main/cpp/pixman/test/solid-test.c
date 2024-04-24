/*
 * Copyright © 2015 RISC OS Open Ltd
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author:  Ben Avison (bavison@riscosopen.org)
 *
 */

#include "utils.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define WIDTH 32
#define HEIGHT 32

static const pixman_op_t op_list[] = {
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

/* The first eight format in the list are by far the most widely
 * used formats, so we test those more than the others
 */
#define N_MOST_LIKELY_FORMATS 8

static const pixman_format_code_t img_fmt_list[] = {
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

static const pixman_format_code_t mask_fmt_list[] = {
    PIXMAN_a8r8g8b8,
    PIXMAN_a8,
    PIXMAN_a4,
    PIXMAN_a1,
    PIXMAN_null
};

static pixman_indexed_t rgb_palette[9];
static pixman_indexed_t y_palette[9];

static pixman_format_code_t
random_format (const pixman_format_code_t *allowed_formats)
{
    int n = 0;

    while (allowed_formats[n] != PIXMAN_null)
        n++;

    if (n > N_MOST_LIKELY_FORMATS && prng_rand_n (4) != 0)
        n = N_MOST_LIKELY_FORMATS;

    return allowed_formats[prng_rand_n (n)];
}

static pixman_image_t *
create_multi_pixel_image (const pixman_format_code_t *allowed_formats,
                          uint32_t                   *buffer,
                          pixman_format_code_t       *used_fmt)
{
    pixman_format_code_t fmt;
    pixman_image_t *img;
    int stride;

    fmt = random_format (allowed_formats);
    stride = (WIDTH * PIXMAN_FORMAT_BPP (fmt) + 31) / 32 * 4;
    img = pixman_image_create_bits (fmt, WIDTH, HEIGHT, buffer, stride);

    if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_COLOR)
        pixman_image_set_indexed (img, &(rgb_palette[PIXMAN_FORMAT_BPP (fmt)]));
    else if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_GRAY)
        pixman_image_set_indexed (img, &(y_palette[PIXMAN_FORMAT_BPP (fmt)]));

    prng_randmemset (buffer, WIDTH * HEIGHT * 4, 0);
    image_endian_swap (img);

    if (used_fmt)
        *used_fmt = fmt;

    return img;
}

static pixman_image_t *
create_solid_image (const pixman_format_code_t *allowed_formats,
                    uint32_t                   *buffer,
                    pixman_format_code_t       *used_fmt)
{
    if (prng_rand_n (2))
    {
        /* Use a repeating 1x1 bitmap image for solid */
        pixman_format_code_t fmt;
        pixman_image_t      *img, *dummy_img;
        uint32_t             bpp, dummy_buf;

        fmt = random_format (allowed_formats);
        bpp = PIXMAN_FORMAT_BPP (fmt);
        img = pixman_image_create_bits (fmt, 1, 1, buffer, 4);
        pixman_image_set_repeat (img, PIXMAN_REPEAT_NORMAL);

        if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_COLOR)
            pixman_image_set_indexed (img, &(rgb_palette[bpp]));
        else if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_GRAY)
            pixman_image_set_indexed (img, &(y_palette[bpp]));

        /* Force the flags to be calculated for image with initial
         * bitmap contents of 0 or 2^bpp-1 by plotting from it into a
         * separate throwaway image. It is simplest to write all 0s
         * or all 1s to the first word irrespective of the colour
         * depth even though we actually only care about the first
         * pixel since the stride has to be a whole number of words.
         */
        *buffer = prng_rand_n (2) ? 0xFFFFFFFFu : 0;
        dummy_img = pixman_image_create_bits (PIXMAN_a8r8g8b8, 1, 1,
                                              &dummy_buf, 4);
        pixman_image_composite (PIXMAN_OP_SRC, img, NULL, dummy_img,
                                0, 0, 0, 0, 0, 0, 1, 1);
        pixman_image_unref (dummy_img);

        /* Now set the bitmap contents to a random value */
        prng_randmemset (buffer, 4, 0);
        image_endian_swap (img);

        if (used_fmt)
            *used_fmt = fmt;

        return img;
    }
    else
    {
        /* Use a native solid image */
        pixman_color_t color;
        pixman_image_t *img;

        color.alpha = prng_rand_n (UINT16_MAX + 1);
        color.red   = prng_rand_n (UINT16_MAX + 1);
        color.green = prng_rand_n (UINT16_MAX + 1);
        color.blue  = prng_rand_n (UINT16_MAX + 1);
        img = pixman_image_create_solid_fill (&color);

        if (used_fmt)
            *used_fmt = PIXMAN_solid;

        return img;
    }
}

static uint32_t
test_solid (int testnum, int verbose)
{
    pixman_op_t          op;
    uint32_t             src_buf[WIDTH * HEIGHT];
    uint32_t             dst_buf[WIDTH * HEIGHT];
    uint32_t             mask_buf[WIDTH * HEIGHT];
    pixman_image_t      *src_img;
    pixman_image_t      *dst_img;
    pixman_image_t      *mask_img = NULL;
    pixman_format_code_t src_fmt, dst_fmt, mask_fmt = PIXMAN_null;
    pixman_bool_t        ca = 0;
    uint32_t             crc32;

    prng_srand (testnum);

    op = op_list[prng_rand_n (ARRAY_LENGTH (op_list))];

    dst_img = create_multi_pixel_image (img_fmt_list, dst_buf, &dst_fmt);
    switch (prng_rand_n (3))
    {
    case 0: /* Solid source, no mask */
        src_img = create_solid_image (img_fmt_list, src_buf, &src_fmt);
        break;
    case 1: /* Solid source, bitmap mask */
        src_img = create_solid_image (img_fmt_list, src_buf, &src_fmt);
        mask_img = create_multi_pixel_image (mask_fmt_list, mask_buf, &mask_fmt);
        break;
    case 2: /* Bitmap image, solid mask */
        src_img = create_multi_pixel_image (img_fmt_list, src_buf, &src_fmt);
        mask_img = create_solid_image (mask_fmt_list, mask_buf, &mask_fmt);
        break;
    default:
        abort ();
    }

    if (mask_img)
    {
        ca = prng_rand_n (2);
        pixman_image_set_component_alpha (mask_img, ca);
    }

    if (verbose)
    {
        printf ("op=%s\n", operator_name (op));
        printf ("src_fmt=%s, dst_fmt=%s, mask_fmt=%s\n",
                format_name (src_fmt), format_name (dst_fmt),
                format_name (mask_fmt));
        printf ("src_size=%u, mask_size=%u, component_alpha=%u\n",
                src_fmt == PIXMAN_solid ? 1 : src_img->bits.width,
                !mask_img || mask_fmt == PIXMAN_solid ? 1 : mask_img->bits.width,
                ca);
    }

    pixman_image_composite (op, src_img, mask_img, dst_img,
                            0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);

    if (verbose)
        print_image (dst_img);

    crc32 = compute_crc32_for_image (0, dst_img);

    pixman_image_unref (src_img);
    pixman_image_unref (dst_img);
    if (mask_img)
        pixman_image_unref (mask_img);

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

    return fuzzer_test_main ("solid", 500000,
                             0xC30FD380,
			     test_solid, argc, argv);
}
