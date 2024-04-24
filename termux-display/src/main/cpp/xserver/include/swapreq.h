/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef SWAPREQ_H
#define SWAPREQ_H 1

extern void SwapColorItem(xColorItem * /* pItem */ );

extern void SwapConnClientPrefix(xConnClientPrefix * /* pCCP */ );

#undef SWAPREQ_PROC

#define SWAPREQ_PROC(func) extern int func(ClientPtr /* client */)

SWAPREQ_PROC(SProcAllocColor);
SWAPREQ_PROC(SProcAllocColorCells);
SWAPREQ_PROC(SProcAllocColorPlanes);
SWAPREQ_PROC(SProcAllocNamedColor);
SWAPREQ_PROC(SProcChangeActivePointerGrab);
SWAPREQ_PROC(SProcChangeGC);
SWAPREQ_PROC(SProcChangeHosts);
SWAPREQ_PROC(SProcChangeKeyboardControl);
SWAPREQ_PROC(SProcChangeKeyboardMapping);
SWAPREQ_PROC(SProcChangePointerControl);
SWAPREQ_PROC(SProcChangeProperty);
SWAPREQ_PROC(SProcChangeWindowAttributes);
SWAPREQ_PROC(SProcClearToBackground);
SWAPREQ_PROC(SProcConfigureWindow);
SWAPREQ_PROC(SProcConvertSelection);
SWAPREQ_PROC(SProcCopyArea);
SWAPREQ_PROC(SProcCopyColormapAndFree);
SWAPREQ_PROC(SProcCopyGC);
SWAPREQ_PROC(SProcCopyPlane);
SWAPREQ_PROC(SProcCreateColormap);
SWAPREQ_PROC(SProcCreateCursor);
SWAPREQ_PROC(SProcCreateGC);
SWAPREQ_PROC(SProcCreateGlyphCursor);
SWAPREQ_PROC(SProcCreatePixmap);
SWAPREQ_PROC(SProcCreateWindow);
SWAPREQ_PROC(SProcDeleteProperty);
SWAPREQ_PROC(SProcFillPoly);
SWAPREQ_PROC(SProcFreeColors);
SWAPREQ_PROC(SProcGetImage);
SWAPREQ_PROC(SProcGetMotionEvents);
SWAPREQ_PROC(SProcGetProperty);
SWAPREQ_PROC(SProcGrabButton);
SWAPREQ_PROC(SProcGrabKey);
SWAPREQ_PROC(SProcGrabKeyboard);
SWAPREQ_PROC(SProcGrabPointer);
SWAPREQ_PROC(SProcImageText);
SWAPREQ_PROC(SProcInternAtom);
SWAPREQ_PROC(SProcListFonts);
SWAPREQ_PROC(SProcListFontsWithInfo);
SWAPREQ_PROC(SProcLookupColor);
SWAPREQ_PROC(SProcNoOperation);
SWAPREQ_PROC(SProcOpenFont);
SWAPREQ_PROC(SProcPoly);
SWAPREQ_PROC(SProcPolyText);
SWAPREQ_PROC(SProcPutImage);
SWAPREQ_PROC(SProcQueryBestSize);
SWAPREQ_PROC(SProcQueryColors);
SWAPREQ_PROC(SProcQueryExtension);
SWAPREQ_PROC(SProcRecolorCursor);
SWAPREQ_PROC(SProcReparentWindow);
SWAPREQ_PROC(SProcResourceReq);
SWAPREQ_PROC(SProcRotateProperties);
SWAPREQ_PROC(SProcSendEvent);
SWAPREQ_PROC(SProcSetClipRectangles);
SWAPREQ_PROC(SProcSetDashes);
SWAPREQ_PROC(SProcSetFontPath);
SWAPREQ_PROC(SProcSetInputFocus);
SWAPREQ_PROC(SProcSetScreenSaver);
SWAPREQ_PROC(SProcSetSelectionOwner);
SWAPREQ_PROC(SProcSimpleReq);
SWAPREQ_PROC(SProcStoreColors);
SWAPREQ_PROC(SProcStoreNamedColor);
SWAPREQ_PROC(SProcTranslateCoords);
SWAPREQ_PROC(SProcUngrabButton);
SWAPREQ_PROC(SProcUngrabKey);
SWAPREQ_PROC(SProcWarpPointer);

#undef SWAPREQ_PROC

#endif                          /* SWAPREQ_H */
