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
#include <stdlib.h>

#include "config.h"
#include "khronos_typedefs.h"

#ifdef HAVE_KHRPLATFORM_H

#include <KHR/khrplatform.h>

#define GET_SIZE(type) sizes[type ## _slot] = sizeof(type)

void
get_system_typedef_sizes(uint32_t *sizes)
{
    GET_SIZE(khronos_int8_t);
    GET_SIZE(khronos_uint8_t);
    GET_SIZE(khronos_int16_t);
    GET_SIZE(khronos_uint16_t);
    GET_SIZE(khronos_int32_t);
    GET_SIZE(khronos_uint32_t);
    GET_SIZE(khronos_int64_t);
    GET_SIZE(khronos_uint64_t);
    GET_SIZE(khronos_intptr_t);
    GET_SIZE(khronos_uintptr_t);
    GET_SIZE(khronos_ssize_t);
    GET_SIZE(khronos_usize_t);
    GET_SIZE(khronos_float_t);
    GET_SIZE(khronos_utime_nanoseconds_t);
    GET_SIZE(khronos_stime_nanoseconds_t);
    GET_SIZE(khronos_boolean_enum_t);
}

#else /* !HAVE_KHRPLATFORM_H */

/* Don't care -- this is a conditional case in test code. */
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"

void
get_system_typedef_sizes(uint32_t *sizes)
{
    fputs("./configure failed to find khrplatform.h\n", stderr);
    exit(77);
}

#endif
