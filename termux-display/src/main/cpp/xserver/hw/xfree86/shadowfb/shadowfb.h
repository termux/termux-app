
#ifndef _SHADOWFB_H
#define _SHADOWFB_H

#include "xf86str.h"

/*
 * User defined callback function.  Passed a pointer to the ScrnInfo struct,
 * the number of dirty rectangles, and a pointer to the first dirty rectangle
 * in the array.
 */
typedef void (*RefreshAreaFuncPtr) (ScrnInfoPtr, int, BoxPtr);

/*
 * ShadowFBInit initializes the shadowfb subsystem.  refreshArea is a pointer
 * to a user supplied callback function.  This function will be called after
 * any operation that modifies the framebuffer.  The newly dirtied rectangles
 * are passed to the callback.
 *
 * Returns FALSE in the event of an error.
 */
extern _X_EXPORT Bool
 ShadowFBInit(ScreenPtr pScreen, RefreshAreaFuncPtr refreshArea);

/*
 * ShadowFBInit2 is a more featureful refinement of the original shadowfb.
 * ShadowFBInit2 allows you to specify two callbacks, one to be called
 * immediately before an operation that modifies the framebuffer, and another
 * to be called immediately after.
 *
 * Returns FALSE in the event of an error
 */
extern _X_EXPORT Bool

ShadowFBInit2(ScreenPtr pScreen,
              RefreshAreaFuncPtr preRefreshArea,
              RefreshAreaFuncPtr postRefreshArea);

#endif                          /* _SHADOWFB_H */
