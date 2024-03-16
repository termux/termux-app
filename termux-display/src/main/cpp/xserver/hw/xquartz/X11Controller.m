/* X11Controller.m -- connect the IB ui, also the NSApp delegate
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

#import "X11Controller.h"
#import "X11Application.h"
#import "NSUserDefaults+XQuartzDefaults.h"

#include "opaque.h"
#include "darwin.h"
#include "darwinEvents.h"
#include "quartz.h"
#include "quartzKeyboard.h"
#include <X11/extensions/applewmconst.h>
#include "applewmExt.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <asl.h>
#include <stdlib.h>

extern aslclient aslc;
extern char *bundle_id_prefix;

@interface X11Controller ()
#ifdef XQUARTZ_SPARKLE
@property (nonatomic, readwrite, strong) NSMenuItem *check_for_updates_item; // Programatically enabled
#endif

@property (nonatomic, readwrite, strong) NSArray <NSArray <NSString *> *> *apps;
@property (nonatomic, readwrite, strong) NSMutableArray <NSMutableArray <NSString *> *> *table_apps;
@property (nonatomic, readwrite, assign) NSInteger windows_menu_nitems;
@property (nonatomic, readwrite, assign) int checked_window_item;
@property (nonatomic, readwrite, assign) x_list *pending_apps;
@property (nonatomic, readwrite, assign) OSX_BOOL finished_launching;
@end

@implementation X11Controller

- (void) awakeFromNib
{
    X11Application *xapp = NSApp;

    /* Point X11Application at ourself. */
    xapp.controller = self;

    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;
    NSArray *appsMenu = [defaults arrayForKey:XQuartzPrefKeyAppsMenu];

    if (appsMenu) {
        int count = appsMenu.count;

        /* convert from [TITLE1 COMMAND1 TITLE2 COMMAND2 ...]
           to [[TITLE1 COMMAND1] [TITLE2 COMMAND2] ...] format. */
        if (count > 0 && ![appsMenu[0] isKindOfClass:NSArray.class]) {
            int i;
            NSMutableArray *copy, *sub;

            copy = [NSMutableArray arrayWithCapacity:(count / 2)];

            for (i = 0; i < count / 2; i++) {
                sub = [[NSMutableArray alloc] initWithCapacity:3];
                [sub addObject:appsMenu[i * 2]];
                [sub addObject:appsMenu[i * 2 + 1]];
                [sub addObject:@""];
                [copy addObject:sub];
                [sub release];
            }

            appsMenu = copy;
            [defaults setObject:appsMenu forKey:XQuartzPrefKeyAppsMenu];
        }

        [self set_apps_menu:appsMenu];
    }

    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(apps_table_done:)
                                               name:NSWindowWillCloseNotification
                                             object:self.apps_table.window];
}

- (void) item_selected:sender
{
    [NSApp activateIgnoringOtherApps:YES];

    DarwinSendDDXEvent(kXquartzControllerNotify, 2,
                       AppleWMWindowMenuItem, [sender tag]);
}

- (void) remove_apps_menu
{
    NSMenu *menu;
    NSMenuItem *item;
    int i;

    NSMenuItem * const apps_separator = self.apps_separator;
    NSMenu * const dock_apps_menu = self.dock_apps_menu;

    if (self.apps == nil || apps_separator == nil) return;

    menu = [apps_separator menu];

    if (menu != nil) {
        for (i = [menu numberOfItems] - 1; i >= 0; i--) {
            item = (NSMenuItem *)[menu itemAtIndex:i];
            if ([item tag] != 0)
                [menu removeItemAtIndex:i];
        }
    }

    if (dock_apps_menu != nil) {
        for (i = [dock_apps_menu numberOfItems] - 1; i >= 0; i--) {
            item = (NSMenuItem *)[dock_apps_menu itemAtIndex:i];
            if ([item tag] != 0)
                [dock_apps_menu removeItemAtIndex:i];
        }
    }

    self.apps = nil;
}

