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

#include "vndserver.h"

#include <pixmapstr.h>

#include "vndservervendor.h"

static ClientPtr requestClient = NULL;

void GlxSetRequestClient(ClientPtr client)
{
    requestClient = client;
}

static GlxServerVendor *LookupXIDMapResource(XID id)
{
    void *ptr = NULL;
    int rv;

    rv = dixLookupResourceByType(&ptr, id, idResource, NULL, DixReadAccess);
    if (rv == Success) {
        return (GlxServerVendor *) ptr;
    } else {
        return NULL;
    }
}

GlxServerVendor *GlxGetXIDMap(XID id)
{
    GlxServerVendor *vendor = LookupXIDMapResource(id);

    if (vendor == NULL) {
        // If we haven't seen this XID before, then it may be a drawable that
        // wasn't created through GLX, like a regular X window or pixmap. Try
        // to look up a matching drawable to find a screen number for it.
        void *ptr = NULL;
        int rv = dixLookupResourceByClass(&ptr, id, RC_DRAWABLE, NULL,
                                         DixGetAttrAccess);
        if (rv == Success && ptr != NULL) {
            DrawablePtr draw = (DrawablePtr) ptr;
            vendor = GlxGetVendorForScreen(requestClient, draw->pScreen);
        }
    }
    return vendor;
}

Bool GlxAddXIDMap(XID id, GlxServerVendor *vendor)
{
    if (id == 0 || vendor == NULL) {
        return FALSE;
    }
    if (LookupXIDMapResource(id) != NULL) {
        return FALSE;
    }
    return AddResource(id, idResource, vendor);
}

void GlxRemoveXIDMap(XID id)
{
    FreeResourceByType(id, idResource, FALSE);
}

GlxContextTagInfo *GlxAllocContextTag(ClientPtr client, GlxServerVendor *vendor)
{
    GlxClientPriv *cl;
    unsigned int index;

    if (vendor == NULL) {
        return NULL;
    }

    cl = GlxGetClientData(client);
    if (cl == NULL) {
        return NULL;
    }

    // Look for a free tag index.
    for (index=0; index<cl->contextTagCount; index++) {
        if (cl->contextTags[index].vendor == NULL) {
            break;
        }
    }
    if (index >= cl->contextTagCount) {
        // We didn't find a free entry, so grow the array.
        GlxContextTagInfo *newTags;
        unsigned int newSize = cl->contextTagCount * 2;
        if (newSize == 0) {
            // TODO: What's a good starting size for this?
            newSize = 16;
        }

        newTags = (GlxContextTagInfo *)
            realloc(cl->contextTags, newSize * sizeof(GlxContextTagInfo));
        if (newTags == NULL) {
            return NULL;
        }

        memset(&newTags[cl->contextTagCount], 0,
                (newSize - cl->contextTagCount) * sizeof(GlxContextTagInfo));

        index = cl->contextTagCount;
        cl->contextTags = newTags;
        cl->contextTagCount = newSize;
    }

    assert(index >= 0);
    assert(index < cl->contextTagCount);
    memset(&cl->contextTags[index], 0, sizeof(GlxContextTagInfo));
    cl->contextTags[index].tag = (GLXContextTag) (index + 1);
    cl->contextTags[index].client = client;
    cl->contextTags[index].vendor = vendor;
    return &cl->contextTags[index];
}

GlxContextTagInfo *GlxLookupContextTag(ClientPtr client, GLXContextTag tag)
{
    GlxClientPriv *cl = GlxGetClientData(client);
    if (cl == NULL) {
        return NULL;
    }

    if (tag > 0 && (tag - 1) < cl->contextTagCount) {
        if (cl->contextTags[tag - 1].vendor != NULL) {
            assert(cl->contextTags[tag - 1].client == client);
            return &cl->contextTags[tag - 1];
        }
    }
    return NULL;
}

void GlxFreeContextTag(GlxContextTagInfo *tagInfo)
{
    if (tagInfo != NULL) {
        tagInfo->vendor = NULL;
        tagInfo->vendor = NULL;
        tagInfo->data = NULL;
        tagInfo->context = None;
        tagInfo->drawable = None;
        tagInfo->readdrawable = None;
    }
}

Bool GlxSetScreenVendor(ScreenPtr screen, GlxServerVendor *vendor)
{
    GlxScreenPriv *priv;

    if (vendor == NULL) {
        return FALSE;
    }

    priv = GlxGetScreen(screen);
    if (priv == NULL) {
        return FALSE;
    }

    if (priv->vendor != NULL) {
        return FALSE;
    }

    priv->vendor = vendor;
    return TRUE;
}

Bool GlxSetClientScreenVendor(ClientPtr client, ScreenPtr screen, GlxServerVendor *vendor)
{
    GlxClientPriv *cl;

    if (screen == NULL || screen->isGPU) {
        return FALSE;
    }

    cl = GlxGetClientData(client);
    if (cl == NULL) {
        return FALSE;
    }

    if (vendor != NULL) {
        cl->vendors[screen->myNum] = vendor;
    } else {
        cl->vendors[screen->myNum] = GlxGetVendorForScreen(NULL, screen);
    }
    return TRUE;
}

GlxServerVendor *GlxGetVendorForScreen(ClientPtr client, ScreenPtr screen)
{
    // Note that the client won't be sending GPU screen numbers, so we don't
    // need per-client mappings for them.
    if (client != NULL && !screen->isGPU) {
        GlxClientPriv *cl = GlxGetClientData(client);
        if (cl != NULL) {
            return cl->vendors[screen->myNum];
        } else {
            return NULL;
        }
    } else {
        GlxScreenPriv *priv = GlxGetScreen(screen);
        if (priv != NULL) {
            return priv->vendor;
        } else {
            return NULL;
        }
    }
}
