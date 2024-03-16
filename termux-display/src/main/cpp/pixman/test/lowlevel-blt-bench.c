/*
 * Copyright © 2009 Nokia Corporation
 * Copyright © 2010 Movial Creative Technologies Oy
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define SOLID_FLAG 1
#define CA_FLAG    2

#define L1CACHE_SIZE (8 * 1024)
#define L2CACHE_SIZE (128 * 1024)

/* This is applied to both L1 and L2 tests - alternatively, you could
 * parameterise bench_L or split it into two functions. It could be
 * read at runtime on some architectures, but it only really matters
 * that it's a number that's an integer divisor of both cacheline
 * lengths, and further, it only really matters for caches that don't
 * do allocate0on-write. */
#define CACHELINE_LENGTH (32) /* bytes */

#define WIDTH  1920
#define HEIGHT 1080
#define BUFSIZE (WIDTH * HEIGHT * 4)
#define XWIDTH 256
#define XHEIGHT 256
#define TILEWIDTH 32
#define TINYWIDTH 8

#define EXCLUDE_OVERHEAD 1

uint32_t *dst;
uint32_t *src;
uint32_t *mask;

double bandwidth = 0.0;

double
bench_memcpy ()
{
    int64_t n = 0, total;
    double  t1, t2;
    int     x = 0;

    t1 = gettime ();
    while (1)
    {
	memcpy (dst, src, BUFSIZE - 64);
	memcpy (src, dst, BUFSIZE - 64);
	n += 4 * (BUFSIZE - 64);
	t2 = gettime ();
	if (t2 - t1 > 0.5)
	    break;
    }
    n = total = n * 5;
    t1 = gettime ();
    while (n > 0)
    {
	if (++x >= 64)
	    x = 0;
	memcpy ((char *)dst + 1, (char *)src + x, BUFSIZE - 64);
	memcpy ((char *)src + 1, (char *)dst + x, BUFSIZE - 64);
	n -= 4 * (BUFSIZE - 64);
    }
    t2 = gettime ();
    return (double)total / (t2 - t1);
}

static pixman_bool_t use_scaling = FALSE;
static pixman_filter_t filter = PIXMAN_FILTER_NEAREST;
static pixman_bool_t use_csv_output = FALSE;

/* nearly 1x scale factor */
static pixman_transform_t m =
{
    {
        { pixman_fixed_1 + 1, 0,              0              },
        { 0,                  pixman_fixed_1, 0              },
        { 0,                  0,              pixman_fixed_1 }
    }
};

static void
pixman_image_composite_wrapper (pixman_implementation_t *impl,
				pixman_composite_info_t *info)
{
    if (use_scaling)
    {
        pixman_image_set_filter (info->src_image, filter, NULL, 0);
        pixman_image_set_transform(info->src_image, &m);
    }
    pixman_image_composite (info->op,
			    info->src_image, info->mask_image, info->dest_image,
			    info->src_x, info->src_y,
			    info->mask_x, info->mask_y,
			    info->dest_x, info->dest_y,
			    info->width, info->height);
}

static void
pixman_image_composite_empty (pixman_implementation_t *impl,
			      pixman_composite_info_t *info)
{
    if (use_scaling)
    {
        pixman_image_set_filter (info->src_image, filter, NULL, 0);
        pixman_image_set_transform(info->src_image, &m);
    }
    pixman_image_composite (info->op,
			    info->src_image, info->mask_image, info->dest_image,
			    0, 0, 0, 0, 0, 0, 1, 1);
}

static inline void
call_func (pixman_composite_func_t func,
	   pixman_op_t             op,
	   pixman_image_t *        src_image,
	   pixman_image_t *        mask_image,
	   pixman_image_t *        dest_image,
	   int32_t		   src_x,
	   int32_t		   src_y,
	   int32_t                 mask_x,
	   int32_t                 mask_y,
	   int32_t                 dest_x,
	   int32_t                 dest_y,
	   int32_t                 width,
	   int32_t                 height)
{
    pixman_composite_info_t info;

    info.op = op;
    info.src_image = src_image;
    info.mask_image = mask_image;
    info.dest_image = dest_image;
    info.src_x = src_x;
    info.src_y = src_y;
    info.mask_x = mask_x;
    info.mask_y = mask_y;
    info.dest_x = dest_x;
    info.dest_y = dest_y;
    info.width = width;
    info.height = height;

    func (0, &info);
}

double
noinline
bench_L  (pixman_op_t              op,
          pixman_image_t *         src_img,
          pixman_image_t *         mask_img,
          pixman_image_t *         dst_img,
          int64_t                  n,
          pixman_composite_func_t  func,
          int                      width,
          int                      lines_count)
{
    int64_t      i, j, k;
    int          x = 0;
    int          q = 0;

    for (i = 0; i < n; i++)
    {
        /* For caches without allocate-on-write, we need to force the
         * destination buffer back into the cache on each iteration,
         * otherwise if they are evicted during the test, they remain
         * uncached. This doesn't matter for tests which read the
         * destination buffer, or for caches that do allocate-on-write,
         * but in those cases this loop just adds constant time, which
         * should be successfully cancelled out.
         */
        for (j = 0; j < lines_count; j++)
        {
            for (k = 0; k < width + 62; k += CACHELINE_LENGTH / sizeof *dst)
            {
                q += dst[j * WIDTH + k];
            }
            q += dst[j * WIDTH + width + 62];
        }
	if (++x >= 64)
	    x = 0;
	call_func (func, op, src_img, mask_img, dst_img, x, 0, x, 0, 63 - x, 0, width, lines_count);
    }

    return (double)n * lines_count * width;
}

