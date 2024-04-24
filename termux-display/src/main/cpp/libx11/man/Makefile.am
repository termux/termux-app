SUBDIRS = xkb

libmandir = $(LIB_MAN_DIR)

libman_PRE = \
	$(all_shadows:=.man) \
	$(file_shadows:=.man) \
	AllPlanes.man \
	BlackPixelOfScreen.man \
	DisplayOfCCC.man \
	ImageByteOrder.man \
	IsCursorKey.man \
	XAddConnectionWatch.man \
	XAddHost.man \
	XAllocClassHint.man \
	XAllocColor.man \
	XAllocIconSize.man \
	XAllocSizeHints.man \
	XAllocStandardColormap.man \
	XAllocWMHints.man \
	XAllowEvents.man \
	XAnyEvent.man \
	XButtonEvent.man \
	XChangeKeyboardControl.man \
	XChangeKeyboardMapping.man \
	XChangePointerControl.man \
	XChangeSaveSet.man \
	XChangeWindowAttributes.man \
	XCirculateEvent.man \
	XCirculateRequestEvent.man \
	XClearArea.man \
	XClientMessageEvent.man \
	XcmsAllocColor.man \
	XcmsCCCOfColormap.man \
	XcmsCIELabQueryMaxC.man \
	XcmsCIELuvQueryMaxC.man \
	XcmsColor.man \
	XcmsConvertColors.man \
	XcmsCreateCCC.man \
	XcmsDefaultCCC.man \
	XcmsQueryBlack.man \
	XcmsQueryColor.man \
	XcmsSetWhitePoint.man \
	XcmsStoreColor.man \
	XcmsTekHVCQueryMaxC.man \
	XColormapEvent.man \
	XConfigureEvent.man \
	XConfigureRequestEvent.man \
	XConfigureWindow.man \
	XCopyArea.man \
	XCreateColormap.man \
	XCreateFontCursor.man \
	XCreateFontSet.man \
	XCreateGC.man \
	XCreateIC.man \
	XCreateOC.man \
	XCreatePixmap.man \
	XCreateRegion.man \
	XCreateWindowEvent.man \
	XCreateWindow.man \
	XCrossingEvent.man \
	XDefineCursor.man \
	XDestroyWindowEvent.man \
	XDestroyWindow.man \
	XDrawArc.man \
	XDrawImageString.man \
	XDrawLine.man \
	XDrawPoint.man \
	XDrawRectangle.man \
	XDrawString.man \
	XDrawText.man \
	XEmptyRegion.man \
	XErrorEvent.man \
	XExposeEvent.man \
	XExtentsOfFontSet.man \
	XFillRectangle.man \
	XFilterEvent.man \
	XFlush.man \
	XFocusChangeEvent.man \
	XFontSetExtents.man \
	XFontsOfFontSet.man \
	XFree.man \
	XGetEventData.man \
	XGetVisualInfo.man \
	XGetWindowAttributes.man \
	XGetWindowProperty.man \
	XGetXCBConnection.man \
	XGrabButton.man \
	XGrabKeyboard.man \
	XGrabKey.man \
	XGrabPointer.man \
	XGrabServer.man \
	XGraphicsExposeEvent.man \
	XGravityEvent.man \
	XIconifyWindow.man \
	XIfEvent.man \
	XInitImage.man \
	XInitThreads.man \
	XInstallColormap.man \
	XInternAtom.man \
	XIntersectRegion.man \
	XKeymapEvent.man \
	XListFonts.man \
	XLoadFont.man \
	XLookupKeysym.man \
	XMapEvent.man \
	XMapRequestEvent.man \
	XMapWindow.man \
	XmbDrawImageString.man \
	XmbDrawString.man \
	XmbDrawText.man \
	XmbLookupString.man \
	XmbResetIC.man \
	XmbTextEscapement.man \
	XmbTextExtents.man \
	XmbTextListToTextProperty.man \
	XmbTextPerCharExtents.man \
	XNextEvent.man \
	XNoOp.man \
	XOpenDisplay.man \
	XOpenIM.man \
	XOpenOM.man \
	XParseGeometry.man \
	XPolygonRegion.man \
	XPropertyEvent.man \
	XPutBackEvent.man \
	XPutImage.man \
	XQueryBestSize.man \
	XQueryColor.man \
	XQueryExtension.man \
	XQueryPointer.man \
	XQueryTree.man \
	XRaiseWindow.man \
	XReadBitmapFile.man \
	XRecolorCursor.man \
	XReparentEvent.man \
	XReparentWindow.man \
	XResizeRequestEvent.man \
	XResourceManagerString.man \
	XrmEnumerateDatabase.man \
	XrmGetFileDatabase.man \
	XrmGetResource.man \
	XrmInitialize.man \
	XrmMergeDatabases.man \
	XrmPutResource.man \
	XrmUniqueQuark.man \
	XSaveContext.man \
	XSelectInput.man \
	XSelectionClearEvent.man \
	XSelectionEvent.man \
	XSelectionRequestEvent.man \
	XSendEvent.man \
	XSetArcMode.man \
	XSetClipOrigin.man \
	XSetCloseDownMode.man \
	XSetCommand.man \
	XSetErrorHandler.man \
	XSetEventQueueOwner.man \
	XSetFillStyle.man \
	XSetFont.man \
	XSetFontPath.man \
	XSetICFocus.man \
	XSetICValues.man \
	XSetInputFocus.man \
	XSetLineAttributes.man \
	XSetPointerMapping.man \
	XSetScreenSaver.man \
	XSetSelectionOwner.man \
	XSetState.man \
	XSetTextProperty.man \
	XSetTile.man \
	XSetTransientForHint.man \
	XSetWMClientMachine.man \
	XSetWMColormapWindows.man \
	XSetWMIconName.man \
	XSetWMName.man \
	XSetWMProperties.man \
	XSetWMProtocols.man \
	XStoreBytes.man \
	XStoreColors.man \
	XStringListToTextProperty.man \
	XStringToKeysym.man \
	XSupportsLocale.man \
	XSynchronize.man \
	XTextExtents.man \
	XTextWidth.man \
	XTranslateCoordinates.man \
	XUnmapEvent.man \
	XUnmapWindow.man \
	XVaCreateNestedList.man \
	XVisibilityEvent.man \
	XWarpPointer.man

