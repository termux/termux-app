/*
 * Copyright © 2014 RISC OS Open Ltd
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "utils.h"

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#else
#include <time.h>
#endif

#define WIDTH  1920
#define HEIGHT 1080

/* How much data to read to flush all cached data to RAM */
#define MAX_L2CACHE_SIZE (8 * 1024 * 1024)

#define PAGE_SIZE (4 * 1024)

struct bench_info
{
    pixman_op_t           op;
    pixman_transform_t    transform;
    pixman_image_t       *src_image;
    pixman_image_t       *mask_image;
    pixman_image_t       *dest_image;
    int32_t               src_x;
    int32_t               src_y;
};

typedef struct bench_info bench_info_t;

struct box_48_16
{
    pixman_fixed_48_16_t        x1;
    pixman_fixed_48_16_t        y1;
    pixman_fixed_48_16_t        x2;
    pixman_fixed_48_16_t        y2;
};

typedef struct box_48_16 box_48_16_t;

/* This function is copied verbatim from pixman.c. */
static pixman_bool_t
compute_transformed_extents (pixman_transform_t   *transform,
			     const pixman_box32_t *extents,
			     box_48_16_t          *transformed)
{
    pixman_fixed_48_16_t tx1, ty1, tx2, ty2;
    pixman_fixed_t x1, y1, x2, y2;
    int i;

    x1 = pixman_int_to_fixed (extents->x1) + pixman_fixed_1 / 2;
    y1 = pixman_int_to_fixed (extents->y1) + pixman_fixed_1 / 2;
    x2 = pixman_int_to_fixed (extents->x2) - pixman_fixed_1 / 2;
    y2 = pixman_int_to_fixed (extents->y2) - pixman_fixed_1 / 2;

    if (!transform)
    {
	transformed->x1 = x1;
	transformed->y1 = y1;
	transformed->x2 = x2;
	transformed->y2 = y2;

	return TRUE;
    }

    tx1 = ty1 = INT64_MAX;
    tx2 = ty2 = INT64_MIN;

    for (i = 0; i < 4; ++i)
    {
	pixman_fixed_48_16_t tx, ty;
	pixman_vector_t v;

	v.vector[0] = (i & 0x01)? x1 : x2;
	v.vector[1] = (i & 0x02)? y1 : y2;
	v.vector[2] = pixman_fixed_1;

	if (!pixman_transform_point (transform, &v))
	    return FALSE;

	tx = (pixman_fixed_48_16_t)v.vector[0];
	ty = (pixman_fixed_48_16_t)v.vector[1];

	if (tx < tx1)
	    tx1 = tx;
	if (ty < ty1)
	    ty1 = ty;
	if (tx > tx2)
	    tx2 = tx;
	if (ty > ty2)
	    ty2 = ty;
    }

    transformed->x1 = tx1;
    transformed->y1 = ty1;
    transformed->x2 = tx2;
    transformed->y2 = ty2;

    return TRUE;
}

static void
create_image (uint32_t                   width,
              uint32_t                   height,
              pixman_format_code_t       format,
              pixman_filter_t            filter,
              uint32_t                 **bits,
              pixman_image_t           **image)
{
    uint32_t stride = (width * PIXMAN_FORMAT_BPP (format) + 31) / 32 * 4;

    *bits = aligned_malloc (PAGE_SIZE, stride * height);
    memset (*bits, 0xCC, stride * height);
    *image = pixman_image_create_bits (format, width, height, *bits, stride);
    pixman_image_set_repeat (*image, PIXMAN_REPEAT_NORMAL);
    pixman_image_set_filter (*image, filter, NULL, 0);
}

/* This needs to match the shortest cacheline length we expect to encounter */
#define CACHE_CLEAN_INCREMENT 32

static void
flush_cache (void)
{
    static const char clean_space[MAX_L2CACHE_SIZE];
    volatile const char *x = clean_space;
    const char *clean_end = clean_space + sizeof clean_space;

    while (x < clean_end)
    {
        (void) *x;
        x += CACHE_CLEAN_INCREMENT;
    }
}

/* Obtain current time in microseconds modulo 2^32 */
uint32_t
gettimei (void)
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;

    gettimeofday (&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
#else
    return (uint64_t) clock () * 1000000 / CLOCKS_PER_SEC;
#endif
}

static void
pixman_image_composite_wrapper (const pixman_composite_info_t *info)
{
    pixman_image_composite (info->op,
                            info->src_image, info->mask_image, info->dest_image,
                            info->src_x, info->src_y,
                            info->mask_x, info->mask_y,
                            info->dest_x, info->dest_y,
                            info->width, info->height);
}

