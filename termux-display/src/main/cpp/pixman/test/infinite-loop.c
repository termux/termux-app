#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

int
main (int argc, char **argv)
{
#define SRC_WIDTH 16
#define SRC_HEIGHT 12
#define DST_WIDTH 7
#define DST_HEIGHT 2

    static const pixman_transform_t transform = {
	{ { 0x200017bd, 0x00000000, 0x000e6465 },
	  { 0x00000000, 0x000a42fd, 0x000e6465 },
	  { 0x00000000, 0x00000000, 0x00010000 },
	}
    };
    pixman_image_t *src, *dest;

    src = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, SRC_WIDTH, SRC_HEIGHT, NULL, -1);
    dest = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, DST_WIDTH, DST_HEIGHT, NULL, -1);

    pixman_image_set_transform (src, &transform);
    pixman_image_set_repeat (src, PIXMAN_REPEAT_NORMAL);
    pixman_image_set_filter (src, PIXMAN_FILTER_BILINEAR, NULL, 0);

    if (argc == 1 || strcmp (argv[1], "-nf") != 0)
	fail_after (1, "infinite loop detected");

    pixman_image_composite (
	PIXMAN_OP_OVER, src, NULL, dest, -3, -3, 0, 0, 0, 0, 6, 2);

    return 0;
}
