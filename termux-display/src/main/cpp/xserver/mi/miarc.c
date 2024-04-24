/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* Author: Keith Packard and Bob Scheifler */
/* Warning: this code is toxic, do not dally very long here. */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <math.h>
#include <X11/X.h>
#include <X11/Xprotostr.h>
#include "misc.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "mifpoly.h"
#include "mi.h"
#include "mifillarc.h"
#include <X11/Xfuncproto.h>

#define EPSILON	0.000001
#define ISEQUAL(a,b) (fabs((a) - (b)) <= EPSILON)
#define UNEQUAL(a,b) (fabs((a) - (b)) > EPSILON)
#define PTISEQUAL(a,b) (ISEQUAL(a.x,b.x) && ISEQUAL(a.y,b.y))
#define SQSECANT 108.856472512142   /* 1/sin^2(11/2) - for 11o miter cutoff */

/* Point with sub-pixel positioning. */
typedef struct _SppPoint {
    double x, y;
} SppPointRec, *SppPointPtr;

typedef struct _SppArc {
    double x, y, width, height;
    double angle1, angle2;
} SppArcRec, *SppArcPtr;

static double miDsin(double a);
static double miDcos(double a);
static double miDasin(double v);
static double miDatan2(double dy, double dx);

#ifndef HAVE_CBRT
static double
cbrt(double x)
{
    if (x > 0.0)
        return pow(x, 1.0 / 3.0);
    else
        return -pow(-x, 1.0 / 3.0);
}
#endif

/*
 * some interesting semantic interpretation of the protocol:
 *
 * Self intersecting arcs (i.e. those spanning 360 degrees)
 *  never join with other arcs, and are drawn without caps
 *  (unless on/off dashed, in which case each dash segment
 *  is capped, except when the last segment meets the
 *  first segment, when no caps are drawn)
 *
 * double dash arcs are drawn in two parts, first the
 *  odd dashes (drawn in background) then the even dashes
 *  (drawn in foreground).  This means that overlapping
 *  sections of foreground/background are drawn twice,
 *  first in background then in foreground.  The double-draw
 *  occurs even when the function uses the destination values
 *  (e.g. xor mode).  This is the same way the wide-line
 *  code works and should be "fixed".
 *
 */

struct bound {
    double min, max;
};

struct ibound {
    int min, max;
};

#define boundedLe(value, bounds)\
	((bounds).min <= (value) && (value) <= (bounds).max)

struct line {
    double m, b;
    int valid;
};

#define intersectLine(y,line) (line.m * (y) + line.b)

/*
 * these are all y value bounds
 */

struct arc_bound {
    struct bound ellipse;
    struct bound inner;
    struct bound outer;
    struct bound right;
    struct bound left;
    struct ibound inneri;
    struct ibound outeri;
};

struct accelerators {
    double tail_y;
    double h2;
    double w2;
    double h4;
    double w4;
    double h2mw2;
    double h2l;
    double w2l;
    double fromIntX;
    double fromIntY;
    struct line left, right;
    int yorgu;
    int yorgl;
    int xorg;
};

struct arc_def {
    double w, h, l;
    double a0, a1;
};

#define todeg(xAngle)	(((double) (xAngle)) / 64.0)

#define RIGHT_END	0
#define LEFT_END	1

typedef struct _miArcJoin {
    int arcIndex0, arcIndex1;
    int phase0, phase1;
    int end0, end1;
} miArcJoinRec, *miArcJoinPtr;

typedef struct _miArcCap {
    int arcIndex;
    int end;
} miArcCapRec, *miArcCapPtr;

typedef struct _miArcFace {
    SppPointRec clock;
    SppPointRec center;
    SppPointRec counterClock;
} miArcFaceRec, *miArcFacePtr;

typedef struct _miArcData {
    xArc arc;
    int render;                 /* non-zero means render after drawing */
    int join;                   /* related join */
    int cap;                    /* related cap */
    int selfJoin;               /* final dash meets first dash */
    miArcFaceRec bounds[2];
    double x0, y0, x1, y1;
} miArcDataRec, *miArcDataPtr;

/*
 * This is an entire sequence of arcs, computed and categorized according
 * to operation.  miDashArcs generates either one or two of these.
 */

typedef struct _miPolyArc {
    int narcs;
    miArcDataPtr arcs;
    int ncaps;
    miArcCapPtr caps;
    int njoins;
    miArcJoinPtr joins;
} miPolyArcRec, *miPolyArcPtr;

typedef struct {
    short lx, lw, rx, rw;
} miArcSpan;

typedef struct {
    miArcSpan *spans;
    int count1, count2, k;
    char top, bot, hole;
} miArcSpanData;

static void fillSpans(DrawablePtr pDrawable, GCPtr pGC);
static void newFinalSpan(int y, int xmin, int xmax);
static miArcSpanData *drawArc(xArc * tarc, int l, int a0, int a1,
                              miArcFacePtr right, miArcFacePtr left,
                              miArcSpanData *spdata);
static void drawZeroArc(DrawablePtr pDraw, GCPtr pGC, xArc * tarc, int lw,
                        miArcFacePtr left, miArcFacePtr right);
static void miArcJoin(DrawablePtr pDraw, GCPtr pGC, miArcFacePtr pLeft,
                      miArcFacePtr pRight, int xOrgLeft, int yOrgLeft,
                      double xFtransLeft, double yFtransLeft,
                      int xOrgRight, int yOrgRight,
                      double xFtransRight, double yFtransRight);
static void miArcCap(DrawablePtr pDraw, GCPtr pGC, miArcFacePtr pFace,
                     int end, int xOrg, int yOrg, double xFtrans,
                     double yFtrans);
static void miRoundCap(DrawablePtr pDraw, GCPtr pGC, SppPointRec pCenter,
                       SppPointRec pEnd, SppPointRec pCorner,
                       SppPointRec pOtherCorner, int fLineEnd,
                       int xOrg, int yOrg, double xFtrans, double yFtrans);
static void miFreeArcs(miPolyArcPtr arcs, GCPtr pGC);
static miPolyArcPtr miComputeArcs(xArc * parcs, int narcs, GCPtr pGC);
static int miGetArcPts(SppArcPtr parc, int cpt, SppPointPtr * ppPts);

#define CUBED_ROOT_2	1.2599210498948732038115849718451499938964
#define CUBED_ROOT_4	1.5874010519681993173435330390930175781250

/*
 * draw one segment of the arc using the arc spans generation routines
 */

static miArcSpanData *
miArcSegment(DrawablePtr pDraw, GCPtr pGC, xArc tarc, miArcFacePtr right,
             miArcFacePtr left, miArcSpanData *spdata)
{
    int l = pGC->lineWidth;
    int a0, a1, startAngle, endAngle;
    miArcFacePtr temp;

    if (!l)
        l = 1;

    if (tarc.width == 0 || tarc.height == 0) {
        drawZeroArc(pDraw, pGC, &tarc, l, left, right);
        return spdata;
    }

    if (pGC->miTranslate) {
        tarc.x += pDraw->x;
        tarc.y += pDraw->y;
    }

    a0 = tarc.angle1;
    a1 = tarc.angle2;
    if (a1 > FULLCIRCLE)
        a1 = FULLCIRCLE;
    else if (a1 < -FULLCIRCLE)
        a1 = -FULLCIRCLE;
    if (a1 < 0) {
        startAngle = a0 + a1;
        endAngle = a0;
        temp = right;
        right = left;
        left = temp;
    }
    else {
        startAngle = a0;
        endAngle = a0 + a1;
    }
    /*
     * bounds check the two angles
     */
    if (startAngle < 0)
        startAngle = FULLCIRCLE - (-startAngle) % FULLCIRCLE;
    if (startAngle >= FULLCIRCLE)
        startAngle = startAngle % FULLCIRCLE;
    if (endAngle < 0)
        endAngle = FULLCIRCLE - (-endAngle) % FULLCIRCLE;
    if (endAngle > FULLCIRCLE)
        endAngle = (endAngle - 1) % FULLCIRCLE + 1;
    if ((startAngle == endAngle) && a1) {
        startAngle = 0;
        endAngle = FULLCIRCLE;
    }

    return drawArc(&tarc, l, startAngle, endAngle, right, left, spdata);
}

/*

Three equations combine to describe the boundaries of the arc

x^2/w^2 + y^2/h^2 = 1			ellipse itself
(X-x)^2 + (Y-y)^2 = r^2			circle at (x, y) on the ellipse
(Y-y) = (X-x)*w^2*y/(h^2*x)		normal at (x, y) on the ellipse

These lead to a quartic relating Y and y

y^4 - (2Y)y^3 + (Y^2 + (h^4 - w^2*r^2)/(w^2 - h^2))y^2
    - (2Y*h^4/(w^2 - h^2))y + (Y^2*h^4)/(w^2 - h^2) = 0

The reducible cubic obtained from this quartic is

z^3 - (3N)z^2 - 2V = 0

where

N = (Y^2 + (h^4 - w^2*r^2/(w^2 - h^2)))/6
V = w^2*r^2*Y^2*h^4/(4 *(w^2 - h^2)^2)

Let

t = z - N
p = -N^2
q = -N^3 - V

Then we get

t^3 + 3pt + 2q = 0

The discriminant of this cubic is

D = q^2 + p^3

When D > 0, a real root is obtained as

z = N + cbrt(-q+sqrt(D)) + cbrt(-q-sqrt(D))

When D < 0, a real root is obtained as

z = N - 2m*cos(acos(-q/m^3)/3)

where

m = sqrt(|p|) * sign(q)

Given a real root Z of the cubic, the roots of the quartic are the roots
of the two quadratics

y^2 + ((b+A)/2)y + (Z + (bZ - d)/A) = 0

where

A = +/- sqrt(8Z + b^2 - 4c)
b, c, d are the cubic, quadratic, and linear coefficients of the quartic

Some experimentation is then required to determine which solutions
correspond to the inner and outer boundaries.

*/

static void drawQuadrant(struct arc_def *def, struct accelerators *acc,
                         int a0, int a1, int mask, miArcFacePtr right,
                         miArcFacePtr left, miArcSpanData * spdata);

static void
miComputeCircleSpans(int lw, xArc * parc, miArcSpanData * spdata)
{
    miArcSpan *span;
    int doinner;
    int x, y, e;
    int xk, yk, xm, ym, dx, dy;
    int slw, inslw;
    int inx = 0, iny, ine = 0;
    int inxk = 0, inyk = 0, inxm = 0, inym = 0;

    doinner = -lw;
    slw = parc->width - doinner;
    y = parc->height >> 1;
    dy = parc->height & 1;
    dx = 1 - dy;
    MIWIDEARCSETUP(x, y, dy, slw, e, xk, xm, yk, ym);
    inslw = parc->width + doinner;
    if (inslw > 0) {
        spdata->hole = spdata->top;
        MIWIDEARCSETUP(inx, iny, dy, inslw, ine, inxk, inxm, inyk, inym);
    }
    else {
        spdata->hole = FALSE;
        doinner = -y;
    }
    spdata->count1 = -doinner - spdata->top;
    spdata->count2 = y + doinner;
    span = spdata->spans;
    while (y) {
        MIFILLARCSTEP(slw);
        span->lx = dy - x;
        if (++doinner <= 0) {
            span->lw = slw;
            span->rx = 0;
            span->rw = span->lx + slw;
        }
        else {
            MIFILLINARCSTEP(inslw);
            span->lw = x - inx;
            span->rx = dy - inx + inslw;
            span->rw = inx - x + slw - inslw;
        }
        span++;
    }
    if (spdata->bot) {
        if (spdata->count2)
            spdata->count2--;
        else {
            if (lw > (int) parc->height)
                span[-1].rx = span[-1].rw = -((lw - (int) parc->height) >> 1);
            else
                span[-1].rw = 0;
            spdata->count1--;
        }
    }
}

