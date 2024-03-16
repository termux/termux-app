
/*

Copyright 1993, 1998  The Open Group
Copyright (C) Colin Harrison 2005-2008

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "winmsg.h"
#include "winconfig.h"
#include "winprefs.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif
#ifdef __CYGWIN__
#include <mntent.h>
#endif
#if defined(WIN32)
#include "xkbsrv.h"
#endif
#ifdef RELOCATE_PROJECTROOT
#pragma push_macro("Status")
#undef Status
#define Status wStatus
#include <shlobj.h>
#pragma pop_macro("Status")
typedef WINAPI HRESULT(*SHGETFOLDERPATHPROC) (HWND hwndOwner,
                                              int nFolder,
                                              HANDLE hToken,
                                              DWORD dwFlags, LPTSTR pszPath);
#endif

#include "winmonitors.h"
#include "nonsdk_extinit.h"
#include "pseudoramiX/pseudoramiX.h"

#include "glx_extinit.h"
#ifdef XWIN_GLX_WINDOWS
#include "glx/glwindows.h"
#include "dri/windowsdri.h"
#endif
#include "winauth.h"

/*
 * References to external symbols
 */

/*
 * Function prototypes
 */

void
 winLogCommandLine(int argc, char *argv[]);

void
 winLogVersionInfo(void);

Bool
 winValidateArgs(void);

#ifdef RELOCATE_PROJECTROOT
const char *winGetBaseDir(void);
#endif

/*
 * For the depth 24 pixmap we default to 32 bits per pixel, but
 * we change this pixmap format later if we detect that the display
 * is going to be running at 24 bits per pixel.
 *
 * FIXME: On second thought, don't DIBs only support 32 bits per pixel?
 * DIBs are the underlying bitmap used for DirectDraw surfaces, so it
 * seems that all pixmap formats with depth 24 would be 32 bits per pixel.
 * Confirm whether depth 24 DIBs can have 24 bits per pixel, then remove/keep
 * the bits per pixel adjustment and update this comment to reflect the
 * situation.  Harold Hunt - 2002/07/02
 */

static PixmapFormatRec g_PixmapFormats[] = {
    {1, 1, BITMAP_SCANLINE_PAD},
    {4, 8, BITMAP_SCANLINE_PAD},
    {8, 8, BITMAP_SCANLINE_PAD},
    {15, 16, BITMAP_SCANLINE_PAD},
    {16, 16, BITMAP_SCANLINE_PAD},
    {24, 32, BITMAP_SCANLINE_PAD},
    {32, 32, BITMAP_SCANLINE_PAD}
};

static Bool noDriExtension;

static const ExtensionModule xwinExtensions[] = {
#ifdef GLXEXT
#ifdef XWIN_WINDOWS_DRI
  { WindowsDRIExtensionInit, "Windows-DRI", &noDriExtension },
#endif
#endif
};

/*
 * XwinExtensionInit
 * Initialises Xwin-specific extensions.
 */
static
void XwinExtensionInit(void)
{
#ifdef XWIN_GLX_WINDOWS
    if (g_fNativeGl) {
        /* install the native GL provider */
        glxWinPushNativeProvider();
    }
#endif

    LoadExtensionList(xwinExtensions, ARRAY_SIZE(xwinExtensions), TRUE);
}

#if defined(DDXBEFORERESET)
/*
 * Called right before KillAllClients when the server is going to reset,
 * allows us to shutdown our separate threads cleanly.
 */

void
ddxBeforeReset(void)
{
    winDebug("ddxBeforeReset - Hello\n");

    winClipboardShutdown();
}
#endif

#if INPUTTHREAD
/** This function is called in Xserver/os/inputthread.c when starting
    the input thread. */
void
ddxInputThreadInit(void)
{
}
#endif

