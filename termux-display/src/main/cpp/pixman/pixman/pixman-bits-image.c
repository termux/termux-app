/*
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *             2005 Lars Knoll & Zack Rusin, Trolltech
 *             2008 Aaron Plattner, NVIDIA Corporation
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007, 2009 Red Hat, Inc.
 * Copyright © 2008 André Tupinambá <andrelrt@gmail.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pixman-private.h"
#include "pixman-combine32.h"
#include "pixman-inlines.h"
#include "dither/blue-noise-64x64.h"

/* Fetch functions */

static force_inline void
fetch_pixel_no_alpha_32 (bits_image_t *image,
			 int x, int y, pixman_bool_t check_bounds,
			 void *out)
{
    uint32_t *ret = out;

    if (check_bounds &&
	(x < 0 || x >= image->width || y < 0 || y >= image->height))
	*ret = 0;
    else
	*ret = image->fetch_pixel_32 (image, x, y);
}

static force_inline void
fetch_pixel_no_alpha_float (bits_image_t *image,
			    int x, int y, pixman_bool_t check_bounds,
			    void *out)
{
    argb_t *ret = out;

    if (check_bounds &&
	(x < 0 || x >= image->width || y < 0 || y >= image->height))
	ret->a = ret->r = ret->g = ret->b = 0.f;
    else
	*ret = image->fetch_pixel_float (image, x, y);
}

typedef void (* get_pixel_t) (bits_image_t *image,
			      int x, int y, pixman_bool_t check_bounds, void *out);

static force_inline void
bits_image_fetch_pixel_nearest (bits_image_t   *image,
				pixman_fixed_t  x,
				pixman_fixed_t  y,
				get_pixel_t	get_pixel,
				void	       *out)
{
    int x0 = pixman_fixed_to_int (x - pixman_fixed_e);
    int y0 = pixman_fixed_to_int (y - pixman_fixed_e);

    if (image->common.repeat != PIXMAN_REPEAT_NONE)
    {
	repeat (image->common.repeat, &x0, image->width);
	repeat (image->common.repeat, &y0, image->height);

	get_pixel (image, x0, y0, FALSE, out);
    }
    else
    {
	get_pixel (image, x0, y0, TRUE, out);
    }
}

static force_inline void
bits_image_fetch_pixel_bilinear_32 (bits_image_t   *image,
				    pixman_fixed_t  x,
				    pixman_fixed_t  y,
				    get_pixel_t	    get_pixel,
				    void	   *out)
{
    pixman_repeat_t repeat_mode = image->common.repeat;
    int width = image->width;
    int height = image->height;
    int x1, y1, x2, y2;
    uint32_t tl, tr, bl, br;
    int32_t distx, disty;
    uint32_t *ret = out;

    x1 = x - pixman_fixed_1 / 2;
    y1 = y - pixman_fixed_1 / 2;

    distx = pixman_fixed_to_bilinear_weight (x1);
    disty = pixman_fixed_to_bilinear_weight (y1);

    x1 = pixman_fixed_to_int (x1);
    y1 = pixman_fixed_to_int (y1);
    x2 = x1 + 1;
    y2 = y1 + 1;

    if (repeat_mode != PIXMAN_REPEAT_NONE)
    {
	repeat (repeat_mode, &x1, width);
	repeat (repeat_mode, &y1, height);
	repeat (repeat_mode, &x2, width);
	repeat (repeat_mode, &y2, height);

	get_pixel (image, x1, y1, FALSE, &tl);
	get_pixel (image, x2, y1, FALSE, &tr);
	get_pixel (image, x1, y2, FALSE, &bl);
	get_pixel (image, x2, y2, FALSE, &br);
    }
    else
    {
	get_pixel (image, x1, y1, TRUE, &tl);
	get_pixel (image, x2, y1, TRUE, &tr);
	get_pixel (image, x1, y2, TRUE, &bl);
	get_pixel (image, x2, y2, TRUE, &br);
    }

    *ret = bilinear_interpolation (tl, tr, bl, br, distx, disty);
}

static force_inline void
bits_image_fetch_pixel_bilinear_float (bits_image_t   *image,
				       pixman_fixed_t  x,
				       pixman_fixed_t  y,
				       get_pixel_t     get_pixel,
				       void	      *out)
{
    pixman_repeat_t repeat_mode = image->common.repeat;
    int width = image->width;
    int height = image->height;
    int x1, y1, x2, y2;
    argb_t tl, tr, bl, br;
    float distx, disty;
    argb_t *ret = out;

    x1 = x - pixman_fixed_1 / 2;
    y1 = y - pixman_fixed_1 / 2;

    distx = ((float)pixman_fixed_fraction(x1)) / 65536.f;
    disty = ((float)pixman_fixed_fraction(y1)) / 65536.f;

    x1 = pixman_fixed_to_int (x1);
    y1 = pixman_fixed_to_int (y1);
    x2 = x1 + 1;
    y2 = y1 + 1;

    if (repeat_mode != PIXMAN_REPEAT_NONE)
    {
	repeat (repeat_mode, &x1, width);
	repeat (repeat_mode, &y1, height);
	repeat (repeat_mode, &x2, width);
	repeat (repeat_mode, &y2, height);

	get_pixel (image, x1, y1, FALSE, &tl);
	get_pixel (image, x2, y1, FALSE, &tr);
	get_pixel (image, x1, y2, FALSE, &bl);
	get_pixel (image, x2, y2, FALSE, &br);
    }
    else
    {
	get_pixel (image, x1, y1, TRUE, &tl);
	get_pixel (image, x2, y1, TRUE, &tr);
	get_pixel (image, x1, y2, TRUE, &bl);
	get_pixel (image, x2, y2, TRUE, &br);
    }

    *ret = bilinear_interpolation_float (tl, tr, bl, br, distx, disty);
}

