/*
 * Copyright © 2018 Broadcom
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

/** @file
 *
 * Touch-testing of the damage extension's implementation of various
 * primitives.  The core initializes the pixmap with some contents,
 * turns on damage and each per-primitive test gets to just make a
 * rendering call that draws some pixels.  Afterwards, the core checks
 * what pixels were modified and makes sure the damage report contains
 * them.
 */

/* Test relies on assert() */
#undef NDEBUG

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <xcb/damage.h>

struct test_setup {
    xcb_connection_t *c;
    xcb_drawable_t d;
    xcb_drawable_t start_drawable;
    uint32_t *start_drawable_contents;
    xcb_screen_t *screen;
    xcb_gc_t gc;
    int width, height;
    uint32_t *expected;
    uint32_t *damaged;
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/**
 * Performs a synchronous GetImage for a test pixmap, returning
 * uint32_t per pixel.
 */
static uint32_t *
get_image(struct test_setup *setup, xcb_drawable_t drawable)
{
    xcb_get_image_cookie_t cookie =
        xcb_get_image(setup->c, XCB_IMAGE_FORMAT_Z_PIXMAP, drawable,
                      0, 0, setup->width, setup->height, ~0);
    xcb_get_image_reply_t *reply =
        xcb_get_image_reply(setup->c, cookie, NULL);
    uint8_t *data = xcb_get_image_data(reply);
    int len = xcb_get_image_data_length(reply);

    /* Do I understand X protocol and our test environment? */
    assert(reply->depth == 24);
    assert(len == 4 * setup->width * setup->height);

    uint32_t *result = malloc(sizeof(uint32_t) *
                              setup->width * setup->height);
    memcpy(result, data, len);
    free(reply);

    return result;
}

/**
 * Gets the image drawn by the test and compares it to the starting
 * image, producing a bitmask of what pixels were damaged.
 */
static void
compute_expected_damage(struct test_setup *setup)
{
    uint32_t *results = get_image(setup, setup->d);
    bool any_modified_pixels = false;

    for (int i = 0; i < setup->width * setup->height; i++) {
        if (results[i] != setup->start_drawable_contents[i]) {
            setup->expected[i / 32] |= 1 << (i % 32);
            any_modified_pixels = true;
        }
    }

    /* Make sure that the testcases actually render something! */
    assert(any_modified_pixels);
}

/**
 * Processes a damage event, filling in the bitmask of pixels reported
 * to be damaged.
 */
static bool
damage_event_handle(struct test_setup *setup,
                    struct xcb_damage_notify_event_t *event)
{
    xcb_rectangle_t *rect = &event->area;
    assert(event->drawable == setup->d);
    for (int y = rect->y; y < rect->y + rect->height; y++) {
        for (int x = rect->x; x < rect->x + rect->width; x++) {
            int bit = y * setup->width + x;
            setup->damaged[bit / 32] |= 1 << (bit % 32);
        }
    }

