/*
 *
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */
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

/*
 * This file contains the Option Record that is passed between the Parser,
 * and Module setup procs.
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _xf86Optrec_h_
#define _xf86Optrec_h_
#include <stdio.h>
#include <string.h>
#include "xf86Optionstr.h"

#include <X11/Xfuncproto.h>

extern _X_EXPORT XF86OptionPtr xf86addNewOption(XF86OptionPtr head, char *name,
                                                char *val);
extern _X_EXPORT XF86OptionPtr xf86optionListDup(XF86OptionPtr opt);
extern _X_EXPORT void xf86optionListFree(XF86OptionPtr opt);
extern _X_EXPORT char *xf86optionName(XF86OptionPtr opt);
extern _X_EXPORT char *xf86optionValue(XF86OptionPtr opt);
extern _X_EXPORT XF86OptionPtr xf86newOption(char *name, char *value);
extern _X_EXPORT XF86OptionPtr xf86nextOption(XF86OptionPtr list);
extern _X_EXPORT XF86OptionPtr xf86findOption(XF86OptionPtr list,
                                              const char *name);
extern _X_EXPORT const char *xf86findOptionValue(XF86OptionPtr list,
                                                 const char *name);
extern _X_EXPORT XF86OptionPtr xf86optionListCreate(const char **options,
                                                    int count, int used);
extern _X_EXPORT XF86OptionPtr xf86optionListMerge(XF86OptionPtr head,
                                                   XF86OptionPtr tail);
extern _X_EXPORT int xf86nameCompare(const char *s1, const char *s2);
extern _X_EXPORT char *xf86uLongToString(unsigned long i);
extern _X_EXPORT XF86OptionPtr xf86parseOption(XF86OptionPtr head);
extern _X_EXPORT void xf86printOptionList(FILE * fp, XF86OptionPtr list,
                                          int tabs);

#endif                          /* _xf86Optrec_h_ */
