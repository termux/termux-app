/*
 * Copyright 2010, 2012, Soren Sandmann <sandmann@cs.au.dk>
 * Copyright 2010, 2011, 2012, Red Hat, Inc
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
 * Author: Soren Sandmann <sandmann@cs.au.dk>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "pixman-private.h"

#include <stdlib.h>

typedef struct glyph_metrics_t glyph_metrics_t;
typedef struct glyph_t glyph_t;

#define TOMBSTONE ((glyph_t *)0x1)

/* XXX: These numbers are arbitrary---we've never done any measurements.
 */
#define N_GLYPHS_HIGH_WATER  (16384)
#define N_GLYPHS_LOW_WATER   (8192)
#define HASH_SIZE (2 * N_GLYPHS_HIGH_WATER)
#define HASH_MASK (HASH_SIZE - 1)

struct glyph_t
{
    void *		font_key;
    void *		glyph_key;
    int			origin_x;
    int			origin_y;
    pixman_image_t *	image;
    pixman_link_t	mru_link;
};

struct pixman_glyph_cache_t
{
    int			n_glyphs;
    int			n_tombstones;
    int			freeze_count;
    pixman_list_t	mru;
    glyph_t *		glyphs[HASH_SIZE];
};

static void
free_glyph (glyph_t *glyph)
{
    pixman_list_unlink (&glyph->mru_link);
    pixman_image_unref (glyph->image);
    free (glyph);
}

static unsigned int
hash (const void *font_key, const void *glyph_key)
{
    size_t key = (size_t)font_key + (size_t)glyph_key;

    /* This hash function is based on one found on Thomas Wang's
     * web page at
     *
     *    http://www.concentric.net/~Ttwang/tech/inthash.htm
     *
     */
    key = (key << 15) - key - 1;
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key + (key << 3) + (key << 11);
    key = key ^ (key >> 16);

    return key;
}

static glyph_t *
lookup_glyph (pixman_glyph_cache_t *cache,
	      void                 *font_key,
	      void                 *glyph_key)
{
    unsigned idx;
    glyph_t *g;

    idx = hash (font_key, glyph_key);
    while ((g = cache->glyphs[idx++ & HASH_MASK]))
    {
	if (g != TOMBSTONE			&&
	    g->font_key == font_key		&&
	    g->glyph_key == glyph_key)
	{
	    return g;
	}
    }

    return NULL;
}

static void
insert_glyph (pixman_glyph_cache_t *cache,
	      glyph_t              *glyph)
{
    unsigned idx;
    glyph_t **loc;

    idx = hash (glyph->font_key, glyph->glyph_key);

    /* Note: we assume that there is room in the table. If there isn't,
     * this will be an infinite loop.
     */
    do
    {
	loc = &cache->glyphs[idx++ & HASH_MASK];
    } while (*loc && *loc != TOMBSTONE);

    if (*loc == TOMBSTONE)
	cache->n_tombstones--;
    cache->n_glyphs++;

    *loc = glyph;
}

static void
remove_glyph (pixman_glyph_cache_t *cache,
	      glyph_t              *glyph)
{
    unsigned idx;

    idx = hash (glyph->font_key, glyph->glyph_key);
    while (cache->glyphs[idx & HASH_MASK] != glyph)
	idx++;

    cache->glyphs[idx & HASH_MASK] = TOMBSTONE;
    cache->n_tombstones++;
    cache->n_glyphs--;

    /* Eliminate tombstones if possible */
    if (cache->glyphs[(idx + 1) & HASH_MASK] == NULL)
    {
	while (cache->glyphs[idx & HASH_MASK] == TOMBSTONE)
	{
	    cache->glyphs[idx & HASH_MASK] = NULL;
	    cache->n_tombstones--;
	    idx--;
	}
    }
}

static void
clear_table (pixman_glyph_cache_t *cache)
{
    int i;

    for (i = 0; i < HASH_SIZE; ++i)
    {
	glyph_t *glyph = cache->glyphs[i];

	if (glyph && glyph != TOMBSTONE)
	    free_glyph (glyph);

	cache->glyphs[i] = NULL;
    }

    cache->n_glyphs = 0;
    cache->n_tombstones = 0;
}

