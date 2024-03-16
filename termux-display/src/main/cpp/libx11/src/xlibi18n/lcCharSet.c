/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "XlcPublic.h"
#include "XlcPubI.h"

/* The list of all known XlcCharSets. They are identified by their name. */

typedef struct _XlcCharSetListRec {
    XlcCharSet charset;
    struct _XlcCharSetListRec *next;
} XlcCharSetListRec, *XlcCharSetList;

static XlcCharSetList charset_list = NULL;

/* Returns the charset with the given name (including side suffix).
   Returns NULL if not found. */
XlcCharSet
_XlcGetCharSet(
    const char *name)
{
    XlcCharSetList list;
    XrmQuark xrm_name;

    xrm_name = XrmStringToQuark(name);

    for (list = charset_list; list; list = list->next) {
	if (xrm_name == list->charset->xrm_name)
	    return (XlcCharSet) list->charset;
    }

    return (XlcCharSet) NULL;
}

/* Returns the charset with the given encoding (no side suffix) and
   responsible for at least the given side (XlcGL or XlcGR).
   Returns NULL if not found. */
XlcCharSet
_XlcGetCharSetWithSide(
    const char *encoding_name,
    XlcSide side)
{
    XlcCharSetList list;
    XrmQuark xrm_encoding_name;

    xrm_encoding_name = XrmStringToQuark(encoding_name);

    for (list = charset_list; list; list = list->next) {
	if (list->charset->xrm_encoding_name == xrm_encoding_name
	    && (list->charset->side == XlcGLGR || list->charset->side == side))
	    return (XlcCharSet) list->charset;
    }

    return (XlcCharSet) NULL;
}

/* Registers an XlcCharSet in the list of character sets.
   Returns True if successful. */
Bool
_XlcAddCharSet(
    XlcCharSet charset)
{
    XlcCharSetList list;

    if (_XlcGetCharSet(charset->name))
	return False;

    list = Xmalloc(sizeof(XlcCharSetListRec));
    if (list == NULL)
	return False;

    list->charset = charset;
    list->next = charset_list;
    charset_list = list;

    return True;
}

/* List of resources for XlcCharSet. */
static XlcResource resources[] = {
    { XlcNName, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, name), XlcGetMask },
    { XlcNEncodingName, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, encoding_name), XlcGetMask },
    { XlcNSide, NULLQUARK, sizeof(XlcSide),
      XOffsetOf(XlcCharSetRec, side), XlcGetMask },
    { XlcNCharSize, NULLQUARK, sizeof(int),
      XOffsetOf(XlcCharSetRec, char_size), XlcGetMask },
    { XlcNSetSize, NULLQUARK, sizeof(int),
      XOffsetOf(XlcCharSetRec, set_size), XlcGetMask },
    { XlcNControlSequence, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, ct_sequence), XlcGetMask }
};

/* Retrieves a number of attributes of an XlcCharSet.
   Return NULL if successful, otherwise the name of the first argument
   specifying a nonexistent attribute. */
static char *
get_values(
    XlcCharSet charset,
    XlcArgList args,
    int num_args)
{
    if (resources[0].xrm_name == NULLQUARK)
	_XlcCompileResourceList(resources, XlcNumber(resources));

    return _XlcGetValues((XPointer) charset, resources, XlcNumber(resources),
			 args, num_args, XlcGetMask);
}

/* Retrieves a number of attributes of an XlcCharSet.
   Return NULL if successful, otherwise the name of the first argument
   specifying a nonexistent attribute. */
char *
_XlcGetCSValues(XlcCharSet charset, ...)
{
    va_list var;
    XlcArgList args;
    char *ret;
    int num_args;

    va_start(var, charset);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    va_start(var, charset);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (char *) NULL;

    ret = get_values(charset, args, num_args);

    Xfree(args);

    return ret;
}

/* Creates a new XlcCharSet, given its name (including side suffix) and
   Compound Text ESC sequence (normally at most 4 bytes). */
XlcCharSet
_XlcCreateDefaultCharSet(
    const char *name,
    const char *ct_sequence)
{
    XlcCharSet charset;
    size_t name_len, ct_sequence_len;
    const char *colon;
    char *tmp;

    charset = Xcalloc(1, sizeof(XlcCharSetRec));
    if (charset == NULL)
	return (XlcCharSet) NULL;

    name_len = strlen(name);
    ct_sequence_len = strlen(ct_sequence);

    /* Fill in name and xrm_name.  */
    tmp = Xmalloc(name_len + 1 + ct_sequence_len + 1);
    if (tmp == NULL) {
	Xfree(charset);
	return (XlcCharSet) NULL;
    }
    memcpy(tmp, name, name_len+1);
    charset->name = tmp;
    charset->xrm_name = XrmStringToQuark(charset->name);

    /* Fill in encoding_name and xrm_encoding_name.  */
    if ((colon = strchr(charset->name, ':')) != NULL) {
        size_t length = (size_t)(colon - charset->name);
        char *encoding_tmp = Xmalloc(length + 1);
        if (encoding_tmp == NULL) {
            Xfree((char *) charset->name);
            Xfree(charset);
            return (XlcCharSet) NULL;
        }
        memcpy(encoding_tmp, charset->name, length);
        encoding_tmp[length] = '\0';
        charset->encoding_name = encoding_tmp;
        charset->xrm_encoding_name = XrmStringToQuark(charset->encoding_name);
    } else {
        charset->encoding_name = charset->name;
        charset->xrm_encoding_name = charset->xrm_name;
    }

    /* Fill in ct_sequence.  */
    tmp += name_len + 1;
    memcpy(tmp, ct_sequence, ct_sequence_len+1);
    charset->ct_sequence = tmp;

    /* Fill in side, char_size, set_size. */
    if (!_XlcParseCharSet(charset))
        /* If ct_sequence is not usable in Compound Text, remove it. */
        charset->ct_sequence = "";

    return (XlcCharSet) charset;
}
