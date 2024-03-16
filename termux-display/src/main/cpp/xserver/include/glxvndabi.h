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

/**
 * \file
 *
 * Defines the interface between the libglvnd server module and a vendor
 * library.
 *
 * Each screen may have one vendor library assigned to it. The GLVND module
 * will examine each GLX request to determine which screen it goes to, and then
 * it will forward that request to whichever vendor is assigned to that screen.
 *
 * Each vendor library is represented by an opaque __GLXServerVendor handle.
 * Display drivers are responsible for creating handles for its GLX
 * implementations, and assigning those handles to each screen.
 *
 * The GLVND module keeps a list of callbacks, which are called from
 * InitExtensions. Drivers should use that callback to assign a vendor
 * handle to whichever screens they support.
 *
 * Additional notes about dispatching:
 * - If a request has one or more GLXContextTag values, then the dispatch stub
 *   must ensure that all of the tags belong to the vendor that it forwards the
 *   request to. Otherwise, if a vendor library tries to look up the private
 *   data for the tag, it could get the data from another vendor and crash.
 * - Following from the last point, if a request takes a GLXContextTag value,
 *   then the dispatch stub should use the tag to select a vendor. If the
 *   request takes two or more tags, then the vendor must ensure that they all
 *   map to the same vendor.
 */

#ifndef GLXVENDORABI_H
#define GLXVENDORABI_H

#include <scrnintstr.h>
#include <extnsionst.h>
#include <GL/glxproto.h>

/*!
 * Current version of the ABI.
 *
 * This version number contains a major number in the high-order 16 bits, and
 * a minor version number in the low-order 16 bits.
 *
 * The major version number is incremented when an interface change will break
 * backwards compatibility with existing vendor libraries. The minor version
 * number is incremented when there's a change but existing vendor libraries
 * will still work.
 */
#define GLXSERVER_VENDOR_ABI_MAJOR_VERSION 0
#define GLXSERVER_VENDOR_ABI_MINOR_VERSION 1

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * An opaque pointer representing a vendor library.
 */
typedef struct GlxServerVendorRec GlxServerVendor;

typedef int (* GlxServerDispatchProc) (ClientPtr client);

typedef struct GlxServerImportsRec GlxServerImports;

/**
 * Functions exported by libglvnd to the vendor library.
 */
