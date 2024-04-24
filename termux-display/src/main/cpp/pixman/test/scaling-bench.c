#include <stdlib.h>
#include "utils.h"

#define SOURCE_WIDTH 320
#define SOURCE_HEIGHT 240
#define TEST_REPEATS 3

static pixman_image_t *
make_source (void)
{
    size_t n_bytes = (SOURCE_WIDTH + 2) * (SOURCE_HEIGHT + 2) * 4;
    uint32_t *data = malloc (n_bytes);
    pixman_image_t *source;

    prng_randmemset (data, n_bytes, 0);
    
    source = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, SOURCE_WIDTH + 2, SOURCE_HEIGHT + 2,
	data,
	(SOURCE_WIDTH + 2) * 4);

    pixman_image_set_filter (source, PIXMAN_FILTER_BILINEAR, NULL, 0);

    return source;
}

int
main ()
{
    double scale;
    pixman_image_t *src;

    prng_srand (23874);
    
    src = make_source ();
    printf ("# %-6s %-22s   %-14s %-12s\n",
	    "ratio",
	    "resolutions",
	    "time / ms",
	    "time per pixel / ns");
    for (scale = 0.1; scale < 10.005; scale += 0.01)
    {
	int i;
	int dest_width = SOURCE_WIDTH * scale + 0.5;
	int dest_height = SOURCE_HEIGHT * scale + 0.5;
	int dest_byte_stride = (dest_width * 4 + 15) & ~15;
	pixman_fixed_t s = (1 / scale) * 65536.0 + 0.5;
	pixman_transform_t transform;
	pixman_image_t *dest;
	double t1, t2, t = -1;
	uint32_t *dest_buf = aligned_malloc (16, dest_byte_stride * dest_height);
	memset (dest_buf, 0, dest_byte_stride * dest_height);

	pixman_transform_init_scale (&transform, s, s);
	pixman_image_set_transform (src, &transform);
	
	dest = pixman_image_create_bits (
	    PIXMAN_a8r8g8b8, dest_width, dest_height, dest_buf, dest_byte_stride);

	for (i = 0; i < TEST_REPEATS; i++)
	{
	    t1 = gettime();
	    pixman_image_composite (
		PIXMAN_OP_OVER, src, NULL, dest,
		scale, scale, 0, 0, 0, 0, dest_width, dest_height);
	    t2 = gettime();
	    if (t < 0 || t2 - t1 < t)
		t = t2 - t1;
	}

	printf ("%6.2f : %4dx%-4d => %4dx%-4d : %12.4f : %12.4f\n",
		scale, SOURCE_WIDTH, SOURCE_HEIGHT, dest_width, dest_height,
		t * 1000, (t / (dest_width * dest_height)) * 1000000000);

	pixman_image_unref (dest);
	free (dest_buf);
    }

    return 0;
}
