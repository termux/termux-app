/* x-selection.m -- proxies between NSPasteboard and X11 selections
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

#import "x-selection.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#import <AppKit/NSGraphics.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSBitmapImageRep.h>

#import "NSUserDefaults+XQuartzDefaults.h"

/*
 * The basic design of the pbproxy code is as follows.
 *
 * When a client selects text, say from an xterm - we only copy it when the
 * X11 Edit->Copy menu item is pressed or the shortcut activated.  In this
 * case we take the PRIMARY selection, and set it as the NSPasteboard data.
 *
 * When an X11 client copies something to the CLIPBOARD, pbproxy greedily grabs
 * the data, sets it as the NSPasteboard data, and finally sets itself as
 * owner of the CLIPBOARD.
 *
 * When an X11 window is activated we check to see if the NSPasteboard has
 * changed.  If the NSPasteboard has changed, then we set pbproxy as owner
 * of the PRIMARY and CLIPBOARD and respond to requests for text and images.
 *
 * The behavior is now dynamic since the information above was written.
 * The behavior is now dependent on the pbproxy_prefs below.
 */

/*
 * TODO:
 * 1. handle MULTIPLE - I need to study the ICCCM further, and find a test app.
 * 2. Handle NSPasteboard updates immediately, not on active/inactive
 *    - Open xterm, run 'cat readme.txt | pbcopy'
 */

static struct {
    BOOL active;
    BOOL primary_on_grab; /* This is provided as an option for people who
                           * want it and has issues that won't ever be
                           * addressed to make it *always* work.
                           */
    BOOL clipboard_to_pasteboard;
    BOOL pasteboard_to_primary;
    BOOL pasteboard_to_clipboard;
} pbproxy_prefs = { YES, NO, YES, YES, YES };

@implementation x_selection

static struct propdata null_propdata = {
    NULL, 0, 0
};

#ifdef DEBUG
static void
dump_prefs()
{
    ErrorF("pbproxy preferences:\n"
           "\tactive %u\n"
           "\tprimary_on_grab %u\n"
           "\tclipboard_to_pasteboard %u\n"
           "\tpasteboard_to_primary %u\n"
           "\tpasteboard_to_clipboard %u\n",
           pbproxy_prefs.active,
           pbproxy_prefs.primary_on_grab,
           pbproxy_prefs.clipboard_to_pasteboard,
           pbproxy_prefs.pasteboard_to_primary,
           pbproxy_prefs.pasteboard_to_clipboard);
}
#endif

static void
init_propdata(struct propdata *pdata)
{
    *pdata = null_propdata;
}

static void
free_propdata(struct propdata *pdata)
{
    free(pdata->data);
    *pdata = null_propdata;
}

/*
 * Return True if an error occurs.  Return False if pdata has data
 * and we finished.
 * The property is only deleted when bytesleft is 0 if delete is True.
 */
static Bool
get_property(Window win, Atom property, struct propdata *pdata, Bool delete,
             Atom *type)
{
    long offset = 0;
    unsigned long numitems, bytesleft = 0;
#ifdef TEST
    /* This is used to test the growth handling. */
    unsigned long length = 4UL;
#else
    unsigned long length = (100000UL + 3) / 4;
#endif
    unsigned char *buf = NULL, *chunk = NULL;
    size_t buflen = 0, chunkbytesize = 0;
    int format;

    TRACE();

    if (None == property)
        return True;

    do {
        unsigned long newbuflen = 0;
        unsigned char *newbuf = NULL;

#ifdef TEST
        ErrorF("bytesleft %lu\n", bytesleft);
#endif

        if (Success != XGetWindowProperty(xpbproxy_dpy, win, property,
                                          offset, length, delete,
                                          AnyPropertyType,
                                          type, &format, &numitems,
                                          &bytesleft, &chunk)) {
            DebugF("Error while getting window property.\n");
            *pdata = null_propdata;
            free(buf);
            return True;
        }

#ifdef TEST
        ErrorF("format %d numitems %lu bytesleft %lu\n",
               format, numitems, bytesleft);

        ErrorF("type %s\n", XGetAtomName(xpbproxy_dpy, *type));
#endif

        /* Format is the number of bits. */
        if (format == 8)
            chunkbytesize = numitems;
        else if (format == 16)
            chunkbytesize = numitems * sizeof(short);
        else if (format == 32)
            chunkbytesize = numitems * sizeof(long);

#ifdef TEST
        ErrorF("chunkbytesize %zu\n", chunkbytesize);
#endif
        newbuflen = buflen + chunkbytesize;
        if (newbuflen > 0) {
            newbuf = realloc(buf, newbuflen);

            if (NULL == newbuf) {
                XFree(chunk);
                free(buf);
                return True;
            }

            memcpy(newbuf + buflen, chunk, chunkbytesize);
            XFree(chunk);
            buf = newbuf;
            buflen = newbuflen;
            /* offset is a multiple of 32 bits*/
            offset += chunkbytesize / 4;
        }
        else {
            if (chunk)
                XFree(chunk);
        }

#ifdef TEST
        ErrorF("bytesleft %lu\n", bytesleft);
#endif
    } while (bytesleft > 0);

    pdata->data = buf;
    pdata->length = buflen;
    pdata->format = format;

    return /*success*/ False;
}

