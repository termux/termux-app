/***********************************************************

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef PRIVATES_H
#define PRIVATES_H 1

#include <X11/Xdefs.h>
#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <assert.h>
#include "misc.h"

/*****************************************************************
 * STUFF FOR PRIVATES
 *****************************************************************/

typedef struct _Private PrivateRec, *PrivatePtr;

typedef enum {
    /* XSELinux uses the same private keys for numerous objects */
    PRIVATE_XSELINUX,

    /* Otherwise, you get a private in just the requested structure
     */
    /* These can have objects created before all of the keys are registered */
    PRIVATE_SCREEN,
    PRIVATE_EXTENSION,
    PRIVATE_COLORMAP,
    PRIVATE_DEVICE,

    /* These cannot have any objects before all relevant keys are registered */
    PRIVATE_CLIENT,
    PRIVATE_PROPERTY,
    PRIVATE_SELECTION,
    PRIVATE_WINDOW,
    PRIVATE_PIXMAP,
    PRIVATE_GC,
    PRIVATE_CURSOR,
    PRIVATE_CURSOR_BITS,

    /* extension privates */
    PRIVATE_GLYPH,
    PRIVATE_GLYPHSET,
    PRIVATE_PICTURE,
    PRIVATE_SYNC_FENCE,

    /* last private type */
    PRIVATE_LAST,
} DevPrivateType;

typedef struct _DevPrivateKeyRec {
    int offset;
    int size;
    Bool initialized;
    Bool allocated;
    DevPrivateType type;
    struct _DevPrivateKeyRec *next;
} DevPrivateKeyRec, *DevPrivateKey;

typedef struct _DevPrivateSetRec {
    DevPrivateKey key;
    unsigned offset;
    int created;
    int allocated;
} DevPrivateSetRec, *DevPrivateSetPtr;

typedef struct _DevScreenPrivateKeyRec {
    DevPrivateKeyRec screenKey;
} DevScreenPrivateKeyRec, *DevScreenPrivateKey;

/*
 * Let drivers know how to initialize private keys
 */

#define HAS_DEVPRIVATEKEYREC		1
#define HAS_DIXREGISTERPRIVATEKEY	1

/*
 * Register a new private index for the private type.
 *
 * This initializes the specified key and optionally requests pre-allocated
 * private space for your driver/module. If you request no extra space, you
 * may set and get a single pointer value using this private key. Otherwise,
 * you can get the address of the extra space and store whatever data you like
 * there.
 *
 * You may call dixRegisterPrivateKey more than once on the same key, but the
 * size and type must match or the server will abort.
 *
 * dixRegisterPrivateKey returns FALSE if it fails to allocate memory
 * during its operation.
 */
extern _X_EXPORT Bool
 dixRegisterPrivateKey(DevPrivateKey key, DevPrivateType type, unsigned size);

/*
 * Check whether a private key has been registered
 */
static inline Bool
dixPrivateKeyRegistered(DevPrivateKey key)
{
    return key->initialized;
}

/*
 * Get the address of the private storage.
 *
 * For keys with pre-defined storage, this gets the base of that storage
 * Otherwise, it returns the place where the private pointer is stored.
 */
static inline void *
dixGetPrivateAddr(PrivatePtr *privates, const DevPrivateKey key)
{
    assert(key->initialized);
    return (char *) (*privates) + key->offset;
}

/*
 * Fetch a private pointer stored in the object
 *
 * Returns the pointer stored with dixSetPrivate.
 * This must only be used with keys that have
 * no pre-defined storage
 */
static inline void *
dixGetPrivate(PrivatePtr *privates, const DevPrivateKey key)
{
    assert(key->size == 0);
    return *(void **) dixGetPrivateAddr(privates, key);
}

/*
 * Associate 'val' with 'key' in 'privates' so that later calls to
 * dixLookupPrivate(privates, key) will return 'val'.
 */
static inline void
dixSetPrivate(PrivatePtr *privates, const DevPrivateKey key, void *val)
{
    assert(key->size == 0);
    *(void **) dixGetPrivateAddr(privates, key) = val;
}

#include "dix.h"
#include "resource.h"

/*
 * Lookup a pointer to the private record.
 *
 * For privates with defined storage, return the address of the
 * storage. For privates without defined storage, return the pointer
 * contents
 */
static inline void *
dixLookupPrivate(PrivatePtr *privates, const DevPrivateKey key)
{
    if (key->size)
        return dixGetPrivateAddr(privates, key);
    else
        return dixGetPrivate(privates, key);
}

/*
 * Look up the address of the pointer to the storage
 *
 * This returns the place where the private pointer is stored,
 * which is only valid for privates without predefined storage.
 */
static inline void **
dixLookupPrivateAddr(PrivatePtr *privates, const DevPrivateKey key)
{
    assert(key->size == 0);
    return (void **) dixGetPrivateAddr(privates, key);
}

extern _X_EXPORT Bool

dixRegisterScreenPrivateKey(DevScreenPrivateKey key, ScreenPtr pScreen,
                            DevPrivateType type, unsigned size);

extern _X_EXPORT DevPrivateKey
 _dixGetScreenPrivateKey(const DevScreenPrivateKey key, ScreenPtr pScreen);

static inline void *
dixGetScreenPrivateAddr(PrivatePtr *privates, const DevScreenPrivateKey key,
                        ScreenPtr pScreen)
{
    return dixGetPrivateAddr(privates, _dixGetScreenPrivateKey(key, pScreen));
}

static inline void *
dixGetScreenPrivate(PrivatePtr *privates, const DevScreenPrivateKey key,
                    ScreenPtr pScreen)
{
    return dixGetPrivate(privates, _dixGetScreenPrivateKey(key, pScreen));
}

