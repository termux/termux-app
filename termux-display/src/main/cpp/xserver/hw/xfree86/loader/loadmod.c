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
 * Copyright (c) 1997-2002 by The XFree86 Project, Inc.
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

#include "dix.h"
#include "os.h"
#include "loaderProcs.h"
#include "xf86Module.h"
#include "loader.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h>
#include <dirent.h>
#include <limits.h>

typedef struct _pattern {
    const char *pattern;
    regex_t rex;
} PatternRec, *PatternPtr;

/* Prototypes for static functions */
static char *FindModule(const char *, const char *, PatternPtr);
static Bool CheckVersion(const char *, XF86ModuleVersionInfo *,
                         const XF86ModReqInfo *);
static char *LoaderGetCanonicalName(const char *, PatternPtr);
static void RemoveChild(ModuleDescPtr);

const ModuleVersions LoaderVersionInfo = {
    XORG_VERSION_CURRENT,
    ABI_ANSIC_VERSION,
    ABI_VIDEODRV_VERSION,
    ABI_XINPUT_VERSION,
    ABI_EXTENSION_VERSION,
};

static int ModuleDuplicated[] = { };

static void
FreeStringList(char **paths)
{
    char **p;

    if (!paths)
        return;

    for (p = paths; *p; p++)
        free(*p);

    free(paths);
}

static char **defaultPathList = NULL;

static Bool
PathIsAbsolute(const char *path)
{
    return *path == '/';
}

/*
 * Convert a comma-separated path into a NULL-terminated array of path
 * elements, rejecting any that are not full absolute paths, and appending
 * a '/' when it isn't already present.
 */
static char **
InitPathList(const char *path)
{
    char *fullpath = NULL;
    char *elem = NULL;
    char **list = NULL, **save = NULL;
    int len;
    int addslash;
    int n = 0;

    fullpath = strdup(path);
    if (!fullpath)
        return NULL;
    elem = strtok(fullpath, ",");
    while (elem) {
        if (PathIsAbsolute(elem)) {
            len = strlen(elem);
            addslash = (elem[len - 1] != '/');
            if (addslash)
                len++;
            save = list;
            list = reallocarray(list, n + 2, sizeof(char *));
            if (!list) {
                if (save) {
                    save[n] = NULL;
                    FreeStringList(save);
                }
                free(fullpath);
                return NULL;
            }
            list[n] = malloc(len + 1);
            if (!list[n]) {
                FreeStringList(list);
                free(fullpath);
                return NULL;
            }
            strcpy(list[n], elem);
            if (addslash) {
                list[n][len - 1] = '/';
                list[n][len] = '\0';
            }
            n++;
        }
        elem = strtok(NULL, ",");
    }
    if (list)
        list[n] = NULL;
    free(fullpath);
    return list;
}

void
LoaderSetPath(const char *path)
{
    if (!path)
        return;

    FreeStringList(defaultPathList);
    defaultPathList = InitPathList(path);
}

/* Standard set of module subdirectories to search, in order of preference */
static const char *stdSubdirs[] = {
    "",
    "input/",
    "drivers/",
    "extensions/",
    NULL
};

/*
 * Standard set of module name patterns to check, in order of preference
 * These are regular expressions (suitable for use with POSIX regex(3)).
 *
 * This list assumes that you're an ELFish platform and therefore your
 * shared libraries are named something.so.  If we're ever nuts enough
 * to port this DDX to, say, Darwin, we'll need to fix this.
 */
static PatternRec stdPatterns[] = {
#ifdef __CYGWIN__
    {"^cyg(.*)\\.dll$",},
    {"(.*)_drv\\.dll$",},
    {"(.*)\\.dll$",},
#else
    {"^lib(.*)\\.so$",},
    {"(.*)_drv\\.so$",},
    {"(.*)\\.so$",},
#endif
    {NULL,}
};