- (void) prepend_apps_item:(NSArray <NSArray <NSString *> *> *)list index:(int)i menu:(NSMenu *)menu
{
    NSString *title, *shortcut = @"";
    NSArray <NSString *> *group;
    NSMenuItem *item;

    group = [list objectAtIndex:i];
    title = [group objectAtIndex:0];
    if ([group count] >= 3)
        shortcut = [group objectAtIndex:2];

    if ([title length] != 0) {
        item = (NSMenuItem *)[menu insertItemWithTitle:title
                                                action:@selector (
                                  app_selected:)
                              keyEquivalent:shortcut atIndex:0];
        [item setTarget:self];
        [item setEnabled:YES];
    }
    else {
        item = (NSMenuItem *)[NSMenuItem separatorItem];
        [menu insertItem:item atIndex:0];
    }

    [item setTag:i + 1];                  /* can't be zero, so add one */
}

- (void) install_apps_menu:(NSArray <NSArray <NSString *> *> *)list
{
    NSMenu *menu;
    int i, count;

    count = [list count];

    NSMenuItem * const apps_separator = self.apps_separator;
    NSMenu * const dock_apps_menu = self.dock_apps_menu;

    if (count == 0 || apps_separator == nil) return;

    menu = [apps_separator menu];

    for (i = count - 1; i >= 0; i--) {
        if (menu != nil)
            [self prepend_apps_item:list index:i menu:menu];
        if (dock_apps_menu != nil)
            [self prepend_apps_item:list index:i menu:dock_apps_menu];
    }

    self.apps = list;
}

- (void) set_window_menu:(NSArray <NSArray <NSString *> *> *)list
{
    NSMenu * const menu = X11App.windowsMenu;
    NSMenu * const dock_menu = self.dock_menu;

    /* First, remove the existing items from the Window Menu */
    NSInteger itemsToRemove = self.windows_menu_nitems;
    if (itemsToRemove > 0) {
        NSInteger indexForRemoval = menu.numberOfItems - itemsToRemove - 1; /* we also want to remove the separator */

        for (NSInteger i = 0 ; i < itemsToRemove + 1 ; i++) {
            [menu removeItemAtIndex:indexForRemoval];
        }

        for (NSInteger i = 0 ; i < itemsToRemove; i++) {
            [dock_menu removeItemAtIndex:0];
        }
    }

    NSInteger const itemsToAdd = list.count;
    self.windows_menu_nitems = itemsToAdd;

    if (itemsToAdd > 0) {
        NSMenuItem *item;

        // Push a Separator
        [menu addItem:[NSMenuItem separatorItem]];

        for (NSInteger i = 0; i < itemsToAdd; i++) {
            NSString *name, *shortcut;

            name = list[i][0];
            shortcut = list[i][1];

            if (windowItemModMask == 0 || windowItemModMask == -1)
                shortcut = @"";

            item = (NSMenuItem *)[menu addItemWithTitle:name
                                                 action:@selector(item_selected:)
                                          keyEquivalent:shortcut];
            [item setKeyEquivalentModifierMask:(NSUInteger)windowItemModMask];
            [item setTarget:self];
            [item setTag:i];
            [item setEnabled:YES];

            item = (NSMenuItem *)[dock_menu  insertItemWithTitle:name
                                                          action:@selector(item_selected:)
                                                   keyEquivalent:shortcut
                                                         atIndex:i];
            [item setKeyEquivalentModifierMask:(NSUInteger)windowItemModMask];
            [item setTarget:self];
            [item setTag:i];
            [item setEnabled:YES];
        }

        int const checked_window_item = self.checked_window_item;
        if (checked_window_item >= 0 && checked_window_item < itemsToAdd) {
            NSInteger first = menu.numberOfItems - itemsToAdd;
            item = (NSMenuItem *)[menu itemAtIndex:first + checked_window_item];
            [item setState:NSOnState];

            item = (NSMenuItem *)[dock_menu itemAtIndex:checked_window_item];
            [item setState:NSOnState];
        }
    }

    DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMWindowMenuNotify);
}

