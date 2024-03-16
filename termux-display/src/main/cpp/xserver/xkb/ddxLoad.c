/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <xkb-config.h>

#include <stdio.h>
#include <ctype.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/extensions/XKM.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include <xkbsrv.h>
#include <X11/extensions/XI.h>
#include "xkb.h"

#define	PRE_ERROR_MSG "\"The XKEYBOARD keymap compiler (xkbcomp) reports:\""
#define	ERROR_PREFIX	"\"> \""
#define	POST_ERROR_MSG1 "\"Errors from xkbcomp are not fatal to the X server\""
#define	POST_ERROR_MSG2 "\"End of messages from xkbcomp\""

#if defined(WIN32)
#define PATHSEPARATOR "\\"
#else
#define PATHSEPARATOR "/"
#endif

char* xkbcomp_argv[16] = {0};
int xkbcomp_argc = 0;

static unsigned
LoadXKM(unsigned want, unsigned need, const char *keymap, XkbDescPtr *xkbRtrn);

static void
OutputDirectory(char *outdir, size_t size)
{
    const char *directory = NULL;
    const char *pathsep = "";
    int r = -1;

#ifndef WIN32
    /* Can we write an xkm and then open it too? */
    if (access(XKM_OUTPUT_DIR, W_OK | X_OK) == 0) {
        directory = XKM_OUTPUT_DIR;
        if (XKM_OUTPUT_DIR[strlen(XKM_OUTPUT_DIR) - 1] != '/')
            pathsep = "/";
    }
#else
    directory = Win32TempDir();
    pathsep = "\\";
#endif

    if (directory)
        r = snprintf(outdir, size, "%s%s", directory, pathsep);
    if (r < 0 || r >= size) {
        assert(strlen("/tmp/") < size);
        strcpy(outdir, "/tmp/");
    }
}

/**
 * Callback invoked by XkbRunXkbComp. Write to out to talk to xkbcomp.
 */
typedef void (*xkbcomp_buffer_callback)(FILE *out, void *userdata);

/**
 * Start xkbcomp, let the callback write into xkbcomp's stdin. When done,
 * return a strdup'd copy of the file name we've written to.
 */
static char *
RunXkbComp(xkbcomp_buffer_callback callback, void *userdata)
{
    FILE *out;
    char *buf = NULL, keymap[PATH_MAX], xkm_output_dir[PATH_MAX];

    const char *emptystring = "";
    char *xkbbasedirflag = NULL;
    const char *xkbbindir = emptystring;
    const char *xkbbindirsep = emptystring;

#ifdef WIN32
    /* WIN32 has no popen. The input must be stored in a file which is
       used as input for xkbcomp. xkbcomp does not read from stdin. */
    char tmpname[PATH_MAX];
    const char *xkmfile = tmpname;
#else
    const char *xkmfile = "-";
#endif

    snprintf(keymap, sizeof(keymap), "server-%s", display);

    OutputDirectory(xkm_output_dir, sizeof(xkm_output_dir));

#ifdef WIN32
    strcpy(tmpname, Win32TempDir());
    strcat(tmpname, "\\xkb_XXXXXX");
    (void) mktemp(tmpname);
#endif

    if (XkbBaseDirectory != NULL) {
        if (asprintf(&xkbbasedirflag, "\"-R%s\"", XkbBaseDirectory) == -1)
            xkbbasedirflag = NULL;
    }

    if (XkbBinDirectory != NULL) {
        int ld = strlen(XkbBinDirectory);
        int lps = strlen(PATHSEPARATOR);

        xkbbindir = XkbBinDirectory;

        if ((ld >= lps) && (strcmp(xkbbindir + ld - lps, PATHSEPARATOR) != 0)) {
            xkbbindirsep = PATHSEPARATOR;
        }
    }

    if (asprintf(&buf,
                 "\"%s%sxkbcomp\" -w %d %s -xkm \"%s\" "
                 "-em1 %s -emp %s -eml %s \"%s%s.xkm\"",
                 xkbbindir, xkbbindirsep,
                 ((xkbDebugFlags < 2) ? 1 :
                  ((xkbDebugFlags > 10) ? 10 : (int) xkbDebugFlags)),
                 xkbbasedirflag ? xkbbasedirflag : "", xkmfile,
                 PRE_ERROR_MSG, ERROR_PREFIX, POST_ERROR_MSG1,
                 xkm_output_dir, keymap) == -1)
        buf = NULL;

    char buf2[256];
    char buf3[256];
    sprintf(buf2, "-R%s", XkbBaseDirectory);
    sprintf(buf3, "%s%s.xkm", xkm_output_dir, keymap);
    xkbcomp_argv[0] = "xkbcomp";
    xkbcomp_argv[1] = "-w";
    xkbcomp_argv[2] = "1";
    xkbcomp_argv[3] = buf2;
    xkbcomp_argv[4] = "-xkm";
    xkbcomp_argv[5] = (char*) xkmfile;
    xkbcomp_argv[6] = "-em1";
    xkbcomp_argv[7] = PRE_ERROR_MSG;
    xkbcomp_argv[8] = "-emp";
    xkbcomp_argv[9] = ERROR_PREFIX;
    xkbcomp_argv[10] = "-eml";
    xkbcomp_argv[11] = POST_ERROR_MSG1;
    xkbcomp_argv[12] = buf3;
    xkbcomp_argc = 13;

    free(xkbbasedirflag);

    if (!buf) {
        LogMessage(X_ERROR,
                   "XKB: Could not invoke xkbcomp: not enough memory\n");
        return NULL;
    }

#ifndef WIN32
    out = Popen(buf, "w");
#else
    out = fopen(tmpname, "w");
#endif

    if (out != NULL) {
        /* Now write to xkbcomp */
        (*callback)(out, userdata);

#ifndef WIN32
        if (Pclose(out) == 0)
#else
        if (fclose(out) == 0 && System(buf) >= 0)
#endif
        {
            if (xkbDebugFlags)
                DebugF("[xkb] xkb executes: %s\n", buf);
            free(buf);
#ifdef WIN32
            unlink(tmpname);
#endif
            return xnfstrdup(keymap);
        }
        else {
            LogMessage(X_ERROR, "Error compiling keymap (%s) executing '%s'\n",
                       keymap, buf);
        }
#ifdef WIN32
        /* remove the temporary file */
        unlink(tmpname);
#endif
    }
    else {
#ifndef WIN32
        LogMessage(X_ERROR, "XKB: Could not invoke xkbcomp\n");
#else
        LogMessage(X_ERROR, "Could not open file %s\n", tmpname);
#endif
    }
    free(buf);
    return NULL;
}

