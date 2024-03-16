/*

Copyright 1988, 1998  The Open Group

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
#include <X11/Xauth.h>
#include <X11/Xos.h>

int
XauUnlockAuth (
_Xconst char *file_name)
{
#ifndef WIN32
    char	creat_name[1025];
#endif
    char	link_name[1025];

    if (strlen (file_name) > 1022)
	return 0;
#ifndef WIN32
    snprintf (creat_name, sizeof(creat_name), "%s-c", file_name);
#endif
    snprintf (link_name, sizeof(link_name), "%s-l", file_name);
    /*
     * I think this is the correct order
     */
#ifndef WIN32
    (void) remove (creat_name);
#endif
    (void) remove (link_name);

    return 1;
}