double
noinline
bench_M (pixman_op_t              op,
         pixman_image_t *         src_img,
         pixman_image_t *         mask_img,
         pixman_image_t *         dst_img,
         int64_t                  n,
         pixman_composite_func_t  func)
{
    int64_t i;
    int     x = 0;

    for (i = 0; i < n; i++)
    {
	if (++x >= 64)
	    x = 0;
	call_func (func, op, src_img, mask_img, dst_img, x, 0, x, 0, 1, 0, WIDTH - 64, HEIGHT);
    }

    return (double)n * (WIDTH - 64) * HEIGHT;
}

double
noinline
bench_HT (pixman_op_t              op,
          pixman_image_t *         src_img,
          pixman_image_t *         mask_img,
          pixman_image_t *         dst_img,
          int64_t                  n,
          pixman_composite_func_t  func)
{
    double  pix_cnt = 0;
    int     x = 0;
    int     y = 0;
    int64_t i;

    srand (0);
    for (i = 0; i < n; i++)
    {
	int w = (rand () % (TILEWIDTH * 2)) + 1;
	int h = (rand () % (TILEWIDTH * 2)) + 1;
	if (x + w > WIDTH)
	{
	    x = 0;
	    y += TILEWIDTH * 2;
	}
	if (y + h > HEIGHT)
	{
	    y = 0;
	}
	call_func (func, op, src_img, mask_img, dst_img, x, y, x, y, x, y, w, h);
	x += w;
	pix_cnt += w * h;
    }
    return pix_cnt;
}

double
noinline
bench_VT (pixman_op_t              op,
          pixman_image_t *         src_img,
          pixman_image_t *         mask_img,
          pixman_image_t *         dst_img,
          int64_t                  n,
          pixman_composite_func_t  func)
{
    double  pix_cnt = 0;
    int     x = 0;
    int     y = 0;
    int64_t i;

    srand (0);
    for (i = 0; i < n; i++)
    {
	int w = (rand () % (TILEWIDTH * 2)) + 1;
	int h = (rand () % (TILEWIDTH * 2)) + 1;
	if (y + h > HEIGHT)
	{
	    y = 0;
	    x += TILEWIDTH * 2;
	}
	if (x + w > WIDTH)
	{
	    x = 0;
	}
	call_func (func, op, src_img, mask_img, dst_img, x, y, x, y, x, y, w, h);
	y += h;
	pix_cnt += w * h;
    }
    return pix_cnt;
}

double
noinline
bench_R (pixman_op_t              op,
         pixman_image_t *         src_img,
         pixman_image_t *         mask_img,
         pixman_image_t *         dst_img,
         int64_t                  n,
         pixman_composite_func_t  func,
         int                      maxw,
         int                      maxh)
{
    double  pix_cnt = 0;
    int64_t i;

    if (maxw <= TILEWIDTH * 2 || maxh <= TILEWIDTH * 2)
    {
	printf("error: maxw <= TILEWIDTH * 2 || maxh <= TILEWIDTH * 2\n");
        return 0;
    }

    srand (0);
    for (i = 0; i < n; i++)
    {
	int w = (rand () % (TILEWIDTH * 2)) + 1;
	int h = (rand () % (TILEWIDTH * 2)) + 1;
	int sx = rand () % (maxw - TILEWIDTH * 2);
	int sy = rand () % (maxh - TILEWIDTH * 2);
	int dx = rand () % (maxw - TILEWIDTH * 2);
	int dy = rand () % (maxh - TILEWIDTH * 2);
	call_func (func, op, src_img, mask_img, dst_img, sx, sy, sx, sy, dx, dy, w, h);
	pix_cnt += w * h;
    }
    return pix_cnt;
}

double
noinline
bench_RT (pixman_op_t              op,
          pixman_image_t *         src_img,
          pixman_image_t *         mask_img,
          pixman_image_t *         dst_img,
          int64_t                  n,
          pixman_composite_func_t  func,
          int                      maxw,
          int                      maxh)
{
    double  pix_cnt = 0;
    int64_t i;

    if (maxw <= TINYWIDTH * 2 || maxh <= TINYWIDTH * 2)
    {
	printf("error: maxw <= TINYWIDTH * 2 || maxh <= TINYWIDTH * 2\n");
        return 0;
    }

    srand (0);
    for (i = 0; i < n; i++)
    {
	int w = (rand () % (TINYWIDTH * 2)) + 1;
	int h = (rand () % (TINYWIDTH * 2)) + 1;
	int sx = rand () % (maxw - TINYWIDTH * 2);
	int sy = rand () % (maxh - TINYWIDTH * 2);
	int dx = rand () % (maxw - TINYWIDTH * 2);
	int dy = rand () % (maxh - TINYWIDTH * 2);
	call_func (func, op, src_img, mask_img, dst_img, sx, sy, sx, sy, dx, dy, w, h);
	pix_cnt += w * h;
    }
    return pix_cnt;
}

static double
Mpx_per_sec (double pix_cnt, double t1, double t2, double t3)
{
    double overhead = t2 - t1;
    double testtime = t3 - t2;

    return pix_cnt / (testtime - overhead) / 1e6;
}

