                            Generic Rootless Layer
                                 Version 1.0
                                July 13, 2004

                               Torrey T. Lyons
                              torrey@xfree86.org


Introduction

        The generic rootless layer allows an X server to be implemented 
on top of another window server in a cooperative manner. This allows the 
X11 windows and native windows of the underlying window server to 
coexist on the same screen. The layer is called "rootless" because the root 
window of the X server is generally not drawn. Instead, each top-level 
child of the root window is represented as a separate on-screen window by 
the underlying window server. The layer is referred to as "generic" 
because it abstracts away the details of the underlying window system and 
contains code that is useful for any rootless X server. The code for the 
generic rootless layer is located in xc/programs/Xserver/miext/rootless. To 
build a complete rootless X server requires a specific rootless 
implementation, which provides functions that allow the generic rootless 
layer to interact with the underlying window system.


Concepts

        In the context of a rootless X server the term window is used to 
mean many fundamentally different things. For X11 a window is a DDX 
resource that describes a visible, or potentially visible, rectangle on the 
screen. A top-level window is a direct child of the root window. To avoid 
confusion, an on-screen native window of the underlying window system 
is referred to as a "frame". The generic rootless layer associates each 
mapped top-level X11 window with a frame. An X11 window may be said 
to be "framed" if it or its top-level parent is represented by a frame.

        The generic rootless layer models each frame as being backed at 
all times by a backing buffer, which is periodically flushed to the screen. 
If the underlying window system does not provide a backing buffer for 
frames, this must be done by the rootless implementation. The generic 
rootless layer model does not assume it always has access to the frames' 
backing buffers. Any drawing to the buffer will be proceeded by a call to 
the rootless implementation's StartDrawing() function and StopDrawing() 
will be called when the drawing is concluded. The address of the frame's 
backing buffer is returned by the StartDrawing() function and it can 
change between successive calls.

        Because each frame is assumed to have a backing buffer, the 
generic rootless layer will stop Expose events being generated when the 
regions of visibility of a frame change on screen. This is similar to backing 
store, but backing buffers are different in that they always store a copy of 
the entire window contents, not just the obscured portions. The price paid 
in increased memory consumption is made up by the greatly decreased 
complexity in not having to track and record regions as they are obscured.


Rootless Implementation

        The specifics of the underlying window system are provided to the 
generic rootless layer through rootless implementation functions, compile-
time options, and runtime parameters. The rootless implementation 
functions are a list of functions that allow the generic rootless layer to 
perform operations such as creating, destroying, moving, and resizing 
frames. Some of the implementation functions are optional. A detailed 
description of the rootless implementation functions is provided in 
Appendix A.

        By design, a rootless implementation should only have to include 
the rootless.h header file. The rootlessCommon.h file contains definitions 
internal to the generic rootless layer. (If you find you need to use 
rootlessCommon.h in your implementation, let the generic rootless layer 
maintainers know. This could be an area where the generic rootless layer 
should be generalized.) A rootless implementation should also modify 
rootlessConfig.h to specify compile time options for its platform.

        The following compile-time options are defined in 
rootlessConfig.h:

      o ROOTLESS_PROTECT_ALPHA: By default for a color bit depth of 24 and
        32 bits per pixel, fb will overwrite the "unused" 8 bits to optimize
        drawing speed. If this is true, the alpha channel of frames is
        protected and is not modified when drawing to them. The bits 
        containing the alpha channel are defined by the macro 
        RootlessAlphaMask(bpp), which should return a bit mask for 
        various bits per pixel.

      o ROOTLESS_REDISPLAY_DELAY: Time in milliseconds between updates to
        the underlying window server. Most operations will be buffered until
        this time has expired.

      o ROOTLESS_RESIZE_GRAVITY: If the underlying window system supports it,
        some frame resizes can be optimized by relying on the frame contents
        maintaining a particular gravity during the resize. In this way less
        of the frame contents need to be preserved by the generic rootless
        layer. If true, the generic rootless layer will pass gravity hints
        during resizing and rely on the frame contents being preserved
        accordingly.

        The following runtime options are defined in rootless.h:

      o rootlessGlobalOffsetX, rootlessGlobalOffsetY: These specify the global
        offset that is applied to all screens when converting from
        screen-local to global coordinates.

      o rootless_CopyBytes_threshold, rootless_CopyWindow_threshold:
        The minimum number of bytes or pixels for which to use the rootless
        implementation's respective acceleration function. The rootless
        acceleration functions are all optional so these will only be used
        if the respective acceleration function pointer is not NULL.


