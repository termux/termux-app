/***********************************************************

Copyright 1987, 1998  The Open Group

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

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

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

/*
 * Copyright (c) 2000 by The XFree86 Project, Inc.
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#include "xf86Extensions.h"
#endif

#ifdef HAVE_DMX_CONFIG_H
#include <dmx-config.h>
#undef XV
#undef DBE
#undef SCREENSAVER
#undef RANDR
#undef DAMAGE
#undef COMPOSITE
#undef MITSHM
#endif

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#undef COMPOSITE
#undef DPMSExtension
#endif

#include "misc.h"
#include "extension.h"
#include "extinit.h"
#include "micmap.h"
#include "os.h"
#include "globals.h"

#include "miinitext.h"

/* List of built-in (statically linked) extensions */
static const ExtensionModule staticExtensions[] = {
    {GEExtensionInit, "Generic Event Extension", &noGEExtension},
    {ShapeExtensionInit, "SHAPE", NULL},
#ifdef MITSHM
    {ShmExtensionInit, "MIT-SHM", &noMITShmExtension},
#endif
    {XInputExtensionInit, "XInputExtension", NULL},
#ifdef XTEST
    {XTestExtensionInit, "XTEST", &noTestExtensions},
#endif
    {BigReqExtensionInit, "BIG-REQUESTS", NULL},
    {SyncExtensionInit, "SYNC", NULL},
    {XkbExtensionInit, "XKEYBOARD", NULL},
    {XCMiscExtensionInit, "XC-MISC", NULL},
#ifdef XCSECURITY
    {SecurityExtensionInit, "SECURITY", &noSecurityExtension},
#endif
#ifdef PANORAMIX
    {PanoramiXExtensionInit, "XINERAMA", &noPanoramiXExtension},
#endif
    /* must be before Render to layer DisplayCursor correctly */
    {XFixesExtensionInit, "XFIXES", &noXFixesExtension},
#ifdef XF86BIGFONT
    {XFree86BigfontExtensionInit, "XFree86-Bigfont", &noXFree86BigfontExtension},
#endif
    {RenderExtensionInit, "RENDER", &noRenderExtension},
#ifdef RANDR
    {RRExtensionInit, "RANDR", &noRRExtension},
#endif
#ifdef COMPOSITE
    {CompositeExtensionInit, "COMPOSITE", &noCompositeExtension},
#endif
#ifdef DAMAGE
    {DamageExtensionInit, "DAMAGE", &noDamageExtension},
#endif
#ifdef SCREENSAVER
    {ScreenSaverExtensionInit, "MIT-SCREEN-SAVER", &noScreenSaverExtension},
#endif
#ifdef DBE
    {DbeExtensionInit, "DOUBLE-BUFFER", &noDbeExtension},
#endif
#ifdef XRECORD
    {RecordExtensionInit, "RECORD", &noTestExtensions},
#endif
#ifdef DPMSExtension
    {DPMSExtensionInit, "DPMS", &noDPMSExtension},
#endif
#ifdef PRESENT
    {present_extension_init, "Present", NULL},
#endif
#ifdef DRI3
    {dri3_extension_init, "DRI3", NULL},
#endif
#ifdef RES
    {ResExtensionInit, "X-Resource", &noResExtension},
#endif
#ifdef XV
    {XvExtensionInit, "XVideo", &noXvExtension},
    {XvMCExtensionInit, "XVideo-MotionCompensation", &noXvExtension},
#endif
#ifdef XSELINUX
    {SELinuxExtensionInit, "SELinux", &noSELinuxExtension},
#endif
#ifdef GLXEXT
    {GlxExtensionInit, "GLX", &noGlxExtension},
#endif
};

void
ListStaticExtensions(void)
{
    const ExtensionModule *ext;
    int i;

    ErrorF(" Only the following extensions can be run-time enabled/disabled:\n");
    for (i = 0; i < ARRAY_SIZE(staticExtensions); i++) {
        ext = &staticExtensions[i];
        if (ext->disablePtr != NULL) {
            ErrorF("\t%s\n", ext->name);
        }
    }
}

Bool
EnableDisableExtension(const char *name, Bool enable)
{
    const ExtensionModule *ext;
    int i;

    for (i = 0; i < ARRAY_SIZE(staticExtensions); i++) {
        ext = &staticExtensions[i];
        if (strcasecmp(name, ext->name) == 0) {
            if (ext->disablePtr != NULL) {
                *ext->disablePtr = !enable;
                return TRUE;
            }
            else {
                /* Extension is always on, impossible to disable */
                return enable;  /* okay if they wanted to enable,
                                   fail if they tried to disable */
            }
        }
    }

    return FALSE;
}

void
EnableDisableExtensionError(const char *name, Bool enable)
{
    const ExtensionModule *ext;
    int i;
    Bool found = FALSE;

    for (i = 0; i < ARRAY_SIZE(staticExtensions); i++) {
        ext = &staticExtensions[i];
        if ((strcmp(name, ext->name) == 0) && (ext->disablePtr == NULL)) {
            ErrorF("[mi] Extension \"%s\" can not be disabled\n", name);
            found = TRUE;
            break;
        }
    }
    if (found == FALSE) {
        ErrorF("[mi] Extension \"%s\" is not recognized\n", name);
        /* Disabling a non-existing extension is a no-op anyway */
        if (enable == FALSE)
            return;
    }
    ListStaticExtensions();
}

static ExtensionModule *ExtensionModuleList = NULL;
static int numExtensionModules = 0;

static void
AddStaticExtensions(void)
{
    static Bool listInitialised = FALSE;

    if (listInitialised)
        return;
    listInitialised = TRUE;

    /* Add built-in extensions to the list. */
    LoadExtensionList(staticExtensions, ARRAY_SIZE(staticExtensions), TRUE);
}

void
InitExtensions(int argc, char *argv[])
{
    int i;
    ExtensionModule *ext;

    AddStaticExtensions();

    for (i = 0; i < numExtensionModules; i++) {
        ext = &ExtensionModuleList[i];
        if (ext->initFunc != NULL &&
            (ext->disablePtr == NULL || !*ext->disablePtr)) {
            LogMessageVerb(X_INFO, 3, "Initializing extension %s\n",
                           ext->name);

            (ext->initFunc) ();
        }
    }
}

static ExtensionModule *
NewExtensionModuleList(int size)
{
    ExtensionModule *save = ExtensionModuleList;
    int n;

    /* Sanity check */
    if (!ExtensionModuleList)
        numExtensionModules = 0;

    n = numExtensionModules + size;
    ExtensionModuleList = reallocarray(ExtensionModuleList, n,
                                       sizeof(ExtensionModule));
    if (ExtensionModuleList == NULL) {
        ExtensionModuleList = save;
        return NULL;
    }
    else {
        numExtensionModules += size;
        return ExtensionModuleList + (numExtensionModules - size);
    }
}

void
LoadExtensionList(const ExtensionModule ext[], int size, Bool builtin)
{
    ExtensionModule *newext;
    int i;

    /* Make sure built-in extensions get added to the list before those
     * in modules. */
    AddStaticExtensions();

    if (!(newext = NewExtensionModuleList(size)))
        return;

    for (i = 0; i < size; i++, newext++) {
        newext->name = ext[i].name;
        newext->initFunc = ext[i].initFunc;
        newext->disablePtr = ext[i].disablePtr;
    }
}
