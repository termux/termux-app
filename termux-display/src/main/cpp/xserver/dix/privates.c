/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/*
 * Copyright © 2010, Keith Packard
 * Copyright © 2010, Jamey Sharp
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stddef.h>
#include "windowstr.h"
#include "resource.h"
#include "privates.h"
#include "gcstruct.h"
#include "cursorstr.h"
#include "colormapst.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "extnsionst.h"
#include "inputstr.h"

static DevPrivateSetRec global_keys[PRIVATE_LAST];

static const Bool xselinux_private[PRIVATE_LAST] = {
    [PRIVATE_SCREEN] = TRUE,
    [PRIVATE_CLIENT] = TRUE,
    [PRIVATE_WINDOW] = TRUE,
    [PRIVATE_PIXMAP] = TRUE,
    [PRIVATE_GC] = TRUE,
    [PRIVATE_CURSOR] = TRUE,
    [PRIVATE_COLORMAP] = TRUE,
    [PRIVATE_DEVICE] = TRUE,
    [PRIVATE_EXTENSION] = TRUE,
    [PRIVATE_SELECTION] = TRUE,
    [PRIVATE_PROPERTY] = TRUE,
    [PRIVATE_PICTURE] = TRUE,
    [PRIVATE_GLYPHSET] = TRUE,
};

static const char *key_names[PRIVATE_LAST] = {
    /* XSELinux uses the same private keys for numerous objects */
    [PRIVATE_XSELINUX] = "XSELINUX",

    /* Otherwise, you get a private in just the requested structure
     */
    /* These can have objects created before all of the keys are registered */
    [PRIVATE_SCREEN] = "SCREEN",
    [PRIVATE_EXTENSION] = "EXTENSION",
    [PRIVATE_COLORMAP] = "COLORMAP",
    [PRIVATE_DEVICE] = "DEVICE",

    /* These cannot have any objects before all relevant keys are registered */
    [PRIVATE_CLIENT] = "CLIENT",
    [PRIVATE_PROPERTY] = "PROPERTY",
    [PRIVATE_SELECTION] = "SELECTION",
    [PRIVATE_WINDOW] = "WINDOW",
    [PRIVATE_PIXMAP] = "PIXMAP",
    [PRIVATE_GC] = "GC",
    [PRIVATE_CURSOR] = "CURSOR",
    [PRIVATE_CURSOR_BITS] = "CURSOR_BITS",

    /* extension privates */
    [PRIVATE_GLYPH] = "GLYPH",
    [PRIVATE_GLYPHSET] = "GLYPHSET",
    [PRIVATE_PICTURE] = "PICTURE",
    [PRIVATE_SYNC_FENCE] = "SYNC_FENCE",
};

static const Bool screen_specific_private[PRIVATE_LAST] = {
    [PRIVATE_SCREEN] = FALSE,
    [PRIVATE_CLIENT] = FALSE,
    [PRIVATE_WINDOW] = TRUE,
    [PRIVATE_PIXMAP] = TRUE,
    [PRIVATE_GC] = TRUE,
    [PRIVATE_CURSOR] = FALSE,
    [PRIVATE_COLORMAP] = FALSE,
    [PRIVATE_DEVICE] = FALSE,
    [PRIVATE_EXTENSION] = FALSE,
    [PRIVATE_SELECTION] = FALSE,
    [PRIVATE_PROPERTY] = FALSE,
    [PRIVATE_PICTURE] = TRUE,
    [PRIVATE_GLYPHSET] = FALSE,
};

typedef Bool (*FixupFunc) (PrivatePtr *privates, int offset, unsigned bytes);

typedef enum { FixupMove, FixupRealloc } FixupType;

static Bool
dixReallocPrivates(PrivatePtr *privates, int old_offset, unsigned bytes)
{
    void *new_privates;

    new_privates = realloc(*privates, old_offset + bytes);
    if (!new_privates)
        return FALSE;
    memset((char *) new_privates + old_offset, '\0', bytes);
    *privates = new_privates;
    return TRUE;
}

