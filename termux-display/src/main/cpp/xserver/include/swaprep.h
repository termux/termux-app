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

#ifndef SWAPREP_H
#define SWAPREP_H 1

extern void Swap32Write(ClientPtr /* pClient */ ,
                        int /* size */ ,
                        CARD32 * /* pbuf */ );

extern void CopySwap32Write(ClientPtr /* pClient */ ,
                            int /* size */ ,
                            CARD32 * /* pbuf */ );

extern void CopySwap16Write(ClientPtr /* pClient */ ,
                            int /* size */ ,
                            short * /* pbuf */ );

extern void SGenericReply(ClientPtr /* pClient */ ,
                          int /* size */ ,
                          xGenericReply * /* pRep */ );

extern void SGetWindowAttributesReply(ClientPtr /* pClient */ ,
                                      int /* size */ ,
                                      xGetWindowAttributesReply *
                                      /* pRep */ );

extern void SGetGeometryReply(ClientPtr /* pClient */ ,
                              int /* size */ ,
                              xGetGeometryReply * /* pRep */ );

extern void SQueryTreeReply(ClientPtr /* pClient */ ,
                            int /* size */ ,
                            xQueryTreeReply * /* pRep */ );

extern void SInternAtomReply(ClientPtr /* pClient */ ,
                             int /* size */ ,
                             xInternAtomReply * /* pRep */ );

extern void SGetAtomNameReply(ClientPtr /* pClient */ ,
                              int /* size */ ,
                              xGetAtomNameReply * /* pRep */ );

extern void SGetPropertyReply(ClientPtr /* pClient */ ,
                              int /* size */ ,
                              xGetPropertyReply * /* pRep */ );

extern void SListPropertiesReply(ClientPtr /* pClient */ ,
                                 int /* size */ ,
                                 xListPropertiesReply * /* pRep */ );

extern void SGetSelectionOwnerReply(ClientPtr /* pClient */ ,
                                    int /* size */ ,
                                    xGetSelectionOwnerReply *
                                    /* pRep */ );

extern void SQueryPointerReply(ClientPtr /* pClient */ ,
                               int /* size */ ,
                               xQueryPointerReply * /* pRep */ );

extern void SwapTimeCoordWrite(ClientPtr /* pClient */ ,
                               int /* size */ ,
                               xTimecoord * /* pRep */ );

extern void SGetMotionEventsReply(ClientPtr /* pClient */ ,
                                  int /* size */ ,
                                  xGetMotionEventsReply *       /* pRep */
    );

extern void STranslateCoordsReply(ClientPtr /* pClient */ ,
                                  int /* size */ ,
                                  xTranslateCoordsReply *       /* pRep */
    );

extern void SGetInputFocusReply(ClientPtr /* pClient */ ,
                                int /* size */ ,
                                xGetInputFocusReply * /* pRep */ );

extern void SQueryKeymapReply(ClientPtr /* pClient */ ,
                              int /* size */ ,
                              xQueryKeymapReply * /* pRep */ );

extern void SQueryFontReply(ClientPtr /* pClient */ ,
                            int /* size */ ,
                            xQueryFontReply * /* pRep */ );

extern void SQueryTextExtentsReply(ClientPtr /* pClient */ ,
                                   int /* size */ ,
                                   xQueryTextExtentsReply *     /* pRep */
    );

extern void SListFontsReply(ClientPtr /* pClient */ ,
                            int /* size */ ,
                            xListFontsReply * /* pRep */ );

extern void SListFontsWithInfoReply(ClientPtr /* pClient */ ,
                                    int /* size */ ,
                                    xListFontsWithInfoReply *
                                    /* pRep */ );

extern void SGetFontPathReply(ClientPtr /* pClient */ ,
                              int /* size */ ,
                              xGetFontPathReply * /* pRep */ );

extern void SGetImageReply(ClientPtr /* pClient */ ,
                           int /* size */ ,
                           xGetImageReply * /* pRep */ );

extern void SListInstalledColormapsReply(ClientPtr /* pClient */ ,
                                         int /* size */ ,
                                         xListInstalledColormapsReply
                                         * /* pRep */ );

extern void SAllocColorReply(ClientPtr /* pClient */ ,
                             int /* size */ ,
                             xAllocColorReply * /* pRep */ );

extern void SAllocNamedColorReply(ClientPtr /* pClient */ ,
                                  int /* size */ ,
                                  xAllocNamedColorReply *       /* pRep */
    );

extern void SAllocColorCellsReply(ClientPtr /* pClient */ ,
                                  int /* size */ ,
                                  xAllocColorCellsReply *       /* pRep */
    );

