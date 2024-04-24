/*

Copyright 1989, 1998  The Open Group

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

/*
 *
 *                                 Global data
 *
 * This file should contain only those objects which must be predefined.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlibint.h>


/*
 * If possible, it is useful to have the global data default to a null value.
 * Some shared library implementations are *much* happier if there isn't any
 * global initialized data.
 */
#ifdef NULL_NOT_ZERO			/* then need to initialize */
#define SetZero(t,var,z) t var = z
#else
#define SetZero(t,var,z) t var
#endif

#ifdef USL_SHAREDLIB			/* then need extra variables */
/*
 * If we need to define extra variables for each global
 */
#define ZEROINIT(t,var,val) SetZero(t,var,val); \
  SetZero (long, _libX_##var##Flag, 0); \
  SetZero (void *, _libX_##var##Ptr, NULL)

#else /* else not USL_SHAREDLIB */
/*
 * no extra crud
 */
#define ZEROINIT(t,var,val) SetZero (t, var, val)

#endif /* USL_SHAREDLIB */


/*
 * Error handlers; used to be in XlibInt.c
 */
ZEROINIT (XErrorHandler, _XErrorFunction, NULL);
ZEROINIT (XIOErrorHandler, _XIOErrorFunction, NULL);
ZEROINIT (_XQEvent *, _qfree, NULL);


/*
 * Debugging information and display list; used to be in XOpenDis.c
 */
#ifndef WIN32
ZEROINIT (int, _Xdebug, 0);
#endif
ZEROINIT (Display *, _XHeadOfDisplayList, NULL);


#ifdef XTEST1
/*
 * Stuff for input synthesis extension:
 */
/*
 * Holds the two event type codes for this extension.  The event type codes
 * for this extension may vary depending on how many extensions are installed
 * already, so the initial values given below will be added to the base event
 * code that is acquired when this extension is installed.
 *
 * These two variables must be available to programs that use this extension.
 */
int			XTestInputActionType = 0;
int			XTestFakeAckType   = 1;
#endif

#ifdef USE_THREAD_SAFETY_CONSTRUCTOR
__attribute__((constructor)) static void
xlib_ctor(void)
{
    XInitThreads();
}

__attribute__((destructor)) static void
xlib_dtor(void)
{
    XFreeThreads();
}
#endif
