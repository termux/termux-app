/* gtf.c  Generate mode timings using the GTF Timing Standard
 *
 * gcc gtf.c -o gtf -lm -Wall
 *
 * Copyright (c) 2001, Andy Ritger  aritger@nvidia.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * o Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * o Neither the name of NVIDIA nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * This program is based on the Generalized Timing Formula(GTF TM)
 * Standard Version: 1.0, Revision: 1.0
 *
 * The GTF Document contains the following Copyright information:
 *
 * Copyright (c) 1994, 1995, 1996 - Video Electronics Standards
 * Association. Duplication of this document within VESA member
 * companies for review purposes is permitted. All other rights
 * reserved.
 *
 * While every precaution has been taken in the preparation
 * of this standard, the Video Electronics Standards Association and
 * its contributors assume no responsibility for errors or omissions,
 * and make no warranties, expressed or implied, of functionality
 * of suitability for any purpose. The sample code contained within
 * this standard may be used without restriction.
 *
 *
 *
 * The GTF EXCEL(TM) SPREADSHEET, a sample (and the definitive)
 * implementation of the GTF Timing Standard, is available at:
 *
 * ftp://ftp.vesa.org/pub/GTF/GTF_V1R1.xls
 *
 *
 *
 * This program takes a desired resolution and vertical refresh rate,
 * and computes mode timings according to the GTF Timing Standard.
 * These mode timings can then be formatted as an XServer modeline
 * or a mode description for use by fbset(8).
 *
 *
 *
 * NOTES:
 *
 * The GTF allows for computation of "margins" (the visible border
 * surrounding the addressable video); on most non-overscan type
 * systems, the margin period is zero.  I've implemented the margin
 * computations but not enabled it because 1) I don't really have
 * any experience with this, and 2) neither XServer modelines nor
 * fbset fb.modes provide an obvious way for margin timings to be
 * included in their mode descriptions (needs more investigation).
 *
 * The GTF provides for computation of interlaced mode timings;
 * I've implemented the computations but not enabled them, yet.
 * I should probably enable and test this at some point.
 *
 *
 *
 * TODO:
 *
 * o Add support for interlaced modes.
 *
 * o Implement the other portions of the GTF: compute mode timings
 *   given either the desired pixel clock or the desired horizontal
 *   frequency.
 *
 * o It would be nice if this were more general purpose to do things
 *   outside the scope of the GTF: like generate double scan mode
 *   timings, for example.
 *
 * o Printing digits to the right of the decimal point when the
 *   digits are 0 annoys me.
 *
 * o Error checking.
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MARGIN_PERCENT    1.8   /* % of active vertical image                */
#define CELL_GRAN         8.0   /* assumed character cell granularity        */
#define MIN_PORCH         1     /* minimum front porch                       */
#define V_SYNC_RQD        3     /* width of vsync in lines                   */
#define H_SYNC_PERCENT    8.0   /* width of hsync as % of total line         */
#define MIN_VSYNC_PLUS_BP 550.0 /* min time of vsync + back porch (microsec) */
#define M                 600.0 /* blanking formula gradient                 */
#define C                 40.0  /* blanking formula offset                   */
#define K                 128.0 /* blanking formula scaling factor           */
#define J                 20.0  /* blanking formula scaling factor           */

/* C' and M' are part of the Blanking Duty Cycle computation */

#define C_PRIME           (((C - J) * K/256.0) + J)
#define M_PRIME           (K/256.0 * M)

/* struct definitions */

typedef struct __mode {
    int hr, hss, hse, hfl;
    int vr, vss, vse, vfl;
    float pclk, h_freq, v_freq;
} mode;

typedef struct __options {
    int x, y;
    int xorgmode, fbmode;
    float v_freq;
} options;

/* prototypes */

void print_value(int n, const char *name, float val);
void print_xf86_mode(mode * m);
void print_fb_mode(mode * m);
mode *vert_refresh(int h_pixels, int v_lines, float freq,
                   int interlaced, int margins);
options *parse_command_line(int argc, char *argv[]);

/*
 * print_value() - print the result of the named computation; this is
 * useful when comparing against the GTF EXCEL spreadsheet.
 */

int global_verbose = 0;

void
print_value(int n, const char *name, float val)
{
    if (global_verbose) {
        printf("%2d: %-27s: %15f\n", n, name, val);
    }
}

/* print_xf86_mode() - print the XServer modeline, given mode timings. */