filemandir = $(FILE_MAN_DIR)
fileman_PRE = Compose.man

libman_DATA = $(libman_PRE:man=@LIB_MAN_SUFFIX@) \
	      $(all_shadows:=.@LIB_MAN_SUFFIX@)

fileman_DATA = $(fileman_PRE:man=@FILE_MAN_SUFFIX@) \
	       $(file_shadows:=.@FILE_MAN_SUFFIX@)

EXTRA_DIST = $(libman_PRE) $(fileman_PRE)

CLEANFILES = $(libman_DATA) $(fileman_DATA)

SUFFIXES = .$(LIB_MAN_SUFFIX) .$(FILE_MAN_SUFFIX) .man

MAN_SUBSTS += -e 's|__xlocaledir__|$(X11_LOCALEDATADIR)|g'

# String replacements in MAN_SUBSTS now come from xorg-macros.m4 via configure
.man.$(LIB_MAN_SUFFIX) .man.$(FILE_MAN_SUFFIX):
	$(AM_V_GEN)$(SED) $(MAN_SUBSTS) < $< > $@

# Generate man page shadow files (Replaces InstallManPageAliases from Imake)
all_shadows =                                \
    $(AllPlanes_shadows)                     \
    $(BlackPixelOfScreen_shadows)            \
    $(DisplayOfCCC_shadows)                  \
    $(ImageByteOrder_shadows)                \
    $(IsCursorKey_shadows)                   \
    $(XAllocClassHint_shadows)               \
    $(XAllocIconSize_shadows)                \
    $(XAllocStandardColormap_shadows)        \
    $(XAllocSizeHints_shadows)               \
    $(XAllocWMHints_shadows)                 \
    $(XAddHost_shadows)                      \
    $(XAllocColor_shadows)                   \
    $(XAnyEvent_shadows)                     \
    $(XButtonEvent_shadows)                  \
    $(XChangeKeyboardControl_shadows)        \
    $(XChangeKeyboardMapping_shadows)        \
    $(XChangePointerControl_shadows)         \
    $(XChangeSaveSet_shadows)                \
    $(XChangeWindowAttributes_shadows)       \
    $(XClearArea_shadows)                    \
    $(XConfigureWindow_shadows)              \
    $(XCopyArea_shadows)                     \
    $(XCreateColormap_shadows)               \
    $(XCreateFontCursor_shadows)             \
    $(XCreateFontSet_shadows)                \
    $(XCreateGC_shadows)                     \
    $(XCreateIC_shadows)                     \
    $(XInitImage_shadows)                    \
    $(XCreateOC_shadows)                     \
    $(XCreatePixmap_shadows)                 \
    $(XCreateRegion_shadows)                 \
    $(XCreateWindow_shadows)                 \
    $(XDefineCursor_shadows)                 \
    $(XDestroyWindow_shadows)                \
    $(XDrawArc_shadows)                      \
    $(XDrawImageString_shadows)              \
    $(XDrawLine_shadows)                     \
    $(XDrawPoint_shadows)                    \
    $(XDrawRectangle_shadows)                \
    $(XDrawString_shadows)                   \
    $(XDrawText_shadows)                     \
    $(XEmptyRegion_shadows)                  \
    $(XFillRectangle_shadows)                \
    $(XFlush_shadows)                        \
    $(XFontsOfFontSet_shadows)               \
    $(XGraphicsExposeEvent_shadows)          \
    $(XrmGetFileDatabase_shadows)            \
    $(XrmGetResource_shadows)                \
    $(XGetEventData_shadows)                 \
    $(XGetVisualInfo_shadows)                \
    $(XGetWindowAttributes_shadows)          \
    $(XGetWindowProperty_shadows)            \
    $(XGrabButton_shadows)                   \
    $(XGrabKey_shadows)                      \
    $(XGrabKeyboard_shadows)                 \
    $(XGrabPointer_shadows)                  \
    $(XGrabServer_shadows)                   \
    $(XIconifyWindow_shadows)                \
    $(XIfEvent_shadows)                      \
    $(XrmInitialize_shadows)                 \
    $(XInstallColormap_shadows)              \
    $(XAddConnectionWatch_shadows)           \
    $(XIntersectRegion_shadows)              \
    $(XInternAtom_shadows)                   \
    $(XListFonts_shadows)                    \
    $(XLoadFont_shadows)                     \
    $(XLookupKeysym_shadows)                 \
    $(XrmMergeDatabases_shadows)             \
    $(XMapEvent_shadows)                     \
    $(XMapWindow_shadows)                    \
    $(XNextEvent_shadows)                    \
    $(XOpenDisplay_shadows)                  \
    $(XOpenIM_shadows)                       \
    $(XOpenOM_shadows)                       \
    $(XParseGeometry_shadows)                \
    $(XPolygonRegion_shadows)                \
    $(XPutImage_shadows)                     \
    $(XrmPutResource_shadows)                \
    $(XQueryBestSize_shadows)                \
    $(XQueryColor_shadows)                   \
    $(XQueryExtension_shadows)               \
    $(XResourceManagerString_shadows)        \
    $(XRaiseWindow_shadows)                  \
    $(XReadBitmapFile_shadows)               \
    $(XRecolorCursor_shadows)                \
    $(XSaveContext_shadows)                  \
    $(XSetICFocus_shadows)                   \
    $(XSetICValues_shadows)                  \
    $(XStringListToTextProperty_shadows)     \
    $(XSetArcMode_shadows)                   \
    $(XSetClipOrigin_shadows)                \
    $(XSetCloseDownMode_shadows)             \
    $(XSetCommand_shadows)                   \
    $(XSetErrorHandler_shadows)              \
    $(XSendEvent_shadows)                    \
    $(XSetFillStyle_shadows)                 \
    $(XSetFontPath_shadows)                  \
    $(XSetInputFocus_shadows)                \
    $(XSetLineAttributes_shadows)            \
    $(XSetPointerMapping_shadows)            \
    $(XSetScreenSaver_shadows)               \
    $(XSetSelectionOwner_shadows)            \
    $(XSetState_shadows)                     \
    $(XSetTransientForHint_shadows)          \
    $(XSetTextProperty_shadows)              \
    $(XSetTile_shadows)                      \
    $(XSetWMClientMachine_shadows)           \
    $(XSetWMColormapWindows_shadows)         \
    $(XSetWMIconName_shadows)                \
    $(XSetWMName_shadows)                    \
    $(XSetWMProperties_shadows)              \
    $(XSetWMProtocols_shadows)               \
    $(XStoreBytes_shadows)                   \
    $(XStoreColors_shadows)                  \
    $(XStringToKeysym_shadows)               \
    $(XSupportsLocale_shadows)               \
    $(XSynchronize_shadows)                  \
    $(XmbTextListToTextProperty_shadows)     \
    $(XTextExtents_shadows)                  \
    $(XTextWidth_shadows)                    \
    $(XInitThreads_shadows)                  \
    $(XrmUniqueQuark_shadows)                \
    $(XUnmapWindow_shadows)                  \
    $(XcmsCCCOfColormap_shadows)             \
    $(XcmsAllocColor_shadows)                \
    $(XcmsColor_shadows)                     \
    $(XcmsCreateCCC_shadows)                 \
    $(XcmsCIELabQueryMaxC_shadows)           \
    $(XcmsCIELuvQueryMaxC_shadows)           \
    $(XcmsQueryBlack_shadows)                \
    $(XcmsQueryColor_shadows)                \
    $(XcmsStoreColor_shadows)                \
    $(XcmsSetWhitePoint_shadows)             \
    $(XcmsTekHVCQueryMaxC_shadows)           \
    $(XmbDrawImageString_shadows)            \
    $(XmbDrawString_shadows)                 \
    $(XmbDrawText_shadows)                   \
    $(XmbLookupString_shadows)               \
    $(XmbResetIC_shadows)                    \
    $(XmbTextEscapement_shadows)             \
    $(XmbTextExtents_shadows)                \
    $(XmbTextPerCharExtents_shadows)