static force_inline void accum_32(unsigned int *satot, unsigned int *srtot,
				  unsigned int *sgtot, unsigned int *sbtot,
				  const void *p, pixman_fixed_t f)
{
    uint32_t pixel = *(uint32_t *)p;

    *srtot += (int)RED_8 (pixel) * f;
    *sgtot += (int)GREEN_8 (pixel) * f;
    *sbtot += (int)BLUE_8 (pixel) * f;
    *satot += (int)ALPHA_8 (pixel) * f;
}

static force_inline void reduce_32(unsigned int satot, unsigned int srtot,
				   unsigned int sgtot, unsigned int sbtot,
                                   void *p)
{
    uint32_t *ret = p;

    satot = (satot + 0x8000) >> 16;
    srtot = (srtot + 0x8000) >> 16;
    sgtot = (sgtot + 0x8000) >> 16;
    sbtot = (sbtot + 0x8000) >> 16;

    satot = CLIP (satot, 0, 0xff);
    srtot = CLIP (srtot, 0, 0xff);
    sgtot = CLIP (sgtot, 0, 0xff);
    sbtot = CLIP (sbtot, 0, 0xff);

    *ret = ((satot << 24) | (srtot << 16) | (sgtot <<  8) | (sbtot));
}

static force_inline void accum_float(unsigned int *satot, unsigned int *srtot,
				     unsigned int *sgtot, unsigned int *sbtot,
				     const void *p, pixman_fixed_t f)
{
    const argb_t *pixel = p;

    *satot += pixel->a * f;
    *srtot += pixel->r * f;
    *sgtot += pixel->g * f;
    *sbtot += pixel->b * f;
}

static force_inline void reduce_float(unsigned int satot, unsigned int srtot,
				      unsigned int sgtot, unsigned int sbtot,
				      void *p)
{
    argb_t *ret = p;

    ret->a = CLIP (satot / 65536.f, 0.f, 1.f);
    ret->r = CLIP (srtot / 65536.f, 0.f, 1.f);
    ret->g = CLIP (sgtot / 65536.f, 0.f, 1.f);
    ret->b = CLIP (sbtot / 65536.f, 0.f, 1.f);
}

typedef void (* accumulate_pixel_t) (unsigned int *satot, unsigned int *srtot,
				     unsigned int *sgtot, unsigned int *sbtot,
				     const void *pixel, pixman_fixed_t f);

typedef void (* reduce_pixel_t) (unsigned int satot, unsigned int srtot,
				 unsigned int sgtot, unsigned int sbtot,
                                 void *out);

static force_inline void
bits_image_fetch_pixel_convolution (bits_image_t   *image,
				    pixman_fixed_t  x,
				    pixman_fixed_t  y,
				    get_pixel_t     get_pixel,
				    void	      *out,
				    accumulate_pixel_t accum,
				    reduce_pixel_t reduce)
{
    pixman_fixed_t *params = image->common.filter_params;
    int x_off = (params[0] - pixman_fixed_1) >> 1;
    int y_off = (params[1] - pixman_fixed_1) >> 1;
    int32_t cwidth = pixman_fixed_to_int (params[0]);
    int32_t cheight = pixman_fixed_to_int (params[1]);
    int32_t i, j, x1, x2, y1, y2;
    pixman_repeat_t repeat_mode = image->common.repeat;
    int width = image->width;
    int height = image->height;
    unsigned int srtot, sgtot, sbtot, satot;

    params += 2;

    x1 = pixman_fixed_to_int (x - pixman_fixed_e - x_off);
    y1 = pixman_fixed_to_int (y - pixman_fixed_e - y_off);
    x2 = x1 + cwidth;
    y2 = y1 + cheight;

    srtot = sgtot = sbtot = satot = 0;

    for (i = y1; i < y2; ++i)
    {
	for (j = x1; j < x2; ++j)
	{
	    int rx = j;
	    int ry = i;

	    pixman_fixed_t f = *params;

	    if (f)
	    {
		/* Must be big enough to hold a argb_t */
		argb_t pixel;

		if (repeat_mode != PIXMAN_REPEAT_NONE)
		{
		    repeat (repeat_mode, &rx, width);
		    repeat (repeat_mode, &ry, height);

		    get_pixel (image, rx, ry, FALSE, &pixel);
		}
		else
		{
		    get_pixel (image, rx, ry, TRUE, &pixel);
		}

		accum (&satot, &srtot, &sgtot, &sbtot, &pixel, f);
	    }

	    params++;
	}
    }

    reduce (satot, srtot, sgtot, sbtot, out);
}

