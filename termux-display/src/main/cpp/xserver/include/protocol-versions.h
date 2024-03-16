/*
 * Copyright © 2009 Red Hat, Inc.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * This file specifies the server-supported protocol versions.
 */
#ifndef _PROTOCOL_VERSIONS_
#define _PROTOCOL_VERSIONS_

/* Apple DRI */
#define SERVER_APPLEDRI_MAJOR_VERSION		1
#define SERVER_APPLEDRI_MINOR_VERSION		0
#define SERVER_APPLEDRI_PATCH_VERSION		0

/* AppleWM */
#define SERVER_APPLEWM_MAJOR_VERSION		1
#define SERVER_APPLEWM_MINOR_VERSION		3
#define SERVER_APPLEWM_PATCH_VERSION		0

/* Composite */
#define SERVER_COMPOSITE_MAJOR_VERSION		0
#define SERVER_COMPOSITE_MINOR_VERSION		4

/* Damage */
#define SERVER_DAMAGE_MAJOR_VERSION		1
#define SERVER_DAMAGE_MINOR_VERSION		1

/* DRI3 */
#define SERVER_DRI3_MAJOR_VERSION               1
#define SERVER_DRI3_MINOR_VERSION               2

/* DMX */
#define SERVER_DMX_MAJOR_VERSION		2
#define SERVER_DMX_MINOR_VERSION		2
#define SERVER_DMX_PATCH_VERSION		20040604

/* Generic event extension */
#define SERVER_GE_MAJOR_VERSION                 1
#define SERVER_GE_MINOR_VERSION                 0

/* GLX */
#define SERVER_GLX_MAJOR_VERSION		1
#define SERVER_GLX_MINOR_VERSION		4

/* Xinerama */
#define SERVER_PANORAMIX_MAJOR_VERSION          1
#define SERVER_PANORAMIX_MINOR_VERSION		1

/* Present */
#define SERVER_PRESENT_MAJOR_VERSION            1
#define SERVER_PRESENT_MINOR_VERSION            2

/* RandR */
#define SERVER_RANDR_MAJOR_VERSION		1
#define SERVER_RANDR_MINOR_VERSION		6

/* Record */
#define SERVER_RECORD_MAJOR_VERSION		1
#define SERVER_RECORD_MINOR_VERSION		13

/* Render */
#define SERVER_RENDER_MAJOR_VERSION		0
#define SERVER_RENDER_MINOR_VERSION		11

/* RandR Xinerama */
#define SERVER_RRXINERAMA_MAJOR_VERSION		1
#define SERVER_RRXINERAMA_MINOR_VERSION		1

/* Screensaver */
#define SERVER_SAVER_MAJOR_VERSION		1
#define SERVER_SAVER_MINOR_VERSION		1

/* Security */
#define SERVER_SECURITY_MAJOR_VERSION		1
#define SERVER_SECURITY_MINOR_VERSION		0

/* Shape */
#define SERVER_SHAPE_MAJOR_VERSION		1
#define SERVER_SHAPE_MINOR_VERSION		1

/* SHM */
#define SERVER_SHM_MAJOR_VERSION		1
#if XTRANS_SEND_FDS
#define SERVER_SHM_MINOR_VERSION		2
#else
#define SERVER_SHM_MINOR_VERSION		1
#endif

/* Sync */
#define SERVER_SYNC_MAJOR_VERSION		3
#define SERVER_SYNC_MINOR_VERSION		1

/* Windows DRI */
#define SERVER_WINDOWSDRI_MAJOR_VERSION		1
#define SERVER_WINDOWSDRI_MINOR_VERSION		0
#define SERVER_WINDOWSDRI_PATCH_VERSION		0

/* Windows WM */
#define SERVER_WINDOWSWM_MAJOR_VERSION		1
#define SERVER_WINDOWSWM_MINOR_VERSION		0
#define SERVER_WINDOWSWM_PATCH_VERSION		0

/* DGA */
#define SERVER_XDGA_MAJOR_VERSION		2
#define SERVER_XDGA_MINOR_VERSION		0

/* Big Font */
#define SERVER_XF86BIGFONT_MAJOR_VERSION	1
#define SERVER_XF86BIGFONT_MINOR_VERSION	1

/* DRI */
#define SERVER_XF86DRI_MAJOR_VERSION		4
#define SERVER_XF86DRI_MINOR_VERSION		1
#define SERVER_XF86DRI_PATCH_VERSION		20040604

/* Vidmode */
#define SERVER_XF86VIDMODE_MAJOR_VERSION	2
#define SERVER_XF86VIDMODE_MINOR_VERSION	2

/* Fixes */
#define SERVER_XFIXES_MAJOR_VERSION		6
#define SERVER_XFIXES_MINOR_VERSION		0

/* X Input */
#define SERVER_XI_MAJOR_VERSION			2
#define SERVER_XI_MINOR_VERSION			4

/* XKB */
#define SERVER_XKB_MAJOR_VERSION		1
#define SERVER_XKB_MINOR_VERSION		0

/* Resource */
#define SERVER_XRES_MAJOR_VERSION		1
#define SERVER_XRES_MINOR_VERSION		2

/* XvMC */
#define SERVER_XVMC_MAJOR_VERSION		1
#define SERVER_XVMC_MINOR_VERSION		1

#endif