AllPlanes_shadows =                         \
    BlackPixel                              \
    WhitePixel                              \
    ConnectionNumber                        \
    DefaultColormap                         \
    DefaultDepth                            \
    XListDepths                             \
    DefaultGC                               \
    DefaultRootWindow                       \
    DefaultScreenOfDisplay                  \
    DefaultScreen                           \
    DefaultVisual                           \
    DisplayCells                            \
    DisplayPlanes                           \
    DisplayString                           \
    XMaxRequestSize                         \
    XExtendedMaxRequestSize                 \
    LastKnownRequestProcessed               \
    NextRequest                             \
    ProtocolVersion                         \
    ProtocolRevision                        \
    QLength                                 \
    RootWindow                              \
    ScreenCount                             \
    ScreenOfDisplay                         \
    ServerVendor                            \
    VendorRelease

BlackPixelOfScreen_shadows =                \
    WhitePixelOfScreen                      \
    CellsOfScreen                           \
    DefaultColormapOfScreen                 \
    DefaultDepthOfScreen                    \
    DefaultGCOfScreen                       \
    DefaultVisualOfScreen                   \
    DoesBackingStore                        \
    DoesSaveUnders                          \
    DisplayOfScreen                         \
    XScreenNumberOfScreen                   \
    EventMaskOfScreen                       \
    HeightOfScreen                          \
    HeightMMOfScreen                        \
    MaxCmapsOfScreen                        \
    MinCmapsOfScreen                        \
    PlanesOfScreen                          \
    RootWindowOfScreen                      \
    WidthOfScreen                           \
    WidthMMOfScreen

