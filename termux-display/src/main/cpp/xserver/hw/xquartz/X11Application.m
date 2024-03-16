/* X11Application.m -- subclass of NSApplication to multiplex events
 *
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#import "X11Application.h"
#import "NSUserDefaults+XQuartzDefaults.h"

#include "darwin.h"
#include "quartz.h"
#include "darwinEvents.h"
#include "quartzKeyboard.h"
#include <X11/extensions/applewmconst.h>
#include "micmap.h"
#include "exglobals.h"

#include <mach/mach.h>
#include <unistd.h>

#include <pthread.h>

#include <Xplugin.h>

// pbproxy/pbproxy.h
extern int
xpbproxy_run(void);

#ifndef XSERVER_VERSION
#define XSERVER_VERSION "?"
#endif

#include <dispatch/dispatch.h>

static dispatch_queue_t eventTranslationQueue;

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#ifndef CF_RETURNS_RETAINED
#if __has_feature(attribute_cf_returns_retained)
#define CF_RETURNS_RETAINED __attribute__((cf_returns_retained))
#else
#define CF_RETURNS_RETAINED
#endif
#endif

#ifndef APPKIT_APPFLAGS_HACK
#define APPKIT_APPFLAGS_HACK 1
#endif

extern Bool noTestExtensions;
extern Bool noRenderExtension;

static TISInputSourceRef last_key_layout;

/* This preference is only tested on Lion or later as it's not relevant to
 * earlier OS versions.
 */
Bool XQuartzScrollInDeviceDirection = FALSE;

extern int darwinFakeButtons;

/* Store the mouse location while in the background, and update X11's pointer
 * location when we become the foreground application
 */
static NSPoint bgMouseLocation;
static BOOL bgMouseLocationUpdated = FALSE;

X11Application *X11App;

#define ALL_KEY_MASKS (NSShiftKeyMask | NSControlKeyMask | \
                       NSAlternateKeyMask | NSCommandKeyMask)

#if APPKIT_APPFLAGS_HACK && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101500
// This was removed from the SDK in 10.15
@interface NSApplication () {
@protected
    /* All instance variables are private */
    short               _running;
    struct __appFlags {
        unsigned int        _hidden:1;
        unsigned int        _appleEventActivationInProgress:1;
        unsigned int        _active:1;
        unsigned int        _hasBeenRun:1;
        unsigned int        _doingUnhide:1;
        unsigned int        _delegateReturnsValidRequestor:1;
        unsigned int        _deactPending:1;
        unsigned int        _invalidState:1;
        unsigned int        _invalidEvent:1;
        unsigned int        _postedWindowsNeedUpdateNote:1;
        unsigned int        _wantsToActivate:1;
        unsigned int        _doingHide:1;
        unsigned int        _dontSendShouldTerminate:1;
        unsigned int        _ignoresFullScreen:1;
        unsigned int        _finishedLaunching:1;
        unsigned int        _hasEventDelegate:1;
        unsigned int        _appTerminating:1;
        unsigned int        _didNSOpenOrPrint:1;
        unsigned int        _inDealloc:1;
        unsigned int        _pendingDidFinish:1;
        unsigned int        _hasKeyFocus:1;
        unsigned int        _panelsNonactivating:1;
        unsigned int        _hiddenOnLaunch:1;
        unsigned int        _openStatus:2;
        unsigned int        _batchOrdering:1;
        unsigned int        _waitingForTerminationReply:1;
        unsigned int        _unused:1;
        unsigned int        _enumeratingMemoryPressureHandlers:1;
        unsigned int        _didTryRestoringPersistentState:1;
        unsigned int        _windowDragging:1;
        unsigned int        _mightBeSwitching:1;
    }                   _appFlags;
    id                  _mainMenu;
}
@end
#endif

@interface NSApplication (Internal)
- (void)_setKeyWindow:(id)window;
- (void)_setMainWindow:(id)window;
@end

@interface X11Application (Private)
- (void) sendX11NSEvent:(NSEvent *)e;
@end

@interface X11Application ()
@property (nonatomic, readwrite, assign) OSX_BOOL x_active;
@end

@implementation X11Application

typedef struct message_struct message;
struct message_struct {
    mach_msg_header_t hdr;
    SEL selector;
    NSObject *arg;
};

/* Quartz mode initialization routine. This is often dynamically loaded
   but is statically linked into this X server. */
Bool
QuartzModeBundleInit(void);

- (void) dealloc
{
    self.controller = nil;
    [super dealloc];
}

- (void) orderFrontStandardAboutPanel: (id) sender
{
    NSMutableDictionary *dict;
    NSDictionary *infoDict;
    NSString *tem;

    dict = [NSMutableDictionary dictionaryWithCapacity:3];
    infoDict = [[NSBundle mainBundle] infoDictionary];

    [dict setObject: NSLocalizedString(@"The X Window System", @"About panel")
             forKey:@"ApplicationName"];

    tem = [infoDict objectForKey:@"CFBundleShortVersionString"];

    [dict setObject:[NSString stringWithFormat:@"XQuartz %@", tem]
             forKey:@"ApplicationVersion"];

    [dict setObject:[NSString stringWithFormat:@"xorg-server %s",
                     XSERVER_VERSION]
     forKey:@"Version"];

    [self orderFrontStandardAboutPanelWithOptions: dict];
}

