#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

typedef struct
{
    int				width;
    int				height;
    int				stride;
    pixman_format_code_t	format;
    
} image_info_t;

typedef struct
{
    pixman_op_t		op;
    
    image_info_t	src;
    image_info_t	dest;

    int			src_x;
    int			src_y;
    int			dest_x;
    int			dest_y;
    int			width;
    int			height;
} composite_info_t;

const composite_info_t info[] =
{
    {
	PIXMAN_OP_SRC,
	{  3, 6, 16, PIXMAN_a8r8g8b8 },
	{  5, 7, 20, PIXMAN_x8r8g8b8 },
	1, 8,
	1, -1,
	1, 8
    },
    {
	PIXMAN_OP_SRC,
	{ 7, 5, 36, PIXMAN_a8r8g8b8 },
	{ 6, 5, 28, PIXMAN_x8r8g8b8 },
	8, 5,
	5, 3,
	1, 2
    },
    {
	PIXMAN_OP_OVER,
	{ 10, 10, 40, PIXMAN_a2b10g10r10 },
	{ 10, 10, 40, PIXMAN_a2b10g10r10 },
	0, 0,
	0, 0,
	10, 10
    },
    {
	PIXMAN_OP_OVER,
	{ 10, 10, 40, PIXMAN_x2b10g10r10 },
	{ 10, 10, 40, PIXMAN_x2b10g10r10 },
	0, 0,
	0, 0,
	10, 10
    },
};

static pixman_image_t *
make_image (const image_info_t *info)
{
    char *data = malloc (info->stride * info->height);
    int i;

    for (i = 0; i < info->height * info->stride; ++i)
	data[i] = (i % 255) ^ (((i % 16) << 4) | (i & 0xf0));

    return pixman_image_create_bits (info->format, info->width, info->height, (uint32_t *)data, info->stride);
}
    
static void
test_composite (const composite_info_t *info)
{
    pixman_image_t *src = make_image (&info->src);
    pixman_image_t *dest = make_image (&info->dest);

    pixman_image_composite (PIXMAN_OP_SRC, src, NULL, dest,
			    info->src_x, info->src_y,
			    0, 0,
			    info->dest_x, info->dest_y,
			    info->width, info->height);
}



int
main (int argc, char **argv)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (info); ++i)
	test_composite (&info[i]);
    
    return 0;
}