DisplayOfCCC_shadows =                      \
    VisualOfCCC                             \
    ScreenNumberOfCCC                       \
    ScreenWhitePointOfCCC                   \
    ClientWhitePointOfCCC

ImageByteOrder_shadows =                    \
    BitmapBitOrder                          \
    BitmapPad                               \
    BitmapUnit                              \
    DisplayHeight                           \
    DisplayHeightMM                         \
    DisplayWidth                            \
    DisplayWidthMM                          \
    XListPixmapFormats                      \
    XPixmapFormatValues

IsCursorKey_shadows =                       \
    IsFunctionKey                           \
    IsKeypadKey                             \
    IsMiscFunctionKey                       \
    IsModifierKey                           \
    IsPFKey                                 \
    IsPrivateKeypadKey

XAllocClassHint_shadows =                   \
    XSetClassHint                           \
    XGetClassHint                           \
    XClassHint

XAllocIconSize_shadows =                    \
    XSetIconSizes                           \
    XGetIconSizes                           \
    XIconSize

XAllocStandardColormap_shadows =            \
    XSetRGBColormaps                        \
    XGetRGBColormaps                        \
    XStandardColormap

XAllocSizeHints_shadows =                   \
    XSetWMNormalHints                       \
    XGetWMNormalHints                       \
    XSetWMSizeHints                         \
    XGetWMSizeHints                         \
    XSizeHints

