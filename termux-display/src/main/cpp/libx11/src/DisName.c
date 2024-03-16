
/*

Copyright 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/* XDisplayName.c */
/*
 * Returns the name of the display XOpenDisplay would use.  This is better
 * than just printing the "display" variable in a program because that
 * could be NULL and/or there could be an environment variable set.
 * This makes it easier for programmers to provide meaningful error
 * messages.
 *
 *
 * For example, this is used in XOpenDisplay() as
 *	strncpy( displaybuf, XDisplayName( display ), sizeof(displaybuf) );
 *      if ( *displaybuf == '\0' ) return( NULL );
 *  This check is actually unnecessary because the next thing is an index()
 *  call looking for a ':' which will fail and we'll return(NULL).
 */
/* Written at Waterloo - JMSellens */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "Xlib.h"

char *
XDisplayName(
    _Xconst char *display)
{
    char *d;
    if ( display != (char *)NULL && *display != '\0' )
	return( (char *)display );
    if ( (d = getenv( "DISPLAY" )) != (char *)NULL )
	return( d );
    return( (char *) "" );
}
