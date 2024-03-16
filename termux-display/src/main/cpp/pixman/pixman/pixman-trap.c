/*
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 * Copyright © 2004 Keith Packard
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
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "pixman-private.h"

/*
 * Compute the smallest value greater than or equal to y which is on a
 * grid row.
 */

PIXMAN_EXPORT pixman_fixed_t
pixman_sample_ceil_y (pixman_fixed_t y, int n)
{
    pixman_fixed_t f = pixman_fixed_frac (y);
    pixman_fixed_t i = pixman_fixed_floor (y);

    f = DIV (f - Y_FRAC_FIRST (n) + (STEP_Y_SMALL (n) - pixman_fixed_e), STEP_Y_SMALL (n)) * STEP_Y_SMALL (n) +
	Y_FRAC_FIRST (n);
    
    if (f > Y_FRAC_LAST (n))
    {
	if (pixman_fixed_to_int (i) == 0x7fff)
	{
	    f = 0xffff; /* saturate */
	}
	else
	{
	    f = Y_FRAC_FIRST (n);
	    i += pixman_fixed_1;
	}
    }
    return (i | f);
}

/*
 * Compute the largest value strictly less than y which is on a
 * grid row.
 */
PIXMAN_EXPORT pixman_fixed_t
pixman_sample_floor_y (pixman_fixed_t y,
                       int            n)
{
    pixman_fixed_t f = pixman_fixed_frac (y);
    pixman_fixed_t i = pixman_fixed_floor (y);

    f = DIV (f - pixman_fixed_e - Y_FRAC_FIRST (n), STEP_Y_SMALL (n)) * STEP_Y_SMALL (n) +
	Y_FRAC_FIRST (n);

    if (f < Y_FRAC_FIRST (n))
    {
	if (pixman_fixed_to_int (i) == 0xffff8000)
	{
	    f = 0; /* saturate */
	}
	else
	{
	    f = Y_FRAC_LAST (n);
	    i -= pixman_fixed_1;
	}
    }
    return (i | f);
}

/*
 * Step an edge by any amount (including negative values)
 */
PIXMAN_EXPORT void
pixman_edge_step (pixman_edge_t *e,
                  int            n)
{
    pixman_fixed_48_16_t ne;

    e->x += n * e->stepx;

    ne = e->e + n * (pixman_fixed_48_16_t) e->dx;

    if (n >= 0)
    {
	if (ne > 0)
	{
	    int nx = (ne + e->dy - 1) / e->dy;
	    e->e = ne - nx * (pixman_fixed_48_16_t) e->dy;
	    e->x += nx * e->signdx;
	}
    }
    else
    {
	if (ne <= -e->dy)
	{
	    int nx = (-ne) / e->dy;
	    e->e = ne + nx * (pixman_fixed_48_16_t) e->dy;
	    e->x -= nx * e->signdx;
	}
    }
}

/*
 * A private routine to initialize the multi-step
 * elements of an edge structure
 */
static void
_pixman_edge_multi_init (pixman_edge_t * e,
                         int             n,
                         pixman_fixed_t *stepx_p,
                         pixman_fixed_t *dx_p)
{
    pixman_fixed_t stepx;
    pixman_fixed_48_16_t ne;

    ne = n * (pixman_fixed_48_16_t) e->dx;
    stepx = n * e->stepx;

    if (ne > 0)
    {
	int nx = ne / e->dy;
	ne -= nx * (pixman_fixed_48_16_t)e->dy;
	stepx += nx * e->signdx;
    }

    *dx_p = ne;
    *stepx_p = stepx;
}

/*
 * Initialize one edge structure given the line endpoints and a
 * starting y value
 */