static void
miComputeEllipseSpans(int lw, xArc * parc, miArcSpanData * spdata)
{
    miArcSpan *span;
    double w, h, r, xorg;
    double Hs, Hf, WH, K, Vk, Nk, Fk, Vr, N, Nc, Z, rs;
    double A, T, b, d, x, y, t, inx, outx = 0.0, hepp, hepm;
    int flip, solution;

    w = (double) parc->width / 2.0;
    h = (double) parc->height / 2.0;
    r = lw / 2.0;
    rs = r * r;
    Hs = h * h;
    WH = w * w - Hs;
    Nk = w * r;
    Vk = (Nk * Hs) / (WH + WH);
    Hf = Hs * Hs;
    Nk = (Hf - Nk * Nk) / WH;
    Fk = Hf / WH;
    hepp = h + EPSILON;
    hepm = h - EPSILON;
    K = h + ((lw - 1) >> 1);
    span = spdata->spans;
    if (parc->width & 1)
        xorg = .5;
    else
        xorg = 0.0;
    if (spdata->top) {
        span->lx = 0;
        span->lw = 1;
        span++;
    }
    spdata->count1 = 0;
    spdata->count2 = 0;
    spdata->hole = (spdata->top &&
                    (int) parc->height * lw <= (int) (parc->width * parc->width)
                    && lw < (int) parc->height);
    for (; K > 0.0; K -= 1.0) {
        N = (K * K + Nk) / 6.0;
        Nc = N * N * N;
        Vr = Vk * K;
        t = Nc + Vr * Vr;
        d = Nc + t;
        if (d < 0.0) {
            d = Nc;
            b = N;
            if ((b < 0.0) == (t < 0.0)) {
                b = -b;
                d = -d;
            }
            Z = N - 2.0 * b * cos(acos(-t / d) / 3.0);
            if ((Z < 0.0) == (Vr < 0.0))
                flip = 2;
            else
                flip = 1;
        }
        else {
            d = Vr * sqrt(d);
            Z = N + cbrt(t + d) + cbrt(t - d);
            flip = 0;
        }
        A = sqrt((Z + Z) - Nk);
        T = (Fk - Z) * K / A;
        inx = 0.0;
        solution = FALSE;
        b = -A + K;
        d = b * b - 4 * (Z + T);
        if (d >= 0) {
            d = sqrt(d);
            y = (b + d) / 2;
            if ((y >= 0.0) && (y < hepp)) {
                solution = TRUE;
                if (y > hepm)
                    y = h;
                t = y / h;
                x = w * sqrt(1 - (t * t));
                t = K - y;
                if (rs - (t * t) >= 0)
                    t = sqrt(rs - (t * t));
                else
                    t = 0;
                if (flip == 2)
                    inx = x - t;
                else
                    outx = x + t;
            }
        }
        b = A + K;
        d = b * b - 4 * (Z - T);
        /* Because of the large magnitudes involved, we lose enough precision
         * that sometimes we end up with a negative value near the axis, when
         * it should be positive.  This is a workaround.
         */
        if (d < 0 && !solution)
            d = 0.0;
        if (d >= 0) {
            d = sqrt(d);
            y = (b + d) / 2;
            if (y < hepp) {
                if (y > hepm)
                    y = h;
                t = y / h;
                x = w * sqrt(1 - (t * t));
                t = K - y;
                if (rs - (t * t) >= 0)
                    inx = x - sqrt(rs - (t * t));
                else
                    inx = x;
            }
            y = (b - d) / 2;
            if (y >= 0.0) {
                if (y > hepm)
                    y = h;
                t = y / h;
                x = w * sqrt(1 - (t * t));
                t = K - y;
                if (rs - (t * t) >= 0)
                    t = sqrt(rs - (t * t));
                else
                    t = 0;
                if (flip == 1)
                    inx = x - t;
                else
                    outx = x + t;
            }
        }
        span->lx = ICEIL(xorg - outx);
        if (inx <= 0.0) {
            spdata->count1++;
            span->lw = ICEIL(xorg + outx) - span->lx;
            span->rx = ICEIL(xorg + inx);
            span->rw = -ICEIL(xorg - inx);
        }
        else {
            spdata->count2++;
            span->lw = ICEIL(xorg - inx) - span->lx;
            span->rx = ICEIL(xorg + inx);
            span->rw = ICEIL(xorg + outx) - span->rx;
        }
        span++;
    }
    if (spdata->bot) {
        outx = w + r;
        if (r >= h && r <= w)
            inx = 0.0;
        else if (Nk < 0.0 && -Nk < Hs) {
            inx = w * sqrt(1 + Nk / Hs) - sqrt(rs + Nk);
            if (inx > w - r)
                inx = w - r;
        }
        else
            inx = w - r;
        span->lx = ICEIL(xorg - outx);
        if (inx <= 0.0) {
            span->lw = ICEIL(xorg + outx) - span->lx;
            span->rx = ICEIL(xorg + inx);
            span->rw = -ICEIL(xorg - inx);
        }
        else {
            span->lw = ICEIL(xorg - inx) - span->lx;
            span->rx = ICEIL(xorg + inx);
            span->rw = ICEIL(xorg + outx) - span->rx;
        }
    }
    if (spdata->hole) {
        span = &spdata->spans[spdata->count1];
        span->lw = -span->lx;
        span->rx = 1;
        span->rw = span->lw;
        spdata->count1--;
        spdata->count2++;
    }
}

static double
tailX(double K,
      struct arc_def *def, struct arc_bound *bounds, struct accelerators *acc)
{
    double w, h, r;
    double Hs, Hf, WH, Vk, Nk, Fk, Vr, N, Nc, Z, rs;
    double A, T, b, d, x, y, t, hepp, hepm;
    int flip, solution;
    double xs[2];
    double *xp;

    w = def->w;
    h = def->h;
    r = def->l;
    rs = r * r;
    Hs = acc->h2;
    WH = -acc->h2mw2;
    Nk = def->w * r;
    Vk = (Nk * Hs) / (WH + WH);
    Hf = acc->h4;
    Nk = (Hf - Nk * Nk) / WH;
    if (K == 0.0) {
        if (Nk < 0.0 && -Nk < Hs) {
            xs[0] = w * sqrt(1 + Nk / Hs) - sqrt(rs + Nk);
            xs[1] = w - r;
            if (acc->left.valid && boundedLe(K, bounds->left) &&
                !boundedLe(K, bounds->outer) && xs[0] >= 0.0 && xs[1] >= 0.0)
                return xs[1];
            if (acc->right.valid && boundedLe(K, bounds->right) &&
                !boundedLe(K, bounds->inner) && xs[0] <= 0.0 && xs[1] <= 0.0)
                return xs[1];
            return xs[0];
        }
        return w - r;
    }
    Fk = Hf / WH;
    hepp = h + EPSILON;
    hepm = h - EPSILON;
    N = (K * K + Nk) / 6.0;
    Nc = N * N * N;
    Vr = Vk * K;
    xp = xs;
    xs[0] = 0.0;
    t = Nc + Vr * Vr;
    d = Nc + t;
    if (d < 0.0) {
        d = Nc;
        b = N;
        if ((b < 0.0) == (t < 0.0)) {
            b = -b;
            d = -d;
        }
        Z = N - 2.0 * b * cos(acos(-t / d) / 3.0);
        if ((Z < 0.0) == (Vr < 0.0))
            flip = 2;
        else
            flip = 1;
    }
    else {
        d = Vr * sqrt(d);
        Z = N + cbrt(t + d) + cbrt(t - d);
        flip = 0;
    }
    A = sqrt((Z + Z) - Nk);
    T = (Fk - Z) * K / A;
    solution = FALSE;
    b = -A + K;
    d = b * b - 4 * (Z + T);
    if (d >= 0 && flip == 2) {
        d = sqrt(d);
        y = (b + d) / 2;
        if ((y >= 0.0) && (y < hepp)) {
            solution = TRUE;
            if (y > hepm)
                y = h;
            t = y / h;
            x = w * sqrt(1 - (t * t));
            t = K - y;
            if (rs - (t * t) >= 0)
                t = sqrt(rs - (t * t));
            else
                t = 0;
            *xp++ = x - t;
        }
    }
    b = A + K;
    d = b * b - 4 * (Z - T);
    /* Because of the large magnitudes involved, we lose enough precision
     * that sometimes we end up with a negative value near the axis, when
     * it should be positive.  This is a workaround.
     */
    if (d < 0 && !solution)
        d = 0.0;
    if (d >= 0) {
        d = sqrt(d);
        y = (b + d) / 2;
        if (y < hepp) {
            if (y > hepm)
                y = h;
            t = y / h;
            x = w * sqrt(1 - (t * t));
            t = K - y;
            if (rs - (t * t) >= 0)
                *xp++ = x - sqrt(rs - (t * t));
            else
                *xp++ = x;
        }
        y = (b - d) / 2;
        if (y >= 0.0 && flip == 1) {
            if (y > hepm)
                y = h;
            t = y / h;
            x = w * sqrt(1 - (t * t));
            t = K - y;
            if (rs - (t * t) >= 0)
                t = sqrt(rs - (t * t));
            else
                t = 0;
            *xp++ = x - t;
        }
    }
    if (xp > &xs[1]) {
        if (acc->left.valid && boundedLe(K, bounds->left) &&
            !boundedLe(K, bounds->outer) && xs[0] >= 0.0 && xs[1] >= 0.0)
            return xs[1];
        if (acc->right.valid && boundedLe(K, bounds->right) &&
            !boundedLe(K, bounds->inner) && xs[0] <= 0.0 && xs[1] <= 0.0)
            return xs[1];
    }
    return xs[0];
}

static miArcSpanData *
miComputeWideEllipse(int lw, xArc * parc)
{
    miArcSpanData *spdata = NULL;
    int k;

    if (!lw)
        lw = 1;
    k = (parc->height >> 1) + ((lw - 1) >> 1);
    spdata = malloc(sizeof(miArcSpanData) + sizeof(miArcSpan) * (k + 2));
    if (!spdata)
        return NULL;
    spdata->spans = (miArcSpan *) (spdata + 1);
    spdata->k = k;
    spdata->top = !(lw & 1) && !(parc->width & 1);
    spdata->bot = !(parc->height & 1);
    if (parc->width == parc->height)
        miComputeCircleSpans(lw, parc, spdata);
    else
        miComputeEllipseSpans(lw, parc, spdata);
    return spdata;
}

static void
miFillWideEllipse(DrawablePtr pDraw, GCPtr pGC, xArc * parc)
{
    DDXPointPtr points;
    DDXPointPtr pts;
    int *widths;
    int *wids;
    miArcSpanData *spdata;
    miArcSpan *span;
    int xorg, yorgu, yorgl;
    int n;

    yorgu = parc->height + pGC->lineWidth;
    n = (sizeof(int) * 2) * yorgu;
    widths = malloc(n + (sizeof(DDXPointRec) * 2) * yorgu);
    if (!widths)
        return;
    points = (DDXPointPtr) ((char *) widths + n);
    spdata = miComputeWideEllipse((int) pGC->lineWidth, parc);
    if (!spdata) {
        free(widths);
        return;
    }
    pts = points;
    wids = widths;
    span = spdata->spans;
    xorg = parc->x + (parc->width >> 1);
    yorgu = parc->y + (parc->height >> 1);
    yorgl = yorgu + (parc->height & 1);
    if (pGC->miTranslate) {
        xorg += pDraw->x;
        yorgu += pDraw->y;
        yorgl += pDraw->y;
    }
    yorgu -= spdata->k;
    yorgl += spdata->k;
    if (spdata->top) {
        pts->x = xorg;
        pts->y = yorgu - 1;
        pts++;
        *wids++ = 1;
        span++;
    }
    for (n = spdata->count1; --n >= 0;) {
        pts[0].x = xorg + span->lx;
        pts[0].y = yorgu;
        wids[0] = span->lw;
        pts[1].x = pts[0].x;
        pts[1].y = yorgl;
        wids[1] = wids[0];
        yorgu++;
        yorgl--;
        pts += 2;
        wids += 2;
        span++;
    }
    if (spdata->hole) {
        pts[0].x = xorg;
        pts[0].y = yorgl;
        wids[0] = 1;
        pts++;
        wids++;
    }
    for (n = spdata->count2; --n >= 0;) {
        pts[0].x = xorg + span->lx;
        pts[0].y = yorgu;
        wids[0] = span->lw;
        pts[1].x = xorg + span->rx;
        pts[1].y = pts[0].y;
        wids[1] = span->rw;
        pts[2].x = pts[0].x;
        pts[2].y = yorgl;
        wids[2] = wids[0];
        pts[3].x = pts[1].x;
        pts[3].y = pts[2].y;
        wids[3] = wids[1];
        yorgu++;
        yorgl--;
        pts += 4;
        wids += 4;
        span++;
    }
    if (spdata->bot) {
        if (span->rw <= 0) {
            pts[0].x = xorg + span->lx;
            pts[0].y = yorgu;
            wids[0] = span->lw;
            pts++;
            wids++;
        }
        else {
            pts[0].x = xorg + span->lx;
            pts[0].y = yorgu;
            wids[0] = span->lw;
            pts[1].x = xorg + span->rx;
            pts[1].y = pts[0].y;
            wids[1] = span->rw;
            pts += 2;
            wids += 2;
        }
    }
    free(spdata);
    (*pGC->ops->FillSpans) (pDraw, pGC, pts - points, points, widths, FALSE);

    free(widths);
}

/*
 * miPolyArc strategy:
 *
 * If arc is zero width and solid, we don't have to worry about the rasterop
 * or join styles.  For wide solid circles, we use a fast integer algorithm.
 * For wide solid ellipses, we use special case floating point code.
 * Otherwise, we set up pDrawTo and pGCTo according to the rasterop, then
 * draw using pGCTo and pDrawTo.  If the raster-op was "tricky," that is,
 * if it involves the destination, then we use PushPixels to move the bits
 * from the scratch drawable to pDraw. (See the wide line code for a
 * fuller explanation of this.)
 */