static Bool
dixMovePrivates(PrivatePtr *privates, int new_offset, unsigned bytes)
{
    memmove((char *) *privates + bytes, *privates, new_offset - bytes);
    memset(*privates, '\0', bytes);
    return TRUE;
}

static Bool
fixupOneScreen(ScreenPtr pScreen, FixupFunc fixup, unsigned bytes)
{
    uintptr_t       old;
    char            *new;
    DevPrivateKey   *keyp, key;
    DevPrivateType  type;
    int             size;

    old = (uintptr_t) pScreen->devPrivates;
    size = global_keys[PRIVATE_SCREEN].offset;
    if (!fixup (&pScreen->devPrivates, size, bytes))
        return FALSE;

    /* Screen privates can contain screen-specific private keys
     * for other types. When they move, the linked list we use to
     * track them gets scrambled. Fix that by computing the change
     * in the location of each private adjusting our linked list
     * pointers to match
     */

    new = (char *) pScreen->devPrivates;

    /* Moving means everyone shifts up in the privates by 'bytes' amount,
     * realloc means the base pointer moves
     */
    if (fixup == dixMovePrivates)
        new += bytes;

    if ((uintptr_t) new != old) {
        for (type = PRIVATE_XSELINUX; type < PRIVATE_LAST; type++)

            /* Walk the privates list, being careful as the
             * pointers are scrambled before we patch them.
             */
            for (keyp = &pScreen->screenSpecificPrivates[type].key;
                 (key = *keyp) != NULL;
                 keyp = &key->next)
            {

                /* Only mangle things if the private structure
                 * is contained within the allocation. Privates
                 * stored elsewhere will be left alone
                 */
                if (old <= (uintptr_t) key && (uintptr_t) key < old + size)
                {
                    /* Compute new location of key (deriving from the new
                     * allocation to avoid UB) */
                    key = (DevPrivateKey) (new + ((uintptr_t) key - old));

                    /* Patch the list */
                    *keyp = key;
                }
            }
    }
    return TRUE;
}

static Bool
fixupScreens(FixupFunc fixup, unsigned bytes)
{
    int s;

    for (s = 0; s < screenInfo.numScreens; s++)
        if (!fixupOneScreen (screenInfo.screens[s], fixup, bytes))
            return FALSE;

    for (s = 0; s < screenInfo.numGPUScreens; s++)
        if (!fixupOneScreen (screenInfo.gpuscreens[s], fixup, bytes))
            return FALSE;
    return TRUE;
}

static Bool
fixupServerClient(FixupFunc fixup, unsigned bytes)
{
    if (serverClient)
        return fixup(&serverClient->devPrivates, global_keys[PRIVATE_CLIENT].offset,
                     bytes);
    return TRUE;
}

static Bool
fixupExtensions(FixupFunc fixup, unsigned bytes)
{
    unsigned char major;
    ExtensionEntry *extension;

    for (major = EXTENSION_BASE; (extension = GetExtensionEntry(major));
         major++)
        if (!fixup
            (&extension->devPrivates, global_keys[PRIVATE_EXTENSION].offset, bytes))
            return FALSE;
    return TRUE;
}

static Bool
fixupDefaultColormaps(FixupFunc fixup, unsigned bytes)
{
    int s;

    for (s = 0; s < screenInfo.numScreens; s++) {
        ColormapPtr cmap;

        dixLookupResourceByType((void **) &cmap,
                                screenInfo.screens[s]->defColormap, RT_COLORMAP,
                                serverClient, DixCreateAccess);
        if (cmap &&
            !fixup(&cmap->devPrivates, screenInfo.screens[s]->screenSpecificPrivates[PRIVATE_COLORMAP].offset, bytes))
            return FALSE;
    }
    return TRUE;
}

