/*
 * Copyright © 2013 Soren Sandmann Pedersen
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Soren Sandmann (soren.sandmann@gmail.com)
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>
#include "pixman-private.h"
#include "pixman-inlines.h"

typedef struct
{
    int		y;
    uint64_t *	buffer;
} line_t;

typedef struct
{
    line_t		lines[2];
    pixman_fixed_t	y;
    pixman_fixed_t	x;
    uint64_t		data[1];
} bilinear_info_t;

static void
ssse3_fetch_horizontal (bits_image_t *image, line_t *line,
			int y, pixman_fixed_t x, pixman_fixed_t ux, int n)
{
    uint32_t *bits = image->bits + y * image->rowstride;
    __m128i vx = _mm_set_epi16 (
	- (x + 1), x, - (x + 1), x,
	- (x + ux + 1), x + ux,  - (x + ux + 1), x + ux);
    __m128i vux = _mm_set_epi16 (
	- 2 * ux, 2 * ux, - 2 * ux, 2 * ux,
	- 2 * ux, 2 * ux, - 2 * ux, 2 * ux);
    __m128i vaddc = _mm_set_epi16 (1, 0, 1, 0, 1, 0, 1, 0);
    __m128i *b = (__m128i *)line->buffer;
    __m128i vrl0, vrl1;

    while ((n -= 2) >= 0)
    {
	__m128i vw, vr, s;

	vrl1 = _mm_loadl_epi64 (
	    (__m128i *)(bits + pixman_fixed_to_int (x + ux)));
	/* vrl1: R1, L1 */

    final_pixel:
	vrl0 = _mm_loadl_epi64 (
	    (__m128i *)(bits + pixman_fixed_to_int (x)));
	/* vrl0: R0, L0 */

	/* The weights are based on vx which is a vector of 
	 *
	 *    - (x + 1), x, - (x + 1), x,
	 *          - (x + ux + 1), x + ux, - (x + ux + 1), x + ux
	 *
	 * so the 16 bit weights end up like this:
	 *
	 *    iw0, w0, iw0, w0, iw1, w1, iw1, w1
	 *
	 * and after shifting and packing, we get these bytes:
	 *
	 *    iw0, w0, iw0, w0, iw1, w1, iw1, w1,
	 *        iw0, w0, iw0, w0, iw1, w1, iw1, w1,
	 *
	 * which means the first and the second input pixel 
	 * have to be interleaved like this:
	 *
	 *    la0, ra0, lr0, rr0, la1, ra1, lr1, rr1,
	 *        lg0, rg0, lb0, rb0, lg1, rg1, lb1, rb1
	 *
	 * before maddubsw can be used.
	 */

	vw = _mm_add_epi16 (
	    vaddc, _mm_srli_epi16 (vx, 16 - BILINEAR_INTERPOLATION_BITS));
	/* vw: iw0, w0, iw0, w0, iw1, w1, iw1, w1
	 */

	vw = _mm_packus_epi16 (vw, vw);
	/* vw: iw0, w0, iw0, w0, iw1, w1, iw1, w1,
	 *         iw0, w0, iw0, w0, iw1, w1, iw1, w1
	 */
	vx = _mm_add_epi16 (vx, vux);

	x += 2 * ux;

	vr = _mm_unpacklo_epi16 (vrl1, vrl0);
	/* vr: rar0, rar1, rgb0, rgb1, lar0, lar1, lgb0, lgb1 */

	s = _mm_shuffle_epi32 (vr, _MM_SHUFFLE (1, 0, 3, 2));
	/* s:  lar0, lar1, lgb0, lgb1, rar0, rar1, rgb0, rgb1 */

	vr = _mm_unpackhi_epi8 (vr, s);
	/* vr: la0, ra0, lr0, rr0, la1, ra1, lr1, rr1,
	 *         lg0, rg0, lb0, rb0, lg1, rg1, lb1, rb1
	 */

	vr = _mm_maddubs_epi16 (vr, vw);

	/* When the weight is 0, the inverse weight is
	 * 128 which can't be represented in a signed byte.
	 * As a result maddubsw computes the following:
	 *
	 *     r = l * -128 + r * 0
	 *
	 * rather than the desired
	 *
	 *     r = l * 128 + r * 0
	 *
	 * We fix this by taking the absolute value of the
	 * result.
	 */
	vr = _mm_abs_epi16 (vr);

	/* vr: A0, R0, A1, R1, G0, B0, G1, B1 */
	_mm_store_si128 (b++, vr);
    }

    if (n == -1)
    {
	vrl1 = _mm_setzero_si128();
	goto final_pixel;
    }

    line->y = y;
}