void
miWideArc(DrawablePtr pDraw, GCPtr pGC, int narcs, xArc * parcs)
{
    int i;
    xArc *parc;
    int xMin, xMax, yMin, yMax;
    int pixmapWidth = 0, pixmapHeight = 0;
    int xOrg = 0, yOrg = 0;
    int width = pGC->lineWidth;
    Bool fTricky;
    DrawablePtr pDrawTo;
    CARD32 fg, bg;
    GCPtr pGCTo;
    miPolyArcPtr polyArcs;
    int cap[2], join[2];
    int iphase;
    int halfWidth;

    if (width == 0 && pGC->lineStyle == LineSolid) {
        for (i = narcs, parc = parcs; --i >= 0; parc++) {
            miArcSpanData *spdata;
            spdata = miArcSegment(pDraw, pGC, *parc, NULL, NULL, NULL);
            free(spdata);
        }
        fillSpans(pDraw, pGC);
        return;
    }

    if ((pGC->lineStyle == LineSolid) && narcs) {
        while (parcs->width && parcs->height &&
               (parcs->angle2 >= FULLCIRCLE || parcs->angle2 <= -FULLCIRCLE)) {
            miFillWideEllipse(pDraw, pGC, parcs);
            if (!--narcs)
                return;
            parcs++;
        }
    }

    /* Set up pDrawTo and pGCTo based on the rasterop */
    switch (pGC->alu) {
    case GXclear:          /* 0 */
    case GXcopy:           /* src */
    case GXcopyInverted:   /* NOT src */
    case GXset:            /* 1 */
        fTricky = FALSE;
        pDrawTo = pDraw;
        pGCTo = pGC;
        break;
    default:
        fTricky = TRUE;

        /* find bounding box around arcs */
        xMin = yMin = MAXSHORT;
        xMax = yMax = MINSHORT;

        for (i = narcs, parc = parcs; --i >= 0; parc++) {
            xMin = min(xMin, parc->x);
            yMin = min(yMin, parc->y);
            xMax = max(xMax, (parc->x + (int) parc->width));
            yMax = max(yMax, (parc->y + (int) parc->height));
        }

        /* expand box to deal with line widths */
        halfWidth = (width + 1) / 2;
        xMin -= halfWidth;
        yMin -= halfWidth;
        xMax += halfWidth;
        yMax += halfWidth;

        /* compute pixmap size; limit it to size of drawable */
        xOrg = max(xMin, 0);
        yOrg = max(yMin, 0);
        pixmapWidth = min(xMax, pDraw->width) - xOrg;
        pixmapHeight = min(yMax, pDraw->height) - yOrg;

        /* if nothing left, return */
        if ((pixmapWidth <= 0) || (pixmapHeight <= 0))
            return;

        for (i = narcs, parc = parcs; --i >= 0; parc++) {
            parc->x -= xOrg;
            parc->y -= yOrg;
        }
        if (pGC->miTranslate) {
            xOrg += pDraw->x;
            yOrg += pDraw->y;
        }

        /* set up scratch GC */
        pGCTo = GetScratchGC(1, pDraw->pScreen);
        if (!pGCTo)
            return;
        {
            ChangeGCVal gcvals[6];

            gcvals[0].val = GXcopy;
            gcvals[1].val = 1;
            gcvals[2].val = 0;
            gcvals[3].val = pGC->lineWidth;
            gcvals[4].val = pGC->capStyle;
            gcvals[5].val = pGC->joinStyle;
            ChangeGC(NullClient, pGCTo, GCFunction |
                     GCForeground | GCBackground | GCLineWidth |
                     GCCapStyle | GCJoinStyle, gcvals);
        }

        /* allocate a bitmap of the appropriate size, and validate it */
        pDrawTo = (DrawablePtr) (*pDraw->pScreen->CreatePixmap)
            (pDraw->pScreen, pixmapWidth, pixmapHeight, 1,
             CREATE_PIXMAP_USAGE_SCRATCH);
        if (!pDrawTo) {
            FreeScratchGC(pGCTo);
            return;
        }
        ValidateGC(pDrawTo, pGCTo);
        miClearDrawable(pDrawTo, pGCTo);
    }

    fg = pGC->fgPixel;
    bg = pGC->bgPixel;

    /* the protocol sez these don't cause color changes */
    if ((pGC->fillStyle == FillTiled) ||
        (pGC->fillStyle == FillOpaqueStippled))
        bg = fg;

    polyArcs = miComputeArcs(parcs, narcs, pGC);
    if (!polyArcs)
        goto out;

    cap[0] = cap[1] = 0;
    join[0] = join[1] = 0;
    for (iphase = (pGC->lineStyle == LineDoubleDash); iphase >= 0; iphase--) {
        miArcSpanData *spdata = NULL;
        xArc lastArc;
        ChangeGCVal gcval;

        if (iphase == 1) {
            gcval.val = bg;
            ChangeGC(NullClient, pGC, GCForeground, &gcval);
            ValidateGC(pDraw, pGC);
        }
        else if (pGC->lineStyle == LineDoubleDash) {
            gcval.val = fg;
            ChangeGC(NullClient, pGC, GCForeground, &gcval);
            ValidateGC(pDraw, pGC);
        }
        for (i = 0; i < polyArcs[iphase].narcs; i++) {
            miArcDataPtr arcData;

            arcData = &polyArcs[iphase].arcs[i];
            if (spdata) {
                if (lastArc.width != arcData->arc.width ||
                    lastArc.height != arcData->arc.height) {
                    free(spdata);
                    spdata = NULL;
                }
            }
            memcpy(&lastArc, &arcData->arc, sizeof(xArc));
            spdata = miArcSegment(pDrawTo, pGCTo, arcData->arc,
                                  &arcData->bounds[RIGHT_END],
                                  &arcData->bounds[LEFT_END], spdata);
            if (polyArcs[iphase].arcs[i].render) {
                fillSpans(pDrawTo, pGCTo);
                /* don't cap self-joining arcs */
                if (polyArcs[iphase].arcs[i].selfJoin &&
                    cap[iphase] < polyArcs[iphase].arcs[i].cap)
                    cap[iphase]++;
                while (cap[iphase] < polyArcs[iphase].arcs[i].cap) {
                    int arcIndex, end;
                    miArcDataPtr arcData0;

                    arcIndex = polyArcs[iphase].caps[cap[iphase]].arcIndex;
                    end = polyArcs[iphase].caps[cap[iphase]].end;
                    arcData0 = &polyArcs[iphase].arcs[arcIndex];
                    miArcCap(pDrawTo, pGCTo,
                             &arcData0->bounds[end], end,
                             arcData0->arc.x, arcData0->arc.y,
                             (double) arcData0->arc.width / 2.0,
                             (double) arcData0->arc.height / 2.0);
                    ++cap[iphase];
                }
                while (join[iphase] < polyArcs[iphase].arcs[i].join) {
                    int arcIndex0, arcIndex1, end0, end1;
                    int phase0, phase1;
                    miArcDataPtr arcData0, arcData1;
                    miArcJoinPtr joinp;

                    joinp = &polyArcs[iphase].joins[join[iphase]];
                    arcIndex0 = joinp->arcIndex0;
                    end0 = joinp->end0;
                    arcIndex1 = joinp->arcIndex1;
                    end1 = joinp->end1;
                    phase0 = joinp->phase0;
                    phase1 = joinp->phase1;
                    arcData0 = &polyArcs[phase0].arcs[arcIndex0];
                    arcData1 = &polyArcs[phase1].arcs[arcIndex1];
                    miArcJoin(pDrawTo, pGCTo,
                              &arcData0->bounds[end0],
                              &arcData1->bounds[end1],
                              arcData0->arc.x, arcData0->arc.y,
                              (double) arcData0->arc.width / 2.0,
                              (double) arcData0->arc.height / 2.0,
                              arcData1->arc.x, arcData1->arc.y,
                              (double) arcData1->arc.width / 2.0,
                              (double) arcData1->arc.height / 2.0);
                    ++join[iphase];
                }
                if (fTricky) {
                    if (pGC->serialNumber != pDraw->serialNumber)
                        ValidateGC(pDraw, pGC);
                    (*pGC->ops->PushPixels) (pGC, (PixmapPtr) pDrawTo,
                                             pDraw, pixmapWidth,
                                             pixmapHeight, xOrg, yOrg);
                    miClearDrawable((DrawablePtr) pDrawTo, pGCTo);
                }
            }
        }
        free(spdata);
        spdata = NULL;
    }
    miFreeArcs(polyArcs, pGC);

out:
    if (fTricky) {
        (*pGCTo->pScreen->DestroyPixmap) ((PixmapPtr) pDrawTo);
        FreeScratchGC(pGCTo);
    }
}

/* Find the index of the point with the smallest y.also return the
 * smallest and largest y */
static int
GetFPolyYBounds(SppPointPtr pts, int n, double yFtrans, int *by, int *ty)
{
    SppPointPtr ptMin;
    double ymin, ymax;
    SppPointPtr ptsStart = pts;

    ptMin = pts;
    ymin = ymax = (pts++)->y;

    while (--n > 0) {
        if (pts->y < ymin) {
            ptMin = pts;
            ymin = pts->y;
        }
        if (pts->y > ymax)
            ymax = pts->y;

        pts++;
    }

    *by = ICEIL(ymin + yFtrans);
    *ty = ICEIL(ymax + yFtrans - 1);
    return ptMin - ptsStart;
}

/*
 *	miFillSppPoly written by Todd Newman; April. 1987.
 *
 *	Fill a convex polygon.  If the given polygon
 *	is not convex, then the result is undefined.
 *	The algorithm is to order the edges from smallest
 *	y to largest by partitioning the array into a left
 *	edge list and a right edge list.  The algorithm used
 *	to traverse each edge is digital differencing analyzer
 *	line algorithm with y as the major axis. There's some funny linear
 *	interpolation involved because of the subpixel postioning.
 */
static void
miFillSppPoly(DrawablePtr dst, GCPtr pgc, int count,    /* number of points */
              SppPointPtr ptsIn,        /* the points */
              int xTrans, int yTrans,   /* Translate each point by this */
              double xFtrans, double yFtrans    /* translate before conversion
                                                   by this amount.  This provides
                                                   a mechanism to match rounding
                                                   errors with any shape that must
                                                   meet the polygon exactly.
                                                 */
    )
{
    double xl = 0.0, xr = 0.0,  /* x vals of left and right edges */
        ml = 0.0,               /* left edge slope */
        mr = 0.0,               /* right edge slope */
        dy,                     /* delta y */
        i;                      /* loop counter */
    int y,                      /* current scanline */
     j, imin,                   /* index of vertex with smallest y */
     ymin,                      /* y-extents of polygon */
     ymax, *width, *FirstWidth, /* output buffer */
    *Marked;                    /* set if this vertex has been used */
    int left, right,            /* indices to first endpoints */
     nextleft, nextright;       /* indices to second endpoints */
    DDXPointPtr ptsOut, FirstPoint;     /* output buffer */

    if (pgc->miTranslate) {
        xTrans += dst->x;
        yTrans += dst->y;
    }

    imin = GetFPolyYBounds(ptsIn, count, yFtrans, &ymin, &ymax);

    y = ymax - ymin + 1;
    if ((count < 3) || (y <= 0))
        return;
    ptsOut = FirstPoint = xallocarray(y, sizeof(DDXPointRec));
    width = FirstWidth = xallocarray(y, sizeof(int));
    Marked = xallocarray(count, sizeof(int));

    if (!ptsOut || !width || !Marked) {
        free(Marked);
        free(width);
        free(ptsOut);
        return;
    }

    for (j = 0; j < count; j++)
        Marked[j] = 0;
    nextleft = nextright = imin;
    Marked[imin] = -1;
    y = ICEIL(ptsIn[nextleft].y + yFtrans);

    /*
     *  loop through all edges of the polygon
     */
    do {
        /* add a left edge if we need to */
        if ((y > (ptsIn[nextleft].y + yFtrans) ||
             ISEQUAL(y, ptsIn[nextleft].y + yFtrans)) &&
            Marked[nextleft] != 1) {
            Marked[nextleft]++;
            left = nextleft++;

            /* find the next edge, considering the end conditions */
            if (nextleft >= count)
                nextleft = 0;

            /* now compute the starting point and slope */
            dy = ptsIn[nextleft].y - ptsIn[left].y;
            if (dy != 0.0) {
                ml = (ptsIn[nextleft].x - ptsIn[left].x) / dy;
                dy = y - (ptsIn[left].y + yFtrans);
                xl = (ptsIn[left].x + xFtrans) + ml * max(dy, 0);
            }
        }

        /* add a right edge if we need to */
        if ((y > ptsIn[nextright].y + yFtrans) ||
            (ISEQUAL(y, ptsIn[nextright].y + yFtrans)
             && Marked[nextright] != 1)) {
            Marked[nextright]++;
            right = nextright--;

            /* find the next edge, considering the end conditions */
            if (nextright < 0)
                nextright = count - 1;

            /* now compute the starting point and slope */
            dy = ptsIn[nextright].y - ptsIn[right].y;
            if (dy != 0.0) {
                mr = (ptsIn[nextright].x - ptsIn[right].x) / dy;
                dy = y - (ptsIn[right].y + yFtrans);
                xr = (ptsIn[right].x + xFtrans) + mr * max(dy, 0);
            }
        }

        /*
         *  generate scans to fill while we still have
         *  a right edge as well as a left edge.
         */
        i = (min(ptsIn[nextleft].y, ptsIn[nextright].y) + yFtrans) - y;

        if (i < EPSILON) {
            if (Marked[nextleft] && Marked[nextright]) {
                /* Arrgh, we're trapped! (no more points)
                 * Out, we've got to get out of here before this decadence saps
                 * our will completely! */
                break;
            }
            continue;
        }
        else {
            j = (int) i;
            if (!j)
                j++;
        }
        while (j > 0) {
            int cxl, cxr;

            ptsOut->y = (y) + yTrans;

            cxl = ICEIL(xl);
            cxr = ICEIL(xr);
            /* reverse the edges if necessary */
            if (xl < xr) {
                *(width++) = cxr - cxl;
                (ptsOut++)->x = cxl + xTrans;
            }
            else {
                *(width++) = cxl - cxr;
                (ptsOut++)->x = cxr + xTrans;
            }
            y++;

            /* increment down the edges */
            xl += ml;
            xr += mr;
            j--;
        }
    } while (y <= ymax);

    /* Finally, fill the spans we've collected */
    (*pgc->ops->FillSpans) (dst, pgc,
                            ptsOut - FirstPoint, FirstPoint, FirstWidth, 1);
    free(Marked);
    free(FirstWidth);
    free(FirstPoint);
}
static double
angleBetween(SppPointRec center, SppPointRec point1, SppPointRec point2)
{
    double a1, a2, a;

    /*
     * reflect from X coordinates back to ellipse
     * coordinates -- y increasing upwards
     */
    a1 = miDatan2(-(point1.y - center.y), point1.x - center.x);
    a2 = miDatan2(-(point2.y - center.y), point2.x - center.x);
    a = a2 - a1;
    if (a <= -180.0)
        a += 360.0;
    else if (a > 180.0)
        a -= 360.0;
    return a;
}

static void
translateBounds(miArcFacePtr b, int x, int y, double fx, double fy)
{
    fx += x;
    fy += y;
    b->clock.x -= fx;
    b->clock.y -= fy;
    b->center.x -= fx;
    b->center.y -= fy;
    b->counterClock.x -= fx;
    b->counterClock.y -= fy;
}