PIXMAN_EXPORT void
pixman_edge_init (pixman_edge_t *e,
                  int            n,
                  pixman_fixed_t y_start,
                  pixman_fixed_t x_top,
                  pixman_fixed_t y_top,
                  pixman_fixed_t x_bot,
                  pixman_fixed_t y_bot)
{
    pixman_fixed_t dx, dy;

    e->x = x_top;
    e->e = 0;
    dx = x_bot - x_top;
    dy = y_bot - y_top;
    e->dy = dy;
    e->dx = 0;

    if (dy)
    {
	if (dx >= 0)
	{
	    e->signdx = 1;
	    e->stepx = dx / dy;
	    e->dx = dx % dy;
	    e->e = -dy;
	}
	else
	{
	    e->signdx = -1;
	    e->stepx = -(-dx / dy);
	    e->dx = -dx % dy;
	    e->e = 0;
	}

	_pixman_edge_multi_init (e, STEP_Y_SMALL (n),
				 &e->stepx_small, &e->dx_small);

	_pixman_edge_multi_init (e, STEP_Y_BIG (n),
				 &e->stepx_big, &e->dx_big);
    }
    pixman_edge_step (e, y_start - y_top);
}

/*
 * Initialize one edge structure given a line, starting y value
 * and a pixel offset for the line
 */
PIXMAN_EXPORT void
pixman_line_fixed_edge_init (pixman_edge_t *            e,
                             int                        n,
                             pixman_fixed_t             y,
                             const pixman_line_fixed_t *line,
                             int                        x_off,
                             int                        y_off)
{
    pixman_fixed_t x_off_fixed = pixman_int_to_fixed (x_off);
    pixman_fixed_t y_off_fixed = pixman_int_to_fixed (y_off);
    const pixman_point_fixed_t *top, *bot;

    if (line->p1.y <= line->p2.y)
    {
	top = &line->p1;
	bot = &line->p2;
    }
    else
    {
	top = &line->p2;
	bot = &line->p1;
    }
    
    pixman_edge_init (e, n, y,
                      top->x + x_off_fixed,
                      top->y + y_off_fixed,
                      bot->x + x_off_fixed,
                      bot->y + y_off_fixed);
}

PIXMAN_EXPORT void
pixman_add_traps (pixman_image_t *     image,
                  int16_t              x_off,
                  int16_t              y_off,
                  int                  ntrap,
                  const pixman_trap_t *traps)
{
    int bpp;
    int height;

    pixman_fixed_t x_off_fixed;
    pixman_fixed_t y_off_fixed;
    pixman_edge_t l, r;
    pixman_fixed_t t, b;

    _pixman_image_validate (image);
    
    height = image->bits.height;
    bpp = PIXMAN_FORMAT_BPP (image->bits.format);

    x_off_fixed = pixman_int_to_fixed (x_off);
    y_off_fixed = pixman_int_to_fixed (y_off);

    while (ntrap--)
    {
	t = traps->top.y + y_off_fixed;
	if (t < 0)
	    t = 0;
	t = pixman_sample_ceil_y (t, bpp);

	b = traps->bot.y + y_off_fixed;
	if (pixman_fixed_to_int (b) >= height)
	    b = pixman_int_to_fixed (height) - 1;
	b = pixman_sample_floor_y (b, bpp);

	if (b >= t)
	{
	    /* initialize edge walkers */
	    pixman_edge_init (&l, bpp, t,
	                      traps->top.l + x_off_fixed,
	                      traps->top.y + y_off_fixed,
	                      traps->bot.l + x_off_fixed,
	                      traps->bot.y + y_off_fixed);

	    pixman_edge_init (&r, bpp, t,
	                      traps->top.r + x_off_fixed,
	                      traps->top.y + y_off_fixed,
	                      traps->bot.r + x_off_fixed,
	                      traps->bot.y + y_off_fixed);

	    pixman_rasterize_edges (image, &l, &r, t, b);
	}

	traps++;
    }
}

