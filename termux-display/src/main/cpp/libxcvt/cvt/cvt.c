/*
 * Copyright 2005-2006 Luc Verhaegen.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* Standalone VESA CVT standard timing modelines generator. */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <libxcvt/libxcvt.h>

bool
cvt_is_standard(int hdisplay, int vdisplay, float vrefresh, bool reduced, bool verbose)
{
    bool is_cvt = true;

    if ((!(vdisplay % 3) && ((vdisplay * 4 / 3) == hdisplay)) ||
        (!(vdisplay % 9) && ((vdisplay * 16 / 9) == hdisplay)) ||
        (!(vdisplay % 10) && ((vdisplay * 16 / 10) == hdisplay)) ||
        (!(vdisplay % 4) && ((vdisplay * 5 / 4) == hdisplay)) ||
        (!(vdisplay % 9) && ((vdisplay * 15 / 9) == hdisplay)));
    else {
        if (verbose)
            fprintf(stderr, "Warning: Aspect Ratio is not CVT standard.\n");
        is_cvt = false;
    }

    if ((vrefresh != 50.0) && (vrefresh != 60.0) &&
        (vrefresh != 75.0) && (vrefresh != 85.0)) {
        if (verbose)
            fprintf(stderr, "Warning: Refresh Rate %.2f is not CVT standard "
                    "(50, 60, 75 or 85Hz).\n", vrefresh);
        is_cvt = false;
    }

    return is_cvt;
}
/*
 * I'm not documenting --interlaced for obvious reasons, even though I did
 * implement it. I also can't deny having looked at gtf here.
 */
static void
print_usage(char *Name)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [-v|--verbose] [-r|--reduced] X Y [refresh]\n",
            Name);
    fprintf(stderr, "\n");
    fprintf(stderr, " -v|--verbose : Warn about CVT standard adherence.\n");
    fprintf(stderr, " -r|--reduced : Create a mode with reduced blanking "
            "(default: normal blanking).\n");
    fprintf(stderr, "            X : Desired horizontal resolution "
            "(multiple of 8, required).\n");
    fprintf(stderr,
            "            Y : Desired vertical resolution (required).\n");
    fprintf(stderr,
            "      refresh : Desired refresh rate (default: 60.0Hz).\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "Calculates VESA CVT (Coordinated Video Timing) modelines"
            " for use with X.\n");
}

/*
 *
 */
static void
print_comment(struct libxcvt_mode_info *mode_info, bool is_cvt, bool reduced)
{
    printf("# %dx%d %.2f Hz ", mode_info->hdisplay, mode_info->vdisplay, mode_info->vrefresh);

    if (is_cvt) {
        printf("(CVT %.2fM",
               ((float) mode_info->hdisplay * mode_info->vdisplay) / 1000000.0);

        if (!(mode_info->vdisplay % 3) &&
            ((mode_info->vdisplay * 4 / 3) == mode_info->hdisplay))
            printf("3");
        else if (!(mode_info->vdisplay % 9) &&
                 ((mode_info->vdisplay * 16 / 9) == mode_info->hdisplay))
            printf("9");
        else if (!(mode_info->vdisplay % 10) &&
                 ((mode_info->vdisplay * 16 / 10) == mode_info->hdisplay))
            printf("A");
        else if (!(mode_info->vdisplay % 4) &&
                 ((mode_info->vdisplay * 5 / 4) == mode_info->hdisplay))
            printf("4");
        else if (!(mode_info->vdisplay % 9) &&
                 ((mode_info->vdisplay * 15 / 9) == mode_info->hdisplay))
            printf("9");

        if (reduced)
            printf("-R");

        printf(") ");
    }
    else
        printf("(CVT) ");

    printf("hsync: %.2f kHz; ", mode_info->hsync);
    printf("pclk: %.2f MHz", ((float) mode_info->dot_clock) / 1000.0);

    printf("\n");
}

/*
 * Originally grabbed from xf86Mode.c.
 *
 * Ignoring the actual mode_info->name, as the user will want something solid
 * to grab hold of.
 */
