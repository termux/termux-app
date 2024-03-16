/* X11Application.h -- subclass of NSApplication to multiplex events
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

#ifndef X11APPLICATION_H
#define X11APPLICATION_H 1

#include <X11/Xdefs.h>

#if __OBJC__

#import "X11Controller.h"

@interface X11Application : NSApplication

@property (nonatomic, readwrite, strong) X11Controller *controller;
@property (nonatomic, readonly, assign) OSX_BOOL x_active;

@end

extern X11Application * X11App;

#endif /* __OBJC__ */

void
X11ApplicationSetWindowMenu(int nitems, const char **items,
                            const char *shortcuts);
void
X11ApplicationSetWindowMenuCheck(int idx);
void
X11ApplicationSetFrontProcess(void);
void
X11ApplicationSetCanQuit(int state);
void
X11ApplicationServerReady(void);
void
X11ApplicationShowHideMenubar(int state);
void
X11ApplicationLaunchClient(const char *cmd);

Bool
X11ApplicationCanEnterRandR(void);

void
X11ApplicationMain(int argc, char **argv, char **envp);

extern Bool XQuartzScrollInDeviceDirection;

#endif /* X11APPLICATION_H */
