/*
 * Copyright © 2014 Intel Corporation
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

#include <stdio.h>
#include "khronos_typedefs.h"
#include "epoxy/gl.h"

#define COMPARE_SIZE(type)                                              \
    do {                                                                \
        if (sizeof(type) != system_sizes[type ## _slot]) {              \
            fprintf(stderr, "system %s is size %d, epoxy is %d\n",      \
                    #type,                                              \
                    (int)system_sizes[type ## _slot],                   \
                    (int)sizeof(type));                                 \
            error = true;                                               \
        }                                                               \
} while (0)

int
main(int argc, char **argv)
{
    uint32_t system_sizes[khronos_typedef_count];
    bool error = false;

    get_system_typedef_sizes(system_sizes);

    COMPARE_SIZE(khronos_int8_t);
    COMPARE_SIZE(khronos_uint8_t);
    COMPARE_SIZE(khronos_int16_t);
    COMPARE_SIZE(khronos_uint16_t);
    COMPARE_SIZE(khronos_int32_t);
    COMPARE_SIZE(khronos_uint32_t);
    COMPARE_SIZE(khronos_int64_t);
    COMPARE_SIZE(khronos_uint64_t);
    COMPARE_SIZE(khronos_intptr_t);
    COMPARE_SIZE(khronos_uintptr_t);
    COMPARE_SIZE(khronos_ssize_t);
    COMPARE_SIZE(khronos_usize_t);
    COMPARE_SIZE(khronos_float_t);
    COMPARE_SIZE(khronos_utime_nanoseconds_t);
    COMPARE_SIZE(khronos_stime_nanoseconds_t);
    COMPARE_SIZE(khronos_boolean_enum_t);

    return error;
}