typedef struct {
    XkbDescPtr xkb;
    XkbComponentNamesPtr names;
    unsigned int want;
    unsigned int need;
} XkbKeymapNamesCtx;

static void
xkb_write_keymap_for_names_cb(FILE *out, void *userdata)
{
    XkbKeymapNamesCtx *ctx = userdata;
#ifdef DEBUG
    if (xkbDebugFlags) {
        ErrorF("[xkb] XkbDDXCompileKeymapByNames compiling keymap:\n");
        XkbWriteXKBKeymapForNames(stderr, ctx->names, ctx->xkb, ctx->want, ctx->need);
    }
#endif
    XkbWriteXKBKeymapForNames(out, ctx->names, ctx->xkb, ctx->want, ctx->need);
}

static Bool
XkbDDXCompileKeymapByNames(XkbDescPtr xkb,
                           XkbComponentNamesPtr names,
                           unsigned want,
                           unsigned need, char *nameRtrn, int nameRtrnLen)
{
    char *keymap;
    Bool rc = FALSE;
    XkbKeymapNamesCtx ctx = {
        .xkb = xkb,
        .names = names,
        .want = want,
        .need = need
    };

    keymap = RunXkbComp(xkb_write_keymap_for_names_cb, &ctx);

    if (keymap) {
        if(nameRtrn)
            strlcpy(nameRtrn, keymap, nameRtrnLen);

        free(keymap);
        rc = TRUE;
    } else if (nameRtrn)
        *nameRtrn = '\0';

    return rc;
}

typedef struct {
    const char *keymap;
    size_t len;
} XkbKeymapString;

static void
xkb_write_keymap_string_cb(FILE *out, void *userdata)
{
    XkbKeymapString *s = userdata;
    fwrite(s->keymap, s->len, 1, out);
}

static unsigned int
XkbDDXLoadKeymapFromString(DeviceIntPtr keybd,
                          const char *keymap, int keymap_length,
                          unsigned int want,
                          unsigned int need,
                          XkbDescPtr *xkbRtrn)
{
    unsigned int have;
    char *map_name;
    XkbKeymapString map = {
        .keymap = keymap,
        .len = keymap_length
    };

    *xkbRtrn = NULL;

    map_name = RunXkbComp(xkb_write_keymap_string_cb, &map);
    if (!map_name) {
        LogMessage(X_ERROR, "XKB: Couldn't compile keymap\n");
        return 0;
    }

    have = LoadXKM(want, need, map_name, xkbRtrn);
    free(map_name);

    return have;
}

