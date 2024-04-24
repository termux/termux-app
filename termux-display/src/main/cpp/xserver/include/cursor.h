/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef CURSOR_H
#define CURSOR_H

#include "misc.h"
#include "screenint.h"
#include "window.h"
#include "privates.h"

#define NullCursor ((CursorPtr)NULL)

/* Provide support for alpha composited cursors */
#define ARGB_CURSOR

struct _DeviceIntRec;

typedef struct _Cursor *CursorPtr;
typedef struct _CursorMetric *CursorMetricPtr;

extern _X_EXPORT DevScreenPrivateKeyRec cursorScreenDevPriv;

#define CursorScreenKey (&cursorScreenDevPriv)

extern _X_EXPORT CursorPtr rootCursor;

extern _X_EXPORT int FreeCursor(void *pCurs,
                                XID cid);

extern _X_EXPORT CursorPtr RefCursor(CursorPtr /* cursor */);
extern _X_EXPORT CursorPtr UnrefCursor(CursorPtr /* cursor */);
extern _X_EXPORT int CursorRefCount(const CursorPtr /* cursor */);

extern _X_EXPORT int AllocARGBCursor(unsigned char * /*psrcbits */ ,
                                     unsigned char * /*pmaskbits */ ,
                                     CARD32 * /*argb */ ,
                                     CursorMetricPtr /*cm */ ,
                                     unsigned /*foreRed */ ,
                                     unsigned /*foreGreen */ ,
                                     unsigned /*foreBlue */ ,
                                     unsigned /*backRed */ ,
                                     unsigned /*backGreen */ ,
                                     unsigned /*backBlue */ ,
                                     CursorPtr * /*ppCurs */ ,
                                     ClientPtr /*client */ ,
                                     XID /*cid */ );

extern _X_EXPORT int AllocGlyphCursor(Font /*source */ ,
                                      unsigned int /*sourceChar */ ,
                                      Font /*mask */ ,
                                      unsigned int /*maskChar */ ,
                                      unsigned /*foreRed */ ,
                                      unsigned /*foreGreen */ ,
                                      unsigned /*foreBlue */ ,
                                      unsigned /*backRed */ ,
                                      unsigned /*backGreen */ ,
                                      unsigned /*backBlue */ ,
                                      CursorPtr * /*ppCurs */ ,
                                      ClientPtr /*client */ ,
                                      XID /*cid */ );

extern _X_EXPORT CursorPtr CreateRootCursor(char * /*pfilename */ ,
                                            unsigned int /*glyph */ );

extern _X_EXPORT int ServerBitsFromGlyph(FontPtr /*pfont */ ,
                                         unsigned int /*ch */ ,
                                         CursorMetricPtr /*cm */ ,
                                         unsigned char ** /*ppbits */ );

extern _X_EXPORT Bool CursorMetricsFromGlyph(FontPtr /*pfont */ ,
                                             unsigned /*ch */ ,
                                             CursorMetricPtr /*cm */ );

extern _X_EXPORT void CheckCursorConfinement(WindowPtr /*pWin */ );

extern _X_EXPORT void NewCurrentScreen(struct _DeviceIntRec * /*pDev */ ,
                                       ScreenPtr /*newScreen */ ,
                                       int /*x */ ,
                                       int /*y */ );

extern _X_EXPORT Bool PointerConfinedToScreen(struct _DeviceIntRec * /* pDev */
                                              );

extern _X_EXPORT void GetSpritePosition(struct _DeviceIntRec * /* pDev */ ,
                                        int * /*px */ ,
                                        int * /*py */ );

#ifdef PANORAMIX
extern _X_EXPORT int XineramaGetCursorScreen(struct _DeviceIntRec *pDev);
#endif                          /* PANORAMIX */

#endif                          /* CURSOR_H */
