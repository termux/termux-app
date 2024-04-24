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

#include <dix-config.h>

#include "hashtable.h"
#include "vndserver.h"
#include "vndservervendor.h"

/**
 * The length of the dispatchFuncs array. Every opcode above this is a
 * X_GLsop_* code, which all can use the same handler.
 */
#define OPCODE_ARRAY_LEN 100

// This hashtable is used to keep track of the dispatch stubs for
// GLXVendorPrivate and GLXVendorPrivateWithReply.
typedef struct GlxVendorPrivDispatchRec {
    CARD32 vendorCode;
    GlxServerDispatchProc proc;
    HashTable hh;
} GlxVendorPrivDispatch;

static GlxServerDispatchProc dispatchFuncs[OPCODE_ARRAY_LEN] = {};
static HashTable vendorPrivHash = NULL;
static HtGenericHashSetupRec vendorPrivSetup = {
    .keySize = sizeof(CARD32)
};

static int DispatchBadRequest(ClientPtr client)
{
    return BadRequest;
}

static GlxVendorPrivDispatch *LookupVendorPrivDispatch(CARD32 vendorCode, Bool create)
{
    GlxVendorPrivDispatch *disp = NULL;

    disp = ht_find(vendorPrivHash, &vendorCode);
    if (disp == NULL && create) {
        if ((disp = ht_add(vendorPrivHash, &vendorCode))) {
            disp->vendorCode = vendorCode;
            disp->proc = NULL;
        }
    }

    return disp;
}

static GlxServerDispatchProc GetVendorDispatchFunc(CARD8 opcode, CARD32 vendorCode)
{
    GlxServerVendor *vendor;

    xorg_list_for_each_entry(vendor, &GlxVendorList, entry) {
        GlxServerDispatchProc proc = vendor->glxvc.getDispatchAddress(opcode, vendorCode);
        if (proc != NULL) {
            return proc;
        }
    }

    return DispatchBadRequest;
}

static void SetReplyHeader(ClientPtr client, void *replyPtr)
{
    xGenericReply *rep = (xGenericReply *) replyPtr;
    rep->type = X_Reply;
    rep->sequenceNumber = client->sequence;
    if (client->swapped) {
	swaps(&rep->sequenceNumber);
    }
    rep->length = 0;
}

/* Include the trivial dispatch handlers */
#include "vnd_dispatch_stubs.c"

static int dispatch_GLXQueryVersion(ClientPtr client)
{
    xGLXQueryVersionReply reply;
    REQUEST_SIZE_MATCH(xGLXQueryVersionReq);

    SetReplyHeader(client, &reply);
    reply.majorVersion = GlxCheckSwap(client, 1);
    reply.minorVersion = GlxCheckSwap(client, 4);

    WriteToClient(client, sz_xGLXQueryVersionReply, &reply);
    return Success;
}

/* broken header workaround */
#ifndef X_GLXSetClientInfo2ARB
#define X_GLXSetClientInfo2ARB X_GLXSetConfigInfo2ARB
#endif

/**
 * This function is used for X_GLXClientInfo, X_GLXSetClientInfoARB, and
 * X_GLXSetClientInfo2ARB.
 */
static int dispatch_GLXClientInfo(ClientPtr client)
{
    GlxServerVendor *vendor;
    void *requestCopy = NULL;
    size_t requestSize = client->req_len * 4;

    if (client->minorOp == X_GLXClientInfo) {
        REQUEST_AT_LEAST_SIZE(xGLXClientInfoReq);
    } else if (client->minorOp == X_GLXSetClientInfoARB) {
        REQUEST_AT_LEAST_SIZE(xGLXSetClientInfoARBReq);
    } else if (client->minorOp == X_GLXSetClientInfo2ARB) {
        REQUEST_AT_LEAST_SIZE(xGLXSetClientInfo2ARBReq);
    } else {
        return BadImplementation;
    }

    // We'll forward this request to each vendor library. Since a vendor might
    // modify the request data in place (e.g., for byte swapping), make a copy
    // of the request first.
    requestCopy = malloc(requestSize);
    if (requestCopy == NULL) {
        return BadAlloc;
    }
    memcpy(requestCopy, client->requestBuffer, requestSize);

    xorg_list_for_each_entry(vendor, &GlxVendorList, entry) {
        vendor->glxvc.handleRequest(client);
        // Revert the request buffer back to our copy.
        memcpy(client->requestBuffer, requestCopy, requestSize);
    }
    free(requestCopy);
    return Success;
}