int
main(int argc, char *argv[], char *envp[])
{
    int iReturn;

    /* Create & acquire the termination mutex */
    iReturn = pthread_mutex_init(&g_pmTerminating, NULL);
    if (iReturn != 0) {
        ErrorF("ddxMain - pthread_mutex_init () failed: %d\n", iReturn);
    }

    iReturn = pthread_mutex_lock(&g_pmTerminating);
    if (iReturn != 0) {
        ErrorF("ddxMain - pthread_mutex_lock () failed: %d\n", iReturn);
    }

    return dix_main(argc, argv, envp);
}

/* See Porting Layer Definition - p. 57 */
void
ddxGiveUp(enum ExitCode error)
{
    int i;

#if CYGDEBUG
    winDebug("ddxGiveUp\n");
#endif

    /* Perform per-screen deinitialization */
    for (i = 0; i < g_iNumScreens; ++i) {
        /* Delete the tray icon */
        if (!g_ScreenInfo[i].fNoTrayIcon && g_ScreenInfo[i].pScreen)
            winDeleteNotifyIcon(winGetScreenPriv(g_ScreenInfo[i].pScreen));
    }

    /* Unload libraries for taskbar grouping */
    winPropertyStoreDestroy();

    /* Notify the worker threads we're exiting */
    winDeinitMultiWindowWM();

#ifdef HAS_DEVWINDOWS
    /* Close our handle to our message queue */
    if (g_fdMessageQueue != WIN_FD_INVALID) {
        /* Close /dev/windows */
        close(g_fdMessageQueue);

        /* Set the file handle to invalid */
        g_fdMessageQueue = WIN_FD_INVALID;
    }
#endif

    if (!g_fLogInited) {
        g_pszLogFile = LogInit(g_pszLogFile, ".old");
        g_fLogInited = TRUE;
    }
    LogClose(error);

    /*
     * At this point we aren't creating any new screens, so
     * we are guaranteed to not need the DirectDraw functions.
     */
    winReleaseDDProcAddresses();

    /* Free concatenated command line */
    free(g_pszCommandLine);
    g_pszCommandLine = NULL;

    /* Remove our keyboard hook if it is installed */
    winRemoveKeyboardHookLL();

    /* Tell Windows that we want to end the app */
    PostQuitMessage(0);

    {
        int iReturn = pthread_mutex_unlock(&g_pmTerminating);

        winDebug("ddxGiveUp - Releasing termination mutex\n");

        if (iReturn != 0) {
            ErrorF("winMsgWindowProc - pthread_mutex_unlock () failed: %d\n",
                   iReturn);
        }
    }

    winDebug("ddxGiveUp - End\n");
}

#ifdef __CYGWIN__
/* hasmntopt is currently not implemented for cygwin */
static const char *
winCheckMntOpt(const struct mntent *mnt, const char *opt)
{
    const char *s;
    size_t len;

    if (mnt == NULL)
        return NULL;
    if (opt == NULL)
        return NULL;
    if (mnt->mnt_opts == NULL)
        return NULL;

    len = strlen(opt);
    s = strstr(mnt->mnt_opts, opt);
    if (s == NULL)
        return NULL;
    if ((s == mnt->mnt_opts || *(s - 1) == ',') &&
        (s[len] == 0 || s[len] == ','))
        return (char *) opt;
    return NULL;
}

static void
winCheckMount(void)
{
    FILE *mnt;
    struct mntent *ent;

    enum { none = 0, sys_root, user_root, sys_tmp, user_tmp }
        level = none, curlevel;
    BOOL binary = TRUE;

    mnt = setmntent("/etc/mtab", "r");
    if (mnt == NULL) {
        ErrorF("setmntent failed");
        return;
    }

    while ((ent = getmntent(mnt)) != NULL) {
        BOOL sys = (winCheckMntOpt(ent, "user") != NULL);
        BOOL root = (strcmp(ent->mnt_dir, "/") == 0);
        BOOL tmp = (strcmp(ent->mnt_dir, "/tmp") == 0);

        if (sys) {
            if (root)
                curlevel = sys_root;
            else if (tmp)
                curlevel = sys_tmp;
            else
                continue;
        }
        else {
            if (root)
                curlevel = user_root;
            else if (tmp)
                curlevel = user_tmp;
            else
                continue;
        }

        if (curlevel <= level)
            continue;
        level = curlevel;

        if ((winCheckMntOpt(ent, "binary") == NULL) &&
            (winCheckMntOpt(ent, "binmode") == NULL))
            binary = FALSE;
        else
            binary = TRUE;
    }

    if (endmntent(mnt) != 1) {
        ErrorF("endmntent failed");
        return;
    }

    if (!binary)
        winMsg(X_WARNING, "/tmp mounted in textmode\n");
}
#else
static void
winCheckMount(void)
{
}
#endif

