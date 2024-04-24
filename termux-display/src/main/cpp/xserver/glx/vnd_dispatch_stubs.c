
#include <dix-config.h>
#include <dix.h>
#include "vndserver.h"

// HACK: The opcode in old glxproto.h has a typo in it.
#if !defined(X_GLXCreateContextAttribsARB)
#define X_GLXCreateContextAttribsARB X_GLXCreateContextAtrribsARB
#endif

static int dispatch_Render(ClientPtr client)
{
    REQUEST(xGLXRenderReq);
    CARD32 contextTag;
    GlxServerVendor *vendor = NULL;
    REQUEST_AT_LEAST_SIZE(*stuff);
    contextTag = GlxCheckSwap(client, stuff->contextTag);
    vendor = glxServer.getContextTag(client, contextTag);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = contextTag;
        return GlxErrorBase + GLXBadContextTag;
    }
}
static int dispatch_RenderLarge(ClientPtr client)
{
    REQUEST(xGLXRenderLargeReq);
    CARD32 contextTag;
    GlxServerVendor *vendor = NULL;
    REQUEST_AT_LEAST_SIZE(*stuff);
    contextTag = GlxCheckSwap(client, stuff->contextTag);
    vendor = glxServer.getContextTag(client, contextTag);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = contextTag;
        return GlxErrorBase + GLXBadContextTag;
    }
}
static int dispatch_CreateContext(ClientPtr client)
{
    REQUEST(xGLXCreateContextReq);
    CARD32 screen, context;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    context = GlxCheckSwap(client, stuff->context);
    LEGAL_NEW_RESOURCE(context, client);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        if (!glxServer.addXIDMap(context, vendor)) {
            return BadAlloc;
        }
        ret = glxServer.forwardRequest(vendor, client);
        if (ret != Success) {
            glxServer.removeXIDMap(context);
        }
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_DestroyContext(ClientPtr client)
{
    REQUEST(xGLXDestroyContextReq);
    CARD32 context;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    context = GlxCheckSwap(client, stuff->context);
    vendor = glxServer.getXIDMap(context);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        if (ret == Success) {
            glxServer.removeXIDMap(context);
        }
        return ret;
    } else {
        client->errorValue = context;
        return GlxErrorBase + GLXBadContext;
    }
}
static int dispatch_WaitGL(ClientPtr client)
{
    REQUEST(xGLXWaitGLReq);
    CARD32 contextTag;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    contextTag = GlxCheckSwap(client, stuff->contextTag);
    vendor = glxServer.getContextTag(client, contextTag);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = contextTag;
        return GlxErrorBase + GLXBadContextTag;
    }
}
static int dispatch_WaitX(ClientPtr client)
{
    REQUEST(xGLXWaitXReq);
    CARD32 contextTag;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    contextTag = GlxCheckSwap(client, stuff->contextTag);
    vendor = glxServer.getContextTag(client, contextTag);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = contextTag;
        return GlxErrorBase + GLXBadContextTag;
    }
}
static int dispatch_UseXFont(ClientPtr client)
{
    REQUEST(xGLXUseXFontReq);
    CARD32 contextTag;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    contextTag = GlxCheckSwap(client, stuff->contextTag);
    vendor = glxServer.getContextTag(client, contextTag);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = contextTag;
        return GlxErrorBase + GLXBadContextTag;
    }
}
static int dispatch_CreateGLXPixmap(ClientPtr client)
{
    REQUEST(xGLXCreateGLXPixmapReq);
    CARD32 screen, glxpixmap;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    glxpixmap = GlxCheckSwap(client, stuff->glxpixmap);
    LEGAL_NEW_RESOURCE(glxpixmap, client);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        if (!glxServer.addXIDMap(glxpixmap, vendor)) {
            return BadAlloc;
        }
        ret = glxServer.forwardRequest(vendor, client);
        if (ret != Success) {
            glxServer.removeXIDMap(glxpixmap);
        }
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_GetVisualConfigs(ClientPtr client)
{
    REQUEST(xGLXGetVisualConfigsReq);
    CARD32 screen;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_DestroyGLXPixmap(ClientPtr client)
{
    REQUEST(xGLXDestroyGLXPixmapReq);
    CARD32 glxpixmap;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    glxpixmap = GlxCheckSwap(client, stuff->glxpixmap);
    vendor = glxServer.getXIDMap(glxpixmap);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = glxpixmap;
        return GlxErrorBase + GLXBadPixmap;
    }
}
static int dispatch_QueryExtensionsString(ClientPtr client)
{
    REQUEST(xGLXQueryExtensionsStringReq);
    CARD32 screen;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_QueryServerString(ClientPtr client)
{
    REQUEST(xGLXQueryServerStringReq);
    CARD32 screen;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_ChangeDrawableAttributes(ClientPtr client)
{
    REQUEST(xGLXChangeDrawableAttributesReq);
    CARD32 drawable;
    GlxServerVendor *vendor = NULL;
    REQUEST_AT_LEAST_SIZE(*stuff);
    drawable = GlxCheckSwap(client, stuff->drawable);
    vendor = glxServer.getXIDMap(drawable);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = drawable;
        return BadDrawable;
    }
}
static int dispatch_CreateNewContext(ClientPtr client)
{
    REQUEST(xGLXCreateNewContextReq);
    CARD32 screen, context;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    context = GlxCheckSwap(client, stuff->context);
    LEGAL_NEW_RESOURCE(context, client);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        if (!glxServer.addXIDMap(context, vendor)) {
            return BadAlloc;
        }
        ret = glxServer.forwardRequest(vendor, client);
        if (ret != Success) {
            glxServer.removeXIDMap(context);
        }
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_CreatePbuffer(ClientPtr client)
{
    REQUEST(xGLXCreatePbufferReq);
    CARD32 screen, pbuffer;
    GlxServerVendor *vendor = NULL;
    REQUEST_AT_LEAST_SIZE(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    pbuffer = GlxCheckSwap(client, stuff->pbuffer);
    LEGAL_NEW_RESOURCE(pbuffer, client);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        if (!glxServer.addXIDMap(pbuffer, vendor)) {
            return BadAlloc;
        }
        ret = glxServer.forwardRequest(vendor, client);
        if (ret != Success) {
            glxServer.removeXIDMap(pbuffer);
        }
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_CreatePixmap(ClientPtr client)
{
    REQUEST(xGLXCreatePixmapReq);
    CARD32 screen, glxpixmap;
    GlxServerVendor *vendor = NULL;
    REQUEST_AT_LEAST_SIZE(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    glxpixmap = GlxCheckSwap(client, stuff->glxpixmap);
    LEGAL_NEW_RESOURCE(glxpixmap, client);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        if (!glxServer.addXIDMap(glxpixmap, vendor)) {
            return BadAlloc;
        }
        ret = glxServer.forwardRequest(vendor, client);
        if (ret != Success) {
            glxServer.removeXIDMap(glxpixmap);
        }
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_CreateWindow(ClientPtr client)
{
    REQUEST(xGLXCreateWindowReq);
    CARD32 screen, glxwindow;
    GlxServerVendor *vendor = NULL;
    REQUEST_AT_LEAST_SIZE(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    glxwindow = GlxCheckSwap(client, stuff->glxwindow);
    LEGAL_NEW_RESOURCE(glxwindow, client);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        if (!glxServer.addXIDMap(glxwindow, vendor)) {
            return BadAlloc;
        }
        ret = glxServer.forwardRequest(vendor, client);
        if (ret != Success) {
            glxServer.removeXIDMap(glxwindow);
        }
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_CreateContextAttribsARB(ClientPtr client)
{
    REQUEST(xGLXCreateContextAttribsARBReq);
    CARD32 screen, context;
    GlxServerVendor *vendor = NULL;
    REQUEST_AT_LEAST_SIZE(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    context = GlxCheckSwap(client, stuff->context);
    LEGAL_NEW_RESOURCE(context, client);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        if (!glxServer.addXIDMap(context, vendor)) {
            return BadAlloc;
        }
        ret = glxServer.forwardRequest(vendor, client);
        if (ret != Success) {
            glxServer.removeXIDMap(context);
        }
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_DestroyPbuffer(ClientPtr client)
{
    REQUEST(xGLXDestroyPbufferReq);
    CARD32 pbuffer;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    pbuffer = GlxCheckSwap(client, stuff->pbuffer);
    vendor = glxServer.getXIDMap(pbuffer);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        if (ret == Success) {
            glxServer.removeXIDMap(pbuffer);
        }
        return ret;
    } else {
        client->errorValue = pbuffer;
        return GlxErrorBase + GLXBadPbuffer;
    }
}
static int dispatch_DestroyPixmap(ClientPtr client)
{
    REQUEST(xGLXDestroyPixmapReq);
    CARD32 glxpixmap;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    glxpixmap = GlxCheckSwap(client, stuff->glxpixmap);
    vendor = glxServer.getXIDMap(glxpixmap);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        if (ret == Success) {
            glxServer.removeXIDMap(glxpixmap);
        }
        return ret;
    } else {
        client->errorValue = glxpixmap;
        return GlxErrorBase + GLXBadPixmap;
    }
}
static int dispatch_DestroyWindow(ClientPtr client)
{
    REQUEST(xGLXDestroyWindowReq);
    CARD32 glxwindow;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    glxwindow = GlxCheckSwap(client, stuff->glxwindow);
    vendor = glxServer.getXIDMap(glxwindow);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        if (ret == Success) {
            glxServer.removeXIDMap(glxwindow);
        }
        return ret;
    } else {
        client->errorValue = glxwindow;
        return GlxErrorBase + GLXBadWindow;
    }
}
static int dispatch_GetDrawableAttributes(ClientPtr client)
{
    REQUEST(xGLXGetDrawableAttributesReq);
    CARD32 drawable;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    drawable = GlxCheckSwap(client, stuff->drawable);
    vendor = glxServer.getXIDMap(drawable);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = drawable;
        return BadDrawable;
    }
}
static int dispatch_GetFBConfigs(ClientPtr client)
{
    REQUEST(xGLXGetFBConfigsReq);
    CARD32 screen;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    screen = GlxCheckSwap(client, stuff->screen);
    if (screen < screenInfo.numScreens) {
        vendor = glxServer.getVendorForScreen(client, screenInfo.screens[screen]);
    }
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = screen;
        return BadMatch;
    }
}
static int dispatch_QueryContext(ClientPtr client)
{
    REQUEST(xGLXQueryContextReq);
    CARD32 context;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    context = GlxCheckSwap(client, stuff->context);
    vendor = glxServer.getXIDMap(context);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = context;
        return GlxErrorBase + GLXBadContext;
    }
}
static int dispatch_IsDirect(ClientPtr client)
{
    REQUEST(xGLXIsDirectReq);
    CARD32 context;
    GlxServerVendor *vendor = NULL;
    REQUEST_SIZE_MATCH(*stuff);
    context = GlxCheckSwap(client, stuff->context);
    vendor = glxServer.getXIDMap(context);
    if (vendor != NULL) {
        int ret;
        ret = glxServer.forwardRequest(vendor, client);
        return ret;
    } else {
        client->errorValue = context;
        return GlxErrorBase + GLXBadContext;
    }
}