static int CommonLoseCurrent(ClientPtr client, GlxContextTagInfo *tagInfo)
{
    int ret;

    ret = tagInfo->vendor->glxvc.makeCurrent(client,
            tagInfo->tag, // No old context tag,
            None, None, None, 0);

    if (ret == Success) {
        GlxFreeContextTag(tagInfo);
    }
    return ret;
}

static int CommonMakeNewCurrent(ClientPtr client,
        GlxServerVendor *vendor,
        GLXDrawable drawable,
        GLXDrawable readdrawable,
        GLXContextID context,
        GLXContextTag *newContextTag)
{
    int ret = BadAlloc;
    GlxContextTagInfo *tagInfo;

    tagInfo = GlxAllocContextTag(client, vendor);

    if (tagInfo) {
        ret = vendor->glxvc.makeCurrent(client,
                0, // No old context tag,
                drawable, readdrawable, context,
                tagInfo->tag);

        if (ret == Success) {
            tagInfo->drawable = drawable;
            tagInfo->readdrawable = readdrawable;
            tagInfo->context = context;
            *newContextTag = tagInfo->tag;
        } else {
            GlxFreeContextTag(tagInfo);
        }
    }

    return ret;
}

static int CommonMakeCurrent(ClientPtr client,
        GLXContextTag oldContextTag,
        GLXDrawable drawable,
        GLXDrawable readdrawable,
        GLXContextID context)
{
    xGLXMakeCurrentReply reply = {};
    GlxContextTagInfo *oldTag = NULL;
    GlxServerVendor *newVendor = NULL;

    oldContextTag = GlxCheckSwap(client, oldContextTag);
    drawable = GlxCheckSwap(client, drawable);
    readdrawable = GlxCheckSwap(client, readdrawable);
    context = GlxCheckSwap(client, context);

    SetReplyHeader(client, &reply);

    if (oldContextTag != 0) {
        oldTag = GlxLookupContextTag(client, oldContextTag);
        if (oldTag == NULL) {
            return GlxErrorBase + GLXBadContextTag;
        }
    }
    if (context != 0) {
        newVendor = GlxGetXIDMap(context);
        if (newVendor == NULL) {
            return GlxErrorBase + GLXBadContext;
        }
    }

    if (oldTag == NULL && newVendor == NULL) {
        // Nothing to do here. Just send a successful reply.
        reply.contextTag = 0;
    } else if (oldTag != NULL && newVendor != NULL
            && oldTag->context == context
            && oldTag->drawable == drawable
            && oldTag->readdrawable == readdrawable)
    {
        // The old and new values are all the same, so send a successful reply.
        reply.contextTag = oldTag->tag;
    } else {
        // TODO: For switching contexts in a single vendor, just make one
        // makeCurrent call?

        // TODO: When changing vendors, would it be better to do the
        // MakeCurrent(new) first, then the LoseCurrent(old)?
        // If the MakeCurrent(new) fails, then the old context will still be current.
        // If the LoseCurrent(old) fails, then we can (probably) undo the MakeCurrent(new) with
        // a LoseCurrent(old).
        // But, if the recovery LoseCurrent(old) fails, then we're really in a bad state.

        // Clear the old context first.
        if (oldTag != NULL) {
            int ret = CommonLoseCurrent(client, oldTag);
            if (ret != Success) {
                return ret;
            }
            oldTag = NULL;
        }

        if (newVendor != NULL) {
            int ret = CommonMakeNewCurrent(client, newVendor, drawable, readdrawable, context, &reply.contextTag);
            if (ret != Success) {
                return ret;
            }
        } else {
            reply.contextTag = 0;
        }
    }

    reply.contextTag = GlxCheckSwap(client, reply.contextTag);
    WriteToClient(client, sz_xGLXMakeCurrentReply, &reply);
    return Success;
}

static int dispatch_GLXMakeCurrent(ClientPtr client)
{
    REQUEST(xGLXMakeCurrentReq);
    REQUEST_SIZE_MATCH(*stuff);

    return CommonMakeCurrent(client, stuff->oldContextTag,
            stuff->drawable, stuff->drawable, stuff->context);
}

static int dispatch_GLXMakeContextCurrent(ClientPtr client)
{
    REQUEST(xGLXMakeContextCurrentReq);
    REQUEST_SIZE_MATCH(*stuff);

    return CommonMakeCurrent(client, stuff->oldContextTag,
            stuff->drawable, stuff->readdrawable, stuff->context);
}

