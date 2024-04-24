/*

Copyright 1986, 1998  The Open Group

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
#include "Xlibint.h"

/*
 * XErrorHandler - This procedure sets the X non-fatal error handler
 * (_XErrorFunction) to be the specified routine.  If NULL is passed in
 * the original error handler is restored.
 */

XErrorHandler
XSetErrorHandler(XErrorHandler handler)
{
    int (*oldhandler)(Display *dpy, XErrorEvent *event);

    _XLockMutex(_Xglobal_lock);
    oldhandler = _XErrorFunction;

    if (!oldhandler)
	oldhandler = _XDefaultError;

    if (handler != NULL) {
	_XErrorFunction = handler;
    }
    else {
	_XErrorFunction = _XDefaultError;
    }
    _XUnlockMutex(_Xglobal_lock);

    return (XErrorHandler) oldhandler;
}

/*
 * XIOErrorHandler - This procedure sets the X fatal I/O error handler
 * (_XIOErrorFunction) to be the specified routine.  If NULL is passed in
 * the original error handler is restored.
 */

XIOErrorHandler
XSetIOErrorHandler(XIOErrorHandler handler)
{
    int (*oldhandler)(Display *dpy);

    _XLockMutex(_Xglobal_lock);
    oldhandler = _XIOErrorFunction;

    if (!oldhandler)
	oldhandler = _XDefaultIOError;

    if (handler != NULL) {
	_XIOErrorFunction = handler;
    }
    else {
	_XIOErrorFunction = _XDefaultIOError;
    }
    _XUnlockMutex(_Xglobal_lock);

    return (XIOErrorHandler) oldhandler;
}

/*
 * XSetIOErrorExitHandler - This procedure sets the X fatal I/O error
 * exit function to be the specified routine. If NULL is passed in
 * the original error exit function is restored. The default routine
 * calls exit(3).
 */
void
XSetIOErrorExitHandler(
    Display *dpy,
    XIOErrorExitHandler handler,
    void *user_data)
{
    LockDisplay(dpy);

    if (handler != NULL) {
	dpy->exit_handler = handler;
	dpy->exit_handler_data = user_data;
    }
    else {
	dpy->exit_handler = _XDefaultIOErrorExit;
	dpy->exit_handler_data = NULL;
    }
    UnlockDisplay(dpy);
}
