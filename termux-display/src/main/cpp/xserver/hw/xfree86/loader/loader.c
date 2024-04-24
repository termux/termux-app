/*
 * Copyright 1995-1998 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <string.h>
#include "os.h"
#include "loader.h"
#include "loaderProcs.h"

#ifdef HAVE_DLFCN_H

#include <dlfcn.h>
#include <X11/Xos.h>

#else
#error i have no dynamic linker and i must scream
#endif

#ifndef XORG_NO_SDKSYMS
extern void *xorg_symbols[];
#endif

void
LoaderInit(void)
{
#ifndef XORG_NO_SDKSYMS
    LogMessageVerb(X_INFO, 2, "Loader magic: %p\n", (void *) xorg_symbols);
#endif
    LogMessageVerb(X_INFO, 2, "Module ABI versions:\n");
    LogWrite(2, "\t%s: %d.%d\n", ABI_CLASS_ANSIC,
             GET_ABI_MAJOR(LoaderVersionInfo.ansicVersion),
             GET_ABI_MINOR(LoaderVersionInfo.ansicVersion));
    LogWrite(2, "\t%s: %d.%d\n", ABI_CLASS_VIDEODRV,
             GET_ABI_MAJOR(LoaderVersionInfo.videodrvVersion),
             GET_ABI_MINOR(LoaderVersionInfo.videodrvVersion));
    LogWrite(2, "\t%s : %d.%d\n", ABI_CLASS_XINPUT,
             GET_ABI_MAJOR(LoaderVersionInfo.xinputVersion),
             GET_ABI_MINOR(LoaderVersionInfo.xinputVersion));
    LogWrite(2, "\t%s : %d.%d\n", ABI_CLASS_EXTENSION,
             GET_ABI_MAJOR(LoaderVersionInfo.extensionVersion),
             GET_ABI_MINOR(LoaderVersionInfo.extensionVersion));

}

/* Public Interface to the loader. */

void *
LoaderOpen(const char *module, int *errmaj)
{
    void *ret;

#if defined(DEBUG)
    ErrorF("LoaderOpen(%s)\n", module);
#endif

    LogMessage(X_INFO, "Loading %s\n", module);

    if (!(ret = dlopen(module, RTLD_LAZY | RTLD_GLOBAL))) {
        LogMessage(X_ERROR, "Failed to load %s: %s\n", module, dlerror());
        if (errmaj)
            *errmaj = LDR_NOLOAD;
        return NULL;
    }

    return ret;
}

void *
LoaderSymbol(const char *name)
{
    static void *global_scope = NULL;
    void *p;

    p = dlsym(RTLD_DEFAULT, name);
    if (p != NULL)
        return p;

    if (!global_scope)
        global_scope = dlopen(NULL, RTLD_LAZY | RTLD_GLOBAL);

    if (global_scope)
        return dlsym(global_scope, name);

    return NULL;
}

void *
LoaderSymbolFromModule(void *handle, const char *name)
{
    ModuleDescPtr mod = handle;
    return dlsym(mod->handle, name);
}

void
LoaderUnload(const char *name, void *handle)
{
    LogMessageVerbSigSafe(X_INFO, 1, "Unloading %s\n", name);
    if (handle)
        dlclose(handle);
}

unsigned long LoaderOptions = 0;

void
LoaderSetOptions(unsigned long opts)
{
    LoaderOptions |= opts;
}

Bool
LoaderShouldIgnoreABI(void)
{
    return (LoaderOptions & LDR_OPT_ABI_MISMATCH_NONFATAL) != 0;
}

int
LoaderGetABIVersion(const char *abiclass)
{
    struct {
        const char *name;
        int version;
    } classes[] = {
        {ABI_CLASS_ANSIC, LoaderVersionInfo.ansicVersion},
        {ABI_CLASS_VIDEODRV, LoaderVersionInfo.videodrvVersion},
        {ABI_CLASS_XINPUT, LoaderVersionInfo.xinputVersion},
        {ABI_CLASS_EXTENSION, LoaderVersionInfo.extensionVersion},
        {NULL, 0}
    };
    int i;

    for (i = 0; classes[i].name; i++) {
        if (!strcmp(classes[i].name, abiclass)) {
            return classes[i].version;
        }
    }

    return 0;
}