void
print_xf86_mode(mode * m)
{
    printf("\n");
    printf("  # %dx%d @ %.2f Hz (GTF) hsync: %.2f kHz; pclk: %.2f MHz\n",
           m->hr, m->vr, m->v_freq, m->h_freq, m->pclk);

    printf("  Modeline \"%dx%d_%.2f\"  %.2f"
           "  %d %d %d %d"
           "  %d %d %d %d"
           "  -HSync +Vsync\n\n",
           m->hr, m->vr, m->v_freq, m->pclk,
           m->hr, m->hss, m->hse, m->hfl, m->vr, m->vss, m->vse, m->vfl);

}

/*
 * print_fb_mode() - print a mode description in fbset(8) format;
 * see the fb.modes(8) manpage.  The timing description used in
 * this is rather odd; they use "left and right margin" to refer
 * to the portion of the hblank before and after the sync pulse
 * by conceptually wrapping the portion of the blank after the pulse
 * to infront of the visible region; ie:
 *
 *
 * Timing description I'm accustomed to:
 *
 *
 *
 *     <--------1--------> <--2--> <--3--> <--4-->
 *                                _________
 *    |-------------------|_______|       |_______
 *
 *                        R       SS      SE     FL
 *
 * 1: visible image
 * 2: blank before sync (aka front porch)
 * 3: sync pulse
 * 4: blank after sync (aka back porch)
 * R: Resolution
 * SS: Sync Start
 * SE: Sync End
 * FL: Frame Length
 *
 *
 * But the fb.modes format is:
 *
 *
 *    <--4--> <--------1--------> <--2--> <--3-->
 *                                       _________
 *    _______|-------------------|_______|       |
 *
 * The fb.modes(8) manpage refers to <4> and <2> as the left and
 * right "margin" (as well as upper and lower margin in the vertical
 * direction) -- note that this has nothing to do with the term
 * "margin" used in the GTF Timing Standard.
 *
 * XXX always prints the 32 bit mode -- should I provide a command
 * line option to specify the bpp?  It's simple enough for a user
 * to edit the mode description after it's generated.
 */

void
print_fb_mode(mode * m)
{
    printf("\n");
    printf("mode \"%dx%d %.2fHz 32bit (GTF)\"\n", m->hr, m->vr, m->v_freq);
    printf("    # PCLK: %.2f MHz, H: %.2f kHz, V: %.2f Hz\n",
           m->pclk, m->h_freq, m->v_freq);
    printf("    geometry %d %d %d %d 32\n", m->hr, m->vr, m->hr, m->vr);
    printf("    timings %d %d %d %d %d %d %d\n", (int)lrint(1000000.0 / m->pclk),       /* pixclock in picoseconds */
           m->hfl - m->hse,     /* left margin (in pixels) */
           m->hss - m->hr,      /* right margin (in pixels) */
           m->vfl - m->vse,     /* upper margin (in pixel lines) */
           m->vss - m->vr,      /* lower margin (in pixel lines) */
           m->hse - m->hss,     /* horizontal sync length (pixels) */
           m->vse - m->vss);    /* vert sync length (pixel lines) */
    printf("    hsync low\n");
    printf("    vsync high\n");
    printf("endmode\n\n");

}

/*
 * vert_refresh() - as defined by the GTF Timing Standard, compute the
 * Stage 1 Parameters using the vertical refresh frequency.  In other
 * words: input a desired resolution and desired refresh rate, and
 * output the GTF mode timings.
 *
 * XXX All the code is in place to compute interlaced modes, but I don't
 * feel like testing it right now.
 *
 * XXX margin computations are implemented but not tested (nor used by
 * XServer of fbset mode descriptions, from what I can tell).
 */