- (void) set_window_menu_check:(NSNumber *)nn
{
    NSMenu * const menu = X11App.windowsMenu;
    NSMenu * const dock_menu = self.dock_menu;
    NSMenuItem *item;
    int n = nn.intValue;

    NSInteger const count = self.windows_menu_nitems;
    NSInteger const first = menu.numberOfItems - count;

    int const checked_window_item = self.checked_window_item;

    if (checked_window_item >= 0 && checked_window_item < count) {
        item = (NSMenuItem *)[menu itemAtIndex:first + checked_window_item];
        [item setState:NSOffState];
        item = (NSMenuItem *)[dock_menu itemAtIndex:checked_window_item];
        [item setState:NSOffState];
    }
    if (n >= 0 && n < count) {
        item = (NSMenuItem *)[menu itemAtIndex:first + n];
        [item setState:NSOnState];
        item = (NSMenuItem *)[dock_menu itemAtIndex:n];
        [item setState:NSOnState];
    }
    self.checked_window_item = n;
}

- (void) set_apps_menu:(NSArray <NSArray <NSString *> *> *)list
{
    [self remove_apps_menu];
    [self install_apps_menu:list];
}

#ifdef XQUARTZ_SPARKLE
- (void) setup_sparkle
{
    if (self.check_for_updates_item)
        return;  // already did it...

    NSMenu *menu = [self.x11_about_item menu];

    NSMenuItem * const check_for_updates_item =
        [menu insertItemWithTitle:NSLocalizedString(@"Check for X11 Updates...", @"Check for X11 Updates...")
                           action:@selector(checkForUpdates:)
                    keyEquivalent:@""
                          atIndex:1];
    [check_for_updates_item setTarget:[SUUpdater sharedUpdater]];
    [check_for_updates_item setEnabled:YES];

    self.check_for_updates_item = check_for_updates_item;

    // Set X11Controller as the delegate for the updater.
    [[SUUpdater sharedUpdater] setDelegate:self];
}

// Sent immediately before installing the specified update.
- (void)updater:(SUUpdater *)updater willInstallUpdate:(SUAppcastItem *)update
{
    //self.can_quit = YES;
}

#endif

- (void) launch_client:(NSString *)filename
{
    int child1, child2 = 0;
    int status;
    const char *newargv[4];
    char buf[128];
    char *s;
    int stdout_pipe[2];
    int stderr_pipe[2];

    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;
    NSString * const shell = [defaults stringForKey:XQuartzPrefKeyLoginShell];

    newargv[0] = shell.fileSystemRepresentation;
    newargv[1] = "-c";
    newargv[2] = filename.fileSystemRepresentation;
    newargv[3] = NULL;

    s = getenv("DISPLAY");
    if (s == NULL || s[0] == 0) {
        snprintf(buf, sizeof(buf), ":%s", display);
        setenv("DISPLAY", buf, TRUE);
    }

    if (&asl_log_descriptor) {
        char *asl_sender;
        aslmsg amsg = asl_new(ASL_TYPE_MSG);
        assert(amsg);

        asprintf(&asl_sender, "%s.%s", bundle_id_prefix, newargv[2]);
        assert(asl_sender);
        for(s = asl_sender + strlen(bundle_id_prefix) + 1; *s; s++) {
            if(! ((*s >= 'a' && *s <= 'z') ||
                  (*s >= 'A' && *s <= 'Z') ||
                  (*s >= '0' && *s <= '9'))) {
                *s = '_';
            }
        }

        (void)asl_set(amsg, ASL_KEY_SENDER, asl_sender);
        free(asl_sender);

        assert(0 == pipe(stdout_pipe));
        fcntl(stdout_pipe[0], F_SETFD, FD_CLOEXEC);
        fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);

        assert(0 == pipe(stderr_pipe));
        fcntl(stderr_pipe[0], F_SETFD, FD_CLOEXEC);
        fcntl(stderr_pipe[0], F_SETFL, O_NONBLOCK);

        asl_log_descriptor(aslc, amsg, ASL_LEVEL_INFO, stdout_pipe[0], ASL_LOG_DESCRIPTOR_READ);
        asl_log_descriptor(aslc, amsg, ASL_LEVEL_NOTICE, stderr_pipe[0], ASL_LOG_DESCRIPTOR_READ);

        asl_free(amsg);
    }

    /* Do the fork-twice trick to avoid having to reap zombies */
    child1 = fork();
    switch (child1) {
    case -1:                                    /* error */
        break;

    case 0:                                     /* child1 */
        child2 = fork();

        switch (child2) {
            int max_files, i;

        case -1:                                    /* error */
            _exit(1);

        case 0:                                     /* child2 */
            if (&asl_log_descriptor) {
                /* Replace our stdout/stderr */
                dup2(stdout_pipe[1], STDOUT_FILENO);
                dup2(stderr_pipe[1], STDERR_FILENO);
            }

            /* close all open files except for standard streams */
            max_files = sysconf(_SC_OPEN_MAX);
            for (i = 3; i < max_files; i++)
                close(i);

            /* ensure stdin is on /dev/null */
            close(0);
            open("/dev/null", O_RDONLY);

            execvp(newargv[0], (char * *const)newargv);
            _exit(2);

        default:                                    /* parent (child1) */
            _exit(0);
        }
        break;

    default:                                    /* parent */
        waitpid(child1, &status, 0);
    }

    if (&asl_log_descriptor) {
        /* Close the write ends of the pipe */
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
    }
}

