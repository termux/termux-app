//
//  NSUserDefaults+XQuartzDefaults.m
//  XQuartz
//
//  Created by Jeremy Huddleston Sequoia on 2021.02.19.
//  Copyright (c) 2021 Apple Inc. All rights reserved.
//

#import "NSUserDefaults+XQuartzDefaults.h"
#import <dispatch/dispatch.h>

NSString * const XQuartzPrefKeyAppsMenu = @"apps_menu";
NSString * const XQuartzPrefKeyFakeButtons = @"enable_fake_buttons";
NSString * const XQuartzPrefKeyFakeButton2 = @"fake_button2";
NSString * const XQuartzPrefKeyFakeButton3 = @"fake_button3";
NSString * const XQuartzPrefKeyKeyEquivs = @"enable_key_equivalents";
NSString * const XQuartzPrefKeyFullscreenHotkeys = @"fullscreen_hotkeys";
NSString * const XQuartzPrefKeyFullscreenMenu = @"fullscreen_menu";
NSString * const XQuartzPrefKeySyncKeymap = @"sync_keymap";
NSString * const XQuartzPrefKeyDepth = @"depth";
NSString * const XQuartzPrefKeyNoAuth = @"no_auth";
NSString * const XQuartzPrefKeyNoTCP = @"nolisten_tcp";
NSString * const XQuartzPrefKeyDoneXinitCheck = @"done_xinit_check";
NSString * const XQuartzPrefKeyNoQuitAlert = @"no_quit_alert";
NSString * const XQuartzPrefKeyNoRANDRAlert = @"no_randr_alert";
NSString * const XQuartzPrefKeyOptionSendsAlt = @"option_sends_alt";
NSString * const XQuartzPrefKeyAppKitModifiers = @"appkit_modifiers";
NSString * const XQuartzPrefKeyWindowItemModifiers = @"window_item_modifiers";
NSString * const XQuartzPrefKeyRootless = @"rootless";
NSString * const XQuartzPrefKeyRENDERExtension = @"enable_render_extension";
NSString * const XQuartzPrefKeyTESTExtension = @"enable_test_extensions";
NSString * const XQuartzPrefKeyLoginShell = @"login_shell";
NSString * const XQuartzPrefKeyUpdateFeed = @"update_feed";
NSString * const XQuartzPrefKeyClickThrough = @"wm_click_through";
NSString * const XQuartzPrefKeyFocusFollowsMouse = @"wm_ffm";
NSString * const XQuartzPrefKeyFocusOnNewWindow = @"wm_focus_on_new_window";

NSString * const XQuartzPrefKeyScrollInDeviceDirection = @"scroll_in_device_direction";
NSString * const XQuartzPrefKeySyncPasteboard = @"sync_pasteboard";
NSString * const XQuartzPrefKeySyncPasteboardToClipboard = @"sync_pasteboard_to_clipboard";
NSString * const XQuartzPrefKeySyncPasteboardToPrimary = @"sync_pasteboard_to_primary";
NSString * const XQuartzPrefKeySyncClipboardToPasteBoard = @"sync_clipboard_to_pasteboard";
NSString * const XQuartzPrefKeySyncPrimaryOnSelect = @"sync_primary_on_select";

@implementation NSUserDefaults (XQuartzDefaults)

+ (NSUserDefaults *)globalDefaults
{
    static dispatch_once_t once;
    static NSUserDefaults *defaults;

    dispatch_once(&once, ^{
        NSString * const defaultsDomain = @".GlobalPreferences";
        defaults = [[[NSUserDefaults alloc] initWithSuiteName:defaultsDomain] retain];

        NSDictionary<NSString *, id> * const defaultDefaultsDict = @{
            @"AppleSpacesSwitchOnActivate" : @(YES),
        };

        [defaults registerDefaults:defaultDefaultsDict];
    });

    return defaults;
}

