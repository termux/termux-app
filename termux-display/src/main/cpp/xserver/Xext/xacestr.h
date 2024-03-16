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

#ifndef _XACESTR_H
#define _XACESTR_H

#include "dix.h"
#include "resource.h"
#include "extnsionst.h"
#include "window.h"
#include "input.h"
#include "property.h"
#include "selection.h"
#include "xace.h"

/* XACE_CORE_DISPATCH */
typedef struct {
    ClientPtr client;
    int status;
} XaceCoreDispatchRec;

/* XACE_RESOURCE_ACCESS */
typedef struct {
    ClientPtr client;
    XID id;
    RESTYPE rtype;
    void *res;
    RESTYPE ptype;
    void *parent;
    Mask access_mode;
    int status;
} XaceResourceAccessRec;

/* XACE_DEVICE_ACCESS */
typedef struct {
    ClientPtr client;
    DeviceIntPtr dev;
    Mask access_mode;
    int status;
} XaceDeviceAccessRec;

/* XACE_PROPERTY_ACCESS */
typedef struct {
    ClientPtr client;
    WindowPtr pWin;
    PropertyPtr *ppProp;
    Mask access_mode;
    int status;
} XacePropertyAccessRec;

/* XACE_SEND_ACCESS */
typedef struct {
    ClientPtr client;
    DeviceIntPtr dev;
    WindowPtr pWin;
    xEventPtr events;
    int count;
    int status;
} XaceSendAccessRec;

/* XACE_RECEIVE_ACCESS */
typedef struct {
    ClientPtr client;
    WindowPtr pWin;
    xEventPtr events;
    int count;
    int status;
} XaceReceiveAccessRec;

/* XACE_CLIENT_ACCESS */
typedef struct {
    ClientPtr client;
    ClientPtr target;
    Mask access_mode;
    int status;
} XaceClientAccessRec;

/* XACE_EXT_DISPATCH */
/* XACE_EXT_ACCESS */
typedef struct {
    ClientPtr client;
    ExtensionEntry *ext;
    Mask access_mode;
    int status;
} XaceExtAccessRec;

/* XACE_SERVER_ACCESS */
typedef struct {
    ClientPtr client;
    Mask access_mode;
    int status;
} XaceServerAccessRec;

/* XACE_SELECTION_ACCESS */
typedef struct {
    ClientPtr client;
    Selection **ppSel;
    Mask access_mode;
    int status;
} XaceSelectionAccessRec;

/* XACE_SCREEN_ACCESS */
/* XACE_SCREENSAVER_ACCESS */
typedef struct {
    ClientPtr client;
    ScreenPtr screen;
    Mask access_mode;
    int status;
} XaceScreenAccessRec;

/* XACE_AUTH_AVAIL */
typedef struct {
    ClientPtr client;
    XID authId;
} XaceAuthAvailRec;

/* XACE_KEY_AVAIL */
typedef struct {
    xEventPtr event;
    DeviceIntPtr keybd;
    int count;
} XaceKeyAvailRec;

/* XACE_AUDIT_BEGIN */
/* XACE_AUDIT_END */
typedef struct {
    ClientPtr client;
    int requestResult;
} XaceAuditRec;

#endif                          /* _XACESTR_H */