static void
pixman_image_composite_empty (const pixman_composite_info_t *info)
{
    pixman_image_composite (info->op,
                            info->src_image, info->mask_image, info->dest_image,
                            info->src_x, info->src_y,
                            info->mask_x, info->mask_y,
                            info->dest_x, info->dest_y,
                            1, 1);
}

static void
bench (const bench_info_t *bi,
       uint32_t            max_n,
       uint32_t            max_time,
       uint32_t           *ret_n,
       uint32_t           *ret_time,
       void              (*func) (const pixman_composite_info_t *info))
{
    uint32_t n = 0;
    uint32_t t0;
    uint32_t t1;
    uint32_t x = 0;
    pixman_transform_t t;
    pixman_composite_info_t info;

    t = bi->transform;
    info.op = bi->op;
    info.src_image = bi->src_image;
    info.mask_image = bi->mask_image;
    info.dest_image = bi->dest_image;
    info.src_x = 0;
    info.src_y = 0;
    info.mask_x = 0;
    info.mask_y = 0;
    /* info.dest_x set below */
    info.dest_y = 0;
    info.width = WIDTH;
    info.height = HEIGHT;

    t0 = gettimei ();

    do
    {

        if (++x >= 64)
            x = 0;

        info.dest_x = 63 - x;

        t.matrix[0][2] = pixman_int_to_fixed (bi->src_x + x);
        t.matrix[1][2] = pixman_int_to_fixed (bi->src_y);
        pixman_image_set_transform (bi->src_image, &t);

        if (bi->mask_image)
            pixman_image_set_transform (bi->mask_image, &t);

        func (&info);
        t1 = gettimei ();
    }
    while (++n < max_n && (t1 - t0) < max_time);

    if (ret_n)
        *ret_n = n;

    *ret_time = t1 - t0;
}

int
parse_fixed_argument (char *arg, pixman_fixed_t *value)
{
    char *tailptr;

    *value = pixman_double_to_fixed (strtod (arg, &tailptr));

    return *tailptr == '\0';
}

int
parse_arguments (int                   argc,
                 char                 *argv[],
                 pixman_transform_t   *t,
                 pixman_op_t          *op,
                 pixman_format_code_t *src_format,
                 pixman_format_code_t *mask_format,
                 pixman_format_code_t *dest_format)
{
    if (!parse_fixed_argument (*argv, &t->matrix[0][0]))
        return 0;

    if (*++argv == NULL)
        return 1;

    if (!parse_fixed_argument (*argv, &t->matrix[0][1]))
        return 0;

    if (*++argv == NULL)
        return 1;

    if (!parse_fixed_argument (*argv, &t->matrix[1][0]))
        return 0;

    if (*++argv == NULL)
        return 1;

    if (!parse_fixed_argument (*argv, &t->matrix[1][1]))
        return 0;

    if (*++argv == NULL)
        return 1;

    *op = operator_from_string (*argv);
    if (*op == PIXMAN_OP_NONE)
        return 0;

    if (*++argv == NULL)
        return 1;

    *src_format = format_from_string (*argv);
    if (*src_format == PIXMAN_null)
        return 0;

    ++argv;
    if (argv[0] && argv[1])
    {
        *mask_format = format_from_string (*argv);
        if (*mask_format == PIXMAN_null)
            return 0;
        ++argv;
    }
    if (*argv)
    {
        *dest_format = format_from_string (*argv);
        if (*dest_format == PIXMAN_null)
            return 0;
    }
    return 1;
}

static void
run_benchmark (const bench_info_t *bi)
{
    uint32_t n;  /* number of iterations in at least 5 seconds */
    uint32_t t1; /* time taken to do n iterations, microseconds */
    uint32_t t2; /* calling overhead for n iterations, microseconds */

    flush_cache ();
    bench (bi, UINT32_MAX, 5000000, &n, &t1, pixman_image_composite_wrapper);
    bench (bi, n, UINT32_MAX, NULL, &t2, pixman_image_composite_empty);

    /* The result indicates the output rate in megapixels/second */
    printf ("%6.2f\n", (double) n * WIDTH * HEIGHT / (t1 - t2));
}