- (void) app_selected:sender
{
    int tag;
    NSString *item;
    NSArray <NSArray <NSString *> *> * const apps = self.apps;

    tag = [sender tag] - 1;
    if (apps == nil || tag < 0 || tag >= [apps count])
        return;

    item = [[apps objectAtIndex:tag] objectAtIndex:1];

    [self launch_client:item];
}

- (IBAction) apps_table_show:sender
{
    NSArray *columns;
    NSMutableArray <NSMutableArray <NSString *> *> * const oldapps = self.table_apps;
    NSTableView * const apps_table = self.apps_table;

    NSMutableArray <NSMutableArray <NSString *> *> * const table_apps = [[NSMutableArray alloc] initWithCapacity:1];
    self.table_apps = table_apps;

    NSArray <NSArray <NSString *> *> * const apps = self.apps;
    if (apps != nil) {
        for (NSArray <NSString *> * row in apps) {
            [table_apps addObject:row.mutableCopy];
        }
    }

    columns = [apps_table tableColumns];
    [[columns objectAtIndex:0] setIdentifier:@"0"];
    [[columns objectAtIndex:1] setIdentifier:@"1"];
    [[columns objectAtIndex:2] setIdentifier:@"2"];

    [apps_table setDataSource:self];
    [apps_table selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
     byExtendingSelection:NO];

    [[apps_table window] makeKeyAndOrderFront:sender];
    [apps_table reloadData];
    if (oldapps != nil)
        [oldapps release];
}

- (IBAction) apps_table_done:sender
{
    NSMutableArray <NSMutableArray <NSString *> *> * const table_apps = self.table_apps;
    NSTableView * const apps_table = self.apps_table;
    [apps_table deselectAll:sender];    /* flush edits? */

    [self remove_apps_menu];
    [self install_apps_menu:table_apps];

    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;
    [defaults setObject:table_apps forKey:XQuartzPrefKeyAppsMenu];

    [[apps_table window] orderOut:sender];

    self.table_apps = nil;
}

- (IBAction) apps_table_new:sender
{
    NSMutableArray *item;
    NSMutableArray <NSMutableArray <NSString *> *> * const table_apps = self.table_apps;
    NSTableView * const apps_table = self.apps_table;

    int row = [apps_table selectedRow], i;

    if (row < 0) row = 0;
    else row = row + 1;

    i = row;
    if (i > [table_apps count])
        return;                         /* avoid exceptions */

    [apps_table deselectAll:sender];

    item = [[NSMutableArray alloc] initWithCapacity:3];
    [item addObject:@""];
    [item addObject:@""];
    [item addObject:@""];

    [table_apps insertObject:item atIndex:i];
    [item release];

    [apps_table reloadData];
    [apps_table selectRowIndexes:[NSIndexSet indexSetWithIndex:row]
     byExtendingSelection:NO];
}

