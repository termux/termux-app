/* pbproxy.h
 *
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#ifndef PBPROXY_H
#define PBPROXY_H 1

#import  <Foundation/Foundation.h>

#include <asl.h>

#define  Cursor X_Cursor
#undef _SHAPE_H_
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#undef   Cursor

#ifdef STANDALONE_XPBPROXY
/* Just used for the standalone to respond to SIGHUP to reload prefs */
extern BOOL xpbproxy_prefs_reload;

/* Setting this to YES (for the standalone app) causes us to ignore the
 * 'sync_pasteboard' defaults preference since we assume it to be on... this is
 * mainly useful for debugging/developing xpbproxy with XQuartz still running.
 * Just disable the one in the server with X11's preference pane, then run
 * the standalone app.
 */
extern BOOL xpbproxy_is_standalone;
#endif

/* from main.m */
extern void
xpbproxy_set_is_active(BOOL state);
extern BOOL
xpbproxy_get_is_active(void);
extern id
xpbproxy_selection_object(void);
extern Time
xpbproxy_current_timestamp(void);
extern int
xpbproxy_run(void);

extern Display *xpbproxy_dpy;
extern int xpbproxy_apple_wm_event_base, xpbproxy_apple_wm_error_base;
extern int xpbproxy_xfixes_event_base, xpbproxy_xfixes_error_base;
extern BOOL xpbproxy_have_xfixes;

/* from x-input.m */
extern BOOL
xpbproxy_input_register(void);

/* os/log.c or app-main.m */
extern void
ErrorF(const char *f, ...) _X_ATTRIBUTE_PRINTF(1, 2);

/* from darwin.h */
_X_ATTRIBUTE_PRINTF(6, 7)
extern void
xq_asl_log(int level, const char *subsystem, const char *file,
           const char *function, int line, const char *fmt,
           ...);

#define ASL_LOG(level, subsystem, msg, args ...) xq_asl_log(level, subsystem, \
                                                            __FILE__, \
                                                            __FUNCTION__, \
                                                            __LINE__, msg, \
                                                            ## args)
#define DebugF(msg, args ...)                    ASL_LOG(ASL_LEVEL_DEBUG, \
                                                         "xpbproxy", msg, \
                                                         ## args)
#define TRACE()                                  DebugF("TRACE")

#endif /* PBPROXY_H */