int
main (int argc, char *argv[])
{
    bench_info_t         binfo;
    pixman_filter_t      filter      = PIXMAN_FILTER_NEAREST;
    pixman_format_code_t src_format  = PIXMAN_a8r8g8b8;
    pixman_format_code_t mask_format = 0;
    pixman_format_code_t dest_format = PIXMAN_a8r8g8b8;
    pixman_box32_t       dest_box    = { 0, 0, WIDTH, HEIGHT };
    box_48_16_t          transformed = { 0 };
    int32_t xmin, ymin, xmax, ymax;
    uint32_t *src, *mask, *dest;

    binfo.op         = PIXMAN_OP_SRC;
    binfo.mask_image = NULL;
    pixman_transform_init_identity (&binfo.transform);

    ++argv;
    if (*argv && (*argv)[0] == '-' && (*argv)[1] == 'n')
    {
        filter = PIXMAN_FILTER_NEAREST;
        ++argv;
        --argc;
    }

    if (*argv && (*argv)[0] == '-' && (*argv)[1] == 'b')
    {
        filter = PIXMAN_FILTER_BILINEAR;
        ++argv;
        --argc;
    }

    if (argc == 1 ||
        !parse_arguments (argc, argv, &binfo.transform, &binfo.op,
                          &src_format, &mask_format, &dest_format))
    {
        printf ("Usage: affine-bench [-n] [-b] axx [axy] [ayx] [ayy] [combine type]\n");
        printf ("                    [src format] [mask format] [dest format]\n");
        printf ("  -n : nearest scaling (default)\n");
        printf ("  -b : bilinear scaling\n");
        printf ("  axx : x_out:x_in factor\n");
        printf ("  axy : x_out:y_in factor (default 0)\n");
        printf ("  ayx : y_out:x_in factor (default 0)\n");
        printf ("  ayy : y_out:y_in factor (default 1)\n");
        printf ("  combine type : src, over, in etc (default src)\n");
        printf ("  src format : a8r8g8b8, r5g6b5 etc (default a8r8g8b8)\n");
        printf ("  mask format : as for src format, but no mask used if omitted\n");
        printf ("  dest format : as for src format (default a8r8g8b8)\n");
        printf ("The output is a single number in megapixels/second.\n");

        return EXIT_FAILURE;
    }

    /* Compute required extents for source and mask image so they qualify
     * for COVER fast paths and get the flags in pixman.c:analyze_extent().
     * These computations are for FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR,
     * but at the same time they also allow COVER_CLIP_NEAREST.
     */
    compute_transformed_extents (&binfo.transform, &dest_box, &transformed);
    xmin = pixman_fixed_to_int (transformed.x1 - pixman_fixed_1 / 2);
    ymin = pixman_fixed_to_int (transformed.y1 - pixman_fixed_1 / 2);
    xmax = pixman_fixed_to_int (transformed.x2 + pixman_fixed_1 / 2);
    ymax = pixman_fixed_to_int (transformed.y2 + pixman_fixed_1 / 2);
    /* Note:
     * The upper limits can be reduced to the following when fetchers
     * are guaranteed to not access pixels with zero weight. This concerns
     * particularly all bilinear samplers.
     *
     * xmax = pixman_fixed_to_int (transformed.x2 + pixman_fixed_1 / 2 - pixman_fixed_e);
     * ymax = pixman_fixed_to_int (transformed.y2 + pixman_fixed_1 / 2 - pixman_fixed_e);
     * This is equivalent to subtracting 0.5 and rounding up, rather than
     * subtracting 0.5, rounding down and adding 1.
     */
    binfo.src_x = -xmin;
    binfo.src_y = -ymin;

    /* Always over-allocate width by 64 pixels for all src, mask and dst,
     * so that we can iterate over an x-offset 0..63 in bench ().
     * This is similar to lowlevel-blt-bench, which uses the same method
     * to hit different cacheline misalignments.
     */
    create_image (xmax - xmin + 64, ymax - ymin + 1, src_format, filter,
                  &src, &binfo.src_image);

    if (mask_format)
    {
        create_image (xmax - xmin + 64, ymax - ymin + 1, mask_format, filter,
                      &mask, &binfo.mask_image);

        if ((PIXMAN_FORMAT_R(mask_format) ||
             PIXMAN_FORMAT_G(mask_format) ||
             PIXMAN_FORMAT_B(mask_format)))
        {
            pixman_image_set_component_alpha (binfo.mask_image, 1);
        }
    }

    create_image (WIDTH + 64, HEIGHT, dest_format, filter,
                  &dest, &binfo.dest_image);

    run_benchmark (&binfo);

    return EXIT_SUCCESS;
}