#ifdef RELOCATE_PROJECTROOT
const char *
winGetBaseDir(void)
{
    static BOOL inited = FALSE;
    static char buffer[MAX_PATH];

    if (!inited) {
        char *fendptr;
        HMODULE module = GetModuleHandle(NULL);
        DWORD size = GetModuleFileName(module, buffer, sizeof(buffer));

        if (sizeof(buffer) > 0)
            buffer[sizeof(buffer) - 1] = 0;

        fendptr = buffer + size;
        while (fendptr > buffer) {
            if (*fendptr == '\\' || *fendptr == '/') {
                *fendptr = 0;
                break;
            }
            fendptr--;
        }
        inited = TRUE;
    }
    return buffer;
}
#endif

static void
winFixupPaths(void)
{
    BOOL changed_fontpath = FALSE;
    MessageType font_from = X_DEFAULT;

#ifdef RELOCATE_PROJECTROOT
    const char *basedir = winGetBaseDir();
    size_t basedirlen = strlen(basedir);
#endif

#ifdef READ_FONTDIRS
    {
        /* Open fontpath configuration file */
        FILE *fontdirs = fopen(ETCX11DIR "/font-dirs", "rt");

        if (fontdirs != NULL) {
            char buffer[256];
            int needs_sep = TRUE;
            int comment_block = FALSE;

            /* get default fontpath */
            char *fontpath = strdup(defaultFontPath);
            size_t size = strlen(fontpath);

            /* read all lines */
            while (!feof(fontdirs)) {
                size_t blen;
                char *hashchar;
                char *str;
                int has_eol = FALSE;

                /* read one line */
                str = fgets(buffer, sizeof(buffer), fontdirs);
                if (str == NULL)        /* stop on error or eof */
                    break;

                if (strchr(str, '\n') != NULL)
                    has_eol = TRUE;

                /* check if block is continued comment */
                if (comment_block) {
                    /* ignore all input */
                    *str = 0;
                    blen = 0;
                    if (has_eol)        /* check if line ended in this block */
                        comment_block = FALSE;
                }
                else {
                    /* find comment character. ignore all trailing input */
                    hashchar = strchr(str, '#');
                    if (hashchar != NULL) {
                        *hashchar = 0;
                        if (!has_eol)   /* mark next block as continued comment */
                            comment_block = TRUE;
                    }
                }

                /* strip whitespaces from beginning */
                while (*str == ' ' || *str == '\t')
                    str++;

                /* get size, strip whitespaces from end */
                blen = strlen(str);
                while (blen > 0 && (str[blen - 1] == ' ' ||
                                    str[blen - 1] == '\t' ||
                                    str[blen - 1] == '\n')) {
                    str[--blen] = 0;
                }

                /* still something left to add? */
                if (blen > 0) {
                    size_t newsize = size + blen;

                    /* reserve one character more for ',' */
                    if (needs_sep)
                        newsize++;

                    /* allocate memory */
                    if (fontpath == NULL)
                        fontpath = malloc(newsize + 1);
                    else
                        fontpath = realloc(fontpath, newsize + 1);

                    /* add separator */
                    if (needs_sep) {
                        fontpath[size] = ',';
                        size++;
                        needs_sep = FALSE;
                    }

                    /* mark next line as new entry */
                    if (has_eol)
                        needs_sep = TRUE;

                    /* add block */
                    strncpy(fontpath + size, str, blen);
                    fontpath[newsize] = 0;
                    size = newsize;
                }
            }

            /* cleanup */
            fclose(fontdirs);
            defaultFontPath = strdup(fontpath);
            free(fontpath);
            changed_fontpath = TRUE;
            font_from = X_CONFIG;
        }
    }
#endif                          /* READ_FONTDIRS */
#ifdef RELOCATE_PROJECTROOT
    {
        const char *libx11dir = PROJECTROOT "/lib/X11";
        size_t libx11dir_len = strlen(libx11dir);
        char *newfp = NULL;
        size_t newfp_len = 0;
        const char *endptr, *ptr, *oldptr = defaultFontPath;

        endptr = oldptr + strlen(oldptr);
        ptr = strchr(oldptr, ',');
        if (ptr == NULL)
            ptr = endptr;
        while (ptr != NULL) {
            size_t oldfp_len = (ptr - oldptr);
            size_t newsize = oldfp_len;
            char *newpath = malloc(newsize + 1);

            strncpy(newpath, oldptr, newsize);
            newpath[newsize] = 0;

            if (strncmp(libx11dir, newpath, libx11dir_len) == 0) {
                char *compose;

                newsize = newsize - libx11dir_len + basedirlen;
                compose = malloc(newsize + 1);
                strcpy(compose, basedir);
                strncat(compose, newpath + libx11dir_len, newsize - basedirlen);
                compose[newsize] = 0;
                free(newpath);
                newpath = compose;
            }

            oldfp_len = newfp_len;
            if (oldfp_len > 0)
                newfp_len++;    /* space for separator */
            newfp_len += newsize;

            if (newfp == NULL)
                newfp = malloc(newfp_len + 1);
            else
                newfp = realloc(newfp, newfp_len + 1);

            if (oldfp_len > 0) {
                strcpy(newfp + oldfp_len, ",");
                oldfp_len++;
            }
            strcpy(newfp + oldfp_len, newpath);

            free(newpath);

            if (*ptr == 0) {
                oldptr = ptr;
                ptr = NULL;
            }
            else {
                oldptr = ptr + 1;
                ptr = strchr(oldptr, ',');
                if (ptr == NULL)
                    ptr = endptr;
            }
        }

        defaultFontPath = strdup(newfp);
        free(newfp);
        changed_fontpath = TRUE;
    }
#endif                          /* RELOCATE_PROJECTROOT */
    if (changed_fontpath)
        winMsg(font_from, "FontPath set to \"%s\"\n", defaultFontPath);

#ifdef RELOCATE_PROJECTROOT
    if (getenv("XKEYSYMDB") == NULL) {
        char buffer[MAX_PATH];

        snprintf(buffer, sizeof(buffer), "XKEYSYMDB=%s\\XKeysymDB", basedir);
        buffer[sizeof(buffer) - 1] = 0;
        putenv(buffer);
    }
    if (getenv("XERRORDB") == NULL) {
        char buffer[MAX_PATH];

        snprintf(buffer, sizeof(buffer), "XERRORDB=%s\\XErrorDB", basedir);
        buffer[sizeof(buffer) - 1] = 0;
        putenv(buffer);
    }
    if (getenv("XLOCALEDIR") == NULL) {
        char buffer[MAX_PATH];

        snprintf(buffer, sizeof(buffer), "XLOCALEDIR=%s\\locale", basedir);
        buffer[sizeof(buffer) - 1] = 0;
        putenv(buffer);
    }
    if (getenv("HOME") == NULL) {
        char buffer[MAX_PATH + 5];

        strncpy(buffer, "HOME=", 5);

        /* query appdata directory */
        if (SHGetFolderPathA
            (NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0,
             buffer + 5) == 0) {
            putenv(buffer);
        }
        else {
            winMsg(X_ERROR, "Can not determine HOME directory\n");
        }
    }
    if (!g_fLogFileChanged) {
        static char buffer[MAX_PATH];
        DWORD size = GetTempPath(sizeof(buffer), buffer);

        if (size && size < sizeof(buffer)) {
            snprintf(buffer + size, sizeof(buffer) - size,
                     "XWin.%s.log", display);
            buffer[sizeof(buffer) - 1] = 0;
            g_pszLogFile = buffer;
            winMsg(X_DEFAULT, "Logfile set to \"%s\"\n", g_pszLogFile);
        }
    }
    {
        static char xkbbasedir[MAX_PATH];

        snprintf(xkbbasedir, sizeof(xkbbasedir), "%s\\xkb", basedir);
        if (sizeof(xkbbasedir) > 0)
            xkbbasedir[sizeof(xkbbasedir) - 1] = 0;
        XkbBaseDirectory = xkbbasedir;
        XkbBinDirectory = basedir;
    }
#endif                          /* RELOCATE_PROJECTROOT */
}