PIXMAN_EXPORT pixman_glyph_cache_t *
pixman_glyph_cache_create (void)
{
    pixman_glyph_cache_t *cache;

    if (!(cache = malloc (sizeof *cache)))
	return NULL;

    memset (cache->glyphs, 0, sizeof (cache->glyphs));
    cache->n_glyphs = 0;
    cache->n_tombstones = 0;
    cache->freeze_count = 0;

    pixman_list_init (&cache->mru);

    return cache;
}

PIXMAN_EXPORT void
pixman_glyph_cache_destroy (pixman_glyph_cache_t *cache)
{
    return_if_fail (cache->freeze_count == 0);

    clear_table (cache);

    free (cache);
}

PIXMAN_EXPORT void
pixman_glyph_cache_freeze (pixman_glyph_cache_t  *cache)
{
    cache->freeze_count++;
}

PIXMAN_EXPORT void
pixman_glyph_cache_thaw (pixman_glyph_cache_t  *cache)
{
    if (--cache->freeze_count == 0					&&
	cache->n_glyphs + cache->n_tombstones > N_GLYPHS_HIGH_WATER)
    {
	if (cache->n_tombstones > N_GLYPHS_HIGH_WATER)
	{
	    /* More than half the entries are
	     * tombstones. Just dump the whole table.
	     */
	    clear_table (cache);
	}

	while (cache->n_glyphs > N_GLYPHS_LOW_WATER)
	{
	    glyph_t *glyph = CONTAINER_OF (glyph_t, mru_link, cache->mru.tail);

	    remove_glyph (cache, glyph);
	    free_glyph (glyph);
	}
    }
}

PIXMAN_EXPORT const void *
pixman_glyph_cache_lookup (pixman_glyph_cache_t  *cache,
			   void                  *font_key,
			   void                  *glyph_key)
{
    return lookup_glyph (cache, font_key, glyph_key);
}

PIXMAN_EXPORT const void *
pixman_glyph_cache_insert (pixman_glyph_cache_t  *cache,
			   void                  *font_key,
			   void                  *glyph_key,
			   int			  origin_x,
			   int                    origin_y,
			   pixman_image_t        *image)
{
    glyph_t *glyph;
    int32_t width, height;

    return_val_if_fail (cache->freeze_count > 0, NULL);
    return_val_if_fail (image->type == BITS, NULL);

    width = image->bits.width;
    height = image->bits.height;

    if (cache->n_glyphs >= HASH_SIZE)
	return NULL;

    if (!(glyph = malloc (sizeof *glyph)))
	return NULL;

    glyph->font_key = font_key;
    glyph->glyph_key = glyph_key;
    glyph->origin_x = origin_x;
    glyph->origin_y = origin_y;

    if (!(glyph->image = pixman_image_create_bits (
	      image->bits.format, width, height, NULL, -1)))
    {
	free (glyph);
	return NULL;
    }

    pixman_image_composite32 (PIXMAN_OP_SRC,
			      image, NULL, glyph->image, 0, 0, 0, 0, 0, 0,
			      width, height);

    if (PIXMAN_FORMAT_A   (glyph->image->bits.format) != 0	&&
	PIXMAN_FORMAT_RGB (glyph->image->bits.format) != 0)
    {
	pixman_image_set_component_alpha (glyph->image, TRUE);
    }

    pixman_list_prepend (&cache->mru, &glyph->mru_link);

    _pixman_image_validate (glyph->image);
    insert_glyph (cache, glyph);

    return glyph;
}

PIXMAN_EXPORT void
pixman_glyph_cache_remove (pixman_glyph_cache_t  *cache,
			   void                  *font_key,
			   void                  *glyph_key)
{
    glyph_t *glyph;

    if ((glyph = lookup_glyph (cache, font_key, glyph_key)))
    {
	remove_glyph (cache, glyph);

	free_glyph (glyph);
    }
}