void
bench_composite (const char *testname,
                 int         src_fmt,
                 int         src_flags,
                 int         op,
                 int         mask_fmt,
                 int         mask_flags,
                 int         dst_fmt,
                 double      npix)
{
    pixman_image_t *                src_img;
    pixman_image_t *                dst_img;
    pixman_image_t *                mask_img;
    pixman_image_t *                xsrc_img;
    pixman_image_t *                xdst_img;
    pixman_image_t *                xmask_img;
    double                          t1, t2, t3, pix_cnt;
    int64_t                         n, l1test_width, nlines;
    double                             bytes_per_pix = 0;
    pixman_bool_t                   bench_pixbuf = FALSE;

    pixman_composite_func_t func = pixman_image_composite_wrapper;

    if (!(src_flags & SOLID_FLAG))
    {
        bytes_per_pix += (src_fmt >> 24) / 8.0;
        src_img = pixman_image_create_bits (src_fmt,
                                            WIDTH, HEIGHT,
                                            src,
                                            WIDTH * 4);
        xsrc_img = pixman_image_create_bits (src_fmt,
                                             XWIDTH, XHEIGHT,
                                             src,
                                             XWIDTH * 4);
    }
    else
    {
        src_img = pixman_image_create_bits (src_fmt,
                                            1, 1,
                                            src,
                                            4);
        xsrc_img = pixman_image_create_bits (src_fmt,
                                             1, 1,
                                             src,
                                             4);
        pixman_image_set_repeat (src_img, PIXMAN_REPEAT_NORMAL);
        pixman_image_set_repeat (xsrc_img, PIXMAN_REPEAT_NORMAL);
    }

    bytes_per_pix += (dst_fmt >> 24) / 8.0;
    dst_img = pixman_image_create_bits (dst_fmt,
                                        WIDTH, HEIGHT,
                                        dst,
                                        WIDTH * 4);

    mask_img = NULL;
    xmask_img = NULL;
    if (strcmp (testname, "pixbuf") == 0 || strcmp (testname, "rpixbuf") == 0)
    {
        bench_pixbuf = TRUE;
    }
    if (!(mask_flags & SOLID_FLAG) && mask_fmt != PIXMAN_null)
    {
        bytes_per_pix += (mask_fmt >> 24) / ((op == PIXMAN_OP_SRC) ? 8.0 : 4.0);
        mask_img = pixman_image_create_bits (mask_fmt,
                                             WIDTH, HEIGHT,
                                             bench_pixbuf ? src : mask,
                                             WIDTH * 4);
        xmask_img = pixman_image_create_bits (mask_fmt,
                                             XWIDTH, XHEIGHT,
                                             bench_pixbuf ? src : mask,
                                             XWIDTH * 4);
    }
    else if (mask_fmt != PIXMAN_null)
    {
        mask_img = pixman_image_create_bits (mask_fmt,
                                             1, 1,
                                             mask,
                                             4);
        xmask_img = pixman_image_create_bits (mask_fmt,
                                             1, 1,
                                             mask,
                                             4 * 4);
       pixman_image_set_repeat (mask_img, PIXMAN_REPEAT_NORMAL);
       pixman_image_set_repeat (xmask_img, PIXMAN_REPEAT_NORMAL);
    }
    if ((mask_flags & CA_FLAG) && mask_fmt != PIXMAN_null)
    {
       pixman_image_set_component_alpha (mask_img, 1);
    }
    xdst_img = pixman_image_create_bits (dst_fmt,
                                         XWIDTH, XHEIGHT,
                                         dst,
                                         XWIDTH * 4);

    if (!use_csv_output)
        printf ("%24s %c", testname, func != pixman_image_composite_wrapper ?
                '-' : '=');

    memcpy (dst, src, BUFSIZE);
    memcpy (src, dst, BUFSIZE);

    l1test_width = L1CACHE_SIZE / 8 - 64;
    if (l1test_width < 1)
	l1test_width = 1;
    if (l1test_width > WIDTH - 64)
	l1test_width = WIDTH - 64;
    n = 1 + npix / (l1test_width * 8);
    t1 = gettime ();
#if EXCLUDE_OVERHEAD
    pix_cnt = bench_L (op, src_img, mask_img, dst_img, n, pixman_image_composite_empty, l1test_width, 1);
#endif
    t2 = gettime ();
    pix_cnt = bench_L (op, src_img, mask_img, dst_img, n, func, l1test_width, 1);
    t3 = gettime ();
    if (use_csv_output)
        printf ("%g,", Mpx_per_sec (pix_cnt, t1, t2, t3));
    else
        printf ("  L1:%7.2f", Mpx_per_sec (pix_cnt, t1, t2, t3));
    fflush (stdout);

    memcpy (dst, src, BUFSIZE);
    memcpy (src, dst, BUFSIZE);

    nlines = (L2CACHE_SIZE / l1test_width) /
	((PIXMAN_FORMAT_BPP(src_fmt) + PIXMAN_FORMAT_BPP(dst_fmt)) / 8);
    if (nlines < 1)
	nlines = 1;
    n = 1 + npix / (l1test_width * nlines);
    t1 = gettime ();
#if EXCLUDE_OVERHEAD
    pix_cnt = bench_L (op, src_img, mask_img, dst_img, n, pixman_image_composite_empty, l1test_width, nlines);
#endif
    t2 = gettime ();
    pix_cnt = bench_L (op, src_img, mask_img, dst_img, n, func, l1test_width, nlines);
    t3 = gettime ();
    if (use_csv_output)
        printf ("%g,", Mpx_per_sec (pix_cnt, t1, t2, t3));
    else
        printf ("  L2:%7.2f", Mpx_per_sec (pix_cnt, t1, t2, t3));
    fflush (stdout);

    memcpy (dst, src, BUFSIZE);
    memcpy (src, dst, BUFSIZE);

    n = 1 + npix / (WIDTH * HEIGHT);
    t1 = gettime ();
#if EXCLUDE_OVERHEAD
    pix_cnt = bench_M (op, src_img, mask_img, dst_img, n, pixman_image_composite_empty);
#endif
    t2 = gettime ();
    pix_cnt = bench_M (op, src_img, mask_img, dst_img, n, func);
    t3 = gettime ();
    if (use_csv_output)
        printf ("%g,", Mpx_per_sec (pix_cnt, t1, t2, t3));
    else
        printf ("  M:%6.2f (%6.2f%%)", Mpx_per_sec (pix_cnt, t1, t2, t3),
                (pix_cnt / ((t3 - t2) - (t2 - t1)) * bytes_per_pix) * (100.0 / bandwidth) );
    fflush (stdout);

    memcpy (dst, src, BUFSIZE);
    memcpy (src, dst, BUFSIZE);

    n = 1 + npix / (8 * TILEWIDTH * TILEWIDTH);
    t1 = gettime ();
#if EXCLUDE_OVERHEAD
    pix_cnt = bench_HT (op, src_img, mask_img, dst_img, n, pixman_image_composite_empty);
#endif
    t2 = gettime ();
    pix_cnt = bench_HT (op, src_img, mask_img, dst_img, n, func);
    t3 = gettime ();
    if (use_csv_output)
        printf ("%g,", Mpx_per_sec (pix_cnt, t1, t2, t3));
    else
        printf ("  HT:%6.2f", Mpx_per_sec (pix_cnt, t1, t2, t3));
    fflush (stdout);

    memcpy (dst, src, BUFSIZE);
    memcpy (src, dst, BUFSIZE);

    n = 1 + npix / (8 * TILEWIDTH * TILEWIDTH);
    t1 = gettime ();
#if EXCLUDE_OVERHEAD
    pix_cnt = bench_VT (op, src_img, mask_img, dst_img, n, pixman_image_composite_empty);
#endif
    t2 = gettime ();
    pix_cnt = bench_VT (op, src_img, mask_img, dst_img, n, func);
    t3 = gettime ();
    if (use_csv_output)
        printf ("%g,", Mpx_per_sec (pix_cnt, t1, t2, t3));
    else
        printf ("  VT:%6.2f", Mpx_per_sec (pix_cnt, t1, t2, t3));
    fflush (stdout);

    memcpy (dst, src, BUFSIZE);
    memcpy (src, dst, BUFSIZE);

    n = 1 + npix / (8 * TILEWIDTH * TILEWIDTH);
    t1 = gettime ();
#if EXCLUDE_OVERHEAD
    pix_cnt = bench_R (op, src_img, mask_img, dst_img, n, pixman_image_composite_empty, WIDTH, HEIGHT);
#endif
    t2 = gettime ();
    pix_cnt = bench_R (op, src_img, mask_img, dst_img, n, func, WIDTH, HEIGHT);
    t3 = gettime ();
    if (use_csv_output)
        printf ("%g,", Mpx_per_sec (pix_cnt, t1, t2, t3));
    else
        printf ("  R:%6.2f", Mpx_per_sec (pix_cnt, t1, t2, t3));
    fflush (stdout);

    memcpy (dst, src, BUFSIZE);
    memcpy (src, dst, BUFSIZE);

    n = 1 + npix / (16 * TINYWIDTH * TINYWIDTH);
    t1 = gettime ();
#if EXCLUDE_OVERHEAD
    pix_cnt = bench_RT (op, src_img, mask_img, dst_img, n, pixman_image_composite_empty, WIDTH, HEIGHT);
#endif
    t2 = gettime ();
    pix_cnt = bench_RT (op, src_img, mask_img, dst_img, n, func, WIDTH, HEIGHT);
    t3 = gettime ();
    if (use_csv_output)
        printf ("%g\n", Mpx_per_sec (pix_cnt, t1, t2, t3));
    else
        printf ("  RT:%6.2f (%4.0fKops/s)\n", Mpx_per_sec (pix_cnt, t1, t2, t3), (double) n / ((t3 - t2) * 1000));

    if (mask_img) {
	pixman_image_unref (mask_img);
	pixman_image_unref (xmask_img);
    }
    pixman_image_unref (src_img);
    pixman_image_unref (dst_img);
    pixman_image_unref (xsrc_img);
    pixman_image_unref (xdst_img);
}

