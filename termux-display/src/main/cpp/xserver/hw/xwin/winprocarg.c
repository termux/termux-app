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

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#include <../xfree86/common/xorgVersion.h>
#include "win.h"
#include "winconfig.h"
#include "winmsg.h"
#include "winmonitors.h"
#include "winprefs.h"

#include "winclipboard/winclipboard.h"

/*
 * Function prototypes
 */

void
 winLogCommandLine(int argc, char *argv[]);

void
 winLogVersionInfo(void);

/*
 * Process arguments on the command line
 */

static int iLastScreen = -1;
static winScreenInfo defaultScreenInfo;

static void
winInitializeScreenDefaults(void)
{
    DWORD dwWidth, dwHeight;
    static Bool fInitializedScreenDefaults = FALSE;

    /* Bail out early if default screen has already been initialized */
    if (fInitializedScreenDefaults)
        return;

    /* Zero the memory used for storing the screen info */
    memset(&defaultScreenInfo, 0, sizeof(winScreenInfo));

    /* Get default width and height */
    /*
     * NOTE: These defaults will cause the window to cover only
     * the primary monitor in the case that we have multiple monitors.
     */
    dwWidth = GetSystemMetrics(SM_CXSCREEN);
    dwHeight = GetSystemMetrics(SM_CYSCREEN);

    winErrorFVerb(2,
                  "winInitializeScreenDefaults - primary monitor w %d h %d\n",
                  (int) dwWidth, (int) dwHeight);

    /* Set a default DPI, if no '-dpi' option was used */
    if (monitorResolution == 0) {
        HDC hdc = GetDC(NULL);

        if (hdc) {
            int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
            int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

            winErrorFVerb(2,
                          "winInitializeScreenDefaults - native DPI x %d y %d\n",
                          dpiX, dpiY);

            monitorResolution = dpiY;
            ReleaseDC(NULL, hdc);
        }
        else {
            winErrorFVerb(1,
                          "winInitializeScreenDefaults - Failed to retrieve native DPI, falling back to default of %d DPI\n",
                          WIN_DEFAULT_DPI);
            monitorResolution = WIN_DEFAULT_DPI;
        }
    }

    defaultScreenInfo.iMonitor = 1;
    defaultScreenInfo.hMonitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
    defaultScreenInfo.dwWidth = dwWidth;
    defaultScreenInfo.dwHeight = dwHeight;
    defaultScreenInfo.dwUserWidth = dwWidth;
    defaultScreenInfo.dwUserHeight = dwHeight;
    defaultScreenInfo.fUserGaveHeightAndWidth =
        WIN_DEFAULT_USER_GAVE_HEIGHT_AND_WIDTH;
    defaultScreenInfo.fUserGavePosition = FALSE;
    defaultScreenInfo.dwBPP = WIN_DEFAULT_BPP;
    defaultScreenInfo.dwClipUpdatesNBoxes = WIN_DEFAULT_CLIP_UPDATES_NBOXES;
#ifdef XWIN_EMULATEPSEUDO
    defaultScreenInfo.fEmulatePseudo = WIN_DEFAULT_EMULATE_PSEUDO;
#endif
    defaultScreenInfo.dwRefreshRate = WIN_DEFAULT_REFRESH;
    defaultScreenInfo.pfb = NULL;
    defaultScreenInfo.fFullScreen = FALSE;
    defaultScreenInfo.fDecoration = TRUE;
    defaultScreenInfo.fRootless = FALSE;
    defaultScreenInfo.fMultiWindow = FALSE;
    defaultScreenInfo.fCompositeWM = TRUE;
    defaultScreenInfo.fMultiMonitorOverride = FALSE;
    defaultScreenInfo.fMultipleMonitors = FALSE;
    defaultScreenInfo.fLessPointer = FALSE;
    defaultScreenInfo.iResizeMode = resizeDefault;
    defaultScreenInfo.fNoTrayIcon = FALSE;
    defaultScreenInfo.iE3BTimeout = WIN_E3B_DEFAULT;
    defaultScreenInfo.fUseWinKillKey = WIN_DEFAULT_WIN_KILL;
    defaultScreenInfo.fUseUnixKillKey = WIN_DEFAULT_UNIX_KILL;
    defaultScreenInfo.fIgnoreInput = FALSE;
    defaultScreenInfo.fExplicitScreen = FALSE;
    defaultScreenInfo.hIcon = (HICON)
        LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_XWIN), IMAGE_ICON,
                  GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    defaultScreenInfo.hIconSm = (HICON)
        LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_XWIN), IMAGE_ICON,
                  GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                  LR_DEFAULTSIZE);

    /* Note that the default screen has been initialized */
    fInitializedScreenDefaults = TRUE;
}