static Bool
fixupDeviceList(DeviceIntPtr device, FixupFunc fixup, unsigned bytes)
{
    while (device) {
        if (!fixup(&device->devPrivates, global_keys[PRIVATE_DEVICE].offset, bytes))
            return FALSE;
        device = device->next;
    }
    return TRUE;
}

static Bool
fixupDevices(FixupFunc fixup, unsigned bytes)
{
    return (fixupDeviceList(inputInfo.devices, fixup, bytes) &&
            fixupDeviceList(inputInfo.off_devices, fixup, bytes));
}

static Bool (*const allocated_early[PRIVATE_LAST]) (FixupFunc, unsigned) = {
    [PRIVATE_SCREEN] = fixupScreens,
    [PRIVATE_CLIENT] = fixupServerClient,
    [PRIVATE_EXTENSION] = fixupExtensions,
    [PRIVATE_COLORMAP] = fixupDefaultColormaps,
    [PRIVATE_DEVICE] = fixupDevices,
};

static void
grow_private_set(DevPrivateSetPtr set, unsigned bytes)
{
    DevPrivateKey       k;

    for (k = set->key; k; k = k->next)
        k->offset += bytes;
    set->offset += bytes;
}

static void
grow_screen_specific_set(DevPrivateType type, unsigned bytes)
{
    int s;

    /* Update offsets for all screen-specific keys */
    for (s = 0; s < screenInfo.numScreens; s++) {
        ScreenPtr       pScreen = screenInfo.screens[s];

        grow_private_set(&pScreen->screenSpecificPrivates[type], bytes);
    }
    for (s = 0; s < screenInfo.numGPUScreens; s++) {
        ScreenPtr       pScreen = screenInfo.gpuscreens[s];

        grow_private_set(&pScreen->screenSpecificPrivates[type], bytes);
    }
}

/*
 * Register a private key. This takes the type of object the key will
 * be used with, which may be PRIVATE_ALL indicating that this key
 * will be used with all of the private objects. If 'size' is
 * non-zero, then the specified amount of space will be allocated in
 * the private storage. Otherwise, space for a single pointer will
 * be allocated which can be set with dixSetPrivate
 */
Bool
dixRegisterPrivateKey(DevPrivateKey key, DevPrivateType type, unsigned size)
{
    DevPrivateType t;
    int offset;
    unsigned bytes;

    if (key->initialized) {
        assert(size == key->size);
        return TRUE;
    }

    /* Compute required space */
    bytes = size;
    if (size == 0)
        bytes = sizeof(void *);

    /* align to pointer size */
    bytes = (bytes + sizeof(void *) - 1) & ~(sizeof(void *) - 1);

    /* Update offsets for all affected keys */
    if (type == PRIVATE_XSELINUX) {

        /* Resize if we can, or make sure nothing's allocated if we can't
         */
        for (t = PRIVATE_XSELINUX; t < PRIVATE_LAST; t++)
            if (xselinux_private[t]) {
                if (!allocated_early[t])
                    assert(!global_keys[t].created);
                else if (!allocated_early[t] (dixReallocPrivates, bytes))
                    return FALSE;
            }

        /* Move all existing keys up in the privates space to make
         * room for this new global key
         */
        for (t = PRIVATE_XSELINUX; t < PRIVATE_LAST; t++) {
            if (xselinux_private[t]) {
                grow_private_set(&global_keys[t], bytes);
                grow_screen_specific_set(t, bytes);
                if (allocated_early[t])
                    allocated_early[t] (dixMovePrivates, bytes);
            }

        }

        offset = 0;
    }
    else {
        /* Resize if we can, or make sure nothing's allocated if we can't */
        if (!allocated_early[type])
            assert(!global_keys[type].created);
        else if (!allocated_early[type] (dixReallocPrivates, bytes))
            return FALSE;
        offset = global_keys[type].offset;
        global_keys[type].offset += bytes;
        grow_screen_specific_set(type, bytes);
    }

    /* Setup this key */
    key->offset = offset;
    key->size = size;
    key->initialized = TRUE;
    key->type = type;
    key->allocated = FALSE;
    key->next = global_keys[type].key;
    global_keys[type].key = key;

    return TRUE;
}