static void
bits_image_fetch_pixel_separable_convolution (bits_image_t  *image,
					      pixman_fixed_t x,
					      pixman_fixed_t y,
					      get_pixel_t    get_pixel,
					      void	    *out,
					      accumulate_pixel_t accum,
					      reduce_pixel_t     reduce)
{
    pixman_fixed_t *params = image->common.filter_params;
    pixman_repeat_t repeat_mode = image->common.repeat;
    int width = image->width;
    int height = image->height;
    int cwidth = pixman_fixed_to_int (params[0]);
    int cheight = pixman_fixed_to_int (params[1]);
    int x_phase_bits = pixman_fixed_to_int (params[2]);
    int y_phase_bits = pixman_fixed_to_int (params[3]);
    int x_phase_shift = 16 - x_phase_bits;
    int y_phase_shift = 16 - y_phase_bits;
    int x_off = ((cwidth << 16) - pixman_fixed_1) >> 1;
    int y_off = ((cheight << 16) - pixman_fixed_1) >> 1;
    pixman_fixed_t *y_params;
    unsigned int srtot, sgtot, sbtot, satot;
    int32_t x1, x2, y1, y2;
    int32_t px, py;
    int i, j;

    /* Round x and y to the middle of the closest phase before continuing. This
     * ensures that the convolution matrix is aligned right, since it was
     * positioned relative to a particular phase (and not relative to whatever
     * exact fraction we happen to get here).
     */
    x = ((x >> x_phase_shift) << x_phase_shift) + ((1 << x_phase_shift) >> 1);
    y = ((y >> y_phase_shift) << y_phase_shift) + ((1 << y_phase_shift) >> 1);

    px = (x & 0xffff) >> x_phase_shift;
    py = (y & 0xffff) >> y_phase_shift;

    y_params = params + 4 + (1 << x_phase_bits) * cwidth + py * cheight;

    x1 = pixman_fixed_to_int (x - pixman_fixed_e - x_off);
    y1 = pixman_fixed_to_int (y - pixman_fixed_e - y_off);
    x2 = x1 + cwidth;
    y2 = y1 + cheight;

    srtot = sgtot = sbtot = satot = 0;

    for (i = y1; i < y2; ++i)
    {
        pixman_fixed_48_16_t fy = *y_params++;
        pixman_fixed_t *x_params = params + 4 + px * cwidth;

        if (fy)
        {
            for (j = x1; j < x2; ++j)
            {
                pixman_fixed_t fx = *x_params++;
		int rx = j;
		int ry = i;

                if (fx)
                {
                    /* Must be big enough to hold a argb_t */
                    argb_t pixel;
                    pixman_fixed_t f;

                    if (repeat_mode != PIXMAN_REPEAT_NONE)
                    {
                        repeat (repeat_mode, &rx, width);
                        repeat (repeat_mode, &ry, height);

                        get_pixel (image, rx, ry, FALSE, &pixel);
                    }
                    else
                    {
                        get_pixel (image, rx, ry, TRUE, &pixel);
		    }

                    f = (fy * fx + 0x8000) >> 16;

		    accum(&satot, &srtot, &sgtot, &sbtot, &pixel, f);
                }
            }
	}
    }


    reduce(satot, srtot, sgtot, sbtot, out);
}

static force_inline void
bits_image_fetch_pixel_filtered (bits_image_t  *image,
				 pixman_bool_t  wide,
				 pixman_fixed_t x,
				 pixman_fixed_t y,
				 get_pixel_t    get_pixel,
				 void          *out)
{
    switch (image->common.filter)
    {
    case PIXMAN_FILTER_NEAREST:
    case PIXMAN_FILTER_FAST:
	bits_image_fetch_pixel_nearest (image, x, y, get_pixel, out);
	break;

    case PIXMAN_FILTER_BILINEAR:
    case PIXMAN_FILTER_GOOD:
    case PIXMAN_FILTER_BEST:
	if (wide)
	    bits_image_fetch_pixel_bilinear_float (image, x, y, get_pixel, out);
	else
	    bits_image_fetch_pixel_bilinear_32 (image, x, y, get_pixel, out);
	break;

    case PIXMAN_FILTER_CONVOLUTION:
	if (wide)
	{
	    bits_image_fetch_pixel_convolution (image, x, y,
						get_pixel, out,
						accum_float,
						reduce_float);
	}
	else
	{
	    bits_image_fetch_pixel_convolution (image, x, y,
						get_pixel, out,
						accum_32, reduce_32);
	}
	break;

    case PIXMAN_FILTER_SEPARABLE_CONVOLUTION:
	if (wide)
	{
	    bits_image_fetch_pixel_separable_convolution (image, x, y,
							  get_pixel, out,
							  accum_float,
							  reduce_float);
	}
	else
	{
	    bits_image_fetch_pixel_separable_convolution (image, x, y,
							  get_pixel, out,
							  accum_32, reduce_32);
	}
        break;

    default:
	assert (0);
        break;
    }
}