void
OsVendorInit(void)
{
    /* Re-initialize global variables on server reset */
    winInitializeGlobals();

    winFixupPaths();

#ifdef DDXOSVERRORF
    if (!OsVendorVErrorFProc)
        OsVendorVErrorFProc = OsVendorVErrorF;
#endif

    if (!g_fLogInited) {
        /* keep this order. If LogInit fails it calls Abort which then calls
         * ddxGiveUp where LogInit is called again and creates an infinite
         * recursion. If we set g_fLogInited to TRUE before the init we
         * avoid the second call
         */
        g_fLogInited = TRUE;
        g_pszLogFile = LogInit(g_pszLogFile, ".old");

    }
    LogSetParameter(XLOG_FLUSH, 1);
    LogSetParameter(XLOG_VERBOSITY, g_iLogVerbose);
    LogSetParameter(XLOG_FILE_VERBOSITY, g_iLogVerbose);

    /* Log the version information */
    if (serverGeneration == 1)
        winLogVersionInfo();

    winCheckMount();

    /* Add a default screen if no screens were specified */
    if (g_iNumScreens == 0) {
        winDebug("OsVendorInit - Creating default screen 0\n");

        /*
         * We need to initialize the default screen 0 if no -screen
         * arguments were processed.
         *
         * Add a screen 0 using the defaults set by winInitializeDefaultScreens()
         * and any additional default screen parameters given
         */
        winInitializeScreens(1);

        /* We have to flag this as an explicit screen, even though it isn't */
        g_ScreenInfo[0].fExplicitScreen = TRUE;
    }

    /* Work out what the default emulate3buttons setting should be, and apply
       it if nothing was explicitly specified */
    {
        int mouseButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);
        int j;

        for (j = 0; j < g_iNumScreens; j++) {
            if (g_ScreenInfo[j].iE3BTimeout == WIN_E3B_DEFAULT) {
                if (mouseButtons < 3) {
                    static Bool reportOnce = TRUE;

                    g_ScreenInfo[j].iE3BTimeout = WIN_DEFAULT_E3B_TIME;
                    if (reportOnce) {
                        reportOnce = FALSE;
                        winMsg(X_PROBED,
                               "Windows reports only %d mouse buttons, defaulting to -emulate3buttons\n",
                               mouseButtons);
                    }
                }
                else {
                    g_ScreenInfo[j].iE3BTimeout = WIN_E3B_OFF;
                }
            }
        }
    }

    /* Work out what the default resize setting should be, and apply it if it
     was not explicitly specified */
    {
        int j;
        for (j = 0; j < g_iNumScreens; j++) {
            if (g_ScreenInfo[j].iResizeMode == resizeDefault) {
                if (g_ScreenInfo[j].fFullScreen)
                    g_ScreenInfo[j].iResizeMode = resizeNotAllowed;
                else
                    g_ScreenInfo[j].iResizeMode = resizeWithRandr;
                }
        }
    }
}

