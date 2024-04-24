/* Copyright (c) 2005-2006, Oracle and/or its affiliates. All rights reserved.
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

/*
 * Xserver dtrace provider definition
 */
#ifdef __APPLE__
#define string char *
#define pid_t uint32_t
#define zoneid_t uint32_t
#elif defined(__FreeBSD__)
#define zoneid_t id_t
#else
#include <sys/types.h>
#endif

typedef const uint8_t *const_uint8_p;
typedef const double *const_double_p;

provider Xserver {
	/* reqType, data, length, client id, request buffer */
	probe request__start(string, uint8_t, uint16_t, int, void *);
	/* reqType, data, sequence, client id, result */
	probe request__done(string, uint8_t, uint32_t, int, int);
	/* client id, client fd */
	probe client__connect(int, int);
	/* client id, client address, client pid, client zone id */
	probe client__auth(int, string, pid_t, zoneid_t);
	/* client id */
	probe client__disconnect(int);
	/* resource id, resource type, value, resource type name */
	probe resource__alloc(uint32_t, uint32_t, void *, string);
	/* resource id, resource type, value, resource type name */
	probe resource__free(uint32_t, uint32_t, void *, string);
	/* client id, event type, event* */
	probe send__event(int, uint8_t, void *);
	/* deviceid, type, button/keycode/touchid, flags, nvalues, mask, values */
	probe input__event(int, int, uint32_t, uint32_t, int8_t, const_uint8_p, const_double_p);
};

#pragma D attributes Unstable/Unstable/Common provider Xserver provider
#pragma D attributes Private/Private/Unknown  provider Xserver module
#pragma D attributes Private/Private/Unknown  provider Xserver function
#pragma D attributes Unstable/Unstable/Common provider Xserver name
#pragma D attributes Unstable/Unstable/Common provider Xserver args