mode *
vert_refresh(int h_pixels, int v_lines, float freq, int interlaced, int margins)
{
    float h_pixels_rnd;
    float v_lines_rnd;
    float v_field_rate_rqd;
    float top_margin;
    float bottom_margin;
    float interlace;
    float h_period_est;
    float vsync_plus_bp;
    float v_back_porch;
    float total_v_lines;
    float v_field_rate_est;
    float h_period;
    float v_field_rate;
    float v_frame_rate;
    float left_margin;
    float right_margin;
    float total_active_pixels;
    float ideal_duty_cycle;
    float h_blank;
    float total_pixels;
    float pixel_freq;
    float h_freq;

    float h_sync;
    float h_front_porch;
    float v_odd_front_porch_lines;

    mode *m = (mode *) malloc(sizeof(mode));

    /*  1. In order to give correct results, the number of horizontal
     *  pixels requested is first processed to ensure that it is divisible
     *  by the character size, by rounding it to the nearest character
     *  cell boundary:
     *
     *  [H PIXELS RND] = ((ROUND([H PIXELS]/[CELL GRAN RND],0))*[CELLGRAN RND])
     */

    h_pixels_rnd = rint((float) h_pixels / CELL_GRAN) * CELL_GRAN;

    print_value(1, "[H PIXELS RND]", h_pixels_rnd);

    /*  2. If interlace is requested, the number of vertical lines assumed
     *  by the calculation must be halved, as the computation calculates
     *  the number of vertical lines per field. In either case, the
     *  number of lines is rounded to the nearest integer.
     *
     *  [V LINES RND] = IF([INT RQD?]="y", ROUND([V LINES]/2,0),
     *                                     ROUND([V LINES],0))
     */

    v_lines_rnd = interlaced ?
        rint((float) v_lines) / 2.0 : rint((float) v_lines);

    print_value(2, "[V LINES RND]", v_lines_rnd);

    /*  3. Find the frame rate required:
     *
     *  [V FIELD RATE RQD] = IF([INT RQD?]="y", [I/P FREQ RQD]*2,
     *                                          [I/P FREQ RQD])
     */

    v_field_rate_rqd = interlaced ? (freq * 2.0) : (freq);

    print_value(3, "[V FIELD RATE RQD]", v_field_rate_rqd);

    /*  4. Find number of lines in Top margin:
     *
     *  [TOP MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
     *          ROUND(([MARGIN%]/100*[V LINES RND]),0),
     *          0)
     */

    top_margin = margins ? rint(MARGIN_PERCENT / 100.0 * v_lines_rnd) : (0.0);

    print_value(4, "[TOP MARGIN (LINES)]", top_margin);

    /*  5. Find number of lines in Bottom margin:
     *
     *  [BOT MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
     *          ROUND(([MARGIN%]/100*[V LINES RND]),0),
     *          0)
     */

    bottom_margin =
        margins ? rint(MARGIN_PERCENT / 100.0 * v_lines_rnd) : (0.0);

    print_value(5, "[BOT MARGIN (LINES)]", bottom_margin);

    /*  6. If interlace is required, then set variable [INTERLACE]=0.5:
     *
     *  [INTERLACE]=(IF([INT RQD?]="y",0.5,0))
     */

    interlace = interlaced ? 0.5 : 0.0;

    print_value(6, "[INTERLACE]", interlace);

    /*  7. Estimate the Horizontal period
     *
     *  [H PERIOD EST] = ((1/[V FIELD RATE RQD]) - [MIN VSYNC+BP]/1000000) /
     *                    ([V LINES RND] + (2*[TOP MARGIN (LINES)]) +
     *                     [MIN PORCH RND]+[INTERLACE]) * 1000000
     */

    h_period_est = (((1.0 / v_field_rate_rqd) - (MIN_VSYNC_PLUS_BP / 1000000.0))
                    / (v_lines_rnd + (2 * top_margin) + MIN_PORCH + interlace)
                    * 1000000.0);

    print_value(7, "[H PERIOD EST]", h_period_est);

    /*  8. Find the number of lines in V sync + back porch:
     *
     *  [V SYNC+BP] = ROUND(([MIN VSYNC+BP]/[H PERIOD EST]),0)
     */

    vsync_plus_bp = rint(MIN_VSYNC_PLUS_BP / h_period_est);

    print_value(8, "[V SYNC+BP]", vsync_plus_bp);

    /*  9. Find the number of lines in V back porch alone:
     *
     *  [V BACK PORCH] = [V SYNC+BP] - [V SYNC RND]
     *
     *  XXX is "[V SYNC RND]" a typo? should be [V SYNC RQD]?
     */

    v_back_porch = vsync_plus_bp - V_SYNC_RQD;

    print_value(9, "[V BACK PORCH]", v_back_porch);

    /*  10. Find the total number of lines in Vertical field period:
     *
     *  [TOTAL V LINES] = [V LINES RND] + [TOP MARGIN (LINES)] +
     *                    [BOT MARGIN (LINES)] + [V SYNC+BP] + [INTERLACE] +
     *                    [MIN PORCH RND]
     */

    total_v_lines = v_lines_rnd + top_margin + bottom_margin + vsync_plus_bp +
        interlace + MIN_PORCH;

    print_value(10, "[TOTAL V LINES]", total_v_lines);

    /*  11. Estimate the Vertical field frequency:
     *
     *  [V FIELD RATE EST] = 1 / [H PERIOD EST] / [TOTAL V LINES] * 1000000
     */

    v_field_rate_est = 1.0 / h_period_est / total_v_lines * 1000000.0;

    print_value(11, "[V FIELD RATE EST]", v_field_rate_est);

    /*  12. Find the actual horizontal period:
     *
     *  [H PERIOD] = [H PERIOD EST] / ([V FIELD RATE RQD] / [V FIELD RATE EST])
     */

    h_period = h_period_est / (v_field_rate_rqd / v_field_rate_est);

    print_value(12, "[H PERIOD]", h_period);

    /*  13. Find the actual Vertical field frequency:
     *
     *  [V FIELD RATE] = 1 / [H PERIOD] / [TOTAL V LINES] * 1000000
     */

    v_field_rate = 1.0 / h_period / total_v_lines * 1000000.0;

    print_value(13, "[V FIELD RATE]", v_field_rate);

    /*  14. Find the Vertical frame frequency:
     *
     *  [V FRAME RATE] = (IF([INT RQD?]="y", [V FIELD RATE]/2, [V FIELD RATE]))
     */

    v_frame_rate = interlaced ? v_field_rate / 2.0 : v_field_rate;

    print_value(14, "[V FRAME RATE]", v_frame_rate);

    /*  15. Find number of pixels in left margin:
     *
     *  [LEFT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
     *          (ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
     *                   [CELL GRAN RND]),0)) * [CELL GRAN RND],
     *          0))
     */

    left_margin = margins ?
        rint(h_pixels_rnd * MARGIN_PERCENT / 100.0 / CELL_GRAN) * CELL_GRAN :
        0.0;

    print_value(15, "[LEFT MARGIN (PIXELS)]", left_margin);

    /*  16. Find number of pixels in right margin:
     *
     *  [RIGHT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
     *          (ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
     *                   [CELL GRAN RND]),0)) * [CELL GRAN RND],
     *          0))
     */

    right_margin = margins ?
        rint(h_pixels_rnd * MARGIN_PERCENT / 100.0 / CELL_GRAN) * CELL_GRAN :
        0.0;

    print_value(16, "[RIGHT MARGIN (PIXELS)]", right_margin);

    /*  17. Find total number of active pixels in image and left and right
     *  margins:
     *
     *  [TOTAL ACTIVE PIXELS] = [H PIXELS RND] + [LEFT MARGIN (PIXELS)] +
     *                          [RIGHT MARGIN (PIXELS)]
     */

    total_active_pixels = h_pixels_rnd + left_margin + right_margin;

    print_value(17, "[TOTAL ACTIVE PIXELS]", total_active_pixels);

    /*  18. Find the ideal blanking duty cycle from the blanking duty cycle
     *  equation:
     *
     *  [IDEAL DUTY CYCLE] = [C'] - ([M']*[H PERIOD]/1000)
     */

    ideal_duty_cycle = C_PRIME - (M_PRIME * h_period / 1000.0);

    print_value(18, "[IDEAL DUTY CYCLE]", ideal_duty_cycle);

    /*  19. Find the number of pixels in the blanking time to the nearest
     *  double character cell:
     *
     *  [H BLANK (PIXELS)] = (ROUND(([TOTAL ACTIVE PIXELS] *
     *                               [IDEAL DUTY CYCLE] /
     *                               (100-[IDEAL DUTY CYCLE]) /
     *                               (2*[CELL GRAN RND])), 0))
     *                       * (2*[CELL GRAN RND])
     */

    h_blank = rint(total_active_pixels *
                   ideal_duty_cycle /
                   (100.0 - ideal_duty_cycle) /
                   (2.0 * CELL_GRAN)) * (2.0 * CELL_GRAN);

    print_value(19, "[H BLANK (PIXELS)]", h_blank);

    /*  20. Find total number of pixels:
     *
     *  [TOTAL PIXELS] = [TOTAL ACTIVE PIXELS] + [H BLANK (PIXELS)]
     */

    total_pixels = total_active_pixels + h_blank;

    print_value(20, "[TOTAL PIXELS]", total_pixels);

    /*  21. Find pixel clock frequency:
     *
     *  [PIXEL FREQ] = [TOTAL PIXELS] / [H PERIOD]
     */

    pixel_freq = total_pixels / h_period;

    print_value(21, "[PIXEL FREQ]", pixel_freq);

    /*  22. Find horizontal frequency:
     *
     *  [H FREQ] = 1000 / [H PERIOD]
     */

    h_freq = 1000.0 / h_period;

    print_value(22, "[H FREQ]", h_freq);

    /* Stage 1 computations are now complete; I should really pass
       the results to another function and do the Stage 2
       computations, but I only need a few more values so I'll just
       append the computations here for now */

    /*  17. Find the number of pixels in the horizontal sync period:
     *
     *  [H SYNC (PIXELS)] =(ROUND(([H SYNC%] / 100 * [TOTAL PIXELS] /
     *                             [CELL GRAN RND]),0))*[CELL GRAN RND]
     */

    h_sync =
        rint(H_SYNC_PERCENT / 100.0 * total_pixels / CELL_GRAN) * CELL_GRAN;

    print_value(17, "[H SYNC (PIXELS)]", h_sync);

    /*  18. Find the number of pixels in the horizontal front porch period:
     *
     *  [H FRONT PORCH (PIXELS)] = ([H BLANK (PIXELS)]/2)-[H SYNC (PIXELS)]
     */

    h_front_porch = (h_blank / 2.0) - h_sync;

    print_value(18, "[H FRONT PORCH (PIXELS)]", h_front_porch);

    /*  36. Find the number of lines in the odd front porch period:
     *
     *  [V ODD FRONT PORCH(LINES)]=([MIN PORCH RND]+[INTERLACE])
     */

    v_odd_front_porch_lines = MIN_PORCH + interlace;

    print_value(36, "[V ODD FRONT PORCH(LINES)]", v_odd_front_porch_lines);

    /* finally, pack the results in the mode struct */

    m->hr = (int) (h_pixels_rnd);
    m->hss = (int) (h_pixels_rnd + h_front_porch);
    m->hse = (int) (h_pixels_rnd + h_front_porch + h_sync);
    m->hfl = (int) (total_pixels);

    m->vr = (int) (v_lines_rnd);
    m->vss = (int) (v_lines_rnd + v_odd_front_porch_lines);
    m->vse = (int) (int) (v_lines_rnd + v_odd_front_porch_lines + V_SYNC_RQD);
    m->vfl = (int) (total_v_lines);

    m->pclk = pixel_freq;
    m->h_freq = h_freq;
    m->v_freq = freq;

    return m;

}

