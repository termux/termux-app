//
//  NSUserDefaults+XQuartzDefaults.h
//  XQuartz
//
//  Created by Jeremy Huddleston Sequoia on 2021.02.19.
//  Copyright (c) 2021 Apple Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString * const XQuartzPrefKeyAppsMenu;
extern NSString * const XQuartzPrefKeyFakeButtons;
extern NSString * const XQuartzPrefKeyFakeButton2;
extern NSString * const XQuartzPrefKeyFakeButton3;
extern NSString * const XQuartzPrefKeyKeyEquivs;
extern NSString * const XQuartzPrefKeyFullscreenHotkeys;
extern NSString * const XQuartzPrefKeyFullscreenMenu;
extern NSString * const XQuartzPrefKeySyncKeymap;
extern NSString * const XQuartzPrefKeyDepth;
extern NSString * const XQuartzPrefKeyNoAuth;
extern NSString * const XQuartzPrefKeyNoTCP;
extern NSString * const XQuartzPrefKeyDoneXinitCheck;
extern NSString * const XQuartzPrefKeyNoQuitAlert;
extern NSString * const XQuartzPrefKeyNoRANDRAlert;
extern NSString * const XQuartzPrefKeyOptionSendsAlt;
extern NSString * const XQuartzPrefKeyAppKitModifiers;
extern NSString * const XQuartzPrefKeyWindowItemModifiers;
extern NSString * const XQuartzPrefKeyRootless;
extern NSString * const XQuartzPrefKeyRENDERExtension;
extern NSString * const XQuartzPrefKeyTESTExtension;
extern NSString * const XQuartzPrefKeyLoginShell;
extern NSString * const XQuartzPrefKeyClickThrough;
extern NSString * const XQuartzPrefKeyFocusFollowsMouse;
extern NSString * const XQuartzPrefKeyFocusOnNewWindow;

extern NSString * const XQuartzPrefKeyScrollInDeviceDirection;
extern NSString * const XQuartzPrefKeySyncPasteboard;
extern NSString * const XQuartzPrefKeySyncPasteboardToClipboard;
extern NSString * const XQuartzPrefKeySyncPasteboardToPrimary;
extern NSString * const XQuartzPrefKeySyncClipboardToPasteBoard;
extern NSString * const XQuartzPrefKeySyncPrimaryOnSelect;

@interface NSUserDefaults (XQuartzDefaults)

+ (NSUserDefaults *)globalDefaults;
+ (NSUserDefaults *)dockDefaults;
+ (NSUserDefaults *)xquartzDefaults;

@end