/* Implementation methods */

/* This finds the preferred type from a TARGETS list.*/
- (Atom) find_preferred:(struct propdata *)pdata
{
    Atom a = None;
    size_t i, step;
    Bool png = False, jpeg = False, utf8 = False, string = False;

    TRACE();

    if (pdata->format != 32) {
        ErrorF(
            "Atom list is expected to be formatted as an array of 32bit values.\n");
        return None;
    }

    for (i = 0, step = sizeof(long); i < pdata->length; i += step) {
        a = (Atom) * (long *)(pdata->data + i);

        if (a == atoms->image_png) {
            png = True;
        }
        else if (a == atoms->image_jpeg) {
            jpeg = True;
        }
        else if (a == atoms->utf8_string) {
            utf8 = True;
        }
        else if (a == atoms->string) {
            string = True;
        }
        else {
            char *type = XGetAtomName(xpbproxy_dpy, a);
            if (type) {
                DebugF("Unhandled X11 mime type: %s", type);
                XFree(type);
            }
        }
    }

    /*We prefer PNG over strings, and UTF8 over a Latin-1 string.*/
    if (png)
        return atoms->image_png;

    if (jpeg)
        return atoms->image_jpeg;

    if (utf8)
        return atoms->utf8_string;

    if (string)
        return atoms->string;

    /* This is evidently something we don't know how to handle.*/
    return None;
}

/* Return True if this is an INCR-style transfer. */
- (Bool) is_incr_type:(XSelectionEvent *)e
{
    Atom seltype;
    int format;
    unsigned long numitems = 0UL, bytesleft = 0UL;
    unsigned char *chunk;

    TRACE();

    if (Success != XGetWindowProperty(xpbproxy_dpy, e->requestor, e->property,
                                      /*offset*/ 0L, /*length*/ 4UL,
                                      /*Delete*/ False,
                                      AnyPropertyType, &seltype, &format,
                                      &numitems, &bytesleft, &chunk)) {
        return False;
    }

    if (chunk)
        XFree(chunk);

    return (seltype == atoms->incr) ? True : False;
}

/*
 * This should be called after a selection has been copied,
 * or when the selection is unfinished before a transfer completes.
 */
- (void) release_pending
{
    TRACE();

    free_propdata(&pending.propdata);
    pending.requestor = None;
    pending.selection = None;
}

/* Return True if an error occurs during an append.*/
/* Return False if the append succeeds. */
- (Bool) append_to_pending:(struct propdata *)pdata requestor:(Window)
   requestor
{
    unsigned char *newdata;
    size_t newlength;

    TRACE();

    if (requestor != pending.requestor) {
        [self release_pending];
        pending.requestor = requestor;
    }

    newlength = pending.propdata.length + pdata->length;
    newdata = realloc(pending.propdata.data, newlength);

    if (NULL == newdata) {
        perror("realloc propdata");
        [self release_pending];
        return True;
    }

    memcpy(newdata + pending.propdata.length, pdata->data, pdata->length);
    pending.propdata.data = newdata;
    pending.propdata.length = newlength;

    return False;
}

/* Called when X11 becomes active (i.e. has key focus) */
- (void) x_active:(Time)timestamp
{
    static NSInteger changeCount;
    NSInteger countNow;
    NSPasteboard *pb;

    TRACE();

    pb = [NSPasteboard generalPasteboard];

    if (nil == pb)
        return;

    countNow = [pb changeCount];

    if (countNow != changeCount) {
        DebugF("changed pasteboard!\n");
        changeCount = countNow;

        if (pbproxy_prefs.pasteboard_to_primary) {
            XSetSelectionOwner(xpbproxy_dpy, atoms->primary,
                               _selection_window,
                               CurrentTime);
        }

        if (pbproxy_prefs.pasteboard_to_clipboard) {
            [self own_clipboard];
        }
    }

#if 0
    /*gstaplin: we should perhaps investigate something like this branch above...*/
    if ([_pasteboard availableTypeFromArray: _known_types] != nil) {
        /* Pasteboard has data we should proxy; I think it makes
           sense to put it on both CLIPBOARD and PRIMARY */

        XSetSelectionOwner(xpbproxy_dpy, atoms->clipboard,
                           _selection_window, timestamp);
        XSetSelectionOwner(xpbproxy_dpy, atoms->primary,
                           _selection_window, timestamp);
    }
#endif
}

/* Called when X11 loses key focus */
- (void) x_inactive:(Time)timestamp
{
    TRACE();
}

/* This requests the TARGETS list from the PRIMARY selection owner. */
- (void) x_copy_request_targets
{
    TRACE();

    request_atom = atoms->targets;
    XConvertSelection(xpbproxy_dpy, atoms->primary, atoms->targets,
                      atoms->primary, _selection_window, CurrentTime);
}

/* Called when the Edit/Copy item on the main X11 menubar is selected
 * and no appkit window claims it. */
- (void) x_copy:(Time)timestamp
{
    Window w;

    TRACE();

    w = XGetSelectionOwner(xpbproxy_dpy, atoms->primary);

    if (None != w) {
        ++pending_copy;

        if (1 == pending_copy) {
            /*
             * There are no other copy operations in progress, so we
             * can proceed safely.  Otherwise the copy_completed method
             * will see that the pending_copy is > 1, and do another copy.
             */
            [self x_copy_request_targets];
        }
    }
}