static void
miArcJoin(DrawablePtr pDraw, GCPtr pGC, miArcFacePtr pLeft,
          miArcFacePtr pRight, int xOrgLeft, int yOrgLeft,
          double xFtransLeft, double yFtransLeft,
          int xOrgRight, int yOrgRight,
          double xFtransRight, double yFtransRight)
{
    SppPointRec center, corner, otherCorner;
    SppPointRec poly[5], e;
    SppPointPtr pArcPts;
    int cpt;
    SppArcRec arc;
    miArcFaceRec Right, Left;
    int polyLen = 0;
    int xOrg, yOrg;
    double xFtrans, yFtrans;
    double a;
    double ae, ac2, ec2, bc2, de;
    double width;

    xOrg = (xOrgRight + xOrgLeft) / 2;
    yOrg = (yOrgRight + yOrgLeft) / 2;
    xFtrans = (xFtransLeft + xFtransRight) / 2;
    yFtrans = (yFtransLeft + yFtransRight) / 2;
    Right = *pRight;
    translateBounds(&Right, xOrg - xOrgRight, yOrg - yOrgRight,
                    xFtrans - xFtransRight, yFtrans - yFtransRight);
    Left = *pLeft;
    translateBounds(&Left, xOrg - xOrgLeft, yOrg - yOrgLeft,
                    xFtrans - xFtransLeft, yFtrans - yFtransLeft);
    pRight = &Right;
    pLeft = &Left;

    if (pRight->clock.x == pLeft->counterClock.x &&
        pRight->clock.y == pLeft->counterClock.y)
        return;
    center = pRight->center;
    if (0 <= (a = angleBetween(center, pRight->clock, pLeft->counterClock))
        && a <= 180.0) {
        corner = pRight->clock;
        otherCorner = pLeft->counterClock;
    }
    else {
        a = angleBetween(center, pLeft->clock, pRight->counterClock);
        corner = pLeft->clock;
        otherCorner = pRight->counterClock;
    }
    switch (pGC->joinStyle) {
    case JoinRound:
        width = (pGC->lineWidth ? (double) pGC->lineWidth : (double) 1);

        arc.x = center.x - width / 2;
        arc.y = center.y - width / 2;
        arc.width = width;
        arc.height = width;
        arc.angle1 = -miDatan2(corner.y - center.y, corner.x - center.x);
        arc.angle2 = a;
        pArcPts = malloc(3 * sizeof(SppPointRec));
        if (!pArcPts)
            return;
        pArcPts[0].x = otherCorner.x;
        pArcPts[0].y = otherCorner.y;
        pArcPts[1].x = center.x;
        pArcPts[1].y = center.y;
        pArcPts[2].x = corner.x;
        pArcPts[2].y = corner.y;
        if ((cpt = miGetArcPts(&arc, 3, &pArcPts))) {
            /* by drawing with miFillSppPoly and setting the endpoints of the arc
             * to be the corners, we assure that the cap will meet up with the
             * rest of the line */
            miFillSppPoly(pDraw, pGC, cpt, pArcPts, xOrg, yOrg, xFtrans,
                          yFtrans);
        }
        free(pArcPts);
        return;
    case JoinMiter:
        /*
         * don't miter arcs with less than 11 degrees between them
         */
        if (a < 169.0) {
            poly[0] = corner;
            poly[1] = center;
            poly[2] = otherCorner;
            bc2 = (corner.x - otherCorner.x) * (corner.x - otherCorner.x) +
                (corner.y - otherCorner.y) * (corner.y - otherCorner.y);
            ec2 = bc2 / 4;
            ac2 = (corner.x - center.x) * (corner.x - center.x) +
                (corner.y - center.y) * (corner.y - center.y);
            ae = sqrt(ac2 - ec2);
            de = ec2 / ae;
            e.x = (corner.x + otherCorner.x) / 2;
            e.y = (corner.y + otherCorner.y) / 2;
            poly[3].x = e.x + de * (e.x - center.x) / ae;
            poly[3].y = e.y + de * (e.y - center.y) / ae;
            poly[4] = corner;
            polyLen = 5;
            break;
        }
    case JoinBevel:
        poly[0] = corner;
        poly[1] = center;
        poly[2] = otherCorner;
        poly[3] = corner;
        polyLen = 4;
        break;
    }
    miFillSppPoly(pDraw, pGC, polyLen, poly, xOrg, yOrg, xFtrans, yFtrans);
}

 /*ARGSUSED*/ static void
miArcCap(DrawablePtr pDraw,
         GCPtr pGC,
         miArcFacePtr pFace,
         int end, int xOrg, int yOrg, double xFtrans, double yFtrans)
{
    SppPointRec corner, otherCorner, center, endPoint, poly[5];

    corner = pFace->clock;
    otherCorner = pFace->counterClock;
    center = pFace->center;
    switch (pGC->capStyle) {
    case CapProjecting:
        poly[0].x = otherCorner.x;
        poly[0].y = otherCorner.y;
        poly[1].x = corner.x;
        poly[1].y = corner.y;
        poly[2].x = corner.x - (center.y - corner.y);
        poly[2].y = corner.y + (center.x - corner.x);
        poly[3].x = otherCorner.x - (otherCorner.y - center.y);
        poly[3].y = otherCorner.y + (otherCorner.x - center.x);
        poly[4].x = otherCorner.x;
        poly[4].y = otherCorner.y;
        miFillSppPoly(pDraw, pGC, 5, poly, xOrg, yOrg, xFtrans, yFtrans);
        break;
    case CapRound:
        /*
         * miRoundCap just needs these to be unequal.
         */
        endPoint = center;
        endPoint.x = endPoint.x + 100;
        miRoundCap(pDraw, pGC, center, endPoint, corner, otherCorner, 0,
                   -xOrg, -yOrg, xFtrans, yFtrans);
        break;
    }
}

/* MIROUNDCAP -- a private helper function
 * Put Rounded cap on end. pCenter is the center of this end of the line
 * pEnd is the center of the other end of the line. pCorner is one of the
 * two corners at this end of the line.
 * NOTE:  pOtherCorner must be counter-clockwise from pCorner.
 */
 /*ARGSUSED*/ static void
miRoundCap(DrawablePtr pDraw,
           GCPtr pGC,
           SppPointRec pCenter,
           SppPointRec pEnd,
           SppPointRec pCorner,
           SppPointRec pOtherCorner,
           int fLineEnd, int xOrg, int yOrg, double xFtrans, double yFtrans)
{
    int cpt;
    double width;
    SppArcRec arc;
    SppPointPtr pArcPts;

    width = (pGC->lineWidth ? (double) pGC->lineWidth : (double) 1);

    arc.x = pCenter.x - width / 2;
    arc.y = pCenter.y - width / 2;
    arc.width = width;
    arc.height = width;
    arc.angle1 = -miDatan2(pCorner.y - pCenter.y, pCorner.x - pCenter.x);
    if (PTISEQUAL(pCenter, pEnd))
        arc.angle2 = -180.0;
    else {
        arc.angle2 =
            -miDatan2(pOtherCorner.y - pCenter.y,
                      pOtherCorner.x - pCenter.x) - arc.angle1;
        if (arc.angle2 < 0)
            arc.angle2 += 360.0;
    }
    pArcPts = (SppPointPtr) NULL;
    if ((cpt = miGetArcPts(&arc, 0, &pArcPts))) {
        /* by drawing with miFillSppPoly and setting the endpoints of the arc
         * to be the corners, we assure that the cap will meet up with the
         * rest of the line */
        miFillSppPoly(pDraw, pGC, cpt, pArcPts, -xOrg, -yOrg, xFtrans, yFtrans);
    }
    free(pArcPts);
}

/*
 * To avoid inaccuracy at the cardinal points, use trig functions
 * which are exact for those angles
 */

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2	1.57079632679489661923
#endif

#define Dsin(d)	((d) == 0.0 ? 0.0 : ((d) == 90.0 ? 1.0 : sin(d*M_PI/180.0)))
#define Dcos(d)	((d) == 0.0 ? 1.0 : ((d) == 90.0 ? 0.0 : cos(d*M_PI/180.0)))
#define mod(a,b)	((a) >= 0 ? (a) % (b) : (b) - (-(a)) % (b))

static double
miDcos(double a)
{
    int i;

    if (floor(a / 90) == a / 90) {
        i = (int) (a / 90.0);
        switch (mod(i, 4)) {
        case 0:
            return 1;
        case 1:
            return 0;
        case 2:
            return -1;
        case 3:
            return 0;
        }
    }
    return cos(a * M_PI / 180.0);
}

static double
miDsin(double a)
{
    int i;

    if (floor(a / 90) == a / 90) {
        i = (int) (a / 90.0);
        switch (mod(i, 4)) {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return 0;
        case 3:
            return -1;
        }
    }
    return sin(a * M_PI / 180.0);
}

static double
miDasin(double v)
{
    if (v == 0)
        return 0.0;
    if (v == 1.0)
        return 90.0;
    if (v == -1.0)
        return -90.0;
    return asin(v) * (180.0 / M_PI);
}

static double
miDatan2(double dy, double dx)
{
    if (dy == 0) {
        if (dx >= 0)
            return 0.0;
        return 180.0;
    }
    else if (dx == 0) {
        if (dy > 0)
            return 90.0;
        return -90.0;
    }
    else if (fabs(dy) == fabs(dx)) {
        if (dy > 0) {
            if (dx > 0)
                return 45.0;
            return 135.0;
        }
        else {
            if (dx > 0)
                return 315.0;
            return 225.0;
        }
    }
    else {
        return atan2(dy, dx) * (180.0 / M_PI);
    }
}

/* MIGETARCPTS -- Converts an arc into a set of line segments -- a helper
 * routine for filled arc and line (round cap) code.
 * Returns the number of points in the arc.  Note that it takes a pointer
 * to a pointer to where it should put the points and an index (cpt).
 * This procedure allocates the space necessary to fit the arc points.
 * Sometimes it's convenient for those points to be at the end of an existing
 * array. (For example, if we want to leave a spare point to make sectors
 * instead of segments.)  So we pass in the malloc()ed chunk that contains the
 * array and an index saying where we should start stashing the points.
 * If there isn't an array already, we just pass in a null pointer and
 * count on realloc() to handle the null pointer correctly.
 */
static int
miGetArcPts(SppArcPtr parc,     /* points to an arc */
            int cpt,            /* number of points already in arc list */
            SppPointPtr * ppPts)
{                               /* pointer to pointer to arc-list -- modified */
    double st,                  /* Start Theta, start angle */
     et,                        /* End Theta, offset from start theta */
     dt,                        /* Delta Theta, angle to sweep ellipse */
     cdt,                       /* Cos Delta Theta, actually 2 cos(dt) */
     x0, y0,                    /* the recurrence formula needs two points to start */
     x1, y1, x2, y2,            /* this will be the new point generated */
     xc, yc;                    /* the center point */
    int count, i;
    SppPointPtr poly;

    /* The spec says that positive angles indicate counterclockwise motion.
     * Given our coordinate system (with 0,0 in the upper left corner),
     * the screen appears flipped in Y.  The easiest fix is to negate the
     * angles given */

    st = -parc->angle1;

    et = -parc->angle2;

    /* Try to get a delta theta that is within 1/2 pixel.  Then adjust it
     * so that it divides evenly into the total.
     * I'm just using cdt 'cause I'm lazy.
     */
    cdt = parc->width;
    if (parc->height > cdt)
        cdt = parc->height;
    cdt /= 2.0;
    if (cdt <= 0)
        return 0;
    if (cdt < 1.0)
        cdt = 1.0;
    dt = miDasin(1.0 / cdt);    /* minimum step necessary */
    count = et / dt;
    count = abs(count) + 1;
    dt = et / count;
    count++;

    cdt = 2 * miDcos(dt);
    if (!(poly = reallocarray(*ppPts, cpt + count, sizeof(SppPointRec))))
        return 0;
    *ppPts = poly;

    xc = parc->width / 2.0;     /* store half width and half height */
    yc = parc->height / 2.0;

    x0 = xc * miDcos(st);
    y0 = yc * miDsin(st);
    x1 = xc * miDcos(st + dt);
    y1 = yc * miDsin(st + dt);
    xc += parc->x;              /* by adding initial point, these become */
    yc += parc->y;              /* the center point */

    poly[cpt].x = (xc + x0);
    poly[cpt].y = (yc + y0);
    poly[cpt + 1].x = (xc + x1);
    poly[cpt + 1].y = (yc + y1);

    for (i = 2; i < count; i++) {
        x2 = cdt * x1 - x0;
        y2 = cdt * y1 - y0;

        poly[cpt + i].x = (xc + x2);
        poly[cpt + i].y = (yc + y2);

        x0 = x1;
        y0 = y1;
        x1 = x2;
        y1 = y2;
    }
    /* adjust the last point */
    if (fabs(parc->angle2) >= 360.0)
        poly[cpt + i - 1] = poly[0];
    else {
        poly[cpt + i - 1].x = (miDcos(st + et) * parc->width / 2.0 + xc);
        poly[cpt + i - 1].y = (miDsin(st + et) * parc->height / 2.0 + yc);
    }

    return count;
}

struct arcData {
    double x0, y0, x1, y1;
    int selfJoin;
};

#define ADD_REALLOC_STEP	20

static void
addCap(miArcCapPtr * capsp, int *ncapsp, int *sizep, int end, int arcIndex)
{
    int newsize;
    miArcCapPtr cap;

    if (*ncapsp == *sizep) {
        newsize = *sizep + ADD_REALLOC_STEP;
        cap = reallocarray(*capsp, newsize, sizeof(**capsp));
        if (!cap)
            return;
        *sizep = newsize;
        *capsp = cap;
    }
    cap = &(*capsp)[*ncapsp];
    cap->end = end;
    cap->arcIndex = arcIndex;
    ++*ncapsp;
}

static void
addJoin(miArcJoinPtr * joinsp,
        int *njoinsp,
        int *sizep,
        int end0, int index0, int phase0, int end1, int index1, int phase1)
{
    int newsize;
    miArcJoinPtr join;

    if (*njoinsp == *sizep) {
        newsize = *sizep + ADD_REALLOC_STEP;
        join = reallocarray(*joinsp, newsize, sizeof(**joinsp));
        if (!join)
            return;
        *sizep = newsize;
        *joinsp = join;
    }
    join = &(*joinsp)[*njoinsp];
    join->end0 = end0;
    join->arcIndex0 = index0;
    join->phase0 = phase0;
    join->end1 = end1;
    join->arcIndex1 = index1;
    join->phase1 = phase1;
    ++*njoinsp;
}