static void
winUseMsg(void)
{
    ErrorF("\n");
    ErrorF("\n");
    ErrorF(EXECUTABLE_NAME " Device Dependent Usage:\n");
    ErrorF("\n");

    ErrorF("-[no]clipboard\n"
           "\tEnable [disable] the clipboard integration. Default is enabled.\n");

    ErrorF("-clipupdates num_boxes\n"
           "\tUse a clipping region to constrain shadow update blits to\n"
           "\tthe updated region when num_boxes, or more, are in the\n"
           "\tupdated region.\n");

    ErrorF("-[no]compositealpha\n"
           "\tX windows with per-pixel alpha are composited into the Windows desktop.\n");
    ErrorF("-[no]compositewm\n"
           "\tUse the Composite extension to keep a bitmap image of each top-level\n"
           "\tX window, so window contents which are occluded show correctly in\n"
           "\ttask bar and task switcher previews.\n");

#ifdef XWIN_XF86CONFIG
    ErrorF("-config\n" "\tSpecify a configuration file.\n");

    ErrorF("-configdir\n" "\tSpecify a configuration directory.\n");
#endif

    ErrorF("-depth bits_per_pixel\n"
           "\tSpecify an optional bitdepth to use in fullscreen mode\n"
           "\twith a DirectDraw engine.\n");

    ErrorF("-[no]emulate3buttons [timeout]\n"
           "\tEmulate 3 button mouse with an optional timeout in\n"
           "\tmilliseconds.\n");

#ifdef XWIN_EMULATEPSEUDO
    ErrorF("-emulatepseudo\n"
           "\tCreate a depth 8 PseudoColor visual when running in\n"
           "\tdepths 15, 16, 24, or 32, collectively known as TrueColor\n"
           "\tdepths.  The PseudoColor visual does not have correct colors,\n"
           "\tand it may crash, but it at least allows you to run your\n"
           "\tapplication in TrueColor modes.\n");
#endif

    ErrorF("-engine engine_type_id\n"
           "\tOverride the server's automatically selected engine type:\n"
           "\t\t1 - Shadow GDI\n"
           "\t\t4 - Shadow DirectDraw4 Non-Locking\n"
        );

    ErrorF("-fullscreen\n" "\tRun the server in fullscreen mode.\n");

    ErrorF("-[no]hostintitle\n"
           "\tIn multiwindow mode, add remote host names to window titles.\n");

    ErrorF("-icon icon_specifier\n" "\tSet screen window icon in windowed mode.\n");

    ErrorF("-ignoreinput\n" "\tIgnore keyboard and mouse input.\n");

#ifdef XWIN_XF86CONFIG
    ErrorF("-keyboard\n"
           "\tSpecify a keyboard device from the configuration file.\n");
#endif

    ErrorF("-[no]keyhook\n"
           "\tGrab special Windows keypresses like Alt-Tab or the Menu "
           "key.\n");

    ErrorF("-lesspointer\n"
           "\tHide the windows mouse pointer when it is over any\n"
           "\t" EXECUTABLE_NAME
           " window.  This prevents ghost cursors appearing when\n"
           "\tthe Windows cursor is drawn on top of the X cursor\n");

    ErrorF("-logfile filename\n" "\tWrite log messages to <filename>.\n");

    ErrorF("-logverbose verbosity\n"
           "\tSet the verbosity of log messages. [NOTE: Only a few messages\n"
           "\trespect the settings yet]\n"
           "\t\t0 - only print fatal error.\n"
           "\t\t1 - print additional configuration information.\n"
           "\t\t2 - print additional runtime information [default].\n"
           "\t\t3 - print debugging and tracing information.\n");

    ErrorF("-[no]multimonitors or -[no]multiplemonitors\n"
           "\tUse the entire virtual screen if multiple\n"
           "\tmonitors are present.\n");

    ErrorF("-multiwindow\n" "\tRun the server in multi-window mode.\n");

    ErrorF("-nodecoration\n"
           "\tDo not draw a window border, title bar, etc.  Windowed\n"
           "\tmode only.\n");

    ErrorF("-[no]primary\n"
           "\tWhen clipboard integration is enabled, map the X11 PRIMARY selection\n"
           "\tto the Windows clipboard. Default is enabled.\n");

    ErrorF("-refresh rate_in_Hz\n"
           "\tSpecify an optional refresh rate to use in fullscreen mode\n"
           "\twith a DirectDraw engine.\n");

    ErrorF("-resize=none|scrollbars|randr\n"
           "\tIn windowed mode, [don't] allow resizing of the window. 'scrollbars'\n"
           "\tmode gives the window scrollbars as needed, 'randr' mode uses the RANR\n"
           "\textension to resize the X screen.  'randr' is the default.\n");

    ErrorF("-rootless\n" "\tRun the server in rootless mode.\n");

    ErrorF("-screen scr_num [width height [x y] | [[WxH[+X+Y]][@m]] ]\n"
           "\tEnable screen scr_num and optionally specify a width and\n"
           "\theight and initial position for that screen. Additionally\n"
           "\ta monitor number can be specified to start the server on,\n"
           "\tat which point, all coordinates become relative to that\n"
           "\tmonitor. Examples:\n"
           "\t -screen 0 800x600+100+100@2 ; 2nd monitor offset 100,100 size 800x600\n"
           "\t -screen 0 1024x768@3        ; 3rd monitor size 1024x768\n"
           "\t -screen 0 @1 ; on 1st monitor using its full resolution (the default)\n");

    ErrorF("-swcursor\n"
           "\tDisable the usage of the Windows cursor and use the X11 software\n"
           "\tcursor instead.\n");

    ErrorF("-[no]trayicon\n"
           "\tDo not create a tray icon.  Default is to create one\n"
           "\ticon per screen.  You can globally disable tray icons with\n"
           "\t-notrayicon, then enable it for specific screens with\n"
           "\t-trayicon for those screens.\n");

    ErrorF("-[no]unixkill\n" "\tCtrl+Alt+Backspace exits the X Server.\n");

#ifdef XWIN_GLX_WINDOWS
    ErrorF("-[no]wgl\n"
           "\tEnable the GLX extension to use the native Windows WGL interface for hardware-accelerated OpenGL\n");
#endif

    ErrorF("-[no]winkill\n" "\tAlt+F4 exits the X Server.\n");

    ErrorF("-xkblayout XKBLayout\n"
           "\tEquivalent to XKBLayout in XF86Config files.\n"
           "\tFor example: -xkblayout de\n");

    ErrorF("-xkbmodel XKBModel\n"
           "\tEquivalent to XKBModel in XF86Config files.\n");

    ErrorF("-xkboptions XKBOptions\n"
           "\tEquivalent to XKBOptions in XF86Config files.\n");

    ErrorF("-xkbrules XKBRules\n"
           "\tEquivalent to XKBRules in XF86Config files.\n");

    ErrorF("-xkbvariant XKBVariant\n"
           "\tEquivalent to XKBVariant in XF86Config files.\n"
           "\tFor example: -xkbvariant nodeadkeys\n");
}