Bool
dixRegisterScreenPrivateKey(DevScreenPrivateKey screenKey, ScreenPtr pScreen,
                            DevPrivateType type, unsigned size)
{
    DevPrivateKey key;

    if (!dixRegisterPrivateKey(&screenKey->screenKey, PRIVATE_SCREEN, 0))
        return FALSE;
    key = dixGetPrivate(&pScreen->devPrivates, &screenKey->screenKey);
    if (key != NULL) {
        assert(key->size == size);
        assert(key->type == type);
        return TRUE;
    }
    key = calloc(sizeof(DevPrivateKeyRec), 1);
    if (!key)
        return FALSE;
    if (!dixRegisterPrivateKey(key, type, size)) {
        free(key);
        return FALSE;
    }
    key->allocated = TRUE;
    dixSetPrivate(&pScreen->devPrivates, &screenKey->screenKey, key);
    return TRUE;
}

DevPrivateKey
_dixGetScreenPrivateKey(const DevScreenPrivateKey key, ScreenPtr pScreen)
{
    return dixGetPrivate(&pScreen->devPrivates, &key->screenKey);
}

/*
 * Initialize privates by zeroing them
 */
void
_dixInitPrivates(PrivatePtr *privates, void *addr, DevPrivateType type)
{
    assert (!screen_specific_private[type]);

    global_keys[type].created++;
    if (xselinux_private[type])
        global_keys[PRIVATE_XSELINUX].created++;
    if (global_keys[type].offset == 0)
        addr = 0;
    *privates = addr;
    if (addr)
        memset(addr, '\0', global_keys[type].offset);
}

/*
 * Clean up privates
 */
void
_dixFiniPrivates(PrivatePtr privates, DevPrivateType type)
{
    global_keys[type].created--;
    if (xselinux_private[type])
        global_keys[PRIVATE_XSELINUX].created--;
}

/*
 * Allocate new object with privates.
 *
 * This is expected to be invoked from the
 * dixAllocateObjectWithPrivates macro
 */
