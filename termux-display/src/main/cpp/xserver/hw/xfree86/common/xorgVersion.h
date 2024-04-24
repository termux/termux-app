
/*
 * Copyright (c) 2004, X.Org Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifndef XORG_VERSION_H
#define XORG_VERSION_H

#ifndef XORG_VERSION_CURRENT
#error
#endif

#define XORG_VERSION_NUMERIC(major,minor,patch,snap,dummy) \
	(((major) * 10000000) + ((minor) * 100000) + ((patch) * 1000) + snap)

#define XORG_GET_MAJOR_VERSION(vers)	((vers) / 10000000)
#define XORG_GET_MINOR_VERSION(vers)	(((vers) % 10000000) / 100000)
#define XORG_GET_PATCH_VERSION(vers)	(((vers) % 100000) / 1000)
#define XORG_GET_SNAP_VERSION(vers)	((vers) % 1000)

#define XORG_VERSION_MAJOR	XORG_GET_MAJOR_VERSION(XORG_VERSION_CURRENT)
#define XORG_VERSION_MINOR	XORG_GET_MINOR_VERSION(XORG_VERSION_CURRENT)
#define XORG_VERSION_PATCH	XORG_GET_PATCH_VERSION(XORG_VERSION_CURRENT)
#define XORG_VERSION_SNAP	XORG_GET_SNAP_VERSION(XORG_VERSION_CURRENT)

#endif