/* Set pbproxy as owner of the SELECTION_MANAGER selection.
 * This prevents tools like xclipboard from causing havoc.
 * Returns TRUE on success
 */
- (BOOL) set_clipboard_manager_status:(BOOL)value
{
    TRACE();

    Window owner = XGetSelectionOwner(xpbproxy_dpy, atoms->clipboard_manager);

    if (value) {
        if (owner == _selection_window)
            return TRUE;

        if (owner != None) {
            ErrorF(
                "A clipboard manager using window 0x%lx already owns the clipboard selection.  "
                "pbproxy will not sync clipboard to pasteboard.\n", owner);
            return FALSE;
        }

        XSetSelectionOwner(xpbproxy_dpy, atoms->clipboard_manager,
                           _selection_window,
                           CurrentTime);
        return (_selection_window ==
                XGetSelectionOwner(xpbproxy_dpy, atoms->clipboard_manager));
    }
    else {
        if (owner != _selection_window)
            return TRUE;

        XSetSelectionOwner(xpbproxy_dpy, atoms->clipboard_manager, None,
                           CurrentTime);
        return (None ==
                XGetSelectionOwner(xpbproxy_dpy, atoms->clipboard_manager));
    }

    return FALSE;
}

/*
 * This occurs when we previously owned a selection,
 * and then lost it from another client.
 */
- (void) clear_event:(XSelectionClearEvent *)e
{

    TRACE();

    DebugF("e->selection %s\n", XGetAtomName(xpbproxy_dpy, e->selection));

    if (e->selection == atoms->clipboard) {
        /*
         * We lost ownership of the CLIPBOARD.
         */
        ++pending_clipboard;

        if (1 == pending_clipboard) {
            /* Claim the clipboard contents from the new owner. */
            [self claim_clipboard];
        }
    }
    else if (e->selection == atoms->clipboard_manager) {
        if (pbproxy_prefs.clipboard_to_pasteboard) {
            /* Another CLIPBOARD_MANAGER has set itself as owner.  Disable syncing
             * to avoid a race.
             */
            ErrorF("Another clipboard manager was started!  "
                   "xpbproxy is disabling syncing with clipboard.\n");
            pbproxy_prefs.clipboard_to_pasteboard = NO;
        }
    }
}

/*
 * We greedily acquire the clipboard after it changes, and on startup.
 */
- (void) claim_clipboard
{
    Window owner;

    TRACE();

    if (!pbproxy_prefs.clipboard_to_pasteboard)
        return;

    owner = XGetSelectionOwner(xpbproxy_dpy, atoms->clipboard);
    if (None == owner) {
        /*
         * The owner probably died or we are just starting up pbproxy.
         * Set pbproxy's _selection_window as the owner, and continue.
         */
        DebugF("No clipboard owner.\n");
        [self copy_completed:atoms->clipboard];
        return;
    }
    else if (owner == _selection_window) {
        [self copy_completed:atoms->clipboard];
        return;
    }

    DebugF("requesting targets\n");

    request_atom = atoms->targets;
    XConvertSelection(xpbproxy_dpy, atoms->clipboard, atoms->targets,
                      atoms->clipboard, _selection_window, CurrentTime);
    XFlush(xpbproxy_dpy);
    /* Now we will get a SelectionNotify event in the future. */
}

/* Greedily acquire the clipboard. */
- (void) own_clipboard
{

    TRACE();

    /* We should perhaps have a boundary limit on the number of iterations... */
    do {
        XSetSelectionOwner(xpbproxy_dpy, atoms->clipboard, _selection_window,
                           CurrentTime);
    } while (_selection_window != XGetSelectionOwner(xpbproxy_dpy,
                                                     atoms->clipboard));
}

- (void) init_reply:(XEvent *)reply request:(XSelectionRequestEvent *)e
{
    reply->xselection.type = SelectionNotify;
    reply->xselection.selection = e->selection;
    reply->xselection.target = e->target;
    reply->xselection.requestor = e->requestor;
    reply->xselection.time = e->time;
    reply->xselection.property = None;
}

- (void) send_reply:(XEvent *)reply
{
    /*
     * We are supposed to use an empty event mask, and not propagate
     * the event, according to the ICCCM.
     */
    DebugF("reply->xselection.requestor 0x%lx\n", reply->xselection.requestor);

    XSendEvent(xpbproxy_dpy, reply->xselection.requestor, False, 0, reply);
    XFlush(xpbproxy_dpy);
}

/*
 * This responds to a TARGETS request.
 * The result is a list of a ATOMs that correspond to the types available
 * for a selection.
 * For instance an application might provide a UTF8_STRING and a STRING
 * (in Latin-1 encoding).  The requestor can then make the choice based on
 * the list.
 */