static PatternPtr
InitPatterns(const char **patternlist)
{
    char errmsg[80];
    int i, e;
    PatternPtr patterns = NULL;
    PatternPtr p = NULL;
    static int firstTime = 1;
    const char **s;

    if (firstTime) {
        /* precompile stdPatterns */
        firstTime = 0;
        for (p = stdPatterns; p->pattern; p++)
            if ((e = regcomp(&p->rex, p->pattern, REG_EXTENDED)) != 0) {
                regerror(e, &p->rex, errmsg, sizeof(errmsg));
                FatalError("InitPatterns: regcomp error for `%s': %s\n",
                           p->pattern, errmsg);
            }
    }

    if (patternlist) {
        for (i = 0, s = patternlist; *s; i++, s++)
            if (*s == DEFAULT_LIST)
                i += ARRAY_SIZE(stdPatterns) - 1 - 1;
        patterns = xallocarray(i + 1, sizeof(PatternRec));
        if (!patterns) {
            return NULL;
        }
        for (i = 0, s = patternlist; *s; i++, s++)
            if (*s != DEFAULT_LIST) {
                p = patterns + i;
                p->pattern = *s;
                if ((e = regcomp(&p->rex, p->pattern, REG_EXTENDED)) != 0) {
                    regerror(e, &p->rex, errmsg, sizeof(errmsg));
                    ErrorF("InitPatterns: regcomp error for `%s': %s\n",
                           p->pattern, errmsg);
                    i--;
                }
            }
            else {
                for (p = stdPatterns; p->pattern; p++, i++)
                    patterns[i] = *p;
                if (p != stdPatterns)
                    i--;
            }
        patterns[i].pattern = NULL;
    }
    else
        patterns = stdPatterns;
    return patterns;
}

static void
FreePatterns(PatternPtr patterns)
{
    if (patterns && patterns != stdPatterns)
        free(patterns);
}

static char *
FindModuleInSubdir(const char *dirpath, const char *module)
{
    struct dirent *direntry = NULL;
    DIR *dir = NULL;
    char *ret = NULL, tmpBuf[PATH_MAX];
    struct stat stat_buf;

    dir = opendir(dirpath);
    if (!dir)
        return NULL;

    while ((direntry = readdir(dir))) {
        if (direntry->d_name[0] == '.')
            continue;
        snprintf(tmpBuf, PATH_MAX, "%s%s/", dirpath, direntry->d_name);
        /* the stat with the appended / fails for normal files,
           and works for sub dirs fine, looks a bit strange in strace
           but does seem to work */
        if ((stat(tmpBuf, &stat_buf) == 0) && S_ISDIR(stat_buf.st_mode)) {
            if ((ret = FindModuleInSubdir(tmpBuf, module)))
                break;
            continue;
        }

#ifdef __CYGWIN__
        snprintf(tmpBuf, PATH_MAX, "cyg%s.dll", module);
#else
        snprintf(tmpBuf, PATH_MAX, "lib%s.so", module);
#endif
        if (strcmp(direntry->d_name, tmpBuf) == 0) {
            if (asprintf(&ret, "%s%s", dirpath, tmpBuf) == -1)
                ret = NULL;
            break;
        }

#ifdef __CYGWIN__
        snprintf(tmpBuf, PATH_MAX, "%s_drv.dll", module);
#else
        snprintf(tmpBuf, PATH_MAX, "%s_drv.so", module);
#endif
        if (strcmp(direntry->d_name, tmpBuf) == 0) {
            if (asprintf(&ret, "%s%s", dirpath, tmpBuf) == -1)
                ret = NULL;
            break;
        }

#ifdef __CYGWIN__
        snprintf(tmpBuf, PATH_MAX, "%s.dll", module);
#else
        snprintf(tmpBuf, PATH_MAX, "%s.so", module);
#endif
        if (strcmp(direntry->d_name, tmpBuf) == 0) {
            if (asprintf(&ret, "%s%s", dirpath, tmpBuf) == -1)
                ret = NULL;
            break;
        }
    }

    closedir(dir);
    return ret;
}

static char *
FindModule(const char *module, const char *dirname, PatternPtr patterns)
{
    char buf[PATH_MAX + 1];
    char *name = NULL;
    const char **s;

    if (strlen(dirname) > PATH_MAX)
        return NULL;

    for (s = stdSubdirs; *s; s++) {
        snprintf(buf, PATH_MAX, "%s%s", dirname, *s);
        if ((name = FindModuleInSubdir(buf, module)))
            break;
    }

    return name;
}