#define PIXMAN_OP_OUT_REV (PIXMAN_OP_OUT_REVERSE)

struct test_entry
{
    const char *testname;
    int         src_fmt;
    int         src_flags;
    int         op;
    int         mask_fmt;
    int         mask_flags;
    int         dst_fmt;
};

typedef struct test_entry test_entry_t;

static const test_entry_t tests_tbl[] =
{
    { "add_8_8_8",             PIXMAN_a8,          0, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_a8 },
    { "add_n_8_8",             PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_a8 },
    { "add_n_8_8888",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_a8r8g8b8 },
    { "add_n_8_x888",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_x8r8g8b8 },
    { "add_n_8_0565",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "add_n_8_1555",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_a1r5g5b5 },
    { "add_n_8_4444",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_a4r4g4b4 },
    { "add_n_8_2222",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_a2r2g2b2 },
    { "add_n_8_2x10",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_x2r10g10b10 },
    { "add_n_8_2a10",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_a8,       0, PIXMAN_a2r10g10b10 },
    { "add_n_8",               PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a8 },
    { "add_n_8888",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "add_n_x888",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "add_n_0565",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "add_n_1555",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a1r5g5b5 },
    { "add_n_4444",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a4r4g4b4 },
    { "add_n_2222",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a2r2g2b2 },
    { "add_n_2x10",            PIXMAN_a2r10g10b10, 1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_x2r10g10b10 },
    { "add_n_2a10",            PIXMAN_a2r10g10b10, 1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a2r10g10b10 },
    { "add_8_8",               PIXMAN_a8,          0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a8 },
    { "add_x888_x888",         PIXMAN_x8r8g8b8,    0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "add_8888_8888",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "add_8888_0565",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "add_8888_1555",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a1r5g5b5 },
    { "add_8888_4444",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a4r4g4b4 },
    { "add_8888_2222",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a2r2g2b2 },
    { "add_0565_0565",         PIXMAN_r5g6b5,      0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "add_1555_1555",         PIXMAN_a1r5g5b5,    0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a1r5g5b5 },
    { "add_0565_2x10",         PIXMAN_r5g6b5,      0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_x2r10g10b10 },
    { "add_2a10_2a10",         PIXMAN_a2r10g10b10, 0, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a2r10g10b10 },
    { "in_n_8_8",              PIXMAN_a8r8g8b8,    1, PIXMAN_OP_IN,      PIXMAN_a8,       0, PIXMAN_a8 },
    { "in_8_8",                PIXMAN_a8,          0, PIXMAN_OP_IN,      PIXMAN_null,     0, PIXMAN_a8 },
    { "src_n_2222",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a2r2g2b2 },
    { "src_n_0565",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "src_n_1555",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a1r5g5b5 },
    { "src_n_4444",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a4r4g4b4 },
    { "src_n_x888",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "src_n_8888",            PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "src_n_2x10",            PIXMAN_a2r10g10b10, 1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_x2r10g10b10 },
    { "src_n_2a10",            PIXMAN_a2r10g10b10, 1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a2r10g10b10 },
    { "src_8888_0565",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "src_0565_8888",         PIXMAN_r5g6b5,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "src_8888_4444",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a4r4g4b4 },
    { "src_8888_2222",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a2r2g2b2 },
    { "src_8888_2x10",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_x2r10g10b10 },
    { "src_8888_2a10",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a2r10g10b10 },
    { "src_0888_0565",         PIXMAN_r8g8b8,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "src_0888_8888",         PIXMAN_r8g8b8,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "src_0888_x888",         PIXMAN_r8g8b8,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "src_0888_8888_rev",     PIXMAN_b8g8r8,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "src_0888_0565_rev",     PIXMAN_b8g8r8,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "src_x888_x888",         PIXMAN_x8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "src_x888_8888",         PIXMAN_x8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "src_8888_8888",         PIXMAN_a8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "src_0565_0565",         PIXMAN_r5g6b5,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "src_1555_0565",         PIXMAN_a1r5g5b5,    0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "src_0565_1555",         PIXMAN_r5g6b5,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a1r5g5b5 },
    { "src_8_8",               PIXMAN_a8,          0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a8 },
    { "src_n_8",               PIXMAN_a8,          1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a8 },
    { "src_n_8_0565",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "src_n_8_1555",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_a1r5g5b5 },
    { "src_n_8_4444",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_a4r4g4b4 },
    { "src_n_8_2222",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_a2r2g2b2 },
    { "src_n_8_x888",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_x8r8g8b8 },
    { "src_n_8_8888",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_a8r8g8b8 },
    { "src_n_8_2x10",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_x2r10g10b10 },
    { "src_n_8_2a10",          PIXMAN_a8r8g8b8,    1, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_a2r10g10b10 },
    { "src_8888_8_0565",       PIXMAN_a8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "src_0888_8_0565",       PIXMAN_r8g8b8,      0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "src_0888_8_8888",       PIXMAN_r8g8b8,      0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_a8r8g8b8 },
    { "src_0888_8_x888",       PIXMAN_r8g8b8,      0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_x8r8g8b8 },
    { "src_x888_8_x888",       PIXMAN_x8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_x8r8g8b8 },
    { "src_x888_8_8888",       PIXMAN_x8r8g8b8,    0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_a8r8g8b8 },
    { "src_0565_8_0565",       PIXMAN_r5g6b5,      0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "src_1555_8_0565",       PIXMAN_a1r5g5b5,    0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "src_0565_8_1555",       PIXMAN_r5g6b5,      0, PIXMAN_OP_SRC,     PIXMAN_a8,       0, PIXMAN_a1r5g5b5 },
    { "over_n_x888",           PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "over_n_8888",           PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "over_n_0565",           PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "over_n_1555",           PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_null,     0, PIXMAN_a1r5g5b5 },
    { "over_8888_0565",        PIXMAN_a8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "over_8888_8888",        PIXMAN_a8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_null,     0, PIXMAN_a8r8g8b8 },
    { "over_8888_x888",        PIXMAN_a8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "over_x888_8_0565",      PIXMAN_x8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "over_x888_8_8888",      PIXMAN_x8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_a8r8g8b8 },
    { "over_n_8_0565",         PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "over_n_8_1555",         PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_a1r5g5b5 },
    { "over_n_8_4444",         PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_a4r4g4b4 },
    { "over_n_8_2222",         PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_a2r2g2b2 },
    { "over_n_8_x888",         PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_x8r8g8b8 },
    { "over_n_8_8888",         PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_a8r8g8b8 },
    { "over_n_8_2x10",         PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_x2r10g10b10 },
    { "over_n_8_2a10",         PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8,       0, PIXMAN_a2r10g10b10 },
    { "over_n_8888_8888_ca",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8r8g8b8, 2, PIXMAN_a8r8g8b8 },
    { "over_n_8888_x888_ca",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8r8g8b8, 2, PIXMAN_x8r8g8b8 },
    { "over_n_8888_0565_ca",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8r8g8b8, 2, PIXMAN_r5g6b5 },
    { "over_n_8888_1555_ca",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8r8g8b8, 2, PIXMAN_a1r5g5b5 },
    { "over_n_8888_4444_ca",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8r8g8b8, 2, PIXMAN_a4r4g4b4 },
    { "over_n_8888_2222_ca",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8r8g8b8, 2, PIXMAN_a2r2g2b2 },
    { "over_n_8888_2x10_ca",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8r8g8b8, 2, PIXMAN_x2r10g10b10 },
    { "over_n_8888_2a10_ca",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER,    PIXMAN_a8r8g8b8, 2, PIXMAN_a2r10g10b10 },
    { "over_8888_n_8888",      PIXMAN_a8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_a8,       1, PIXMAN_a8r8g8b8 },
    { "over_8888_n_x888",      PIXMAN_a8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_a8,       1, PIXMAN_x8r8g8b8 },
    { "over_8888_n_0565",      PIXMAN_a8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_a8,       1, PIXMAN_r5g6b5 },
    { "over_8888_n_1555",      PIXMAN_a8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_a8,       1, PIXMAN_a1r5g5b5 },
    { "over_x888_n_8888",      PIXMAN_x8r8g8b8,    0, PIXMAN_OP_OVER,    PIXMAN_a8,       1, PIXMAN_a8r8g8b8 },
    { "outrev_n_8_0565",       PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OUT_REV, PIXMAN_a8,       0, PIXMAN_r5g6b5 },
    { "outrev_n_8_1555",       PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OUT_REV, PIXMAN_a8,       0, PIXMAN_a1r5g5b5 },
    { "outrev_n_8_x888",       PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OUT_REV, PIXMAN_a8,       0, PIXMAN_x8r8g8b8 },
    { "outrev_n_8_8888",       PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OUT_REV, PIXMAN_a8,       0, PIXMAN_a8r8g8b8 },
    { "outrev_n_8888_0565_ca", PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OUT_REV, PIXMAN_a8r8g8b8, 2, PIXMAN_r5g6b5 },
    { "outrev_n_8888_1555_ca", PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OUT_REV, PIXMAN_a8r8g8b8, 2, PIXMAN_a1r5g5b5 },
    { "outrev_n_8888_x888_ca", PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OUT_REV, PIXMAN_a8r8g8b8, 2, PIXMAN_x8r8g8b8 },
    { "outrev_n_8888_8888_ca", PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OUT_REV, PIXMAN_a8r8g8b8, 2, PIXMAN_a8r8g8b8 },
    { "over_reverse_n_8888",   PIXMAN_a8r8g8b8,    1, PIXMAN_OP_OVER_REVERSE, PIXMAN_null, 0, PIXMAN_a8r8g8b8 },
    { "in_reverse_8888_8888",  PIXMAN_a8r8g8b8,    0, PIXMAN_OP_IN_REVERSE, PIXMAN_null,  0, PIXMAN_a8r8g8b8 },
    { "pixbuf",                PIXMAN_x8b8g8r8,    0, PIXMAN_OP_SRC,     PIXMAN_a8b8g8r8, 0, PIXMAN_a8r8g8b8 },
    { "rpixbuf",               PIXMAN_x8b8g8r8,    0, PIXMAN_OP_SRC,     PIXMAN_a8b8g8r8, 0, PIXMAN_a8b8g8r8 },
};