- (void) send_targets:(XSelectionRequestEvent *)e pasteboard:(NSPasteboard *)
   pb
{
    XEvent reply;
    NSArray *pbtypes;

    [self init_reply:&reply request:e];

    pbtypes = [pb types];
    if (pbtypes) {
        long list[7]; /* Don't forget to increase this if we handle more types! */
        long count = 0;

        /*
         * I'm not sure if this is needed, but some toolkits/clients list
         * TARGETS in response to targets.
         */
        list[count] = atoms->targets;
        ++count;

        if ([pbtypes containsObject:NSStringPboardType]) {
            /* We have a string type that we can convert to UTF8, or Latin-1... */
            DebugF("NSStringPboardType\n");
            list[count] = atoms->utf8_string;
            ++count;
            list[count] = atoms->string;
            ++count;
            list[count] = atoms->compound_text;
            ++count;
        }

        /* TODO add the NSPICTPboardType back again, once we have conversion
         * functionality in send_image.
         */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations" // NSPICTPboardType
#endif

        if ([pbtypes containsObject:NSPICTPboardType]
            || [pbtypes containsObject:NSTIFFPboardType]) {
            /* We can convert a TIFF to a PNG or JPEG. */
            DebugF("NSTIFFPboardType\n");
            list[count] = atoms->image_png;
            ++count;
            list[count] = atoms->image_jpeg;
            ++count;
        }

#ifdef __clang__
#pragma clang diagnostic pop
#endif

        if (count) {
            /* We have a list of ATOMs to send. */
            XChangeProperty(xpbproxy_dpy, e->requestor, e->property,
                            atoms->atom, 32,
                            PropModeReplace, (unsigned char *)list,
                            count);

            reply.xselection.property = e->property;
        }
    }

    [self send_reply:&reply];
}

- (void) send_string:(XSelectionRequestEvent *)e utf8:(BOOL)utf8 pasteboard:(
       NSPasteboard *)pb
{
    XEvent reply;
    NSArray *pbtypes;
    NSString *data;
    const char *bytes;
    NSUInteger length;

    TRACE();

    [self init_reply:&reply request:e];

    pbtypes = [pb types];

    if (![pbtypes containsObject:NSStringPboardType]) {
        [self send_reply:&reply];
        return;
    }

    DebugF("pbtypes retainCount after containsObject: %lu\n",
           [pbtypes retainCount]);

    data = [pb stringForType:NSStringPboardType];

    if (nil == data) {
        [self send_reply:&reply];
        return;
    }

    if (utf8) {
        bytes = [data UTF8String];
        /*
         * We don't want the UTF-8 string length here.
         * We want the length in bytes.
         */
        length = strlen(bytes);

        if (length < 50) {
            DebugF("UTF-8: %s\n", bytes);
            DebugF("UTF-8 length: %lu\n", length);
        }
    }
    else {
        DebugF("Latin-1\n");
        bytes = [data cStringUsingEncoding:NSISOLatin1StringEncoding];
        /*WARNING: bytes is not NUL-terminated. */
        length = [data lengthOfBytesUsingEncoding:NSISOLatin1StringEncoding];
    }

    DebugF("e->target %s\n", XGetAtomName(xpbproxy_dpy, e->target));

    XChangeProperty(xpbproxy_dpy, e->requestor, e->property, e->target,
                    8, PropModeReplace, (unsigned char *)bytes, length);

    reply.xselection.property = e->property;

    [self send_reply:&reply];
}

- (void) send_compound_text:(XSelectionRequestEvent *)e pasteboard:(
       NSPasteboard *)pb
{
    XEvent reply;
    NSArray *pbtypes;

    TRACE();

    [self init_reply:&reply request:e];

    pbtypes = [pb types];

    if ([pbtypes containsObject: NSStringPboardType]) {
        NSString *data = [pb stringForType:NSStringPboardType];
        if (nil != data) {
            /*
             * Cast to (void *) to avoid a const warning.
             * AFAIK Xutf8TextListToTextProperty does not modify the input memory.
             */
            void *utf8 = (void *)[data UTF8String];
            char *list[] = { utf8, NULL };
            XTextProperty textprop;

            textprop.value = NULL;

            if (Success == Xutf8TextListToTextProperty(xpbproxy_dpy, list, 1,
                                                       XCompoundTextStyle,
                                                       &textprop)) {

                if (8 != textprop.format)
                    DebugF(
                        "textprop.format is unexpectedly not 8 - it's %d instead\n",
                        textprop.format);

                XChangeProperty(xpbproxy_dpy, e->requestor, e->property,
                                atoms->compound_text, textprop.format,
                                PropModeReplace, textprop.value,
                                textprop.nitems);

                reply.xselection.property = e->property;
            }

            if (textprop.value)
                XFree(textprop.value);

        }
    }

    [self send_reply:&reply];
}

/* Finding a test application that uses MULTIPLE has proven to be difficult. */
- (void) send_multiple:(XSelectionRequestEvent *)e
{
    XEvent reply;

    TRACE();

    [self init_reply:&reply request:e];

    if (None != e->property) {}

    [self send_reply:&reply];
}

/* Return nil if an error occurred. */
/* DO NOT retain the encdata for longer than the length of an event response.
 * The autorelease pool will reuse/free it.
 */
