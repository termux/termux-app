/*
 * Don't #include any of the AppKit, etc stuff directly since it will
 * pollute the X11 namespace.
 */

#ifndef _XQ_SANITIZED_COCOA_H_
#define _XQ_SANITIZED_COCOA_H_

// QuickDraw in ApplicationServices has the following conflicts with
// the basic X server headers. Use QD_<name> to use the QuickDraw
// definition of any of these symbols, or the normal name for the
// X11 definition.
#define Cursor    QD_Cursor
#define WindowPtr QD_WindowPtr
#define Picture   QD_Picture
#define BOOL      OSX_BOOL
#define EventType HIT_EventType

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#undef Cursor
#undef WindowPtr
#undef Picture
#undef BOOL
#undef EventType

#ifndef __has_feature
#define __has_feature(x) 0 // Compatibility with non-clang compilers.
#endif

#ifndef NS_RETURNS_RETAINED
#if __has_feature(attribute_ns_returns_retained)
#define NS_RETURNS_RETAINED __attribute__((ns_returns_retained))
#else
#define NS_RETURNS_RETAINED
#endif
#endif

#ifndef NS_RETURNS_NOT_RETAINED
#if __has_feature(attribute_ns_returns_not_retained)
#define NS_RETURNS_NOT_RETAINED __attribute__((ns_returns_not_retained))
#else
#define NS_RETURNS_NOT_RETAINED
#endif
#endif

#ifndef CF_RETURNS_RETAINED
#if __has_feature(attribute_cf_returns_retained)
#define CF_RETURNS_RETAINED __attribute__((cf_returns_retained))
#else
#define CF_RETURNS_RETAINED
#endif
#endif

#ifndef CF_RETURNS_NOT_RETAINED
#if __has_feature(attribute_cf_returns_not_retained)
#define CF_RETURNS_NOT_RETAINED __attribute__((cf_returns_not_retained))
#else
#define CF_RETURNS_NOT_RETAINED
#endif
#endif

#endif  /* _XQ_SANITIZED_COCOA_H_ */
