/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * ephyr_glamor_glx.h
 *
 * Prototypes exposed by ephyr_glamor_glx.c, without including any
 * server headers.
 */

#include <xcb/xcb.h>
#include "dix-config.h"

struct ephyr_glamor;
struct pixman_region16;

xcb_connection_t *
ephyr_glamor_connect(void);

void
ephyr_glamor_set_texture(struct ephyr_glamor *ephyr_glamor, uint32_t tex);

xcb_visualtype_t *
ephyr_glamor_get_visual(void);

struct ephyr_glamor *
ephyr_glamor_glx_screen_init(xcb_window_t win);

void
ephyr_glamor_glx_screen_fini(struct ephyr_glamor *glamor);

#ifdef GLAMOR
void
ephyr_glamor_set_window_size(struct ephyr_glamor *glamor,
                             unsigned width, unsigned height);

void
ephyr_glamor_damage_redisplay(struct ephyr_glamor *glamor,
                              struct pixman_region16 *damage);

void
ephyr_glamor_process_event(xcb_generic_event_t *xev);

#else /* !GLAMOR */

static inline void
ephyr_glamor_set_window_size(struct ephyr_glamor *glamor,
                             unsigned width, unsigned height)
{
}

static inline void
ephyr_glamor_damage_redisplay(struct ephyr_glamor *glamor,
                              struct pixman_region16 *damage)
{
}

static inline void
ephyr_glamor_process_event(xcb_generic_event_t *xev)
{
}

#endif /* !GLAMOR */
