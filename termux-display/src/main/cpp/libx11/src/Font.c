/*

Copyright 1986, 1998  The Open Group
Copyright (c) 2000  The XFree86 Project, Inc.

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
X CONSORTIUM OR THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Except as contained in this notice, the name of the X Consortium or of the
XFree86 Project shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the X Consortium and the XFree86 Project.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "reallocarray.h"
#include <limits.h>

#if defined(XF86BIGFONT)
#define USE_XF86BIGFONT
#endif
#ifdef USE_XF86BIGFONT
#include <sys/types.h>
#ifdef HAS_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <X11/extensions/xf86bigfproto.h>
#endif

#include "Xlcint.h"
#include "XlcPubI.h"


static XFontStruct *_XQueryFont(
    Display*		/* dpy */,
    Font		/* fid */,
    unsigned long	/* seq */
);

#ifdef USE_XF86BIGFONT

/* Private data for this extension. */
typedef struct {
    XExtCodes *codes;
    CARD32 serverSignature;
    CARD32 serverCapabilities;
} XF86BigfontCodes;

/* Additional bit masks that can be set in serverCapabilities */
#define CAP_VerifiedLocal 256

static XF86BigfontCodes *_XF86BigfontCodes(
    Display*		/* dpy */
);

static XFontStruct *_XF86BigfontQueryFont(
    Display*		/* dpy */,
    XF86BigfontCodes*	/* extcodes */,
    Font		/* fid */,
    unsigned long	/* seq */
);

void _XF86BigfontFreeFontMetrics(
    XFontStruct*	/* fs */
);

#endif /* USE_XF86BIGFONT */


XFontStruct *XLoadQueryFont(
   register Display *dpy,
   _Xconst char *name)
{
    XFontStruct *font_result;
    register long nbytes;
    Font fid;
    xOpenFontReq *req;
    unsigned long seq;
#ifdef USE_XF86BIGFONT
    XF86BigfontCodes *extcodes = _XF86BigfontCodes(dpy);
#endif

    if (name != NULL && strlen(name) >= USHRT_MAX)
        return NULL;
    if (_XF86LoadQueryLocaleFont(dpy, name, &font_result, (Font *)0))
      return font_result;
    LockDisplay(dpy);
    GetReq(OpenFont, req);
    seq = dpy->request; /* Can't use extended sequence number here */
    nbytes = req->nbytes = (CARD16) (name ? strlen(name) : 0);
    req->fid = fid = XAllocID(dpy);
    req->length += (nbytes+3)>>2;
    Data (dpy, name, nbytes);
    font_result = NULL;
#ifdef USE_XF86BIGFONT
    if (extcodes) {
	font_result = _XF86BigfontQueryFont(dpy, extcodes, fid, seq);
	seq = 0;
    }
#endif
    if (!font_result)
	font_result = _XQueryFont(dpy, fid, seq);
    UnlockDisplay(dpy);
    SyncHandle();
    return font_result;
}

XFontStruct *XQueryFont (
    register Display *dpy,
    Font fid)
{
    XFontStruct *font_result;
#ifdef USE_XF86BIGFONT
    XF86BigfontCodes *extcodes = _XF86BigfontCodes(dpy);
#endif

    LockDisplay(dpy);
    font_result = NULL;
#ifdef USE_XF86BIGFONT
    if (extcodes) {
	font_result = _XF86BigfontQueryFont(dpy, extcodes, fid, 0L);
    }
#endif
    if (!font_result)
	font_result = _XQueryFont(dpy, fid, 0L);
    UnlockDisplay(dpy);
    SyncHandle();
    return font_result;
}

int
XFreeFont(
    register Display *dpy,
    XFontStruct *fs)
{
    register xResourceReq *req;
    register _XExtension *ext;

    LockDisplay(dpy);
    /* call out to any extensions interested */
    for (ext = dpy->ext_procs; ext; ext = ext->next)
	if (ext->free_Font) (*ext->free_Font)(dpy, fs, &ext->codes);
    GetResReq (CloseFont, fs->fid, req);
    UnlockDisplay(dpy);
    SyncHandle();
    if (fs->per_char) {
#ifdef USE_XF86BIGFONT
	_XF86BigfontFreeFontMetrics(fs);
#else
	Xfree (fs->per_char);
#endif
    }
    _XFreeExtData(fs->ext_data);

    Xfree (fs->properties);
    Xfree (fs);
    return 1;
}


