#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

static void
make_random_region (pixman_region32_t *region)
{
    int n_boxes;

    pixman_region32_init (region);

    n_boxes = prng_rand_n (64);
    while (n_boxes--)
    {
	int32_t x, y;
	uint32_t w, h;

	x = (int32_t)prng_rand() >> 2;
	y = (int32_t)prng_rand() >> 2;
	w = prng_rand() >> 2;
	h = prng_rand() >> 2;

	pixman_region32_union_rect (region, region, x, y, w, h);
    }
}

static void
print_box (pixman_box32_t *box)
{
    printf ("    %d %d %d %d\n", box->x1, box->y1, box->x2, box->y2);
}

static int32_t
random_coord (pixman_region32_t *region, pixman_bool_t x)
{
    pixman_box32_t *b, *bb;
    int n_boxes;
    int begin, end;

    if (prng_rand_n (14))
    {
	bb = pixman_region32_rectangles (region, &n_boxes);
	if (n_boxes == 0)
	    goto use_extent;
	b = bb + prng_rand_n (n_boxes);
    }
    else
    {
    use_extent:
	b = pixman_region32_extents (region);
	n_boxes = 1;
    }

    if (x)
    {
	begin = b->x1;
	end = b->x2;
    }
    else
    {
	begin = b->y1;
	end = b->y2;
    }

    switch (prng_rand_n (5))
    {
    case 0:
	return begin - prng_rand();
    case 1:
	return end + prng_rand ();
    case 2:
	return end;
    case 3:
	return begin;
    default:
	return (end - begin) / 2 + begin;
    }
    return 0;
}

static uint32_t
compute_crc32_u32 (uint32_t crc32, uint32_t v)
{
    if (!is_little_endian())
    {
	v = ((v & 0xff000000) >> 24)	|
	    ((v & 0x00ff0000) >> 8)	|
	    ((v & 0x0000ff00) << 8)	|
	    ((v & 0x000000ff) << 24);
    }

    return compute_crc32 (crc32, &v, sizeof (int32_t));
}

static uint32_t
crc32_box32 (uint32_t crc32, pixman_box32_t *box)
{
    crc32 = compute_crc32_u32 (crc32, box->x1);
    crc32 = compute_crc32_u32 (crc32, box->y1);
    crc32 = compute_crc32_u32 (crc32, box->x2);
    crc32 = compute_crc32_u32 (crc32, box->y2);

    return crc32;
}

static uint32_t
test_region_contains_rectangle (int i, int verbose)
{
    pixman_box32_t box;
    pixman_box32_t rbox = { 0, 0, 0, 0 };
    pixman_region32_t region;
    uint32_t r, r1, r2, r3, r4, crc32;

    prng_srand (i);

    make_random_region (&region);

    box.x1 = random_coord (&region, TRUE);
    box.x2 = box.x1 + prng_rand ();
    box.y1 = random_coord (&region, FALSE);
    box.y2 = box.y1 + prng_rand ();

    if (verbose)
    {
	int n_rects;
	pixman_box32_t *boxes;

	boxes = pixman_region32_rectangles (&region, &n_rects);

	printf ("region:\n");
	while (n_rects--)
	    print_box (boxes++);
	printf ("box:\n");
	print_box (&box);
    }

    crc32 = 0;

    r1 = pixman_region32_contains_point (&region, box.x1, box.y1, &rbox);
    crc32 = crc32_box32 (crc32, &rbox);
    r2 = pixman_region32_contains_point (&region, box.x1, box.y2, &rbox);
    crc32 = crc32_box32 (crc32, &rbox);
    r3 = pixman_region32_contains_point (&region, box.x2, box.y1, &rbox);
    crc32 = crc32_box32 (crc32, &rbox);
    r4 = pixman_region32_contains_point (&region, box.x2, box.y2, &rbox);
    crc32 = crc32_box32 (crc32, &rbox);

    r = pixman_region32_contains_rectangle (&region, &box);
    r = (i << 8) | (r << 4) | (r1 << 3) | (r2 << 2) | (r3 << 1) | (r4 << 0);

    crc32 = compute_crc32_u32 (crc32, r);

    if (verbose)
	printf ("results: %d %d %d %d %d\n", (r & 0xf0) >> 4, r1, r2, r3, r4);

    pixman_region32_fini (&region);

    return crc32;
}

int
main (int argc, const char *argv[])
{
    return fuzzer_test_main ("region_contains",
			     1000000,
			     0x548E0F3F,
			     test_region_contains_rectangle,
			     argc, argv);
}