void *
_dixAllocateObjectWithPrivates(unsigned baseSize, unsigned clear,
                               unsigned offset, DevPrivateType type)
{
    unsigned totalSize;
    void *object;
    PrivatePtr privates;
    PrivatePtr *devPrivates;

    assert(type > PRIVATE_SCREEN);
    assert(type < PRIVATE_LAST);
    assert(!screen_specific_private[type]);

    /* round up so that void * is aligned */
    baseSize = (baseSize + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
    totalSize = baseSize + global_keys[type].offset;
    object = malloc(totalSize);
    if (!object)
        return NULL;

    memset(object, '\0', clear);
    privates = (PrivatePtr) (((char *) object) + baseSize);
    devPrivates = (PrivatePtr *) ((char *) object + offset);

    _dixInitPrivates(devPrivates, privates, type);

    return object;
}

/*
 * Allocate privates separately from containing object.
 * Used for clients and screens.
 */
Bool
dixAllocatePrivates(PrivatePtr *privates, DevPrivateType type)
{
    unsigned size;
    PrivatePtr p;

    assert(type > PRIVATE_XSELINUX);
    assert(type < PRIVATE_LAST);
    assert(!screen_specific_private[type]);

    size = global_keys[type].offset;
    if (!size) {
        p = NULL;
    }
    else {
        if (!(p = malloc(size)))
            return FALSE;
    }

    _dixInitPrivates(privates, p, type);
    ++global_keys[type].allocated;

    return TRUE;
}

/*
 * Free an object that has privates
 *
 * This is expected to be invoked from the
 * dixFreeObjectWithPrivates macro
 */
void
_dixFreeObjectWithPrivates(void *object, PrivatePtr privates,
                           DevPrivateType type)
{
    _dixFiniPrivates(privates, type);
    free(object);
}

/*
 * Called to free screen or client privates
 */
void
dixFreePrivates(PrivatePtr privates, DevPrivateType type)
{
    _dixFiniPrivates(privates, type);
    --global_keys[type].allocated;
    free(privates);
}

/*
 * Return size of privates for the specified type
 */
extern _X_EXPORT int
dixPrivatesSize(DevPrivateType type)
{
    assert(type >= PRIVATE_SCREEN);
    assert(type < PRIVATE_LAST);
    assert (!screen_specific_private[type]);

    return global_keys[type].offset;
}

/* Table of devPrivates offsets */
static const int offsets[] = {
    -1,                         /* RT_NONE */
    offsetof(WindowRec, devPrivates),   /* RT_WINDOW */
    offsetof(PixmapRec, devPrivates),   /* RT_PIXMAP */
    offsetof(GC, devPrivates),  /* RT_GC */
    -1,                         /* RT_FONT */
    offsetof(CursorRec, devPrivates),   /* RT_CURSOR */
    offsetof(ColormapRec, devPrivates), /* RT_COLORMAP */
};

int
dixLookupPrivateOffset(RESTYPE type)
{
    /*
     * Special kludge for DBE which registers a new resource type that
     * points at pixmaps (thanks, DBE)
     */
    if (type & RC_DRAWABLE) {
        if (type == RT_WINDOW)
            return offsets[RT_WINDOW & TypeMask];
        else
            return offsets[RT_PIXMAP & TypeMask];
    }
    type = type & TypeMask;
    if (type < ARRAY_SIZE(offsets))
        return offsets[type];
    return -1;
}

/*
 * Screen-specific privates
 */

extern _X_EXPORT Bool
dixRegisterScreenSpecificPrivateKey(ScreenPtr pScreen, DevPrivateKey key,
                                    DevPrivateType type, unsigned size)
{
    int offset;
    unsigned bytes;

    if (!screen_specific_private[type])
        FatalError("Attempt to allocate screen-specific private storage for type %s\n",
                   key_names[type]);

    if (key->initialized) {
        assert(size == key->size);
        return TRUE;
    }

    /* Compute required space */
    bytes = size;
    if (size == 0)
        bytes = sizeof(void *);

    /* align to void * size */
    bytes = (bytes + sizeof(void *) - 1) & ~(sizeof(void *) - 1);

    assert (!allocated_early[type]);
    assert (!pScreen->screenSpecificPrivates[type].created);
    offset = pScreen->screenSpecificPrivates[type].offset;
    pScreen->screenSpecificPrivates[type].offset += bytes;

    /* Setup this key */
    key->offset = offset;
    key->size = size;
    key->initialized = TRUE;
    key->type = type;
    key->allocated = FALSE;
    key->next = pScreen->screenSpecificPrivates[type].key;
    pScreen->screenSpecificPrivates[type].key = key;

    return TRUE;
}

/* Clean up screen-specific privates before CloseScreen */
void
dixFreeScreenSpecificPrivates(ScreenPtr pScreen)
{
    DevPrivateType t;

    for (t = PRIVATE_XSELINUX; t < PRIVATE_LAST; t++) {
        DevPrivateKey key;

        for (key = pScreen->screenSpecificPrivates[t].key; key; key = key->next) {
            key->initialized = FALSE;
        }
    }
}

/* Initialize screen-specific privates in AddScreen */
void
dixInitScreenSpecificPrivates(ScreenPtr pScreen)
{
    DevPrivateType      t;

    for (t = PRIVATE_XSELINUX; t < PRIVATE_LAST; t++)
        pScreen->screenSpecificPrivates[t].offset = global_keys[t].offset;
}

/* Initialize screen-specific privates in AddScreen */
void
_dixInitScreenPrivates(ScreenPtr pScreen, PrivatePtr *privates, void *addr, DevPrivateType type)
{
    int privates_size;
    assert (screen_specific_private[type]);

    if (pScreen) {
        privates_size = pScreen->screenSpecificPrivates[type].offset;
        pScreen->screenSpecificPrivates[type].created++;
    }
    else
        privates_size = global_keys[type].offset;

    global_keys[type].created++;
    if (xselinux_private[type])
        global_keys[PRIVATE_XSELINUX].created++;
    if (privates_size == 0)
        addr = 0;
    *privates = addr;
    if (addr)
        memset(addr, '\0', privates_size);
}

void *
_dixAllocateScreenObjectWithPrivates(ScreenPtr pScreen,
                                     unsigned baseSize,
                                     unsigned clear,
                                     unsigned offset,
                                     DevPrivateType type)
{
    unsigned totalSize;
    void *object;
    PrivatePtr privates;
    PrivatePtr *devPrivates;
    int privates_size;

    assert(type > PRIVATE_SCREEN);
    assert(type < PRIVATE_LAST);
    assert (screen_specific_private[type]);

    if (pScreen)
        privates_size = pScreen->screenSpecificPrivates[type].offset;
    else
        privates_size = global_keys[type].offset;
    /* round up so that pointer is aligned */
    baseSize = (baseSize + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
    totalSize = baseSize + privates_size;
    object = malloc(totalSize);
    if (!object)
        return NULL;

    memset(object, '\0', clear);
    privates = (PrivatePtr) (((char *) object) + baseSize);
    devPrivates = (PrivatePtr *) ((char *) object + offset);

    _dixInitScreenPrivates(pScreen, devPrivates, privates, type);

    return object;
}

int
dixScreenSpecificPrivatesSize(ScreenPtr pScreen, DevPrivateType type)
{
    assert(type >= PRIVATE_SCREEN);
    assert(type < PRIVATE_LAST);

    if (screen_specific_private[type])
        return pScreen->screenSpecificPrivates[type].offset;
    else
        return global_keys[type].offset;
}

void
dixPrivateUsage(void)
{
    int objects = 0;
    int bytes = 0;
    int alloc = 0;
    DevPrivateType t;

    for (t = PRIVATE_XSELINUX + 1; t < PRIVATE_LAST; t++) {
        if (global_keys[t].offset) {
            ErrorF
                ("%s: %d objects of %d bytes = %d total bytes %d private allocs\n",
                 key_names[t], global_keys[t].created, global_keys[t].offset,
                 global_keys[t].created * global_keys[t].offset, global_keys[t].allocated);
            bytes += global_keys[t].created * global_keys[t].offset;
            objects += global_keys[t].created;
            alloc += global_keys[t].allocated;
        }
    }
    ErrorF("TOTAL: %d objects, %d bytes, %d allocs\n", objects, bytes, alloc);
}

void
dixResetPrivates(void)
{
    DevPrivateType t;

    for (t = PRIVATE_XSELINUX; t < PRIVATE_LAST; t++) {
        DevPrivateKey key, next;

        for (key = global_keys[t].key; key; key = next) {
            next = key->next;
            key->offset = 0;
            key->initialized = FALSE;
            key->size = 0;
            key->type = 0;
            if (key->allocated)
                free(key);
        }
        if (global_keys[t].created) {
            ErrorF("%d %ss still allocated at reset\n",
                   global_keys[t].created, key_names[t]);
            dixPrivateUsage();
        }
        global_keys[t].key = NULL;
        global_keys[t].offset = 0;
        global_keys[t].created = 0;
        global_keys[t].allocated = 0;
    }
}

Bool
dixPrivatesCreated(DevPrivateType type)
{
    if (global_keys[type].created)
        return TRUE;
    else
        return FALSE;
}
