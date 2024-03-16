/******************************************************************************
 * Copyright (c) 1994, 1995  Hewlett-Packard Company
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Hewlett-Packard
 * Company shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the Hewlett-Packard Company.
 *
 *     Header file for users of machine-independent DBE code
 *
 *****************************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef MIDBE_H
#define MIDBE_H

#include "privates.h"

/* EXTERNS */

extern Bool miDbeInit(ScreenPtr pScreen, DbeScreenPrivPtr pDbeScreenPriv);

extern DevPrivateKeyRec dbeScreenPrivKeyRec;

#define dbeScreenPrivKey (&dbeScreenPrivKeyRec)

extern DevPrivateKeyRec dbeWindowPrivKeyRec;

#define dbeWindowPrivKey (&dbeWindowPrivKeyRec)

extern RESTYPE dbeDrawableResType;
extern RESTYPE dbeWindowPrivResType;

#endif                          /* MIDBE_H */