static inline void
dixSetScreenPrivate(PrivatePtr *privates, const DevScreenPrivateKey key,
                    ScreenPtr pScreen, void *val)
{
    dixSetPrivate(privates, _dixGetScreenPrivateKey(key, pScreen), val);
}

static inline void *
dixLookupScreenPrivate(PrivatePtr *privates, const DevScreenPrivateKey key,
                       ScreenPtr pScreen)
{
    return dixLookupPrivate(privates, _dixGetScreenPrivateKey(key, pScreen));
}

static inline void **
dixLookupScreenPrivateAddr(PrivatePtr *privates, const DevScreenPrivateKey key,
                           ScreenPtr pScreen)
{
    return dixLookupPrivateAddr(privates,
                                _dixGetScreenPrivateKey(key, pScreen));
}

/*
 * These functions relate to allocations related to a specific screen;
 * space will only be available for objects allocated for use on that
 * screen. As such, only objects which are related directly to a specific
 * screen are candidates for allocation this way, this includes
 * windows, pixmaps, gcs, pictures and colormaps. This key is
 * used just like any other key using dixGetPrivate and friends.
 *
 * This is distinctly different from the ScreenPrivateKeys above which
 * allocate space in global objects like cursor bits for a specific
 * screen, allowing multiple screen-related chunks of storage in a
 * single global object.
 */

#define HAVE_SCREEN_SPECIFIC_PRIVATE_KEYS       1

extern _X_EXPORT Bool
dixRegisterScreenSpecificPrivateKey(ScreenPtr pScreen, DevPrivateKey key,
                                    DevPrivateType type, unsigned size);

/* Clean up screen-specific privates before CloseScreen */
extern void
dixFreeScreenSpecificPrivates(ScreenPtr pScreen);

/* Initialize screen-specific privates in AddScreen */
extern void
dixInitScreenSpecificPrivates(ScreenPtr pScreen);

/* is this private created - so hotplug can avoid crashing */
Bool dixPrivatesCreated(DevPrivateType type);

extern _X_EXPORT void *
_dixAllocateScreenObjectWithPrivates(ScreenPtr pScreen,
                                     unsigned size,
                                     unsigned clear,
                                     unsigned offset,
                                     DevPrivateType type);

#define dixAllocateScreenObjectWithPrivates(s, t, type) _dixAllocateScreenObjectWithPrivates(s, sizeof(t), sizeof(t), offsetof(t, devPrivates), type)

extern _X_EXPORT int
dixScreenSpecificPrivatesSize(ScreenPtr pScreen, DevPrivateType type);

extern _X_EXPORT void
_dixInitScreenPrivates(ScreenPtr pScreen, PrivatePtr *privates, void *addr, DevPrivateType type);

#define dixInitScreenPrivates(s, o, v, type) _dixInitScreenPrivates(s, &(o)->devPrivates, (v), type);

/*
 * Allocates private data separately from main object.
 *
 * For objects created during server initialization, this allows those
 * privates to be re-allocated as new private keys are registered.
 *
 * This includes screens, the serverClient, default colormaps and
 * extensions entries.
 */
extern _X_EXPORT Bool
 dixAllocatePrivates(PrivatePtr *privates, DevPrivateType type);

/*
 * Frees separately allocated private data
 */
extern _X_EXPORT void
 dixFreePrivates(PrivatePtr privates, DevPrivateType type);

/*
 * Initialize privates by zeroing them
 */
extern _X_EXPORT void
_dixInitPrivates(PrivatePtr *privates, void *addr, DevPrivateType type);

#define dixInitPrivates(o, v, type) _dixInitPrivates(&(o)->devPrivates, (v), type);

/*
 * Clean up privates
 */
extern _X_EXPORT void
 _dixFiniPrivates(PrivatePtr privates, DevPrivateType type);

#define dixFiniPrivates(o,t)	_dixFiniPrivates((o)->devPrivates,t)

/*
 * Allocates private data at object creation time. Required
 * for almost all objects, except for the list described
 * above for dixAllocatePrivates.
 */
extern _X_EXPORT void *_dixAllocateObjectWithPrivates(unsigned size,
                                                      unsigned clear,
                                                      unsigned offset,
                                                      DevPrivateType type);

#define dixAllocateObjectWithPrivates(t, type) (t *) _dixAllocateObjectWithPrivates(sizeof(t), sizeof(t), offsetof(t, devPrivates), type)

extern _X_EXPORT void

_dixFreeObjectWithPrivates(void *object, PrivatePtr privates,
                           DevPrivateType type);

#define dixFreeObjectWithPrivates(o,t) _dixFreeObjectWithPrivates(o, (o)->devPrivates, t)

/*
 * Return size of privates for the specified type
 */
extern _X_EXPORT int
 dixPrivatesSize(DevPrivateType type);

/*
 * Dump out private stats to ErrorF
 */
extern void
 dixPrivateUsage(void);

/*
 * Resets the privates subsystem.  dixResetPrivates is called from the main loop
 * before each server generation.  This function must only be called by main().
 */
extern _X_EXPORT void
 dixResetPrivates(void);

/*
 * Looks up the offset where the devPrivates field is located.
 *
 * Returns -1 if the specified resource has no dev privates.
 * The position of the devPrivates field varies by structure
 * and calling code might only know the resource type, not the
 * structure definition.
 */
extern _X_EXPORT int
 dixLookupPrivateOffset(RESTYPE type);

/*
 * Convenience macro for adding an offset to an object pointer
 * when making a call to one of the devPrivates functions
 */
#define DEVPRIV_AT(ptr, offset) ((PrivatePtr *)((char *)(ptr) + offset))

#endif                          /* PRIVATES_H */
