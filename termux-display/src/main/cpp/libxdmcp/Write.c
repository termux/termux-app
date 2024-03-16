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
XdmcpWriteHeader (
    XdmcpBufferPtr  buffer,
    const XdmcpHeaderPtr  header)
{
    BYTE    *newData;

    if ((int)buffer->size < 6 + (int)header->length)
    {
	newData = (BYTE *) malloc(XDM_MAX_MSGLEN * sizeof (BYTE));
	if (!newData)
	    return FALSE;
	free((unsigned long *)(buffer->data));
	buffer->data = newData;
	buffer->size = XDM_MAX_MSGLEN;
    }
    buffer->pointer = 0;
    if (!XdmcpWriteCARD16 (buffer, header->version))
	return FALSE;
    if (!XdmcpWriteCARD16 (buffer, header->opcode))
	return FALSE;
    if (!XdmcpWriteCARD16 (buffer, header->length))
	return FALSE;
    return TRUE;
}

int
XdmcpWriteARRAY8 (XdmcpBufferPtr buffer, const ARRAY8Ptr array)
{
    int	i;

    if (!XdmcpWriteCARD16 (buffer, array->length))
	return FALSE;
    for (i = 0; i < (int)array->length; i++)
	if (!XdmcpWriteCARD8 (buffer, array->data[i]))
	    return FALSE;
    return TRUE;
}

int
XdmcpWriteARRAY16 (XdmcpBufferPtr buffer, const ARRAY16Ptr array)
{
    int	i;

    if (!XdmcpWriteCARD8 (buffer, array->length))
	return FALSE;
    for (i = 0; i < (int)array->length; i++)
	if (!XdmcpWriteCARD16 (buffer, array->data[i]))
	    return FALSE;
    return TRUE;
}

int
XdmcpWriteARRAY32 (XdmcpBufferPtr buffer, const ARRAY32Ptr array)
{
    int	i;

    if (!XdmcpWriteCARD8 (buffer, array->length))
	return FALSE;
    for (i = 0; i < (int)array->length; i++)
	if (!XdmcpWriteCARD32 (buffer, array->data[i]))
	    return FALSE;
    return TRUE;
}

int
XdmcpWriteARRAYofARRAY8 (XdmcpBufferPtr buffer, ARRAYofARRAY8Ptr array)
{
    int	i;

    if (!XdmcpWriteCARD8 (buffer, array->length))
	return FALSE;
    for (i = 0; i < (int)array->length; i++)
	if (!XdmcpWriteARRAY8 (buffer, &array->data[i]))
	    return FALSE;
    return TRUE;
}

int
XdmcpWriteCARD8 (
    XdmcpBufferPtr  buffer,
    unsigned	    value)
{
    if (buffer->pointer >= buffer->size)
	return FALSE;
    buffer->data[buffer->pointer++] = (BYTE) value;
    return TRUE;
}

int
XdmcpWriteCARD16 (
    XdmcpBufferPtr  buffer,
    unsigned	    value)
{
    if (!XdmcpWriteCARD8 (buffer, value >> 8))
	return FALSE;
    if (!XdmcpWriteCARD8 (buffer, value & 0xff))
	return FALSE;
    return TRUE;
}

int
XdmcpWriteCARD32 (
    XdmcpBufferPtr  buffer,
    unsigned	    value)
{
    if (!XdmcpWriteCARD8 (buffer, value >> 24))
	return FALSE;
    if (!XdmcpWriteCARD8 (buffer, (value >> 16) & 0xff))
	return FALSE;
    if (!XdmcpWriteCARD8 (buffer, (value >> 8) & 0xff))
	return FALSE;
    if (!XdmcpWriteCARD8 (buffer, value & 0xff))
	return FALSE;
    return TRUE;
}
