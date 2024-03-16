/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef _XF86_RANDR_H_
#define _XF86_RANDR_H_
#include <randrstr.h>
#include <X11/extensions/render.h>

extern _X_EXPORT Bool xf86RandR12CreateScreenResources(ScreenPtr pScreen);
extern _X_EXPORT Bool xf86RandR12Init(ScreenPtr pScreen);
extern _X_EXPORT void xf86RandR12CloseScreen(ScreenPtr pScreen);
extern _X_EXPORT void xf86RandR12SetRotations(ScreenPtr pScreen,
                                              Rotation rotation);
extern _X_EXPORT void xf86RandR12SetTransformSupport(ScreenPtr pScreen,
                                                     Bool transforms);
extern _X_EXPORT Bool xf86RandR12SetConfig(ScreenPtr pScreen, Rotation rotation,
                                           int rate, RRScreenSizePtr pSize);
extern _X_EXPORT Rotation xf86RandR12GetRotation(ScreenPtr pScreen);
extern _X_EXPORT void xf86RandR12GetOriginalVirtualSize(ScrnInfoPtr pScrn,
                                                        int *x, int *y);
extern _X_EXPORT Bool xf86RandR12PreInit(ScrnInfoPtr pScrn);
extern _X_EXPORT void xf86RandR12TellChanged(ScreenPtr pScreen);

extern void xf86RandR12LoadPalette(ScrnInfoPtr pScrn, int numColors,
                                   int *indices, LOCO *colors,
                                   VisualPtr pVisual);
extern Bool xf86RandR12InitGamma(ScrnInfoPtr pScrn, unsigned gammaSize);

#endif                          /* _XF86_RANDR_H_ */