#if 0
static void
dump_image (pixman_image_t *image,
            const char *    title)
{
    int i, j;

    if (!image->type == BITS)
	printf ("%s is not a regular image\n", title);

    if (!image->bits.format == PIXMAN_a8)
	printf ("%s is not an alpha mask\n", title);

    printf ("\n\n\n%s: \n", title);

    for (i = 0; i < image->bits.height; ++i)
    {
	uint8_t *line =
	    (uint8_t *)&(image->bits.bits[i * image->bits.rowstride]);

	for (j = 0; j < image->bits.width; ++j)
	    printf ("%c", line[j] ? '#' : ' ');

	printf ("\n");
    }
}
#endif

PIXMAN_EXPORT void
pixman_add_trapezoids (pixman_image_t *          image,
                       int16_t                   x_off,
                       int                       y_off,
                       int                       ntraps,
                       const pixman_trapezoid_t *traps)
{
    int i;

#if 0
    dump_image (image, "before");
#endif

    for (i = 0; i < ntraps; ++i)
    {
	const pixman_trapezoid_t *trap = &(traps[i]);

	if (!pixman_trapezoid_valid (trap))
	    continue;

	pixman_rasterize_trapezoid (image, trap, x_off, y_off);
    }

#if 0
    dump_image (image, "after");
#endif
}

PIXMAN_EXPORT void
pixman_rasterize_trapezoid (pixman_image_t *          image,
                            const pixman_trapezoid_t *trap,
                            int                       x_off,
                            int                       y_off)
{
    int bpp;
    int height;

    pixman_fixed_t y_off_fixed;
    pixman_edge_t l, r;
    pixman_fixed_t t, b;

    return_if_fail (image->type == BITS);

    _pixman_image_validate (image);
    
    if (!pixman_trapezoid_valid (trap))
	return;

    height = image->bits.height;
    bpp = PIXMAN_FORMAT_BPP (image->bits.format);

    y_off_fixed = pixman_int_to_fixed (y_off);

    t = trap->top + y_off_fixed;
    if (t < 0)
	t = 0;
    t = pixman_sample_ceil_y (t, bpp);

    b = trap->bottom + y_off_fixed;
    if (pixman_fixed_to_int (b) >= height)
	b = pixman_int_to_fixed (height) - 1;
    b = pixman_sample_floor_y (b, bpp);
    
    if (b >= t)
    {
	/* initialize edge walkers */
	pixman_line_fixed_edge_init (&l, bpp, t, &trap->left, x_off, y_off);
	pixman_line_fixed_edge_init (&r, bpp, t, &trap->right, x_off, y_off);

	pixman_rasterize_edges (image, &l, &r, t, b);
    }
}

static const pixman_bool_t zero_src_has_no_effect[PIXMAN_N_OPERATORS] =
{
    FALSE,	/* Clear		0			0    */
    FALSE,	/* Src			1			0    */
    TRUE,	/* Dst			0			1    */
    TRUE,	/* Over			1			1-Aa */
    TRUE,	/* OverReverse		1-Ab			1    */
    FALSE,	/* In			Ab			0    */
    FALSE,	/* InReverse		0			Aa   */
    FALSE,	/* Out			1-Ab			0    */
    TRUE,	/* OutReverse		0			1-Aa */
    TRUE,	/* Atop			Ab			1-Aa */
    FALSE,	/* AtopReverse		1-Ab			Aa   */
    TRUE,	/* Xor			1-Ab			1-Aa */
    TRUE,	/* Add			1			1    */
};

