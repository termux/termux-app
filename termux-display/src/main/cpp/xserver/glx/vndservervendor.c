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

#include "vndservervendor.h"

struct xorg_list GlxVendorList = { &GlxVendorList, &GlxVendorList };

GlxServerVendor *GlxCreateVendor(const GlxServerImports *imports)
{
    GlxServerVendor *vendor = NULL;

    if (imports == NULL) {
        ErrorF("GLX: Vendor library did not provide an imports table\n");
        return NULL;
    }

    if (imports->extensionCloseDown == NULL
            || imports->handleRequest == NULL
            || imports->getDispatchAddress == NULL
            || imports->makeCurrent == NULL) {
        ErrorF("GLX: Vendor library is missing required callback functions.\n");
        return NULL;
    }

    vendor = (GlxServerVendor *) calloc(1, sizeof(GlxServerVendor));
    if (vendor == NULL) {
        ErrorF("GLX: Can't allocate vendor library.\n");
        return NULL;
    }
    memcpy(&vendor->glxvc, imports, sizeof(GlxServerImports));

    xorg_list_append(&vendor->entry, &GlxVendorList);
    return vendor;
}

void GlxDestroyVendor(GlxServerVendor *vendor)
{
    if (vendor != NULL) {
        xorg_list_del(&vendor->entry);
        free(vendor);
    }
}

void GlxVendorExtensionReset(const ExtensionEntry *extEntry)
{
    GlxServerVendor *vendor, *tempVendor;

    // TODO: Do we allow the driver to destroy a vendor library handle from
    // here?
    xorg_list_for_each_entry_safe(vendor, tempVendor, &GlxVendorList, entry) {
        if (vendor->glxvc.extensionCloseDown != NULL) {
            vendor->glxvc.extensionCloseDown(extEntry);
        }
    }

    // If the server is exiting instead of starting a new generation, then
    // free the remaining GlxServerVendor structs.
    //
    // XXX this used to be conditional on xf86ServerIsExiting, but it's
    // cleaner to just always create the vendor struct on every generation,
    // if nothing else so all ddxes get the same behavior.
    xorg_list_for_each_entry_safe(vendor, tempVendor, &GlxVendorList, entry) {
        GlxDestroyVendor(vendor);
    }
}
