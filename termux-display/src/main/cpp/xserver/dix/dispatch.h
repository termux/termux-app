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

/*
 * This prototypes the dispatch.c module (except for functions declared in
 * global headers), plus related dispatch procedures from devices.c, events.c,
 * extension.c, property.c.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef DISPATCH_H
#define DISPATCH_H 1

int ProcAllocColor(ClientPtr /* client */ );
int ProcAllocColorCells(ClientPtr /* client */ );
int ProcAllocColorPlanes(ClientPtr /* client */ );
int ProcAllocNamedColor(ClientPtr /* client */ );
int ProcBell(ClientPtr /* client */ );
int ProcChangeAccessControl(ClientPtr /* client */ );
int ProcChangeCloseDownMode(ClientPtr /* client */ );
int ProcChangeGC(ClientPtr /* client */ );
int ProcChangeHosts(ClientPtr /* client */ );
int ProcChangeKeyboardControl(ClientPtr /* client */ );
int ProcChangeKeyboardMapping(ClientPtr /* client */ );
int ProcChangePointerControl(ClientPtr /* client */ );
int ProcChangeProperty(ClientPtr /* client */ );
int ProcChangeSaveSet(ClientPtr /* client */ );
int ProcChangeWindowAttributes(ClientPtr /* client */ );
int ProcCirculateWindow(ClientPtr /* client */ );
int ProcClearToBackground(ClientPtr /* client */ );
int ProcCloseFont(ClientPtr /* client */ );
int ProcConfigureWindow(ClientPtr /* client */ );
int ProcConvertSelection(ClientPtr /* client */ );
int ProcCopyArea(ClientPtr /* client */ );
int ProcCopyColormapAndFree(ClientPtr /* client */ );
int ProcCopyGC(ClientPtr /* client */ );
int ProcCopyPlane(ClientPtr /* client */ );
int ProcCreateColormap(ClientPtr /* client */ );
int ProcCreateCursor(ClientPtr /* client */ );
int ProcCreateGC(ClientPtr /* client */ );
int ProcCreateGlyphCursor(ClientPtr /* client */ );
int ProcCreatePixmap(ClientPtr /* client */ );
int ProcCreateWindow(ClientPtr /* client */ );
int ProcDeleteProperty(ClientPtr /* client */ );
int ProcDestroySubwindows(ClientPtr /* client */ );
int ProcDestroyWindow(ClientPtr /* client */ );
int ProcEstablishConnection(ClientPtr /* client */ );
int ProcFillPoly(ClientPtr /* client */ );
int ProcForceScreenSaver(ClientPtr /* client */ );
int ProcFreeColormap(ClientPtr /* client */ );
int ProcFreeColors(ClientPtr /* client */ );
int ProcFreeCursor(ClientPtr /* client */ );
int ProcFreeGC(ClientPtr /* client */ );
int ProcFreePixmap(ClientPtr /* client */ );
int ProcGetAtomName(ClientPtr /* client */ );
int ProcGetFontPath(ClientPtr /* client */ );
int ProcGetGeometry(ClientPtr /* client */ );
int ProcGetImage(ClientPtr /* client */ );
int ProcGetKeyboardControl(ClientPtr /* client */ );
int ProcGetKeyboardMapping(ClientPtr /* client */ );
int ProcGetModifierMapping(ClientPtr /* client */ );
int ProcGetMotionEvents(ClientPtr /* client */ );
int ProcGetPointerControl(ClientPtr /* client */ );
int ProcGetPointerMapping(ClientPtr /* client */ );
int ProcGetProperty(ClientPtr /* client */ );
int ProcGetScreenSaver(ClientPtr /* client */ );
int ProcGetSelectionOwner(ClientPtr /* client */ );
int ProcGetWindowAttributes(ClientPtr /* client */ );
int ProcGrabServer(ClientPtr /* client */ );
int ProcImageText16(ClientPtr /* client */ );
int ProcImageText8(ClientPtr /* client */ );
int ProcInitialConnection(ClientPtr /* client */ );
int ProcInstallColormap(ClientPtr /* client */ );
int ProcInternAtom(ClientPtr /* client */ );
int ProcKillClient(ClientPtr /* client */ );
int ProcListExtensions(ClientPtr /* client */ );
int ProcListFonts(ClientPtr /* client */ );
int ProcListFontsWithInfo(ClientPtr /* client */ );
int ProcListHosts(ClientPtr /* client */ );
int ProcListInstalledColormaps(ClientPtr /* client */ );
int ProcListProperties(ClientPtr /* client */ );
int ProcLookupColor(ClientPtr /* client */ );
int ProcMapSubwindows(ClientPtr /* client */ );
int ProcMapWindow(ClientPtr /* client */ );
int ProcNoOperation(ClientPtr /* client */ );
int ProcOpenFont(ClientPtr /* client */ );
int ProcPolyArc(ClientPtr /* client */ );
int ProcPolyFillArc(ClientPtr /* client */ );
int ProcPolyFillRectangle(ClientPtr /* client */ );
int ProcPolyLine(ClientPtr /* client */ );
int ProcPolyPoint(ClientPtr /* client */ );
int ProcPolyRectangle(ClientPtr /* client */ );
int ProcPolySegment(ClientPtr /* client */ );
int ProcPolyText(ClientPtr /* client */ );
int ProcPutImage(ClientPtr /* client */ );
int ProcQueryBestSize(ClientPtr /* client */ );
int ProcQueryColors(ClientPtr /* client */ );
int ProcQueryExtension(ClientPtr /* client */ );
int ProcQueryFont(ClientPtr /* client */ );
int ProcQueryKeymap(ClientPtr /* client */ );
int ProcQueryTextExtents(ClientPtr /* client */ );
int ProcQueryTree(ClientPtr /* client */ );
int ProcReparentWindow(ClientPtr /* client */ );
int ProcRotateProperties(ClientPtr /* client */ );
int ProcSetClipRectangles(ClientPtr /* client */ );
int ProcSetDashes(ClientPtr /* client */ );
int ProcSetFontPath(ClientPtr /* client */ );
int ProcSetModifierMapping(ClientPtr /* client */ );
int ProcSetPointerMapping(ClientPtr /* client */ );
int ProcSetScreenSaver(ClientPtr /* client */ );
int ProcSetSelectionOwner(ClientPtr /* client */ );
int ProcStoreColors(ClientPtr /* client */ );
int ProcStoreNamedColor(ClientPtr /* client */ );
int ProcTranslateCoords(ClientPtr /* client */ );
int ProcUngrabServer(ClientPtr /* client */ );
int ProcUninstallColormap(ClientPtr /* client */ );
int ProcUnmapSubwindows(ClientPtr /* client */ );
int ProcUnmapWindow(ClientPtr /* client */ );

#endif                          /* DISPATCH_H */