static uint32_t *
__bits_image_fetch_affine_no_alpha (pixman_iter_t *  iter,
				    pixman_bool_t    wide,
				    const uint32_t * mask)
{
    pixman_image_t *image  = iter->image;
    int             offset = iter->x;
    int             line   = iter->y++;
    int             width  = iter->width;
    uint32_t *      buffer = iter->buffer;

    const uint32_t wide_zero[4] = {0};
    pixman_fixed_t x, y;
    pixman_fixed_t ux, uy;
    pixman_vector_t v;
    int i;
    get_pixel_t get_pixel =
	wide ? fetch_pixel_no_alpha_float : fetch_pixel_no_alpha_32;

    /* reference point is the center of the pixel */
    v.vector[0] = pixman_int_to_fixed (offset) + pixman_fixed_1 / 2;
    v.vector[1] = pixman_int_to_fixed (line) + pixman_fixed_1 / 2;
    v.vector[2] = pixman_fixed_1;

    if (image->common.transform)
    {
	if (!pixman_transform_point_3d (image->common.transform, &v))
	    return iter->buffer;

	ux = image->common.transform->matrix[0][0];
	uy = image->common.transform->matrix[1][0];
    }
    else
    {
	ux = pixman_fixed_1;
	uy = 0;
    }

    x = v.vector[0];
    y = v.vector[1];

    for (i = 0; i < width; ++i)
    {
	if (!mask || (!wide && mask[i]) ||
	    (wide && memcmp(&mask[4 * i], wide_zero, 16) != 0))
	{
	    bits_image_fetch_pixel_filtered (
		&image->bits, wide, x, y, get_pixel, buffer);
	}

	x += ux;
	y += uy;
	buffer += wide ? 4 : 1;
    }

    return iter->buffer;
}

static uint32_t *
bits_image_fetch_affine_no_alpha_32 (pixman_iter_t  *iter,
				     const uint32_t *mask)
{
    return __bits_image_fetch_affine_no_alpha(iter, FALSE, mask);
}

static uint32_t *
bits_image_fetch_affine_no_alpha_float (pixman_iter_t  *iter,
					const uint32_t *mask)
{
    return __bits_image_fetch_affine_no_alpha(iter, TRUE, mask);
}

/* General fetcher */
static force_inline void
fetch_pixel_general_32 (bits_image_t *image,
			int x, int y, pixman_bool_t check_bounds,
			void *out)
{
    uint32_t pixel, *ret = out;

    if (check_bounds &&
	(x < 0 || x >= image->width || y < 0 || y >= image->height))
    {
	*ret = 0;
	return;
    }

    pixel = image->fetch_pixel_32 (image, x, y);

    if (image->common.alpha_map)
    {
	uint32_t pixel_a;

	x -= image->common.alpha_origin_x;
	y -= image->common.alpha_origin_y;

	if (x < 0 || x >= image->common.alpha_map->width ||
	    y < 0 || y >= image->common.alpha_map->height)
	{
	    pixel_a = 0;
	}
	else
	{
	    pixel_a = image->common.alpha_map->fetch_pixel_32 (
		image->common.alpha_map, x, y);

	    pixel_a = ALPHA_8 (pixel_a);
	}

	pixel &= 0x00ffffff;
	pixel |= (pixel_a << 24);
    }

    *ret = pixel;
}

static force_inline void
fetch_pixel_general_float (bits_image_t *image,
			int x, int y, pixman_bool_t check_bounds,
			void *out)
{
    argb_t *ret = out;

    if (check_bounds &&
	(x < 0 || x >= image->width || y < 0 || y >= image->height))
    {
	ret->a = ret->r = ret->g = ret->b = 0;
	return;
    }

    *ret = image->fetch_pixel_float (image, x, y);

    if (image->common.alpha_map)
    {
	x -= image->common.alpha_origin_x;
	y -= image->common.alpha_origin_y;

	if (x < 0 || x >= image->common.alpha_map->width ||
	    y < 0 || y >= image->common.alpha_map->height)
	{
	    ret->a = 0.f;
	}
	else
	{
	    argb_t alpha;

	    alpha = image->common.alpha_map->fetch_pixel_float (
		    image->common.alpha_map, x, y);

	    ret->a = alpha.a;
	}
    }
}

