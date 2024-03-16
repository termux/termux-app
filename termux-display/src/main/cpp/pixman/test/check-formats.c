#include <ctype.h>
#include "utils.h"

static int
check_op (pixman_op_t          op,
          pixman_format_code_t src_format,
          pixman_format_code_t dest_format)
{
    uint32_t src_alpha_mask, src_green_mask;
    uint32_t dest_alpha_mask, dest_green_mask;
    pixel_checker_t src_checker, dest_checker;
    pixman_image_t *si, *di;
    uint32_t sa, sg, da, dg;
    uint32_t s, d;
    int retval = 0;

    pixel_checker_init (&src_checker, src_format);
    pixel_checker_init (&dest_checker, dest_format);

    pixel_checker_get_masks (
        &src_checker, &src_alpha_mask, NULL, &src_green_mask, NULL);
    pixel_checker_get_masks (
        &dest_checker, &dest_alpha_mask, NULL, &dest_green_mask, NULL);

    /* printf ("masks: %x %x %x %x\n", */
    /* 	    src_alpha_mask, src_green_mask, */
    /* 	    dest_alpha_mask, dest_green_mask); */

    si = pixman_image_create_bits (src_format, 1, 1, &s, 4);
    di = pixman_image_create_bits (dest_format, 1, 1, &d, 4);

    sa = 0;
    do
    {
        sg = 0;
        do
        {
            da = 0;
            do
            {
                dg = 0;
                do
                {
                    color_t src_color, dest_color, result_color;
                    uint32_t orig_d;

                    s = sa | sg;
                    d = da | dg;

                    orig_d = d;

		    pixel_checker_convert_pixel_to_color (&src_checker, s, &src_color);
		    pixel_checker_convert_pixel_to_color (&dest_checker, d, &dest_color);

		    do_composite (op, &src_color, NULL, &dest_color, &result_color, FALSE);


		    if (!is_little_endian())
                    {
			s <<= 32 - PIXMAN_FORMAT_BPP (src_format);
			d <<= 32 - PIXMAN_FORMAT_BPP (dest_format);
                    }

		    pixman_image_composite32 (op, si, NULL, di,
					      0, 0, 0, 0, 0, 0, 1, 1);

		    if (!is_little_endian())
                        d >>= (32 - PIXMAN_FORMAT_BPP (dest_format));

                    if (!pixel_checker_check (&dest_checker, d, &result_color))
                    {
                        printf ("---- test failed ----\n");
                        printf ("operator: %-32s\n", operator_name (op));
                        printf ("source:   %-12s pixel: %08x\n", format_name (src_format), s);
                        printf ("dest:     %-12s pixel: %08x\n", format_name (dest_format), orig_d);
                        printf ("got:      %-12s pixel: %08x\n", format_name (dest_format), d);

                        retval = 1;
                    }

                    dg -= dest_green_mask;
                    dg &= dest_green_mask;
                }
                while (dg != 0);

                da -= dest_alpha_mask;
                da &= dest_alpha_mask;
            }
            while (da != 0);

            sg -= src_green_mask;
            sg &= src_green_mask;
        }
        while (sg != 0);

        sa -= src_alpha_mask;
        sa &= src_alpha_mask;
    }
    while (sa != 0);

    pixman_image_unref (si);
    pixman_image_unref (di);

    return retval;
}

int
main (int argc, char **argv)
{
    enum { OPTION_OP, OPTION_SRC, OPTION_DEST, LAST_OPTION } option;
    pixman_format_code_t src_fmt, dest_fmt;
    pixman_op_t op;

    op = PIXMAN_OP_NONE;
    src_fmt = PIXMAN_null;
    dest_fmt = PIXMAN_null;

    argc--;
    argv++;

    for (option = OPTION_OP; option < LAST_OPTION; ++option)
    {
        char *arg = NULL;

        if (argc)
        {
            argc--;
            arg = *argv++;
        }

        switch (option)
        {
        case OPTION_OP:
            if (!arg)
                printf ("  - missing operator\n");
            else if ((op = operator_from_string (arg)) == PIXMAN_OP_NONE)
                printf ("  - unknown operator %s\n", arg);
            break;

        case OPTION_SRC:
            if (!arg)
                printf ("  - missing source format\n");
            else if ((src_fmt = format_from_string (arg)) == PIXMAN_null)
                printf ("  - unknown source format %s\n", arg);
            break;

        case OPTION_DEST:
            if (!arg)
                printf ("  - missing destination format\n");
            else if ((dest_fmt = format_from_string (arg)) == PIXMAN_null)
                printf ("  - unknown destination format %s\n", arg);
            break;

        default:
            assert (0);
            break;
        }
    }

    while (argc--)
    {
        op = PIXMAN_OP_NONE;
        printf ("  - unexpected argument: %s\n", *argv++);
    }

    if (op == PIXMAN_OP_NONE || src_fmt == PIXMAN_null || dest_fmt == PIXMAN_null)
    {
        printf ("\nUsage:\n    check-formats <operator> <src-format> <dest-format>\n\n");
        list_operators();
        list_formats();

        return -1;
    }

    return check_op (op, src_fmt, dest_fmt);
}