static FILE *
XkbDDXOpenConfigFile(const char *mapName, char *fileNameRtrn, int fileNameRtrnLen)
{
    char buf[PATH_MAX], xkm_output_dir[PATH_MAX];
    FILE *file;

    buf[0] = '\0';
    if (mapName != NULL) {
        OutputDirectory(xkm_output_dir, sizeof(xkm_output_dir));
        if ((XkbBaseDirectory != NULL) && (xkm_output_dir[0] != '/')
#ifdef WIN32
            && (!isalpha(xkm_output_dir[0]) || xkm_output_dir[1] != ':')
#endif
            ) {
            if (snprintf(buf, PATH_MAX, "%s/%s%s.xkm", XkbBaseDirectory,
                         xkm_output_dir, mapName) >= PATH_MAX)
                buf[0] = '\0';
        }
        else {
            if (snprintf(buf, PATH_MAX, "%s%s.xkm", xkm_output_dir, mapName)
                >= PATH_MAX)
                buf[0] = '\0';
        }
        if (buf[0] != '\0')
            file = fopen(buf, "rb");
        else
            file = NULL;
    }
    else
        file = NULL;
    if ((fileNameRtrn != NULL) && (fileNameRtrnLen > 0)) {
        strlcpy(fileNameRtrn, buf, fileNameRtrnLen);
    }
    return file;
}

static unsigned
LoadXKM(unsigned want, unsigned need, const char *keymap, XkbDescPtr *xkbRtrn)
{
    FILE *file;
    char fileName[PATH_MAX];
    unsigned missing;

    file = XkbDDXOpenConfigFile(keymap, fileName, PATH_MAX);
    if (file == NULL) {
        LogMessage(X_ERROR, "Couldn't open compiled keymap file %s\n",
                   fileName);
        return 0;
    }
    missing = XkmReadFile(file, need, want, xkbRtrn);
    if (*xkbRtrn == NULL) {
        LogMessage(X_ERROR, "Error loading keymap %s\n", fileName);
        fclose(file);
        (void) unlink(fileName);
        return 0;
    }
    else {
        DebugF("Loaded XKB keymap %s, defined=0x%x\n", fileName,
               (*xkbRtrn)->defined);
    }
    fclose(file);
    (void) unlink(fileName);
    return (need | want) & (~missing);
}

unsigned
XkbDDXLoadKeymapByNames(DeviceIntPtr keybd,
                        XkbComponentNamesPtr names,
                        unsigned want,
                        unsigned need,
                        XkbDescPtr *xkbRtrn, char *nameRtrn, int nameRtrnLen)
{
    XkbDescPtr xkb;

    *xkbRtrn = NULL;
    if ((keybd == NULL) || (keybd->key == NULL) ||
        (keybd->key->xkbInfo == NULL))
        xkb = NULL;
    else
        xkb = keybd->key->xkbInfo->desc;
    if ((names->keycodes == NULL) && (names->types == NULL) &&
        (names->compat == NULL) && (names->symbols == NULL) &&
        (names->geometry == NULL)) {
        LogMessage(X_ERROR, "XKB: No components provided for device %s\n",
                   keybd->name ? keybd->name : "(unnamed keyboard)");
        return 0;
    }
    else if (!XkbDDXCompileKeymapByNames(xkb, names, want, need,
                                         nameRtrn, nameRtrnLen)) {
        LogMessage(X_ERROR, "XKB: Couldn't compile keymap\n");
        return 0;
    }

    return LoadXKM(want, need, nameRtrn, xkbRtrn);
}

Bool
XkbDDXNamesFromRules(DeviceIntPtr keybd,
                     const char *rules_name,
                     XkbRF_VarDefsPtr defs, XkbComponentNamesPtr names)
{
    char buf[PATH_MAX];
    FILE *file;
    Bool complete;
    XkbRF_RulesPtr rules;

    if (!rules_name)
        return FALSE;

    if (snprintf(buf, PATH_MAX, "%s/rules/%s", XkbBaseDirectory, rules_name)
        >= PATH_MAX) {
        LogMessage(X_ERROR, "XKB: Rules name is too long\n");
        return FALSE;
    }

    file = fopen(buf, "r");
    if (!file) {
        LogMessage(X_ERROR, "XKB: Couldn't open rules file %s\n", buf);
        return FALSE;
    }

    rules = XkbRF_Create();
    if (!rules) {
        LogMessage(X_ERROR, "XKB: Couldn't create rules struct\n");
        fclose(file);
        return FALSE;
    }

    if (!XkbRF_LoadRules(file, rules)) {
        LogMessage(X_ERROR, "XKB: Couldn't parse rules file %s\n", rules_name);
        fclose(file);
        XkbRF_Free(rules, TRUE);
        return FALSE;
    }

    memset(names, 0, sizeof(*names));
    complete = XkbRF_GetComponents(rules, defs, names);
    fclose(file);
    XkbRF_Free(rules, TRUE);

    if (!complete)
        LogMessage(X_ERROR, "XKB: Rules returned no components\n");

    return complete;
}