static const test_entry_t special_patterns[] =
{
    { "add_n_2x10",            PIXMAN_a2r10g10b10, 1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_x2r10g10b10 },
    { "add_n_2a10",            PIXMAN_a2r10g10b10, 1, PIXMAN_OP_ADD,     PIXMAN_null,     0, PIXMAN_a2r10g10b10 },
    { "src_n_2x10",            PIXMAN_a2r10g10b10, 1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_x2r10g10b10 },
    { "src_n_2a10",            PIXMAN_a2r10g10b10, 1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a2r10g10b10 },
    { "src_0888_8888_rev",     PIXMAN_b8g8r8,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_x8r8g8b8 },
    { "src_0888_0565_rev",     PIXMAN_b8g8r8,      0, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_r5g6b5 },
    { "src_n_8",               PIXMAN_a8,          1, PIXMAN_OP_SRC,     PIXMAN_null,     0, PIXMAN_a8 },
    { "pixbuf",                PIXMAN_x8b8g8r8,    0, PIXMAN_OP_SRC,     PIXMAN_a8b8g8r8, 0, PIXMAN_a8r8g8b8 },
    { "rpixbuf",               PIXMAN_x8b8g8r8,    0, PIXMAN_OP_SRC,     PIXMAN_a8b8g8r8, 0, PIXMAN_a8b8g8r8 },
};

