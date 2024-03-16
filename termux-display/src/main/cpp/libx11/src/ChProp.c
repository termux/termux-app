/*

Copyright 1986, 1998  The Open Group

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

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

int
XChangeProperty (
    register Display *dpy,
    Window w,
    Atom property,
    Atom type,
    int format,  /* 8, 16, or 32 */
    int mode,  /* PropModeReplace, PropModePrepend, PropModeAppend */
    _Xconst unsigned char *data,
    int nelements)
    {
    register xChangePropertyReq *req;
    register long len;

    LockDisplay(dpy);
    GetReq (ChangeProperty, req);
    req->window = (CARD32) w;
    req->property = (CARD32) property;
    req->type = (CARD32) type;
    req->mode = (CARD8) mode;
    if (nelements < 0) {
	req->nUnits = 0;
	req->format = 0; /* ask for garbage, get garbage */
    } else {
	req->nUnits = (CARD32) nelements;
	req->format = (CARD8) format;
    }

    switch (req->format) {
      case 8:
	len = ((long)nelements + 3)>>2;
	if (dpy->bigreq_size || req->length + len <= (unsigned) 65535) {
	    SetReqLen(req, len, len);
	    Data (dpy, (_Xconst char *)data, nelements);
	} /* else force BadLength */
        break;

      case 16:
	len = ((long)nelements + 1)>>1;
	if (dpy->bigreq_size || req->length + len <= (unsigned) 65535) {
	    SetReqLen(req, len, len);
	    len = (long)nelements << 1;
	    Data16 (dpy, (_Xconst short *) data, len);
	} /* else force BadLength */
	break;

      case 32:
	len = nelements;
	if (dpy->bigreq_size || req->length + len <= (unsigned) 65535) {
	    SetReqLen(req, len, len);
	    len = (long)nelements << 2;
	    Data32 (dpy, (_Xconst long *) data, len);
	} /* else force BadLength */
	break;

      default:
        /* BadValue will be generated */ ;
      }

    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
    }