extern void SAllocColorPlanesReply(ClientPtr /* pClient */ ,
                                   int /* size */ ,
                                   xAllocColorPlanesReply *     /* pRep */
    );

extern void SQColorsExtend(ClientPtr /* pClient */ ,
                           int /* size */ ,
                           xrgb * /* prgb */ );

extern void SQueryColorsReply(ClientPtr /* pClient */ ,
                              int /* size */ ,
                              xQueryColorsReply * /* pRep */ );

extern void SLookupColorReply(ClientPtr /* pClient */ ,
                              int /* size */ ,
                              xLookupColorReply * /* pRep */ );

extern void SQueryBestSizeReply(ClientPtr /* pClient */ ,
                                int /* size */ ,
                                xQueryBestSizeReply * /* pRep */ );

extern void SListExtensionsReply(ClientPtr /* pClient */ ,
                                 int /* size */ ,
                                 xListExtensionsReply * /* pRep */ );

extern void SGetKeyboardMappingReply(ClientPtr /* pClient */ ,
                                     int /* size */ ,
                                     xGetKeyboardMappingReply *
                                     /* pRep */ );

extern void SGetPointerMappingReply(ClientPtr /* pClient */ ,
                                    int /* size */ ,
                                    xGetPointerMappingReply *
                                    /* pRep */ );

extern void SGetModifierMappingReply(ClientPtr /* pClient */ ,
                                     int /* size */ ,
                                     xGetModifierMappingReply *
                                     /* pRep */ );

extern void SGetKeyboardControlReply(ClientPtr /* pClient */ ,
                                     int /* size */ ,
                                     xGetKeyboardControlReply *
                                     /* pRep */ );

extern void SGetPointerControlReply(ClientPtr /* pClient */ ,
                                    int /* size */ ,
                                    xGetPointerControlReply *
                                    /* pRep */ );

extern void SGetScreenSaverReply(ClientPtr /* pClient */ ,
                                 int /* size */ ,
                                 xGetScreenSaverReply * /* pRep */ );

extern void SLHostsExtend(ClientPtr /* pClient */ ,
                          int /* size */ ,
                          char * /* buf */ );

extern void SListHostsReply(ClientPtr /* pClient */ ,
                            int /* size */ ,
                            xListHostsReply * /* pRep */ );

extern void SErrorEvent(xError * /* from */ ,
                        xError * /* to */ );

extern void SwapConnSetupInfo(char * /* pInfo */ ,
                              char * /* pInfoTBase */ );

extern void WriteSConnectionInfo(ClientPtr /* pClient */ ,
                                 unsigned long /* size */ ,
                                 char * /* pInfo */ );

extern void SwapConnSetupPrefix(xConnSetupPrefix * /* pcspFrom */ ,
                                xConnSetupPrefix * /* pcspTo */ );

extern void WriteSConnSetupPrefix(ClientPtr /* pClient */ ,
                                  xConnSetupPrefix * /* pcsp */ );

#undef SWAPREP_PROC
#define SWAPREP_PROC(func) extern void func(xEvent * /* from */, xEvent * /* to */)

SWAPREP_PROC(SCirculateEvent);
SWAPREP_PROC(SClientMessageEvent);
SWAPREP_PROC(SColormapEvent);
SWAPREP_PROC(SConfigureNotifyEvent);
SWAPREP_PROC(SConfigureRequestEvent);
SWAPREP_PROC(SCreateNotifyEvent);
SWAPREP_PROC(SDestroyNotifyEvent);
SWAPREP_PROC(SEnterLeaveEvent);
SWAPREP_PROC(SExposeEvent);
SWAPREP_PROC(SFocusEvent);
SWAPREP_PROC(SGraphicsExposureEvent);
SWAPREP_PROC(SGravityEvent);
SWAPREP_PROC(SKeyButtonPtrEvent);
SWAPREP_PROC(SKeymapNotifyEvent);
SWAPREP_PROC(SMapNotifyEvent);
SWAPREP_PROC(SMapRequestEvent);
SWAPREP_PROC(SMappingEvent);
SWAPREP_PROC(SNoExposureEvent);
SWAPREP_PROC(SPropertyEvent);
SWAPREP_PROC(SReparentEvent);
SWAPREP_PROC(SResizeRequestEvent);
SWAPREP_PROC(SSelectionClearEvent);
SWAPREP_PROC(SSelectionNotifyEvent);
SWAPREP_PROC(SSelectionRequestEvent);
SWAPREP_PROC(SUnmapNotifyEvent);
SWAPREP_PROC(SVisibilityEvent);

#undef SWAPREP_PROC

#endif                          /* SWAPREP_H */