static miArcDataPtr
addArc(miArcDataPtr * arcsp, int *narcsp, int *sizep, xArc * xarc)
{
    int newsize;
    miArcDataPtr arc;

    if (*narcsp == *sizep) {
        newsize = *sizep + ADD_REALLOC_STEP;
        arc = reallocarray(*arcsp, newsize, sizeof(**arcsp));
        if (!arc)
            return NULL;
        *sizep = newsize;
        *arcsp = arc;
    }
    arc = &(*arcsp)[*narcsp];
    arc->arc = *xarc;
    ++*narcsp;
    return arc;
}

static void
miFreeArcs(miPolyArcPtr arcs, GCPtr pGC)
{
    int iphase;

    for (iphase = ((pGC->lineStyle == LineDoubleDash) ? 1 : 0);
         iphase >= 0; iphase--) {
        if (arcs[iphase].narcs > 0)
            free(arcs[iphase].arcs);
        if (arcs[iphase].njoins > 0)
            free(arcs[iphase].joins);
        if (arcs[iphase].ncaps > 0)
            free(arcs[iphase].caps);
    }
    free(arcs);
}

/*
 * map angles to radial distance.  This only deals with the first quadrant
 */

/*
 * a polygonal approximation to the arc for computing arc lengths
 */

#define DASH_MAP_SIZE	91

#define dashIndexToAngle(di)	((((double) (di)) * 90.0) / ((double) DASH_MAP_SIZE - 1))
#define xAngleToDashIndex(xa)	((((long) (xa)) * (DASH_MAP_SIZE - 1)) / (90 * 64))
#define dashIndexToXAngle(di)	((((long) (di)) * (90 * 64)) / (DASH_MAP_SIZE - 1))
#define dashXAngleStep	(((double) (90 * 64)) / ((double) (DASH_MAP_SIZE - 1)))

typedef struct {
    double map[DASH_MAP_SIZE];
} dashMap;

static int computeAngleFromPath(int startAngle, int endAngle, dashMap * map,
                                int *lenp, int backwards);

static void
computeDashMap(xArc * arcp, dashMap * map)
{
    int di;
    double a, x, y, prevx = 0.0, prevy = 0.0, dist;

    for (di = 0; di < DASH_MAP_SIZE; di++) {
        a = dashIndexToAngle(di);
        x = ((double) arcp->width / 2.0) * miDcos(a);
        y = ((double) arcp->height / 2.0) * miDsin(a);
        if (di == 0) {
            map->map[di] = 0.0;
        }
        else {
            dist = hypot(x - prevx, y - prevy);
            map->map[di] = map->map[di - 1] + dist;
        }
        prevx = x;
        prevy = y;
    }
}

typedef enum { HORIZONTAL, VERTICAL, OTHER } arcTypes;

/* this routine is a bit gory */

static miPolyArcPtr
miComputeArcs(xArc * parcs, int narcs, GCPtr pGC)
{
    int isDashed, isDoubleDash;
    int dashOffset;
    miPolyArcPtr arcs;
    int start, i, j, k = 0, nexti, nextk = 0;
    int joinSize[2];
    int capSize[2];
    int arcSize[2];
    int angle2;
    double a0, a1;
    struct arcData *data;
    miArcDataPtr arc;
    xArc xarc;
    int iphase, prevphase = 0, joinphase;
    int arcsJoin;
    int selfJoin;

    int iDash = 0, dashRemaining = 0;
    int iDashStart = 0, dashRemainingStart = 0, iphaseStart;
    int startAngle, spanAngle, endAngle, backwards = 0;
    int prevDashAngle, dashAngle;
    dashMap map;

    isDashed = !(pGC->lineStyle == LineSolid);
    isDoubleDash = (pGC->lineStyle == LineDoubleDash);
    dashOffset = pGC->dashOffset;

    data = xallocarray(narcs, sizeof(struct arcData));
    if (!data)
        return NULL;
    arcs = xallocarray(isDoubleDash ? 2 : 1, sizeof(*arcs));
    if (!arcs) {
        free(data);
        return NULL;
    }
    for (i = 0; i < narcs; i++) {
        a0 = todeg(parcs[i].angle1);
        angle2 = parcs[i].angle2;
        if (angle2 > FULLCIRCLE)
            angle2 = FULLCIRCLE;
        else if (angle2 < -FULLCIRCLE)
            angle2 = -FULLCIRCLE;
        data[i].selfJoin = angle2 == FULLCIRCLE || angle2 == -FULLCIRCLE;
        a1 = todeg(parcs[i].angle1 + angle2);
        data[i].x0 =
            parcs[i].x + (double) parcs[i].width / 2 * (1 + miDcos(a0));
        data[i].y0 =
            parcs[i].y + (double) parcs[i].height / 2 * (1 - miDsin(a0));
        data[i].x1 =
            parcs[i].x + (double) parcs[i].width / 2 * (1 + miDcos(a1));
        data[i].y1 =
            parcs[i].y + (double) parcs[i].height / 2 * (1 - miDsin(a1));
    }

    for (iphase = 0; iphase < (isDoubleDash ? 2 : 1); iphase++) {
        arcs[iphase].njoins = 0;
        arcs[iphase].joins = 0;
        joinSize[iphase] = 0;

        arcs[iphase].ncaps = 0;
        arcs[iphase].caps = 0;
        capSize[iphase] = 0;

        arcs[iphase].narcs = 0;
        arcs[iphase].arcs = 0;
        arcSize[iphase] = 0;
    }

    iphase = 0;
    if (isDashed) {
        iDash = 0;
        dashRemaining = pGC->dash[0];
        while (dashOffset > 0) {
            if (dashOffset >= dashRemaining) {
                dashOffset -= dashRemaining;
                iphase = iphase ? 0 : 1;
                iDash++;
                if (iDash == pGC->numInDashList)
                    iDash = 0;
                dashRemaining = pGC->dash[iDash];
            }
            else {
                dashRemaining -= dashOffset;
                dashOffset = 0;
            }
        }
        iDashStart = iDash;
        dashRemainingStart = dashRemaining;
    }
    iphaseStart = iphase;

    for (i = narcs - 1; i >= 0; i--) {
        j = i + 1;
        if (j == narcs)
            j = 0;
        if (data[i].selfJoin || i == j ||
            (UNEQUAL(data[i].x1, data[j].x0) ||
             UNEQUAL(data[i].y1, data[j].y0))) {
            if (iphase == 0 || isDoubleDash)
                addCap(&arcs[iphase].caps, &arcs[iphase].ncaps,
                       &capSize[iphase], RIGHT_END, 0);
            break;
        }
    }
    start = i + 1;
    if (start == narcs)
        start = 0;
    i = start;
    for (;;) {
        j = i + 1;
        if (j == narcs)
            j = 0;
        nexti = i + 1;
        if (nexti == narcs)
            nexti = 0;
        if (isDashed) {
            /*
             ** deal with dashed arcs.  Use special rules for certain 0 area arcs.
             ** Presumably, the other 0 area arcs still aren't done right.
             */
            arcTypes arcType = OTHER;
            CARD16 thisLength;

            if (parcs[i].height == 0
                && (parcs[i].angle1 % FULLCIRCLE) == 0x2d00
                && parcs[i].angle2 == 0x2d00)
                arcType = HORIZONTAL;
            else if (parcs[i].width == 0
                     && (parcs[i].angle1 % FULLCIRCLE) == 0x1680
                     && parcs[i].angle2 == 0x2d00)
                arcType = VERTICAL;
            if (arcType == OTHER) {
                /*
                 * precompute an approximation map
                 */
                computeDashMap(&parcs[i], &map);
                /*
                 * compute each individual dash segment using the path
                 * length function
                 */
                startAngle = parcs[i].angle1;
                spanAngle = parcs[i].angle2;
                if (spanAngle > FULLCIRCLE)
                    spanAngle = FULLCIRCLE;
                else if (spanAngle < -FULLCIRCLE)
                    spanAngle = -FULLCIRCLE;
                if (startAngle < 0)
                    startAngle = FULLCIRCLE - (-startAngle) % FULLCIRCLE;
                if (startAngle >= FULLCIRCLE)
                    startAngle = startAngle % FULLCIRCLE;
                endAngle = startAngle + spanAngle;
                backwards = spanAngle < 0;
            }
            else {
                xarc = parcs[i];
                if (arcType == VERTICAL) {
                    xarc.angle1 = 0x1680;
                    startAngle = parcs[i].y;
                    endAngle = startAngle + parcs[i].height;
                }
                else {
                    xarc.angle1 = 0x2d00;
                    startAngle = parcs[i].x;
                    endAngle = startAngle + parcs[i].width;
                }
            }
            dashAngle = startAngle;
            selfJoin = data[i].selfJoin && (iphase == 0 || isDoubleDash);
            /*
             * add dashed arcs to each bucket
             */
            arc = 0;
            while (dashAngle != endAngle) {
                prevDashAngle = dashAngle;
                if (arcType == OTHER) {
                    dashAngle = computeAngleFromPath(prevDashAngle, endAngle,
                                                     &map, &dashRemaining,
                                                     backwards);
                    /* avoid troubles with huge arcs and small dashes */
                    if (dashAngle == prevDashAngle) {
                        if (backwards)
                            dashAngle--;
                        else
                            dashAngle++;
                    }
                }
                else {
                    thisLength = (dashAngle + dashRemaining <= endAngle) ?
                        dashRemaining : endAngle - dashAngle;
                    if (arcType == VERTICAL) {
                        xarc.y = dashAngle;
                        xarc.height = thisLength;
                    }
                    else {
                        xarc.x = dashAngle;
                        xarc.width = thisLength;
                    }
                    dashAngle += thisLength;
                    dashRemaining -= thisLength;
                }
                if (iphase == 0 || isDoubleDash) {
                    if (arcType == OTHER) {
                        xarc = parcs[i];
                        spanAngle = prevDashAngle;
                        if (spanAngle < 0)
                            spanAngle = FULLCIRCLE - (-spanAngle) % FULLCIRCLE;
                        if (spanAngle >= FULLCIRCLE)
                            spanAngle = spanAngle % FULLCIRCLE;
                        xarc.angle1 = spanAngle;
                        spanAngle = dashAngle - prevDashAngle;
                        if (backwards) {
                            if (dashAngle > prevDashAngle)
                                spanAngle = -FULLCIRCLE + spanAngle;
                        }
                        else {
                            if (dashAngle < prevDashAngle)
                                spanAngle = FULLCIRCLE + spanAngle;
                        }
                        if (spanAngle > FULLCIRCLE)
                            spanAngle = FULLCIRCLE;
                        if (spanAngle < -FULLCIRCLE)
                            spanAngle = -FULLCIRCLE;
                        xarc.angle2 = spanAngle;
                    }
                    arc = addArc(&arcs[iphase].arcs, &arcs[iphase].narcs,
                                 &arcSize[iphase], &xarc);
                    if (!arc)
                        goto arcfail;
                    /*
                     * cap each end of an on/off dash
                     */
                    if (!isDoubleDash) {
                        if (prevDashAngle != startAngle) {
                            addCap(&arcs[iphase].caps,
                                   &arcs[iphase].ncaps,
                                   &capSize[iphase], RIGHT_END,
                                   arc - arcs[iphase].arcs);

                        }
                        if (dashAngle != endAngle) {
                            addCap(&arcs[iphase].caps,
                                   &arcs[iphase].ncaps,
                                   &capSize[iphase], LEFT_END,
                                   arc - arcs[iphase].arcs);
                        }
                    }
                    arc->cap = arcs[iphase].ncaps;
                    arc->join = arcs[iphase].njoins;
                    arc->render = 0;
                    arc->selfJoin = 0;
                    if (dashAngle == endAngle)
                        arc->selfJoin = selfJoin;
                }
                prevphase = iphase;
                if (dashRemaining <= 0) {
                    ++iDash;
                    if (iDash == pGC->numInDashList)
                        iDash = 0;
                    iphase = iphase ? 0 : 1;
                    dashRemaining = pGC->dash[iDash];
                }
            }
            /*
             * make sure a place exists for the position data when
             * drawing a zero-length arc
             */
            if (startAngle == endAngle) {
                prevphase = iphase;
                if (!isDoubleDash && iphase == 1)
                    prevphase = 0;
                arc = addArc(&arcs[prevphase].arcs, &arcs[prevphase].narcs,
                             &arcSize[prevphase], &parcs[i]);
                if (!arc)
                    goto arcfail;
                arc->join = arcs[prevphase].njoins;
                arc->cap = arcs[prevphase].ncaps;
                arc->selfJoin = data[i].selfJoin;
            }
        }
        else {
            arc = addArc(&arcs[iphase].arcs, &arcs[iphase].narcs,
                         &arcSize[iphase], &parcs[i]);
            if (!arc)
                goto arcfail;
            arc->join = arcs[iphase].njoins;
            arc->cap = arcs[iphase].ncaps;
            arc->selfJoin = data[i].selfJoin;
            prevphase = iphase;
        }
        if (prevphase == 0 || isDoubleDash)
            k = arcs[prevphase].narcs - 1;
        if (iphase == 0 || isDoubleDash)
            nextk = arcs[iphase].narcs;
        if (nexti == start) {
            nextk = 0;
            if (isDashed) {
                iDash = iDashStart;
                iphase = iphaseStart;
                dashRemaining = dashRemainingStart;
            }
        }
        arcsJoin = narcs > 1 && i != j &&
            ISEQUAL(data[i].x1, data[j].x0) &&
            ISEQUAL(data[i].y1, data[j].y0) &&
            !data[i].selfJoin && !data[j].selfJoin;
        if (arc) {
            if (arcsJoin)
                arc->render = 0;
            else
                arc->render = 1;
        }
        if (arcsJoin &&
            (prevphase == 0 || isDoubleDash) && (iphase == 0 || isDoubleDash)) {
            joinphase = iphase;
            if (isDoubleDash) {
                if (nexti == start)
                    joinphase = iphaseStart;
                /*
                 * if the join is right at the dash,
                 * draw the join in foreground
                 * This is because the foreground
                 * arcs are computed second, the results
                 * of which are needed to draw the join
                 */
                if (joinphase != prevphase)
                    joinphase = 0;
            }
            if (joinphase == 0 || isDoubleDash) {
                addJoin(&arcs[joinphase].joins,
                        &arcs[joinphase].njoins,
                        &joinSize[joinphase],
                        LEFT_END, k, prevphase, RIGHT_END, nextk, iphase);
                arc->join = arcs[prevphase].njoins;
            }
        }
        else {
            /*
             * cap the left end of this arc
             * unless it joins itself
             */
            if ((prevphase == 0 || isDoubleDash) && !arc->selfJoin) {
                addCap(&arcs[prevphase].caps, &arcs[prevphase].ncaps,
                       &capSize[prevphase], LEFT_END, k);
                arc->cap = arcs[prevphase].ncaps;
            }
            if (isDashed && !arcsJoin) {
                iDash = iDashStart;
                iphase = iphaseStart;
                dashRemaining = dashRemainingStart;
            }
            nextk = arcs[iphase].narcs;
            if (nexti == start) {
                nextk = 0;
                iDash = iDashStart;
                iphase = iphaseStart;
                dashRemaining = dashRemainingStart;
            }
            /*
             * cap the right end of the next arc.  If the
             * next arc is actually the first arc, only
             * cap it if it joins with this arc.  This
             * case will occur when the final dash segment
             * of an on/off dash is off.  Of course, this
             * cap will be drawn at a strange time, but that
             * hardly matters...
             */
            if ((iphase == 0 || isDoubleDash) &&
                (nexti != start || (arcsJoin && isDashed)))
                addCap(&arcs[iphase].caps, &arcs[iphase].ncaps,
                       &capSize[iphase], RIGHT_END, nextk);
        }
        i = nexti;
        if (i == start)
            break;
    }
    /*
     * make sure the last section is rendered
     */
    for (iphase = 0; iphase < (isDoubleDash ? 2 : 1); iphase++)
        if (arcs[iphase].narcs > 0) {
            arcs[iphase].arcs[arcs[iphase].narcs - 1].render = 1;
            arcs[iphase].arcs[arcs[iphase].narcs - 1].join =
                arcs[iphase].njoins;
            arcs[iphase].arcs[arcs[iphase].narcs - 1].cap = arcs[iphase].ncaps;
        }
    free(data);
    return arcs;
 arcfail:
    miFreeArcs(arcs, pGC);
    free(data);
    return NULL;
}