static uint32_t *
__bits_image_fetch_general (pixman_iter_t  *iter,
			    pixman_bool_t wide,
			    const uint32_t *mask)
{
    pixman_image_t *image  = iter->image;
    int             offset = iter->x;
    int             line   = iter->y++;
    int             width  = iter->width;
    uint32_t *      buffer = iter->buffer;
    get_pixel_t     get_pixel =
	wide ? fetch_pixel_general_float : fetch_pixel_general_32;

    const uint32_t wide_zero[4] = {0};
    pixman_fixed_t x, y, w;
    pixman_fixed_t ux, uy, uw;
    pixman_vector_t v;
    int i;

    /* reference point is the center of the pixel */
    v.vector[0] = pixman_int_to_fixed (offset) + pixman_fixed_1 / 2;
    v.vector[1] = pixman_int_to_fixed (line) + pixman_fixed_1 / 2;
    v.vector[2] = pixman_fixed_1;

    if (image->common.transform)
    {
	if (!pixman_transform_point_3d (image->common.transform, &v))
	    return buffer;

	ux = image->common.transform->matrix[0][0];
	uy = image->common.transform->matrix[1][0];
	uw = image->common.transform->matrix[2][0];
    }
    else
    {
	ux = pixman_fixed_1;
	uy = 0;
	uw = 0;
    }

    x = v.vector[0];
    y = v.vector[1];
    w = v.vector[2];

    for (i = 0; i < width; ++i)
    {
	pixman_fixed_t x0, y0;

	if (!mask || (!wide && mask[i]) ||
	    (wide && memcmp(&mask[4 * i], wide_zero, 16) != 0))
	{
	    if (w != 0)
	    {
		x0 = ((uint64_t)x << 16) / w;
		y0 = ((uint64_t)y << 16) / w;
	    }
	    else
	    {
		x0 = 0;
		y0 = 0;
	    }

	    bits_image_fetch_pixel_filtered (
		&image->bits, wide, x0, y0, get_pixel, buffer);
	}

	x += ux;
	y += uy;
	w += uw;
	buffer += wide ? 4 : 1;
    }

    return iter->buffer;
}

static uint32_t *
bits_image_fetch_general_32 (pixman_iter_t  *iter,
			     const uint32_t *mask)
{
    return __bits_image_fetch_general(iter, FALSE, mask);
}

static uint32_t *
bits_image_fetch_general_float (pixman_iter_t  *iter,
				const uint32_t *mask)
{
    return __bits_image_fetch_general(iter, TRUE, mask);
}

static void
replicate_pixel_32 (bits_image_t *   bits,
		    int              x,
		    int              y,
		    int              width,
		    uint32_t *       buffer)
{
    uint32_t color;
    uint32_t *end;

    color = bits->fetch_pixel_32 (bits, x, y);

    end = buffer + width;
    while (buffer < end)
	*(buffer++) = color;
}

static void
replicate_pixel_float (bits_image_t *   bits,
		       int              x,
		       int              y,
		       int              width,
		       uint32_t *       b)
{
    argb_t color;
    argb_t *buffer = (argb_t *)b;
    argb_t *end;

    color = bits->fetch_pixel_float (bits, x, y);

    end = buffer + width;
    while (buffer < end)
	*(buffer++) = color;
}

static void
bits_image_fetch_untransformed_repeat_none (bits_image_t *image,
                                            pixman_bool_t wide,
                                            int           x,
                                            int           y,
                                            int           width,
                                            uint32_t *    buffer)
{
    uint32_t w;

    if (y < 0 || y >= image->height)
    {
	memset (buffer, 0, width * (wide? sizeof (argb_t) : 4));
	return;
    }

    if (x < 0)
    {
	w = MIN (width, -x);

	memset (buffer, 0, w * (wide ? sizeof (argb_t) : 4));

	width -= w;
	buffer += w * (wide? 4 : 1);
	x += w;
    }

    if (x < image->width)
    {
	w = MIN (width, image->width - x);

	if (wide)
	    image->fetch_scanline_float (image, x, y, w, buffer, NULL);
	else
	    image->fetch_scanline_32 (image, x, y, w, buffer, NULL);

	width -= w;
	buffer += w * (wide? 4 : 1);
	x += w;
    }

    memset (buffer, 0, width * (wide ? sizeof (argb_t) : 4));
}

static void
bits_image_fetch_untransformed_repeat_normal (bits_image_t *image,
                                              pixman_bool_t wide,
                                              int           x,
                                              int           y,
                                              int           width,
                                              uint32_t *    buffer)
{
    uint32_t w;

    while (y < 0)
	y += image->height;

    while (y >= image->height)
	y -= image->height;

    if (image->width == 1)
    {
	if (wide)
	    replicate_pixel_float (image, 0, y, width, buffer);
	else
	    replicate_pixel_32 (image, 0, y, width, buffer);

	return;
    }

    while (width)
    {
	while (x < 0)
	    x += image->width;
	while (x >= image->width)
	    x -= image->width;

	w = MIN (width, image->width - x);

	if (wide)
	    image->fetch_scanline_float (image, x, y, w, buffer, NULL);
	else
	    image->fetch_scanline_32 (image, x, y, w, buffer, NULL);

	buffer += w * (wide? 4 : 1);
	x += w;
	width -= w;
    }
}

static uint32_t *
bits_image_fetch_untransformed_32 (pixman_iter_t * iter,
				   const uint32_t *mask)
{
    pixman_image_t *image  = iter->image;
    int             x      = iter->x;
    int             y      = iter->y;
    int             width  = iter->width;
    uint32_t *      buffer = iter->buffer;

    if (image->common.repeat == PIXMAN_REPEAT_NONE)
    {
	bits_image_fetch_untransformed_repeat_none (
	    &image->bits, FALSE, x, y, width, buffer);
    }
    else
    {
	bits_image_fetch_untransformed_repeat_normal (
	    &image->bits, FALSE, x, y, width, buffer);
    }

    iter->y++;
    return buffer;
}

