/*
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *             2005 Lars Knoll & Zack Rusin, Trolltech
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
#include <pixman-config.h>
#endif
#include "pixman-private.h"

void
_pixman_gradient_walker_init (pixman_gradient_walker_t *walker,
                              gradient_t *              gradient,
                              pixman_repeat_t		repeat)
{
    walker->num_stops = gradient->n_stops;
    walker->stops     = gradient->stops;
    walker->left_x    = 0;
    walker->right_x   = 0x10000;
    walker->a_s       = 0.0f;
    walker->a_b       = 0.0f;
    walker->r_s       = 0.0f;
    walker->r_b       = 0.0f;
    walker->g_s       = 0.0f;
    walker->g_b       = 0.0f;
    walker->b_s       = 0.0f;
    walker->b_b       = 0.0f;
    walker->repeat    = repeat;

    walker->need_reset = TRUE;
}

static void
gradient_walker_reset (pixman_gradient_walker_t *walker,
		       pixman_fixed_48_16_t      pos)
{
    int64_t x, left_x, right_x;
    pixman_color_t *left_c, *right_c;
    int n, count = walker->num_stops;
    pixman_gradient_stop_t *stops = walker->stops;
    float la, lr, lg, lb;
    float ra, rr, rg, rb;
    float lx, rx;

    if (walker->repeat == PIXMAN_REPEAT_NORMAL)
    {
	x = (int32_t)pos & 0xffff;
    }
    else if (walker->repeat == PIXMAN_REPEAT_REFLECT)
    {
	x = (int32_t)pos & 0xffff;
	if ((int32_t)pos & 0x10000)
	    x = 0x10000 - x;
    }
    else
    {
	x = pos;
    }
    
    for (n = 0; n < count; n++)
    {
	if (x < stops[n].x)
	    break;
    }
    
    left_x =  stops[n - 1].x;
    left_c = &stops[n - 1].color;
    
    right_x =  stops[n].x;
    right_c = &stops[n].color;

    if (walker->repeat == PIXMAN_REPEAT_NORMAL)
    {
	left_x  += (pos - x);
	right_x += (pos - x);
    }
    else if (walker->repeat == PIXMAN_REPEAT_REFLECT)
    {
	if ((int32_t)pos & 0x10000)
	{
	    pixman_color_t  *tmp_c;
	    int32_t tmp_x;

	    tmp_x   = 0x10000 - right_x;
	    right_x = 0x10000 - left_x;
	    left_x  = tmp_x;

	    tmp_c   = right_c;
	    right_c = left_c;
	    left_c  = tmp_c;

	    x = 0x10000 - x;
	}
	left_x  += (pos - x);
	right_x += (pos - x);
    }
    else if (walker->repeat == PIXMAN_REPEAT_NONE)
    {
	if (n == 0)
	    right_c = left_c;
	else if (n == count)
	    left_c = right_c;
    }

    /* The alpha/red/green/blue channels are scaled to be in [0, 1].
     * This ensures that after premultiplication all channels will
     * be in the [0, 1] interval.
     */
    la = (left_c->alpha * (1.0f/257.0f));
    lr = (left_c->red * (1.0f/257.0f));
    lg = (left_c->green * (1.0f/257.0f));
    lb = (left_c->blue * (1.0f/257.0f));

    ra = (right_c->alpha * (1.0f/257.0f));
    rr = (right_c->red * (1.0f/257.0f));
    rg = (right_c->green * (1.0f/257.0f));
    rb = (right_c->blue * (1.0f/257.0f));
    
    lx = left_x * (1.0f/65536.0f);
    rx = right_x * (1.0f/65536.0f);
    
    if (FLOAT_IS_ZERO (rx - lx) || left_x == INT32_MIN || right_x == INT32_MAX)
    {
	walker->a_s = walker->r_s = walker->g_s = walker->b_s = 0.0f;
	walker->a_b = (la + ra) / 510.0f;
	walker->r_b = (lr + rr) / 510.0f;
	walker->g_b = (lg + rg) / 510.0f;
	walker->b_b = (lb + rb) / 510.0f;
    }
    else
    {
	float w_rec = 1.0f / (rx - lx);

	walker->a_b = (la * rx - ra * lx) * w_rec * (1.0f/255.0f);
	walker->r_b = (lr * rx - rr * lx) * w_rec * (1.0f/255.0f);
	walker->g_b = (lg * rx - rg * lx) * w_rec * (1.0f/255.0f);
	walker->b_b = (lb * rx - rb * lx) * w_rec * (1.0f/255.0f);

	walker->a_s = (ra - la) * w_rec * (1.0f/255.0f);
	walker->r_s = (rr - lr) * w_rec * (1.0f/255.0f);
	walker->g_s = (rg - lg) * w_rec * (1.0f/255.0f);
	walker->b_s = (rb - lb) * w_rec * (1.0f/255.0f);
    }
   
    walker->left_x = left_x;
    walker->right_x = right_x;

    walker->need_reset = FALSE;
}