+ (NSUserDefaults *)dockDefaults
{
    static dispatch_once_t once;
    static NSUserDefaults *defaults;

    dispatch_once(&once, ^{
        NSString * const defaultsDomain = @"com.apple.dock";
        defaults = [[[NSUserDefaults alloc] initWithSuiteName:defaultsDomain] retain];

        NSDictionary<NSString *, id> * const defaultDefaultsDict = @{
            @"workspaces" : @(NO),
        };

        [defaults registerDefaults:defaultDefaultsDict];
    });

    return defaults;
}

+ (NSUserDefaults *)xquartzDefaults
{
    static dispatch_once_t once;
    static NSUserDefaults *defaults;

    dispatch_once(&once, ^{
        NSString * const defaultsDomain = @(BUNDLE_ID_PREFIX ".X11");
        NSString * const defaultDefaultsDomain = NSBundle.mainBundle.bundleIdentifier;
        if ([defaultsDomain isEqualToString:defaultDefaultsDomain]) {
            defaults = [NSUserDefaults.standardUserDefaults retain];
        } else {
            defaults = [[[NSUserDefaults alloc] initWithSuiteName:defaultsDomain] retain];
        }

        NSString *defaultWindowItemModifiers = @"command";
        NSString * const defaultWindowItemModifiersLocalized = NSLocalizedString(@"window item modifiers", @"window item modifiers");
        if (![defaultWindowItemModifiersLocalized isEqualToString:@"window item modifiers"]) {
            defaultWindowItemModifiers = defaultWindowItemModifiersLocalized;
        }

        NSDictionary<NSString *, id> * const defaultDefaultsDict = @{
            XQuartzPrefKeyFakeButtons : @(NO),
            // XQuartzPrefKeyFakeButton2 nil default
            // XQuartzPrefKeyFakeButton3 nil default
            XQuartzPrefKeyKeyEquivs : @(YES),
            XQuartzPrefKeyFullscreenHotkeys : @(NO),
            XQuartzPrefKeyFullscreenMenu : @(NO),
            XQuartzPrefKeySyncKeymap : @(NO),
            XQuartzPrefKeyDepth : @(-1),
            XQuartzPrefKeyNoAuth : @(NO),
            XQuartzPrefKeyNoTCP : @(NO),
            XQuartzPrefKeyDoneXinitCheck : @(NO),
            XQuartzPrefKeyNoQuitAlert : @(NO),
            XQuartzPrefKeyNoRANDRAlert : @(NO),
            XQuartzPrefKeyOptionSendsAlt : @(NO),
            // XQuartzPrefKeyAppKitModifiers nil default
            XQuartzPrefKeyWindowItemModifiers : defaultWindowItemModifiers,
            XQuartzPrefKeyRootless : @(YES),
            XQuartzPrefKeyRENDERExtension : @(YES),
            XQuartzPrefKeyTESTExtension : @(NO),
            XQuartzPrefKeyLoginShell : @"/bin/sh",
            XQuartzPrefKeyClickThrough : @(NO),
            XQuartzPrefKeyFocusFollowsMouse : @(NO),
            XQuartzPrefKeyFocusOnNewWindow : @(YES),

            XQuartzPrefKeyScrollInDeviceDirection : @(NO),
            XQuartzPrefKeySyncPasteboard : @(YES),
            XQuartzPrefKeySyncPasteboardToClipboard : @(YES),
            XQuartzPrefKeySyncPasteboardToPrimary : @(YES),
            XQuartzPrefKeySyncClipboardToPasteBoard : @(YES),
            XQuartzPrefKeySyncPrimaryOnSelect : @(NO),
        };

        [defaults registerDefaults:defaultDefaultsDict];

        NSString * const systemDefaultsPlistPath = [@(XQUARTZ_DATA_DIR) stringByAppendingPathComponent:@"defaults.plist"];
        NSDictionary <NSString *, id> * const systemDefaultsDict = [NSDictionary dictionaryWithContentsOfFile:systemDefaultsPlistPath];
        [defaults registerDefaults:systemDefaultsDict];
    });

    return defaults;
}

@end
