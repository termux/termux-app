#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

static const pixman_fixed_t entries[] =
{
    pixman_double_to_fixed (-1.0),
    pixman_double_to_fixed (-0.5),
    pixman_double_to_fixed (-1/3.0),
    pixman_double_to_fixed (0.0),
    pixman_double_to_fixed (0.5),
    pixman_double_to_fixed (1.0),
    pixman_double_to_fixed (1.5),
    pixman_double_to_fixed (2.0),
    pixman_double_to_fixed (3.0),
};

#define SIZE 12

static uint32_t
test_scale (const pixman_transform_t *xform, uint32_t crc)
{
    uint32_t *srcbuf, *dstbuf;
    pixman_image_t *src, *dest;

    srcbuf = malloc (SIZE * SIZE * 4);
    prng_randmemset (srcbuf, SIZE * SIZE * 4, 0);
    src = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, SIZE, SIZE, srcbuf, SIZE * 4);

    dstbuf = malloc (SIZE * SIZE * 4);
    prng_randmemset (dstbuf, SIZE * SIZE * 4, 0);
    dest = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, SIZE, SIZE, dstbuf, SIZE * 4);

    pixman_image_set_transform (src, xform);
    pixman_image_set_repeat (src, PIXMAN_REPEAT_NORMAL);
    pixman_image_set_filter (src, PIXMAN_FILTER_BILINEAR, NULL, 0);

    image_endian_swap (src);
    image_endian_swap (dest);

    pixman_image_composite (PIXMAN_OP_SRC,
			    src, NULL, dest,
			    0, 0, 0, 0, 0, 0,
			    SIZE, SIZE);

    crc = compute_crc32_for_image (crc, dest);

    pixman_image_unref (src);
    pixman_image_unref (dest);

    free (srcbuf);
    free (dstbuf);

    return crc;
}

#if BILINEAR_INTERPOLATION_BITS == 7
#define CHECKSUM 0x02169677
#elif BILINEAR_INTERPOLATION_BITS == 4
#define CHECKSUM 0xE44B29AC
#else
#define CHECKSUM 0x00000000
#endif

int
main (int argc, const char *argv[])
{
    const pixman_fixed_t *end = entries + ARRAY_LENGTH (entries);
    const pixman_fixed_t *t0, *t1, *t2, *t3, *t4, *t5;
    uint32_t crc = 0;

    prng_srand (0x56EA1DBD);

    for (t0 = entries; t0 < end; ++t0)
    {
	for (t1 = entries; t1 < end; ++t1)
	{
	    for (t2 = entries; t2 < end; ++t2)
	    {
		for (t3 = entries; t3 < end; ++t3)
		{
		    for (t4 = entries; t4 < end; ++t4)
		    {
			for (t5 = entries; t5 < end; ++t5)
			{
			    pixman_transform_t xform = {
				{ { *t0, *t1, *t2 },
				  { *t3, *t4, *t5 },
				  { 0, 0, pixman_fixed_1 } }
			    };

			    crc = test_scale (&xform, crc);
			}
		    }
		}
	    }
	}
    }

    if (crc != CHECKSUM)
    {
	printf ("filter-reduction-test failed! (checksum=0x%08X, expected 0x%08X)\n", crc, CHECKSUM);
	return 1;
    }
    else
    {
	printf ("filter-reduction-test passed (checksum=0x%08X)\n", crc);
	return 0;
    }
}
