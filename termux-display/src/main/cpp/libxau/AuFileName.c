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
#include <assert.h>
#include <stdlib.h>

static char *buf = NULL;

static void
free_filename_buffer(void)
{
    free(buf);
    buf = NULL;
}

char *
XauFileName (void)
{
    const char *slashDotXauthority = "/.Xauthority";
    char    *name;
    static size_t	bsize;
    static int atexit_registered = 0;
#ifdef WIN32
    char    dir[128];
#endif
    size_t  size;

    if ((name = getenv ("XAUTHORITY")))
	return name;
    name = getenv ("HOME");
    if (!name) {
#ifdef WIN32
	if ((name = getenv("USERNAME"))) {
	    snprintf(dir, sizeof(dir), "/users/%s", name);
	    name = dir;
	}
	if (!name)
#endif
	return NULL;
    }
    size = strlen (name) + strlen(&slashDotXauthority[1]) + 2;
    if ((size > bsize) || (buf == NULL)) {
	free (buf);
        assert(size > 0);
	buf = malloc (size);
	if (!buf) {
	    bsize = 0;
	    return NULL;
	}

        if (!atexit_registered) {
            atexit(free_filename_buffer);
            atexit_registered = 1;
        }

	bsize = size;
    }
    snprintf (buf, bsize, "%s%s", name,
              slashDotXauthority + (name[0] == '/' && name[1] == '\0' ? 1 : 0));
    return buf;
}