static uint32_t *
ssse3_fetch_bilinear_cover (pixman_iter_t *iter, const uint32_t *mask)
{
    pixman_fixed_t fx, ux;
    bilinear_info_t *info = iter->data;
    line_t *line0, *line1;
    int y0, y1;
    int32_t dist_y;
    __m128i vw;
    int i;

    fx = info->x;
    ux = iter->image->common.transform->matrix[0][0];

    y0 = pixman_fixed_to_int (info->y);
    y1 = y0 + 1;

    line0 = &info->lines[y0 & 0x01];
    line1 = &info->lines[y1 & 0x01];

    if (line0->y != y0)
    {
	ssse3_fetch_horizontal (
	    &iter->image->bits, line0, y0, fx, ux, iter->width);
    }

    if (line1->y != y1)
    {
	ssse3_fetch_horizontal (
	    &iter->image->bits, line1, y1, fx, ux, iter->width);
    }

    dist_y = pixman_fixed_to_bilinear_weight (info->y);
    dist_y <<= (16 - BILINEAR_INTERPOLATION_BITS);

    vw = _mm_set_epi16 (
	dist_y, dist_y, dist_y, dist_y, dist_y, dist_y, dist_y, dist_y);

    for (i = 0; i + 3 < iter->width; i += 4)
    {
	__m128i top0 = _mm_load_si128 ((__m128i *)(line0->buffer + i));
	__m128i bot0 = _mm_load_si128 ((__m128i *)(line1->buffer + i));
	__m128i top1 = _mm_load_si128 ((__m128i *)(line0->buffer + i + 2));
	__m128i bot1 = _mm_load_si128 ((__m128i *)(line1->buffer + i + 2));
	__m128i r0, r1, tmp, p;

	r0 = _mm_mulhi_epu16 (
	    _mm_sub_epi16 (bot0, top0), vw);
	tmp = _mm_cmplt_epi16 (bot0, top0);
	tmp = _mm_and_si128 (tmp, vw);
	r0 = _mm_sub_epi16 (r0, tmp);
	r0 = _mm_add_epi16 (r0, top0);
	r0 = _mm_srli_epi16 (r0, BILINEAR_INTERPOLATION_BITS);
	/* r0:  A0 R0 A1 R1 G0 B0 G1 B1 */
	r0 = _mm_shuffle_epi32 (r0, _MM_SHUFFLE (2, 0, 3, 1));
	/* r0:  A1 R1 G1 B1 A0 R0 G0 B0 */

	r1 = _mm_mulhi_epu16 (
	    _mm_sub_epi16 (bot1, top1), vw);
	tmp = _mm_cmplt_epi16 (bot1, top1);
	tmp = _mm_and_si128 (tmp, vw);
	r1 = _mm_sub_epi16 (r1, tmp);
	r1 = _mm_add_epi16 (r1, top1);
	r1 = _mm_srli_epi16 (r1, BILINEAR_INTERPOLATION_BITS);
	r1 = _mm_shuffle_epi32 (r1, _MM_SHUFFLE (2, 0, 3, 1));
	/* r1: A3 R3 G3 B3 A2 R2 G2 B2 */

	p = _mm_packus_epi16 (r0, r1);

	_mm_storeu_si128 ((__m128i *)(iter->buffer + i), p);
    }

    while (i < iter->width)
    {
	__m128i top0 = _mm_load_si128 ((__m128i *)(line0->buffer + i));
	__m128i bot0 = _mm_load_si128 ((__m128i *)(line1->buffer + i));
	__m128i r0, tmp, p;

	r0 = _mm_mulhi_epu16 (
	    _mm_sub_epi16 (bot0, top0), vw);
	tmp = _mm_cmplt_epi16 (bot0, top0);
	tmp = _mm_and_si128 (tmp, vw);
	r0 = _mm_sub_epi16 (r0, tmp);
	r0 = _mm_add_epi16 (r0, top0);
	r0 = _mm_srli_epi16 (r0, BILINEAR_INTERPOLATION_BITS);
	/* r0:  A0 R0 A1 R1 G0 B0 G1 B1 */
	r0 = _mm_shuffle_epi32 (r0, _MM_SHUFFLE (2, 0, 3, 1));
	/* r0:  A1 R1 G1 B1 A0 R0 G0 B0 */

	p = _mm_packus_epi16 (r0, r0);

	if (iter->width - i == 1)
	{
	    *(uint32_t *)(iter->buffer + i) = _mm_cvtsi128_si32 (p);
	    i++;
	}
	else
	{
	    _mm_storel_epi64 ((__m128i *)(iter->buffer + i), p);
	    i += 2;
	}
    }
    
    info->y += iter->image->common.transform->matrix[1][1];

    return iter->buffer;
}