static double
angleToLength(int angle, dashMap * map)
{
    double len, excesslen, sidelen = map->map[DASH_MAP_SIZE - 1], totallen;
    int di;
    int excess;
    Bool oddSide = FALSE;

    totallen = 0;
    if (angle >= 0) {
        while (angle >= 90 * 64) {
            angle -= 90 * 64;
            totallen += sidelen;
            oddSide = !oddSide;
        }
    }
    else {
        while (angle < 0) {
            angle += 90 * 64;
            totallen -= sidelen;
            oddSide = !oddSide;
        }
    }
    if (oddSide)
        angle = 90 * 64 - angle;

    di = xAngleToDashIndex(angle);
    excess = angle - dashIndexToXAngle(di);

    len = map->map[di];
    /*
     * linearly interpolate between this point and the next
     */
    if (excess > 0) {
        excesslen = (map->map[di + 1] - map->map[di]) *
            ((double) excess) / dashXAngleStep;
        len += excesslen;
    }
    if (oddSide)
        totallen += (sidelen - len);
    else
        totallen += len;
    return totallen;
}

/*
 * len is along the arc, but may be more than one rotation
 */

static int
lengthToAngle(double len, dashMap * map)
{
    double sidelen = map->map[DASH_MAP_SIZE - 1];
    int angle, angleexcess;
    Bool oddSide = FALSE;
    int a0, a1, a;

    angle = 0;
    /*
     * step around the ellipse, subtracting sidelens and
     * adding 90 degrees.  oddSide will tell if the
     * map should be interpolated in reverse
     */
    if (len >= 0) {
        if (sidelen == 0)
            return 2 * FULLCIRCLE;      /* infinity */
        while (len >= sidelen) {
            angle += 90 * 64;
            len -= sidelen;
            oddSide = !oddSide;
        }
    }
    else {
        if (sidelen == 0)
            return -2 * FULLCIRCLE;     /* infinity */
        while (len < 0) {
            angle -= 90 * 64;
            len += sidelen;
            oddSide = !oddSide;
        }
    }
    if (oddSide)
        len = sidelen - len;
    a0 = 0;
    a1 = DASH_MAP_SIZE - 1;
    /*
     * binary search for the closest pre-computed length
     */
    while (a1 - a0 > 1) {
        a = (a0 + a1) / 2;
        if (len > map->map[a])
            a0 = a;
        else
            a1 = a;
    }
    angleexcess = dashIndexToXAngle(a0);
    /*
     * linearly interpolate to the next point
     */
    angleexcess += (len - map->map[a0]) /
        (map->map[a0 + 1] - map->map[a0]) * dashXAngleStep;
    if (oddSide)
        angle += (90 * 64) - angleexcess;
    else
        angle += angleexcess;
    return angle;
}

/*
 * compute the angle of an ellipse which corresponds to
 * the given path length.  Note that the correct solution
 * to this problem is an eliptic integral, we'll punt and
 * approximate (it's only for dashes anyway).  This
 * approximation uses a polygon.
 *
 * The remaining portion of len is stored in *lenp -
 * this will be negative if the arc extends beyond
 * len and positive if len extends beyond the arc.
 */

static int
computeAngleFromPath(int startAngle, int endAngle,      /* normalized absolute angles in *64 degrees */
                     dashMap * map, int *lenp, int backwards)
{
    int a0, a1, a;
    double len0;
    int len;

    a0 = startAngle;
    a1 = endAngle;
    len = *lenp;
    if (backwards) {
        /*
         * flip the problem around to always be
         * forwards
         */
        a0 = FULLCIRCLE - a0;
        a1 = FULLCIRCLE - a1;
    }
    if (a1 < a0)
        a1 += FULLCIRCLE;
    len0 = angleToLength(a0, map);
    a = lengthToAngle(len0 + len, map);
    if (a > a1) {
        a = a1;
        len -= angleToLength(a1, map) - len0;
    }
    else
        len = 0;
    if (backwards)
        a = FULLCIRCLE - a;
    *lenp = len;
    return a;
}

/*
 * scan convert wide arcs.
 */

/*
 * draw zero width/height arcs
 */

static void
drawZeroArc(DrawablePtr pDraw,
            GCPtr pGC,
            xArc * tarc, int lw, miArcFacePtr left, miArcFacePtr right)
{
    double x0 = 0.0, y0 = 0.0, x1 = 0.0, y1 = 0.0, w, h, x, y;
    double xmax, ymax, xmin, ymin;
    int a0, a1;
    double a, startAngle, endAngle;
    double l, lx, ly;

    l = lw / 2.0;
    a0 = tarc->angle1;
    a1 = tarc->angle2;
    if (a1 > FULLCIRCLE)
        a1 = FULLCIRCLE;
    else if (a1 < -FULLCIRCLE)
        a1 = -FULLCIRCLE;
    w = (double) tarc->width / 2.0;
    h = (double) tarc->height / 2.0;
    /*
     * play in X coordinates right away
     */
    startAngle = -((double) a0 / 64.0);
    endAngle = -((double) (a0 + a1) / 64.0);

    xmax = -w;
    xmin = w;
    ymax = -h;
    ymin = h;
    a = startAngle;
    for (;;) {
        x = w * miDcos(a);
        y = h * miDsin(a);
        if (a == startAngle) {
            x0 = x;
            y0 = y;
        }
        if (a == endAngle) {
            x1 = x;
            y1 = y;
        }
        if (x > xmax)
            xmax = x;
        if (x < xmin)
            xmin = x;
        if (y > ymax)
            ymax = y;
        if (y < ymin)
            ymin = y;
        if (a == endAngle)
            break;
        if (a1 < 0) {           /* clockwise */
            if (floor(a / 90.0) == floor(endAngle / 90.0))
                a = endAngle;
            else
                a = 90 * (floor(a / 90.0) + 1);
        }
        else {
            if (ceil(a / 90.0) == ceil(endAngle / 90.0))
                a = endAngle;
            else
                a = 90 * (ceil(a / 90.0) - 1);
        }
    }
    lx = ly = l;
    if ((x1 - x0) + (y1 - y0) < 0)
        lx = ly = -l;
    if (h) {
        ly = 0.0;
        lx = -lx;
    }
    else
        lx = 0.0;
    if (right) {
        right->center.x = x0;
        right->center.y = y0;
        right->clock.x = x0 - lx;
        right->clock.y = y0 - ly;
        right->counterClock.x = x0 + lx;
        right->counterClock.y = y0 + ly;
    }
    if (left) {
        left->center.x = x1;
        left->center.y = y1;
        left->clock.x = x1 + lx;
        left->clock.y = y1 + ly;
        left->counterClock.x = x1 - lx;
        left->counterClock.y = y1 - ly;
    }

    x0 = xmin;
    x1 = xmax;
    y0 = ymin;
    y1 = ymax;
    if (ymin != y1) {
        xmin = -l;
        xmax = l;
    }
    else {
        ymin = -l;
        ymax = l;
    }
    if (xmax != xmin && ymax != ymin) {
        int minx, maxx, miny, maxy;
        xRectangle rect;

        minx = ICEIL(xmin + w) + tarc->x;
        maxx = ICEIL(xmax + w) + tarc->x;
        miny = ICEIL(ymin + h) + tarc->y;
        maxy = ICEIL(ymax + h) + tarc->y;
        rect.x = minx;
        rect.y = miny;
        rect.width = maxx - minx;
        rect.height = maxy - miny;
        (*pGC->ops->PolyFillRect) (pDraw, pGC, 1, &rect);
    }
}

/*
 * this computes the ellipse y value associated with the
 * bottom of the tail.
 */

static void
tailEllipseY(struct arc_def *def, struct accelerators *acc)
{
    double t;

    acc->tail_y = 0.0;
    if (def->w == def->h)
        return;
    t = def->l * def->w;
    if (def->w > def->h) {
        if (t < acc->h2)
            return;
    }
    else {
        if (t > acc->h2)
            return;
    }
    t = 2.0 * def->h * t;
    t = (CUBED_ROOT_4 * acc->h2 - cbrt(t * t)) / acc->h2mw2;
    if (t > 0.0)
        acc->tail_y = def->h / CUBED_ROOT_2 * sqrt(t);
}

/*
 * inverse functions -- compute edge coordinates
 * from the ellipse
 */

static double
outerXfromXY(double x, double y, struct arc_def *def, struct accelerators *acc)
{
    return x + (x * acc->h2l) / sqrt(x * x * acc->h4 + y * y * acc->w4);
}

static double
outerYfromXY(double x, double y, struct arc_def *def, struct accelerators *acc)
{
    return y + (y * acc->w2l) / sqrt(x * x * acc->h4 + y * y * acc->w4);
}

static double
innerXfromXY(double x, double y, struct arc_def *def, struct accelerators *acc)
{
    return x - (x * acc->h2l) / sqrt(x * x * acc->h4 + y * y * acc->w4);
}

static double
innerYfromXY(double x, double y, struct arc_def *def, struct accelerators *acc)
{
    return y - (y * acc->w2l) / sqrt(x * x * acc->h4 + y * y * acc->w4);
}

static double
innerYfromY(double y, struct arc_def *def, struct accelerators *acc)
{
    double x;

    x = (def->w / def->h) * sqrt(acc->h2 - y * y);

    return y - (y * acc->w2l) / sqrt(x * x * acc->h4 + y * y * acc->w4);
}

static void
computeLine(double x1, double y1, double x2, double y2, struct line *line)
{
    if (y1 == y2)
        line->valid = 0;
    else {
        line->m = (x1 - x2) / (y1 - y2);
        line->b = x1 - y1 * line->m;
        line->valid = 1;
    }
}

/*
 * compute various accelerators for an ellipse.  These
 * are simply values that are used repeatedly in
 * the computations
 */

static void
computeAcc(xArc * tarc, int lw, struct arc_def *def, struct accelerators *acc)
{
    def->w = ((double) tarc->width) / 2.0;
    def->h = ((double) tarc->height) / 2.0;
    def->l = ((double) lw) / 2.0;
    acc->h2 = def->h * def->h;
    acc->w2 = def->w * def->w;
    acc->h4 = acc->h2 * acc->h2;
    acc->w4 = acc->w2 * acc->w2;
    acc->h2l = acc->h2 * def->l;
    acc->w2l = acc->w2 * def->l;
    acc->h2mw2 = acc->h2 - acc->w2;
    acc->fromIntX = (tarc->width & 1) ? 0.5 : 0.0;
    acc->fromIntY = (tarc->height & 1) ? 0.5 : 0.0;
    acc->xorg = tarc->x + (tarc->width >> 1);
    acc->yorgu = tarc->y + (tarc->height >> 1);
    acc->yorgl = acc->yorgu + (tarc->height & 1);
    tailEllipseY(def, acc);
}

/*
 * compute y value bounds of various portions of the arc,
 * the outer edge, the ellipse and the inner edge.
 */

static void
computeBound(struct arc_def *def,
             struct arc_bound *bound,
             struct accelerators *acc, miArcFacePtr right, miArcFacePtr left)
{
    double t;
    double innerTaily;
    double tail_y;
    struct bound innerx, outerx;
    struct bound ellipsex;

