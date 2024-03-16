/*
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "pixman-private.h"

#ifdef PIXMAN_TIMERS

static pixman_timer_t *timers;

static void
dump_timers (void)
{
    pixman_timer_t *timer;

    for (timer = timers; timer != NULL; timer = timer->next)
    {
	printf ("%s:   total: %llu     n: %llu      avg: %f\n",
	        timer->name,
	        timer->total,
	        timer->n_times,
	        timer->total / (double)timer->n_times);
    }
}

void
pixman_timer_register (pixman_timer_t *timer)
{
    static int initialized;

    int atexit (void (*function)(void));

    if (!initialized)
    {
	atexit (dump_timers);
	initialized = 1;
    }

    timer->next = timers;
    timers = timer;
}

#endif