- (IBAction) apps_table_duplicate:sender
{
    NSMutableArray <NSMutableArray <NSString *> *> * const table_apps = self.table_apps;
    NSTableView * const apps_table = self.apps_table;
    int row = [apps_table selectedRow], i;
    NSMutableArray <NSString *> *item;

    if (row < 0) {
        [self apps_table_new:sender];
        return;
    }

    i = row;
    if (i > [table_apps count] - 1) return;                             /* avoid exceptions */

    [apps_table deselectAll:sender];

    item = [[table_apps objectAtIndex:i] mutableCopy];
    [table_apps insertObject:item atIndex:i];
    [item release];

    [apps_table reloadData];
    [apps_table selectRowIndexes:[NSIndexSet indexSetWithIndex:row +
                                  1] byExtendingSelection:NO];
}

- (IBAction) apps_table_delete:sender
{
    NSMutableArray <NSMutableArray <NSString *> *> * const table_apps = self.table_apps;
    NSTableView * const apps_table = self.apps_table;
    int row = [apps_table selectedRow];

    if (row >= 0) {
        int i = row;

        if (i > [table_apps count] - 1) return;                 /* avoid exceptions */

        [apps_table deselectAll:sender];

        [table_apps removeObjectAtIndex:i];
    }

    [apps_table reloadData];

    row = MIN(row, [table_apps count] - 1);
    if (row >= 0)
        [apps_table selectRowIndexes:[NSIndexSet indexSetWithIndex:row]
         byExtendingSelection:NO];
}

- (NSInteger) numberOfRowsInTableView:(NSTableView *)tableView
{
    NSMutableArray <NSMutableArray <NSString *> *> * const table_apps = self.table_apps;
    if (table_apps == nil) return 0;

    return [table_apps count];
}

- (id)             tableView:(NSTableView *)tableView
   objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    NSMutableArray <NSMutableArray <NSString *> *> * const table_apps = self.table_apps;
    NSArray *item;
    int col;

    if (table_apps == nil) return nil;

    col = [[tableColumn identifier] intValue];

    item = [table_apps objectAtIndex:row];
    if ([item count] > col)
        return [item objectAtIndex:col];
    else
        return @"";
}

- (void) tableView:(NSTableView *)tableView setObjectValue:(id)object
    forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    NSMutableArray <NSMutableArray <NSString *> *> * const table_apps = self.table_apps;
    NSMutableArray <NSString *> *item;
    int col;

    if (table_apps == nil) return;

    col = [[tableColumn identifier] intValue];

    item = [table_apps objectAtIndex:row];
    [item replaceObjectAtIndex:col withObject:object];
}

- (void) hide_window:sender
{
    if ([X11App x_active])
        DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMHideWindow);
    else
        NSBeep();                       /* FIXME: something here */
}

- (IBAction)bring_to_front:sender
{
    DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMBringAllToFront);
}

- (IBAction)close_window:sender
{
    if ([X11App x_active])
        DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMCloseWindow);
    else
        [[NSApp keyWindow] performClose:sender];
}

- (IBAction)minimize_window:sender
{
    if ([X11App x_active])
        DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMMinimizeWindow);
    else
        [[NSApp keyWindow] performMiniaturize:sender];
}

- (IBAction)zoom_window:sender
{
    if ([X11App x_active])
        DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMZoomWindow);
    else
        [[NSApp keyWindow] performZoom:sender];
}

- (IBAction) next_window:sender
{
    DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMNextWindow);
}

- (IBAction) previous_window:sender
{
    DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMPreviousWindow);
}

