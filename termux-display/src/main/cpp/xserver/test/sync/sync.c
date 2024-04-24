/*
 * Copyright © 2017 Broadcom
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <xcb/sync.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static const int64_t some_values[] = {
        0,
        1,
        -1,
        LLONG_MAX,
        LLONG_MIN,
};

static int64_t
pack_sync_value(xcb_sync_int64_t val)
{
    return ((int64_t)val.hi << 32) | val.lo;
}

static int64_t
counter_value(struct xcb_connection_t *c,
              xcb_sync_query_counter_cookie_t cookie)
{
    xcb_sync_query_counter_reply_t *reply =
        xcb_sync_query_counter_reply(c, cookie, NULL);
    int64_t value = pack_sync_value(reply->counter_value);

    free(reply);
    return value;
}

static xcb_sync_int64_t
sync_value(int64_t value)
{
    xcb_sync_int64_t v = {
        .hi = value >> 32,
        .lo = value,
    };

    return v;
}

/* Initializes counters with a bunch of interesting values and makes
 * sure it comes back the same.
 */
static void
test_create_counter(xcb_connection_t *c)
{
    xcb_sync_query_counter_cookie_t queries[ARRAY_SIZE(some_values)];

    for (int i = 0; i < ARRAY_SIZE(some_values); i++) {
        xcb_sync_counter_t counter = xcb_generate_id(c);
        xcb_sync_create_counter(c, counter, sync_value(some_values[i]));
        queries[i] = xcb_sync_query_counter_unchecked(c, counter);
    }

    for (int i = 0; i < ARRAY_SIZE(some_values); i++) {
        int64_t value = counter_value(c, queries[i]);

        if (value != some_values[i]) {
            fprintf(stderr, "Creating counter with %lld returned %lld\n",
                    (long long)some_values[i],
                    (long long)value);
            exit(1);
        }
    }
}

/* Set a single counter to a bunch of interesting values and make sure
 * it comes the same.
 */
static void
test_set_counter(xcb_connection_t *c)
{
    xcb_sync_counter_t counter = xcb_generate_id(c);
    xcb_sync_query_counter_cookie_t queries[ARRAY_SIZE(some_values)];

    xcb_sync_create_counter(c, counter, sync_value(0));

    for (int i = 0; i < ARRAY_SIZE(some_values); i++) {
        xcb_sync_set_counter(c, counter, sync_value(some_values[i]));
        queries[i] = xcb_sync_query_counter_unchecked(c, counter);
    }

    for (int i = 0; i < ARRAY_SIZE(some_values); i++) {
        int64_t value = counter_value(c, queries[i]);

        if (value != some_values[i]) {
            fprintf(stderr, "Setting counter to %lld returned %lld\n",
                    (long long)some_values[i],
                    (long long)value);
            exit(1);
        }
    }
}

/* Add [0, 1, 2, 3] to a counter and check that the values stick. */
static void
test_change_counter_basic(xcb_connection_t *c)
{
    int iterations = 4;
    xcb_sync_query_counter_cookie_t queries[iterations];

    xcb_sync_counter_t counter = xcb_generate_id(c);
    xcb_sync_create_counter(c, counter, sync_value(0));

    for (int i = 0; i < iterations; i++) {
        xcb_sync_change_counter(c, counter, sync_value(i));
        queries[i] = xcb_sync_query_counter_unchecked(c, counter);
    }

    int64_t expected_value = 0;
    for (int i = 0; i < iterations; i++) {
        expected_value += i;
        int64_t value = counter_value(c, queries[i]);

        if (value != expected_value) {
            fprintf(stderr, "Adding %d to counter expected %lld returned %lld\n",
                    i,
                    (long long)expected_value,
                    (long long)value);
            exit(1);
        }
    }
}