static uint32_t *
bits_image_fetch_untransformed_float (pixman_iter_t * iter,
				      const uint32_t *mask)
{
    pixman_image_t *image  = iter->image;
    int             x      = iter->x;
    int             y      = iter->y;
    int             width  = iter->width;
    uint32_t *      buffer = iter->buffer;

    if (image->common.repeat == PIXMAN_REPEAT_NONE)
    {
	bits_image_fetch_untransformed_repeat_none (
	    &image->bits, TRUE, x, y, width, buffer);
    }
    else
    {
	bits_image_fetch_untransformed_repeat_normal (
	    &image->bits, TRUE, x, y, width, buffer);
    }

    iter->y++;
    return buffer;
}

typedef struct
{
    pixman_format_code_t	format;
    uint32_t			flags;
    pixman_iter_get_scanline_t	get_scanline_32;
    pixman_iter_get_scanline_t  get_scanline_float;
} fetcher_info_t;

static const fetcher_info_t fetcher_info[] =
{
    { PIXMAN_any,
      (FAST_PATH_NO_ALPHA_MAP			|
       FAST_PATH_ID_TRANSFORM			|
       FAST_PATH_NO_CONVOLUTION_FILTER		|
       FAST_PATH_NO_PAD_REPEAT			|
       FAST_PATH_NO_REFLECT_REPEAT),
      bits_image_fetch_untransformed_32,
      bits_image_fetch_untransformed_float
    },

    /* Affine, no alpha */
    { PIXMAN_any,
      (FAST_PATH_NO_ALPHA_MAP | FAST_PATH_HAS_TRANSFORM | FAST_PATH_AFFINE_TRANSFORM),
      bits_image_fetch_affine_no_alpha_32,
      bits_image_fetch_affine_no_alpha_float,
    },

    /* General */
    { PIXMAN_any,
      0,
      bits_image_fetch_general_32,
      bits_image_fetch_general_float,
    },

    { PIXMAN_null },
};

static void
bits_image_property_changed (pixman_image_t *image)
{
    _pixman_bits_image_setup_accessors (&image->bits);
}

void
_pixman_bits_image_src_iter_init (pixman_image_t *image, pixman_iter_t *iter)
{
    pixman_format_code_t format = image->common.extended_format_code;
    uint32_t flags = image->common.flags;
    const fetcher_info_t *info;

    for (info = fetcher_info; info->format != PIXMAN_null; ++info)
    {
	if ((info->format == format || info->format == PIXMAN_any)	&&
	    (info->flags & flags) == info->flags)
	{
	    if (iter->iter_flags & ITER_NARROW)
	    {
		iter->get_scanline = info->get_scanline_32;
	    }
	    else
	    {
		iter->get_scanline = info->get_scanline_float;
	    }
	    return;
	}
    }

    /* Just in case we somehow didn't find a scanline function */
    iter->get_scanline = _pixman_iter_get_scanline_noop;
}

static uint32_t *
dest_get_scanline_narrow (pixman_iter_t *iter, const uint32_t *mask)
{
    pixman_image_t *image  = iter->image;
    int             x      = iter->x;
    int             y      = iter->y;
    int             width  = iter->width;
    uint32_t *	    buffer = iter->buffer;

    image->bits.fetch_scanline_32 (&image->bits, x, y, width, buffer, mask);
    if (image->common.alpha_map)
    {
	uint32_t *alpha;

	if ((alpha = malloc (width * sizeof (uint32_t))))
	{
	    int i;

	    x -= image->common.alpha_origin_x;
	    y -= image->common.alpha_origin_y;

	    image->common.alpha_map->fetch_scanline_32 (
		image->common.alpha_map, x, y, width, alpha, mask);

	    for (i = 0; i < width; ++i)
	    {
		buffer[i] &= ~0xff000000;
		buffer[i] |= (alpha[i] & 0xff000000);
	    }

	    free (alpha);
	}
    }

    return iter->buffer;
}

static uint32_t *
dest_get_scanline_wide (pixman_iter_t *iter, const uint32_t *mask)
{
    bits_image_t *  image  = &iter->image->bits;
    int             x      = iter->x;
    int             y      = iter->y;
    int             width  = iter->width;
    argb_t *	    buffer = (argb_t *)iter->buffer;

    image->fetch_scanline_float (
	image, x, y, width, (uint32_t *)buffer, mask);
    if (image->common.alpha_map)
    {
	argb_t *alpha;

	if ((alpha = malloc (width * sizeof (argb_t))))
	{
	    int i;

	    x -= image->common.alpha_origin_x;
	    y -= image->common.alpha_origin_y;

	    image->common.alpha_map->fetch_scanline_float (
		image->common.alpha_map, x, y, width, (uint32_t *)alpha, mask);

	    for (i = 0; i < width; ++i)
		buffer[i].a = alpha[i].a;

	    free (alpha);
	}
    }

    return iter->buffer;
}