static pixman_bool_t
get_trap_extents (pixman_op_t op, pixman_image_t *dest,
		  const pixman_trapezoid_t *traps, int n_traps,
		  pixman_box32_t *box)
{
    int i;

    /* When the operator is such that a zero source has an
     * effect on the underlying image, we have to
     * composite across the entire destination
     */
    if (!zero_src_has_no_effect [op])
    {
	box->x1 = 0;
	box->y1 = 0;
	box->x2 = dest->bits.width;
	box->y2 = dest->bits.height;
	return TRUE;
    }
    
    box->x1 = INT32_MAX;
    box->y1 = INT32_MAX;
    box->x2 = INT32_MIN;
    box->y2 = INT32_MIN;
	
    for (i = 0; i < n_traps; ++i)
    {
	const pixman_trapezoid_t *trap = &(traps[i]);
	int y1, y2;
	    
	if (!pixman_trapezoid_valid (trap))
	    continue;
	    
	y1 = pixman_fixed_to_int (trap->top);
	if (y1 < box->y1)
	    box->y1 = y1;
	    
	y2 = pixman_fixed_to_int (pixman_fixed_ceil (trap->bottom));
	if (y2 > box->y2)
	    box->y2 = y2;
	    
#define EXTEND_MIN(x)							\
	if (pixman_fixed_to_int ((x)) < box->x1)			\
	    box->x1 = pixman_fixed_to_int ((x));
#define EXTEND_MAX(x)							\
	if (pixman_fixed_to_int (pixman_fixed_ceil ((x))) > box->x2)	\
	    box->x2 = pixman_fixed_to_int (pixman_fixed_ceil ((x)));
	    
#define EXTEND(x)							\
	EXTEND_MIN(x);							\
	EXTEND_MAX(x);
	    
	EXTEND(trap->left.p1.x);
	EXTEND(trap->left.p2.x);
	EXTEND(trap->right.p1.x);
	EXTEND(trap->right.p2.x);
    }
	
    if (box->x1 >= box->x2 || box->y1 >= box->y2)
	return FALSE;

    return TRUE;
}

/*
 * pixman_composite_trapezoids()
 *
 * All the trapezoids are conceptually rendered to an infinitely big image.
 * The (0, 0) coordinates of this image are then aligned with the (x, y)
 * coordinates of the source image, and then both images are aligned with
 * the (x, y) coordinates of the destination. Then these three images are
 * composited across the entire destination.
 */
PIXMAN_EXPORT void
pixman_composite_trapezoids (pixman_op_t		op,
			     pixman_image_t *		src,
			     pixman_image_t *		dst,
			     pixman_format_code_t	mask_format,
			     int			x_src,
			     int			y_src,
			     int			x_dst,
			     int			y_dst,
			     int			n_traps,
			     const pixman_trapezoid_t *	traps)
{
    int i;

    return_if_fail (PIXMAN_FORMAT_TYPE (mask_format) == PIXMAN_TYPE_A);
    
    if (n_traps <= 0)
	return;

    _pixman_image_validate (src);
    _pixman_image_validate (dst);

    if (op == PIXMAN_OP_ADD &&
	(src->common.flags & FAST_PATH_IS_OPAQUE)		&&
	(mask_format == dst->common.extended_format_code)	&&
	!(dst->common.have_clip_region))
    {
	for (i = 0; i < n_traps; ++i)
	{
	    const pixman_trapezoid_t *trap = &(traps[i]);
	    
	    if (!pixman_trapezoid_valid (trap))
		continue;
	    
	    pixman_rasterize_trapezoid (dst, trap, x_dst, y_dst);
	}
    }
    else
    {
	pixman_image_t *tmp;
	pixman_box32_t box;
	int i;

	if (!get_trap_extents (op, dst, traps, n_traps, &box))
	    return;
	
	if (!(tmp = pixman_image_create_bits (
		  mask_format, box.x2 - box.x1, box.y2 - box.y1, NULL, -1)))
	    return;
	
	for (i = 0; i < n_traps; ++i)
	{
	    const pixman_trapezoid_t *trap = &(traps[i]);
	    
	    if (!pixman_trapezoid_valid (trap))
		continue;
	    
	    pixman_rasterize_trapezoid (tmp, trap, - box.x1, - box.y1);
	}
	
	pixman_image_composite (op, src, tmp, dst,
				x_src + box.x1, y_src + box.y1,
				0, 0,
				x_dst + box.x1, y_dst + box.y1,
				box.x2 - box.x1, box.y2 - box.y1);
	
	pixman_image_unref (tmp);
    }
}

