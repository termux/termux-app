/* $XFree86: xc/include/extensions/xtrapproto.h,v 1.1 2001/11/02 23:29:26 dawes Exp $ */

#ifndef __XTRAPPROTO__
#define __XTRAPPROTO__

/*****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991 by Digital Equipment Corp., Maynard, MA

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

*****************************************************************************/
/*
 *
 *  CONTRIBUTORS:
 *
 *      Dick Annicchiarico
 *      Robert Chesler
 *      Dan Coutu
 *      Gene Durso
 *      Marc Evans
 *      Alan Jamison
 *      Mark Henry
 *      Ken Miller
 *
 *  DESCRIPTION:
 *      This header file contains the function prototypes for extension
 *      routines sorted by module (globally defined routines *only*).
 */
#ifndef Bool
# define Bool int
#endif
/* xtrapdi.c */
int XETrapDestroyEnv (pointer value , XID id );
void XETrapCloseDown ( ExtensionEntry *extEntry );
Bool XETrapRedirectDevices (void );
void DEC_XTRAPInit (void );
int XETrapCreateEnv (ClientPtr client );
int XETrapDispatch (ClientPtr client );
int sXETrapDispatch (ClientPtr client );
int XETrapReset (xXTrapReq *request , ClientPtr client );
int XETrapGetAvailable (xXTrapGetReq *request , ClientPtr client );
int XETrapGetCurrent (xXTrapReq *request , ClientPtr client );
int XETrapGetStatistics (xXTrapReq *request , ClientPtr client );
int XETrapConfig (xXTrapConfigReq *request , ClientPtr client );
int XETrapStartTrap (xXTrapReq *request , ClientPtr client );
int XETrapStopTrap (xXTrapReq *request , ClientPtr client );
int XETrapGetVersion (xXTrapGetReq *request , ClientPtr client );
int XETrapGetLastInpTime (xXTrapReq *request , ClientPtr client );
int XETrapRequestVector (ClientPtr client );
int XETrapKeyboard (xEvent *x_event , DevicePtr keybd , int count );
#ifndef VECTORED_EVENTS
int XETrapPointer (xEvent *x_event , DevicePtr ptrdev , int count );
#else
int XETrapEventVector (ClientPtr client , xEvent *x_event );
#endif
void XETrapStampAndMail (xEvent *x_event );
void sReplyXTrapDispatch (ClientPtr client , int size , char *reply );
int XETrapWriteXLib (XETrapEnv *penv , BYTE *data , CARD32 nbytes );

/* xtrapddmi.c */
void XETrapPlatformSetup (void );
int XETrapSimulateXEvent (xXTrapInputReq *request , ClientPtr client );