    bound->ellipse.min = Dsin(def->a0) * def->h;
    bound->ellipse.max = Dsin(def->a1) * def->h;
    if (def->a0 == 45 && def->w == def->h)
        ellipsex.min = bound->ellipse.min;
    else
        ellipsex.min = Dcos(def->a0) * def->w;
    if (def->a1 == 45 && def->w == def->h)
        ellipsex.max = bound->ellipse.max;
    else
        ellipsex.max = Dcos(def->a1) * def->w;
    bound->outer.min = outerYfromXY(ellipsex.min, bound->ellipse.min, def, acc);
    bound->outer.max = outerYfromXY(ellipsex.max, bound->ellipse.max, def, acc);
    bound->inner.min = innerYfromXY(ellipsex.min, bound->ellipse.min, def, acc);
    bound->inner.max = innerYfromXY(ellipsex.max, bound->ellipse.max, def, acc);

    outerx.min = outerXfromXY(ellipsex.min, bound->ellipse.min, def, acc);
    outerx.max = outerXfromXY(ellipsex.max, bound->ellipse.max, def, acc);
    innerx.min = innerXfromXY(ellipsex.min, bound->ellipse.min, def, acc);
    innerx.max = innerXfromXY(ellipsex.max, bound->ellipse.max, def, acc);

    /*
     * save the line end points for the
     * cap code to use.  Careful here, these are
     * in cartesean coordinates (y increasing upwards)
     * while the cap code uses inverted coordinates
     * (y increasing downwards)
     */

    if (right) {
        right->counterClock.y = bound->outer.min;
        right->counterClock.x = outerx.min;
        right->center.y = bound->ellipse.min;
        right->center.x = ellipsex.min;
        right->clock.y = bound->inner.min;
        right->clock.x = innerx.min;
    }

    if (left) {
        left->clock.y = bound->outer.max;
        left->clock.x = outerx.max;
        left->center.y = bound->ellipse.max;
        left->center.x = ellipsex.max;
        left->counterClock.y = bound->inner.max;
        left->counterClock.x = innerx.max;
    }

    bound->left.min = bound->inner.max;
    bound->left.max = bound->outer.max;
    bound->right.min = bound->inner.min;
    bound->right.max = bound->outer.min;

    computeLine(innerx.min, bound->inner.min, outerx.min, bound->outer.min,
                &acc->right);
    computeLine(innerx.max, bound->inner.max, outerx.max, bound->outer.max,
                &acc->left);

    if (bound->inner.min > bound->inner.max) {
        t = bound->inner.min;
        bound->inner.min = bound->inner.max;
        bound->inner.max = t;
    }
    tail_y = acc->tail_y;
    if (tail_y > bound->ellipse.max)
        tail_y = bound->ellipse.max;
    else if (tail_y < bound->ellipse.min)
        tail_y = bound->ellipse.min;
    innerTaily = innerYfromY(tail_y, def, acc);
    if (bound->inner.min > innerTaily)
        bound->inner.min = innerTaily;
    if (bound->inner.max < innerTaily)
        bound->inner.max = innerTaily;
    bound->inneri.min = ICEIL(bound->inner.min - acc->fromIntY);
    bound->inneri.max = floor(bound->inner.max - acc->fromIntY);
    bound->outeri.min = ICEIL(bound->outer.min - acc->fromIntY);
    bound->outeri.max = floor(bound->outer.max - acc->fromIntY);
}

/*
 * this section computes the x value of the span at y
 * intersected with the specified face of the ellipse.
 *
 * this is the min/max X value over the set of normal
 * lines to the entire ellipse,  the equation of the
 * normal lines is:
 *
 *     ellipse_x h^2                   h^2
 * x = ------------ y + ellipse_x (1 - --- )
 *     ellipse_y w^2                   w^2
 *
 * compute the derivative with-respect-to ellipse_y and solve
 * for zero:
 *
 *       (w^2 - h^2) ellipse_y^3 + h^4 y
 * 0 = - ----------------------------------
 *       h w ellipse_y^2 sqrt (h^2 - ellipse_y^2)
 *
 *             (   h^4 y     )
 * ellipse_y = ( ----------  ) ^ (1/3)
 *             ( (h^2 - w^2) )
 *
 * The other two solutions to the equation are imaginary.
 *
 * This gives the position on the ellipse which generates
 * the normal with the largest/smallest x intersection point.
 *
 * Now compute the second derivative to check whether
 * the intersection is a minimum or maximum:
 *
 *    h (y0^3 (w^2 - h^2) + h^2 y (3y0^2 - 2h^2))
 * -  -------------------------------------------
 *          w y0^3 (sqrt (h^2 - y^2)) ^ 3
 *
 * as we only care about the sign,
 *
 * - (y0^3 (w^2 - h^2) + h^2 y (3y0^2 - 2h^2))
 *
 * or (to use accelerators),
 *
 * y0^3 (h^2 - w^2) - h^2 y (3y0^2 - 2h^2)
 *
 */

/*
 * computes the position on the ellipse whose normal line
 * intersects the given scan line maximally
 */

static double
hookEllipseY(double scan_y,
             struct arc_bound *bound, struct accelerators *acc, int left)
{
    double ret;

    if (acc->h2mw2 == 0) {
        if ((scan_y > 0 && !left) || (scan_y < 0 && left))
            return bound->ellipse.min;
        return bound->ellipse.max;
    }
    ret = (acc->h4 * scan_y) / (acc->h2mw2);
    if (ret >= 0)
        return cbrt(ret);
    else
        return -cbrt(-ret);
}

/*
 * computes the X value of the intersection of the
 * given scan line with the right side of the lower hook
 */

static double
hookX(double scan_y,
      struct arc_def *def,
      struct arc_bound *bound, struct accelerators *acc, int left)
{
    double ellipse_y, x;
    double maxMin;

    if (def->w != def->h) {
        ellipse_y = hookEllipseY(scan_y, bound, acc, left);
        if (boundedLe(ellipse_y, bound->ellipse)) {
            /*
             * compute the value of the second
             * derivative
             */
            maxMin = ellipse_y * ellipse_y * ellipse_y * acc->h2mw2 -
                acc->h2 * scan_y * (3 * ellipse_y * ellipse_y - 2 * acc->h2);
            if ((left && maxMin > 0) || (!left && maxMin < 0)) {
                if (ellipse_y == 0)
                    return def->w + left ? -def->l : def->l;
                x = (acc->h2 * scan_y - ellipse_y * acc->h2mw2) *
                    sqrt(acc->h2 - ellipse_y * ellipse_y) /
                    (def->h * def->w * ellipse_y);
                return x;
            }
        }
    }
    if (left) {
        if (acc->left.valid && boundedLe(scan_y, bound->left)) {
            x = intersectLine(scan_y, acc->left);
        }
        else {
            if (acc->right.valid)
                x = intersectLine(scan_y, acc->right);
            else
                x = def->w - def->l;
        }
    }
    else {
        if (acc->right.valid && boundedLe(scan_y, bound->right)) {
            x = intersectLine(scan_y, acc->right);
        }
        else {
            if (acc->left.valid)
                x = intersectLine(scan_y, acc->left);
            else
                x = def->w - def->l;
        }
    }
    return x;
}

/*
 * generate the set of spans with
 * the given y coordinate
 */

static void
arcSpan(int y,
        int lx,
        int lw,
        int rx,
        int rw,
        struct arc_def *def,
        struct arc_bound *bounds, struct accelerators *acc, int mask)
{
    int linx, loutx, rinx, routx;
    double x, altx;

    if (boundedLe(y, bounds->inneri)) {
        linx = -(lx + lw);
        rinx = rx;
    }
    else {
        /*
         * intersection with left face
         */
        x = hookX(y + acc->fromIntY, def, bounds, acc, 1);
        if (acc->right.valid && boundedLe(y + acc->fromIntY, bounds->right)) {
            altx = intersectLine(y + acc->fromIntY, acc->right);
            if (altx < x)
                x = altx;
        }
        linx = -ICEIL(acc->fromIntX - x);
        rinx = ICEIL(acc->fromIntX + x);
    }
    if (boundedLe(y, bounds->outeri)) {
        loutx = -lx;
        routx = rx + rw;
    }
    else {
        /*
         * intersection with right face
         */
        x = hookX(y + acc->fromIntY, def, bounds, acc, 0);
        if (acc->left.valid && boundedLe(y + acc->fromIntY, bounds->left)) {
            altx = x;
            x = intersectLine(y + acc->fromIntY, acc->left);
            if (x < altx)
                x = altx;
        }
        loutx = -ICEIL(acc->fromIntX - x);
        routx = ICEIL(acc->fromIntX + x);
    }
    if (routx > rinx) {
        if (mask & 1)
            newFinalSpan(acc->yorgu - y, acc->xorg + rinx, acc->xorg + routx);
        if (mask & 8)
            newFinalSpan(acc->yorgl + y, acc->xorg + rinx, acc->xorg + routx);
    }
    if (loutx > linx) {
        if (mask & 2)
            newFinalSpan(acc->yorgu - y, acc->xorg - loutx, acc->xorg - linx);
        if (mask & 4)
            newFinalSpan(acc->yorgl + y, acc->xorg - loutx, acc->xorg - linx);
    }
}

static void
arcSpan0(int lx,
         int lw,
         int rx,
         int rw,
         struct arc_def *def,
         struct arc_bound *bounds, struct accelerators *acc, int mask)
{
    double x;

    if (boundedLe(0, bounds->inneri) &&
        acc->left.valid && boundedLe(0, bounds->left) && acc->left.b > 0) {
        x = def->w - def->l;
        if (acc->left.b < x)
            x = acc->left.b;
        lw = ICEIL(acc->fromIntX - x) - lx;
        rw += rx;
        rx = ICEIL(acc->fromIntX + x);
        rw -= rx;
    }
    arcSpan(0, lx, lw, rx, rw, def, bounds, acc, mask);
}

static void
tailSpan(int y,
         int lw,
         int rw,
         struct arc_def *def,
         struct arc_bound *bounds, struct accelerators *acc, int mask)
{
    double yy, xalt, x, lx, rx;
    int n;

    if (boundedLe(y, bounds->outeri))
        arcSpan(y, 0, lw, -rw, rw, def, bounds, acc, mask);
    else if (def->w != def->h) {
        yy = y + acc->fromIntY;
        x = tailX(yy, def, bounds, acc);
        if (yy == 0.0 && x == -rw - acc->fromIntX)
            return;
        if (acc->right.valid && boundedLe(yy, bounds->right)) {
            rx = x;
            lx = -x;
            xalt = intersectLine(yy, acc->right);
            if (xalt >= -rw - acc->fromIntX && xalt <= rx)
                rx = xalt;
            n = ICEIL(acc->fromIntX + lx);
            if (lw > n) {
                if (mask & 2)
                    newFinalSpan(acc->yorgu - y, acc->xorg + n, acc->xorg + lw);
                if (mask & 4)
                    newFinalSpan(acc->yorgl + y, acc->xorg + n, acc->xorg + lw);
            }
            n = ICEIL(acc->fromIntX + rx);
            if (n > -rw) {
                if (mask & 1)
                    newFinalSpan(acc->yorgu - y, acc->xorg - rw, acc->xorg + n);
                if (mask & 8)
                    newFinalSpan(acc->yorgl + y, acc->xorg - rw, acc->xorg + n);
            }
        }
        arcSpan(y,
                ICEIL(acc->fromIntX - x), 0,
                ICEIL(acc->fromIntX + x), 0, def, bounds, acc, mask);
    }
}

/*
 * create whole arcs out of pieces.  This code is
 * very bad.
 */

static struct finalSpan **finalSpans = NULL;
static int finalMiny = 0, finalMaxy = -1;
static int finalSize = 0;

static int nspans = 0;          /* total spans, not just y coords */

struct finalSpan {
    struct finalSpan *next;
    int min, max;               /* x values */
};

static struct finalSpan *freeFinalSpans, *tmpFinalSpan;

#define allocFinalSpan()   (freeFinalSpans ?\
				((tmpFinalSpan = freeFinalSpans), \
				 (freeFinalSpans = freeFinalSpans->next), \
				 (tmpFinalSpan->next = 0), \
				 tmpFinalSpan) : \
			     realAllocSpan ())

#define SPAN_CHUNK_SIZE    128

struct finalSpanChunk {
    struct finalSpan data[SPAN_CHUNK_SIZE];
    struct finalSpanChunk *next;
};

static struct finalSpanChunk *chunks;

static struct finalSpan *
realAllocSpan(void)
{
    struct finalSpanChunk *newChunk;
    struct finalSpan *span;
    int i;

    newChunk = malloc(sizeof(struct finalSpanChunk));
    if (!newChunk)
        return (struct finalSpan *) NULL;
    newChunk->next = chunks;
    chunks = newChunk;
    freeFinalSpans = span = newChunk->data + 1;
    for (i = 1; i < SPAN_CHUNK_SIZE - 1; i++) {
        span->next = span + 1;
        span++;
    }
    span->next = 0;
    span = newChunk->data;
    span->next = 0;
    return span;
}

static void
disposeFinalSpans(void)
{
    struct finalSpanChunk *chunk, *next;

    for (chunk = chunks; chunk; chunk = next) {
        next = chunk->next;
        free(chunk);
    }
    chunks = 0;
    freeFinalSpans = 0;
    free(finalSpans);
    finalSpans = 0;
}

static void
fillSpans(DrawablePtr pDrawable, GCPtr pGC)
{
    struct finalSpan *span;
    DDXPointPtr xSpan;
    int *xWidth;
    int i;
    struct finalSpan **f;
    int spany;
    DDXPointPtr xSpans;
    int *xWidths;

    if (nspans == 0)
        return;
    xSpan = xSpans = xallocarray(nspans, sizeof(DDXPointRec));
    xWidth = xWidths = xallocarray(nspans, sizeof(int));
    if (xSpans && xWidths) {
        i = 0;
        f = finalSpans;
        for (spany = finalMiny; spany <= finalMaxy; spany++, f++) {
            for (span = *f; span; span = span->next) {
                if (span->max <= span->min)
                    continue;
                xSpan->x = span->min;
                xSpan->y = spany;
                ++xSpan;
                *xWidth++ = span->max - span->min;
                ++i;
            }
        }
        (*pGC->ops->FillSpans) (pDrawable, pGC, i, xSpans, xWidths, TRUE);
    }
    disposeFinalSpans();
    free(xSpans);
    free(xWidths);
    finalMiny = 0;
    finalMaxy = -1;
    finalSize = 0;
    nspans = 0;
}