const char **
LoaderListDir(const char *subdir, const char **patternlist)
{
    char buf[PATH_MAX + 1];
    char **pathlist;
    char **elem;
    PatternPtr patterns = NULL;
    PatternPtr p;
    DIR *d;
    struct dirent *dp;
    regmatch_t match[2];
    struct stat stat_buf;
    int len, dirlen;
    char *fp;
    char **listing = NULL;
    char **save;
    char **ret = NULL;
    int n = 0;

    if (!(pathlist = defaultPathList))
        return NULL;
    if (!(patterns = InitPatterns(patternlist)))
        goto bail;

    for (elem = pathlist; *elem; elem++) {
        dirlen = snprintf(buf, PATH_MAX, "%s/%s", *elem, subdir);
        fp = buf + dirlen;
        if (stat(buf, &stat_buf) == 0 && S_ISDIR(stat_buf.st_mode) &&
            (d = opendir(buf))) {
            if (buf[dirlen - 1] != '/') {
                buf[dirlen++] = '/';
                fp++;
            }
            while ((dp = readdir(d))) {
                if (dirlen + strlen(dp->d_name) > PATH_MAX)
                    continue;
                strcpy(fp, dp->d_name);
                if (!(stat(buf, &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)))
                    continue;
                for (p = patterns; p->pattern; p++) {
                    if (regexec(&p->rex, dp->d_name, 2, match, 0) == 0 &&
                        match[1].rm_so != -1) {
                        len = match[1].rm_eo - match[1].rm_so;
                        save = listing;
                        listing = reallocarray(listing, n + 2, sizeof(char *));
                        if (!listing) {
                            if (save) {
                                save[n] = NULL;
                                FreeStringList(save);
                            }
                            closedir(d);
                            goto bail;
                        }
                        listing[n] = malloc(len + 1);
                        if (!listing[n]) {
                            FreeStringList(listing);
                            closedir(d);
                            goto bail;
                        }
                        strncpy(listing[n], dp->d_name + match[1].rm_so, len);
                        listing[n][len] = '\0';
                        n++;
                        break;
                    }
                }
            }
            closedir(d);
        }
    }
    if (listing)
        listing[n] = NULL;
    ret = listing;

 bail:
    FreePatterns(patterns);
    return (const char **) ret;
}

