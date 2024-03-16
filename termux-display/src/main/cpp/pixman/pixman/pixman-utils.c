/*
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "pixman-private.h"

pixman_bool_t
_pixman_multiply_overflows_size (size_t a, size_t b)
{
    return a >= SIZE_MAX / b;
}

pixman_bool_t
_pixman_multiply_overflows_int (unsigned int a, unsigned int b)
{
    return a >= INT32_MAX / b;
}

pixman_bool_t
_pixman_addition_overflows_int (unsigned int a, unsigned int b)
{
    return a > INT32_MAX - b;
}

void *
pixman_malloc_ab_plus_c (unsigned int a, unsigned int b, unsigned int c)
{
    if (!b || a >= INT32_MAX / b || (a * b) > INT32_MAX - c)
	return NULL;

    return malloc (a * b + c);
}

void *
pixman_malloc_ab (unsigned int a,
                  unsigned int b)
{
    if (a >= INT32_MAX / b)
	return NULL;

    return malloc (a * b);
}

void *
pixman_malloc_abc (unsigned int a,
                   unsigned int b,
                   unsigned int c)
{
    if (a >= INT32_MAX / b)
	return NULL;
    else if (a * b >= INT32_MAX / c)
	return NULL;
    else
	return malloc (a * b * c);
}

static force_inline uint16_t
float_to_unorm (float f, int n_bits)
{
    uint32_t u;

    if (f > 1.0)
	f = 1.0;
    if (f < 0.0)
	f = 0.0;

    u = f * (1 << n_bits);
    u -= (u >> n_bits);

    return u;
}

static force_inline float
unorm_to_float (uint16_t u, int n_bits)
{
    uint32_t m = ((1 << n_bits) - 1);

    return (u & m) * (1.f / (float)m);
}

/*
 * This function expands images from a8r8g8b8 to argb_t.  To preserve
 * precision, it needs to know from which source format the a8r8g8b8 pixels
 * originally came.
 *
 * For example, if the source was PIXMAN_x1r5g5b5 and the red component
 * contained bits 12345, then the 8-bit value is 12345123.  To correctly
 * expand this to floating point, it should be 12345 / 31.0 and not
 * 12345123 / 255.0.
 */
void
pixman_expand_to_float (argb_t               *dst,
			const uint32_t       *src,
			pixman_format_code_t  format,
			int                   width)
{
    static const float multipliers[16] = {
	0.0f,
	1.0f / ((1 <<  1) - 1),
	1.0f / ((1 <<  2) - 1),
	1.0f / ((1 <<  3) - 1),
	1.0f / ((1 <<  4) - 1),
	1.0f / ((1 <<  5) - 1),
	1.0f / ((1 <<  6) - 1),
	1.0f / ((1 <<  7) - 1),
	1.0f / ((1 <<  8) - 1),
	1.0f / ((1 <<  9) - 1),
	1.0f / ((1 << 10) - 1),
	1.0f / ((1 << 11) - 1),
	1.0f / ((1 << 12) - 1),
	1.0f / ((1 << 13) - 1),
	1.0f / ((1 << 14) - 1),
	1.0f / ((1 << 15) - 1),
    };
    int a_size, r_size, g_size, b_size;
    int a_shift, r_shift, g_shift, b_shift;
    float a_mul, r_mul, g_mul, b_mul;
    uint32_t a_mask, r_mask, g_mask, b_mask;
    int i;

    if (!PIXMAN_FORMAT_VIS (format))
	format = PIXMAN_a8r8g8b8;

    /*
     * Determine the sizes of each component and the masks and shifts
     * required to extract them from the source pixel.
     */
    a_size = PIXMAN_FORMAT_A (format);
    r_size = PIXMAN_FORMAT_R (format);
    g_size = PIXMAN_FORMAT_G (format);
    b_size = PIXMAN_FORMAT_B (format);

    a_shift = 32 - a_size;
    r_shift = 24 - r_size;
    g_shift = 16 - g_size;
    b_shift =  8 - b_size;

    a_mask = ((1 << a_size) - 1);
    r_mask = ((1 << r_size) - 1);
    g_mask = ((1 << g_size) - 1);
    b_mask = ((1 << b_size) - 1);

    a_mul = multipliers[a_size];
    r_mul = multipliers[r_size];
    g_mul = multipliers[g_size];
    b_mul = multipliers[b_size];

    /* Start at the end so that we can do the expansion in place
     * when src == dst
     */
    for (i = width - 1; i >= 0; i--)
    {
	const uint32_t pixel = src[i];

	dst[i].a = a_mask? ((pixel >> a_shift) & a_mask) * a_mul : 1.0f;
	dst[i].r = ((pixel >> r_shift) & r_mask) * r_mul;
	dst[i].g = ((pixel >> g_shift) & g_mask) * g_mul;
	dst[i].b = ((pixel >> b_shift) & b_mask) * b_mul;
    }
}