/* Test change_counter where we trigger an integer overflow. */
static void
test_change_counter_overflow(xcb_connection_t *c)
{
    int iterations = 4;
    xcb_sync_query_counter_cookie_t queries[iterations];
    xcb_void_cookie_t changes[iterations];
    static const struct {
        int64_t a, b;
    } overflow_args[] = {
        { LLONG_MAX, 1 },
        { LLONG_MAX, LLONG_MAX },
        { LLONG_MIN, -1 },
        { LLONG_MIN, LLONG_MIN },
    };

    xcb_sync_counter_t counter = xcb_generate_id(c);
    xcb_sync_create_counter(c, counter, sync_value(0));

    for (int i = 0; i < ARRAY_SIZE(overflow_args); i++) {
        int64_t a = overflow_args[i].a;
        int64_t b = overflow_args[i].b;
        xcb_sync_set_counter(c, counter, sync_value(a));
        changes[i] = xcb_sync_change_counter_checked(c, counter,
                                                     sync_value(b));
        queries[i] = xcb_sync_query_counter(c, counter);
    }

    for (int i = 0; i < ARRAY_SIZE(overflow_args); i++) {
        int64_t a = overflow_args[i].a;
        int64_t b = overflow_args[i].b;
        xcb_sync_query_counter_reply_t *reply =
            xcb_sync_query_counter_reply(c, queries[i], NULL);
        int64_t value = (((int64_t)reply->counter_value.hi << 32) |
                         reply->counter_value.lo);
        int64_t expected_value = a;

        /* The change_counter should have thrown BadValue */
        xcb_generic_error_t *e = xcb_request_check(c, changes[i]);
        if (!e) {
            fprintf(stderr, "(%lld + %lld) failed to return an error\n",
                    (long long)a,
                    (long long)b);
            exit(1);
        }

        if (e->error_code != XCB_VALUE) {
            fprintf(stderr, "(%lld + %lld) returned %d, not BadValue\n",
                    (long long)a,
                    (long long)b,
                    e->error_code);
            exit(1);
        }

        /* The change_counter should have had no other effect if it
         * errored out.
         */
        if (value != expected_value) {
            fprintf(stderr, "(%lld + %lld) expected %lld returned %lld\n",
                    (long long)a,
                    (long long)b,
                    (long long)expected_value,
                    (long long)value);
            exit(1);
        }

        free(e);
        free(reply);
    }
}

static void
test_change_alarm_value(xcb_connection_t *c)
{
    xcb_sync_alarm_t alarm = xcb_generate_id(c);
    xcb_sync_query_alarm_cookie_t queries[ARRAY_SIZE(some_values)];

    xcb_sync_create_alarm(c, alarm, 0, NULL);

    for (int i = 0; i < ARRAY_SIZE(some_values); i++) {
        uint32_t values[] = { some_values[i] >> 32, some_values[i] };

        xcb_sync_change_alarm(c, alarm, XCB_SYNC_CA_VALUE, values);
        queries[i] = xcb_sync_query_alarm_unchecked(c, alarm);
    }

    for (int i = 0; i < ARRAY_SIZE(some_values); i++) {
        xcb_sync_query_alarm_reply_t *reply =
            xcb_sync_query_alarm_reply(c, queries[i], NULL);
        int64_t value = pack_sync_value(reply->trigger.wait_value);

        if (value != some_values[i]) {
            fprintf(stderr, "Setting alarm value to %lld returned %lld\n",
                    (long long)some_values[i],
                    (long long)value);
            exit(1);
        }
        free(reply);
    }
}

static void
test_change_alarm_delta(xcb_connection_t *c)
{
    xcb_sync_alarm_t alarm = xcb_generate_id(c);
    xcb_sync_query_alarm_cookie_t queries[ARRAY_SIZE(some_values)];

    xcb_sync_create_alarm(c, alarm, 0, NULL);

    for (int i = 0; i < ARRAY_SIZE(some_values); i++) {
        uint32_t values[] = { some_values[i] >> 32, some_values[i] };

        xcb_sync_change_alarm(c, alarm, XCB_SYNC_CA_DELTA, values);
        queries[i] = xcb_sync_query_alarm_unchecked(c, alarm);
    }

    for (int i = 0; i < ARRAY_SIZE(some_values); i++) {
        xcb_sync_query_alarm_reply_t *reply =
            xcb_sync_query_alarm_reply(c, queries[i], NULL);
        int64_t value = pack_sync_value(reply->delta);

        if (value != some_values[i]) {
            fprintf(stderr, "Setting alarm delta to %lld returned %lld\n",
                    (long long)some_values[i],
                    (long long)value);
            exit(1);
        }
        free(reply);
    }
}

int main(int argc, char **argv)
{
    int screen;
    xcb_connection_t *c = xcb_connect(NULL, &screen);
    const xcb_query_extension_reply_t *ext = xcb_get_extension_data(c, &xcb_sync_id);

    if (!ext->present) {
        printf("No XSync present\n");
        exit(77);
    }

    test_create_counter(c);
    test_set_counter(c);
    test_change_counter_basic(c);
    test_change_counter_overflow(c);
    test_change_alarm_value(c);
    test_change_alarm_delta(c);

    xcb_disconnect(c);
    exit(0);
}