static void
winInitializeScreen(int i)
{
    winErrorFVerb(3, "winInitializeScreen - %d\n", i);

    /* Initialize default screen values, if needed */
    winInitializeScreenDefaults();

    /* Copy the default screen info */
    g_ScreenInfo[i] = defaultScreenInfo;

    /* Set the screen number */
    g_ScreenInfo[i].dwScreen = i;
}

void
winInitializeScreens(int maxscreens)
{
    int i;

    winErrorFVerb(3, "winInitializeScreens - %i\n", maxscreens);

    if (maxscreens > g_iNumScreens) {
        /* Reallocate the memory for DDX-specific screen info */
        g_ScreenInfo =
            realloc(g_ScreenInfo, maxscreens * sizeof(winScreenInfo));

        /* Set default values for any new screens */
        for (i = g_iNumScreens; i < maxscreens; i++)
            winInitializeScreen(i);

        /* Keep a count of the number of screens */
        g_iNumScreens = maxscreens;
    }
}

/* See Porting Layer Definition - p. 57 */
/*
 * INPUT
 * argv: pointer to an array of null-terminated strings, one for
 *   each token in the X Server command line; the first token
 *   is 'XWin.exe', or similar.
 * argc: a count of the number of tokens stored in argv.
 * i: a zero-based index into argv indicating the current token being
 *   processed.
 *
 * OUTPUT
 * return: return the number of tokens processed correctly.
 *
 * NOTE
 * When looking for n tokens, check that i + n is less than argc.  Or,
 *   you may check if i is greater than or equal to argc, in which case
 *   you should display the UseMsg () and return 0.
 */

/* Check if enough arguments are given for the option */
#define CHECK_ARGS(count) if (i + count >= argc) { UseMsg (); return 0; }

/* Compare the current option with the string. */
#define IS_OPTION(name) (strcmp (argv[i], name) == 0)