PIXMAN_EXPORT void
pixman_glyph_get_extents (pixman_glyph_cache_t *cache,
			  int                   n_glyphs,
			  pixman_glyph_t       *glyphs,
			  pixman_box32_t       *extents)
{
    int i;

    extents->x1 = extents->y1 = INT32_MAX;
    extents->x2 = extents->y2 = INT32_MIN;

    for (i = 0; i < n_glyphs; ++i)
    {
	glyph_t *glyph = (glyph_t *)glyphs[i].glyph;
	int x1, y1, x2, y2;

	x1 = glyphs[i].x - glyph->origin_x;
	y1 = glyphs[i].y - glyph->origin_y;
	x2 = glyphs[i].x - glyph->origin_x + glyph->image->bits.width;
	y2 = glyphs[i].y - glyph->origin_y + glyph->image->bits.height;

	if (x1 < extents->x1)
	    extents->x1 = x1;
	if (y1 < extents->y1)
	    extents->y1 = y1;
	if (x2 > extents->x2)
	    extents->x2 = x2;
	if (y2 > extents->y2)
	    extents->y2 = y2;
    }
}

/* This function returns a format that is suitable for use as a mask for the
 * set of glyphs in question.
 */
PIXMAN_EXPORT pixman_format_code_t
pixman_glyph_get_mask_format (pixman_glyph_cache_t *cache,
			      int		    n_glyphs,
			      const pixman_glyph_t *glyphs)
{
    pixman_format_code_t format = PIXMAN_a1;
    int i;

    for (i = 0; i < n_glyphs; ++i)
    {
	const glyph_t *glyph = glyphs[i].glyph;
	pixman_format_code_t glyph_format = glyph->image->bits.format;

	if (PIXMAN_FORMAT_TYPE (glyph_format) == PIXMAN_TYPE_A)
	{
	    if (PIXMAN_FORMAT_A (glyph_format) > PIXMAN_FORMAT_A (format))
		format = glyph_format;
	}
	else
	{
	    return PIXMAN_a8r8g8b8;
	}
    }

    return format;
}

static pixman_bool_t
box32_intersect (pixman_box32_t *dest,
		 const pixman_box32_t *box1,
		 const pixman_box32_t *box2)
{
    dest->x1 = MAX (box1->x1, box2->x1);
    dest->y1 = MAX (box1->y1, box2->y1);
    dest->x2 = MIN (box1->x2, box2->x2);
    dest->y2 = MIN (box1->y2, box2->y2);

    return dest->x2 > dest->x1 && dest->y2 > dest->y1;
}

