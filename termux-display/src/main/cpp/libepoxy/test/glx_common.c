/*
 * Copyright © 2009, 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "glx_common.h"

Display *
get_display_or_skip(void)
{
	Display *dpy = XOpenDisplay(NULL);

	if (!dpy) {
		fputs("couldn't open display\n", stderr);
		exit(77);
	}

	return dpy;
}

XVisualInfo *
get_glx_visual(Display *dpy)
{
	XVisualInfo *visinfo;
	int attrib[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		None
	};
	int screen = DefaultScreen(dpy);

	visinfo = glXChooseVisual(dpy, screen, attrib);
	if (visinfo == NULL) {
		fputs("Couldn't get an RGBA, double-buffered visual\n", stderr);
		exit(1);
	}

	return visinfo;
}

Window
get_glx_window(Display *dpy, XVisualInfo *visinfo, bool map)
{
	XSetWindowAttributes window_attr;
	unsigned long mask;
	int screen = DefaultScreen(dpy);
	Window root_win = RootWindow(dpy, screen);
	Window win;

	window_attr.background_pixel = 0;
	window_attr.border_pixel = 0;
	window_attr.colormap = XCreateColormap(dpy, root_win,
					       visinfo->visual, AllocNone);
	window_attr.event_mask = StructureNotifyMask | ExposureMask |
		KeyPressMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	win = XCreateWindow(dpy, root_win, 0, 0,
			    10, 10, /* width, height */
			    0, visinfo->depth, InputOutput,
			    visinfo->visual, mask, &window_attr);

	return win;
}

void
make_glx_context_current_or_skip(Display *dpy)
{
	GLXContext ctx;
	XVisualInfo *visinfo = get_glx_visual(dpy);
	Window win = get_glx_window(dpy, visinfo, false);

	ctx = glXCreateContext(dpy, visinfo, False, True);
	if (ctx == None) {
		fputs("glXCreateContext failed\n", stderr);
		exit(1);
	}

	glXMakeCurrent(dpy, win, ctx);
}

GLXFBConfig
get_fbconfig_for_visinfo(Display *dpy, XVisualInfo *visinfo)
{
	int i, nconfigs;
	GLXFBConfig ret = None, *configs;

	configs = glXGetFBConfigs(dpy, visinfo->screen, &nconfigs);
	if (!configs)
		return None;

	for (i = 0; i < nconfigs; i++) {
		int v;

		if (glXGetFBConfigAttrib(dpy, configs[i], GLX_VISUAL_ID, &v))
			continue;

		if (v == visinfo->visualid) {
			ret = configs[i];
			break;
		}
	}

	XFree(configs);
	return ret;
}