int
ddxProcessArgument(int argc, char *argv[], int i)
{
    static Bool s_fBeenHere = FALSE;
    winScreenInfo *screenInfoPtr = NULL;

    /* Initialize once */
    if (!s_fBeenHere) {
#ifdef DDXOSVERRORF
        /*
         * This initialises our hook into VErrorF () for catching log messages
         * that are generated before OsInit () is called.
         */
        OsVendorVErrorFProc = OsVendorVErrorF;
#endif

        s_fBeenHere = TRUE;

        /* Initialize only if option is not -help */
        if (!IS_OPTION("-help") && !IS_OPTION("-h") && !IS_OPTION("--help") &&
            !IS_OPTION("-version") && !IS_OPTION("--version")) {

            /* Log the version information */
            winLogVersionInfo();

            /* Log the command line */
            winLogCommandLine(argc, argv);

            /*
             * Initialize default screen settings.  We have to do this before
             * OsVendorInit () gets called, otherwise we will overwrite
             * settings changed by parameters such as -fullscreen, etc.
             */
            winErrorFVerb(3, "ddxProcessArgument - Initializing default "
                          "screens\n");
            winInitializeScreenDefaults();
        }
    }

#if CYGDEBUG
    winDebug("ddxProcessArgument - arg: %s\n", argv[i]);
#endif

    /*
     * Look for the '-help' and similar options
     */
    if (IS_OPTION("-help") || IS_OPTION("-h") || IS_OPTION("--help")) {
        /* Reset logfile. We don't need that helpmessage in the logfile */
        g_pszLogFile = NULL;
        g_fNoHelpMessageBox = TRUE;
        UseMsg();
        exit(0);
        return 1;
    }

    if (IS_OPTION("-version") || IS_OPTION("--version")) {
        /* Reset logfile. We don't need that versioninfo in the logfile */
        g_pszLogFile = NULL;
        winLogVersionInfo();
        exit(0);
        return 1;
    }

    /*
     * Look for the '-screen scr_num [width height]' argument
     */
    if (IS_OPTION("-screen")) {
        int iArgsProcessed = 1;
        int nScreenNum;
        int iWidth, iHeight, iX, iY;
        int iMonitor;

#if CYGDEBUG
        winDebug("ddxProcessArgument - screen - argc: %d i: %d\n", argc, i);
#endif

        /* Display the usage message if the argument is malformed */
        if (i + 1 >= argc) {
            return 0;
        }

        /* Grab screen number */
        nScreenNum = atoi(argv[i + 1]);

        /* Validate the specified screen number */
        if (nScreenNum < 0) {
            ErrorF("ddxProcessArgument - screen - Invalid screen number %d\n",
                   nScreenNum);
            UseMsg();
            return 0;
        }

        /*
           Initialize default values for any new screens

           Note that default values can't change after a -screen option is
           seen, so it's safe to do this for each screen as it is introduced
         */
        winInitializeScreens(nScreenNum + 1);

        /* look for @m where m is monitor number */
        if (i + 2 < argc && 1 == sscanf(argv[i + 2], "@%d", (int *) &iMonitor)) {
            struct GetMonitorInfoData data;

            if (QueryMonitor(iMonitor, &data)) {
                winErrorFVerb(2,
                              "ddxProcessArgument - screen - Found Valid ``@Monitor'' = %d arg\n",
                              iMonitor);
                iArgsProcessed = 3;
                g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth = FALSE;
                g_ScreenInfo[nScreenNum].fUserGavePosition = TRUE;
                g_ScreenInfo[nScreenNum].iMonitor = iMonitor;
                g_ScreenInfo[nScreenNum].hMonitor = data.monitorHandle;
                g_ScreenInfo[nScreenNum].dwWidth = data.monitorWidth;
                g_ScreenInfo[nScreenNum].dwHeight = data.monitorHeight;
                g_ScreenInfo[nScreenNum].dwUserWidth = data.monitorWidth;
                g_ScreenInfo[nScreenNum].dwUserHeight = data.monitorHeight;
                g_ScreenInfo[nScreenNum].dwInitialX = data.monitorOffsetX;
                g_ScreenInfo[nScreenNum].dwInitialY = data.monitorOffsetY;
            }
            else {
                /* monitor does not exist, error out */
                ErrorF
                    ("ddxProcessArgument - screen - Invalid monitor number %d\n",
                     iMonitor);
                exit(1);
                return 0;
            }
        }

        /* Look for 'WxD' or 'W D' */
        else if (i + 2 < argc
                 && 2 == sscanf(argv[i + 2], "%dx%d",
                                (int *) &iWidth, (int *) &iHeight)) {
            winErrorFVerb(2,
                          "ddxProcessArgument - screen - Found ``WxD'' arg\n");
            iArgsProcessed = 3;
            g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth = TRUE;
            g_ScreenInfo[nScreenNum].dwWidth = iWidth;
            g_ScreenInfo[nScreenNum].dwHeight = iHeight;
            g_ScreenInfo[nScreenNum].dwUserWidth = iWidth;
            g_ScreenInfo[nScreenNum].dwUserHeight = iHeight;
            /* Look for WxD+X+Y */
            if (2 == sscanf(argv[i + 2], "%*dx%*d+%d+%d",
                            (int *) &iX, (int *) &iY)) {
                winErrorFVerb(2,
                              "ddxProcessArgument - screen - Found ``X+Y'' arg\n");
                g_ScreenInfo[nScreenNum].fUserGavePosition = TRUE;
                g_ScreenInfo[nScreenNum].dwInitialX = iX;
                g_ScreenInfo[nScreenNum].dwInitialY = iY;

                /* look for WxD+X+Y@m where m is monitor number. take X,Y to be offsets from monitor's root position */
                if (1 == sscanf(argv[i + 2], "%*dx%*d+%*d+%*d@%d",
                                (int *) &iMonitor)) {
                    struct GetMonitorInfoData data;

                    if (QueryMonitor(iMonitor, &data)) {
                        g_ScreenInfo[nScreenNum].iMonitor = iMonitor;
                        g_ScreenInfo[nScreenNum].hMonitor = data.monitorHandle;
                        g_ScreenInfo[nScreenNum].dwInitialX +=
                            data.monitorOffsetX;
                        g_ScreenInfo[nScreenNum].dwInitialY +=
                            data.monitorOffsetY;
                    }
                    else {
                        /* monitor does not exist, error out */
                        ErrorF
                            ("ddxProcessArgument - screen - Invalid monitor number %d\n",
                             iMonitor);
                        exit(1);
                        return 0;
                    }
                }
            }

            /* look for WxD@m where m is monitor number */
            else if (1 == sscanf(argv[i + 2], "%*dx%*d@%d", (int *) &iMonitor)) {
                struct GetMonitorInfoData data;

                if (QueryMonitor(iMonitor, &data)) {
                    winErrorFVerb(2,
                                  "ddxProcessArgument - screen - Found Valid ``@Monitor'' = %d arg\n",
                                  iMonitor);
                    g_ScreenInfo[nScreenNum].fUserGavePosition = TRUE;
                    g_ScreenInfo[nScreenNum].iMonitor = iMonitor;
                    g_ScreenInfo[nScreenNum].hMonitor = data.monitorHandle;
                    g_ScreenInfo[nScreenNum].dwInitialX = data.monitorOffsetX;
                    g_ScreenInfo[nScreenNum].dwInitialY = data.monitorOffsetY;
                }
                else {
                    /* monitor does not exist, error out */
                    ErrorF
                        ("ddxProcessArgument - screen - Invalid monitor number %d\n",
                         iMonitor);
                    exit(1);
                    return 0;
                }
            }
        }
        else if (i + 3 < argc && 1 == sscanf(argv[i + 2], "%d", (int *) &iWidth)
                 && 1 == sscanf(argv[i + 3], "%d", (int *) &iHeight)) {
            winErrorFVerb(2,
                          "ddxProcessArgument - screen - Found ``W D'' arg\n");
            iArgsProcessed = 4;
            g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth = TRUE;
            g_ScreenInfo[nScreenNum].dwWidth = iWidth;
            g_ScreenInfo[nScreenNum].dwHeight = iHeight;
            g_ScreenInfo[nScreenNum].dwUserWidth = iWidth;
            g_ScreenInfo[nScreenNum].dwUserHeight = iHeight;
            if (i + 5 < argc && 1 == sscanf(argv[i + 4], "%d", (int *) &iX)
                && 1 == sscanf(argv[i + 5], "%d", (int *) &iY)) {
                winErrorFVerb(2,
                              "ddxProcessArgument - screen - Found ``X Y'' arg\n");
                iArgsProcessed = 6;
                g_ScreenInfo[nScreenNum].fUserGavePosition = TRUE;
                g_ScreenInfo[nScreenNum].dwInitialX = iX;
                g_ScreenInfo[nScreenNum].dwInitialY = iY;
            }
        }
        else {
            winErrorFVerb(2,
                          "ddxProcessArgument - screen - Did not find size arg. "
                          "dwWidth: %d dwHeight: %d\n",
                          (int) g_ScreenInfo[nScreenNum].dwWidth,
                          (int) g_ScreenInfo[nScreenNum].dwHeight);
            iArgsProcessed = 2;
            g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth = FALSE;
        }

        /* Flag that this screen was explicitly specified by the user */
        g_ScreenInfo[nScreenNum].fExplicitScreen = TRUE;

        /*
         * Keep track of the last screen number seen, as parameters seen
         * before a screen number apply to all screens, whereas parameters
         * seen after a screen number apply to that screen number only.
         */
        iLastScreen = nScreenNum;

        return iArgsProcessed;
    }

    /*
     * Is this parameter attached to a screen or global?
     *
     * If the parameter is for all screens (appears before
     * any -screen option), store it in the default screen
     * info
     *
     * If the parameter is for a single screen (appears
     * after a -screen option), store it in the screen info
     * for that screen
     *
     */
    if (iLastScreen == -1) {
        screenInfoPtr = &defaultScreenInfo;
    }
    else {
        screenInfoPtr = &(g_ScreenInfo[iLastScreen]);
    }

    /*
     * Look for the '-engine n' argument
     */
    if (IS_OPTION("-engine")) {
        DWORD dwEngine = 0;
        CARD8 c8OnBits = 0;

        /* Display the usage message if the argument is malformed */
        if (++i >= argc) {
            UseMsg();
            return 0;
        }

        /* Grab the argument */
        dwEngine = atoi(argv[i]);

        /* Count the one bits in the engine argument */
        c8OnBits = winCountBits(dwEngine);

        /* Argument should only have a single bit on */
        if (c8OnBits != 1) {
            UseMsg();
            return 0;
        }

        screenInfoPtr->dwEnginePreferred = dwEngine;

        /* Indicate that we have processed the argument */
        return 2;
    }

    /*
     * Look for the '-fullscreen' argument
     */
    if (IS_OPTION("-fullscreen")) {
        if (!screenInfoPtr->fMultiMonitorOverride)
            screenInfoPtr->fMultipleMonitors = FALSE;
        screenInfoPtr->fFullScreen = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-lesspointer' argument
     */
    if (IS_OPTION("-lesspointer")) {
        screenInfoPtr->fLessPointer = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-nodecoration' argument
     */
    if (IS_OPTION("-nodecoration")) {
        if (!screenInfoPtr->fMultiMonitorOverride)
            screenInfoPtr->fMultipleMonitors = FALSE;
        screenInfoPtr->fDecoration = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-rootless' argument
     */
    if (IS_OPTION("-rootless")) {
        if (!screenInfoPtr->fMultiMonitorOverride)
            screenInfoPtr->fMultipleMonitors = FALSE;
        screenInfoPtr->fRootless = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-multiwindow' argument
     */
    if (IS_OPTION("-multiwindow")) {
        if (!screenInfoPtr->fMultiMonitorOverride)
            screenInfoPtr->fMultipleMonitors = TRUE;
        screenInfoPtr->fMultiWindow = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-compositewm' argument
     */
    if (IS_OPTION("-compositewm")) {
        screenInfoPtr->fCompositeWM = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }
    /*
     * Look for the '-nocompositewm' argument
     */
    if (IS_OPTION("-nocompositewm")) {
        screenInfoPtr->fCompositeWM = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-compositealpha' argument
     */
    if (IS_OPTION("-compositealpha")) {
        g_fCompositeAlpha = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }
    /*
     * Look for the '-nocompositealpha' argument
     */
    if (IS_OPTION("-nocompositealpha")) {
        g_fCompositeAlpha  = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-multiplemonitors' argument
     */
    if (IS_OPTION("-multiplemonitors")
        || IS_OPTION("-multimonitors")) {
        screenInfoPtr->fMultiMonitorOverride = TRUE;
        screenInfoPtr->fMultipleMonitors = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-nomultiplemonitors' argument
     */
    if (IS_OPTION("-nomultiplemonitors")
        || IS_OPTION("-nomultimonitors")) {
        screenInfoPtr->fMultiMonitorOverride = TRUE;
        screenInfoPtr->fMultipleMonitors = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-scrollbars' argument
     */
    if (IS_OPTION("-scrollbars")) {

        screenInfoPtr->iResizeMode = resizeWithScrollbars;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-resize' argument
     */
    if (IS_OPTION("-resize") || IS_OPTION("-noresize") ||
        (strncmp(argv[i], "-resize=", strlen("-resize=")) == 0)) {
        winResizeMode mode;

        if (IS_OPTION("-resize"))
            mode = resizeWithRandr;
        else if (IS_OPTION("-noresize"))
            mode = resizeNotAllowed;
        else if (strncmp(argv[i], "-resize=", strlen("-resize=")) == 0) {
            char *option = argv[i] + strlen("-resize=");

            if (strcmp(option, "randr") == 0)
                mode = resizeWithRandr;
            else if (strcmp(option, "scrollbars") == 0)
                mode = resizeWithScrollbars;
            else if (strcmp(option, "none") == 0)
                mode = resizeNotAllowed;
            else {
                ErrorF("ddxProcessArgument - resize - Invalid resize mode %s\n",
                       option);
                return 0;
            }
        }
        else {
            ErrorF("ddxProcessArgument - resize - Invalid resize option %s\n",
                   argv[i]);
            return 0;
        }

        screenInfoPtr->iResizeMode = mode;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-clipboard' argument
     */
    if (IS_OPTION("-clipboard")) {
        /* Now the default, we still accept the arg for backwards compatibility */
        g_fClipboard = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-noclipboard' argument
     */
    if (IS_OPTION("-noclipboard")) {
        g_fClipboard = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-primary' argument
     */
    if (IS_OPTION("-primary")) {
        fPrimarySelection = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-noprimary' argument
     */
    if (IS_OPTION("-noprimary")) {
        fPrimarySelection = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-ignoreinput' argument
     */
    if (IS_OPTION("-ignoreinput")) {
        screenInfoPtr->fIgnoreInput = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-emulate3buttons' argument
     */
    if (IS_OPTION("-emulate3buttons")) {
        int iArgsProcessed = 1;
        int iE3BTimeout = WIN_DEFAULT_E3B_TIME;

        /* Grab the optional timeout value */
        if (i + 1 < argc && 1 == sscanf(argv[i + 1], "%d", &iE3BTimeout)) {
            /* Indicate that we have processed the next argument */
            iArgsProcessed++;
        }
        else {
            /*
             * sscanf () won't modify iE3BTimeout if it doesn't find
             * the specified format; however, I want to be explicit
             * about setting the default timeout in such cases to
             * prevent some programs (me) from getting confused.
             */
            iE3BTimeout = WIN_DEFAULT_E3B_TIME;
        }

        screenInfoPtr->iE3BTimeout = iE3BTimeout;

        /* Indicate that we have processed this argument */
        return iArgsProcessed;
    }

    /*
     * Look for the '-noemulate3buttons' argument
     */
    if (IS_OPTION("-noemulate3buttons")) {
        screenInfoPtr->iE3BTimeout = WIN_E3B_OFF;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-depth n' argument
     */
    if (IS_OPTION("-depth")) {
        DWORD dwBPP = 0;

        /* Display the usage message if the argument is malformed */
        if (++i >= argc) {
            UseMsg();
            return 0;
        }

        /* Grab the argument */
        dwBPP = atoi(argv[i]);

        screenInfoPtr->dwBPP = dwBPP;

        /* Indicate that we have processed the argument */
        return 2;
    }

    /*
     * Look for the '-refresh n' argument
     */
    if (IS_OPTION("-refresh")) {
        DWORD dwRefreshRate = 0;

        /* Display the usage message if the argument is malformed */
        if (++i >= argc) {
            UseMsg();
            return 0;
        }

        /* Grab the argument */
        dwRefreshRate = atoi(argv[i]);

        screenInfoPtr->dwRefreshRate = dwRefreshRate;

        /* Indicate that we have processed the argument */
        return 2;
    }

    /*
     * Look for the '-clipupdates num_boxes' argument
     */
    if (IS_OPTION("-clipupdates")) {
        DWORD dwNumBoxes = 0;

        /* Display the usage message if the argument is malformed */
        if (++i >= argc) {
            UseMsg();
            return 0;
        }

        /* Grab the argument */
        dwNumBoxes = atoi(argv[i]);

        screenInfoPtr->dwClipUpdatesNBoxes = dwNumBoxes;

        /* Indicate that we have processed the argument */
        return 2;
    }

#ifdef XWIN_EMULATEPSEUDO
    /*
     * Look for the '-emulatepseudo' argument
     */
    if (IS_OPTION("-emulatepseudo")) {
        screenInfoPtr->fEmulatePseudo = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }
#endif

    /*
     * Look for the '-nowinkill' argument
     */
    if (IS_OPTION("-nowinkill")) {
        screenInfoPtr->fUseWinKillKey = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-winkill' argument
     */
    if (IS_OPTION("-winkill")) {
        screenInfoPtr->fUseWinKillKey = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-nounixkill' argument
     */
    if (IS_OPTION("-nounixkill")) {
        screenInfoPtr->fUseUnixKillKey = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-unixkill' argument
     */
    if (IS_OPTION("-unixkill")) {
        screenInfoPtr->fUseUnixKillKey = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-notrayicon' argument
     */
    if (IS_OPTION("-notrayicon")) {
        screenInfoPtr->fNoTrayIcon = TRUE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-trayicon' argument
     */
    if (IS_OPTION("-trayicon")) {
        screenInfoPtr->fNoTrayIcon = FALSE;

        /* Indicate that we have processed this argument */
        return 1;
    }

    /*
     * Look for the '-fp' argument
     */
    if (IS_OPTION("-fp")) {
        CHECK_ARGS(1);
        g_cmdline.fontPath = argv[++i];
        return 0;               /* Let DIX parse this again */
    }

    /*
     * Look for the '-query' argument
     */
    if (IS_OPTION("-query")) {
        CHECK_ARGS(1);
        g_fXdmcpEnabled = TRUE;
        g_pszQueryHost = argv[++i];
        return 0;               /* Let DIX parse this again */
    }

    /*
     * Look for the '-auth' argument
     */
    if (IS_OPTION("-auth")) {
        g_fAuthEnabled = TRUE;
        return 0;               /* Let DIX parse this again */
    }

    /*
     * Look for the '-indirect' or '-broadcast' arguments
     */
    if (IS_OPTION("-indirect")
        || IS_OPTION("-broadcast")) {
        g_fXdmcpEnabled = TRUE;
        return 0;               /* Let DIX parse this again */
    }

    /*
     * Look for the '-config' argument
     */
    if (IS_OPTION("-config")
        || IS_OPTION("-xf86config")) {
        CHECK_ARGS(1);
#ifdef XWIN_XF86CONFIG
        g_cmdline.configFile = argv[++i];
#else
        winMessageBoxF("The %s option is not supported in this "
                       "release.\n"
                       "Ignoring this option and continuing.\n",
                       MB_ICONINFORMATION, argv[i]);
#endif
        return 2;
    }

    /*
     * Look for the '-configdir' argument
     */
    if (IS_OPTION("-configdir")) {
        CHECK_ARGS(1);
#ifdef XWIN_XF86CONFIG
        g_cmdline.configDir = argv[++i];
#else
        winMessageBoxF("The %s option is not supported in this "
                       "release.\n"
                       "Ignoring this option and continuing.\n",
                       MB_ICONINFORMATION, argv[i]);
#endif
        return 2;
    }

    /*
     * Look for the '-keyboard' argument
     */
    if (IS_OPTION("-keyboard")) {
#ifdef XWIN_XF86CONFIG
        CHECK_ARGS(1);
        g_cmdline.keyboard = argv[++i];
#else
        winMessageBoxF("The -keyboard option is not supported in this "
                       "release.\n"
                       "Ignoring this option and continuing.\n",
                       MB_ICONINFORMATION);
#endif
        return 2;
    }

    /*
     * Look for the '-logfile' argument
     */
    if (IS_OPTION("-logfile")) {
        CHECK_ARGS(1);
        g_pszLogFile = argv[++i];
#ifdef RELOCATE_PROJECTROOT
        g_fLogFileChanged = TRUE;
#endif
        return 2;
    }

    /*
     * Look for the '-logverbose' argument
     */
    if (IS_OPTION("-logverbose")) {
        CHECK_ARGS(1);
        g_iLogVerbose = atoi(argv[++i]);
        return 2;
    }

    if (IS_OPTION("-xkbrules")) {
        CHECK_ARGS(1);
        g_cmdline.xkbRules = argv[++i];
        return 2;
    }
    if (IS_OPTION("-xkbmodel")) {
        CHECK_ARGS(1);
        g_cmdline.xkbModel = argv[++i];
        return 2;
    }
    if (IS_OPTION("-xkblayout")) {
        CHECK_ARGS(1);
        g_cmdline.xkbLayout = argv[++i];
        return 2;
    }
    if (IS_OPTION("-xkbvariant")) {
        CHECK_ARGS(1);
        g_cmdline.xkbVariant = argv[++i];
        return 2;
    }
    if (IS_OPTION("-xkboptions")) {
        CHECK_ARGS(1);
        g_cmdline.xkbOptions = argv[++i];
        return 2;
    }

    if (IS_OPTION("-keyhook")) {
        g_fKeyboardHookLL = TRUE;
        return 1;
    }

    if (IS_OPTION("-nokeyhook")) {
        g_fKeyboardHookLL = FALSE;
        return 1;
    }

    if (IS_OPTION("-swcursor")) {
        g_fSoftwareCursor = TRUE;
        return 1;
    }

    if (IS_OPTION("-wgl")) {
        g_fNativeGl = TRUE;
        return 1;
    }

    if (IS_OPTION("-nowgl")) {
        g_fNativeGl = FALSE;
        return 1;
    }

    if (IS_OPTION("-hostintitle")) {
        g_fHostInTitle = TRUE;
        return 1;
    }

    if (IS_OPTION("-nohostintitle")) {
        g_fHostInTitle = FALSE;
        return 1;
    }

    if (IS_OPTION("-icon")) {
        char *iconspec;
        CHECK_ARGS(1);
        iconspec = argv[++i];
        screenInfoPtr->hIcon = LoadImageComma(iconspec, NULL,
                                              GetSystemMetrics(SM_CXICON),
                                              GetSystemMetrics(SM_CYICON),
                                              0);
        screenInfoPtr->hIconSm = LoadImageComma(iconspec, NULL,
                                                GetSystemMetrics(SM_CXSMICON),
                                                GetSystemMetrics(SM_CYSMICON),
                                                LR_DEFAULTSIZE);
        if ((screenInfoPtr->hIcon == NULL) ||
            (screenInfoPtr->hIconSm == NULL)) {
            ErrorF("ddxProcessArgument - icon - Invalid icon specification %s\n",
                   iconspec);
            exit(1);
        }

        /* Indicate that we have processed the argument */
        return 2;
    }

    return 0;
}

/*
 * winLogCommandLine - Write entire command line to the log file
 */

void
winLogCommandLine(int argc, char *argv[])
{
    int i;
    int iSize = 0;
    int iCurrLen = 0;

#define CHARS_PER_LINE 60

    /* Bail if command line has already been logged */
    if (g_pszCommandLine)
        return;

    /* Count how much memory is needed for concatenated command line */
    for (i = 0, iCurrLen = 0; i < argc; ++i)
        if (argv[i]) {
            /* Adds two characters for lines that overflow */
            if ((strlen(argv[i]) < CHARS_PER_LINE
                 && iCurrLen + strlen(argv[i]) > CHARS_PER_LINE)
                || strlen(argv[i]) > CHARS_PER_LINE) {
                iCurrLen = 0;
                iSize += 2;
            }

            /* Add space for item and trailing space */
            iSize += strlen(argv[i]) + 1;

            /* Update current line length */
            iCurrLen += strlen(argv[i]);
        }

    /* Allocate memory for concatenated command line */
    g_pszCommandLine = malloc(iSize + 1);
    if (!g_pszCommandLine)
        FatalError("winLogCommandLine - Could not allocate memory for "
                   "command line string.  Exiting.\n");

    /* Set first character to concatenated command line to null */
    g_pszCommandLine[0] = '\0';

    /* Loop through all args */
    for (i = 0, iCurrLen = 0; i < argc; ++i) {
        /* Add a character for lines that overflow */
        if ((strlen(argv[i]) < CHARS_PER_LINE
             && iCurrLen + strlen(argv[i]) > CHARS_PER_LINE)
            || strlen(argv[i]) > CHARS_PER_LINE) {
            iCurrLen = 0;

            /* Add line break if it fits */
            strncat(g_pszCommandLine, "\n ", iSize - strlen(g_pszCommandLine));
        }

        strncat(g_pszCommandLine, argv[i], iSize - strlen(g_pszCommandLine));
        strncat(g_pszCommandLine, " ", iSize - strlen(g_pszCommandLine));

        /* Save new line length */
        iCurrLen += strlen(argv[i]);
    }

    ErrorF("XWin was started with the following command line:\n\n"
           "%s\n\n", g_pszCommandLine);
}

/*
 * winLogVersionInfo - Log version information
 */

void
winLogVersionInfo(void)
{
    static Bool s_fBeenHere = FALSE;

    if (s_fBeenHere)
        return;
    s_fBeenHere = TRUE;

    ErrorF("Welcome to the XWin X Server\n");
    ErrorF("Vendor: %s\n", XVENDORNAME);
    ErrorF("Release: %d.%d.%d.%d\n", XORG_VERSION_MAJOR,
           XORG_VERSION_MINOR, XORG_VERSION_PATCH, XORG_VERSION_SNAP);
#ifdef HAVE_SYS_UTSNAME_H
    {
        struct utsname name;

        if (uname(&name) >= 0) {
            ErrorF("OS: %s %s %s %s %s\n", name.sysname, name.nodename,
                   name.release, name.version, name.machine);
        }
    }
#endif
    winOS();
    if (strlen(BUILDERSTRING))
        ErrorF("%s\n", BUILDERSTRING);
    ErrorF("Contact: %s\n", BUILDERADDR);
    ErrorF("\n");
}