XAllocWMHints_shadows =                     \
    XSetWMHints                             \
    XGetWMHints                             \
    XWMHints

XAddHost_shadows =                          \
    XAddHosts                               \
    XListHosts                              \
    XRemoveHost                             \
    XRemoveHosts                            \
    XSetAccessControl                       \
    XEnableAccessControl                    \
    XDisableAccessControl                   \
    XHostAddress

XAllocColor_shadows =                       \
    XAllocNamedColor                        \
    XAllocColorCells                        \
    XAllocColorPlanes                       \
    XFreeColors

XAnyEvent_shadows =                         \
    XEvent

XButtonEvent_shadows =                      \
    XKeyEvent                               \
    XMotionEvent

XChangeKeyboardControl_shadows =            \
    XGetKeyboardControl                     \
    XAutoRepeatOn                           \
    XAutoRepeatOff                          \
    XBell                                   \
    XQueryKeymap                            \
    XKeyboardControl

XChangeKeyboardMapping_shadows =            \
    XGetKeyboardMapping                     \
    XDisplayKeycodes                        \
    XSetModifierMapping                     \
    XGetModifierMapping                     \
    XNewModifiermap                         \
    XInsertModifiermapEntry                 \
    XDeleteModifiermapEntry                 \
    XFreeModifiermap                        \
    XModifierKeymap

XChangePointerControl_shadows =             \
    XGetPointerControl

XChangeSaveSet_shadows =                    \
    XAddToSaveSet                           \
    XRemoveFromSaveSet

XChangeWindowAttributes_shadows =           \
    XSetWindowBackground                    \
    XSetWindowBackgroundPixmap              \
    XSetWindowBorder                        \
    XSetWindowBorderPixmap                  \
    XSetWindowColormap

XClearArea_shadows =                        \
    XClearWindow

XConfigureWindow_shadows =                  \
    XMoveWindow                             \
    XResizeWindow                           \
    XMoveResizeWindow                       \
    XSetWindowBorderWidth                   \
    XWindowChanges

XCopyArea_shadows =                         \
    XCopyPlane

XCreateColormap_shadows =                   \
    XCopyColormapAndFree                    \
    XFreeColormap                           \
    XColor

XCreateFontCursor_shadows =                 \
    XCreatePixmapCursor                     \
    XCreateGlyphCursor

XCreateFontSet_shadows =                    \
    XFreeFontSet

XCreateGC_shadows =                         \
    XCopyGC                                 \
    XChangeGC                               \
    XGetGCValues                            \
    XFreeGC                                 \
    XGContextFromGC                         \
    XGCValues

XCreateIC_shadows =                         \
    XDestroyIC                              \
    XIMOfIC

XInitImage_shadows =                        \
    XCreateImage                            \
    XGetPixel                               \
    XPutPixel                               \
    XSubImage                               \
    XAddPixel                               \
    XDestroyImage

XCreateOC_shadows =                         \
    XDestroyOC                              \
    XSetOCValues                            \
    XGetOCValues                            \
    XOMOfOC

XCreatePixmap_shadows =                     \
    XFreePixmap

XCreateRegion_shadows =                     \
    XSetRegion                              \
    XDestroyRegion

XCreateWindow_shadows =                     \
    XCreateSimpleWindow                     \
    XSetWindowAttributes

XDefineCursor_shadows =                     \
    XUndefineCursor

XDestroyWindow_shadows =                    \
    XDestroySubwindows

XDrawArc_shadows =                          \
    XDrawArcs                               \
    XArc

XDrawImageString_shadows =                  \
    XDrawImageString16

XDrawLine_shadows =                         \
    XDrawLines                              \
    XDrawSegments                           \
    XSegment

XDrawPoint_shadows =                        \
    XDrawPoints                             \
    XPoint

XDrawRectangle_shadows =                    \
    XDrawRectangles                         \
    XRectangle

XDrawString_shadows =                       \
    XDrawString16