static argb_t
pixman_gradient_walker_pixel_float (pixman_gradient_walker_t *walker,
				    pixman_fixed_48_16_t      x)
{
    argb_t f;
    float y;

    if (walker->need_reset || x < walker->left_x || x >= walker->right_x)
	gradient_walker_reset (walker, x);

    y = x * (1.0f / 65536.0f);

    f.a = walker->a_s * y + walker->a_b;
    f.r = f.a * (walker->r_s * y + walker->r_b);
    f.g = f.a * (walker->g_s * y + walker->g_b);
    f.b = f.a * (walker->b_s * y + walker->b_b);

    return f;
}

static uint32_t
pixman_gradient_walker_pixel_32 (pixman_gradient_walker_t *walker,
				 pixman_fixed_48_16_t      x)
{
    argb_t f;
    float y;

    if (walker->need_reset || x < walker->left_x || x >= walker->right_x)
	gradient_walker_reset (walker, x);

    y = x * (1.0f / 65536.0f);

    /* Instead of [0...1] for ARGB, we want [0...255],
     * multiply alpha with 255 and the color channels
     * also get multiplied by the alpha multiplier.
     *
     * We don't use pixman_contract_from_float because it causes a 2x
     * slowdown to do so, and the values are already normalized,
     * so we don't have to worry about values < 0.f or > 1.f
     */
    f.a = 255.f * (walker->a_s * y + walker->a_b);
    f.r = f.a * (walker->r_s * y + walker->r_b);
    f.g = f.a * (walker->g_s * y + walker->g_b);
    f.b = f.a * (walker->b_s * y + walker->b_b);

    return (((uint32_t)(f.a + .5f) << 24) & 0xff000000) |
           (((uint32_t)(f.r + .5f) << 16) & 0x00ff0000) |
           (((uint32_t)(f.g + .5f) <<  8) & 0x0000ff00) |
           (((uint32_t)(f.b + .5f) >>  0) & 0x000000ff);
}

void
_pixman_gradient_walker_write_narrow (pixman_gradient_walker_t *walker,
				      pixman_fixed_48_16_t      x,
				      uint32_t                 *buffer)
{
    *buffer = pixman_gradient_walker_pixel_32 (walker, x);
}

void
_pixman_gradient_walker_write_wide (pixman_gradient_walker_t *walker,
				    pixman_fixed_48_16_t      x,
				    uint32_t                 *buffer)
{
    *(argb_t *)buffer = pixman_gradient_walker_pixel_float (walker, x);
}

void
_pixman_gradient_walker_fill_narrow (pixman_gradient_walker_t *walker,
				     pixman_fixed_48_16_t      x,
				     uint32_t                 *buffer,
				     uint32_t                 *end)
{
    register uint32_t color;

    color = pixman_gradient_walker_pixel_32 (walker, x);
    while (buffer < end)
	*buffer++ = color;
}

void
_pixman_gradient_walker_fill_wide (pixman_gradient_walker_t *walker,
				   pixman_fixed_48_16_t      x,
				   uint32_t                 *buffer,
				   uint32_t                 *end)
{
    register argb_t color;
    argb_t *buffer_wide = (argb_t *)buffer;
    argb_t *end_wide    = (argb_t *)end;

    color = pixman_gradient_walker_pixel_float (walker, x);
    while (buffer_wide < end_wide)
	*buffer_wide++ = color;
}