- (IBAction) enable_fullscreen_changed:sender
{
    XQuartzRootlessDefault = !self.enable_fullscreen.state;

    [self.enable_fullscreen_menu setEnabled:!XQuartzRootlessDefault];
    [self.enable_fullscreen_menu_text setTextColor:XQuartzRootlessDefault ? NSColor.disabledControlTextColor : NSColor.controlTextColor];

    DarwinSendDDXEvent(kXquartzSetRootless, 1, XQuartzRootlessDefault);

    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;
    [defaults setBool:XQuartzRootlessDefault forKey:XQuartzPrefKeyRootless];
}

- (IBAction) toggle_fullscreen:sender
{
    DarwinSendDDXEvent(kXquartzToggleFullscreen, 0);
}

- (IBAction)prefs_changed:sender
{
    if (!sender)
        return;

    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;

    if (sender == self.fake_buttons) {
        darwinFakeButtons = !!self.fake_buttons.state;
        [defaults setBool:darwinFakeButtons forKey:XQuartzPrefKeyFakeButtons];
    } else if (sender == self.enable_keyequivs) {
        XQuartzEnableKeyEquivalents = !!self.enable_keyequivs.state;
        [defaults setBool:XQuartzEnableKeyEquivalents forKey:XQuartzPrefKeyKeyEquivs];
    } else if (sender == self.sync_keymap) {
        darwinSyncKeymap = !!self.sync_keymap.state;
        [defaults setBool:darwinSyncKeymap forKey:XQuartzPrefKeySyncKeymap];
    } else if (sender == self.enable_fullscreen_menu) {
        XQuartzFullscreenMenu = !!self.enable_fullscreen_menu.state;
        [defaults setBool:XQuartzFullscreenMenu forKey:XQuartzPrefKeyFullscreenMenu];
    } else if (sender == self.option_sends_alt) {
        BOOL prev_opt_sends_alt = XQuartzOptionSendsAlt;

        XQuartzOptionSendsAlt = !!self.option_sends_alt.state;
        [defaults setBool:XQuartzOptionSendsAlt forKey:XQuartzPrefKeyOptionSendsAlt];

        if (prev_opt_sends_alt != XQuartzOptionSendsAlt)
            QuartsResyncKeymap(TRUE);
    } else if (sender == self.click_through) {
        [defaults setBool:!!self.click_through.state forKey:XQuartzPrefKeyClickThrough];
    } else if (sender == self.focus_follows_mouse) {
        [defaults setBool:!!self.focus_follows_mouse.state forKey:XQuartzPrefKeyFocusFollowsMouse];
    } else if (sender == self.focus_on_new_window) {
        [defaults setBool:!!self.focus_on_new_window.state forKey:XQuartzPrefKeyFocusOnNewWindow];
    } else if (sender == self.enable_auth) {
        [defaults setBool:!self.enable_auth.state forKey:XQuartzPrefKeyNoAuth];
    } else if (sender == self.enable_tcp) {
        [defaults setBool:!self.enable_tcp.state forKey:XQuartzPrefKeyNoTCP];
    } else if (sender == self.depth) {
        [defaults setInteger:self.depth.selectedTag forKey:XQuartzPrefKeyDepth];
    } else if (sender == self.sync_pasteboard) {
        BOOL pbproxy_active = self.sync_pasteboard.intValue;
        [defaults setBool:pbproxy_active forKey:XQuartzPrefKeySyncPasteboard];

        [self.sync_pasteboard_to_clipboard setEnabled:pbproxy_active];
        [self.sync_pasteboard_to_primary setEnabled:pbproxy_active];
        [self.sync_clipboard_to_pasteboard setEnabled:pbproxy_active];
        [self.sync_primary_immediately setEnabled:pbproxy_active];

        // setEnabled doesn't do this...
        [self.sync_text1 setTextColor:pbproxy_active ? NSColor.controlTextColor : NSColor.disabledControlTextColor];
        [self.sync_text2 setTextColor:pbproxy_active ? NSColor.controlTextColor : NSColor.disabledControlTextColor];
    } else if (sender == self.sync_pasteboard_to_clipboard) {
        [defaults setBool:!!self.sync_pasteboard_to_clipboard.state forKey:XQuartzPrefKeySyncPasteboardToClipboard];
    } else if (sender == self.sync_pasteboard_to_primary) {
        [defaults setBool:!!self.sync_pasteboard_to_primary.state forKey:XQuartzPrefKeySyncPasteboardToPrimary];
    } else if (sender == self.sync_clipboard_to_pasteboard) {
        [defaults setBool:!!self.sync_clipboard_to_pasteboard.state forKey:XQuartzPrefKeySyncClipboardToPasteBoard];
    } else if (sender == self.sync_primary_immediately) {
        [defaults setBool:!!self.sync_primary_immediately.state forKey:XQuartzPrefKeySyncPrimaryOnSelect];
    } else if (sender == self.scroll_in_device_direction) {
        XQuartzScrollInDeviceDirection = !!self.scroll_in_device_direction.state;
        [defaults setBool:XQuartzScrollInDeviceDirection forKey:XQuartzPrefKeyScrollInDeviceDirection];
    }

    DarwinSendDDXEvent(kXquartzReloadPreferences, 0);
}

