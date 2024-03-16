/**
 * Copyright © 2011 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <xfixesint.h>
#include <X11/extensions/xfixeswire.h>

#include "tests-common.h"

static void
_fixes_test_direction(struct PointerBarrier *barrier, int d[4], int permitted)
{
    BOOL blocking;
    int i, j;
    int dir = barrier_get_direction(d[0], d[1], d[2], d[3]);

    barrier->directions = 0;
    blocking = barrier_is_blocking_direction(barrier, dir);
    assert(blocking);

    for (j = 0; j <= BarrierNegativeY; j++) {
        for (i = 0; i <= BarrierNegativeY; i++) {
            barrier->directions |= 1 << i;
            blocking = barrier_is_blocking_direction(barrier, dir);
            assert((barrier->directions & permitted) ==
                   permitted ? !blocking : blocking);
        }
    }

}

static void
fixes_pointer_barrier_direction_test(void)
{
    struct PointerBarrier barrier;

    int x = 100;
    int y = 100;

    int directions[8][4] = {
        {x, y, x, y + 100},     /* S  */
        {x + 50, y, x - 50, y + 100},   /* SW */
        {x + 100, y, x, y},     /* W  */
        {x + 100, y + 50, x, y - 50},   /* NW */
        {x, y + 100, x, y},     /* N  */
        {x - 50, y + 100, x + 50, y},   /* NE */
        {x, y, x + 100, y},     /* E  */
        {x, y - 50, x + 100, y + 50},   /* SE */
    };

    barrier.x1 = x;
    barrier.x2 = x;
    barrier.y1 = y - 50;
    barrier.y2 = y + 49;

    _fixes_test_direction(&barrier, directions[0], BarrierPositiveY);
    _fixes_test_direction(&barrier, directions[1],
                          BarrierPositiveY | BarrierNegativeX);
    _fixes_test_direction(&barrier, directions[2], BarrierNegativeX);
    _fixes_test_direction(&barrier, directions[3],
                          BarrierNegativeY | BarrierNegativeX);
    _fixes_test_direction(&barrier, directions[4], BarrierNegativeY);
    _fixes_test_direction(&barrier, directions[5],
                          BarrierPositiveX | BarrierNegativeY);
    _fixes_test_direction(&barrier, directions[6], BarrierPositiveX);
    _fixes_test_direction(&barrier, directions[7],
                          BarrierPositiveY | BarrierPositiveX);

}