XDrawText_shadows =                         \
    XDrawText16                             \
    XTextItem                               \
    XTextItem16

XEmptyRegion_shadows =                      \
    XEqualRegion                            \
    XPointInRegion                          \
    XRectInRegion

XFillRectangle_shadows =                    \
    XFillRectangles                         \
    XFillPolygon                            \
    XFillArc                                \
    XFillArcs

XFlush_shadows =                            \
    XSync                                   \
    XEventsQueued                           \
    XPending

XFontsOfFontSet_shadows =                   \
    XBaseFontNameListOfFontSet              \
    XLocaleOfFontSet                        \
    XContextDependentDrawing                \
    XContextualDrawing                      \
    XDirectionalDependentDrawing

XGraphicsExposeEvent_shadows =              \
    XNoExposeEvent

XrmGetFileDatabase_shadows =                \
    XrmPutFileDatabase                      \
    XrmGetStringDatabase                    \
    XrmLocaleOfDatabase                     \
    XrmGetDatabase                          \
    XrmSetDatabase                          \
    XrmDestroyDatabase

XrmGetResource_shadows =                    \
    XrmQGetResource                         \
    XrmQGetSearchList                       \
    XrmQGetSearchResource

XGetEventData_shadows =                     \
    XFreeEventData                          \
    XGenericEventCookie

XGetVisualInfo_shadows =                    \
    XMatchVisualInfo                        \
    XVisualIDFromVisual                     \
    XVisualInfo

XGetWindowAttributes_shadows =              \
    XGetGeometry                            \
    XWindowAttributes

XGetWindowProperty_shadows =                \
    XListProperties                         \
    XChangeProperty                         \
    XRotateWindowProperties                 \
    XDeleteProperty

XGrabButton_shadows =                       \
    XUngrabButton

XGrabKey_shadows =                          \
    XUngrabKey

XGrabKeyboard_shadows =                     \
    XUngrabKeyboard

XGrabPointer_shadows =                      \
    XUngrabPointer                          \
    XChangeActivePointerGrab

XGrabServer_shadows =                       \
    XUngrabServer

XIconifyWindow_shadows =                    \
    XWithdrawWindow                         \
    XReconfigureWMWindow

XIfEvent_shadows =                          \
    XCheckIfEvent                           \
    XPeekIfEvent

XrmInitialize_shadows =                     \
    XrmParseCommand                         \
    XrmValue                                \
    XrmOptionKind                           \
    XrmOptionDescRec

XInstallColormap_shadows =                  \
    XUninstallColormap                      \
    XListInstalledColormaps

XAddConnectionWatch_shadows =               \
    XRemoveConnectionWatch                  \
    XProcessInternalConnection              \
    XInternalConnectionNumbers

XIntersectRegion_shadows =                  \
    XUnionRegion                            \
    XUnionRectWithRegion                    \
    XSubtractRegion                         \
    XXorRegion                              \
    XOffsetRegion                           \
    XShrinkRegion

XInternAtom_shadows =                       \
    XInternAtoms                            \
    XGetAtomName                            \
    XGetAtomNames

XListFonts_shadows =                        \
    XFreeFontNames                          \
    XListFontsWithInfo                      \
    XFreeFontInfo

XLoadFont_shadows =                         \
    XQueryFont                              \
    XLoadQueryFont                          \
    XFreeFont                               \
    XGetFontProperty                        \
    XUnloadFont                             \
    XCharStruct                             \
    XFontProp                               \
    XChar2b                                 \
    XFontStruct

XLookupKeysym_shadows =                     \
    XRefreshKeyboardMapping                 \
    XLookupString                           \
    XRebindKeysym

XrmMergeDatabases_shadows =                 \
    XrmCombineDatabase                      \
    XrmCombineFileDatabase

XMapEvent_shadows =                         \
    XMappingEvent

XMapWindow_shadows =                        \
    XMapRaised                              \
    XMapSubwindows

XNextEvent_shadows =                        \
    XPeekEvent                              \
    XWindowEvent                            \
    XCheckWindowEvent                       \
    XMaskEvent                              \
    XCheckMaskEvent                         \
    XCheckTypedEvent                        \
    XCheckTypedWindowEvent

XOpenDisplay_shadows =                      \
    XCloseDisplay