/* Returns the sub-string's end pointer in string. */
static const char *
copy_sub_string (char       *buf,
                 const char *string,
                 const char *scan_from,
                 const char *end)
{
    const char *delim;
    size_t n;

    delim = strchr (scan_from, '_');
    if (!delim)
        delim = end;

    n = delim - string;
    strncpy(buf, string, n);
    buf[n] = '\0';

    return delim;
}

static pixman_op_t
parse_longest_operator (char *buf, const char **strp, const char *end)
{
    const char *p = *strp;
    const char *sub_end;
    const char *best_end = p;
    pixman_op_t best_op = PIXMAN_OP_NONE;
    pixman_op_t op;

    while (p < end)
    {
        sub_end = copy_sub_string (buf, *strp, p, end);
        op = operator_from_string (buf);
        p = sub_end + 1;

        if (op != PIXMAN_OP_NONE)
        {
            best_end = p;
            best_op = op;
        }
    }

    *strp = best_end;
    return best_op;
}

static pixman_format_code_t
parse_format (char *buf, const char **p, const char *end)
{
    pixman_format_code_t format;
    const char *delim;

    if (*p >= end)
        return PIXMAN_null;

    delim = copy_sub_string (buf, *p, *p, end);
    format = format_from_string (buf);

    if (format != PIXMAN_null)
        *p = delim + 1;

    return format;
}