- (void) activateX:(OSX_BOOL)state
{
    OSX_BOOL const x_active = self.x_active;

    if (x_active == state)
        return;

    DEBUG_LOG("state=%d, x_active=%d, \n", state, x_active);
    if (state) {
        if (bgMouseLocationUpdated) {
            DarwinSendPointerEvents(darwinPointer, MotionNotify, 0,
                                    bgMouseLocation.x, bgMouseLocation.y,
                                    0.0, 0.0);
            bgMouseLocationUpdated = FALSE;
        }
        DarwinSendDDXEvent(kXquartzActivate, 0);
    }
    else {

        if (darwin_all_modifier_flags)
            DarwinUpdateModKeys(0);

        DarwinInputReleaseButtonsAndKeys(darwinKeyboard);
        DarwinInputReleaseButtonsAndKeys(darwinPointer);
        DarwinInputReleaseButtonsAndKeys(darwinTabletCursor);
        DarwinInputReleaseButtonsAndKeys(darwinTabletStylus);
        DarwinInputReleaseButtonsAndKeys(darwinTabletEraser);

        DarwinSendDDXEvent(kXquartzDeactivate, 0);
    }

    self.x_active = state;
}

- (void) became_key:(NSWindow *)win
{
    [self activateX:NO];
}

- (void) sendEvent:(NSEvent *)e
{
    /* Don't try sending to X if we haven't initialized.  This can happen if AppKit takes over
     * (eg: uncaught exception) early in launch.
     */
    if (!eventTranslationQueue) {
        [super sendEvent:e];
        return;
    }

    OSX_BOOL for_appkit, for_x;
    OSX_BOOL const x_active = self.x_active;

    /* By default pass down the responder chain and to X. */
    for_appkit = YES;
    for_x = YES;

    switch ([e type]) {
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
    case NSScrollWheel:

        if ([e window] != nil) {
            /* Pointer event has an (AppKit) window. Probably something for the kit. */
            for_x = NO;
            if (x_active) [self activateX:NO];
        }
        else if ([self modalWindow] == nil) {
            /* Must be an X window. Tell appkit windows to resign main/key */
            for_appkit = NO;

            if (!x_active && quartzProcs->IsX11Window([e windowNumber])) {
                if ([self respondsToSelector:@selector(_setKeyWindow:)] && [self respondsToSelector:@selector(_setMainWindow:)]) {
                    NSWindow *keyWindow = [self keyWindow];
                    if (keyWindow) {
                        [self _setKeyWindow:nil];
                        [keyWindow resignKeyWindow];
                    }

                    NSWindow *mainWindow = [self mainWindow];
                    if (mainWindow) {
                        [self _setMainWindow:nil];
                        [mainWindow resignMainWindow];
                   }
                 } else {
                    /* This has a side effect of causing background apps to steal focus from XQuartz.
                     * Unfortunately, there is no public and stable API to do what we want, but this
                     * is a decent fallback in the off chance that the above selectors get dropped
                     * in the future.
                     */
                    [self deactivate];
                }

                [self activateX:YES];
            }
        }

        /* We want to force sending to appkit if we're over the menu bar */
        if (!for_appkit) {
            NSPoint NSlocation = [e locationInWindow];
            NSWindow *window = [e window];
            NSRect NSframe, NSvisibleFrame;
            CGRect CGframe, CGvisibleFrame;
            CGPoint CGlocation;

            if (window != nil) {
                NSRect frame = [window frame];
                NSlocation.x += frame.origin.x;
                NSlocation.y += frame.origin.y;
            }

            NSframe = [[NSScreen mainScreen] frame];
            NSvisibleFrame = [[NSScreen mainScreen] visibleFrame];

            CGframe = CGRectMake(NSframe.origin.x, NSframe.origin.y,
                                 NSframe.size.width, NSframe.size.height);
            CGvisibleFrame = CGRectMake(NSvisibleFrame.origin.x,
                                        NSvisibleFrame.origin.y,
                                        NSvisibleFrame.size.width,
                                        NSvisibleFrame.size.height);
            CGlocation = CGPointMake(NSlocation.x, NSlocation.y);

            if (CGRectContainsPoint(CGframe, CGlocation) &&
                !CGRectContainsPoint(CGvisibleFrame, CGlocation))
                for_appkit = YES;
        }

        break;

    case NSKeyDown:
    case NSKeyUp:

        if (_x_active) {
            static BOOL do_swallow = NO;
            static int swallow_keycode;

            if ([e type] == NSKeyDown) {
                /* Before that though, see if there are any global
                 * shortcuts bound to it. */

                if (darwinAppKitModMask &[e modifierFlags]) {
                    /* Override to force sending to Appkit */
                    swallow_keycode = [e keyCode];
                    do_swallow = YES;
                    for_x = NO;
                } else if (XQuartzEnableKeyEquivalents &&
                         xp_is_symbolic_hotkey_event([e eventRef])) {
                    swallow_keycode = [e keyCode];
                    do_swallow = YES;
                    for_x = NO;
                }
                else if (XQuartzEnableKeyEquivalents &&
                         [[self mainMenu] performKeyEquivalent:e]) {
                    swallow_keycode = [e keyCode];
                    do_swallow = YES;
                    for_appkit = NO;
                    for_x = NO;
                }
                else if (!XQuartzIsRootless
                         && ([e modifierFlags] & ALL_KEY_MASKS) ==
                         (NSCommandKeyMask | NSAlternateKeyMask)
                         && ([e keyCode] == 0 /*a*/ || [e keyCode] ==
                             53 /*Esc*/)) {
                    /* We have this here to force processing fullscreen
                     * toggle even if XQuartzEnableKeyEquivalents is disabled */
                    swallow_keycode = [e keyCode];
                    do_swallow = YES;
                    for_x = NO;
                    for_appkit = NO;
                    DarwinSendDDXEvent(kXquartzToggleFullscreen, 0);
                }
                else {
                    /* No kit window is focused, so send it to X. */
                    for_appkit = NO;

                    /* Reset our swallow state if we're seeing the same keyCode again.
                     * This can happen if we become !_x_active when the keyCode we
                     * intended to swallow is delivered.  See:
                     * https://bugs.freedesktop.org/show_bug.cgi?id=92648
                     */
                    if ([e keyCode] == swallow_keycode) {
                        do_swallow = NO;
                    }
                }
            }
            else {       /* KeyUp */
                /* If we saw a key equivalent on the down, don't pass
                 * the up through to X. */
                if (do_swallow && [e keyCode] == swallow_keycode) {
                    do_swallow = NO;
                    for_x = NO;
                }
            }
        }
        else {       /* !_x_active */
            for_x = NO;
        }
        break;

    case NSFlagsChanged:
        /* Don't tell X11 about modifiers changing while it's not active */
        if (!_x_active)
            for_x = NO;
        break;

    case NSAppKitDefined:
        switch ([e subtype]) {
            static BOOL x_was_active = NO;

        case NSApplicationActivatedEventType:
            for_x = NO;
            if ([e window] == nil && x_was_active) {
                BOOL order_all_windows = YES;
                for_appkit = NO;

#if APPKIT_APPFLAGS_HACK
                /* FIXME: This is a hack to avoid passing the event to AppKit which
                 *        would result in it raising one of its windows.
                 */
                _appFlags._active = YES;
#endif

                [self set_front_process:nil];

                /* Get the Spaces preference for SwitchOnActivate */
                BOOL const workspaces = [NSUserDefaults.dockDefaults boolForKey:@"workspaces"];
                if (workspaces) {
                    order_all_windows = [NSUserDefaults.globalDefaults boolForKey:@"AppleSpacesSwitchOnActivate"];
                }

                /* TODO: In the workspaces && !AppleSpacesSwitchOnActivate case, the windows are ordered
                 *       correctly, but we need to activate the top window on this space if there is
                 *       none active.
                 *
                 *       If there are no active windows, and there are minimized windows, we should
                 *       be restoring one of them.
                 */
                if ([e data2] & 0x10) {         // 0x10 (bfCPSOrderAllWindowsForward) is set when we use cmd-tab or the dock icon
                    DarwinSendDDXEvent(kXquartzBringAllToFront, 1, order_all_windows);
                }
            }
            break;

        case 18:         /* ApplicationDidReactivate */
            if (XQuartzFullscreenVisible) for_appkit = NO;
            break;

        case NSApplicationDeactivatedEventType:
            for_x = NO;

            x_was_active = _x_active;
            if (_x_active)
                [self activateX:NO];
            break;
        }
        break;

    default:
        break;          /* for gcc */
    }

    if (for_appkit) {
        [super sendEvent:e];
    }

    if (for_x) {
        dispatch_async(eventTranslationQueue, ^{
            [self sendX11NSEvent:e];
        });
    }
}