XOpenIM_shadows =                           \
    XCloseIM                                \
    XSetIMValues                            \
    XGetIMValues                            \
    XDisplayOfIM                            \
    XLocaleOfIM                             \
    XRegisterIMInstantiateCallback          \
    XUnregisterIMInstantiateCallback

XOpenOM_shadows =                           \
    XCloseOM                                \
    XSetOMValues                            \
    XGetOMValues                            \
    XDisplayOfOM                            \
    XLocaleOfOM

XParseGeometry_shadows =                    \
    XWMGeometry

XPolygonRegion_shadows =                    \
    XClipBox

XPutImage_shadows =                         \
    XGetImage                               \
    XGetSubImage

XrmPutResource_shadows =                    \
    XrmQPutResource                         \
    XrmPutStringResource                    \
    XrmQPutStringResource                   \
    XrmPutLineResource

XQueryBestSize_shadows =                    \
    XQueryBestTile                          \
    XQueryBestStipple

XQueryColor_shadows =                       \
    XQueryColors                            \
    XLookupColor                            \
    XParseColor

XQueryExtension_shadows =                   \
    XListExtensions                         \
    XFreeExtensionList

XResourceManagerString_shadows =            \
    XScreenResourceString

XRaiseWindow_shadows =                      \
    XLowerWindow                            \
    XCirculateSubwindows                    \
    XCirculateSubwindowsUp                  \
    XCirculateSubwindowsDown                \
    XRestackWindows

XReadBitmapFile_shadows =                   \
    XReadBitmapFileData                     \
    XWriteBitmapFile                        \
    XCreatePixmapFromBitmapData             \
    XCreateBitmapFromData

XRecolorCursor_shadows =                    \
    XFreeCursor                             \
    XQueryBestCursor

XSaveContext_shadows =                      \
    XFindContext                            \
    XDeleteContext                          \
    XUniqueContext

XSetICFocus_shadows =                       \
    XUnsetICFocus

XSetICValues_shadows =                      \
    XGetICValues

XStringListToTextProperty_shadows =         \
    XTextPropertyToStringList               \
    XFreeStringList                         \
    XTextProperty

XSetArcMode_shadows =                       \
    XSetSubwindowMode                       \
    XSetGraphicsExposure

XSetClipOrigin_shadows =                    \
    XSetClipMask                            \
    XSetClipRectangles

XSetCloseDownMode_shadows =                 \
    XKillClient

XSetCommand_shadows =                       \
    XGetCommand

XSetErrorHandler_shadows =                  \
    XGetErrorText                           \
    XDisplayName                            \
    XSetIOErrorHandler                      \
    XGetErrorDatabaseText

XSendEvent_shadows =                        \
    XDisplayMotionBufferSize                \
    XGetMotionEvents                        \
    XTimeCoord

XSetFillStyle_shadows =                     \
    XSetFillRule

XSetFontPath_shadows =                      \
    XGetFontPath                            \
    XFreeFontPath

XSetInputFocus_shadows =                    \
    XGetInputFocus

XSetLineAttributes_shadows =                \
    XSetDashes

XSetPointerMapping_shadows =                \
    XGetPointerMapping

XSetScreenSaver_shadows =                   \
    XForceScreenSaver                       \
    XActivateScreenSaver                    \
    XResetScreenSaver                       \
    XGetScreenSaver

XSetSelectionOwner_shadows =                \
    XGetSelectionOwner                      \
    XConvertSelection

XSetState_shadows =                         \
    XSetFunction                            \
    XSetPlaneMask                           \
    XSetForeground                          \
    XSetBackground

XSetTransientForHint_shadows =              \
    XGetTransientForHint

XSetTextProperty_shadows =                  \
    XGetTextProperty

XSetTile_shadows =                          \
    XSetStipple                             \
    XSetTSOrigin

XSetWMClientMachine_shadows =               \
    XGetWMClientMachine

XSetWMColormapWindows_shadows =             \
    XGetWMColormapWindows

XSetWMIconName_shadows =                    \
    XGetWMIconName                          \
    XSetIconName                            \
    XGetIconName

XSetWMName_shadows =                        \
    XGetWMName                              \
    XStoreName                              \
    XFetchName

XSetWMProperties_shadows =                  \
    XmbSetWMProperties                      \
    Xutf8SetWMProperties

