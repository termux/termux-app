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
#include <stdlib.h>

int
XdmcpReadHeader (XdmcpBufferPtr buffer, XdmcpHeaderPtr header)
{
    if (XdmcpReadCARD16 (buffer, &header->version) &&
        XdmcpReadCARD16 (buffer, &header->opcode) &&
	XdmcpReadCARD16 (buffer, &header->length))
	return TRUE;
    return FALSE;
}

int
XdmcpReadRemaining (const XdmcpBufferPtr  buffer)
{
    return buffer->count - buffer->pointer;
}

int
XdmcpReadARRAY8 (XdmcpBufferPtr buffer, ARRAY8Ptr array)
{
    int	    i;

    /*
     * When returning FALSE, guarantee that array->data = 0.
     * This allows the user to safely call XdmcpDisposeARRAY8(array)
     * regardless of the return value below.
     * Note that XdmcpDisposeARRAY*(array) will call free(array->data),
     * so we must guarantee that array->data is NULL or a malloced pointer.
     */
    if (!XdmcpReadCARD16 (buffer, &array->length)) {
 	array->data = NULL;
	return FALSE;
    }
    if (!array->length)
    {
	array->data = NULL;
	return TRUE;
    }
    array->data = (CARD8 *) malloc(array->length * sizeof (CARD8));
    if (!array->data)
	return FALSE;
    for (i = 0; i < (int)array->length; i++)
    {
	if (!XdmcpReadCARD8 (buffer, &array->data[i]))
	{
	    free(array->data);
	    array->data = NULL;
	    array->length = 0;
	    return FALSE;
	}
    }
    return TRUE;
}

int
XdmcpReadARRAY16 (XdmcpBufferPtr buffer, ARRAY16Ptr array)
{
    int	    i;

    /*
     * When returning FALSE, guarantee that array->data = 0.
     * This allows the user to safely call XdmcpDisposeARRAY16(array)
     * regardless of the return value below.
     * Note that XdmcpDisposeARRAY*(array) will call free(array->data),
     * so we must guarantee that array->data is NULL or a malloced pointer.
     */
    if (!XdmcpReadCARD8 (buffer, &array->length)) {
	array->data = NULL;
	return FALSE;
    }
    if (!array->length)
    {
	array->data = NULL;
	return TRUE;
    }
    array->data = (CARD16 *) malloc(array->length * sizeof (CARD16));
    if (!array->data)
	return FALSE;
    for (i = 0; i < (int)array->length; i++)
    {
	if (!XdmcpReadCARD16 (buffer, &array->data[i]))
	{
	    free(array->data);
	    array->data = NULL;
	    array->length = 0;
	    return FALSE;
	}
    }
    return TRUE;
}

int
XdmcpReadARRAY32 (XdmcpBufferPtr buffer, ARRAY32Ptr array)
{
    int	    i;

    /*
     * When returning FALSE, guarantee that array->data = 0.
     * This allows the user to safely call XdmcpDisposeARRAY32(array)
     * regardless of the return value below.
     * Note that XdmcpDisposeARRAY*(array) will call free(array->data),
     * so we must guarantee that array->data is NULL or a malloced pointer.
     */
    if (!XdmcpReadCARD8 (buffer, &array->length)) {
	array->data = NULL;
	return FALSE;
    }
    if (!array->length)
    {
	array->data = NULL;
	return TRUE;
    }
    array->data = (CARD32 *) malloc(array->length * sizeof (CARD32));
    if (!array->data)
	return FALSE;
    for (i = 0; i < (int)array->length; i++)
    {
	if (!XdmcpReadCARD32 (buffer, &array->data[i]))
	{
	    free(array->data);
	    array->data = NULL;
	    array->length = 0;
	    return FALSE;
	}
    }
    return TRUE;
}

int
XdmcpReadARRAYofARRAY8 (XdmcpBufferPtr buffer, ARRAYofARRAY8Ptr array)
{
    CARD8    i;

    /*
     * When returning FALSE, guarantee that array->data = 0.
     * This allows the user to safely call XdmcpDisposeARRAYofARRAY8(array)
     * regardless of the return value below.
     * Note that XdmcpDisposeARRAY*(array) will call free(array->data),
     * so we must guarantee that array->data is NULL or a malloced pointer.
     */
    if (!XdmcpReadCARD8 (buffer, &array->length)) {
	array->data = NULL;
	return FALSE;
    }
    if (!array->length)
    {
	array->data = NULL;
	return TRUE;
    }
    array->data = (ARRAY8 *) malloc(array->length * sizeof (ARRAY8));
    if (!array->data)
	return FALSE;
    for (i = 0; i < array->length; i++)
    {
	if (!XdmcpReadARRAY8 (buffer, &array->data[i]))
	{
	    /*
	     * We must free all of the arrays allocated thus far in the loop
	     * and free array->data and finally set array->data = 0;
	     * The easiest way to do this is to reset the length and call
	     * XdmcpDisposeARRAYofARRAY8(array).
	     */
	    array->length = i;
	    XdmcpDisposeARRAYofARRAY8(array);
	    return FALSE;
	}
    }
    return TRUE;
}

int
XdmcpReadCARD8 (XdmcpBufferPtr buffer, CARD8Ptr valuep)
{
    if (buffer->pointer >= buffer->count)
	return FALSE;
    *valuep = (CARD8) buffer->data[buffer->pointer++];
    return TRUE;
}

int
XdmcpReadCARD16 (XdmcpBufferPtr buffer, CARD16Ptr valuep)
{
    CARD8   high, low;

    if (XdmcpReadCARD8 (buffer, &high) &&
        XdmcpReadCARD8 (buffer, &low))
    {
	*valuep = (((CARD16) high) << 8) | ((CARD16) low);
	return TRUE;
    }
    return FALSE;
}

int
XdmcpReadCARD32 (XdmcpBufferPtr buffer, CARD32Ptr valuep)
{
    CARD8   byte0, byte1, byte2, byte3;
    if (XdmcpReadCARD8 (buffer, &byte0) &&
        XdmcpReadCARD8 (buffer, &byte1) &&
	XdmcpReadCARD8 (buffer, &byte2) &&
	XdmcpReadCARD8 (buffer, &byte3))
    {
	*valuep = (((CARD32) byte0) << 24) |
		  (((CARD32) byte1) << 16) |
		  (((CARD32) byte2) << 8) |
		  (((CARD32) byte3));
	return TRUE;
    }
    return FALSE;
}
