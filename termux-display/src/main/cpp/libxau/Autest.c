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
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
    Xauth   test_data;
    char    defname[] = "XAU-TEST-1";
    char    defdata[] = "Do not begin the test until instructed to do so.";
    char    empty[] = "";
    char    *name = defname;
    char    *data = defdata;
    char    *file = NULL;
    int	    state = 0;
    FILE    *output;

    while (*++argv) {
	if (!strcmp (*argv, "-file"))
	    file = *++argv;
	else if (state == 0) {
	    name = *argv;
	    ++state;
	} else if (state == 1) {
	    data = *argv;
	    ++state;
	}
    }
    test_data.family = 0;
    test_data.address_length = 0;
    test_data.address = empty;
    test_data.number_length = 0;
    test_data.number = empty;
    test_data.name_length = strlen (name);
    test_data.name = name;
    test_data.data_length = strlen (data);
    test_data.data = data;
    if (!file) {
	output = tmpfile();
    } else {
	output = fopen (file, "w");
    }
    if (output) {
	state = XauWriteAuth (output, &test_data);
	fclose (output);
    }
    return (state = 1) ? 0 : 1;
}