/*
 * parse_command_line() - parse the command line and return an
 * alloced structure containing the results.  On error print usage
 * and return NULL.
 */

options *
parse_command_line(int argc, char *argv[])
{
    int n;

    options *o = (options *) calloc(1, sizeof(options));

    if (argc < 4)
        goto bad_option;

    o->x = atoi(argv[1]);
    o->y = atoi(argv[2]);
    o->v_freq = atof(argv[3]);

    /* XXX should check for errors in the above */

    n = 4;

    while (n < argc) {
        if ((strcmp(argv[n], "-v") == 0) || (strcmp(argv[n], "--verbose") == 0)) {
            global_verbose = 1;
        }
        else if ((strcmp(argv[n], "-f") == 0) ||
                 (strcmp(argv[n], "--fbmode") == 0)) {
            o->fbmode = 1;
        }
        else if ((strcmp(argv[n], "-x") == 0) ||
                 (strcmp(argv[n], "--xorgmode") == 0) ||
                 (strcmp(argv[n], "--xf86mode") == 0)) {
            o->xorgmode = 1;
        }
        else {
            goto bad_option;
        }

        n++;
    }

    /* if neither xorgmode nor fbmode were requested, default to
       xorgmode */

    if (!o->fbmode && !o->xorgmode)
        o->xorgmode = 1;

    return o;

 bad_option:

    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s x y refresh [-v|--verbose] "
            "[-f|--fbmode] [-x|--xorgmode]\n", argv[0]);

    fprintf(stderr, "\n");

    fprintf(stderr, "            x : the desired horizontal "
            "resolution (required)\n");
    fprintf(stderr, "            y : the desired vertical "
            "resolution (required)\n");
    fprintf(stderr, "      refresh : the desired refresh " "rate (required)\n");
    fprintf(stderr, " -v|--verbose : enable verbose printouts "
            "(traces each step of the computation)\n");
    fprintf(stderr, "  -f|--fbmode : output an fbset(8)-style mode "
            "description\n");
    fprintf(stderr, " -x|--xorgmode : output an " __XSERVERNAME__ "-style mode "
            "description (this is the default\n"
            "                if no mode description is requested)\n");

    fprintf(stderr, "\n");

    free(o);
    return NULL;

}

int
main(int argc, char *argv[])
{
    mode *m;
    options *o;

    o = parse_command_line(argc, argv);
    if (!o)
        exit(1);

    m = vert_refresh(o->x, o->y, o->v_freq, 0, 0);
    if (!m)
        exit(1);

    if (o->xorgmode)
        print_xf86_mode(m);

    if (o->fbmode)
        print_fb_mode(m);

    free(m);

    return 0;

}