static int
greater_y (const pixman_point_fixed_t *a, const pixman_point_fixed_t *b)
{
    if (a->y == b->y)
	return a->x > b->x;
    return a->y > b->y;
}

/*
 * Note that the definition of this function is a bit odd because
 * of the X coordinate space (y increasing downwards).
 */
static int
clockwise (const pixman_point_fixed_t *ref,
	   const pixman_point_fixed_t *a,
	   const pixman_point_fixed_t *b)
{
    pixman_point_fixed_t	ad, bd;

    ad.x = a->x - ref->x;
    ad.y = a->y - ref->y;
    bd.x = b->x - ref->x;
    bd.y = b->y - ref->y;

    return ((pixman_fixed_32_32_t) bd.y * ad.x -
	    (pixman_fixed_32_32_t) ad.y * bd.x) < 0;
}

static void
triangle_to_trapezoids (const pixman_triangle_t *tri, pixman_trapezoid_t *traps)
{
    const pixman_point_fixed_t *top, *left, *right, *tmp;

    top = &tri->p1;
    left = &tri->p2;
    right = &tri->p3;

    if (greater_y (top, left))
    {
	tmp = left;
	left = top;
	top = tmp;
    }

    if (greater_y (top, right))
    {
	tmp = right;
	right = top;
	top = tmp;
    }

    if (clockwise (top, right, left))
    {
	tmp = right;
	right = left;
	left = tmp;
    }
    
    /*
     * Two cases:
     *
     *		+		+
     *	       / \             / \
     *	      /   \           /	  \
     *	     /     +         +	   \
     *      /    --           --    \
     *     /   --               --   \
     *    / ---                   --- \
     *	 +--                         --+
     */

    traps->top = top->y;
    traps->left.p1 = *top;
    traps->left.p2 = *left;
    traps->right.p1 = *top;
    traps->right.p2 = *right;

    if (right->y < left->y)
	traps->bottom = right->y;
    else
	traps->bottom = left->y;

    traps++;

    *traps = *(traps - 1);
    
    if (right->y < left->y)
    {
	traps->top = right->y;
	traps->bottom = left->y;
	traps->right.p1 = *right;
	traps->right.p2 = *left;
    }
    else
    {
	traps->top = left->y;
	traps->bottom = right->y;
	traps->left.p1 = *left;
	traps->left.p2 = *right;
    }
}

static pixman_trapezoid_t *
convert_triangles (int n_tris, const pixman_triangle_t *tris)
{
    pixman_trapezoid_t *traps;
    int i;

    if (n_tris <= 0)
	return NULL;
    
    traps = pixman_malloc_ab (n_tris, 2 * sizeof (pixman_trapezoid_t));
    if (!traps)
	return NULL;

    for (i = 0; i < n_tris; ++i)
	triangle_to_trapezoids (&(tris[i]), traps + 2 * i);

    return traps;
}

PIXMAN_EXPORT void
pixman_composite_triangles (pixman_op_t			op,
			    pixman_image_t *		src,
			    pixman_image_t *		dst,
			    pixman_format_code_t	mask_format,
			    int				x_src,
			    int				y_src,
			    int				x_dst,
			    int				y_dst,
			    int				n_tris,
			    const pixman_triangle_t *	tris)
{
    pixman_trapezoid_t *traps;

    if ((traps = convert_triangles (n_tris, tris)))
    {
	pixman_composite_trapezoids (op, src, dst, mask_format,
				     x_src, y_src, x_dst, y_dst,
				     n_tris * 2, traps);
	
	free (traps);
    }
}

PIXMAN_EXPORT void
pixman_add_triangles (pixman_image_t          *image,
		      int32_t	               x_off,
		      int32_t	               y_off,
		      int	               n_tris,
		      const pixman_triangle_t *tris)
{
    pixman_trapezoid_t *traps;

    if ((traps = convert_triangles (n_tris, tris)))
    {
	pixman_add_trapezoids (image, x_off, y_off,
			       n_tris * 2, traps);

	free (traps);
    }
}
