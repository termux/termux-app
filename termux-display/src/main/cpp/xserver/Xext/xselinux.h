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

#ifndef _XSELINUX_H
#define _XSELINUX_H

/* Extension info */
#define SELINUX_EXTENSION_NAME		"SELinux"
#define SELINUX_MAJOR_VERSION		1
#define SELINUX_MINOR_VERSION		1
#define SELinuxNumberEvents		0
#define SELinuxNumberErrors		0

/* Extension protocol */
#define X_SELinuxQueryVersion			0
#define X_SELinuxSetDeviceCreateContext		1
#define X_SELinuxGetDeviceCreateContext		2
#define X_SELinuxSetDeviceContext		3
#define X_SELinuxGetDeviceContext		4
#define X_SELinuxSetDrawableCreateContext	5
#define X_SELinuxGetDrawableCreateContext	6
#define X_SELinuxGetDrawableContext		7
#define X_SELinuxSetPropertyCreateContext	8
#define X_SELinuxGetPropertyCreateContext	9
#define X_SELinuxSetPropertyUseContext		10
#define X_SELinuxGetPropertyUseContext		11
#define X_SELinuxGetPropertyContext		12
#define X_SELinuxGetPropertyDataContext		13
#define X_SELinuxListProperties			14
#define X_SELinuxSetSelectionCreateContext	15
#define X_SELinuxGetSelectionCreateContext	16
#define X_SELinuxSetSelectionUseContext		17
#define X_SELinuxGetSelectionUseContext		18
#define X_SELinuxGetSelectionContext		19
#define X_SELinuxGetSelectionDataContext	20
#define X_SELinuxListSelections			21
#define X_SELinuxGetClientContext		22

typedef struct {
    CARD8 reqType;
    CARD8 SELinuxReqType;
    CARD16 length;
    CARD8 client_major;
    CARD8 client_minor;
} SELinuxQueryVersionReq;

typedef struct {
    CARD8 type;
    CARD8 pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD16 server_major;
    CARD16 server_minor;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} SELinuxQueryVersionReply;

typedef struct {
    CARD8 reqType;
    CARD8 SELinuxReqType;
    CARD16 length;
    CARD32 context_len;
} SELinuxSetCreateContextReq;

typedef struct {
    CARD8 reqType;
    CARD8 SELinuxReqType;
    CARD16 length;
} SELinuxGetCreateContextReq;

typedef struct {
    CARD8 reqType;
    CARD8 SELinuxReqType;
    CARD16 length;
    CARD32 id;
    CARD32 context_len;
} SELinuxSetContextReq;

typedef struct {
    CARD8 reqType;
    CARD8 SELinuxReqType;
    CARD16 length;
    CARD32 id;
} SELinuxGetContextReq;

typedef struct {
    CARD8 reqType;
    CARD8 SELinuxReqType;
    CARD16 length;
    CARD32 window;
    CARD32 property;
} SELinuxGetPropertyContextReq;

typedef struct {
    CARD8 type;
    CARD8 pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 context_len;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} SELinuxGetContextReply;

typedef struct {
    CARD8 type;
    CARD8 pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 count;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} SELinuxListItemsReply;

#endif                          /* _XSELINUX_H */