Accelerated Drawing

	The rootless implementation typically does not have direct access 
to the hardware. Its access to the graphics hardware is generally through 
the API of the underlying window system. This underlying API may not 
overlap well with the X11 drawing primitives. The generic rootless layer 
falls back to using fb for all its 2-D drawing. Providing optional rootless 
implementation acceleration functions can accelerate some graphics 
primitives and some window functions. Typically calling through to the 
underlying window systems API will not speed up these operations for 
small enough areas. The rootless_*_threshold runtime options allow the 
rootless implementation to provide hints for when the acceleration 
functions should be used instead of fb.


Alpha Channel Protection

	If the bits per pixel is greater then the color bit depth, the contents 
of the extra bits are undefined by the X11 protocol. Some window systems 
will use these extra bits as an alpha channel. The generic rootless layer can 
be configured to protect these bits and make sure they are not modified by 
other parts of the X server. To protect the alpha channel 
ROOTLESS_PROTECT_ALPHA and RootlessAlphaMask(bpp) must be 
set appropriately as described under the compile time options. This 
ensures that the X11 graphics primitives do not overwrite the alpha 
channel in an attempt to optimize drawing. In addition, the window 
functions PaintWindow() and Composite() must be replaced by alpha 
channel safe variants. These are provided in rootless/safeAlpha.


Credits

	The generic rootless layer was originally conceived and developed 
by Greg Parker as part of the XDarwin X server on Mac OS X. John 
Harper made later optimizations to this code but removed its generic 
independence of the underlying window system. Torrey T. Lyons 
reintroduced the generic abstractions and made the rootless code suitable 
for use by other X servers.


Appendix A: Rootless Implementation Functions

	The rootless implementation functions are defined in rootless.h. It 
is intended that rootless.h contains the complete interface that is needed by 
rootless implementations. The definitions contained in rootlessCommon.h 
are intended for internal use by the generic rootless layer and are more 
likely to change.

	Most of these functions take a RootlessFrameID as a parameter. 
The RootlessFrameID is an opaque object that is returned by the 
implementation's CreateFrame() function. The generic rootless layer does 
not use this frame id other than to pass it back to the rootless 
implementation to indicate the frame to operate on.

/*
 * Create a new frame.
 *  The frame is created unmapped.
 *
 *  pFrame      RootlessWindowPtr for this frame should be completely
 *              initialized before calling except for pFrame->wid, which
 *              is set by this function.
 *  pScreen     Screen on which to place the new frame
 *  newX, newY  Position of the frame.
 *  pNewShape   Shape for the frame (in frame-local coordinates). NULL for
 *              unshaped frames.
 */
typedef Bool (*RootlessCreateFrameProc)
    (RootlessWindowPtr pFrame, ScreenPtr pScreen, int newX, int newY,
     RegionPtr pNewShape);

/*
 * Destroy a frame.
 *  Drawing is stopped and all updates are flushed before this is called.
 *
 *  wid         Frame id
 */
typedef void (*RootlessDestroyFrameProc)
    (RootlessFrameID wid);

/*
 * Move a frame on screen.
 *  Drawing is stopped and all updates are flushed before this is called.
 *
 *  wid         Frame id
 *  pScreen     Screen to move the new frame to
 *  newX, newY  New position of the frame
 */
typedef void (*RootlessMoveFrameProc) 
    (RootlessFrameID wid, ScreenPtr pScreen, int newX, int newY);

/*
 * Resize and move a frame.
 *  Drawing is stopped and all updates are flushed before this is called.
 *
 *  wid         Frame id
 *  pScreen     Screen to move the new frame to
 *  newX, newY  New position of the frame
 *  newW, newH  New size of the frame
 *  gravity     Gravity for window contents (rl_gravity_enum). This is always
 *              RL_GRAVITY_NONE unless ROOTLESS_RESIZE_GRAVITY is set.
 */