static XFontStruct *
_XQueryFont (
    register Display *dpy,
    Font fid,
    unsigned long seq)
{
    register XFontStruct *fs;
    unsigned long nbytes;
    unsigned long reply_left; /* unused data words left in reply buffer */
    xQueryFontReply reply;
    register xResourceReq *req;
    register _XExtension *ext;
    _XAsyncHandler async;
    _XAsyncErrorState async_state;

    if (seq) {
	async_state.min_sequence_number = seq;
	async_state.max_sequence_number = seq;
	async_state.error_code = BadName;
	async_state.major_opcode = X_OpenFont;
	async_state.minor_opcode = 0;
	async_state.error_count = 0;
	async.next = dpy->async_handlers;
	async.handler = _XAsyncErrorHandler;
	async.data = (XPointer)&async_state;
	dpy->async_handlers = &async;
    }
    GetResReq(QueryFont, fid, req);
    if (!_XReply (dpy, (xReply *) &reply,
       ((SIZEOF(xQueryFontReply) - SIZEOF(xReply)) >> 2), xFalse)) {
	if (seq)
	    DeqAsyncHandler(dpy, &async);
	return (XFontStruct *)NULL;
    }
    if (seq)
	DeqAsyncHandler(dpy, &async);
    reply_left = reply.length -
	((SIZEOF(xQueryFontReply) - SIZEOF(xReply)) >> 2);
    if (! (fs = Xmalloc (sizeof (XFontStruct)))) {
	_XEatDataWords(dpy, reply_left);
	return (XFontStruct *)NULL;
    }
    fs->ext_data 		= NULL;
    fs->fid 			= fid;
    fs->direction 		= reply.drawDirection;
    fs->min_char_or_byte2	= reply.minCharOrByte2;
    fs->max_char_or_byte2 	= reply.maxCharOrByte2;
    fs->min_byte1 		= reply.minByte1;
    fs->max_byte1 		= reply.maxByte1;
    fs->default_char 		= reply.defaultChar;
    fs->all_chars_exist 	= reply.allCharsExist;
    fs->ascent 			= cvtINT16toInt (reply.fontAscent);
    fs->descent 		= cvtINT16toInt (reply.fontDescent);

    /* XXX the next two statements won't work if short isn't 16 bits */
    fs->min_bounds = * (XCharStruct *) &reply.minBounds;
    fs->max_bounds = * (XCharStruct *) &reply.maxBounds;

    fs->n_properties = reply.nFontProps;
    /*
     * if no properties defined for the font, then it is bad
     * font, but shouldn't try to read nothing.
     */
    fs->properties = NULL;
    if (fs->n_properties > 0) {
	    /* nFontProps is a CARD16 */
	    nbytes = reply.nFontProps * SIZEOF(xFontProp);
	    if ((nbytes >> 2) <= reply_left) {
		fs->properties = Xmallocarray (reply.nFontProps,
                                               sizeof(XFontProp));
	    }
	    if (! fs->properties) {
		Xfree(fs);
		_XEatDataWords(dpy, reply_left);
		return (XFontStruct *)NULL;
	    }
	    _XRead32 (dpy, (long *)fs->properties, nbytes);
	    reply_left -= (nbytes >> 2);
    }
    /*
     * If no characters in font, then it is a bad font, but
     * shouldn't try to read nothing.
     */
    fs->per_char = NULL;
    if (reply.nCharInfos > 0){
	/* nCharInfos is a CARD32 */
	if (reply.nCharInfos < (INT_MAX / sizeof(XCharStruct))) {
	    nbytes = reply.nCharInfos * SIZEOF(xCharInfo);
	    if ((nbytes >> 2) <= reply_left) {
		fs->per_char = Xmallocarray (reply.nCharInfos,
                                             sizeof(XCharStruct));
	    }
	}
	if (! fs->per_char) {
	    Xfree(fs->properties);
	    Xfree(fs);
	    _XEatDataWords(dpy, reply_left);
	    return (XFontStruct *)NULL;
	}

	_XRead16 (dpy, (char *)fs->per_char, nbytes);
    }

    /* call out to any extensions interested */
    for (ext = dpy->ext_procs; ext; ext = ext->next)
	if (ext->create_Font) (*ext->create_Font)(dpy, fs, &ext->codes);
    return fs;
}