- (void) set_apps_menu:(NSArray *)list
{
    [self.controller set_apps_menu:list];
}

- (void) set_front_process:unused
{
    [NSApp activateIgnoringOtherApps:YES];

    if ([self modalWindow] == nil)
        [self activateX:YES];
}

- (void) show_hide_menubar:(NSNumber *)state
{
    /* Also shows/hides the dock */
    if ([state boolValue])
        SetSystemUIMode(kUIModeNormal, 0);
    else
        SetSystemUIMode(kUIModeAllHidden,
                        XQuartzFullscreenMenu ? kUIOptionAutoShowMenuBar : 0);                   // kUIModeAllSuppressed or kUIOptionAutoShowMenuBar can be used to allow "mouse-activation"
}

- (void) launch_client:(NSString *)cmd
{
    (void)[self.controller application:self openFile:cmd];
}


- (void) read_defaults
{
    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;

    XQuartzRootlessDefault = [defaults boolForKey:XQuartzPrefKeyRootless];
    XQuartzFullscreenMenu = [defaults boolForKey:XQuartzPrefKeyFullscreenMenu];
    XQuartzFullscreenDisableHotkeys = ![defaults boolForKey:XQuartzPrefKeyFullscreenHotkeys];

    darwinFakeButtons = [defaults boolForKey:XQuartzPrefKeyFakeButtons];
    XQuartzOptionSendsAlt = [defaults boolForKey:XQuartzPrefKeyOptionSendsAlt];

    if (darwinFakeButtons) {
        NSString * const fake2 = [defaults stringForKey:XQuartzPrefKeyFakeButton2];
        if (fake2) {
            darwinFakeMouse2Mask = DarwinParseModifierList(fake2.UTF8String, TRUE);
        }

        NSString * const fake3 = [defaults stringForKey:XQuartzPrefKeyFakeButton3];
        if (fake3) {
            darwinFakeMouse3Mask = DarwinParseModifierList(fake3.UTF8String, TRUE);
        }
    }

    NSString * const appKitModifiers = [defaults stringForKey:XQuartzPrefKeyAppKitModifiers];
    if (appKitModifiers) {
        darwinAppKitModMask = DarwinParseModifierList(appKitModifiers.UTF8String, TRUE);
    }

    NSString * const windowItemModifiers = [defaults stringForKey:XQuartzPrefKeyWindowItemModifiers];
    if (windowItemModifiers) {
        windowItemModMask = DarwinParseModifierList(windowItemModifiers.UTF8String, FALSE);
    }

    XQuartzEnableKeyEquivalents = [defaults boolForKey:XQuartzPrefKeyKeyEquivs];

    darwinSyncKeymap = [defaults boolForKey:XQuartzPrefKeySyncKeymap];

    darwinDesiredDepth = [defaults integerForKey:XQuartzPrefKeyDepth];

    noTestExtensions = ![defaults boolForKey:XQuartzPrefKeyTESTExtension];
    noRenderExtension = ![defaults boolForKey:XQuartzPrefKeyRENDERExtension];

    XQuartzScrollInDeviceDirection = [defaults boolForKey:XQuartzPrefKeyScrollInDeviceDirection];
}

