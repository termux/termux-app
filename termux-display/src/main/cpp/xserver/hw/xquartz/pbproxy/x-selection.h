/* x-selection.h -- proxies between NSPasteboard and X11 selections
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

#ifndef X_SELECTION_H
#define X_SELECTION_H 1

#include "pbproxy.h"

#define  Cursor X_Cursor
#include <X11/extensions/Xfixes.h>
#undef Cursor

#include <AppKit/NSPasteboard.h>

/* This stores image data or text. */
struct propdata {
    unsigned char *data;
    size_t length;
    int format;
};

struct atom_list {
    Atom primary, clipboard, text, utf8_string, string, targets, multiple,
         cstring, image_png, image_jpeg, incr, atom, clipboard_manager,
         compound_text, atom_pair;
};

@interface x_selection : NSObject
{
    @private

    /* The unmapped window we use for fetching selections. */
    Window _selection_window;

    Atom request_atom;

    struct {
        struct propdata propdata;
        Window requestor;
        Atom selection;
    } pending;

    /*
     * This is the number of times the user has requested a copy.
     * Once the copy is completed, we --pending_copy, and if the
     * pending_copy is > 0 we do it again.
     */
    int pending_copy;
    /*
     * This is used for the same purpose as pending_copy, but for the
     * CLIPBOARD.  It also prevents a race with INCR transfers.
     */
    int pending_clipboard;

    struct atom_list atoms[1];
}

- (void)x_active:(Time)timestamp;
- (void)x_inactive:(Time)timestamp;

- (void)x_copy:(Time)timestamp;

- (void)clear_event:(XSelectionClearEvent *)e;
- (void)request_event:(XSelectionRequestEvent *)e;
- (void)notify_event:(XSelectionEvent *)e;
- (void)property_event:(XPropertyEvent *)e;
- (void)xfixes_selection_notify:(XFixesSelectionNotifyEvent *)e;
- (void)handle_selection:(Atom) selection type:(Atom) type propdata:(struct
                                                                     propdata
                                                                     *)pdata;
- (void)claim_clipboard;
- (BOOL)set_clipboard_manager_status:(BOOL)value;
- (void)own_clipboard;
- (void)copy_completed:(Atom)selection;

- (void)reload_preferences;
- (BOOL)is_active;
- (void)send_none:(XSelectionRequestEvent *)e;
@end

/* main.m */
extern x_selection * _selection_object;

#endif /* X_SELECTION_H */