- (NSData *) encode_image_data:(NSData *)data type:(NSBitmapImageFileType)
   enctype
{
    NSBitmapImageRep *bmimage = nil;
    NSData *encdata = nil;
    NSDictionary *dict = nil;

    bmimage = [[NSBitmapImageRep alloc] initWithData:data];

    if (nil == bmimage)
        return nil;

    dict = [[NSDictionary alloc] init];
    encdata = [bmimage representationUsingType:enctype properties:dict];

    if (nil == encdata) {
        [dict autorelease];
        [bmimage autorelease];
        return nil;
    }

    [dict autorelease];
    [bmimage autorelease];

    return encdata;
}

/* Return YES when an error has occurred when trying to send the PICT. */
/* The caller should send a default response with a property of None when an error occurs. */
- (BOOL) send_image_pict_reply:(XSelectionRequestEvent *)e
                    pasteboard:(NSPasteboard *)pb
                          type:(NSBitmapImageFileType)imagetype
{
    XEvent reply;
    NSImage *img = nil;
    NSData *data = nil, *encdata = nil;
    NSUInteger length;
    const void *bytes = NULL;

    img = [[NSImage alloc] initWithPasteboard:pb];

    if (nil == img) {
        return YES;
    }

    data = [img TIFFRepresentation];

    if (nil == data) {
        [img autorelease];
        ErrorF("unable to convert PICT to TIFF!\n");
        return YES;
    }

    encdata = [self encode_image_data:data type:imagetype];
    if (nil == encdata) {
        [img autorelease];
        return YES;
    }

    [self init_reply:&reply request:e];

    length = [encdata length];
    bytes = [encdata bytes];

    XChangeProperty(xpbproxy_dpy, e->requestor, e->property, e->target,
                    8, PropModeReplace, bytes, length);
    reply.xselection.property = e->property;

    [self send_reply:&reply];

    [img autorelease];

    return NO; /*no error*/
}

/* Return YES if an error occurred. */
/* The caller should send a reply with a property of None when an error occurs. */
- (BOOL) send_image_tiff_reply:(XSelectionRequestEvent *)e
                    pasteboard:(NSPasteboard *)pb
                          type:(NSBitmapImageFileType)imagetype
{
    XEvent reply;
    NSData *data = nil;
    NSData *encdata = nil;
    NSUInteger length;
    const void *bytes = NULL;

    data = [pb dataForType:NSTIFFPboardType];

    if (nil == data)
        return YES;

    encdata = [self encode_image_data:data type:imagetype];

    if (nil == encdata)
        return YES;

    [self init_reply:&reply request:e];

    length = [encdata length];
    bytes = [encdata bytes];

    XChangeProperty(xpbproxy_dpy, e->requestor, e->property, e->target,
                    8, PropModeReplace, bytes, length);
    reply.xselection.property = e->property;

    [self send_reply:&reply];

    return NO; /*no error*/
}

- (void) send_image:(XSelectionRequestEvent *)e pasteboard:(NSPasteboard *)pb
{
    NSArray *pbtypes = nil;
    NSBitmapImageFileType imagetype = NSPNGFileType;

    TRACE();

    if (e->target == atoms->image_png)
        imagetype = NSPNGFileType;
    else if (e->target == atoms->image_jpeg)
        imagetype = NSJPEGFileType;
    else {
        ErrorF(
            "internal failure in xpbproxy!  imagetype being sent isn't PNG or JPEG.\n");
    }

    pbtypes = [pb types];

    if (pbtypes) {
        if ([pbtypes containsObject:NSTIFFPboardType]) {
            if (NO ==
                [self send_image_tiff_reply:e pasteboard:pb type:imagetype])
                return;
        }
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations" // NSPICTPboardType
#endif
        else if ([pbtypes containsObject:NSPICTPboardType])
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        {
            if (NO ==
                [self send_image_pict_reply:e pasteboard:pb type:imagetype])
                return;

            /* Fall through intentionally to the send_none: */
        }
    }

    [self send_none:e];
}

- (void)send_none:(XSelectionRequestEvent *)e
{
    XEvent reply;

    TRACE();

    [self init_reply:&reply request:e];
    [self send_reply:&reply];
}

/* Another client requested the data or targets of data available from the clipboard. */
- (void)request_event:(XSelectionRequestEvent *)e
{
    NSPasteboard *pb;

    TRACE();

    /* TODO We should also keep track of the time of the selection, and
     * according to the ICCCM "refuse the request" if the event timestamp
     * is before we owned it.
     * What should we base the time on?  How can we get the current time just
     * before an XSetSelectionOwner?  Is it the server's time, or the clients?
     * According to the XSelectionRequestEvent manual page, the Time value
     * may be set to CurrentTime or a time, so that makes it a bit different.
     * Perhaps we should just punt and ignore races.
     */

    /*TODO we need a COMPOUND_TEXT test app*/
    /*TODO we need a MULTIPLE test app*/

    pb = [NSPasteboard generalPasteboard];
    if (nil == pb) {
        [self send_none:e];
        return;
    }

    if (None != e->target)
        DebugF("e->target %s\n", XGetAtomName(xpbproxy_dpy, e->target));

    if (e->target == atoms->targets) {
        /* The paste requestor wants to know what TARGETS we support. */
        [self send_targets:e pasteboard:pb];
    }
    else if (e->target == atoms->multiple) {
        /*
         * This isn't finished, and may never be, unless I can find
         * a good test app.
         */
        [self send_multiple:e];
    }
    else if (e->target == atoms->utf8_string) {
        [self send_string:e utf8:YES pasteboard:pb];
    }
    else if (e->target == atoms->string) {
        [self send_string:e utf8:NO pasteboard:pb];
    }
    else if (e->target == atoms->compound_text) {
        [self send_compound_text:e pasteboard:pb];
    }
    else if (e->target == atoms->multiple) {
        [self send_multiple:e];
    }
    else if (e->target == atoms->image_png || e->target ==
             atoms->image_jpeg) {
        [self send_image:e pasteboard:pb];
    }
    else {
        [self send_none:e];
    }
}