XSetWMProtocols_shadows =                   \
    XGetWMProtocols

XStoreBytes_shadows =                       \
    XStoreBuffer                            \
    XFetchBytes                             \
    XFetchBuffer                            \
    XRotateBuffers

XStoreColors_shadows =                      \
    XStoreColor                             \
    XStoreNamedColor

XStringToKeysym_shadows =                   \
    XKeysymToString                         \
    XKeycodeToKeysym                        \
    XKeysymToKeycode                        \
    XConvertCase

XSupportsLocale_shadows =                   \
    XSetLocaleModifiers

XSynchronize_shadows =                      \
    XSetAfterFunction

XmbTextListToTextProperty_shadows =         \
    XwcTextListToTextProperty               \
    Xutf8TextListToTextProperty             \
    XmbTextPropertyToTextList               \
    XwcTextPropertyToTextList               \
    Xutf8TextPropertyToTextList             \
    XwcFreeStringList                       \
    XDefaultString

XTextExtents_shadows =                      \
    XTextExtents16                          \
    XQueryTextExtents                       \
    XQueryTextExtents16

XTextWidth_shadows =                        \
    XTextWidth16

XInitThreads_shadows =                      \
    XLockDisplay                            \
    XUnlockDisplay

XrmUniqueQuark_shadows =                    \
    XrmStringToQuark                        \
    XrmPermStringToQuark                    \
    XrmQuarkToString                        \
    XrmStringToQuarkList                    \
    XrmStringToBindingQuarkList

XUnmapWindow_shadows =                      \
    XUnmapSubwindows

XcmsCCCOfColormap_shadows =                 \
    XcmsSetCCCOfColormap

XcmsAllocColor_shadows =                    \
    XcmsAllocNamedColor

XcmsColor_shadows =                         \
    XcmsRGB                                 \
    XcmsRGBi                                \
    XcmsCIEXYZ                              \
    XcmsCIEuvY                              \
    XcmsCIExyY                              \
    XcmsCIELab                              \
    XcmsCIELuv                              \
    XcmsTekHVC                              \
    XcmsPad

XcmsCreateCCC_shadows =                     \
    XcmsFreeCCC

XcmsCIELabQueryMaxC_shadows =               \
    XcmsCIELabQueryMaxL                     \
    XcmsCIELabQueryMaxLC                    \
    XcmsCIELabQueryMinL

XcmsCIELuvQueryMaxC_shadows =               \
    XcmsCIELuvQueryMaxL                     \
    XcmsCIELuvQueryMaxLC                    \
    XcmsCIELuvQueryMinL

XcmsQueryBlack_shadows =                    \
    XcmsQueryBlue                           \
    XcmsQueryGreen                          \
    XcmsQueryRed                            \
    XcmsQueryWhite

XcmsQueryColor_shadows =                    \
    XcmsQueryColors                         \
    XcmsLookupColor

XcmsStoreColor_shadows =                    \
    XcmsStoreColors

XcmsSetWhitePoint_shadows =                 \
    XcmsSetWhiteAdjustProc

XcmsTekHVCQueryMaxC_shadows =               \
    XcmsTekHVCQueryMaxV                     \
    XcmsTekHVCQueryMaxVC                    \
    XcmsTekHVCQueryMaxVSamples              \
    XcmsTekHVCQueryMinV

XmbDrawImageString_shadows =                \
    XwcDrawImageString                      \
    Xutf8DrawImageString

XmbDrawString_shadows =                     \
    XwcDrawString                           \
    Xutf8DrawString

XmbDrawText_shadows =                       \
    XwcDrawText                             \
    Xutf8DrawText

XmbLookupString_shadows =                   \
    XwcLookupString                         \
    Xutf8LookupString

XmbResetIC_shadows =                        \
    XwcResetIC                              \
    Xutf8ResetIC

XmbTextEscapement_shadows =                 \
    XwcTextEscapement                       \
    Xutf8TextEscapement

XmbTextExtents_shadows =                    \
    XwcTextExtents                          \
    Xutf8TextExtents

XmbTextPerCharExtents_shadows =             \
    XwcTextPerCharExtents                   \
    Xutf8TextPerCharExtents

file_shadows =			            \
    $(Compose_shadows)

Compose_shadows =			    \
    XCompose

