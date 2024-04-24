#include <stdlib.h>
#include "utils.h"

static const pixman_op_t pdf_ops[] =
{
    PIXMAN_OP_MULTIPLY,
    PIXMAN_OP_SCREEN,
    PIXMAN_OP_OVERLAY,
    PIXMAN_OP_DARKEN,
    PIXMAN_OP_LIGHTEN,
    PIXMAN_OP_COLOR_DODGE,
    PIXMAN_OP_COLOR_BURN,
    PIXMAN_OP_HARD_LIGHT,
    PIXMAN_OP_SOFT_LIGHT,
    PIXMAN_OP_DIFFERENCE,
    PIXMAN_OP_EXCLUSION,
    PIXMAN_OP_HSL_HUE,
    PIXMAN_OP_HSL_SATURATION,
    PIXMAN_OP_HSL_COLOR,
    PIXMAN_OP_HSL_LUMINOSITY
};

static const uint32_t pixels[] =
{
    0x00808080,
    0x80123456,
    0x00000000,
    0xffffffff,
    0x00ffffff,
    0x80808080,
    0x00123456,
};

int
main ()
{
    int o, s, m, d;

    enable_divbyzero_exceptions();

    for (o = 0; o < ARRAY_LENGTH (pdf_ops); ++o)
    {
	pixman_op_t op = pdf_ops[o];

	for (s = 0; s < ARRAY_LENGTH (pixels); ++s)
	{
	    pixman_image_t *src;

	    src = pixman_image_create_bits (
		PIXMAN_a8r8g8b8, 1, 1, (uint32_t *)&(pixels[s]), 4);

	    for (m = -1; m < ARRAY_LENGTH (pixels); ++m)
	    {
		pixman_image_t *msk = NULL;
		if (m >= 0)
		{
		    msk = pixman_image_create_bits (
			PIXMAN_a8r8g8b8, 1, 1, (uint32_t *)&(pixels[m]), 4);
		}

		for (d = 0; d < ARRAY_LENGTH (pixels); ++d)
		{
		    pixman_image_t *dst;
		    uint32_t dp = pixels[d];

		    dst = pixman_image_create_bits (
			PIXMAN_a8r8g8b8, 1, 1, &dp, 4);

		    pixman_image_composite (op, src, msk, dst,
					    0, 0, 0, 0, 0, 0, 1, 1);

		    pixman_image_unref (dst);
		}
		if (msk)
		    pixman_image_unref (msk);
	    }

	    pixman_image_unref (src);
	}
    }

    return 0;
}
