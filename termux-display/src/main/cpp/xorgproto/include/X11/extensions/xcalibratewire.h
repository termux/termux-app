/*
 * Copyright © 2003 Philip Blundell
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Philip Blundell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Philip Blundell makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * PHILIP BLUNDELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL PHILIP BLUNDELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef XCALIBRATEWIRE_H
#define XCALIBRATEWIRE_H

#define XCALIBRATE_MAJOR_VERSION 0
#define XCALIBRATE_MINOR_VERSION 1
#define XCALIBRATE_NAME "XCALIBRATE"

#define X_XCalibrateQueryVersion		0
#define X_XCalibrateRawMode			1
#define X_XCalibrateScreenToCoord		2

#define XCalibrateNumberRequests		(X_XCalibrateScreenToCoord + 1)

#define X_XCalibrateRawTouchscreen		0

#define XCalibrateNumberEvents			(X_XCalibrateRawTouchscreen + 1)

#define XCalibrateNumberErrors			0

#endif