#ifdef USE_XF86BIGFONT

/* Magic cookie for finding the right XExtData structure on the display's
   extension list. */
static int XF86BigfontNumber = 1040697125;

static int
_XF86BigfontFreeCodes (
    XExtData *extension)
{
    /* Don't Xfree(extension->private_data) because it is on the same malloc
       chunk as extension. */
    /* Don't Xfree(extension->private_data->codes) because this is shared with
       the display's ext_procs list. */
    return 0;
}

static XF86BigfontCodes *
_XF86BigfontCodes (
    register Display *dpy)
{
    XEDataObject dpy_union;
    XExtData *pData;
    XF86BigfontCodes *pCodes;
    char *envval;

    dpy_union.display = dpy;

    /* If the server is known to support the XF86Bigfont extension,
     * return the extension codes. If the server is known to not support
     * the extension, don't bother checking again.
     */
    pData = XFindOnExtensionList(XEHeadOfExtensionList(dpy_union),
				 XF86BigfontNumber);
    if (pData)
	return (XF86BigfontCodes *) pData->private_data;

    pData = Xmalloc(sizeof(XExtData) + sizeof(XF86BigfontCodes));
    if (!pData) {
	/* Out of luck. */
	return (XF86BigfontCodes *) NULL;
    }

    /* See if the server supports the XF86Bigfont extension. */
    envval = getenv("XF86BIGFONT_DISABLE"); /* Let the user disable it. */
    if (envval != NULL && envval[0] != '\0')
	pCodes = NULL;
    else {
	XExtCodes *codes = XInitExtension(dpy, XF86BIGFONTNAME);
	if (codes == NULL)
	    pCodes = NULL;
	else {
	    pCodes = (XF86BigfontCodes *) &pData[1];
	    pCodes->codes = codes;
	}
    }
    pData->number = XF86BigfontNumber;
    pData->private_data = (XPointer) pCodes;
    pData->free_private = _XF86BigfontFreeCodes;
    XAddToExtensionList(XEHeadOfExtensionList(dpy_union), pData);
    if (pCodes) {
	int result;

	/* See if the server supports the XF86BigfontQueryFont request. */
	xXF86BigfontQueryVersionReply reply;
	register xXF86BigfontQueryVersionReq *req;

	LockDisplay(dpy);

	GetReq(XF86BigfontQueryVersion, req);
	req->reqType = pCodes->codes->major_opcode;
	req->xf86bigfontReqType = X_XF86BigfontQueryVersion;

	result = _XReply (dpy, (xReply *) &reply,
		(SIZEOF(xXF86BigfontQueryVersionReply) - SIZEOF(xReply)) >> 2,
		xFalse);

	UnlockDisplay(dpy);
    	SyncHandle();

	if(!result)
	    goto ignore_extension;

	/* No need to provide backward compatibility with version 1.0. It
	   was never widely distributed. */
	if (!(reply.majorVersion > 1
	      || (reply.majorVersion == 1 && reply.minorVersion >= 1)))
	    goto ignore_extension;

	pCodes->serverSignature = reply.signature;
	pCodes->serverCapabilities = reply.capabilities;
    }
    return pCodes;

  ignore_extension:
    /* No need to Xfree(pCodes) or Xfree(pCodes->codes), see
       _XF86BigfontFreeCodes comment. */
    pCodes = (XF86BigfontCodes *) NULL;
    pData->private_data = (XPointer) pCodes;
    return pCodes;
}

static int
_XF86BigfontFreeNop (
    XExtData *extension)
{
    return 0;
}