/* This handles the events resulting from an XConvertSelection request. */
- (void) notify_event:(XSelectionEvent *)e
{
    Atom type;
    struct propdata pdata;

    TRACE();

    [self release_pending];

    if (None == e->property) {
        DebugF("e->property is None.\n");
        [self copy_completed:e->selection];
        /* Nothing is selected. */
        return;
    }

#if 0
    ErrorF("e->selection %s\n", XGetAtomName(xpbproxy_dpy, e->selection));
    ErrorF("e->property %s\n", XGetAtomName(xpbproxy_dpy, e->property));
#endif

    if ([self is_incr_type:e]) {
        /*
         * This is an INCR-style transfer, which means that we
         * will get the data after a series of PropertyNotify events.
         */
        DebugF("is INCR\n");

        if (get_property(e->requestor, e->property, &pdata, /*Delete*/ True,
                         &type)) {
            /*
             * An error occurred, so we should invoke the copy_completed:, but
             * not handle_selection:type:propdata:
             */
            [self copy_completed:e->selection];
            return;
        }

        free_propdata(&pdata);

        pending.requestor = e->requestor;
        pending.selection = e->selection;

        DebugF("set pending.requestor to 0x%lx\n", pending.requestor);
    }
    else {
        if (get_property(e->requestor, e->property, &pdata, /*Delete*/ True,
                         &type)) {
            [self copy_completed:e->selection];
            return;
        }

        /* We have the complete selection data.*/
        [self handle_selection:e->selection type:type propdata:&pdata];

        DebugF("handled selection with the first notify_event\n");
    }
}

/* This is used for INCR transfers.  See the ICCCM for the details. */
/* This is used to retrieve PRIMARY and CLIPBOARD selections. */
- (void) property_event:(XPropertyEvent *)e
{
    struct propdata pdata;
    Atom type;

    TRACE();

    if (None != e->atom) {
#ifdef DEBUG
        char *name = XGetAtomName(xpbproxy_dpy, e->atom);

        if (name) {
            DebugF("e->atom %s\n", name);
            XFree(name);
        }
#endif
    }

    if (None != pending.requestor && PropertyNewValue == e->state) {
        DebugF("pending.requestor 0x%lx\n", pending.requestor);

        if (get_property(e->window, e->atom, &pdata, /*Delete*/ True,
                         &type)) {
            [self copy_completed:pending.selection];
            [self release_pending];
            return;
        }

        if (0 == pdata.length) {
            /*
             * We completed the transfer.
             * handle_selection will call copy_completed: for us.
             */
            [self handle_selection:pending.selection type:type propdata:&
             pending.propdata];
            free_propdata(&pdata);
            pending.propdata = null_propdata;
            pending.requestor = None;
            pending.selection = None;
        }
        else {
            [self append_to_pending:&pdata requestor:e->window];
            free_propdata(&pdata);
        }
    }
}

- (void) xfixes_selection_notify:(XFixesSelectionNotifyEvent *)e
{
    if (!pbproxy_prefs.active)
        return;

    switch (e->subtype) {
    case XFixesSetSelectionOwnerNotify:
        if (e->selection == atoms->primary && pbproxy_prefs.primary_on_grab)
            [self x_copy:e->timestamp];
        break;

    case XFixesSelectionWindowDestroyNotify:
    case XFixesSelectionClientCloseNotify:
    default:
        ErrorF("Unhandled XFixesSelectionNotifyEvent: subtype=%d\n",
               e->subtype);
        break;
    }
}

- (void) handle_targets: (Atom)selection propdata:(struct propdata *)pdata
{
    /* Find a type we can handle and prefer from the list of ATOMs. */
    Atom preferred;
    char *name;

    TRACE();

    preferred = [self find_preferred:pdata];

    if (None == preferred) {
        /*
         * This isn't required by the ICCCM, but some apps apparently
         * don't respond to TARGETS properly.
         */
        preferred = atoms->string;
    }

    (void)name; /* Avoid a warning with non-debug compiles. */
#ifdef DEBUG
    name = XGetAtomName(xpbproxy_dpy, preferred);

    if (name) {
        DebugF("requesting %s\n", name);
    }
#endif
    request_atom = preferred;
    XConvertSelection(xpbproxy_dpy, selection, preferred, selection,
                      _selection_window, CurrentTime);
}