#define SPAN_REALLOC	100

#define findSpan(y) ((finalMiny <= (y) && (y) <= finalMaxy) ? \
			  &finalSpans[(y) - finalMiny] : \
			  realFindSpan (y))

static struct finalSpan **
realFindSpan(int y)
{
    struct finalSpan **newSpans;
    int newSize, newMiny, newMaxy;
    int change;
    int i;

    if (y < finalMiny || y > finalMaxy) {
        if (!finalSize) {
            finalMiny = y;
            finalMaxy = y - 1;
        }
        if (y < finalMiny)
            change = finalMiny - y;
        else
            change = y - finalMaxy;
        if (change >= SPAN_REALLOC)
            change += SPAN_REALLOC;
        else
            change = SPAN_REALLOC;
        newSize = finalSize + change;
        newSpans = xallocarray(newSize, sizeof(struct finalSpan *));
        if (!newSpans)
            return NULL;
        newMiny = finalMiny;
        newMaxy = finalMaxy;
        if (y < finalMiny)
            newMiny = finalMiny - change;
        else
            newMaxy = finalMaxy + change;
        if (finalSpans) {
            memmove(((char *) newSpans) +
                    (finalMiny - newMiny) * sizeof(struct finalSpan *),
                    (char *) finalSpans,
                    finalSize * sizeof(struct finalSpan *));
            free(finalSpans);
        }
        if ((i = finalMiny - newMiny) > 0)
            memset((char *) newSpans, 0, i * sizeof(struct finalSpan *));
        if ((i = newMaxy - finalMaxy) > 0)
            memset((char *) (newSpans + newSize - i), 0,
                   i * sizeof(struct finalSpan *));
        finalSpans = newSpans;
        finalMaxy = newMaxy;
        finalMiny = newMiny;
        finalSize = newSize;
    }
    return &finalSpans[y - finalMiny];
}

static void
newFinalSpan(int y, int xmin, int xmax)
{
    struct finalSpan *x;
    struct finalSpan **f;
    struct finalSpan *oldx;
    struct finalSpan *prev;

    f = findSpan(y);
    if (!f)
        return;
    oldx = 0;
    for (;;) {
        prev = 0;
        for (x = *f; x; x = x->next) {
            if (x == oldx) {
                prev = x;
                continue;
            }
            if (x->min <= xmax && xmin <= x->max) {
                if (oldx) {
                    oldx->min = min(x->min, xmin);
                    oldx->max = max(x->max, xmax);
                    if (prev)
                        prev->next = x->next;
                    else
                        *f = x->next;
                    --nspans;
                }
                else {
                    x->min = min(x->min, xmin);
                    x->max = max(x->max, xmax);
                    oldx = x;
                }
                xmin = oldx->min;
                xmax = oldx->max;
                break;
            }
            prev = x;
        }
        if (!x)
            break;
    }
    if (!oldx) {
        x = allocFinalSpan();
        if (x) {
            x->min = xmin;
            x->max = xmax;
            x->next = *f;
            *f = x;
            ++nspans;
        }
    }
}

static void
mirrorSppPoint(int quadrant, SppPointPtr sppPoint)
{
    switch (quadrant) {
    case 0:
        break;
    case 1:
        sppPoint->x = -sppPoint->x;
        break;
    case 2:
        sppPoint->x = -sppPoint->x;
        sppPoint->y = -sppPoint->y;
        break;
    case 3:
        sppPoint->y = -sppPoint->y;
        break;
    }
    /*
     * and translate to X coordinate system
     */
    sppPoint->y = -sppPoint->y;
}

/*
 * split an arc into pieces which are scan-converted
 * in the first-quadrant and mirrored into position.
 * This is necessary as the scan-conversion code can
 * only deal with arcs completely contained in the
 * first quadrant.
 */

static miArcSpanData *
drawArc(xArc * tarc, int l, int a0, int a1, miArcFacePtr right,
        miArcFacePtr left, miArcSpanData *spdata)
{                               /* save end line points */
    struct arc_def def;
    struct accelerators acc;
    int startq, endq, curq;
    int rightq, leftq = 0, righta = 0, lefta = 0;
    miArcFacePtr passRight, passLeft;
    int q0 = 0, q1 = 0, mask;
    struct band {
        int a0, a1;
        int mask;
    } band[5], sweep[20];
    int bandno, sweepno;
    int i, j;
    int flipRight = 0, flipLeft = 0;
    int copyEnd = 0;

    if (!spdata)
        spdata = miComputeWideEllipse(l, tarc);
    if (!spdata)
        return NULL;

    if (a1 < a0)
        a1 += 360 * 64;
    startq = a0 / (90 * 64);
    if (a0 == a1)
        endq = startq;
    else
        endq = (a1 - 1) / (90 * 64);
    bandno = 0;
    curq = startq;
    rightq = -1;
    for (;;) {
        switch (curq) {
        case 0:
            if (a0 > 90 * 64)
                q0 = 0;
            else
                q0 = a0;
            if (a1 < 360 * 64)
                q1 = min(a1, 90 * 64);
            else
                q1 = 90 * 64;
            if (curq == startq && a0 == q0 && rightq < 0) {
                righta = q0;
                rightq = curq;
            }
            if (curq == endq && a1 == q1) {
                lefta = q1;
                leftq = curq;
            }
            break;
        case 1:
            if (a1 < 90 * 64)
                q0 = 0;
            else
                q0 = 180 * 64 - min(a1, 180 * 64);
            if (a0 > 180 * 64)
                q1 = 90 * 64;
            else
                q1 = 180 * 64 - max(a0, 90 * 64);
            if (curq == startq && 180 * 64 - a0 == q1) {
                righta = q1;
                rightq = curq;
            }
            if (curq == endq && 180 * 64 - a1 == q0) {
                lefta = q0;
                leftq = curq;
            }
            break;
        case 2:
            if (a0 > 270 * 64)
                q0 = 0;
            else
                q0 = max(a0, 180 * 64) - 180 * 64;
            if (a1 < 180 * 64)
                q1 = 90 * 64;
            else
                q1 = min(a1, 270 * 64) - 180 * 64;
            if (curq == startq && a0 - 180 * 64 == q0) {
                righta = q0;
                rightq = curq;
            }
            if (curq == endq && a1 - 180 * 64 == q1) {
                lefta = q1;
                leftq = curq;
            }
            break;
        case 3:
            if (a1 < 270 * 64)
                q0 = 0;
            else
                q0 = 360 * 64 - min(a1, 360 * 64);
            q1 = 360 * 64 - max(a0, 270 * 64);
            if (curq == startq && 360 * 64 - a0 == q1) {
                righta = q1;
                rightq = curq;
            }
            if (curq == endq && 360 * 64 - a1 == q0) {
                lefta = q0;
                leftq = curq;
            }
            break;
        }
        band[bandno].a0 = q0;
        band[bandno].a1 = q1;
        band[bandno].mask = 1 << curq;
        bandno++;
        if (curq == endq)
            break;
        curq++;
        if (curq == 4) {
            a0 = 0;
            a1 -= 360 * 64;
            curq = 0;
            endq -= 4;
        }
    }
    sweepno = 0;
    for (;;) {
        q0 = 90 * 64;
        mask = 0;
        /*
         * find left-most point
         */
        for (i = 0; i < bandno; i++)
            if (band[i].a0 <= q0) {
                q0 = band[i].a0;
                q1 = band[i].a1;
                mask = band[i].mask;
            }
        if (!mask)
            break;
        /*
         * locate next point of change
         */
        for (i = 0; i < bandno; i++)
            if (!(mask & band[i].mask)) {
                if (band[i].a0 == q0) {
                    if (band[i].a1 < q1)
                        q1 = band[i].a1;
                    mask |= band[i].mask;
                }
                else if (band[i].a0 < q1)
                    q1 = band[i].a0;
            }
        /*
         * create a new sweep
         */
        sweep[sweepno].a0 = q0;
        sweep[sweepno].a1 = q1;
        sweep[sweepno].mask = mask;
        sweepno++;
        /*
         * subtract the sweep from the affected bands
         */
        for (i = 0; i < bandno; i++)
            if (band[i].a0 == q0) {
                band[i].a0 = q1;
                /*
                 * check if this band is empty
                 */
                if (band[i].a0 == band[i].a1)
                    band[i].a1 = band[i].a0 = 90 * 64 + 1;
            }
    }
    computeAcc(tarc, l, &def, &acc);
    for (j = 0; j < sweepno; j++) {
        mask = sweep[j].mask;
        passRight = passLeft = 0;
        if (mask & (1 << rightq)) {
            if (sweep[j].a0 == righta)
                passRight = right;
            else if (sweep[j].a1 == righta) {
                passLeft = right;
                flipRight = 1;
            }
        }
        if (mask & (1 << leftq)) {
            if (sweep[j].a1 == lefta) {
                if (passLeft)
                    copyEnd = 1;
                passLeft = left;
            }
            else if (sweep[j].a0 == lefta) {
                if (passRight)
                    copyEnd = 1;
                passRight = left;
                flipLeft = 1;
            }
        }
        drawQuadrant(&def, &acc, sweep[j].a0, sweep[j].a1, mask,
                     passRight, passLeft, spdata);
    }
    /*
     * when copyEnd is set, both ends of the arc were computed
     * at the same time; drawQuadrant only takes one end though,
     * so the left end will be the only one holding the data.  Copy
     * it from there.
     */
    if (copyEnd)
        *right = *left;
    /*
     * mirror the coordinates generated for the
     * faces of the arc
     */
    if (right) {
        mirrorSppPoint(rightq, &right->clock);
        mirrorSppPoint(rightq, &right->center);
        mirrorSppPoint(rightq, &right->counterClock);
        if (flipRight) {
            SppPointRec temp;

            temp = right->clock;
            right->clock = right->counterClock;
            right->counterClock = temp;
        }
    }
    if (left) {
        mirrorSppPoint(leftq, &left->counterClock);
        mirrorSppPoint(leftq, &left->center);
        mirrorSppPoint(leftq, &left->clock);
        if (flipLeft) {
            SppPointRec temp;

            temp = left->clock;
            left->clock = left->counterClock;
            left->counterClock = temp;
        }
    }
    return spdata;
}

static void
drawQuadrant(struct arc_def *def,
             struct accelerators *acc,
             int a0,
             int a1,
             int mask,
             miArcFacePtr right, miArcFacePtr left, miArcSpanData * spdata)
{
    struct arc_bound bound;
    double yy, x, xalt;
    int y, miny, maxy;
    int n;
    miArcSpan *span;

    def->a0 = ((double) a0) / 64.0;
    def->a1 = ((double) a1) / 64.0;
    computeBound(def, &bound, acc, right, left);
    yy = bound.inner.min;
    if (bound.outer.min < yy)
        yy = bound.outer.min;
    miny = ICEIL(yy - acc->fromIntY);
    yy = bound.inner.max;
    if (bound.outer.max > yy)
        yy = bound.outer.max;
    maxy = floor(yy - acc->fromIntY);
    y = spdata->k;
    span = spdata->spans;
    if (spdata->top) {
        if (a1 == 90 * 64 && (mask & 1))
            newFinalSpan(acc->yorgu - y - 1, acc->xorg, acc->xorg + 1);
        span++;
    }
    for (n = spdata->count1; --n >= 0;) {
        if (y < miny)
            return;
        if (y <= maxy) {
            arcSpan(y,
                    span->lx, -span->lx, 0, span->lx + span->lw,
                    def, &bound, acc, mask);
            if (span->rw + span->rx)
                tailSpan(y, -span->rw, -span->rx, def, &bound, acc, mask);
        }
        y--;
        span++;
    }
    if (y < miny)
        return;
    if (spdata->hole) {
        if (y <= maxy)
            arcSpan(y, 0, 0, 0, 1, def, &bound, acc, mask & 0xc);
    }
    for (n = spdata->count2; --n >= 0;) {
        if (y < miny)
            return;
        if (y <= maxy)
            arcSpan(y, span->lx, span->lw, span->rx, span->rw,
                    def, &bound, acc, mask);
        y--;
        span++;
    }
    if (spdata->bot && miny <= y && y <= maxy) {
        n = mask;
        if (y == miny)
            n &= 0xc;
        if (span->rw <= 0) {
            arcSpan0(span->lx, -span->lx, 0, span->lx + span->lw,
                     def, &bound, acc, n);
            if (span->rw + span->rx)
                tailSpan(y, -span->rw, -span->rx, def, &bound, acc, n);
        }
        else
            arcSpan0(span->lx, span->lw, span->rx, span->rw,
                     def, &bound, acc, n);
        y--;
    }
    while (y >= miny) {
        yy = y + acc->fromIntY;
        if (def->w == def->h) {
            xalt = def->w - def->l;
            x = -sqrt(xalt * xalt - yy * yy);
        }
        else {
            x = tailX(yy, def, &bound, acc);
            if (acc->left.valid && boundedLe(yy, bound.left)) {
                xalt = intersectLine(yy, acc->left);
                if (xalt < x)
                    x = xalt;
            }
            if (acc->right.valid && boundedLe(yy, bound.right)) {
                xalt = intersectLine(yy, acc->right);
                if (xalt < x)
                    x = xalt;
            }
        }
        arcSpan(y,
                ICEIL(acc->fromIntX - x), 0,
                ICEIL(acc->fromIntX + x), 0, def, &bound, acc, mask);
        y--;
    }
}

void
miPolyArc(DrawablePtr pDraw, GCPtr pGC, int narcs, xArc * parcs)
{
    if (pGC->lineWidth == 0)
        miZeroPolyArc(pDraw, pGC, narcs, parcs);
    else
        miWideArc(pDraw, pGC, narcs, parcs);
}