static XFontStruct *
_XF86BigfontQueryFont (
    register Display *dpy,
    XF86BigfontCodes *extcodes,
    Font fid,
    unsigned long seq)
{
    register XFontStruct *fs;
    unsigned long nbytes;
    unsigned long reply_left; /* unused data left in reply buffer */
    xXF86BigfontQueryFontReply reply;
    register xXF86BigfontQueryFontReq *req;
    register _XExtension *ext;
    _XAsyncHandler async1;
    _XAsyncErrorState async1_state;
    _XAsyncHandler async2;
    _XAsyncErrorState async2_state;

    if (seq) {
	async1_state.min_sequence_number = seq;
	async1_state.max_sequence_number = seq;
	async1_state.error_code = BadName;
	async1_state.major_opcode = X_OpenFont;
	async1_state.minor_opcode = 0;
	async1_state.error_count = 0;
	async1.next = dpy->async_handlers;
	async1.handler = _XAsyncErrorHandler;
	async1.data = (XPointer)&async1_state;
	dpy->async_handlers = &async1;
    }

    GetReq(XF86BigfontQueryFont, req);
    req->reqType = extcodes->codes->major_opcode;
    req->xf86bigfontReqType = X_XF86BigfontQueryFont;
    req->id = fid;
    req->flags = (extcodes->serverCapabilities & XF86Bigfont_CAP_LocalShm
		  ? XF86Bigfont_FLAGS_Shm : 0);

    /* The function _XQueryFont benefits from a "magic" error handler for
       BadFont coming from a X_QueryFont request. (See function _XReply.)
       We have to establish an error handler ourselves. */
    async2_state.min_sequence_number = dpy->request;
    async2_state.max_sequence_number = dpy->request;
    async2_state.error_code = BadFont;
    async2_state.major_opcode = extcodes->codes->major_opcode;
    async2_state.minor_opcode = X_XF86BigfontQueryFont;
    async2_state.error_count = 0;
    async2.next = dpy->async_handlers;
    async2.handler = _XAsyncErrorHandler;
    async2.data = (XPointer)&async2_state;
    dpy->async_handlers = &async2;

    if (!_XReply (dpy, (xReply *) &reply,
       ((SIZEOF(xXF86BigfontQueryFontReply) - SIZEOF(xReply)) >> 2), xFalse)) {
	DeqAsyncHandler(dpy, &async2);
	if (seq)
	    DeqAsyncHandler(dpy, &async1);
	return (XFontStruct *)NULL;
    }
    DeqAsyncHandler(dpy, &async2);
    if (seq)
	DeqAsyncHandler(dpy, &async1);
    reply_left = reply.length -
	((SIZEOF(xXF86BigfontQueryFontReply) - SIZEOF(xReply)) >> 2);
    if (! (fs = Xmalloc (sizeof (XFontStruct)))) {
	_XEatDataWords(dpy, reply_left);
	return (XFontStruct *)NULL;
    }
    fs->ext_data 		= NULL;
    fs->fid 			= fid;
    fs->direction 		= reply.drawDirection;
    fs->min_char_or_byte2	= reply.minCharOrByte2;
    fs->max_char_or_byte2 	= reply.maxCharOrByte2;
    fs->min_byte1 		= reply.minByte1;
    fs->max_byte1 		= reply.maxByte1;
    fs->default_char 		= reply.defaultChar;
    fs->all_chars_exist 	= reply.allCharsExist;
    fs->ascent 			= cvtINT16toInt (reply.fontAscent);
    fs->descent 		= cvtINT16toInt (reply.fontDescent);

    /* XXX the next two statements won't work if short isn't 16 bits */
    fs->min_bounds = * (XCharStruct *) &reply.minBounds;
    fs->max_bounds = * (XCharStruct *) &reply.maxBounds;

    fs->n_properties = reply.nFontProps;
    /*
     * if no properties defined for the font, then it is bad
     * font, but shouldn't try to read nothing.
     */
    fs->properties = NULL;
    if (fs->n_properties > 0) {
	/* nFontProps is a CARD16 */
	nbytes = reply.nFontProps * SIZEOF(xFontProp);
	if ((nbytes >> 2) <= reply_left) {
	    fs->properties = Xmallocarray (reply.nFontProps,
                                           sizeof(XFontProp));
	}
	if (! fs->properties) {
	    Xfree(fs);
	    _XEatDataWords(dpy, reply_left);
	    return (XFontStruct *)NULL;
	}
	_XRead32 (dpy, (long *)fs->properties, nbytes);
	reply_left -= (nbytes >> 2);
    }

    fs->per_char = NULL;
#ifndef LONG64
    /* compares each part to half the maximum, which should be far more than
       any real font needs, so the combined total doesn't overflow either */
    if (reply.nUniqCharInfos > ((ULONG_MAX / 2) / SIZEOF(xCharInfo)) ||
	reply.nCharInfos > ((ULONG_MAX / 2) / sizeof(CARD16))) {
	Xfree(fs->properties);
	Xfree(fs);
	_XEatDataWords(dpy, reply_left);
	return (XFontStruct *)NULL;
    }
#endif
    if (reply.nCharInfos > 0) {
	/* fprintf(stderr, "received font metrics, nCharInfos = %d, nUniqCharInfos = %d, shmid = %d\n", reply.nCharInfos, reply.nUniqCharInfos, reply.shmid); */
	if (reply.shmid == (CARD32)(-1)) {
	    xCharInfo* pUniqCI;
	    CARD16* pIndex2UniqIndex;
	    int i;

	    nbytes = reply.nUniqCharInfos * SIZEOF(xCharInfo)
	             + (reply.nCharInfos+1)/2 * 2 * sizeof(CARD16);
	    pUniqCI = Xmalloc (nbytes);
	    if (!pUniqCI) {
		Xfree(fs->properties);
		Xfree(fs);
		_XEatDataWords(dpy, reply_left);
		return (XFontStruct *)NULL;
	    }
	    if (! (fs->per_char = Xmallocarray (reply.nCharInfos,
                                                sizeof(XCharStruct)))) {
		Xfree(pUniqCI);
		Xfree(fs->properties);
		Xfree(fs);
		_XEatDataWords(dpy, reply_left);
		return (XFontStruct *)NULL;
	    }
	    _XRead16 (dpy, (char *) pUniqCI, nbytes);
	    pIndex2UniqIndex = (CARD16*) (pUniqCI + reply.nUniqCharInfos);
	    for (i = 0; i < reply.nCharInfos; i++) {
		if (pIndex2UniqIndex[i] >= reply.nUniqCharInfos) {
		    fprintf(stderr, "_XF86BigfontQueryFont: server returned wrong data\n");
		    Xfree(pUniqCI);
		    Xfree(fs->properties);
		    Xfree(fs);
		    return (XFontStruct *)NULL;
		}
		/* XXX the next statement won't work if short isn't 16 bits */
		fs->per_char[i] = * (XCharStruct *) &pUniqCI[pIndex2UniqIndex[i]];
	    }
	    Xfree(pUniqCI);
	} else {
#ifdef HAS_SHM
	    XExtData *pData;
	    XEDataObject fs_union;
	    char *addr;

	    pData = Xmalloc(sizeof(XExtData));
	    if (!pData) {
		Xfree(fs->properties);
		Xfree(fs);
		return (XFontStruct *)NULL;
	    }

	    /* In some cases (e.g. an ssh daemon forwarding an X session to
	       a remote machine) it is possible that the X server thinks we
	       are running on the same machine (because getpeername() and
	       LocalClient() cannot know about the forwarding) but we are
	       not really local. Therefore, when we attach the first shared
	       memory segment, we verify that we are on the same machine as
	       the X server by checking that 1. shmat() succeeds, 2. the
	       segment has a sufficient size, 3. it contains the X server's
	       signature. Then we set the CAP_VerifiedLocal bit to indicate
	       the verification was successful. */

	    if ((addr = shmat(reply.shmid, NULL, SHM_RDONLY)) == (char *)-1) {
		if (extcodes->serverCapabilities & CAP_VerifiedLocal)
		    fprintf(stderr, "_XF86BigfontQueryFont: could not attach shm segment\n");
	        Xfree(pData);
	        Xfree(fs->properties);
	        Xfree(fs);
		/* Stop requesting shared memory transport from now on. */
		extcodes->serverCapabilities &= ~ XF86Bigfont_CAP_LocalShm;
	        return (XFontStruct *)NULL;
	    }

	    if (!(extcodes->serverCapabilities & CAP_VerifiedLocal)) {
		struct shmid_ds buf;
		if (!(shmctl(reply.shmid, IPC_STAT, &buf) >= 0
		      && reply.nCharInfos < (INT_MAX / sizeof(XCharStruct))
		      && buf.shm_segsz >= reply.shmsegoffset + reply.nCharInfos * sizeof(XCharStruct) + sizeof(CARD32)
		      && *(CARD32 *)(addr + reply.shmsegoffset + reply.nCharInfos * sizeof(XCharStruct)) == extcodes->serverSignature)) {
		    shmdt(addr);
		    Xfree(pData);
		    Xfree(fs->properties);
		    Xfree(fs);
		    /* Stop requesting shared memory transport from now on. */
		    extcodes->serverCapabilities &= ~ XF86Bigfont_CAP_LocalShm;
		    return (XFontStruct *)NULL;
		}
		extcodes->serverCapabilities |= CAP_VerifiedLocal;
	    }

	    pData->number = XF86BigfontNumber;
	    pData->private_data = (XPointer) addr;
	    pData->free_private = _XF86BigfontFreeNop;
	    fs_union.font = fs;
	    XAddToExtensionList(XEHeadOfExtensionList(fs_union), pData);

	    fs->per_char = (XCharStruct *) (addr + reply.shmsegoffset);
#else
	    fprintf(stderr, "_XF86BigfontQueryFont: try recompiling libX11 with HasShm, Xserver has shm support\n");
	    if (fs->properties) Xfree(fs->properties);
	    Xfree(fs);
	    /* Stop requesting shared memory transport from now on. */
	    extcodes->serverCapabilities &= ~ XF86Bigfont_CAP_LocalShm;
	    return (XFontStruct *)NULL;
#endif
	}
    }

    /* call out to any extensions interested */
    for (ext = dpy->ext_procs; ext; ext = ext->next)
	if (ext->create_Font) (*ext->create_Font)(dpy, fs, &ext->codes);
    return fs;
}