/* This handles the image type of selection (typically in CLIPBOARD). */
/* We convert to a TIFF, so that other applications can paste more easily. */
- (void) handle_image: (struct propdata *)pdata pasteboard:(NSPasteboard *)pb
{
    NSArray *pbtypes;
    NSUInteger length;
    NSData *data, *tiff;
    NSBitmapImageRep *bmimage;

    TRACE();

    length = pdata->length;
    data = [[NSData alloc] initWithBytes:pdata->data length:length];

    if (nil == data) {
        DebugF("unable to create NSData object!\n");
        return;
    }

    DebugF("data retainCount before NSBitmapImageRep initWithData: %lu\n",
           [data retainCount]);

    bmimage = [[NSBitmapImageRep alloc] initWithData:data];

    if (nil == bmimage) {
        [data autorelease];
        DebugF("unable to create NSBitmapImageRep!\n");
        return;
    }

    DebugF("data retainCount after NSBitmapImageRep initWithData: %lu\n",
           [data retainCount]);

    @try
    {
        tiff = [bmimage TIFFRepresentation];
    }

    @catch (NSException *e)
    {
        DebugF("NSTIFFException!\n");
        [data autorelease];
        [bmimage autorelease];
        return;
    }

    DebugF("bmimage retainCount after TIFFRepresentation %lu\n",
           [bmimage retainCount]);

    pbtypes = [NSArray arrayWithObjects:NSTIFFPboardType, nil];

    if (nil == pbtypes) {
        [data autorelease];
        [bmimage autorelease];
        return;
    }

    [pb declareTypes:pbtypes owner:nil];
    if (YES != [pb setData:tiff forType:NSTIFFPboardType]) {
        DebugF("writing pasteboard data failed!\n");
    }

    [data autorelease];

    DebugF("bmimage retainCount before release %lu\n", [bmimage retainCount]);

    [bmimage autorelease];
}

/* This handles the UTF8_STRING type of selection. */
- (void) handle_utf8_string:(struct propdata *)pdata pasteboard:(NSPasteboard
                                                                 *)pb
{
    NSString *string;
    NSArray *pbtypes;

    TRACE();

    string =
        [[NSString alloc] initWithBytes:pdata->data length:pdata->length
         encoding:
         NSUTF8StringEncoding];

    if (nil == string)
        return;

    pbtypes = [NSArray arrayWithObjects:NSStringPboardType, nil];

    if (nil == pbtypes) {
        [string autorelease];
        return;
    }

    [pb declareTypes:pbtypes owner:nil];

    if (YES != [pb setString:string forType:NSStringPboardType]) {
        ErrorF("pasteboard setString:forType: failed!\n");
    }
    [string autorelease];
    DebugF("done handling utf8 string\n");
}

/* This handles the STRING type, which should be in Latin-1. */
- (void) handle_string: (struct propdata *)pdata pasteboard:(NSPasteboard *)
   pb
{
    NSString *string;
    NSArray *pbtypes;

    TRACE();

    string =
        [[NSString alloc] initWithBytes:pdata->data length:pdata->length
         encoding:
         NSISOLatin1StringEncoding];

    if (nil == string)
        return;

    pbtypes = [NSArray arrayWithObjects:NSStringPboardType, nil];

    if (nil == pbtypes) {
        [string autorelease];
        return;
    }

    [pb declareTypes:pbtypes owner:nil];
    if (YES != [pb setString:string forType:NSStringPboardType]) {
        ErrorF("pasteboard setString:forType failed in handle_string!\n");
    }
    [string autorelease];
}

/* This is called when the selection is completely retrieved from another client. */
/* Warning: this frees the propdata. */
- (void) handle_selection:(Atom)selection type:(Atom)type propdata:(struct
                                                                    propdata
                                                                    *)pdata
{
    NSPasteboard *pb;

    TRACE();

    pb = [NSPasteboard generalPasteboard];

    if (nil == pb) {
        [self copy_completed:selection];
        free_propdata(pdata);
        return;
    }

    /*
     * Some apps it seems set the type to TARGETS instead of ATOM, such as Eterm.
     * These aren't ICCCM compliant apps, but we need these to work...
     */
    if (request_atom == atoms->targets
        && (type == atoms->atom || type == atoms->targets)) {
        [self handle_targets:selection propdata:pdata];
        free_propdata(pdata);
        return;
    }
    else if (type == atoms->image_png) {
        [self handle_image:pdata pasteboard:pb];
    }
    else if (type == atoms->image_jpeg) {
        [self handle_image:pdata pasteboard:pb];
    }
    else if (type == atoms->utf8_string) {
        [self handle_utf8_string:pdata pasteboard:pb];
    }
    else if (type == atoms->string) {
        [self handle_string:pdata pasteboard:pb];
    }

    free_propdata(pdata);

    [self copy_completed:selection];
}

- (void) copy_completed:(Atom)selection
{
    TRACE();
    char *name;

    (void)name; /* Avoid warning with non-debug compiles. */
#ifdef DEBUG
    name = XGetAtomName(xpbproxy_dpy, selection);
    if (name) {
        DebugF("copy_completed: %s\n", name);
        XFree(name);
    }
#endif

    if (selection == atoms->primary && pending_copy > 0) {
        --pending_copy;
        if (pending_copy > 0) {
            /* Copy PRIMARY again. */
            [self x_copy_request_targets];
            return;
        }
    }
    else if (selection == atoms->clipboard && pending_clipboard > 0) {
        --pending_clipboard;
        if (pending_clipboard > 0) {
            /* Copy CLIPBOARD. */
            [self claim_clipboard];
            return;
        }
        else {
            /* We got the final data.  Now set pbproxy as the owner. */
            [self own_clipboard];
            return;
        }
    }

    /*
     * We had 1 or more primary in progress, and the clipboard arrived
     * while we were busy.
     */
    if (pending_clipboard > 0) {
        [self claim_clipboard];
    }
}

