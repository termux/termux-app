/*
 * gtf.c  Generate mode timings using the GTF Timing Standard
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
 */

/* Ruthlessly converted to server code by Adam Jackson <ajax@redhat.com> */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86Modes.h"
#include <string.h>

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

/*
 * xf86GTFMode() - as defined by the GTF Timing Standard, compute the
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

DisplayModePtr
xf86GTFMode(int h_pixels, int v_lines, float freq, int interlaced, int margins)
{
    DisplayModeRec *mode = xnfcalloc(1, sizeof(DisplayModeRec));

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

    /*  1. In order to give correct results, the number of horizontal
     *  pixels requested is first processed to ensure that it is divisible
     *  by the character size, by rounding it to the nearest character
     *  cell boundary:
     *
     *  [H PIXELS RND] = ((ROUND([H PIXELS]/[CELL GRAN RND],0))*[CELLGRAN RND])
     */

    h_pixels_rnd = rint((float) h_pixels / CELL_GRAN) * CELL_GRAN;

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

    /*  3. Find the frame rate required:
     *
     *  [V FIELD RATE RQD] = IF([INT RQD?]="y", [I/P FREQ RQD]*2,
     *                                          [I/P FREQ RQD])
     */

    v_field_rate_rqd = interlaced ? (freq * 2.0) : (freq);

    /*  4. Find number of lines in Top margin:
     *
     *  [TOP MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
     *          ROUND(([MARGIN%]/100*[V LINES RND]),0),
     *          0)
     */

    top_margin = margins ? rint(MARGIN_PERCENT / 100.0 * v_lines_rnd) : (0.0);

    /*  5. Find number of lines in Bottom margin:
     *
     *  [BOT MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
     *          ROUND(([MARGIN%]/100*[V LINES RND]),0),
     *          0)
     */

    bottom_margin =
        margins ? rint(MARGIN_PERCENT / 100.0 * v_lines_rnd) : (0.0);

    /*  6. If interlace is required, then set variable [INTERLACE]=0.5:
     *
     *  [INTERLACE]=(IF([INT RQD?]="y",0.5,0))
     */

    interlace = interlaced ? 0.5 : 0.0;

    /*  7. Estimate the Horizontal period
     *
     *  [H PERIOD EST] = ((1/[V FIELD RATE RQD]) - [MIN VSYNC+BP]/1000000) /
     *                    ([V LINES RND] + (2*[TOP MARGIN (LINES)]) +
     *                     [MIN PORCH RND]+[INTERLACE]) * 1000000
     */

    h_period_est = (((1.0 / v_field_rate_rqd) - (MIN_VSYNC_PLUS_BP / 1000000.0))
                    / (v_lines_rnd + (2 * top_margin) + MIN_PORCH + interlace)
                    * 1000000.0);

    /*  8. Find the number of lines in V sync + back porch:
     *
     *  [V SYNC+BP] = ROUND(([MIN VSYNC+BP]/[H PERIOD EST]),0)
     */

    vsync_plus_bp = rint(MIN_VSYNC_PLUS_BP / h_period_est);

    /*  9. Find the number of lines in V back porch alone:
     *
     *  [V BACK PORCH] = [V SYNC+BP] - [V SYNC RND]
     *
     *  XXX is "[V SYNC RND]" a typo? should be [V SYNC RQD]?
     */

    v_back_porch = vsync_plus_bp - V_SYNC_RQD;
    (void) v_back_porch;

    /*  10. Find the total number of lines in Vertical field period:
     *
     *  [TOTAL V LINES] = [V LINES RND] + [TOP MARGIN (LINES)] +
     *                    [BOT MARGIN (LINES)] + [V SYNC+BP] + [INTERLACE] +
     *                    [MIN PORCH RND]
     */

    total_v_lines = v_lines_rnd + top_margin + bottom_margin + vsync_plus_bp +
        interlace + MIN_PORCH;

    /*  11. Estimate the Vertical field frequency:
     *
     *  [V FIELD RATE EST] = 1 / [H PERIOD EST] / [TOTAL V LINES] * 1000000
     */

    v_field_rate_est = 1.0 / h_period_est / total_v_lines * 1000000.0;

    /*  12. Find the actual horizontal period:
     *
     *  [H PERIOD] = [H PERIOD EST] / ([V FIELD RATE RQD] / [V FIELD RATE EST])
     */

    h_period = h_period_est / (v_field_rate_rqd / v_field_rate_est);

    /*  13. Find the actual Vertical field frequency:
     *
     *  [V FIELD RATE] = 1 / [H PERIOD] / [TOTAL V LINES] * 1000000
     */

    v_field_rate = 1.0 / h_period / total_v_lines * 1000000.0;

    /*  14. Find the Vertical frame frequency:
     *
     *  [V FRAME RATE] = (IF([INT RQD?]="y", [V FIELD RATE]/2, [V FIELD RATE]))
     */

    v_frame_rate = interlaced ? v_field_rate / 2.0 : v_field_rate;
    (void) v_frame_rate;

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

    /*  17. Find total number of active pixels in image and left and right
     *  margins:
     *
     *  [TOTAL ACTIVE PIXELS] = [H PIXELS RND] + [LEFT MARGIN (PIXELS)] +
     *                          [RIGHT MARGIN (PIXELS)]
     */

    total_active_pixels = h_pixels_rnd + left_margin + right_margin;

    /*  18. Find the ideal blanking duty cycle from the blanking duty cycle
     *  equation:
     *
     *  [IDEAL DUTY CYCLE] = [C'] - ([M']*[H PERIOD]/1000)
     */

    ideal_duty_cycle = C_PRIME - (M_PRIME * h_period / 1000.0);

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

    /*  20. Find total number of pixels:
     *
     *  [TOTAL PIXELS] = [TOTAL ACTIVE PIXELS] + [H BLANK (PIXELS)]
     */

    total_pixels = total_active_pixels + h_blank;

    /*  21. Find pixel clock frequency:
     *
     *  [PIXEL FREQ] = [TOTAL PIXELS] / [H PERIOD]
     */

    pixel_freq = total_pixels / h_period;

    /*  22. Find horizontal frequency:
     *
     *  [H FREQ] = 1000 / [H PERIOD]
     */

    h_freq = 1000.0 / h_period;

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

    /*  18. Find the number of pixels in the horizontal front porch period:
     *
     *  [H FRONT PORCH (PIXELS)] = ([H BLANK (PIXELS)]/2)-[H SYNC (PIXELS)]
     */

    h_front_porch = (h_blank / 2.0) - h_sync;

    /*  36. Find the number of lines in the odd front porch period:
     *
     *  [V ODD FRONT PORCH(LINES)]=([MIN PORCH RND]+[INTERLACE])
     */

    v_odd_front_porch_lines = MIN_PORCH + interlace;

    /* finally, pack the results in the mode struct */

    mode->HDisplay = (int) (h_pixels_rnd);
    mode->HSyncStart = (int) (h_pixels_rnd + h_front_porch);
    mode->HSyncEnd = (int) (h_pixels_rnd + h_front_porch + h_sync);
    mode->HTotal = (int) (total_pixels);
    mode->VDisplay = (int) (v_lines_rnd);
    mode->VSyncStart = (int) (v_lines_rnd + v_odd_front_porch_lines);
    mode->VSyncEnd = (int) (v_lines_rnd + v_odd_front_porch_lines + V_SYNC_RQD);
    mode->VTotal = (int) (total_v_lines);

    mode->Clock = (int) (pixel_freq * 1000.0);
    mode->HSync = h_freq;
    mode->VRefresh = freq;

    xf86SetModeDefaultName(mode);

    mode->Flags = V_NHSYNC | V_PVSYNC;
    if (interlaced) {
        mode->VTotal *= 2;
        mode->Flags |= V_INTERLACE;
    }

    return mode;
}