/* See Porting Layer Definition - p. 57 */
void
ddxUseMsg(void)
{
    /* Set a flag so that FatalError won't give duplicate warning message */
    g_fSilentFatalError = TRUE;

    winUseMsg();

    /* Log file will not be opened for UseMsg unless we open it now */
    if (!g_fLogInited) {
        g_pszLogFile = LogInit(g_pszLogFile, ".old");
        g_fLogInited = TRUE;
    }
    LogClose(EXIT_NO_ERROR);

    /* Notify user where UseMsg text can be found. */
    if (!g_fNoHelpMessageBox)
        winMessageBoxF("The " PROJECT_NAME " help text has been printed to "
                       "%s.\n"
                       "Please open %s to read the help text.\n",
                       MB_ICONINFORMATION, g_pszLogFile, g_pszLogFile);
}

/* See Porting Layer Definition - p. 20 */
/*
 * Do any global initialization, then initialize each screen.
 *
 * NOTE: We use ddxProcessArgument, so we don't need to touch argc and argv
 */

void
InitOutput(ScreenInfo * pScreenInfo, int argc, char *argv[])
{
    int i;

    if (serverGeneration == 1)
        XwinExtensionInit();

    /* Log the command line */
    winLogCommandLine(argc, argv);

#if CYGDEBUG
    winDebug("InitOutput\n");
#endif

    /* Validate command-line arguments */
    if (serverGeneration == 1 && !winValidateArgs()) {
        FatalError("InitOutput - Invalid command-line arguments found.  "
                   "Exiting.\n");
    }

#ifdef XWIN_XF86CONFIG
    /* Try to read the xorg.conf-style configuration file */
    if (!winReadConfigfile())
        winErrorFVerb(1, "InitOutput - Error reading config file\n");
#else
    winMsg(X_INFO, "xorg.conf is not supported\n");
    winMsg(X_INFO, "See http://x.cygwin.com/docs/faq/cygwin-x-faq.html "
           "for more information\n");
    winConfigFiles();
#endif

    /* Load preferences from XWinrc file */
    LoadPreferences();

    /* Setup global screen info parameters */
    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
    pScreenInfo->numPixmapFormats = ARRAY_SIZE(g_PixmapFormats);

    /* Describe how we want common pixmap formats padded */
    for (i = 0; i < ARRAY_SIZE(g_PixmapFormats); i++) {
        pScreenInfo->formats[i] = g_PixmapFormats[i];
    }

    /* Load pointers to DirectDraw functions */
    winGetDDProcAddresses();

    /* Detect supported engines */
    winDetectSupportedEngines();
    /* Load libraries for taskbar grouping */
    winPropertyStoreInit();

    /* Store the instance handle */
    g_hInstance = GetModuleHandle(NULL);

    /* Create the messaging window */
    if (serverGeneration == 1)
        winCreateMsgWindowThread();

    /* Initialize each screen */
    for (i = 0; i < g_iNumScreens; ++i) {
        /* Initialize the screen */
        if (-1 == AddScreen(winScreenInit, argc, argv)) {
            FatalError("InitOutput - Couldn't add screen %d", i);
        }
    }

  /*
     Unless full xinerama has been explicitly enabled, register all native screens with pseudoramiX
  */
  if (!noPanoramiXExtension)
      noPseudoramiXExtension = TRUE;

  if ((g_ScreenInfo[0].fMultipleMonitors) && !noPseudoramiXExtension)
    {
      int pass;

      PseudoramiXExtensionInit();

      /* Add primary monitor on pass 0, other monitors on pass 1, to ensure
       the primary monitor is first in XINERAMA list */
      for (pass = 0; pass < 2; pass++)
        {
          int iMonitor;

          for (iMonitor = 1; ; iMonitor++)
            {
              struct GetMonitorInfoData data;
              if (QueryMonitor(iMonitor, &data))
                {
                  MONITORINFO mi;
                  mi.cbSize = sizeof(MONITORINFO);

                  if (GetMonitorInfo(data.monitorHandle, &mi))
                    {
                      /* pass == 1 XOR primary monitor flags is set */
                      if ((!(pass == 1)) != (!(mi.dwFlags & MONITORINFOF_PRIMARY)))
                        {
                          /*
                            Note the screen origin in a normalized coordinate space where (0,0) is at the top left
                            of the native virtual desktop area
                          */
                          data.monitorOffsetX = data.monitorOffsetX - GetSystemMetrics(SM_XVIRTUALSCREEN);
                          data.monitorOffsetY = data.monitorOffsetY - GetSystemMetrics(SM_YVIRTUALSCREEN);

                          winDebug ("InitOutput - screen %d added at virtual desktop coordinate (%d,%d) (pseudoramiX) \n",
                                    iMonitor-1, data.monitorOffsetX, data.monitorOffsetY);

                          PseudoramiXAddScreen(data.monitorOffsetX, data.monitorOffsetY,
                                               data.monitorWidth, data.monitorHeight);
                        }
                    }
                }
              else
                break;
            }
        }
    }

    xorgGlxCreateVendor();

    /* Generate a cookie used by internal clients for authorization */
    if (g_fXdmcpEnabled || g_fAuthEnabled)
        winGenerateAuthorization();


#if CYGDEBUG || YES
    winDebug("InitOutput - Returning.\n");
#endif
}
