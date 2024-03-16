/************************************************************

Author: Eamon Walsh <ewalsh@tycho.nsa.gov>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

#ifndef _XACE_H
#define _XACE_H

#ifdef XACE

#define XACE_MAJOR_VERSION		2
#define XACE_MINOR_VERSION		0

#include "pixmap.h"
#include "region.h"
#include "window.h"
#include "property.h"
#include "selection.h"

/* Default window background */
#define XaceBackgroundNoneState(w) ((w)->forcedBG ? BackgroundPixel : None)

/* security hooks */
/* Constants used to identify the available security hooks
 */
#define XACE_CORE_DISPATCH		0
#define XACE_EXT_DISPATCH		1
#define XACE_RESOURCE_ACCESS		2
#define XACE_DEVICE_ACCESS		3
#define XACE_PROPERTY_ACCESS		4
#define XACE_SEND_ACCESS		5
#define XACE_RECEIVE_ACCESS		6
#define XACE_CLIENT_ACCESS		7
#define XACE_EXT_ACCESS			8
#define XACE_SERVER_ACCESS		9
#define XACE_SELECTION_ACCESS		10
#define XACE_SCREEN_ACCESS		11
#define XACE_SCREENSAVER_ACCESS		12
#define XACE_AUTH_AVAIL			13
#define XACE_KEY_AVAIL			14
#define XACE_NUM_HOOKS			15

extern _X_EXPORT CallbackListPtr XaceHooks[XACE_NUM_HOOKS];

/* Entry point for hook functions.  Called by Xserver.
 * Required by libdbe and libextmod
 */
extern _X_EXPORT int XaceHook(int /*hook */ ,
                              ...       /*appropriate args for hook */
    );

/* determine whether any callbacks are present for the XACE hook */
extern _X_EXPORT int XaceHookIsSet(int hook);

/* Special-cased hook functions
 */
extern _X_EXPORT int XaceHookDispatch(ClientPtr ptr, int major);
#define XaceHookDispatch(c, m) \
    ((XaceHooks[XACE_EXT_DISPATCH] && (m) >= EXTENSION_BASE) ? \
    XaceHookDispatch((c), (m)) : \
    Success)

extern _X_EXPORT int XaceHookPropertyAccess(ClientPtr ptr, WindowPtr pWin,
                                            PropertyPtr *ppProp,
                                            Mask access_mode);
extern _X_EXPORT int XaceHookSelectionAccess(ClientPtr ptr, Selection ** ppSel,
                                             Mask access_mode);

/* Register a callback for a given hook.
 */
#define XaceRegisterCallback(hook,callback,data) \
    AddCallback(XaceHooks+(hook), callback, data)

/* Unregister an existing callback for a given hook.
 */
#define XaceDeleteCallback(hook,callback,data) \
    DeleteCallback(XaceHooks+(hook), callback, data)

/* XTrans wrappers for use by security modules
 */
extern _X_EXPORT int XaceGetConnectionNumber(ClientPtr ptr);
extern _X_EXPORT int XaceIsLocal(ClientPtr ptr);

/* From the original Security extension...
 */

extern _X_EXPORT void XaceCensorImage(ClientPtr client,
                                      RegionPtr pVisibleRegion,
                                      long widthBytesLine,
                                      DrawablePtr pDraw,
                                      int x, int y, int w, int h,
                                      unsigned int format, char *pBuf);

#else                           /* XACE */

/* Default window background */
#define XaceBackgroundNoneState(w)		None

/* Define calls away when XACE is not being built. */

#ifdef __GNUC__
#define XaceHook(args...) Success
#define XaceHookIsSet(args...) 0
#define XaceHookDispatch(args...) Success
#define XaceHookPropertyAccess(args...) Success
#define XaceHookSelectionAccess(args...) Success
#define XaceCensorImage(args...) { ; }
#else
#define XaceHook(...) Success
#define XaceHookIsSet(...) 0
#define XaceHookDispatch(...) Success
#define XaceHookPropertyAccess(...) Success
#define XaceHookSelectionAccess(...) Success
#define XaceCensorImage(...) { ; }
#endif

#endif                          /* XACE */

#endif                          /* _XACE_H */