static Bool
CheckVersion(const char *module, XF86ModuleVersionInfo * data,
             const XF86ModReqInfo * req)
{
    int vercode[4];
    long ver = data->xf86version;
    MessageType errtype;

    LogMessage(X_INFO, "Module %s: vendor=\"%s\"\n",
               data->modname ? data->modname : "UNKNOWN!",
               data->vendor ? data->vendor : "UNKNOWN!");

    vercode[0] = ver / 10000000;
    vercode[1] = (ver / 100000) % 100;
    vercode[2] = (ver / 1000) % 100;
    vercode[3] = ver % 1000;
    LogWrite(1, "\tcompiled for %d.%d.%d", vercode[0], vercode[1], vercode[2]);
    if (vercode[3] != 0)
        LogWrite(1, ".%d", vercode[3]);
    LogWrite(1, ", module version = %d.%d.%d\n", data->majorversion,
             data->minorversion, data->patchlevel);

    if (data->moduleclass)
        LogWrite(2, "\tModule class: %s\n", data->moduleclass);

    ver = -1;
    if (data->abiclass) {
        int abimaj, abimin;
        int vermaj, vermin;

        if (!strcmp(data->abiclass, ABI_CLASS_ANSIC))
            ver = LoaderVersionInfo.ansicVersion;
        else if (!strcmp(data->abiclass, ABI_CLASS_VIDEODRV))
            ver = LoaderVersionInfo.videodrvVersion;
        else if (!strcmp(data->abiclass, ABI_CLASS_XINPUT))
            ver = LoaderVersionInfo.xinputVersion;
        else if (!strcmp(data->abiclass, ABI_CLASS_EXTENSION))
            ver = LoaderVersionInfo.extensionVersion;

        abimaj = GET_ABI_MAJOR(data->abiversion);
        abimin = GET_ABI_MINOR(data->abiversion);
        LogWrite(2, "\tABI class: %s, version %d.%d\n",
                 data->abiclass, abimaj, abimin);
        if (ver != -1) {
            vermaj = GET_ABI_MAJOR(ver);
            vermin = GET_ABI_MINOR(ver);
            if (abimaj != vermaj) {
                if (LoaderOptions & LDR_OPT_ABI_MISMATCH_NONFATAL)
                    errtype = X_WARNING;
                else
                    errtype = X_ERROR;
                LogMessageVerb(errtype, 0, "%s: module ABI major version (%d) "
                               "doesn't match the server's version (%d)\n",
                               module, abimaj, vermaj);
                if (!(LoaderOptions & LDR_OPT_ABI_MISMATCH_NONFATAL))
                    return FALSE;
            }
            else if (abimin > vermin) {
                if (LoaderOptions & LDR_OPT_ABI_MISMATCH_NONFATAL)
                    errtype = X_WARNING;
                else
                    errtype = X_ERROR;
                LogMessageVerb(errtype, 0, "%s: module ABI minor version (%d) "
                               "is newer than the server's version (%d)\n",
                               module, abimin, vermin);
                if (!(LoaderOptions & LDR_OPT_ABI_MISMATCH_NONFATAL))
                    return FALSE;
            }
        }
    }

    /* Check against requirements that the caller has specified */
    if (req) {
        if (data->majorversion != req->majorversion) {
            LogMessageVerb(X_WARNING, 2, "%s: module major version (%d) "
                           "doesn't match required major version (%d)\n",
                           module, data->majorversion, req->majorversion);
            return FALSE;
        }
        else if (data->minorversion < req->minorversion) {
            LogMessageVerb(X_WARNING, 2, "%s: module minor version (%d) is "
                          "less than the required minor version (%d)\n",
                          module, data->minorversion, req->minorversion);
            return FALSE;
        }
        else if (data->minorversion == req->minorversion &&
                 data->patchlevel < req->patchlevel) {
            LogMessageVerb(X_WARNING, 2, "%s: module patch level (%d) "
                           "is less than the required patch level "
                           "(%d)\n", module, data->patchlevel, req->patchlevel);
            return FALSE;
        }
        if (req->moduleclass) {
            if (!data->moduleclass ||
                strcmp(req->moduleclass, data->moduleclass)) {
                LogMessageVerb(X_WARNING, 2, "%s: Module class (%s) doesn't "
                               "match the required class (%s)\n", module,
                               data->moduleclass ? data->moduleclass : "<NONE>",
                               req->moduleclass);
                return FALSE;
            }
        }
        else if (req->abiclass != ABI_CLASS_NONE) {
            if (!data->abiclass || strcmp(req->abiclass, data->abiclass)) {
                LogMessageVerb(X_WARNING, 2, "%s: ABI class (%s) doesn't match"
                               " the required ABI class (%s)\n", module,
                               data->abiclass ? data->abiclass : "<NONE>",
                               req->abiclass);
                return FALSE;
            }
        }
        if (req->abiclass != ABI_CLASS_NONE) {
            int reqmaj, reqmin, maj, min;

            reqmaj = GET_ABI_MAJOR(req->abiversion);
            reqmin = GET_ABI_MINOR(req->abiversion);
            maj = GET_ABI_MAJOR(data->abiversion);
            min = GET_ABI_MINOR(data->abiversion);
            if (maj != reqmaj) {
                LogMessageVerb(X_WARNING, 2, "%s: ABI major version (%d) "
                               "doesn't match the required ABI major version "
                               "(%d)\n", module, maj, reqmaj);
                return FALSE;
            }
            /* XXX Maybe this should be the other way around? */
            if (min > reqmin) {
                LogMessageVerb(X_WARNING, 2, "%s: module ABI minor version "
                               "(%d) is newer than that available (%d)\n",
                               module, min, reqmin);
                return FALSE;
            }
        }
    }
    return TRUE;
}

static ModuleDescPtr
AddSibling(ModuleDescPtr head, ModuleDescPtr new)
{
    new->sib = head;
    return new;
}

