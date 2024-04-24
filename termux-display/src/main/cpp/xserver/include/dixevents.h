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

#ifndef DIXEVENTS_H
#define DIXEVENTS_H

extern _X_EXPORT void SetCriticalEvent(int /* event */ );

extern _X_EXPORT CursorPtr GetSpriteCursor(DeviceIntPtr /*pDev */ );

extern _X_EXPORT int ProcAllowEvents(ClientPtr /* client */ );

extern _X_EXPORT int MaybeDeliverEventsToClient(WindowPtr /* pWin */ ,
                                                xEvent * /* pEvents */ ,
                                                int /* count */ ,
                                                Mask /* filter */ ,
                                                ClientPtr /* dontClient */ );

extern _X_EXPORT int ProcWarpPointer(ClientPtr /* client */ );

extern _X_EXPORT int EventSelectForWindow(WindowPtr /* pWin */ ,
                                          ClientPtr /* client */ ,
                                          Mask /* mask */ );

extern _X_EXPORT int EventSuppressForWindow(WindowPtr /* pWin */ ,
                                            ClientPtr /* client */ ,
                                            Mask /* mask */ ,
                                            Bool * /* checkOptional */ );

extern _X_EXPORT int ProcSetInputFocus(ClientPtr /* client */ );

extern _X_EXPORT int ProcGetInputFocus(ClientPtr /* client */ );

extern _X_EXPORT int ProcGrabPointer(ClientPtr /* client */ );

extern _X_EXPORT int ProcChangeActivePointerGrab(ClientPtr /* client */ );

extern _X_EXPORT int ProcUngrabPointer(ClientPtr /* client */ );

extern _X_EXPORT int ProcGrabKeyboard(ClientPtr /* client */ );

extern _X_EXPORT int ProcUngrabKeyboard(ClientPtr /* client */ );

extern _X_EXPORT int ProcQueryPointer(ClientPtr /* client */ );

extern _X_EXPORT int ProcSendEvent(ClientPtr /* client */ );

extern _X_EXPORT int ProcUngrabKey(ClientPtr /* client */ );

extern _X_EXPORT int ProcGrabKey(ClientPtr /* client */ );

extern _X_EXPORT int ProcGrabButton(ClientPtr /* client */ );

extern _X_EXPORT int ProcUngrabButton(ClientPtr /* client */ );

extern _X_EXPORT int ProcRecolorCursor(ClientPtr /* client */ );

#endif                          /* DIXEVENTS_H */
