/*
Copyright 1989, 1998  The Open Group

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
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xos.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xdmcp.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * This variant of malloc does not return NULL if zero size is passed into.
 */
static void *
xmalloc(size_t size)
{
    return malloc(size ? size : 1);
}

/*
 * This variant of calloc does not return NULL if zero count is passed into.
 */
static void *
xcalloc(size_t n, size_t size)
{
    return calloc(n ? n : 1, size);
}

/*
 * This variant of realloc does not return NULL if zero size is passed into
 */
static void *
xrealloc(void *ptr, size_t size)
{
    return realloc(ptr, size ? size : 1);
}

int
XdmcpAllocARRAY8 (ARRAY8Ptr array, int length)
{
    /* length defined in ARRAY8 struct is a CARD16 (not CARD8 like the rest) */
    if ((length > UINT16_MAX) || (length < 0))
        array->data = NULL;
    else
        array->data = xmalloc(length * sizeof (CARD8));

    if (array->data == NULL) {
	array->length = 0;
	return FALSE;
    }
    array->length = (CARD16) length;
    return TRUE;
}

int
XdmcpAllocARRAY16 (ARRAY16Ptr array, int length)
{
    /* length defined in ARRAY16 struct is a CARD8 */
    if ((length > UINT8_MAX) || (length < 0))
        array->data = NULL;
    else
        array->data = xmalloc(length * sizeof (CARD16));

    if (array->data == NULL) {
	array->length = 0;
	return FALSE;
    }
    array->length = (CARD8) length;
    return TRUE;
}

int
XdmcpAllocARRAY32 (ARRAY32Ptr array, int length)
{
    /* length defined in ARRAY32 struct is a CARD8 */
    if ((length > UINT8_MAX) || (length < 0))
        array->data = NULL;
    else
        array->data = xmalloc(length * sizeof (CARD32));

    if (array->data == NULL) {
	array->length = 0;
	return FALSE;
    }
    array->length = (CARD8) length;
    return TRUE;
}

int
XdmcpAllocARRAYofARRAY8 (ARRAYofARRAY8Ptr array, int length)
{
    /* length defined in ARRAYofARRAY8 struct is a CARD8 */
    if ((length > UINT8_MAX) || (length < 0))
        array->data = NULL;
    else
        /*
         * Use calloc to ensure the pointers are cleared out so we
         * don't try to free garbage if XdmcpDisposeARRAYofARRAY8()
         * is called before the caller sets them to valid pointers.
         */
        array->data = xcalloc(length, sizeof (ARRAY8));

    if (array->data == NULL) {
	array->length = 0;
	return FALSE;
    }
    array->length = (CARD8) length;
    return TRUE;
}

int
XdmcpARRAY8Equal (const ARRAY8Ptr array1, const ARRAY8Ptr array2)
{
    if (array1->length != array2->length)
	return FALSE;
    if (memcmp(array1->data, array2->data, array1->length) != 0)
	return FALSE;
    return TRUE;
}

int
XdmcpCopyARRAY8 (const ARRAY8Ptr src, ARRAY8Ptr dst)
{
    if (!XdmcpAllocARRAY8(dst, src->length))
	return FALSE;
    memcpy(dst->data, src->data, src->length * sizeof (CARD8));
    return TRUE;
}

int
XdmcpReallocARRAY8 (ARRAY8Ptr array, int length)
{
    CARD8Ptr	newData;

    /* length defined in ARRAY8 struct is a CARD16 (not CARD8 like the rest) */
    if ((length > UINT16_MAX) || (length < 0))
	return FALSE;

    newData = (CARD8Ptr) xrealloc(array->data, length * sizeof (CARD8));
    if (!newData)
	return FALSE;
    array->length = (CARD16) length;
    array->data = newData;
    return TRUE;
}

int
XdmcpReallocARRAYofARRAY8 (ARRAYofARRAY8Ptr array, int length)
{
    ARRAY8Ptr	newData;

    /* length defined in ARRAYofARRAY8 struct is a CARD8 */
    if ((length > UINT8_MAX) || (length < 0))
	return FALSE;

    newData = (ARRAY8Ptr) xrealloc(array->data, length * sizeof (ARRAY8));
    if (!newData)
	return FALSE;
    if (length > array->length)
        memset(newData + array->length, 0,
               (length - array->length) * sizeof (ARRAY8));
    array->length = (CARD8) length;
    array->data = newData;
    return TRUE;
}

int
XdmcpReallocARRAY16 (ARRAY16Ptr array, int length)
{
    CARD16Ptr	newData;

    /* length defined in ARRAY16 struct is a CARD8 */
    if ((length > UINT8_MAX) || (length < 0))
	return FALSE;
    newData = (CARD16Ptr) xrealloc(array->data, length * sizeof (CARD16));
    if (!newData)
	return FALSE;
    array->length = (CARD8) length;
    array->data = newData;
    return TRUE;
}

int
XdmcpReallocARRAY32 (ARRAY32Ptr array, int length)
{
    CARD32Ptr	newData;

    /* length defined in ARRAY32 struct is a CARD8 */
    if ((length > UINT8_MAX) || (length < 0))
	return FALSE;

    newData = (CARD32Ptr) xrealloc(array->data, length * sizeof (CARD32));
    if (!newData)
	return FALSE;
    array->length = (CARD8) length;
    array->data = newData;
    return TRUE;
}

void
XdmcpDisposeARRAY8 (ARRAY8Ptr array)
{
    free(array->data);
    array->length = 0;
    array->data = NULL;
}

void
XdmcpDisposeARRAY16 (ARRAY16Ptr array)
{
    free(array->data);
    array->length = 0;
    array->data = NULL;
}

void
XdmcpDisposeARRAY32 (ARRAY32Ptr array)
{
    free(array->data);
    array->length = 0;
    array->data = NULL;
}

void
XdmcpDisposeARRAYofARRAY8 (ARRAYofARRAY8Ptr array)
{
    if (array->data != NULL) {
	for (unsigned int i = 0; i < (unsigned int) array->length; i++)
	    XdmcpDisposeARRAY8 (&array->data[i]);
	free(array->data);
    }
    array->length = 0;
    array->data = NULL;
}