static Bool
XkbRMLVOtoKcCGST(DeviceIntPtr dev, XkbRMLVOSet * rmlvo,
                 XkbComponentNamesPtr kccgst)
{
    XkbRF_VarDefsRec mlvo;

    mlvo.model = rmlvo->model;
    mlvo.layout = rmlvo->layout;
    mlvo.variant = rmlvo->variant;
    mlvo.options = rmlvo->options;

    return XkbDDXNamesFromRules(dev, rmlvo->rules, &mlvo, kccgst);
}

/**
 * Compile the given RMLVO keymap and return it. Returns the XkbDescPtr on
 * success or NULL on failure. If the components compiled are not a superset
 * or equal to need, the compilation is treated as failure.
 */
static XkbDescPtr
XkbCompileKeymapForDevice(DeviceIntPtr dev, XkbRMLVOSet * rmlvo, int need)
{
    XkbDescPtr xkb = NULL;
    unsigned int provided;
    XkbComponentNamesRec kccgst = { 0 };
    char name[PATH_MAX];

    if (XkbRMLVOtoKcCGST(dev, rmlvo, &kccgst)) {
        provided =
            XkbDDXLoadKeymapByNames(dev, &kccgst, XkmAllIndicesMask, need, &xkb,
                                    name, PATH_MAX);
        if ((need & provided) != need) {
            if (xkb) {
                XkbFreeKeyboard(xkb, 0, TRUE);
                xkb = NULL;
            }
        }
    }

    XkbFreeComponentNames(&kccgst, FALSE);
    return xkb;
}

static XkbDescPtr
KeymapOrDefaults(DeviceIntPtr dev, XkbDescPtr xkb)
{
    XkbRMLVOSet dflts;

    if (xkb)
        return xkb;

    /* we didn't get what we really needed. And that will likely leave
     * us with a keyboard that doesn't work. Use the defaults instead */
    LogMessage(X_ERROR, "XKB: Failed to load keymap. Loading default "
                        "keymap instead.\n");

    XkbGetRulesDflts(&dflts);

    xkb = XkbCompileKeymapForDevice(dev, &dflts, 0);

    XkbFreeRMLVOSet(&dflts, FALSE);

    return xkb;
}


XkbDescPtr
XkbCompileKeymap(DeviceIntPtr dev, XkbRMLVOSet * rmlvo)
{
    XkbDescPtr xkb;
    unsigned int need;

    if (!dev || !rmlvo) {
        LogMessage(X_ERROR, "XKB: No device or RMLVO specified\n");
        return NULL;
    }

    /* These are the components we really really need */
    need = XkmSymbolsMask | XkmCompatMapMask | XkmTypesMask |
        XkmKeyNamesMask | XkmVirtualModsMask;

    xkb = XkbCompileKeymapForDevice(dev, rmlvo, need);

    return KeymapOrDefaults(dev, xkb);
}

XkbDescPtr
XkbCompileKeymapFromString(DeviceIntPtr dev,
                           const char *keymap, int keymap_length)
{
    XkbDescPtr xkb;
    unsigned int need, provided;

    if (!dev || !keymap) {
        LogMessage(X_ERROR, "XKB: No device or keymap specified\n");
        return NULL;
    }

    /* These are the components we really really need */
    need = XkmSymbolsMask | XkmCompatMapMask | XkmTypesMask |
           XkmKeyNamesMask | XkmVirtualModsMask;

    provided =
        XkbDDXLoadKeymapFromString(dev, keymap, keymap_length,
                                   XkmAllIndicesMask, need, &xkb);
    if ((need & provided) != need) {
        if (xkb) {
            XkbFreeKeyboard(xkb, 0, TRUE);
            xkb = NULL;
        }
    }

    return KeymapOrDefaults(dev, xkb);
}