- (IBAction) prefs_show:sender
{
    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;

    BOOL pbproxy_active = [defaults boolForKey:XQuartzPrefKeySyncPasteboard];

    [self.scroll_in_device_direction setIntValue:XQuartzScrollInDeviceDirection];

    [self.fake_buttons setIntValue:darwinFakeButtons];
    [self.enable_keyequivs setIntValue:XQuartzEnableKeyEquivalents];
    [self.sync_keymap setIntValue:darwinSyncKeymap];
    [self.option_sends_alt setIntValue:XQuartzOptionSendsAlt];
    [self.click_through setIntValue:[defaults boolForKey:XQuartzPrefKeyClickThrough]];
    [self.focus_follows_mouse setIntValue:[defaults boolForKey:XQuartzPrefKeyFocusFollowsMouse]];
    [self.focus_on_new_window setIntValue:[defaults boolForKey:XQuartzPrefKeyFocusOnNewWindow]];

    [self.enable_auth setIntValue:![defaults boolForKey:XQuartzPrefKeyNoAuth]];
    [self.enable_tcp setIntValue:![defaults boolForKey:XQuartzPrefKeyNoTCP]];

    [self.depth selectItemAtIndex:[self.depth indexOfItemWithTag:[defaults integerForKey:XQuartzPrefKeyDepth]]];

    [self.sync_pasteboard setIntValue:pbproxy_active];
    [self.sync_pasteboard_to_clipboard setIntValue:[defaults boolForKey:XQuartzPrefKeySyncPasteboardToClipboard]];
    [self.sync_pasteboard_to_primary setIntValue:[defaults boolForKey:XQuartzPrefKeySyncPasteboardToPrimary]];
    [self.sync_clipboard_to_pasteboard setIntValue:[defaults boolForKey:XQuartzPrefKeySyncClipboardToPasteBoard]];
    [self.sync_primary_immediately setIntValue:[defaults boolForKey:XQuartzPrefKeySyncPrimaryOnSelect]];

    [self.sync_pasteboard_to_clipboard setEnabled:pbproxy_active];
    [self.sync_pasteboard_to_primary setEnabled:pbproxy_active];
    [self.sync_clipboard_to_pasteboard setEnabled:pbproxy_active];
    [self.sync_primary_immediately setEnabled:pbproxy_active];

    // setEnabled doesn't do this...
    [self.sync_text1 setTextColor:pbproxy_active ? NSColor.controlTextColor : NSColor.disabledControlTextColor];
    [self.sync_text2 setTextColor:pbproxy_active ? NSColor.controlTextColor : NSColor.disabledControlTextColor];

    [self.enable_fullscreen setIntValue:!XQuartzRootlessDefault];
    [self.enable_fullscreen_menu setIntValue:XQuartzFullscreenMenu];
    [self.enable_fullscreen_menu setEnabled:!XQuartzRootlessDefault];
    [self.enable_fullscreen_menu_text setTextColor:XQuartzRootlessDefault ? NSColor.disabledControlTextColor : NSColor.controlTextColor];

    [self.prefs_panel makeKeyAndOrderFront:sender];
}

