
/*
 *	Server dispatcher function replacements
 */

extern int PanoramiXCreateWindow(ClientPtr client);
extern int PanoramiXChangeWindowAttributes(ClientPtr client);
extern int PanoramiXDestroyWindow(ClientPtr client);
extern int PanoramiXDestroySubwindows(ClientPtr client);
extern int PanoramiXChangeSaveSet(ClientPtr client);
extern int PanoramiXReparentWindow(ClientPtr client);
extern int PanoramiXMapWindow(ClientPtr client);
extern int PanoramiXMapSubwindows(ClientPtr client);
extern int PanoramiXUnmapWindow(ClientPtr client);
extern int PanoramiXUnmapSubwindows(ClientPtr client);
extern int PanoramiXConfigureWindow(ClientPtr client);
extern int PanoramiXCirculateWindow(ClientPtr client);
extern int PanoramiXGetGeometry(ClientPtr client);
extern int PanoramiXTranslateCoords(ClientPtr client);
extern int PanoramiXCreatePixmap(ClientPtr client);
extern int PanoramiXFreePixmap(ClientPtr client);
extern int PanoramiXChangeGC(ClientPtr client);
extern int PanoramiXCopyGC(ClientPtr client);
extern int PanoramiXCopyColormapAndFree(ClientPtr client);
extern int PanoramiXCreateGC(ClientPtr client);
extern int PanoramiXSetDashes(ClientPtr client);
extern int PanoramiXSetClipRectangles(ClientPtr client);
extern int PanoramiXFreeGC(ClientPtr client);
extern int PanoramiXClearToBackground(ClientPtr client);
extern int PanoramiXCopyArea(ClientPtr client);
extern int PanoramiXCopyPlane(ClientPtr client);
extern int PanoramiXPolyPoint(ClientPtr client);
extern int PanoramiXPolyLine(ClientPtr client);
extern int PanoramiXPolySegment(ClientPtr client);
extern int PanoramiXPolyRectangle(ClientPtr client);
extern int PanoramiXPolyArc(ClientPtr client);
extern int PanoramiXFillPoly(ClientPtr client);
extern int PanoramiXPolyFillArc(ClientPtr client);
extern int PanoramiXPolyFillRectangle(ClientPtr client);
extern int PanoramiXPutImage(ClientPtr client);
extern int PanoramiXGetImage(ClientPtr client);
extern int PanoramiXPolyText8(ClientPtr client);
extern int PanoramiXPolyText16(ClientPtr client);
extern int PanoramiXImageText8(ClientPtr client);
extern int PanoramiXImageText16(ClientPtr client);
extern int PanoramiXCreateColormap(ClientPtr client);
extern int PanoramiXFreeColormap(ClientPtr client);
extern int PanoramiXInstallColormap(ClientPtr client);
extern int PanoramiXUninstallColormap(ClientPtr client);
extern int PanoramiXAllocColor(ClientPtr client);
extern int PanoramiXAllocNamedColor(ClientPtr client);
extern int PanoramiXAllocColorCells(ClientPtr client);
extern int PanoramiXStoreNamedColor(ClientPtr client);
extern int PanoramiXFreeColors(ClientPtr client);
extern int PanoramiXStoreColors(ClientPtr client);
extern int PanoramiXAllocColorPlanes(ClientPtr client);

#define PROC_EXTERN(pfunc)      extern int pfunc(ClientPtr)

PROC_EXTERN(ProcPanoramiXQueryVersion);
PROC_EXTERN(ProcPanoramiXGetState);
PROC_EXTERN(ProcPanoramiXGetScreenCount);
PROC_EXTERN(ProcPanoramiXGetScreenSize);

PROC_EXTERN(ProcXineramaQueryScreens);
PROC_EXTERN(ProcXineramaIsActive);

extern int SProcPanoramiXDispatch(ClientPtr client);

extern int connBlockScreenStart;
extern xConnSetupPrefix connSetupPrefix;

extern int (*SavedProcVector[256]) (ClientPtr client);
