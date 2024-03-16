
/***********************************************************

Copyright 1987, 1988, 1998  The Open Group

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


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* XrmParseCommand()

   Parse command line and store argument values into resource database

   Allows any un-ambiguous abbreviation for an option name, but requires
   that the table be ordered with any options that are prefixes of
   other options appearing before the longer version in the table.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include <X11/Xresource.h>
#include <stdio.h>


static void _XReportParseError(XrmOptionDescRec *arg, const char *msg)
{
    (void) fprintf(stderr, "Error parsing argument \"%s\" (%s); %s\n",
		   arg->option, arg->specifier, msg);
    exit(1);
}

void
XrmParseCommand(
    XrmDatabase		*pdb,		/* data base */
    register XrmOptionDescList options, /* pointer to table of valid options */
    int			num_options,	/* number of options		     */
    _Xconst char	*prefix,	/* name to prefix resources with     */
    int			*argc,		/* address of argument count 	     */
    char		**argv)		/* argument list (command line)	     */
{
    int 		foundOption;
    char		**argsave;
    register int	i, myargc;
    XrmBinding		bindings[100];
    XrmQuark		quarks[100];
    XrmBinding		*start_bindings;
    XrmQuark		*start_quarks;
    char		*optP, *argP = NULL, optchar, argchar = 0;
    int			matches;
    enum {DontCare, Check, NotSorted, Sorted} table_is_sorted;
    char		**argend;

#define PutCommandResource(value_str)				\
    {								\
    XrmStringToBindingQuarkList(				\
	options[i].specifier, start_bindings, start_quarks);    \
    XrmQPutStringResource(pdb, bindings, quarks, value_str);    \
    } /* PutCommandResource */

    myargc = (*argc);
    argend = argv + myargc;
    argsave = ++argv;

    /* Initialize bindings/quark list with prefix (typically app name). */
    quarks[0] = XrmStringToName(prefix);
    bindings[0] = XrmBindTightly;
    start_quarks = quarks+1;
    start_bindings = bindings+1;

    table_is_sorted = (myargc > 2) ? Check : DontCare;
    for (--myargc; myargc > 0; --myargc, ++argv) {
	foundOption = False;
	matches = 0;
	for (i=0; i < num_options; ++i) {
	    /* checking the sort order first insures we don't have to
	       re-do the check if the arg hits on the last entry in
	       the table.  Useful because usually '=' is the last entry
	       and users frequently specify geometry early in the command */
	    if (table_is_sorted == Check && i > 0 &&
		strcmp(options[i].option, options[i-1].option) < 0) {
		table_is_sorted = NotSorted;
	    }
	    for (argP = *argv, optP = options[i].option;
		 (optchar = *optP++) &&
		 (argchar = *argP++) &&
		 argchar == optchar;);
	    if (!optchar) {
		if (!*argP ||
		    options[i].argKind == XrmoptionStickyArg ||
		    options[i].argKind == XrmoptionIsArg) {
		    /* give preference to exact matches, StickyArg and IsArg */
		    matches = 1;
		    foundOption = i;
		    break;
		}
	    }
	    else if (!argchar) {
		/* may be an abbreviation for this option */
		matches++;
		foundOption = i;
	    }
	    else if (table_is_sorted == Sorted && optchar > argchar) {
		break;
	    }
	    if (table_is_sorted == Check && i > 0 &&
		strcmp(options[i].option, options[i-1].option) < 0) {
		table_is_sorted = NotSorted;
	    }
	}
	if (table_is_sorted == Check && i >= (num_options-1))
	    table_is_sorted = Sorted;
	if (matches == 1) {
		i = foundOption;
		switch (options[i].argKind){
		case XrmoptionNoArg:
		    --(*argc);
		    PutCommandResource(options[i].value);
		    break;

		case XrmoptionIsArg:
		    --(*argc);
		    PutCommandResource(*argv);
		    break;

		case XrmoptionStickyArg:
		    --(*argc);
		    PutCommandResource(argP);
		    break;

		case XrmoptionSepArg:
		    if (myargc > 1) {
			++argv; --myargc; --(*argc); --(*argc);
			PutCommandResource(*argv);
		    } else
			(*argsave++) = (*argv);
		    break;

		case XrmoptionResArg:
		    if (myargc > 1) {
			++argv; --myargc; --(*argc); --(*argc);
			XrmPutLineResource(pdb, *argv);
		    } else
			(*argsave++) = (*argv);
		    break;

		case XrmoptionSkipArg:
		    if (myargc > 1) {
			--myargc;
			(*argsave++) = (*argv++);
		    }
		    (*argsave++) = (*argv);
		    break;

		case XrmoptionSkipLine:
		    for (; myargc > 0; myargc--)
			(*argsave++) = (*argv++);
		    break;

		case XrmoptionSkipNArgs:
		    {
			register int j = 1 + (long) options[i].value;

			if (j > myargc) j = myargc;
			for (; j > 0; j--) {
			    (*argsave++) = (*argv++);
			    myargc--;
			}
			argv--;		/* went one too far before */
			myargc++;
		    }
		    break;

		default:
		    _XReportParseError (&options[i], "unknown kind");
		    break;
		}
	}
	else
	    (*argsave++) = (*argv);  /*compress arglist*/
    }

    if (argsave < argend)
	(*argsave)=NULL; /* put NULL terminator on compressed argv */
}