static void
dest_write_back_narrow (pixman_iter_t *iter)
{
    bits_image_t *  image  = &iter->image->bits;
    int             x      = iter->x;
    int             y      = iter->y;
    int             width  = iter->width;
    const uint32_t *buffer = iter->buffer;

    image->store_scanline_32 (image, x, y, width, buffer);

    if (image->common.alpha_map)
    {
	x -= image->common.alpha_origin_x;
	y -= image->common.alpha_origin_y;

	image->common.alpha_map->store_scanline_32 (
	    image->common.alpha_map, x, y, width, buffer);
    }

    iter->y++;
}

static float
dither_factor_blue_noise_64 (int x, int y)
{
    float m = dither_blue_noise_64x64[((y & 0x3f) << 6) | (x & 0x3f)];
    return m * (1. / 4096.f) + (1. / 8192.f);
}

static float
dither_factor_bayer_8 (int x, int y)
{
    uint32_t m;

    y ^= x;

    /* Compute reverse(interleave(xor(x mod n, y mod n), x mod n))
     * Here n = 8 and `mod n` is the bottom 3 bits.
     */
    m = ((y & 0x1) << 5) | ((x & 0x1) << 4) |
	((y & 0x2) << 2) | ((x & 0x2) << 1) |
	((y & 0x4) >> 1) | ((x & 0x4) >> 2);

    /* m is in range [0, 63].  We scale it to [0, 63.0f/64.0f], then
     * shift it to to [1.0f/128.0f, 127.0f/128.0f] so that 0 < d < 1.
     * This ensures exact values are not changed by dithering.
     */
    return (float)(m) * (1 / 64.0f) + (1.0f / 128.0f);
}

typedef float (* dither_factor_t)(int x, int y);

static force_inline float
dither_apply_channel (float f, float d, float s)
{
    /* float_to_unorm splits the [0, 1] segment in (1 << n_bits)
     * subsections of equal length; however unorm_to_float does not
     * map to the center of those sections.  In fact, pixel value u is
     * mapped to:
     *
     *       u              u              u               1
     * -------------- = ---------- + -------------- * ----------
     *  2^n_bits - 1     2^n_bits     2^n_bits - 1     2^n_bits
     *
     * Hence if f = u / (2^n_bits - 1) is exactly representable on a
     * n_bits palette, all the numbers between
     *
     *     u
     * ----------  =  f - f * 2^n_bits = f + (0 - f) * 2^n_bits
     *  2^n_bits
     *
     *  and
     *
     *    u + 1
     * ---------- = f - (f - 1) * 2^n_bits = f + (1 - f) * 2^n_bits
     *  2^n_bits
     *
     * are also mapped back to u.
     *
     * Hence the following calculation ensures that we add as much
     * noise as possible without perturbing values which are exactly
     * representable in the target colorspace.  Note that this corresponds to
     * mixing the original color with noise with a ratio of `1 / 2^n_bits`.
     */
    return f + (d - f) * s;
}

static force_inline float
dither_compute_scale (int n_bits)
{
    // No dithering for wide formats
    if (n_bits == 0 || n_bits >= 32)
	return 0.f;

    return 1.f / (float)(1 << n_bits);
}

static const uint32_t *
dither_apply_ordered (pixman_iter_t *iter, dither_factor_t factor)
{
    bits_image_t        *image  = &iter->image->bits;
    int                  x      = iter->x + image->dither_offset_x;
    int                  y      = iter->y + image->dither_offset_y;
    int                  width  = iter->width;
    argb_t              *buffer = (argb_t *)iter->buffer;

    pixman_format_code_t format = image->format;
    int                  a_size = PIXMAN_FORMAT_A (format);
    int                  r_size = PIXMAN_FORMAT_R (format);
    int                  g_size = PIXMAN_FORMAT_G (format);
    int                  b_size = PIXMAN_FORMAT_B (format);

    float a_scale = dither_compute_scale (a_size);
    float r_scale = dither_compute_scale (r_size);
    float g_scale = dither_compute_scale (g_size);
    float b_scale = dither_compute_scale (b_size);

    int   i;
    float d;

    for (i = 0; i < width; ++i)
    {
	d = factor (x + i, y);

	buffer->a = dither_apply_channel (buffer->a, d, a_scale);
	buffer->r = dither_apply_channel (buffer->r, d, r_scale);
	buffer->g = dither_apply_channel (buffer->g, d, g_scale);
	buffer->b = dither_apply_channel (buffer->b, d, b_scale);

	buffer++;
    }

    return iter->buffer;
}

static void
dest_write_back_wide (pixman_iter_t *iter)
{
    bits_image_t *  image  = &iter->image->bits;
    int             x      = iter->x;
    int             y      = iter->y;
    int             width  = iter->width;
    const uint32_t *buffer = iter->buffer;

    switch (image->dither)
    {
    case PIXMAN_DITHER_NONE:
	break;

    case PIXMAN_DITHER_GOOD:
    case PIXMAN_DITHER_BEST:
    case PIXMAN_DITHER_ORDERED_BLUE_NOISE_64:
	buffer = dither_apply_ordered (iter, dither_factor_blue_noise_64);
	break;

    case PIXMAN_DITHER_FAST:
    case PIXMAN_DITHER_ORDERED_BAYER_8:
	buffer = dither_apply_ordered (iter, dither_factor_bayer_8);
	break;
    }

    image->store_scanline_float (image, x, y, width, buffer);

    if (image->common.alpha_map)
    {
	x -= image->common.alpha_origin_x;
	y -= image->common.alpha_origin_y;

	image->common.alpha_map->store_scanline_float (
	    image->common.alpha_map, x, y, width, buffer);
    }

    iter->y++;
}

