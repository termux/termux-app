/*
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <X11/Xdmcp.h>
#include <inttypes.h>

/* Test what happens if you try to allocate an array with too many entries */
#define TestAllocOversize(type, len) {                          \
    type newArray = { -1, (void *) -1 };                        \
    int result;                                                 \
    printf("Checking XdmcpAlloc%s(%d)...\n", #type, len);       \
    result = XdmcpAlloc##type(&newArray, len)    ;              \
    assert(result == FALSE);                                    \
    assert(newArray.length == 0);                               \
    assert(newArray.data == NULL);                              \
    printf("Checking XdmcpRealloc%s(%d)...\n", #type, len);     \
    result = XdmcpRealloc##type(&newArray, len);                \
    assert(result == FALSE);                                    \
    assert(newArray.length == 0);                               \
    assert(newArray.data == NULL);                              \
    XdmcpDispose##type(&newArray);                              \
}

static void
TestAllocOversizeArrays(void)
{
    TestAllocOversize(ARRAY8, UINT16_MAX + 1);
    TestAllocOversize(ARRAY16, UINT8_MAX + 1);
    TestAllocOversize(ARRAY32, UINT8_MAX + 1);
    TestAllocOversize(ARRAYofARRAY8, UINT8_MAX + 1);
    TestAllocOversize(ARRAY8, -1);
    TestAllocOversize(ARRAY16, -1);
    TestAllocOversize(ARRAY32, -1);
    TestAllocOversize(ARRAYofARRAY8, -1);
}

static void
TestZeroFillARRAYofARRAY8(void)
{
    ARRAYofARRAY8 aa;
    int result;
    char *noise;

    printf("Checking XdmcpAllocARRAYofARRAY8 zero fills array...\n");
    /* prefill memory with junk - hopefully next malloc will pick up some */
    noise = malloc(32 * sizeof(ARRAY8));
    memset(noise, 0xdeadbeef, 32 * sizeof(ARRAY8));
    free(noise);
    result = XdmcpAllocARRAYofARRAY8(&aa, 32);
    assert(result == TRUE);
    assert(aa.length == 32);
    assert(aa.data[4].data == NULL);
    printf("Checking XdmcpReallocARRAYofARRAY8 zero fills array...\n");
    result = XdmcpAllocARRAYofARRAY8(&aa, 48);
    assert(result == TRUE);
    assert(aa.length == 48);
    assert(aa.data[40].data == NULL);
    XdmcpDisposeARRAYofARRAY8(&aa);
}

int
main(int argc, char **argv)
{
    TestAllocOversizeArrays();
    TestZeroFillARRAYofARRAY8();

    exit(0);
}