- (IBAction) quit:sender
{
    DarwinSendDDXEvent(kXquartzQuit, 0);
}

- (IBAction) x11_help:sender
{
    AHLookupAnchor(CFSTR("com.apple.machelp"), CFSTR("mchlp2276"));
}

- (OSX_BOOL) validateMenuItem:(NSMenuItem *)item
{
    NSMenu *menu = [item menu];
    NSMenu * const dock_menu = self.dock_menu;

    if (item == self.toggle_fullscreen_item)
        return !XQuartzIsRootless;
    else if (menu == [X11App windowsMenu] || menu == dock_menu
             || (menu == [self.x11_about_item menu] && [item tag] == 42))
        return (AppleWMSelectedEvents() & AppleWMControllerNotifyMask) != 0;
    else
        return TRUE;
}

- (void) applicationDidHide:(NSNotification *)notify
{
    DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMHideAll);

    /* Toggle off fullscreen mode to leave our non-default video
     * mode and hide our guard window.
     */
    if (!XQuartzIsRootless && XQuartzFullscreenVisible) {
        DarwinSendDDXEvent(kXquartzToggleFullscreen, 0);
    }
}

- (void) applicationDidUnhide:(NSNotification *)notify
{
    DarwinSendDDXEvent(kXquartzControllerNotify, 1, AppleWMShowAll);
}

- (NSApplicationTerminateReply) applicationShouldTerminate:sender
{
    NSString *msg;
    NSString *title;

    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;

    if (self.can_quit || [defaults boolForKey:XQuartzPrefKeyNoQuitAlert]) {
        return NSTerminateNow;
    }

    /* Make sure we're frontmost. */
    [NSApp activateIgnoringOtherApps:YES];

    title = NSLocalizedString(@"Do you really want to quit X11?",
                              @"Dialog title when quitting");
    msg = NSLocalizedString(
        @"Any open X11 applications will stop immediately, and you will lose any unsaved changes.",
        @"Dialog when quitting");

    /* FIXME: safe to run the alert in here? Or should we return Later
     *        and then run the alert on a timer? It seems to work here, so..
     */

    NSInteger result = NSRunAlertPanel(title, @"%@", NSLocalizedString(@"Quit", @""),
                                       NSLocalizedString(@"Cancel", @""), nil, msg);
    return (result == NSAlertDefaultReturn) ? NSTerminateNow : NSTerminateCancel;
}

- (void) applicationWillTerminate:(NSNotification *)aNotification _X_NORETURN
{
    /* shutdown the X server, it will exit () for us. */
    DarwinSendDDXEvent(kXquartzQuit, 0);

    /* In case it doesn't, exit anyway after 5s. */
    [NSThread sleepForTimeInterval:5.0];

    exit(1);
}

- (void) server_ready
{
    x_list *node;

    self.finished_launching = YES;

    for (node = self.pending_apps; node != NULL; node = node->next) {
        NSString *filename = node->data;
        [self launch_client:filename];
        [filename release];
    }

    x_list_free(self.pending_apps);
    self.pending_apps = NULL;
}

- (OSX_BOOL) application:(NSApplication *)app openFile:(NSString *)filename
{
    const char *name = [filename UTF8String];

    if (self.finished_launching)
        [self launch_client:filename];
    else if (name[0] != ':')            /* ignore display names */
        self.pending_apps = x_list_prepend(self.pending_apps, [filename retain]);

    /* FIXME: report failures. */
    return YES;
}

@end

void
X11ControllerMain(int argc, char **argv, char **envp)
{
    X11ApplicationMain(argc, argv, envp);
}