void
_pixman_bits_image_dest_iter_init (pixman_image_t *image, pixman_iter_t *iter)
{
    if (iter->iter_flags & ITER_NARROW)
    {
	if ((iter->iter_flags & (ITER_IGNORE_RGB | ITER_IGNORE_ALPHA)) ==
	    (ITER_IGNORE_RGB | ITER_IGNORE_ALPHA))
	{
	    iter->get_scanline = _pixman_iter_get_scanline_noop;
	}
	else
	{
	    iter->get_scanline = dest_get_scanline_narrow;
	}
	
	iter->write_back = dest_write_back_narrow;
    }
    else
    {
	iter->get_scanline = dest_get_scanline_wide;
	iter->write_back = dest_write_back_wide;
    }
}

static uint32_t *
create_bits (pixman_format_code_t format,
             int                  width,
             int                  height,
             int *		  rowstride_bytes,
	     pixman_bool_t	  clear)
{
    int stride;
    size_t buf_size;
    int bpp;

    /* what follows is a long-winded way, avoiding any possibility of integer
     * overflows, of saying:
     * stride = ((width * bpp + 0x1f) >> 5) * sizeof (uint32_t);
     */

    bpp = PIXMAN_FORMAT_BPP (format);
    if (_pixman_multiply_overflows_int (width, bpp))
	return NULL;

    stride = width * bpp;
    if (_pixman_addition_overflows_int (stride, 0x1f))
	return NULL;

    stride += 0x1f;
    stride >>= 5;

    stride *= sizeof (uint32_t);

    if (_pixman_multiply_overflows_size (height, stride))
	return NULL;

    buf_size = (size_t)height * stride;

    if (rowstride_bytes)
	*rowstride_bytes = stride;

    if (clear)
	return calloc (buf_size, 1);
    else
	return malloc (buf_size);
}

pixman_bool_t
_pixman_bits_image_init (pixman_image_t *     image,
                         pixman_format_code_t format,
                         int                  width,
                         int                  height,
                         uint32_t *           bits,
                         int                  rowstride,
			 pixman_bool_t	      clear)
{
    uint32_t *free_me = NULL;

    if (PIXMAN_FORMAT_BPP (format) == 128)
	return_val_if_fail(!(rowstride % 4), FALSE);

    if (!bits && width && height)
    {
	int rowstride_bytes;

	free_me = bits = create_bits (format, width, height, &rowstride_bytes, clear);

	if (!bits)
	    return FALSE;

	rowstride = rowstride_bytes / (int) sizeof (uint32_t);
    }

    _pixman_image_init (image);

    image->type = BITS;
    image->bits.format = format;
    image->bits.width = width;
    image->bits.height = height;
    image->bits.bits = bits;
    image->bits.free_me = free_me;
    image->bits.dither = PIXMAN_DITHER_NONE;
    image->bits.dither_offset_x = 0;
    image->bits.dither_offset_y = 0;
    image->bits.read_func = NULL;
    image->bits.write_func = NULL;
    image->bits.rowstride = rowstride;
    image->bits.indexed = NULL;

    image->common.property_changed = bits_image_property_changed;

    _pixman_image_reset_clip_region (image);

    return TRUE;
}

static pixman_image_t *
create_bits_image_internal (pixman_format_code_t format,
			    int                  width,
			    int                  height,
			    uint32_t *           bits,
			    int                  rowstride_bytes,
			    pixman_bool_t	 clear)
{
    pixman_image_t *image;

    /* must be a whole number of uint32_t's
     */
    return_val_if_fail (
	bits == NULL || (rowstride_bytes % sizeof (uint32_t)) == 0, NULL);

    return_val_if_fail (PIXMAN_FORMAT_BPP (format) >= PIXMAN_FORMAT_DEPTH (format), NULL);

    image = _pixman_image_allocate ();

    if (!image)
	return NULL;

    if (!_pixman_bits_image_init (image, format, width, height, bits,
				  rowstride_bytes / (int) sizeof (uint32_t),
				  clear))
    {
	free (image);
	return NULL;
    }

    return image;
}

/* If bits is NULL, a buffer will be allocated and initialized to 0 */
PIXMAN_EXPORT pixman_image_t *
pixman_image_create_bits (pixman_format_code_t format,
                          int                  width,
                          int                  height,
                          uint32_t *           bits,
                          int                  rowstride_bytes)
{
    return create_bits_image_internal (
	format, width, height, bits, rowstride_bytes, TRUE);
}


/* If bits is NULL, a buffer will be allocated and _not_ initialized */
PIXMAN_EXPORT pixman_image_t *
pixman_image_create_bits_no_clear (pixman_format_code_t format,
				   int                  width,
				   int                  height,
				   uint32_t *           bits,
				   int                  rowstride_bytes)
{
    return create_bits_image_internal (
	format, width, height, bits, rowstride_bytes, FALSE);
}