static void
print_mode_line(struct libxcvt_mode_info *mode_info, int hdisplay, int vdisplay, float vrefresh,
              bool reduced)
{
    if (reduced)
        printf("Modeline \"%dx%dR\"  ", hdisplay, vdisplay);
    else
        printf("Modeline \"%dx%d_%.2f\"  ", hdisplay, vdisplay, vrefresh);

    printf("%6.2f  %i %i %i %i  %i %i %i %i", mode_info->dot_clock / 1000.,
           mode_info->hdisplay, mode_info->hsync_start, mode_info->hsync_end, mode_info->htotal,
           mode_info->vdisplay, mode_info->vsync_start, mode_info->vsync_end, mode_info->vtotal);

    if (mode_info->mode_flags & LIBXCVT_MODE_FLAG_INTERLACE)
        printf(" interlace");
    if (mode_info->mode_flags & LIBXCVT_MODE_FLAG_HSYNC_POSITIVE)
        printf(" +hsync");
    if (mode_info->mode_flags & LIBXCVT_MODE_FLAG_HSYNC_NEGATIVE)
        printf(" -hsync");
    if (mode_info->mode_flags & LIBXCVT_MODE_FLAG_VSYNC_POSITIVE)
        printf(" +vsync");
    if (mode_info->mode_flags & LIBXCVT_MODE_FLAG_VSYNC_NEGATIVE)
        printf(" -vsync");

    printf("\n");
}

/*
 *
 */
int
main(int argc, char *argv[])
{
    struct libxcvt_mode_info *mode_info;
    int hdisplay = 0, vdisplay = 0;
    float vrefresh = 0.0;
    bool reduced = false, verbose = false, is_cvt;
    bool interlaced = false;
    int n;

    if ((argc < 3) || (argc > 7)) {
        print_usage(argv[0]);
        return 1;
    }

    /* This doesn't filter out bad flags properly. Bad flags get passed down
     * to atoi/atof, which then return 0, so that these variables can get
     * filled next time round. So this is just a cosmetic problem.
     */
    for (n = 1; n < argc; n++) {
        if (!strcmp(argv[n], "-r") || !strcmp(argv[n], "--reduced"))
            reduced = true;
        else if (!strcmp(argv[n], "-i") || !strcmp(argv[n], "--interlaced"))
            interlaced = true;
        else if (!strcmp(argv[n], "-v") || !strcmp(argv[n], "--verbose"))
            verbose = true;
        else if (!strcmp(argv[n], "-h") || !strcmp(argv[n], "--help")) {
            print_usage(argv[0]);
            return 0;
        }
        else if (!hdisplay) {
            hdisplay = atoi(argv[n]);
            if (!hdisplay) {
                print_usage(argv[0]);
                return 1;
            }
        }
        else if (!vdisplay) {
            vdisplay = atoi(argv[n]);
            if (!vdisplay) {
                print_usage(argv[0]);
                return 1;
            }
        }
        else if (!vrefresh) {
            vrefresh = atof(argv[n]);
            if (!vrefresh) {
                print_usage(argv[0]);
                return 1;
            }
        }
        else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!hdisplay || !vdisplay) {
        print_usage(argv[0]);
        return 0;
    }

    /* Default to 60.0Hz */
    if (!vrefresh)
        vrefresh = 60.0;

    /* Horizontal timing is always a multiple of 8: round up. */
    if (hdisplay & 0x07) {
        hdisplay &= ~0x07;
        hdisplay += 8;
    }

    if (reduced) {
        if ((vrefresh / 60.0) != floor(vrefresh / 60.0)) {
            fprintf(stderr,
                    "\nERROR: Multiple of 60Hz refresh rate required for "
                    " reduced blanking.\n");
            print_usage(argv[0]);
            return 0;
        }
    }

    mode_info = libxcvt_gen_mode_info(hdisplay, vdisplay, vrefresh, reduced, interlaced);
    if (!mode_info) {
        fprintf(stderr, "Out of memory!\n");
            return 0;
    }

    is_cvt = cvt_is_standard(hdisplay, vdisplay, vrefresh, reduced, verbose);
    print_comment(mode_info, is_cvt, reduced);
    print_mode_line(mode_info, hdisplay, vdisplay, vrefresh, reduced);
    free(mode_info);

    return 0;
}