- (void) reload_preferences
{
    NSUserDefaults * const defaults = NSUserDefaults.xquartzDefaults;

#ifdef STANDALONE_XPBPROXY
    if (xpbproxy_is_standalone)
        pbproxy_prefs.active = YES;
    else
#endif
    pbproxy_prefs.active = [defaults boolForKey:XQuartzPrefKeySyncPasteboard];
    pbproxy_prefs.primary_on_grab = [defaults boolForKey:XQuartzPrefKeySyncPrimaryOnSelect];
    pbproxy_prefs.clipboard_to_pasteboard = [defaults boolForKey:XQuartzPrefKeySyncClipboardToPasteBoard];
    pbproxy_prefs.pasteboard_to_primary = [defaults boolForKey:XQuartzPrefKeySyncPasteboardToPrimary];
    pbproxy_prefs.pasteboard_to_clipboard = [defaults boolForKey:XQuartzPrefKeySyncPasteboardToClipboard];

    /* This is used for debugging. */
    //dump_prefs();

    if (pbproxy_prefs.active && pbproxy_prefs.primary_on_grab &&
        !xpbproxy_have_xfixes) {
        ErrorF(
            "Disabling sync_primary_on_select functionality due to missing XFixes extension.\n");
        pbproxy_prefs.primary_on_grab = NO;
    }

    /* Claim or release the CLIPBOARD_MANAGER atom */
    if (![self set_clipboard_manager_status:(pbproxy_prefs.active &&
                                             pbproxy_prefs.
                                             clipboard_to_pasteboard)])
        pbproxy_prefs.clipboard_to_pasteboard = NO;

    if (pbproxy_prefs.active && pbproxy_prefs.clipboard_to_pasteboard)
        [self claim_clipboard];
}

- (BOOL) is_active
{
    return pbproxy_prefs.active;
}

/* NSPasteboard-required methods */

- (void) paste:(id)sender
{
    TRACE();
}

- (void) pasteboard:(NSPasteboard *)pb provideDataForType:(NSString *)type
{
    TRACE();
}

- (void) pasteboardChangedOwner:(NSPasteboard *)pb
{
    TRACE();

    /* Right now we don't care with this. */
}

/* Allocation */

- (id) init
{
    unsigned long pixel;

    self = [super init];
    if (self == nil)
        return nil;

    atoms->primary = XInternAtom(xpbproxy_dpy, "PRIMARY", False);
    atoms->clipboard = XInternAtom(xpbproxy_dpy, "CLIPBOARD", False);
    atoms->text = XInternAtom(xpbproxy_dpy, "TEXT", False);
    atoms->utf8_string = XInternAtom(xpbproxy_dpy, "UTF8_STRING", False);
    atoms->string = XInternAtom(xpbproxy_dpy, "STRING", False);
    atoms->targets = XInternAtom(xpbproxy_dpy, "TARGETS", False);
    atoms->multiple = XInternAtom(xpbproxy_dpy, "MULTIPLE", False);
    atoms->cstring = XInternAtom(xpbproxy_dpy, "CSTRING", False);
    atoms->image_png = XInternAtom(xpbproxy_dpy, "image/png", False);
    atoms->image_jpeg = XInternAtom(xpbproxy_dpy, "image/jpeg", False);
    atoms->incr = XInternAtom(xpbproxy_dpy, "INCR", False);
    atoms->atom = XInternAtom(xpbproxy_dpy, "ATOM", False);
    atoms->clipboard_manager = XInternAtom(xpbproxy_dpy, "CLIPBOARD_MANAGER",
                                           False);
    atoms->compound_text = XInternAtom(xpbproxy_dpy, "COMPOUND_TEXT", False);
    atoms->atom_pair = XInternAtom(xpbproxy_dpy, "ATOM_PAIR", False);

    pixel = BlackPixel(xpbproxy_dpy, DefaultScreen(xpbproxy_dpy));
    _selection_window =
        XCreateSimpleWindow(xpbproxy_dpy, DefaultRootWindow(xpbproxy_dpy),
                            0, 0, 1, 1, 0, pixel, pixel);

    /* This is used to get PropertyNotify events when doing INCR transfers. */
    XSelectInput(xpbproxy_dpy, _selection_window, PropertyChangeMask);

    request_atom = None;

    init_propdata(&pending.propdata);
    pending.requestor = None;
    pending.selection = None;

    pending_copy = 0;
    pending_clipboard = 0;

    if (xpbproxy_have_xfixes)
        XFixesSelectSelectionInput(xpbproxy_dpy, _selection_window,
                                   atoms->primary,
                                   XFixesSetSelectionOwnerNotifyMask);

    [self reload_preferences];

    return self;
}

- (void) dealloc
{
    if (None != _selection_window) {
        XDestroyWindow(xpbproxy_dpy, _selection_window);
        _selection_window = None;
    }

    free_propdata(&pending.propdata);

    [super dealloc];
}

@end
