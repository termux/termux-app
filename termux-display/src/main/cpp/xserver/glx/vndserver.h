/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifndef VNDSERVER_H
#define VNDSERVER_H

#include <dix-config.h>
#include "glxvndabi.h"

#define GLXContextID CARD32
#define GLXDrawable CARD32

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct GlxScreenPrivRec {
    GlxServerVendor *vendor;
} GlxScreenPriv;

typedef struct GlxContextTagInfoRec {
    GLXContextTag tag;
    ClientPtr client;
    GlxServerVendor *vendor;
    void *data;
    GLXContextID context;
    GLXDrawable drawable;
    GLXDrawable readdrawable;
} GlxContextTagInfo;

typedef struct GlxClientPrivRec {
    GlxContextTagInfo *contextTags;
    unsigned int contextTagCount;

    /**
     * The vendor handles for each screen.
     */
    GlxServerVendor **vendors;
} GlxClientPriv;

extern int GlxErrorBase;
extern RESTYPE idResource;

extern ExtensionEntry *GlxExtensionEntry;
Bool GlxDispatchInit(void);
void GlxDispatchReset(void);

/**
 * Handles a request from the client.
 *
 * This function will look up the correct handler function and forward the
 * request to it.
 */
int GlxDispatchRequest(ClientPtr client);

/**
 * Looks up the GlxClientPriv struct for a client. If we don't have a
 * GlxClientPriv struct yet, then allocate one.
 */
GlxClientPriv *GlxGetClientData(ClientPtr client);

/**
 * Frees any data that's specific to a client. This should be called when a
 * client disconnects.
 */
void GlxFreeClientData(ClientPtr client);

Bool GlxAddXIDMap(XID id, GlxServerVendor *vendor);
GlxServerVendor * GlxGetXIDMap(XID id);
void GlxRemoveXIDMap(XID id);

/**
 * Records the client that sent the current request. This is needed in
 * GlxGetXIDMap to know which client's (screen -> vendor) mapping to use for a
 * regular X window.
 */
void GlxSetRequestClient(ClientPtr client);

GlxContextTagInfo *GlxAllocContextTag(ClientPtr client, GlxServerVendor *vendor);
GlxContextTagInfo *GlxLookupContextTag(ClientPtr client, GLXContextTag tag);
void GlxFreeContextTag(GlxContextTagInfo *tagInfo);

Bool GlxSetScreenVendor(ScreenPtr screen, GlxServerVendor *vendor);
Bool GlxSetClientScreenVendor(ClientPtr client, ScreenPtr screen, GlxServerVendor *vendor);
GlxScreenPriv *GlxGetScreen(ScreenPtr pScreen);
GlxServerVendor *GlxGetVendorForScreen(ClientPtr client, ScreenPtr screen);

static inline CARD32 GlxCheckSwap(ClientPtr client, CARD32 value)
{
    if (client->swapped)
    {
        value = ((value & 0XFF000000) >> 24) | ((value & 0X00FF0000) >>  8)
            | ((value & 0X0000FF00) <<  8) | ((value & 0X000000FF) << 24);
    }
    return value;
}

#if defined(__cplusplus)
}
#endif

_X_EXPORT const GlxServerExports *glvndGetExports(void);

#endif // VNDSERVER_H