void *
LoadSubModule(void *_parent, const char *module,
              const char **subdirlist, const char **patternlist,
              void *options, const XF86ModReqInfo * modreq,
              int *errmaj, int *errmin)
{
    ModuleDescPtr submod;
    ModuleDescPtr parent = (ModuleDescPtr) _parent;

    LogMessageVerb(X_INFO, 3, "Loading sub module \"%s\"\n", module);

    if (PathIsAbsolute(module)) {
        LogMessage(X_ERROR, "LoadSubModule: "
                   "Absolute module path not permitted: \"%s\"\n", module);
        if (errmaj)
            *errmaj = LDR_BADUSAGE;
        if (errmin)
            *errmin = 0;
        return NULL;
    }

    submod = LoadModule(module, options, modreq, errmaj);
    if (submod && submod != (ModuleDescPtr) 1) {
        parent->child = AddSibling(parent->child, submod);
        submod->parent = parent;
    }
    return submod;
}

ModuleDescPtr
DuplicateModule(ModuleDescPtr mod, ModuleDescPtr parent)
{
    ModuleDescPtr ret;

    if (!mod)
        return NULL;

    ret = calloc(1, sizeof(ModuleDesc));
    if (ret == NULL)
        return NULL;

    ret->handle = mod->handle;

    ret->SetupProc = mod->SetupProc;
    ret->TearDownProc = mod->TearDownProc;
    ret->TearDownData = ModuleDuplicated;
    ret->child = DuplicateModule(mod->child, ret);
    ret->sib = DuplicateModule(mod->sib, parent);
    ret->parent = parent;
    ret->VersionInfo = mod->VersionInfo;

    return ret;
}

static const char *compiled_in_modules[] = {
    "ddc",
    "fb",
    "i2c",
    "ramdac",
    "dbe",
    "record",
    "extmod",
    "dri",
    "dri2",
#ifdef DRI3
    "dri3",
#endif
#ifdef PRESENT
    "present",
#endif
    NULL
};

/*
 * LoadModule: load a module
 *
 * module       The module name.  Normally this is not a filename but the
 *              module's "canonical name.  A full pathname is, however,
 *              also accepted.
 * options      A NULL terminated list of Options that are passed to the
 *              module's SetupProc function.
 * modreq       An optional XF86ModReqInfo* containing
 *              version/ABI/vendor-ABI requirements to check for when
 *              loading the module.  The following fields of the
 *              XF86ModReqInfo struct are checked:
 *                majorversion - must match the module's majorversion exactly
 *                minorversion - the module's minorversion must be >= this
 *                patchlevel   - the module's minorversion.patchlevel must be
 *                               >= this.  Patchlevel is ignored when
 *                               minorversion is not set.
 *                abiclass     - (string) must match the module's abiclass
 *                abiversion   - must be consistent with the module's
 *                               abiversion (major equal, minor no older)
 *                moduleclass  - string must match the module's moduleclass
 *                               string
 *              "don't care" values are ~0 for numbers, and NULL for strings
 * errmaj       Major error return.
 *
 */