/* xtrapdiswap.c */
int sXETrapReset (xXTrapReq *request , ClientPtr client );
int sXETrapGetAvailable (xXTrapGetReq *request , ClientPtr client );
int sXETrapConfig (xXTrapConfigReq *request , ClientPtr client );
int sXETrapStartTrap (xXTrapReq *request , ClientPtr client );
int sXETrapStopTrap (xXTrapReq *request , ClientPtr client );
int sXETrapGetCurrent (xXTrapReq *request , ClientPtr client );
int sXETrapGetStatistics (xXTrapReq *request , ClientPtr client );
int sXETrapSimulateXEvent (xXTrapInputReq *request , ClientPtr client );
int sXETrapGetVersion (xXTrapGetReq *request , ClientPtr client );
int sXETrapGetLastInpTime (xXTrapReq *request , ClientPtr client );
void sReplyXETrapGetAvail (ClientPtr client , int size , char *reply );
void sReplyXETrapGetVers (ClientPtr client , int size , char *reply );
void sReplyXETrapGetLITim (ClientPtr client , int size , char *reply );
void sReplyXETrapGetCur (ClientPtr client , int size , char *reply );
void sReplyXETrapGetStats (ClientPtr client , int size , char *reply );
void sXETrapHeader (XETrapHeader *hdr );
void XETSwSimpleReq (xReq *data );
void XETSwResourceReq (xResourceReq *data );
void XETSwCreateWindow (xCreateWindowReq *data , ClientPtr client );
void XETSwChangeWindowAttributes (xChangeWindowAttributesReq *data , ClientPtr client );
void XETSwReparentWindow (xReparentWindowReq *data );
void XETSwConfigureWindow (xConfigureWindowReq *data  , ClientPtr client );
void XETSwInternAtom (xInternAtomReq *data );
void XETSwChangeProperty (xChangePropertyReq *data );
void XETSwDeleteProperty (xDeletePropertyReq *data );
void XETSwGetProperty (xGetPropertyReq *data );
void XETSwSetSelectionOwner (xSetSelectionOwnerReq *data );
void XETSwConvertSelection (xConvertSelectionReq *data );
void XETSwSendEvent (xSendEventReq *data );
void XETSwGrabPointer (xGrabPointerReq *data );
void XETSwGrabButton (xGrabButtonReq *data );
void XETSwUngrabButton (xUngrabButtonReq *data );
void XETSwChangeActivePointerGrab (xChangeActivePointerGrabReq *data );
void XETSwGrabKeyboard (xGrabKeyboardReq *data );
void XETSwGrabKey (xGrabKeyReq *data );
void XETSwUngrabKey (xUngrabKeyReq *data );
void XETSwGetMotionEvents (xGetMotionEventsReq *data );
void XETSwTranslateCoords (xTranslateCoordsReq *data );
void XETSwWarpPointer (xWarpPointerReq *data );
void XETSwSetInputFocus (xSetInputFocusReq *data );
void XETSwOpenFont (xOpenFontReq *data );
void XETSwListFonts (xListFontsReq *data );
void XETSwListFontsWithInfo (xListFontsWithInfoReq *data );
void XETSwSetFontPath (xSetFontPathReq *data );
void XETSwCreatePixmap (xCreatePixmapReq *data );
void XETSwCreateGC (xCreateGCReq *data , ClientPtr client );
void XETSwChangeGC (xChangeGCReq *data , ClientPtr client );
void XETSwCopyGC (xCopyGCReq *data );
void XETSwSetDashes (xSetDashesReq *data );
void XETSwSetClipRectangles (xSetClipRectanglesReq *data , ClientPtr client );
void XETSwClearToBackground (xClearAreaReq *data );
void XETSwCopyArea (xCopyAreaReq *data );
void XETSwCopyPlane (xCopyPlaneReq *data );
void XETSwPoly (xPolyPointReq *data , ClientPtr client );
void XETSwFillPoly (xFillPolyReq *data , ClientPtr client );
void XETSwPutImage (xPutImageReq *data );
void XETSwGetImage (xGetImageReq *data );
void XETSwPolyText (xPolyTextReq *data );
void XETSwImageText (xImageTextReq *data );
void XETSwCreateColormap (xCreateColormapReq *data );
void XETSwCopyColormapAndFree (xCopyColormapAndFreeReq *data );
void XETSwAllocColor (xAllocColorReq *data );
void XETSwAllocNamedColor (xAllocNamedColorReq *data );
void XETSwAllocColorCells (xAllocColorCellsReq *data );
void XETSwAllocColorPlanes (xAllocColorPlanesReq *data );
void XETSwFreeColors (xFreeColorsReq *data , ClientPtr client );
void XETSwStoreColors (xStoreColorsReq *data , ClientPtr client );
void XETSwStoreNamedColor (xStoreNamedColorReq *data );
void XETSwQueryColors (xQueryColorsReq *data , ClientPtr client );
void XETSwLookupColor (xLookupColorReq *data );
void XETSwCreateCursor (xCreateCursorReq *data );
void XETSwCreateGlyphCursor (xCreateGlyphCursorReq *data );
void XETSwRecolorCursor (xRecolorCursorReq *data );
void XETSwQueryBestSize (xQueryBestSizeReq *data );
void XETSwQueryExtension (xQueryExtensionReq *data );
void XETSwChangeKeyboardMapping (xChangeKeyboardMappingReq *data );
void XETSwChangeKeyboardControl (xChangeKeyboardControlReq *data , ClientPtr client );
void XETSwChangePointerControl (xChangePointerControlReq *data );
void XETSwSetScreenSaver (xSetScreenSaverReq *data );
void XETSwChangeHosts (xChangeHostsReq *data );
void XETSwRotateProperties (xRotatePropertiesReq *data , ClientPtr client );
void XETSwNoOperation (xReq *data );
#ifdef vms
void SwapLongs (long *list , unsigned long count );
void SwapShorts (short *list , unsigned long count );
int SwapColorItem (xColorItem *pItem );
#endif /* vms */


#endif /* __XTRAPPROTO__ */