/* This will end up at the end of the responder chain. */
- (void) copy:sender
{
    DarwinSendDDXEvent(kXquartzPasteboardNotify, 1,
                       AppleWMCopyToPasteboard);
}

@end

void
X11ApplicationSetWindowMenu(int nitems, const char **items,
                            const char *shortcuts)
{
    @autoreleasepool {
        NSMutableArray <NSArray <NSString *> *> * const allMenuItems = [NSMutableArray array];

        for (int i = 0; i < nitems; i++) {
            NSMutableArray <NSString *> * const menuItem = [NSMutableArray array];
            [menuItem addObject:@(items[i])];

            if (shortcuts[i] == 0) {
                [menuItem addObject:@""];
            } else {
                [menuItem addObject:[NSString stringWithFormat:@"%d", shortcuts[i]]];
            }

            [allMenuItems addObject:menuItem];
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            [X11App.controller set_window_menu:allMenuItems];
        });
    }
}

void
X11ApplicationSetWindowMenuCheck(int idx)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [X11App.controller set_window_menu_check:@(idx)];
    });
}

void
X11ApplicationSetFrontProcess(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [X11App set_front_process:nil];
    });
}

void
X11ApplicationSetCanQuit(int state)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        X11App.controller.can_quit = !!state;
    });
}

void
X11ApplicationServerReady(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [X11App.controller server_ready];
    });
}

void
X11ApplicationShowHideMenubar(int state)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [X11App show_hide_menubar:@(state)];
    });
}

void
X11ApplicationLaunchClient(const char *cmd)
{
    @autoreleasepool {
        NSString *string = @(cmd);
        dispatch_async(dispatch_get_main_queue(), ^{
            [X11App launch_client:string];
        });
    }
}

/* This is a special function in that it is run from the *SERVER* thread and
 * not the AppKit thread.  We want to block entering a screen-capturing RandR
 * mode until we notify the user about how to get out if the X11 client crashes.
 */
Bool
X11ApplicationCanEnterRandR(void)
{
    NSString *title, *msg;
    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;

    if ([defaults boolForKey:XQuartzPrefKeyNoRANDRAlert] ||
        XQuartzShieldingWindowLevel != 0)
        return TRUE;

    title = NSLocalizedString(@"Enter RandR mode?",
                              @"Dialog title when switching to RandR");
    msg = NSLocalizedString(
        @"An application has requested X11 to change the resolution of your display.  X11 will restore the display to its previous state when the requesting application requests to return to the previous state.  Alternatively, you can use the ⌥⌘A key sequence to force X11 to return to the previous state.",
        @"Dialog when switching to RandR");

    if (!XQuartzIsRootless)
        QuartzShowFullscreen(FALSE);

    NSInteger __block alert_result;
    dispatch_sync(dispatch_get_main_queue(), ^{
        alert_result = NSRunAlertPanel(title, @"%@",
                                       NSLocalizedString(@"Allow", @""),
                                       NSLocalizedString(@"Cancel", @""),
                                       NSLocalizedString(@"Always Allow", @""), msg);
    });

    switch (alert_result) {
    case NSAlertOtherReturn:
        [defaults setBool:YES forKey:XQuartzPrefKeyNoRANDRAlert];

    case NSAlertDefaultReturn:
        return YES;

    default:
        return NO;
    }
}

static void
check_xinitrc(void)
{
    char *tem, buf[1024];
    NSString *msg;
    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;

    if ([defaults boolForKey:XQuartzPrefKeyDoneXinitCheck])
        return;

    tem = getenv("HOME");
    if (tem == NULL) goto done;

    snprintf(buf, sizeof(buf), "%s/.xinitrc", tem);
    if (access(buf, F_OK) != 0)
        goto done;

    msg =
        NSLocalizedString(
            @"You have an existing ~/.xinitrc file.\n\n\
                             Windows displayed by X11 applications may not have titlebars, or may look \
                             different to windows displayed by native applications.\n\n\
                             Would you like to move aside the existing file and use the standard X11 \
                             environment the next time you start X11?"                                                                                                                                                                                                                                                                                                                                                                  ,
            @"Startup xinitrc dialog");

    if (NSAlertDefaultReturn ==
        NSRunAlertPanel(nil, @"%@", NSLocalizedString(@"Yes", @""),
                        NSLocalizedString(@"No", @""), nil, msg)) {
        char buf2[1024];
        int i = -1;

        snprintf(buf2, sizeof(buf2), "%s.old", buf);

        for (i = 1; access(buf2, F_OK) == 0; i++)
            snprintf(buf2, sizeof(buf2), "%s.old.%d", buf, i);

        rename(buf, buf2);
    }

done:
    [defaults setBool:YES forKey:XQuartzPrefKeyDoneXinitCheck];
}