ModuleDescPtr
LoadModule(const char *module, void *options, const XF86ModReqInfo *modreq,
           int *errmaj)
{
    XF86ModuleData *initdata = NULL;
    char **pathlist = NULL;
    char *found = NULL;
    char *name = NULL;
    char **path_elem = NULL;
    char *p = NULL;
    ModuleDescPtr ret = NULL;
    PatternPtr patterns = NULL;
    int noncanonical = 0;
    char *m = NULL;
    const char **cim;

    LogMessageVerb(X_INFO, 3, "LoadModule: \"%s\"", module);

    patterns = InitPatterns(NULL);
    name = LoaderGetCanonicalName(module, patterns);
    noncanonical = (name && strcmp(module, name) != 0);
    if (noncanonical) {
        LogWrite(3, " (%s)\n", name);
        LogMessageVerb(X_WARNING, 1,
                       "LoadModule: given non-canonical module name \"%s\"\n",
                       module);
        m = name;
    }
    else {
        LogWrite(3, "\n");
        m = (char *) module;
    }

    /* Backward compatibility, vbe and int10 are merged into int10 now */
    if (!strcmp(m, "vbe"))
        m = name = strdup("int10");

    for (cim = compiled_in_modules; *cim; cim++)
        if (!strcmp(m, *cim)) {
            LogMessageVerb(X_INFO, 3, "Module \"%s\" already built-in\n", m);
            ret = (ModuleDescPtr) 1;
            goto LoadModule_exit;
        }

    if (!name) {
        if (errmaj)
            *errmaj = LDR_BADUSAGE;
        goto LoadModule_fail;
    }
    ret = calloc(1, sizeof(ModuleDesc));
    if (!ret) {
        if (errmaj)
            *errmaj = LDR_NOMEM;
        goto LoadModule_fail;
    }

    pathlist = defaultPathList;
    if (!pathlist) {
        /* This could be a malloc failure too */
        if (errmaj)
            *errmaj = LDR_BADUSAGE;
        goto LoadModule_fail;
    }

    /*
     * if the module name is not a full pathname, we need to
     * check the elements in the path
     */
    if (PathIsAbsolute(module))
        found = xstrdup(module);
    path_elem = pathlist;
    while (!found && *path_elem != NULL) {
        found = FindModule(m, *path_elem, patterns);
        path_elem++;
        /*
         * When the module name isn't the canonical name, search for the
         * former if no match was found for the latter.
         */
        if (!*path_elem && m == name) {
            path_elem = pathlist;
            m = (char *) module;
        }
    }

    /*
     * did we find the module?
     */
    if (!found) {
        LogMessage(X_WARNING, "Warning, couldn't open module %s\n", module);
        if (errmaj)
            *errmaj = LDR_NOENT;
        goto LoadModule_fail;
    }
    ret->handle = LoaderOpen(found, errmaj);
    if (ret->handle == NULL)
        goto LoadModule_fail;

    /* drop any explicit suffix from the module name */
    p = strchr(name, '.');
    if (p)
        *p = '\0';

    /*
     * now check if the special data object <modulename>ModuleData is
     * present.
     */
    if (asprintf(&p, "%sModuleData", name) == -1) {
        p = NULL;
        if (errmaj)
            *errmaj = LDR_NOMEM;
        goto LoadModule_fail;
    }
    initdata = LoaderSymbolFromModule(ret, p);
    if (initdata) {
        ModuleSetupProc setup;
        ModuleTearDownProc teardown;
        XF86ModuleVersionInfo *vers;

        vers = initdata->vers;
        setup = initdata->setup;
        teardown = initdata->teardown;

        if (vers) {
            if (!CheckVersion(module, vers, modreq)) {
                if (errmaj)
                    *errmaj = LDR_MISMATCH;
                goto LoadModule_fail;
            }
        }
        else {
            LogMessage(X_ERROR, "LoadModule: Module %s does not supply"
                       " version information\n", module);
            if (errmaj)
                *errmaj = LDR_INVALID;
            goto LoadModule_fail;
        }
        if (setup)
            ret->SetupProc = setup;
        if (teardown)
            ret->TearDownProc = teardown;
        ret->VersionInfo = vers;
    }
    else {
        /* no initdata, fail the load */
        LogMessage(X_ERROR, "LoadModule: Module %s does not have a %s "
                   "data object.\n", module, p);
        if (errmaj)
            *errmaj = LDR_INVALID;
        goto LoadModule_fail;
    }
    if (ret->SetupProc) {
        ret->TearDownData = ret->SetupProc(ret, options, errmaj, NULL);
        if (!ret->TearDownData) {
            goto LoadModule_fail;
        }
    }
    else if (options) {
        LogMessage(X_WARNING, "Module Options present, but no SetupProc "
                   "available for %s\n", module);
    }
    goto LoadModule_exit;

 LoadModule_fail:
    UnloadModule(ret);
    ret = NULL;

 LoadModule_exit:
    FreePatterns(patterns);
    free(found);
    free(name);
    free(p);

    return ret;
}

