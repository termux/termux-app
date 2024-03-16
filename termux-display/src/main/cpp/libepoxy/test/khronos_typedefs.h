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

#include <stdint.h>

enum typedef_slot {
    khronos_int8_t_slot,
    khronos_uint8_t_slot,
    khronos_int16_t_slot,
    khronos_uint16_t_slot,
    khronos_int32_t_slot,
    khronos_uint32_t_slot,
    khronos_int64_t_slot,
    khronos_uint64_t_slot,
    khronos_intptr_t_slot,
    khronos_uintptr_t_slot,
    khronos_ssize_t_slot,
    khronos_usize_t_slot,
    khronos_float_t_slot,
    /* khrplatform.h claims it defines khronos_time_ns_t, but it doesn't. */
    khronos_utime_nanoseconds_t_slot,
    khronos_stime_nanoseconds_t_slot,
    khronos_boolean_enum_t_slot,
    khronos_typedef_count
};

void get_system_typedef_sizes(uint32_t *sizes);