static inline pthread_t
create_thread(void *(*func)(void *), void *arg)
{
    pthread_attr_t attr;
    pthread_t tid;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, func, arg);
    pthread_attr_destroy(&attr);

    return tid;
}

static void *
xpbproxy_x_thread(void *args)
{
    xpbproxy_run();

    ErrorF("xpbproxy thread is terminating unexpectedly.\n");
    return NULL;
}

void
X11ApplicationMain(int argc, char **argv, char **envp)
{
#ifdef DEBUG
    while (access("/tmp/x11-block", F_OK) == 0) sleep(1);
#endif

    @autoreleasepool {
        X11App = (X11Application *)[X11Application sharedApplication];
        [X11App read_defaults];

        [NSBundle loadNibNamed:@"main" owner:NSApp];
        [NSNotificationCenter.defaultCenter addObserver:NSApp
                                               selector:@selector (became_key:)
                                                   name:NSWindowDidBecomeKeyNotification
                                                 object:nil];

        /*
         * The xpr Quartz mode is statically linked into this server.
         * Initialize all the Quartz functions.
         */
        QuartzModeBundleInit();

        /* Calculate the height of the menubar so we can avoid it. */
        aquaMenuBarHeight = NSApp.mainMenu.menuBarHeight;
        if (!aquaMenuBarHeight) {
            NSScreen* primaryScreen = NSScreen.screens[0];
            aquaMenuBarHeight = NSHeight(primaryScreen.frame) - NSMaxY(primaryScreen.visibleFrame);
        }

        eventTranslationQueue = dispatch_queue_create(BUNDLE_ID_PREFIX ".X11.NSEventsToX11EventsQueue", NULL);
        assert(eventTranslationQueue != NULL);

        /* Set the key layout seed before we start the server */
        last_key_layout = TISCopyCurrentKeyboardLayoutInputSource();

        if (!last_key_layout) {
            ErrorF("X11ApplicationMain: Unable to determine TISCopyCurrentKeyboardLayoutInputSource() at startup.\n");
        }

        if (!QuartsResyncKeymap(FALSE)) {
            ErrorF("X11ApplicationMain: Could not build a valid keymap.\n");
        }

        /* Tell the server thread that it can proceed */
        QuartzInitServer(argc, argv, envp);

        /* This must be done after QuartzInitServer because it can result in
         * an mieqEnqueue() - <rdar://problem/6300249>
         */
        check_xinitrc();

        create_thread(xpbproxy_x_thread, NULL);

#if XQUARTZ_SPARKLE
        [[X11App controller] setup_sparkle];
        [[SUUpdater sharedUpdater] resetUpdateCycle];
        //    [[SUUpdater sharedUpdater] checkForUpdates:X11App];
#endif
    }

    [NSApp run];
    /* not reached */
}

@implementation X11Application (Private)

#ifdef NX_DEVICELCMDKEYMASK
/* This is to workaround a bug in the VNC server where we sometimes see the L
 * modifier and sometimes see no "side"
 */
static inline int
ensure_flag(int flags, int device_independent, int device_dependents,
            int device_dependent_default)
{
    if ((flags & device_independent) &&
        !(flags & device_dependents))
        flags |= device_dependent_default;
    return flags;
}
#endif

#ifdef DEBUG_UNTRUSTED_POINTER_DELTA
static const char *
untrusted_str(NSEvent *e)
{
    switch ([e type]) {
    case NSScrollWheel:
        return "NSScrollWheel";

    case NSTabletPoint:
        return "NSTabletPoint";

    case NSOtherMouseDown:
        return "NSOtherMouseDown";

    case NSOtherMouseUp:
        return "NSOtherMouseUp";

    case NSLeftMouseDown:
        return "NSLeftMouseDown";

    case NSLeftMouseUp:
        return "NSLeftMouseUp";

    default:
        switch ([e subtype]) {
        case NSTabletPointEventSubtype:
            return "NSTabletPointEventSubtype";

        case NSTabletProximityEventSubtype:
            return "NSTabletProximityEventSubtype";

        default:
            return "Other";
        }
    }
}
#endif

extern void
wait_for_mieq_init(void);