void
UnloadModule(void *_mod)
{
    ModuleDescPtr mod = _mod;

    if (mod == (ModuleDescPtr) 1)
        return;

    if (mod == NULL)
        return;

    if (mod->VersionInfo) {
        const char *name = mod->VersionInfo->modname;

        if (mod->parent)
            LogMessageVerbSigSafe(X_INFO, 3, "UnloadSubModule: \"%s\"\n", name);
        else
            LogMessageVerbSigSafe(X_INFO, 3, "UnloadModule: \"%s\"\n", name);

        if (mod->TearDownData != ModuleDuplicated) {
            if ((mod->TearDownProc) && (mod->TearDownData))
                mod->TearDownProc(mod->TearDownData);
            LoaderUnload(name, mod->handle);
        }
    }

    if (mod->child)
        UnloadModule(mod->child);
    if (mod->sib)
        UnloadModule(mod->sib);
    free(mod);
}

void
UnloadSubModule(void *_mod)
{
    ModuleDescPtr mod = (ModuleDescPtr) _mod;

    /* Some drivers are calling us on built-in submodules, ignore them */
    if (mod == (ModuleDescPtr) 1)
        return;
    RemoveChild(mod);
    UnloadModule(mod);
}

static void
RemoveChild(ModuleDescPtr child)
{
    ModuleDescPtr mdp;
    ModuleDescPtr prevsib;
    ModuleDescPtr parent;

    if (!child->parent)
        return;

    parent = child->parent;
    if (parent->child == child) {
        parent->child = child->sib;
        return;
    }

    prevsib = parent->child;
    mdp = prevsib->sib;
    while (mdp && mdp != child) {
        prevsib = mdp;
        mdp = mdp->sib;
    }
    if (mdp == child)
        prevsib->sib = child->sib;
    child->sib = NULL;
    return;
}

void
LoaderErrorMsg(const char *name, const char *modname, int errmaj, int errmin)
{
    const char *msg;
    MessageType type = X_ERROR;

    switch (errmaj) {
    case LDR_NOERROR:
        msg = "no error";
        break;
    case LDR_NOMEM:
        msg = "out of memory";
        break;
    case LDR_NOENT:
        msg = "module does not exist";
        break;
    case LDR_NOLOAD:
        msg = "loader failed";
        break;
    case LDR_ONCEONLY:
        msg = "already loaded";
        type = X_INFO;
        break;
    case LDR_MISMATCH:
        msg = "module requirement mismatch";
        break;
    case LDR_BADUSAGE:
        msg = "invalid argument(s) to LoadModule()";
        break;
    case LDR_INVALID:
        msg = "invalid module";
        break;
    case LDR_BADOS:
        msg = "module doesn't support this OS";
        break;
    case LDR_MODSPECIFIC:
        msg = "module-specific error";
        break;
    default:
        msg = "unknown error";
    }
    if (name)
        LogMessage(type, "%s: Failed to load module \"%s\" (%s, %d)\n",
                   name, modname, msg, errmin);
    else
        LogMessage(type, "Failed to load module \"%s\" (%s, %d)\n",
                   modname, msg, errmin);
}

/* Given a module path or file name, return the module's canonical name */
static char *
LoaderGetCanonicalName(const char *modname, PatternPtr patterns)
{
    char *str;
    const char *s;
    int len;
    PatternPtr p;
    regmatch_t match[2];

    /* Strip off any leading path */
    s = strrchr(modname, '/');
    if (s == NULL)
        s = modname;
    else
        s++;

    /* Find the first regex that is matched */
    for (p = patterns; p->pattern; p++)
        if (regexec(&p->rex, s, 2, match, 0) == 0 && match[1].rm_so != -1) {
            len = match[1].rm_eo - match[1].rm_so;
            str = malloc(len + 1);
            if (!str)
                return NULL;
            strncpy(str, s + match[1].rm_so, len);
            str[len] = '\0';
            return str;
        }

    /* If there is no match, return the whole name minus the leading path */
    return strdup(s);
}

/*
 * Return the module version information.
 */
unsigned long
LoaderGetModuleVersion(ModuleDescPtr mod)
{
    if (!mod || mod == (ModuleDescPtr) 1 || !mod->VersionInfo)
        return 0;

    return MODULE_VERSION_NUMERIC(mod->VersionInfo->majorversion,
                                  mod->VersionInfo->minorversion,
                                  mod->VersionInfo->patchlevel);
}