static int
parse_test_pattern (test_entry_t *test, const char *pattern)
{
    const char *p = pattern;
    const char *end = pattern + strlen (pattern);
    char buf[1024];
    pixman_format_code_t format[3];
    int i;

    if (strlen (pattern) > sizeof (buf) - 1)
        return -1;

    /* Special cases that the parser cannot produce. */
    for (i = 0; i < ARRAY_LENGTH (special_patterns); i++)
    {
        if (strcmp (pattern, special_patterns[i].testname) == 0)
        {
            *test = special_patterns[i];
            return 0;
        }
    }

    test->testname = pattern;

    /* Extract operator, may contain delimiters,
     * so take the longest string that matches.
     */
    test->op = parse_longest_operator (buf, &p, end);
    if (test->op == PIXMAN_OP_NONE)
        return -1;

    /* extract up to three pixel formats */
    format[0] = parse_format (buf, &p, end);
    format[1] = parse_format (buf, &p, end);
    format[2] = parse_format (buf, &p, end);

    if (format[0] == PIXMAN_null || format[1] == PIXMAN_null)
        return -1;

    /* recognize CA flag */
    test->mask_flags = 0;
    if (p < end)
    {
        if (strcmp (p, "ca") == 0)
            test->mask_flags |= CA_FLAG;
        else
            return -1; /* trailing garbage */
    }

    test->src_fmt = format[0];
    if (format[2] == PIXMAN_null)
    {
        test->mask_fmt = PIXMAN_null;
        test->dst_fmt = format[1];
    }
    else
    {
        test->mask_fmt = format[1];
        test->dst_fmt = format[2];
    }

    test->src_flags = 0;
    if (test->src_fmt == PIXMAN_solid)
    {
        test->src_fmt = PIXMAN_a8r8g8b8;
        test->src_flags |= SOLID_FLAG;
    }

    if (test->mask_fmt == PIXMAN_solid)
    {
        if (test->mask_flags & CA_FLAG)
            test->mask_fmt = PIXMAN_a8r8g8b8;
        else
            test->mask_fmt = PIXMAN_a8;

        test->mask_flags |= SOLID_FLAG;
    }

    return 0;
}

static int
check_int (int got, int expected, const char *name, const char *field)
{
    if (got == expected)
        return 0;

    printf ("%s: %s failure: expected %d, got %d.\n",
            name, field, expected, got);

    return 1;
}

static int
check_format (int got, int expected, const char *name, const char *field)
{
    if (got == expected)
        return 0;

    printf ("%s: %s failure: expected %s (%#x), got %s (%#x).\n",
            name, field,
            format_name (expected), expected,
            format_name (got), got);

    return 1;
}

static void
parser_self_test (void)
{
    const test_entry_t *ent;
    test_entry_t test;
    int fails = 0;
    int i;

    for (i = 0; i < ARRAY_LENGTH (tests_tbl); i++)
    {
        ent = &tests_tbl[i];

        if (parse_test_pattern (&test, ent->testname) < 0)
        {
            printf ("parsing failed for '%s'\n", ent->testname);
            fails++;
            continue;
        }

        fails += check_format (test.src_fmt, ent->src_fmt,
                               ent->testname, "src_fmt");
        fails += check_format (test.mask_fmt, ent->mask_fmt,
                               ent->testname, "mask_fmt");
        fails += check_format (test.dst_fmt, ent->dst_fmt,
                               ent->testname, "dst_fmt");
        fails += check_int    (test.src_flags, ent->src_flags,
                               ent->testname, "src_flags");
        fails += check_int    (test.mask_flags, ent->mask_flags,
                               ent->testname, "mask_flags");
        fails += check_int    (test.op, ent->op, ent->testname, "op");
    }

    if (fails)
    {
        printf ("Parser self-test failed.\n");
        exit (EXIT_FAILURE);
    }

    if (!use_csv_output)
        printf ("Parser self-test complete.\n");
}