static int dispatch_GLXMakeCurrentReadSGI(ClientPtr client)
{
    REQUEST(xGLXMakeCurrentReadSGIReq);
    REQUEST_SIZE_MATCH(*stuff);

    return CommonMakeCurrent(client, stuff->oldContextTag,
            stuff->drawable, stuff->readable, stuff->context);
}

static int dispatch_GLXCopyContext(ClientPtr client)
{
    REQUEST(xGLXCopyContextReq);
    GlxServerVendor *vendor;
    REQUEST_SIZE_MATCH(*stuff);

    // If we've got a context tag, then we'll use it to select a vendor. If we
    // don't have a tag, then we'll look up one of the contexts. In either
    // case, it's up to the vendor library to make sure that the context ID's
    // are valid.
    if (stuff->contextTag != 0) {
        GlxContextTagInfo *tagInfo = GlxLookupContextTag(client, GlxCheckSwap(client, stuff->contextTag));
        if (tagInfo == NULL) {
            return GlxErrorBase + GLXBadContextTag;
        }
        vendor = tagInfo->vendor;
    } else {
        vendor = GlxGetXIDMap(GlxCheckSwap(client, stuff->source));
        if (vendor == NULL) {
            return GlxErrorBase + GLXBadContext;
        }
    }
    return vendor->glxvc.handleRequest(client);
}

static int dispatch_GLXSwapBuffers(ClientPtr client)
{
    GlxServerVendor *vendor = NULL;
    REQUEST(xGLXSwapBuffersReq);
    REQUEST_SIZE_MATCH(*stuff);

    if (stuff->contextTag != 0) {
        // If the request has a context tag, then look up a vendor from that.
        // The vendor library is then responsible for validating the drawable.
        GlxContextTagInfo *tagInfo = GlxLookupContextTag(client, GlxCheckSwap(client, stuff->contextTag));
        if (tagInfo == NULL) {
            return GlxErrorBase + GLXBadContextTag;
        }
        vendor = tagInfo->vendor;
    } else {
        // We don't have a context tag, so look up the vendor from the
        // drawable.
        vendor = GlxGetXIDMap(GlxCheckSwap(client, stuff->drawable));
        if (vendor == NULL) {
            return GlxErrorBase + GLXBadDrawable;
        }
    }

    return vendor->glxvc.handleRequest(client);
}

/**
 * This is a generic handler for all of the X_GLXsop* requests.
 */
static int dispatch_GLXSingle(ClientPtr client)
{
    REQUEST(xGLXSingleReq);
    GlxContextTagInfo *tagInfo;
    REQUEST_AT_LEAST_SIZE(*stuff);

    tagInfo = GlxLookupContextTag(client, GlxCheckSwap(client, stuff->contextTag));
    if (tagInfo != NULL) {
        return tagInfo->vendor->glxvc.handleRequest(client);
    } else {
        return GlxErrorBase + GLXBadContextTag;
    }
}

static int dispatch_GLXVendorPriv(ClientPtr client)
{
    GlxVendorPrivDispatch *disp;
    REQUEST(xGLXVendorPrivateReq);
    REQUEST_AT_LEAST_SIZE(*stuff);

    disp = LookupVendorPrivDispatch(GlxCheckSwap(client, stuff->vendorCode), TRUE);
    if (disp == NULL) {
        return BadAlloc;
    }

    if (disp->proc == NULL) {
        // We don't have a dispatch function for this request yet. Check with
        // each vendor library to find one.
        // Note that even if none of the vendors provides a dispatch stub,
        // we'll still add an entry to the dispatch table, so that we don't
        // have to look it up again later.

        disp->proc = GetVendorDispatchFunc(stuff->glxCode,
                                           GlxCheckSwap(client,
                                                        stuff->vendorCode));
    }
    return disp->proc(client);
}