uint16_t
pixman_float_to_unorm (float f, int n_bits)
{
    return float_to_unorm (f, n_bits);
}

float
pixman_unorm_to_float (uint16_t u, int n_bits)
{
    return unorm_to_float (u, n_bits);
}

void
pixman_contract_from_float (uint32_t     *dst,
			    const argb_t *src,
			    int           width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t a, r, g, b;

	a = float_to_unorm (src[i].a, 8);
	r = float_to_unorm (src[i].r, 8);
	g = float_to_unorm (src[i].g, 8);
	b = float_to_unorm (src[i].b, 8);

	dst[i] = (a << 24) | (r << 16) | (g << 8) | (b << 0);
    }
}

uint32_t *
_pixman_iter_get_scanline_noop (pixman_iter_t *iter, const uint32_t *mask)
{
    return iter->buffer;
}

void
_pixman_iter_init_bits_stride (pixman_iter_t *iter, const pixman_iter_info_t *info)
{
    pixman_image_t *image = iter->image;
    uint8_t *b = (uint8_t *)image->bits.bits;
    int s = image->bits.rowstride * 4;

    iter->bits = b + s * iter->y + iter->x * PIXMAN_FORMAT_BPP (info->format) / 8;
    iter->stride = s;
}

#define N_TMP_BOXES (16)

pixman_bool_t
pixman_region16_copy_from_region32 (pixman_region16_t *dst,
                                    pixman_region32_t *src)
{
    int n_boxes, i;
    pixman_box32_t *boxes32;
    pixman_box16_t *boxes16;
    pixman_bool_t retval;

    boxes32 = pixman_region32_rectangles (src, &n_boxes);

    boxes16 = pixman_malloc_ab (n_boxes, sizeof (pixman_box16_t));

    if (!boxes16)
	return FALSE;

    for (i = 0; i < n_boxes; ++i)
    {
	boxes16[i].x1 = boxes32[i].x1;
	boxes16[i].y1 = boxes32[i].y1;
	boxes16[i].x2 = boxes32[i].x2;
	boxes16[i].y2 = boxes32[i].y2;
    }

    pixman_region_fini (dst);
    retval = pixman_region_init_rects (dst, boxes16, n_boxes);
    free (boxes16);
    return retval;
}

pixman_bool_t
pixman_region32_copy_from_region16 (pixman_region32_t *dst,
                                    pixman_region16_t *src)
{
    int n_boxes, i;
    pixman_box16_t *boxes16;
    pixman_box32_t *boxes32;
    pixman_box32_t tmp_boxes[N_TMP_BOXES];
    pixman_bool_t retval;

    boxes16 = pixman_region_rectangles (src, &n_boxes);

    if (n_boxes > N_TMP_BOXES)
	boxes32 = pixman_malloc_ab (n_boxes, sizeof (pixman_box32_t));
    else
	boxes32 = tmp_boxes;

    if (!boxes32)
	return FALSE;

    for (i = 0; i < n_boxes; ++i)
    {
	boxes32[i].x1 = boxes16[i].x1;
	boxes32[i].y1 = boxes16[i].y1;
	boxes32[i].x2 = boxes16[i].x2;
	boxes32[i].y2 = boxes16[i].y2;
    }

    pixman_region32_fini (dst);
    retval = pixman_region32_init_rects (dst, boxes32, n_boxes);

    if (boxes32 != tmp_boxes)
	free (boxes32);

    return retval;
}

/* This function is exported for the sake of the test suite and not part
 * of the ABI.
 */
PIXMAN_EXPORT pixman_implementation_t *
_pixman_internal_only_get_implementation (void)
{
    return get_implementation ();
}

void
_pixman_log_error (const char *function, const char *message)
{
    static int n_messages = 0;

    if (n_messages < 10)
    {
	fprintf (stderr,
		 "*** BUG ***\n"
		 "In %s: %s\n"
		 "Set a breakpoint on '_pixman_log_error' to debug\n\n",
                 function, message);

	n_messages++;
    }
}