static void
ssse3_bilinear_cover_iter_fini (pixman_iter_t *iter)
{
    free (iter->data);
}

static void
ssse3_bilinear_cover_iter_init (pixman_iter_t *iter, const pixman_iter_info_t *iter_info)
{
    int width = iter->width;
    bilinear_info_t *info;
    pixman_vector_t v;

    /* Reference point is the center of the pixel */
    v.vector[0] = pixman_int_to_fixed (iter->x) + pixman_fixed_1 / 2;
    v.vector[1] = pixman_int_to_fixed (iter->y) + pixman_fixed_1 / 2;
    v.vector[2] = pixman_fixed_1;

    if (!pixman_transform_point_3d (iter->image->common.transform, &v))
	goto fail;

    info = malloc (sizeof (*info) + (2 * width - 1) * sizeof (uint64_t) + 64);
    if (!info)
	goto fail;

    info->x = v.vector[0] - pixman_fixed_1 / 2;
    info->y = v.vector[1] - pixman_fixed_1 / 2;

#define ALIGN(addr)							\
    ((void *)((((uintptr_t)(addr)) + 15) & (~15)))

    /* It is safe to set the y coordinates to -1 initially
     * because COVER_CLIP_BILINEAR ensures that we will only
     * be asked to fetch lines in the [0, height) interval
     */
    info->lines[0].y = -1;
    info->lines[0].buffer = ALIGN (&(info->data[0]));
    info->lines[1].y = -1;
    info->lines[1].buffer = ALIGN (info->lines[0].buffer + width);

    iter->get_scanline = ssse3_fetch_bilinear_cover;
    iter->fini = ssse3_bilinear_cover_iter_fini;

    iter->data = info;
    return;

fail:
    /* Something went wrong, either a bad matrix or OOM; in such cases,
     * we don't guarantee any particular rendering.
     */
    _pixman_log_error (
	FUNC, "Allocation failure or bad matrix, skipping rendering\n");
    
    iter->get_scanline = _pixman_iter_get_scanline_noop;
    iter->fini = NULL;
}

static const pixman_iter_info_t ssse3_iters[] = 
{
    { PIXMAN_a8r8g8b8,
      (FAST_PATH_STANDARD_FLAGS			|
       FAST_PATH_SCALE_TRANSFORM		|
       FAST_PATH_BILINEAR_FILTER		|
       FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR),
      ITER_NARROW | ITER_SRC,
      ssse3_bilinear_cover_iter_init,
      NULL, NULL
    },

    { PIXMAN_null },
};

static const pixman_fast_path_t ssse3_fast_paths[] =
{
    { PIXMAN_OP_NONE },
};

pixman_implementation_t *
_pixman_implementation_create_ssse3 (pixman_implementation_t *fallback)
{
    pixman_implementation_t *imp =
	_pixman_implementation_create (fallback, ssse3_fast_paths);

    imp->iter_info = ssse3_iters;

    return imp;
}