#if defined(__GNUC__) && !defined(__x86_64__) && !defined(__amd64__)
__attribute__((__force_align_arg_pointer__))
#endif
PIXMAN_EXPORT void
pixman_composite_glyphs_no_mask (pixman_op_t            op,
				 pixman_image_t        *src,
				 pixman_image_t        *dest,
				 int32_t                src_x,
				 int32_t                src_y,
				 int32_t                dest_x,
				 int32_t                dest_y,
				 pixman_glyph_cache_t  *cache,
				 int                    n_glyphs,
				 const pixman_glyph_t  *glyphs)
{
    pixman_region32_t region;
    pixman_format_code_t glyph_format = PIXMAN_null;
    uint32_t glyph_flags = 0;
    pixman_format_code_t dest_format;
    uint32_t dest_flags;
    pixman_composite_func_t func = NULL;
    pixman_implementation_t *implementation = NULL;
    pixman_composite_info_t info;
    int i;

    _pixman_image_validate (src);
    _pixman_image_validate (dest);
    
    dest_format = dest->common.extended_format_code;
    dest_flags = dest->common.flags;
    
    pixman_region32_init (&region);
    if (!_pixman_compute_composite_region32 (
	    &region,
	    src, NULL, dest,
	    src_x - dest_x, src_y - dest_y, 0, 0, 0, 0,
	    dest->bits.width, dest->bits.height))
    {
	goto out;
    }

    info.op = op;
    info.src_image = src;
    info.dest_image = dest;
    info.src_flags = src->common.flags;
    info.dest_flags = dest->common.flags;

    for (i = 0; i < n_glyphs; ++i)
    {
	glyph_t *glyph = (glyph_t *)glyphs[i].glyph;
	pixman_image_t *glyph_img = glyph->image;
	pixman_box32_t glyph_box;
	pixman_box32_t *pbox;
	uint32_t extra = FAST_PATH_SAMPLES_COVER_CLIP_NEAREST;
	pixman_box32_t composite_box;
	int n;

	glyph_box.x1 = dest_x + glyphs[i].x - glyph->origin_x;
	glyph_box.y1 = dest_y + glyphs[i].y - glyph->origin_y;
	glyph_box.x2 = glyph_box.x1 + glyph->image->bits.width;
	glyph_box.y2 = glyph_box.y1 + glyph->image->bits.height;
	
	pbox = pixman_region32_rectangles (&region, &n);
	
	info.mask_image = glyph_img;

	while (n--)
	{
	    if (box32_intersect (&composite_box, pbox, &glyph_box))
	    {
		if (glyph_img->common.extended_format_code != glyph_format	||
		    glyph_img->common.flags != glyph_flags)
		{
		    glyph_format = glyph_img->common.extended_format_code;
		    glyph_flags = glyph_img->common.flags;

		    _pixman_implementation_lookup_composite (
			get_implementation(), op,
			src->common.extended_format_code, src->common.flags,
			glyph_format, glyph_flags | extra,
			dest_format, dest_flags,
			&implementation, &func);
		}

		info.src_x = src_x + composite_box.x1 - dest_x;
		info.src_y = src_y + composite_box.y1 - dest_y;
		info.mask_x = composite_box.x1 - (dest_x + glyphs[i].x - glyph->origin_x);
		info.mask_y = composite_box.y1 - (dest_y + glyphs[i].y - glyph->origin_y);
		info.dest_x = composite_box.x1;
		info.dest_y = composite_box.y1;
		info.width = composite_box.x2 - composite_box.x1;
		info.height = composite_box.y2 - composite_box.y1;

		info.mask_flags = glyph_flags;

		func (implementation, &info);
	    }

	    pbox++;
	}
	pixman_list_move_to_front (&cache->mru, &glyph->mru_link);
    }

out:
    pixman_region32_fini (&region);
}