typedef struct GlxServerExportsRec {
    int majorVersion;
    int minorVersion;

    /**
     * This callback is called during each server generation when the GLX
     * extension is initialized.
     *
     * Drivers may create a __GLXServerVendor handle at any time, but may only
     * assign a vendor to a screen from this callback.
     *
     * The callback is called with the ExtensionEntry pointer for the GLX
     * extension.
     */
    CallbackListPtr *extensionInitCallback;

    /**
     * Allocates and zeroes a __GLXserverImports structure.
     *
     * Future versions of the GLVND interface may add optional members to the
     * end of the __GLXserverImports struct. Letting the GLVND layer allocate
     * the __GLXserverImports struct allows backward compatibility with
     * existing drivers.
     */
    GlxServerImports * (* allocateServerImports) (void);

    /**
     * Frees a __GLXserverImports structure that was allocated with
     * \c allocateServerImports.
     */
    void (* freeServerImports) (GlxServerImports *imports);

    /**
     * Creates a new vendor library handle.
     */
    GlxServerVendor * (* createVendor) (const GlxServerImports *imports);

    /**
     * Destroys a vendor library handle.
     *
     * This function may not be called while the vendor handle is assigned to a
     * screen, but it may be called from the __GLXserverImports::extensionCloseDown
     * callback.
     */
    void (* destroyVendor) (GlxServerVendor *vendor);

    /**
     * Sets the vendor library to use for a screen.
     *
     * This function should be called from the screen's CreateScreenResources
     * callback.
     */
    Bool (* setScreenVendor) (ScreenPtr screen, GlxServerVendor *vendor);


    /**
     * Adds an entry to the XID map.
     *
     * This mapping is used to dispatch requests based on an XID.
     *
     * Client-generated XID's (contexts, drawables, etc) must be added to the
     * map by the dispatch stub.
     *
     * XID's that are generated in the server should be added by the vendor
     * library.
     *
     * Vendor libraries are responsible for keeping track of any additional
     * data they need for the XID's.
     *
     * Note that adding GLXFBConfig ID's appears to be unnecessary -- every GLX
     * request I can find that takes a GLXFBConfig also takes a screen number.
     *
     * \param id The XID to add to the map. The XID must not already be in the
     *      map.
     * \param vendor The vendor library to associate with \p id.
     * \return True on success, or False on failure.
     */
    Bool (* addXIDMap) (XID id, GlxServerVendor *vendor);

    /**
     * Returns the vendor and data for an XID, as added with \c addXIDMap.
     *
     * If \p id wasn't added with \c addXIDMap (for example, if it's a regular
     * X window), then libglvnd will try to look it up as a drawable and return
     * the vendor for whatever screen it's on.
     *
     * \param id The XID to look up.
     * \return The vendor that owns the XID, or \c NULL if no matching vendor
     * was found.
     */
    GlxServerVendor * (* getXIDMap) (XID id);

    /**
     * Removes an entry from the XID map.
     */
    void (* removeXIDMap) (XID id);

    /**
     * Looks up a context tag.
     *
     * Context tags are created and managed by libglvnd to ensure that they're
     * unique between vendors.
     *
     * \param client The client connection.
     * \param tag The context tag.
     * \return The vendor that owns the context tag, or \c NULL if the context
     * tag is invalid.
     */
    GlxServerVendor * (* getContextTag)(ClientPtr client, GLXContextTag tag);

    /**
     * Assigns a pointer to vendor-private data for a context tag.
     *
     * Since the tag values are assigned by GLVND, vendors can use this
     * function to store any private data they need for a context tag.
     *
     * \param client The client connection.
     * \param tag The context tag.
     * \param data An arbitrary pointer value.
     */
    Bool (* setContextTagPrivate)(ClientPtr client, GLXContextTag tag, void *data);

    /**
     * Returns the private data pointer that was assigned from
     * setContextTagPrivate.
     *
     * This function is safe to use in __GLXserverImports::makeCurrent to look
     * up the old context private pointer.
     *
     * However, this function is not safe to use from a ClientStateCallback,
     * because GLVND may have already deleted the tag by that point.
     */
    void * (* getContextTagPrivate)(ClientPtr client, GLXContextTag tag);

    GlxServerVendor * (* getVendorForScreen) (ClientPtr client, ScreenPtr screen);

    /**
     * Forwards a request to a vendor library.
     *
     * \param vendor The vendor to send the request to.
     * \param client The client.
     */
    int (* forwardRequest) (GlxServerVendor *vendor, ClientPtr client);

    /**
     * Sets the vendor library to use for a screen for a specific client.
     *
     * This function changes which vendor should handle GLX requests for a
     * screen. Unlike \c setScreenVendor, this function can be called at any
     * time, and only applies to requests from a single client.
     *
     * This function is available in GLXVND version 0.1 or later.
     */
    Bool (* setClientScreenVendor) (ClientPtr client, ScreenPtr screen, GlxServerVendor *vendor);
} GlxServerExports;

extern _X_EXPORT const GlxServerExports glxServer;

/**
 * Functions exported by the vendor library to libglvnd.
 */
struct GlxServerImportsRec {
    /**
     * Called on a server reset.
     *
     * This is called from the extension's CloseDown callback.
     *
     * Note that this is called after freeing all of GLVND's per-screen data,
     * so the callback may destroy any vendor handles.
     *
     * If the server is exiting, then GLVND will free any remaining vendor
     * handles after calling the extensionCloseDown callbacks.
     */
    void (* extensionCloseDown) (const ExtensionEntry *extEntry);

    /**
     * Handles a GLX request.
     */
    int (* handleRequest) (ClientPtr client);

    /**
     * Returns a dispatch function for a request.
     *
     * \param minorOpcode The minor opcode of the request.
     * \param vendorCode The vendor opcode, if \p minorOpcode
     *      is \c X_GLXVendorPrivate or \c X_GLXVendorPrivateWithReply.
     * \return A dispatch function, or NULL if the vendor doesn't support this
     *      request.
     */
    GlxServerDispatchProc (* getDispatchAddress) (CARD8 minorOpcode, CARD32 vendorCode);

    /**
     * Handles a MakeCurrent request.
     *
     * This function is called to handle any MakeCurrent request. The vendor
     * library should deal with changing the current context. After the vendor
     * returns GLVND will send the reply.
     *
     * In addition, GLVND will call this function with any current contexts
     * when a client disconnects.
     *
     * To ensure that context tags are unique, libglvnd will select a context
     * tag and pass it to the vendor library.
     *
     * The vendor can use \c __GLXserverExports::getContextTagPrivate to look
     * up the private data pointer for \p oldContextTag.
     *
     * Likewise, the vendor can use \c __GLXserverExports::setContextTagPrivate
     * to assign a private data pointer to \p newContextTag.
     */
    int (* makeCurrent) (ClientPtr client,
        GLXContextTag oldContextTag,
        XID drawable,
        XID readdrawable,
        XID context,
        GLXContextTag newContextTag);
};

#if defined(__cplusplus)
}
#endif

#endif // GLXVENDORABI_H