typedef void (*RootlessResizeFrameProc)
    (RootlessFrameID wid, ScreenPtr pScreen,
     int newX, int newY, unsigned int newW, unsigned int newH,
     unsigned int gravity);

/*
 * Change frame ordering (AKA stacking, layering).
 *  Drawing is stopped before this is called. Unmapped frames are mapped by
 *  setting their ordering.
 *
 *  wid         Frame id
 *  nextWid     Frame id of frame that is now above this one or NULL if this
 *              frame is at the top.
 */
typedef void (*RootlessRestackFrameProc)
    (RootlessFrameID wid, RootlessFrameID nextWid);

/*
 * Change frame's shape.
 *  Drawing is stopped before this is called.
 *
 *  wid         Frame id
 *  pNewShape   New shape for the frame (in frame-local coordinates)
 *              or NULL if now unshaped.
 */
typedef void (*RootlessReshapeFrameProc)
    (RootlessFrameID wid, RegionPtr pNewShape);

/*
 * Unmap a frame.
 *
 *  wid         Frame id
 */
typedef void (*RootlessUnmapFrameProc)
    (RootlessFrameID wid);

/*
 * Start drawing to a frame.
 *  Prepare a frame for direct access to its backing buffer.
 *
 *  wid         Frame id
 *  pixelData   Address of the backing buffer (returned)
 *  bytesPerRow Width in bytes of the backing buffer (returned)
 */
typedef void (*RootlessStartDrawingProc)
    (RootlessFrameID wid, char **pixelData, int *bytesPerRow);

/*
 * Stop drawing to a frame.
 *  No drawing to the frame's backing buffer will occur until drawing
 *  is started again.
 *
 *  wid         Frame id
 *  flush       Flush drawing updates for this frame to the screen.
 */
typedef void (*RootlessStopDrawingProc)
    (RootlessFrameID wid, Bool flush);

/*
 * Flush drawing updates to the screen.
 *  Drawing is stopped before this is called.
 *
 *  wid         Frame id
 *  pDamage     Region containing all the changed pixels in frame-local
 *              coordinates. This is clipped to the window's clip.
 */
typedef void (*RootlessUpdateRegionProc)
    (RootlessFrameID wid, RegionPtr pDamage);

/*
 * Mark damaged rectangles as requiring redisplay to screen.
 *
 *  wid         Frame id
 *  nrects      Number of damaged rectangles
 *  rects       Array of damaged rectangles in frame-local coordinates
 *  shift_x,    Vector to shift rectangles by
 *   shift_y
 */
typedef void (*RootlessDamageRectsProc)
    (RootlessFrameID wid, int nrects, const BoxRec *rects,
     int shift_x, int shift_y);

/*
 * Switch the window associated with a frame. (Optional)
 *  When a framed window is reparented, the frame is resized and set to
 *  use the new top-level parent. If defined this function will be called
 *  afterwards for implementation specific bookkeeping.
 *
 *  pFrame      Frame whose window has switched
 *  oldWin      Previous window wrapped by this frame
 */
typedef void (*RootlessSwitchWindowProc)
    (RootlessWindowPtr pFrame, WindowPtr oldWin);

/*
 * Copy bytes. (Optional)
 *  Source and destinate may overlap and the right thing should happen.
 *
 *  width       Bytes to copy per row
 *  height      Number of rows
 *  src         Source data
 *  srcRowBytes Width of source in bytes
 *  dst         Destination data
 *  dstRowBytes Width of destination in bytes
 */
typedef void (*RootlessCopyBytesProc)
    (unsigned int width, unsigned int height,
     const void *src, unsigned int srcRowBytes,
     void *dst, unsigned int dstRowBytes);

/*
 * Copy area in frame to another part of frame. (Optional)
 *
 *  wid         Frame id
 *  dstNrects   Number of rectangles to copy
 *  dstRects    Array of rectangles to copy
 *  dx, dy      Number of pixels away to copy area
 */
typedef void (*RootlessCopyWindowProc)
    (RootlessFrameID wid, int dstNrects, const BoxRec *dstRects,
     int dx, int dy);

