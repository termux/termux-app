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

#ifndef VND_SERVER_VENDOR_H
#define VND_SERVER_VENDOR_H

#include <dix-config.h>

#include "glxvndabi.h"
#include "list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Info related to a single vendor library.
 */
struct GlxServerVendorRec {
    GlxServerImports glxvc;

    struct xorg_list entry;
};

/**
 * A linked list of vendor libraries.
 *
 * Note that this list only includes vendor libraries that were successfully
 * initialized.
 */
extern struct xorg_list GlxVendorList;

GlxServerVendor *GlxCreateVendor(const GlxServerImports *imports);
void GlxDestroyVendor(GlxServerVendor *vendor);

void GlxVendorExtensionReset(const ExtensionEntry *extEntry);

#if defined(__cplusplus)
}
#endif

#endif // VND_SERVER_VENDOR_H