static void
add_glyphs (pixman_glyph_cache_t *cache,
	    pixman_image_t *dest,
	    int off_x, int off_y,
	    int n_glyphs, const pixman_glyph_t *glyphs)
{
    pixman_format_code_t glyph_format = PIXMAN_null;
    uint32_t glyph_flags = 0;
    pixman_composite_func_t func = NULL;
    pixman_implementation_t *implementation = NULL;
    pixman_format_code_t dest_format;
    uint32_t dest_flags;
    pixman_box32_t dest_box;
    pixman_composite_info_t info;
    pixman_image_t *white_img = NULL;
    pixman_bool_t white_src = FALSE;
    int i;

    _pixman_image_validate (dest);

    dest_format = dest->common.extended_format_code;
    dest_flags = dest->common.flags;

    info.op = PIXMAN_OP_ADD;
    info.dest_image = dest;
    info.src_x = 0;
    info.src_y = 0;
    info.dest_flags = dest_flags;

    dest_box.x1 = 0;
    dest_box.y1 = 0;
    dest_box.x2 = dest->bits.width;
    dest_box.y2 = dest->bits.height;

    for (i = 0; i < n_glyphs; ++i)
    {
	glyph_t *glyph = (glyph_t *)glyphs[i].glyph;
	pixman_image_t *glyph_img = glyph->image;
	pixman_box32_t glyph_box;
	pixman_box32_t composite_box;

	if (glyph_img->common.extended_format_code != glyph_format	||
	    glyph_img->common.flags != glyph_flags)
	{
	    pixman_format_code_t src_format, mask_format;

	    glyph_format = glyph_img->common.extended_format_code;
	    glyph_flags = glyph_img->common.flags;

	    if (glyph_format == dest->bits.format)
	    {
		src_format = glyph_format;
		mask_format = PIXMAN_null;
		info.src_flags = glyph_flags | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST;
		info.mask_flags = FAST_PATH_IS_OPAQUE;
		info.mask_image = NULL;
		white_src = FALSE;
	    }
	    else
	    {
		if (!white_img)
		{
		    static const pixman_color_t white = { 0xffff, 0xffff, 0xffff, 0xffff };

		    if (!(white_img = pixman_image_create_solid_fill (&white)))
			goto out;

		    _pixman_image_validate (white_img);
		}

		src_format = PIXMAN_solid;
		mask_format = glyph_format;
		info.src_flags = white_img->common.flags;
		info.mask_flags = glyph_flags | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST;
		info.src_image = white_img;
		white_src = TRUE;
	    }

	    _pixman_implementation_lookup_composite (
		get_implementation(), PIXMAN_OP_ADD,
		src_format, info.src_flags,
		mask_format, info.mask_flags,
		dest_format, dest_flags,
		&implementation, &func);
	}

	glyph_box.x1 = glyphs[i].x - glyph->origin_x + off_x;
	glyph_box.y1 = glyphs[i].y - glyph->origin_y + off_y;
	glyph_box.x2 = glyph_box.x1 + glyph->image->bits.width;
	glyph_box.y2 = glyph_box.y1 + glyph->image->bits.height;
	
	if (box32_intersect (&composite_box, &glyph_box, &dest_box))
	{
	    int src_x = composite_box.x1 - glyph_box.x1;
	    int src_y = composite_box.y1 - glyph_box.y1;

	    if (white_src)
		info.mask_image = glyph_img;
	    else
		info.src_image = glyph_img;

	    info.mask_x = info.src_x = src_x;
	    info.mask_y = info.src_y = src_y;
	    info.dest_x = composite_box.x1;
	    info.dest_y = composite_box.y1;
	    info.width = composite_box.x2 - composite_box.x1;
	    info.height = composite_box.y2 - composite_box.y1;

	    func (implementation, &info);

	    pixman_list_move_to_front (&cache->mru, &glyph->mru_link);
	}
    }

out:
    if (white_img)
	pixman_image_unref (white_img);
}

/* Conceptually, for each glyph, (white IN glyph) is PIXMAN_OP_ADDed to an
 * infinitely big mask image at the position such that the glyph origin point
 * is positioned at the (glyphs[i].x, glyphs[i].y) point.
 *
 * Then (mask_x, mask_y) in the infinite mask and (src_x, src_y) in the source
 * image are both aligned with (dest_x, dest_y) in the destination image. Then
 * these three images are composited within the 
 *
 *       (dest_x, dest_y, dst_x + width, dst_y + height)
 *
 * rectangle.
 *
 * TODO:
 *   - Trim the mask to the destination clip/image?
 *   - Trim composite region based on sources, when the op ignores 0s.
 */
#if defined(__GNUC__) && !defined(__x86_64__) && !defined(__amd64__)
__attribute__((__force_align_arg_pointer__))
#endif
PIXMAN_EXPORT void
pixman_composite_glyphs (pixman_op_t            op,
			 pixman_image_t        *src,
			 pixman_image_t        *dest,
			 pixman_format_code_t   mask_format,
			 int32_t                src_x,
			 int32_t                src_y,
			 int32_t		mask_x,
			 int32_t		mask_y,
			 int32_t                dest_x,
			 int32_t                dest_y,
			 int32_t                width,
			 int32_t                height,
			 pixman_glyph_cache_t  *cache,
			 int			n_glyphs,
			 const pixman_glyph_t  *glyphs)
{
    pixman_image_t *mask;

    if (!(mask = pixman_image_create_bits (mask_format, width, height, NULL, -1)))
	return;

    if (PIXMAN_FORMAT_A   (mask_format) != 0 &&
	PIXMAN_FORMAT_RGB (mask_format) != 0)
    {
	pixman_image_set_component_alpha (mask, TRUE);
    }

    add_glyphs (cache, mask, - mask_x, - mask_y, n_glyphs, glyphs);

    pixman_image_composite32 (op, src, mask, dest,
			      src_x, src_y,
			      0, 0,
			      dest_x, dest_y,
			      width, height);

    pixman_image_unref (mask);
}