- (void) sendX11NSEvent:(NSEvent *)e
{
    NSPoint location = NSZeroPoint;
    int ev_button, ev_type;
    static float pressure = 0.0;       // static so ProximityOut will have the value from the previous tablet event
    static NSPoint tilt;               // static so ProximityOut will have the value from the previous tablet event
    static DeviceIntPtr darwinTabletCurrent = NULL;
    static BOOL needsProximityIn = NO; // Do we do need to handle a pending ProximityIn once we have pressure/tilt?
    DeviceIntPtr pDev;
    int modifierFlags;
    BOOL isMouseOrTabletEvent, isTabletEvent;

    if (!darwinTabletCurrent) {
        /* Ensure that the event system is initialized */
        wait_for_mieq_init();
        assert(darwinTabletStylus);

        tilt = NSZeroPoint;
        darwinTabletCurrent = darwinTabletStylus;
    }

    isMouseOrTabletEvent = [e type] == NSLeftMouseDown ||
                           [e type] == NSOtherMouseDown ||
                           [e type] == NSRightMouseDown ||
                           [e type] == NSLeftMouseUp ||
                           [e type] == NSOtherMouseUp ||
                           [e type] == NSRightMouseUp ||
                           [e type] == NSLeftMouseDragged ||
                           [e type] == NSOtherMouseDragged ||
                           [e type] == NSRightMouseDragged ||
                           [e type] == NSMouseMoved ||
                           [e type] == NSTabletPoint || 
                           [e type] == NSScrollWheel;

    isTabletEvent = ([e type] == NSTabletPoint) ||
                    (isMouseOrTabletEvent &&
                     ([e subtype] == NSTabletPointEventSubtype ||
                      [e subtype] == NSTabletProximityEventSubtype));

    if (isMouseOrTabletEvent) {
        static NSPoint lastpt;
        NSWindow *window = [e window];
        NSRect screen = [[[NSScreen screens] objectAtIndex:0] frame];
        BOOL hasUntrustedPointerDelta;

        // NSEvents for tablets are not consistent wrt deltaXY between events, so we cannot rely on that
        // Thus tablets will be subject to the warp-pointer bug worked around by the delta, but tablets
        // are not normally used in cases where that bug would present itself, so this is a fair tradeoff
        // <rdar://problem/7111003> deltaX and deltaY are incorrect for NSMouseMoved, NSTabletPointEventSubtype
        // http://xquartz.macosforge.org/trac/ticket/288
        hasUntrustedPointerDelta = isTabletEvent;

        // The deltaXY for middle click events also appear erroneous after fast user switching
        // <rdar://problem/7979468> deltaX and deltaY are incorrect for NSOtherMouseDown and NSOtherMouseUp after FUS
        // http://xquartz.macosforge.org/trac/ticket/389
        hasUntrustedPointerDelta |= [e type] == NSOtherMouseDown ||
                                    [e type] == NSOtherMouseUp;

        // The deltaXY for scroll events correspond to the scroll delta, not the pointer delta
        // <rdar://problem/7989690> deltaXY for wheel events are being sent as mouse movement
        hasUntrustedPointerDelta |= [e type] == NSScrollWheel;

#ifdef DEBUG_UNTRUSTED_POINTER_DELTA
        hasUntrustedPointerDelta |= [e type] == NSLeftMouseDown ||
                                    [e type] == NSLeftMouseUp;
#endif

        if (window != nil) {
            NSRect frame = [window frame];
            location = [e locationInWindow];
            location.x += frame.origin.x;
            location.y += frame.origin.y;
            lastpt = location;
        }
        else if (hasUntrustedPointerDelta) {
#ifdef DEBUG_UNTRUSTED_POINTER_DELTA
            ErrorF("--- Begin Event Debug ---\n");
            ErrorF("Event type: %s\n", untrusted_str(e));
            ErrorF("old lastpt: (%0.2f, %0.2f)\n", lastpt.x, lastpt.y);
            ErrorF("     delta: (%0.2f, %0.2f)\n", [e deltaX], -[e deltaY]);
            ErrorF("  location: (%0.2f, %0.2f)\n", lastpt.x + [e deltaX],
                   lastpt.y - [e deltaY]);
            ErrorF("workaround: (%0.2f, %0.2f)\n", [e locationInWindow].x,
                   [e locationInWindow].y);
            ErrorF("--- End Event Debug ---\n");

            location.x = lastpt.x + [e deltaX];
            location.y = lastpt.y - [e deltaY];
            lastpt = [e locationInWindow];
#else
            location = [e locationInWindow];
            lastpt = location;
#endif
        }
        else {
            location.x = lastpt.x + [e deltaX];
            location.y = lastpt.y - [e deltaY];
            lastpt = [e locationInWindow];
        }

        /* Convert coordinate system */
        location.y = (screen.origin.y + screen.size.height) - location.y;
    }

    modifierFlags = [e modifierFlags];

#ifdef NX_DEVICELCMDKEYMASK
    /* This is to workaround a bug in the VNC server where we sometimes see the L
     * modifier and sometimes see no "side"
     */
    modifierFlags = ensure_flag(modifierFlags, NX_CONTROLMASK,
                                NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK,
                                NX_DEVICELCTLKEYMASK);
    modifierFlags = ensure_flag(modifierFlags, NX_SHIFTMASK,
                                NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK, 
                                NX_DEVICELSHIFTKEYMASK);
    modifierFlags = ensure_flag(modifierFlags, NX_COMMANDMASK,
                                NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK,
                                NX_DEVICELCMDKEYMASK);
    modifierFlags = ensure_flag(modifierFlags, NX_ALTERNATEMASK,
                                NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK,
                                NX_DEVICELALTKEYMASK);
#endif

    modifierFlags &= darwin_all_modifier_mask;

    /* We don't receive modifier key events while out of focus, and 3button
     * emulation mucks this up, so we need to check our modifier flag state
     * on every event... ugg
     */

    if (darwin_all_modifier_flags != modifierFlags)
        DarwinUpdateModKeys(modifierFlags);

    switch ([e type]) {
    case NSLeftMouseDown:
        ev_button = 1;
        ev_type = ButtonPress;
        goto handle_mouse;

    case NSOtherMouseDown:
        // Get the AppKit button number, and convert it from 0-based to 1-based
        ev_button = [e buttonNumber] + 1;

        /* Translate middle mouse button (3 in AppKit) to button 2 in X11,
         * and translate additional mouse buttons (4 and higher in AppKit)
         * to buttons 8 and higher in X11, to match default behavior of X11
         * on other platforms
         */
        ev_button = (ev_button == 3) ? 2 : (ev_button + 4);

        ev_type = ButtonPress;
        goto handle_mouse;

    case NSRightMouseDown:
        ev_button = 3;
        ev_type = ButtonPress;
        goto handle_mouse;

    case NSLeftMouseUp:
        ev_button = 1;
        ev_type = ButtonRelease;
        goto handle_mouse;

    case NSOtherMouseUp:
        // See above comments for NSOtherMouseDown
        ev_button = [e buttonNumber] + 1;
        ev_button = (ev_button == 3) ? 2 : (ev_button + 4);
        ev_type = ButtonRelease;
        goto handle_mouse;

    case NSRightMouseUp:
        ev_button = 3;
        ev_type = ButtonRelease;
        goto handle_mouse;

    case NSLeftMouseDragged:
        ev_button = 1;
        ev_type = MotionNotify;
        goto handle_mouse;

    case NSOtherMouseDragged:
        // See above comments for NSOtherMouseDown
        ev_button = [e buttonNumber] + 1;
        ev_button = (ev_button == 3) ? 2 : (ev_button + 4);
        ev_type = MotionNotify;
        goto handle_mouse;

    case NSRightMouseDragged:
        ev_button = 3;
        ev_type = MotionNotify;
        goto handle_mouse;

    case NSMouseMoved:
        ev_button = 0;
        ev_type = MotionNotify;
        goto handle_mouse;

    case NSTabletPoint:
        ev_button = 0;
        ev_type = MotionNotify;
        goto handle_mouse;

handle_mouse:
        pDev = darwinPointer;

        /* NSTabletPoint can have no subtype */
        if ([e type] != NSTabletPoint &&
            [e subtype] == NSTabletProximityEventSubtype) {
            switch ([e pointingDeviceType]) {
            case NSEraserPointingDevice:
                darwinTabletCurrent = darwinTabletEraser;
                break;

            case NSPenPointingDevice:
                darwinTabletCurrent = darwinTabletStylus;
                break;

            case NSCursorPointingDevice:
            case NSUnknownPointingDevice:
            default:
                darwinTabletCurrent = darwinTabletCursor;
                break;
            }

            if ([e isEnteringProximity])
                needsProximityIn = YES;
            else
                DarwinSendTabletEvents(darwinTabletCurrent, ProximityOut, 0,
                                       location.x, location.y, pressure,
                                       tilt.x, tilt.y);
            return;
        }

        if ([e type] == NSTabletPoint ||
            [e subtype] == NSTabletPointEventSubtype) {
            pressure = [e pressure];
            tilt = [e tilt];

            pDev = darwinTabletCurrent;

            if (needsProximityIn) {
                DarwinSendTabletEvents(darwinTabletCurrent, ProximityIn, 0,
                                       location.x, location.y, pressure,
                                       tilt.x, tilt.y);

                needsProximityIn = NO;
            }
        }

        if (!XQuartzServerVisible && noTestExtensions) {
            xp_window_id wid = 0;
            xp_error err;

            /* Sigh. Need to check that we're really over one of
             * our windows. (We need to receive pointer events while
             * not in the foreground, but we don't want to receive them
             * when another window is over us or we might show a tooltip)
             */

            err = xp_find_window(location.x, location.y, 0, &wid);

            if (err != XP_Success || (err == XP_Success && wid == 0))
            {
                bgMouseLocation = location;
                bgMouseLocationUpdated = TRUE;
                return;
            }
        }

        if (bgMouseLocationUpdated) {
            if (!(ev_type == MotionNotify && ev_button == 0)) {
                DarwinSendPointerEvents(darwinPointer, MotionNotify, 0,
                                        location.x, location.y,
                                        0.0, 0.0);
            }
            bgMouseLocationUpdated = FALSE;
        }

        if (pDev == darwinPointer) {
            DarwinSendPointerEvents(pDev, ev_type, ev_button,
                                    location.x, location.y,
                                    [e deltaX], [e deltaY]);
        } else {
            DarwinSendTabletEvents(pDev, ev_type, ev_button,
                                   location.x, location.y, pressure,
                                   tilt.x, tilt.y);
        }

        break;

    case NSTabletProximity:
        switch ([e pointingDeviceType]) {
        case NSEraserPointingDevice:
            darwinTabletCurrent = darwinTabletEraser;
            break;

        case NSPenPointingDevice:
            darwinTabletCurrent = darwinTabletStylus;
            break;

        case NSCursorPointingDevice:
        case NSUnknownPointingDevice:
        default:
            darwinTabletCurrent = darwinTabletCursor;
            break;
        }

        if ([e isEnteringProximity])
            needsProximityIn = YES;
        else
            DarwinSendTabletEvents(darwinTabletCurrent, ProximityOut, 0,
                                   location.x, location.y, pressure,
                                   tilt.x, tilt.y);
        break;

    case NSScrollWheel:
    {
        CGFloat deltaX = [e deltaX];
        CGFloat deltaY = [e deltaY];
        CGEventRef cge = [e CGEvent];
        BOOL isContinuous =
            CGEventGetIntegerValueField(cge, kCGScrollWheelEventIsContinuous);

#if 0
        /* Scale the scroll value by line height */
        CGEventSourceRef source = CGEventCreateSourceFromEvent(cge);
        if (source) {
            double lineHeight = CGEventSourceGetPixelsPerLine(source);
            CFRelease(source);
            
            /* There's no real reason for the 1/5 ratio here other than that
             * it feels like a good ratio after some testing.
             */
            
            deltaX *= lineHeight / 5.0;
            deltaY *= lineHeight / 5.0;
        }
#endif
        
        if (XQuartzScrollInDeviceDirection &&
            [e isDirectionInvertedFromDevice]) {
            deltaX *= -1;
            deltaY *= -1;
        }
        /* This hack is in place to better deal with "clicky" scroll wheels:
         * http://xquartz.macosforge.org/trac/ticket/562
         */
        if (!isContinuous) {
            static NSTimeInterval lastScrollTime = 0.0;

            /* These store how much extra we have already scrolled.
             * ie, this is how much we ignore on the next event.
             */
            static double deficit_x = 0.0;
            static double deficit_y = 0.0;

            /* If we have past a second since the last scroll, wipe the slate
             * clean
             */
            if ([e timestamp] - lastScrollTime > 1.0) {
                deficit_x = deficit_y = 0.0;
            }
            lastScrollTime = [e timestamp];

            if (deltaX != 0.0) {
                /* If we changed directions, wipe the slate clean */
                if ((deficit_x < 0.0 && deltaX > 0.0) ||
                    (deficit_x > 0.0 && deltaX < 0.0)) {
                    deficit_x = 0.0;
                }

                /* Eat up the deficit, but ensure that something is
                 * always sent 
                 */
                if (fabs(deltaX) > fabs(deficit_x)) {
                    deltaX -= deficit_x;

                    if (deltaX > 0.0) {
                        deficit_x = ceil(deltaX) - deltaX;
                        deltaX = ceil(deltaX);
                    } else {
                        deficit_x = floor(deltaX) - deltaX;
                        deltaX = floor(deltaX);
                    }
                } else {
                    deficit_x -= deltaX;

                    if (deltaX > 0.0) {
                        deltaX = 1.0;
                    } else {
                        deltaX = -1.0;
                    }

                    deficit_x += deltaX;
                }
            }

            if (deltaY != 0.0) {
                /* If we changed directions, wipe the slate clean */
                if ((deficit_y < 0.0 && deltaY > 0.0) ||
                    (deficit_y > 0.0 && deltaY < 0.0)) {
                    deficit_y = 0.0;
                }

                /* Eat up the deficit, but ensure that something is
                 * always sent 
                 */
                if (fabs(deltaY) > fabs(deficit_y)) {
                    deltaY -= deficit_y;

                    if (deltaY > 0.0) {
                        deficit_y = ceil(deltaY) - deltaY;
                        deltaY = ceil(deltaY);
                    } else {
                        deficit_y = floor(deltaY) - deltaY;
                        deltaY = floor(deltaY);
                    }
                } else {
                    deficit_y -= deltaY;

                    if (deltaY > 0.0) {
                        deltaY = 1.0;
                    } else {
                        deltaY = -1.0;
                    }

                    deficit_y += deltaY;
                }
            }
        }

        DarwinSendScrollEvents(deltaX, deltaY);
        break;
    }

    case NSKeyDown:
    case NSKeyUp:
    {
        /* XKB clobbers our keymap at startup, so we need to force it on the first keypress.
         * TODO: Make this less of a kludge.
         */
        static int force_resync_keymap = YES;
        if (force_resync_keymap) {
            DarwinSendDDXEvent(kXquartzReloadKeymap, 0);
            force_resync_keymap = NO;
        }
    }

        if (darwinSyncKeymap) {
            __block TISInputSourceRef key_layout;
            dispatch_block_t copyCurrentKeyboardLayoutInputSource = ^{
                key_layout = TISCopyCurrentKeyboardLayoutInputSource();
            };
            /* This is an ugly ant-pattern, but it is more expedient to address the problem right now. */
            if (pthread_main_np()) {
                copyCurrentKeyboardLayoutInputSource();
            } else {
                dispatch_sync(dispatch_get_main_queue(), copyCurrentKeyboardLayoutInputSource);
            }

            TISInputSourceRef clear;
            if (CFEqual(key_layout, last_key_layout)) {
                CFRelease(key_layout);
            }
            else {
                /* Swap/free thread-safely */
                clear = last_key_layout;
                last_key_layout = key_layout;
                CFRelease(clear);

                /* Update keyInfo */
                if (!QuartsResyncKeymap(TRUE)) {
                    ErrorF(
                        "sendX11NSEvent: Could not build a valid keymap.\n");
                }
            }
        }

        ev_type = ([e type] == NSKeyDown) ? KeyPress : KeyRelease;
        DarwinSendKeyboardEvents(ev_type, [e keyCode]);
        break;

    default:
        break;              /* for gcc */
    }
}
@end
