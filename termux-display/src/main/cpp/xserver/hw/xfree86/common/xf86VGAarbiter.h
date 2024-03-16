/*
 * Copyright (c) 2009 Tiago Vignatti
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __XF86VGAARBITER_H
#define __XF86VGAARBITER_H

#include "screenint.h"
#include "misc.h"
#include "xf86.h"

/* Functions */
extern void xf86VGAarbiterInit(void);
extern void xf86VGAarbiterFini(void);
void xf86VGAarbiterScrnInit(ScrnInfoPtr pScrn);
extern Bool xf86VGAarbiterWrapFunctions(void);
extern void xf86VGAarbiterLock(ScrnInfoPtr pScrn);
extern void xf86VGAarbiterUnlock(ScrnInfoPtr pScrn);

/* allow a driver to remove itself from arbiter - really should be
 * done in the kernel though */
extern _X_EXPORT void xf86VGAarbiterDeviceDecodes(ScrnInfoPtr pScrn, int rsrc);

/* DRI and arbiter are really not possible together,
 * you really want to remove the card from arbitration if you can */
extern _X_EXPORT Bool xf86VGAarbiterAllowDRI(ScreenPtr pScreen);

#endif                          /* __XF86VGAARBITER_H */
