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
#include <limits.h>
#include <stdio.h>
#include "Xlibint.h"
#include "Xcmsint.h"

Status
XAllocNamedColor(
register Display *dpy,
Colormap cmap,
_Xconst char *colorname, /* STRING8 */
XColor *hard_def, /* RETURN */
XColor *exact_def) /* RETURN */
{

    long nbytes;
    xAllocNamedColorReply rep;
    xAllocNamedColorReq *req;

    XcmsCCC ccc;
    XcmsColor cmsColor_exact;
    Status ret;

    if (colorname != NULL && strlen(colorname) >= USHRT_MAX)
        return (0);

#ifdef XCMS
    /*
     * Let's Attempt to use Xcms and i18n approach to Parse Color
     */
    if ((ccc = XcmsCCCOfColormap(dpy, cmap)) != (XcmsCCC)NULL) {
	const char *tmpName = colorname;

	switch (_XcmsResolveColorString(ccc, &tmpName, &cmsColor_exact,
					XcmsRGBFormat)) {
	case XcmsSuccess:
	case XcmsSuccessWithCompression:
	    _XcmsRGB_to_XColor(&cmsColor_exact, exact_def, 1);
	    memcpy((char *)hard_def, (char *)exact_def, sizeof(XColor));
	    ret = XAllocColor(dpy, cmap, hard_def);
	    exact_def->pixel = hard_def->pixel;
	    return(ret);
	case XcmsFailure:
	case _XCMS_NEWNAME:
	    /*
	     * if the result was _XCMS_NEWNAME tmpName points to
	     * a string in cmsColNm.c:pairs table, for example,
	     * gray70 would become tekhvc:0.0/70.0/0.0
	     */
	    break;
	}
    }
#endif

    /*
     * Xcms and i18n approach failed.
     */
    LockDisplay(dpy);
    GetReq(AllocNamedColor, req);

    req->cmap = cmap;
    nbytes = req->nbytes = (CARD16) strlen(colorname);
    req->length += (nbytes + 3) >> 2; /* round up to mult of 4 */

    _XSend(dpy, colorname, nbytes);
       /* _XSend is more efficient that Data, since _XReply follows */

    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay(dpy);
        SyncHandle();
        return (0);
    }

    exact_def->red = rep.exactRed;
    exact_def->green = rep.exactGreen;
    exact_def->blue = rep.exactBlue;

    hard_def->red = rep.screenRed;
    hard_def->green = rep.screenGreen;
    hard_def->blue = rep.screenBlue;

    exact_def->pixel = hard_def->pixel = rep.pixel;

    UnlockDisplay(dpy);
    SyncHandle();
    return (1);
}