static void
fixes_pointer_barriers_test(void)
{
    struct PointerBarrier barrier;
    int x1, y1, x2, y2;
    double distance;

    int x = 100;
    int y = 100;

    /* vert barrier */
    barrier.x1 = x;
    barrier.x2 = x;
    barrier.y1 = y - 50;
    barrier.y2 = y + 50;

    /* across at half-way */
    x1 = x + 1;
    x2 = x - 1;
    y1 = y;
    y2 = y;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));
    assert(distance == 1);

    /* definitely not across */
    x1 = x + 10;
    x2 = x + 5;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* across, but outside of y range */
    x1 = x + 1;
    x2 = x - 1;
    y1 = y + 100;
    y2 = y + 100;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* across, diagonally */
    x1 = x + 5;
    x2 = x - 5;
    y1 = y + 5;
    y2 = y - 5;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* across but outside boundary, diagonally */
    x1 = x + 5;
    x2 = x - 5;
    y1 = y + 100;
    y2 = y + 50;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* edge case: startpoint of movement on barrier → blocking */
    x1 = x;
    x2 = x - 1;
    y1 = y;
    y2 = y;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* edge case: startpoint of movement on barrier → not blocking, positive */
    x1 = x;
    x2 = x + 1;
    y1 = y;
    y2 = y;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* edge case: startpoint of movement on barrier → not blocking, negative */
    x1 = x - 1;
    x2 = x - 2;
    y1 = y;
    y2 = y;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* edge case: endpoint of movement on barrier → blocking */
    x1 = x + 1;
    x2 = x;
    y1 = y;
    y2 = y;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* startpoint on barrier but outside y range */
    x1 = x;
    x2 = x - 1;
    y1 = y + 100;
    y2 = y + 100;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* endpoint on barrier but outside y range */
    x1 = x + 1;
    x2 = x;
    y1 = y + 100;
    y2 = y + 100;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* horizontal barrier */
    barrier.x1 = x - 50;
    barrier.x2 = x + 50;
    barrier.y1 = y;
    barrier.y2 = y;

    /* across at half-way */
    x1 = x;
    x2 = x;
    y1 = y - 1;
    y2 = y + 1;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* definitely not across */
    y1 = y + 10;
    y2 = y + 5;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* across, but outside of y range */
    x1 = x + 100;
    x2 = x + 100;
    y1 = y + 1;
    y2 = y - 1;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* across, diagonally */
    y1 = y + 5;
    y2 = y - 5;
    x1 = x + 5;
    x2 = x - 5;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* across but outside boundary, diagonally */
    y1 = y + 5;
    y2 = y - 5;
    x1 = x + 100;
    x2 = x + 50;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* edge case: startpoint of movement on barrier → blocking */
    y1 = y;
    y2 = y - 1;
    x1 = x;
    x2 = x;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* edge case: startpoint of movement on barrier → not blocking, positive */
    y1 = y;
    y2 = y + 1;
    x1 = x;
    x2 = x;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* edge case: startpoint of movement on barrier → not blocking, negative */
    y1 = y - 1;
    y2 = y - 2;
    x1 = x;
    x2 = x;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* edge case: endpoint of movement on barrier → blocking */
    y1 = y + 1;
    y2 = y;
    x1 = x;
    x2 = x;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* startpoint on barrier but outside y range */
    y1 = y;
    y2 = y - 1;
    x1 = x + 100;
    x2 = x + 100;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* endpoint on barrier but outside y range */
    y1 = y + 1;
    y2 = y;
    x1 = x + 100;
    x2 = x + 100;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* ray vert barrier */
    barrier.x1 = x;
    barrier.x2 = x;
    barrier.y1 = -1;
    barrier.y2 = y + 100;

    /* ray barrier simple case */
    y1 = y;
    y2 = y;
    x1 = x + 50;
    x2 = x - 50;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* endpoint outside y range; should be blocked */
    y1 = y - 1000;
    y2 = y - 1000;
    x1 = x + 50;
    x2 = x - 50;
    assert(barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));

    /* endpoint outside y range */
    y1 = y + 150;
    y2 = y + 150;
    x1 = x + 50;
    x2 = x - 50;
    assert(!barrier_is_blocking(&barrier, x1, y1, x2, y2, &distance));
}

static void
fixes_pointer_barrier_clamp_test(void)
{
    struct PointerBarrier barrier;

    int x = 100;
    int y = 100;

    int cx, cy;                 /* clamped */

    /* vert barrier */
    barrier.x1 = x;
    barrier.x2 = x;
    barrier.y1 = y - 50;
    barrier.y2 = y + 49;
    barrier.directions = 0;

    cx = INT_MAX;
    cy = INT_MAX;
    barrier_clamp_to_barrier(&barrier, BarrierPositiveX, &cx, &cy);
    assert(cx == barrier.x1 - 1);
    assert(cy == INT_MAX);

    cx = 0;
    cy = INT_MAX;
    barrier_clamp_to_barrier(&barrier, BarrierNegativeX, &cx, &cy);
    assert(cx == barrier.x1);
    assert(cy == INT_MAX);

    /* horiz barrier */
    barrier.x1 = x - 50;
    barrier.x2 = x + 49;
    barrier.y1 = y;
    barrier.y2 = y;
    barrier.directions = 0;

    cx = INT_MAX;
    cy = INT_MAX;
    barrier_clamp_to_barrier(&barrier, BarrierPositiveY, &cx, &cy);
    assert(cx == INT_MAX);
    assert(cy == barrier.y1 - 1);

    cx = INT_MAX;
    cy = 0;
    barrier_clamp_to_barrier(&barrier, BarrierNegativeY, &cx, &cy);
    assert(cx == INT_MAX);
    assert(cy == barrier.y1);
}

int
fixes_test(void)
{

    fixes_pointer_barriers_test();
    fixes_pointer_barrier_direction_test();
    fixes_pointer_barrier_clamp_test();

    return 0;
}