static void
print_test_details (const test_entry_t *test)
{
    printf ("%s: %s, src %s%s, mask %s%s%s, dst %s\n",
            test->testname,
            operator_name (test->op),
            format_name (test->src_fmt),
            test->src_flags & SOLID_FLAG ? " solid" : "",
            format_name (test->mask_fmt),
            test->mask_flags & SOLID_FLAG ? " solid" : "",
            test->mask_flags & CA_FLAG ? " CA" : "",
            format_name (test->dst_fmt));
}

static void
run_one_test (const char *pattern, double bandwidth_, pixman_bool_t prdetails)
{
    test_entry_t test;

    if (parse_test_pattern (&test, pattern) < 0)
    {
        printf ("Error: Could not parse the test pattern '%s'.\n", pattern);
        return;
    }

    if (prdetails)
    {
        print_test_details (&test);
        printf ("---\n");
    }

    bench_composite (pattern,
                     test.src_fmt,
                     test.src_flags,
                     test.op,
                     test.mask_fmt,
                     test.mask_flags,
                     test.dst_fmt,
                     bandwidth_ / 8);
}

static void
run_default_tests (double bandwidth_)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (tests_tbl); i++)
        run_one_test (tests_tbl[i].testname, bandwidth_, FALSE);
}

static void
print_explanation (void)
{
    printf ("Benchmark for a set of most commonly used functions\n");
    printf ("---\n");
    printf ("All results are presented in millions of pixels per second\n");
    printf ("L1  - small Xx1 rectangle (fitting L1 cache), always blitted at the same\n");
    printf ("      memory location with small drift in horizontal direction\n");
    printf ("L2  - small XxY rectangle (fitting L2 cache), always blitted at the same\n");
    printf ("      memory location with small drift in horizontal direction\n");
    printf ("M   - large %dx%d rectangle, always blitted at the same\n",
            WIDTH - 64, HEIGHT);
    printf ("      memory location with small drift in horizontal direction\n");
    printf ("HT  - random rectangles with %dx%d average size are copied from\n",
            TILEWIDTH, TILEWIDTH);
    printf ("      one %dx%d buffer to another, traversing from left to right\n",
            WIDTH, HEIGHT);
    printf ("      and from top to bottom\n");
    printf ("VT  - random rectangles with %dx%d average size are copied from\n",
            TILEWIDTH, TILEWIDTH);
    printf ("      one %dx%d buffer to another, traversing from top to bottom\n",
            WIDTH, HEIGHT);
    printf ("      and from left to right\n");
    printf ("R   - random rectangles with %dx%d average size are copied from\n",
            TILEWIDTH, TILEWIDTH);
    printf ("      random locations of one %dx%d buffer to another\n",
            WIDTH, HEIGHT);
    printf ("RT  - as R, but %dx%d average sized rectangles are copied\n",
            TINYWIDTH, TINYWIDTH);
    printf ("---\n");
}

static void
print_speed_scaling (double bw)
{
    printf ("reference memcpy speed = %.1fMB/s (%.1fMP/s for 32bpp fills)\n",
            bw / 1000000., bw / 4000000);

    if (use_scaling)
    {
	printf ("---\n");
	if (filter == PIXMAN_FILTER_BILINEAR)
	    printf ("BILINEAR scaling\n");
	else if (filter == PIXMAN_FILTER_NEAREST)
	    printf ("NEAREST scaling\n");
	else
	    printf ("UNKNOWN scaling\n");
    }

    printf ("---\n");
}

static void
usage (const char *progname)
{
    printf ("Usage: %s [-b] [-n] [-c] [-m M] pattern\n", progname);
    printf ("  -n : benchmark nearest scaling\n");
    printf ("  -b : benchmark bilinear scaling\n");
    printf ("  -c : print output as CSV data\n");
    printf ("  -m M : set reference memcpy speed to M MB/s instead of measuring it\n");
}

int
main (int argc, char *argv[])
{
    int i;
    const char *pattern = NULL;

    for (i = 1; i < argc; i++)
    {
	if (argv[i][0] == '-')
	{
	    if (strchr (argv[i] + 1, 'b'))
	    {
		use_scaling = TRUE;
		filter = PIXMAN_FILTER_BILINEAR;
	    }
	    else if (strchr (argv[i] + 1, 'n'))
	    {
		use_scaling = TRUE;
		filter = PIXMAN_FILTER_NEAREST;
	    }

	    if (strchr (argv[i] + 1, 'c'))
		use_csv_output = TRUE;

	    if (strcmp (argv[i], "-m") == 0 && i + 1 < argc)
		bandwidth = atof (argv[++i]) * 1e6;
	}
	else
	{
	    if (pattern)
	    {
		pattern = NULL;
		printf ("Error: extra arguments given.\n");
		break;
	    }
	    pattern = argv[i];
	}
    }

    if (!pattern)
    {
	usage (argv[0]);
	return 1;
    }

    parser_self_test ();

    src = aligned_malloc (4096, BUFSIZE * 3);
    memset (src, 0xCC, BUFSIZE * 3);
    dst = src + (BUFSIZE / 4);
    mask = dst + (BUFSIZE / 4);

    if (!use_csv_output)
        print_explanation ();

    if (bandwidth < 1.0)
        bandwidth = bench_memcpy ();
    if (!use_csv_output)
        print_speed_scaling (bandwidth);

    if (strcmp (pattern, "all") == 0)
        run_default_tests (bandwidth);
    else
        run_one_test (pattern, bandwidth, !use_csv_output);

    free (src);
    return 0;
}