void
_XF86BigfontFreeFontMetrics (XFontStruct *fs)
{
#ifdef HAS_SHM
    XExtData *pData;
    XEDataObject fs_union;

    fs_union.font = fs;
    if ((pData = XFindOnExtensionList(XEHeadOfExtensionList(fs_union),
				      XF86BigfontNumber)))
	shmdt ((char *) pData->private_data);
    else
	Xfree (fs->per_char);
#else
    Xfree (fs->per_char);
#endif
}

#endif /* USE_XF86BIGFONT */

int _XF86LoadQueryLocaleFont(
   Display *dpy,
   _Xconst char *name,
   XFontStruct **xfp,
   Font *fidp)
{
    size_t l;
    const char *charset, *p;
    char buf[256];
    XFontStruct *fs;
    XLCd lcd;

    if (!name)
	return 0;
    l = strlen(name);
    if (l < 2 || name[l - 1] != '*' || name[l - 2] != '-' || l >= USHRT_MAX)
	return 0;
    charset = NULL;
    /* next three lines stolen from _XkbGetCharset() */
    lcd = _XlcCurrentLC();
    if ((lcd = _XlcCurrentLC()) != 0)
	charset = XLC_PUBLIC(lcd, encoding_name);
    if (!charset || (p = strrchr(charset, '-')) == 0 || p == charset || p[1] == 0 || (p[1] == '*' && p[2] == 0)) {
	/* prefer latin1 if no encoding found */
	charset = "ISO8859-1";
	p = charset + 7;
    }
    if (l - 2 < p - charset)
	return 0;
    if (_XlcNCompareISOLatin1(name + l - 2 - (p - charset), charset, p - charset))
	return 0;
    if (strlen(p + 1) + l - 1 >= sizeof(buf) - 1)
	return 0;
    strcpy(buf, name);
    strcpy(buf + l - 1, p + 1);
    fs = XLoadQueryFont(dpy, buf);
    if (!fs)
	return 0;
    if (xfp) {
	*xfp = fs;
	if (fidp)
	    *fidp = fs->fid;
    } else if (fidp) {
	if (fs->per_char) {
#ifdef USE_XF86BIGFONT
	    _XF86BigfontFreeFontMetrics(fs);
#else
	    Xfree (fs->per_char);
#endif
	}
	_XFreeExtData(fs->ext_data);

	Xfree (fs->properties);
	*fidp = fs->fid;
	Xfree (fs);
    } else {
	XFreeFont(dpy, fs);
    }
    return 1;
}
