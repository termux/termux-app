/*
 * Copyright (c) 1997-2001 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/* Private procs.  Public procs are in xf86Parser.h and xf86Optrec.h */

/* exported functions are/were used by the X Server */

/* Device.c */
XF86ConfDevicePtr xf86parseDeviceSection(void);
void xf86printDeviceSection(FILE * cf, XF86ConfDevicePtr ptr);
void xf86freeDeviceList(XF86ConfDevicePtr ptr);
int xf86validateDevice(XF86ConfigPtr p);

/* Files.c */
XF86ConfFilesPtr xf86parseFilesSection(void);
void xf86printFileSection(FILE * cf, XF86ConfFilesPtr ptr);
void xf86freeFiles(XF86ConfFilesPtr p);

/* Flags.c */
XF86ConfFlagsPtr xf86parseFlagsSection(void);
void xf86printServerFlagsSection(FILE * f, XF86ConfFlagsPtr flags);
void xf86freeFlags(XF86ConfFlagsPtr flags);

/* Input.c */
XF86ConfInputPtr xf86parseInputSection(void);
void xf86printInputSection(FILE * f, XF86ConfInputPtr ptr);
void xf86freeInputList(XF86ConfInputPtr ptr);
int xf86validateInput(XF86ConfigPtr p);

/* InputClass.c */
XF86ConfInputClassPtr xf86parseInputClassSection(void);
void xf86printInputClassSection(FILE * f, XF86ConfInputClassPtr ptr);

/* OutputClass.c */
XF86ConfOutputClassPtr xf86parseOutputClassSection(void);
void xf86printOutputClassSection(FILE * f, XF86ConfOutputClassPtr ptr);

/* Layout.c */
XF86ConfLayoutPtr xf86parseLayoutSection(void);
void xf86printLayoutSection(FILE * cf, XF86ConfLayoutPtr ptr);
void xf86freeLayoutList(XF86ConfLayoutPtr ptr);
int xf86validateLayout(XF86ConfigPtr p);

/* Module.c */
XF86ConfModulePtr xf86parseModuleSection(void);
void xf86printModuleSection(FILE * cf, XF86ConfModulePtr ptr);
extern _X_EXPORT XF86LoadPtr xf86addNewLoadDirective(XF86LoadPtr head,
                                                     const char *name, int type,
                                                     XF86OptionPtr opts);
void xf86freeModules(XF86ConfModulePtr ptr);

/* Monitor.c */
XF86ConfMonitorPtr xf86parseMonitorSection(void);
XF86ConfModesPtr xf86parseModesSection(void);
void xf86printMonitorSection(FILE * cf, XF86ConfMonitorPtr ptr);
void xf86printModesSection(FILE * cf, XF86ConfModesPtr ptr);
extern _X_EXPORT void xf86freeMonitorList(XF86ConfMonitorPtr ptr);
void xf86freeModesList(XF86ConfModesPtr ptr);
int xf86validateMonitor(XF86ConfigPtr p, XF86ConfScreenPtr screen);

/* Pointer.c */
XF86ConfInputPtr xf86parsePointerSection(void);

/* Screen.c */
XF86ConfScreenPtr xf86parseScreenSection(void);
void xf86printScreenSection(FILE * cf, XF86ConfScreenPtr ptr);
extern _X_EXPORT void xf86freeScreenList(XF86ConfScreenPtr ptr);
int xf86validateScreen(XF86ConfigPtr p);

/* Vendor.c */
XF86ConfVendorPtr xf86parseVendorSection(void);
void xf86freeVendorList(XF86ConfVendorPtr p);
void xf86printVendorSection(FILE * cf, XF86ConfVendorPtr ptr);

/* Video.c */
XF86ConfVideoAdaptorPtr xf86parseVideoAdaptorSection(void);
void xf86printVideoAdaptorSection(FILE * cf, XF86ConfVideoAdaptorPtr ptr);
void xf86freeVideoAdaptorList(XF86ConfVideoAdaptorPtr ptr);

/* scan.c */
int xf86getToken(const xf86ConfigSymTabRec * tab);
int xf86getSubToken(char **comment);
int xf86getSubTokenWithTab(char **comment, const xf86ConfigSymTabRec * tab);
void xf86unGetToken(int token);
char *xf86tokenString(void);
void
xf86parseError(const char *format, ...)
_X_ATTRIBUTE_PRINTF(1, 2);
void
xf86validationError(const char *format, ...)
_X_ATTRIBUTE_PRINTF(1, 2);
void
xf86setSection(const char *section);
int
xf86getStringToken(const xf86ConfigSymTabRec * tab);

/* write.c */
/* DRI.c */
XF86ConfDRIPtr
xf86parseDRISection(void);
void
xf86printDRISection(FILE * cf, XF86ConfDRIPtr ptr);
void
xf86freeDRI(XF86ConfDRIPtr ptr);

/* Extensions.c */
XF86ConfExtensionsPtr
xf86parseExtensionsSection(void);
void
xf86printExtensionsSection(FILE * cf, XF86ConfExtensionsPtr ptr);
void
xf86freeExtensions(XF86ConfExtensionsPtr ptr);

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef HAVE_XORG_CONFIG_H
/* Externally provided functions */
void
ErrorF(const char *f, ...);
void
VErrorF(const char *f, va_list args);
#endif