    return event->level & 0x80; /* XXX: MORE is missing from xcb. */
}

/**
 * Collects a series of damage events (while the MORE flag is set)
 * into the damaged bitmask.
 */
static void
collect_damage(struct test_setup *setup)
{
    const xcb_query_extension_reply_t *ext =
        xcb_get_extension_data(setup->c, &xcb_damage_id);
    xcb_generic_event_t *ge;

    xcb_flush(setup->c);
    while ((ge = xcb_wait_for_event(setup->c))) {
        int event = ge->response_type & ~0x80;

        if (event == (ext->first_event + XCB_DAMAGE_NOTIFY)) {
            if (!damage_event_handle(setup,
                                     (struct xcb_damage_notify_event_t *)ge)) {
                return;
            }
        } else {
            switch (ge->response_type) {
            case 0: {
                xcb_generic_error_t *error = (xcb_generic_error_t *)ge;
                fprintf(stderr, "X error %d at sequence %d\n",
                        error->error_code, error->sequence);
                exit(1);
            }

            case XCB_GRAPHICS_EXPOSURE:
            case XCB_NO_EXPOSURE:
                break;

            default:
                fprintf(stderr, "Unexpected event %d\n", ge->response_type);
                exit(1);
            }
        }
    }

    fprintf(stderr, "I/O error\n");
    exit(1);
}

/**
 * Wrapper to set up the test pixmap, attach damage to it, and see if
 * the reported damage matches the testcase's rendering.
 */
static bool
damage_test(struct test_setup *setup,
            void (*test)(struct test_setup *setup),
            const char *name)
{
    uint32_t expected[32] = { 0 };
    uint32_t damaged[32] = { 0 };

    printf("Testing %s\n", name);

    setup->expected = expected;
    setup->damaged = damaged;

    /* Create our pixmap for this test and fill it with the
     * starting image.
     */
    setup->d = xcb_generate_id(setup->c);
    xcb_create_pixmap(setup->c, setup->screen->root_depth,
                      setup->d, setup->screen->root,
                      setup->width, setup->height);

    setup->gc = xcb_generate_id(setup->c);
    uint32_t values[]  = { setup->screen->black_pixel };
    xcb_create_gc(setup->c, setup->gc, setup->screen->root,
                  XCB_GC_FOREGROUND, values);

    xcb_copy_area(setup->c,
                  setup->start_drawable,
                  setup->d,
                  setup->gc,
                  0, 0,
                  0, 0,
                  setup->width, setup->height);

    /* Start watching for damage now that we have the initial contents. */
    xcb_damage_damage_t damage = xcb_generate_id(setup->c);
    xcb_damage_create(setup->c, damage, setup->d,
                      XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);

    test(setup);

    compute_expected_damage(setup);

    xcb_damage_destroy(setup->c, damage);
    xcb_free_gc(setup->c, setup->gc);
    xcb_free_pixmap(setup->c, setup->d);
    collect_damage(setup);

    for (int bit = 0; bit < setup->width * setup->height; bit++) {
        if ((expected[bit / 32] & (1 << bit %  32)) &&
            !(damaged[bit / 32] & (1 << bit %  32))) {
            fprintf(stderr, "  fail: %s(): Damage report missed %d, %d\n",
                    name, bit % setup->width, bit / setup->width);
            return false;
        }
    }

    return true;
}

/**
 * Creates the pixmap of contents that will be the initial state of
 * each test's drawable.
 */
static void
create_start_pixmap(struct test_setup *setup)
{
    setup->start_drawable = xcb_generate_id(setup->c);
    xcb_create_pixmap(setup->c, setup->screen->root_depth,
                      setup->start_drawable, setup->screen->root,
                      setup->width, setup->height);

    /* Fill pixmap so it has defined contents */
    xcb_gc_t fill = xcb_generate_id(setup->c);
    uint32_t fill_values[]  = { setup->screen->white_pixel };
    xcb_create_gc(setup->c, fill, setup->screen->root,
                  XCB_GC_FOREGROUND, fill_values);

    xcb_rectangle_t rect_all = { 0, 0, setup->width, setup->height};
    xcb_poly_fill_rectangle(setup->c, setup->start_drawable,
                            fill, 1, &rect_all);
    xcb_free_gc(setup->c, fill);

    /* Draw a rectangle */
    xcb_gc_t gc = xcb_generate_id(setup->c);
    uint32_t values[]  = { 0xaaaaaaaa };
    xcb_create_gc(setup->c, gc, setup->screen->root,
                  XCB_GC_FOREGROUND, values);

    xcb_rectangle_t rect = { 5, 5, 10, 15 };
    xcb_poly_rectangle(setup->c, setup->start_drawable, gc, 1, &rect);

    xcb_free_gc(setup->c, gc);

    /* Capture the rendered start contents once, for comparing each
     * test's rendering output to the start contents.
     */
    setup->start_drawable_contents = get_image(setup, setup->start_drawable);
}

static void
test_poly_point_origin(struct test_setup *setup)
{
    struct xcb_point_t points[] = { {1, 2}, {3, 4} };
    xcb_poly_point(setup->c, XCB_COORD_MODE_ORIGIN, setup->d, setup->gc,
                   ARRAY_SIZE(points), points);
}

static void
test_poly_point_previous(struct test_setup *setup)
{
    struct xcb_point_t points[] = { {1, 2}, {3, 4} };
    xcb_poly_point(setup->c, XCB_COORD_MODE_PREVIOUS, setup->d, setup->gc,
                   ARRAY_SIZE(points), points);
}

static void
test_poly_line_origin(struct test_setup *setup)
{
    struct xcb_point_t points[] = { {1, 2}, {3, 4}, {5, 6} };
    xcb_poly_line(setup->c, XCB_COORD_MODE_ORIGIN, setup->d, setup->gc,
                   ARRAY_SIZE(points), points);
}

static void
test_poly_line_previous(struct test_setup *setup)
{
    struct xcb_point_t points[] = { {1, 2}, {3, 4}, {5, 6} };
    xcb_poly_line(setup->c, XCB_COORD_MODE_PREVIOUS, setup->d, setup->gc,
                   ARRAY_SIZE(points), points);
}

static void
test_poly_fill_rectangle(struct test_setup *setup)
{
    struct xcb_rectangle_t rects[] = { {1, 2, 3, 4},
                                       {5, 6, 7, 8} };
    xcb_poly_fill_rectangle(setup->c, setup->d, setup->gc,
                   ARRAY_SIZE(rects), rects);
}

static void
test_poly_rectangle(struct test_setup *setup)
{
    struct xcb_rectangle_t rects[] = { {1, 2, 3, 4},
                                       {5, 6, 7, 8} };
    xcb_poly_rectangle(setup->c, setup->d, setup->gc,
                       ARRAY_SIZE(rects), rects);
}

static void
test_poly_segment(struct test_setup *setup)
{
    struct xcb_segment_t segs[] = { {1, 2, 3, 4},
                                    {5, 6, 7, 8} };
    xcb_poly_segment(setup->c, setup->d, setup->gc,
                     ARRAY_SIZE(segs), segs);
}

int main(int argc, char **argv)
{
    int screen;
    xcb_connection_t *c = xcb_connect(NULL, &screen);
    const xcb_query_extension_reply_t *ext =
        xcb_get_extension_data(c, &xcb_damage_id);

    if (!ext->present) {
        printf("No XDamage present\n");
        exit(77);
    }

    struct test_setup setup = {
        .c = c,
        .width = 32,
        .height = 32,
    };

    /* Get the screen so we have the root window. */
    xcb_screen_iterator_t iter;
    iter = xcb_setup_roots_iterator (xcb_get_setup (c));
    setup.screen = iter.data;

    xcb_damage_query_version(c, 1, 1);

    create_start_pixmap(&setup);

    bool pass = true;
#define test(x) pass = damage_test(&setup, x, #x) && pass;

    test(test_poly_point_origin);
    test(test_poly_point_previous);
    test(test_poly_line_origin);
    test(test_poly_line_previous);
    test(test_poly_fill_rectangle);
    test(test_poly_rectangle);
    test(test_poly_segment);

    xcb_disconnect(c);
    exit(pass ? 0 : 1);
}