Bool GlxDispatchInit(void)
{
    GlxVendorPrivDispatch *disp;

    vendorPrivHash = ht_create(sizeof(CARD32), sizeof(GlxVendorPrivDispatch),
                               ht_generic_hash, ht_generic_compare,
                               (void *) &vendorPrivSetup);
    if (!vendorPrivHash) {
        return FALSE;
    }

    // Assign a custom dispatch stub GLXMakeCurrentReadSGI. This is the only
    // vendor private request that we need to deal with in libglvnd itself.
    disp = LookupVendorPrivDispatch(X_GLXvop_MakeCurrentReadSGI, TRUE);
    if (disp == NULL) {
        return FALSE;
    }
    disp->proc = dispatch_GLXMakeCurrentReadSGI;

    // Assign the dispatch stubs for requests that need special handling.
    dispatchFuncs[X_GLXQueryVersion] = dispatch_GLXQueryVersion;
    dispatchFuncs[X_GLXMakeCurrent] = dispatch_GLXMakeCurrent;
    dispatchFuncs[X_GLXMakeContextCurrent] = dispatch_GLXMakeContextCurrent;
    dispatchFuncs[X_GLXCopyContext] = dispatch_GLXCopyContext;
    dispatchFuncs[X_GLXSwapBuffers] = dispatch_GLXSwapBuffers;

    dispatchFuncs[X_GLXClientInfo] = dispatch_GLXClientInfo;
    dispatchFuncs[X_GLXSetClientInfoARB] = dispatch_GLXClientInfo;
    dispatchFuncs[X_GLXSetClientInfo2ARB] = dispatch_GLXClientInfo;

    dispatchFuncs[X_GLXVendorPrivate] = dispatch_GLXVendorPriv;
    dispatchFuncs[X_GLXVendorPrivateWithReply] = dispatch_GLXVendorPriv;

    // Assign the trivial stubs
    dispatchFuncs[X_GLXRender] = dispatch_Render;
    dispatchFuncs[X_GLXRenderLarge] = dispatch_RenderLarge;
    dispatchFuncs[X_GLXCreateContext] = dispatch_CreateContext;
    dispatchFuncs[X_GLXDestroyContext] = dispatch_DestroyContext;
    dispatchFuncs[X_GLXWaitGL] = dispatch_WaitGL;
    dispatchFuncs[X_GLXWaitX] = dispatch_WaitX;
    dispatchFuncs[X_GLXUseXFont] = dispatch_UseXFont;
    dispatchFuncs[X_GLXCreateGLXPixmap] = dispatch_CreateGLXPixmap;
    dispatchFuncs[X_GLXGetVisualConfigs] = dispatch_GetVisualConfigs;
    dispatchFuncs[X_GLXDestroyGLXPixmap] = dispatch_DestroyGLXPixmap;
    dispatchFuncs[X_GLXQueryExtensionsString] = dispatch_QueryExtensionsString;
    dispatchFuncs[X_GLXQueryServerString] = dispatch_QueryServerString;
    dispatchFuncs[X_GLXChangeDrawableAttributes] = dispatch_ChangeDrawableAttributes;
    dispatchFuncs[X_GLXCreateNewContext] = dispatch_CreateNewContext;
    dispatchFuncs[X_GLXCreatePbuffer] = dispatch_CreatePbuffer;
    dispatchFuncs[X_GLXCreatePixmap] = dispatch_CreatePixmap;
    dispatchFuncs[X_GLXCreateWindow] = dispatch_CreateWindow;
    dispatchFuncs[X_GLXCreateContextAttribsARB] = dispatch_CreateContextAttribsARB;
    dispatchFuncs[X_GLXDestroyPbuffer] = dispatch_DestroyPbuffer;
    dispatchFuncs[X_GLXDestroyPixmap] = dispatch_DestroyPixmap;
    dispatchFuncs[X_GLXDestroyWindow] = dispatch_DestroyWindow;
    dispatchFuncs[X_GLXGetDrawableAttributes] = dispatch_GetDrawableAttributes;
    dispatchFuncs[X_GLXGetFBConfigs] = dispatch_GetFBConfigs;
    dispatchFuncs[X_GLXQueryContext] = dispatch_QueryContext;
    dispatchFuncs[X_GLXIsDirect] = dispatch_IsDirect;

    return TRUE;
}

void GlxDispatchReset(void)
{
    memset(dispatchFuncs, 0, sizeof(dispatchFuncs));

    ht_destroy(vendorPrivHash);
    vendorPrivHash = NULL;
}

int GlxDispatchRequest(ClientPtr client)
{
    REQUEST(xReq);
    int result;

    if (GlxExtensionEntry->base == 0)
        return BadRequest;

    GlxSetRequestClient(client);

    if (stuff->data < OPCODE_ARRAY_LEN) {
        if (dispatchFuncs[stuff->data] == NULL) {
            // Try to find a dispatch stub.
            dispatchFuncs[stuff->data] = GetVendorDispatchFunc(stuff->data, 0);
        }
        result = dispatchFuncs[stuff->data](client);
    } else {
        result = dispatch_GLXSingle(client);
    }

    GlxSetRequestClient(NULL);

    return result;
}
