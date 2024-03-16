/************************************************************

Author: Eamon Walsh <ewalsh@tycho.nsa.gov>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <selinux/label.h>

#include "registry.h"
#include "xselinuxint.h"

/* selection and property atom cache */
typedef struct {
    SELinuxObjectRec prp;
    SELinuxObjectRec sel;
} SELinuxAtomRec;

/* dynamic array */
typedef struct {
    unsigned size;
    void **array;
} SELinuxArrayRec;

/* labeling handle */
static struct selabel_handle *label_hnd;

/* Array of object classes indexed by resource type */
SELinuxArrayRec arr_types;

/* Array of event SIDs indexed by event type */
SELinuxArrayRec arr_events;

/* Array of property and selection SID structures */
SELinuxArrayRec arr_atoms;

/*
 * Dynamic array helpers
 */
static void *
SELinuxArrayGet(SELinuxArrayRec * rec, unsigned key)
{
    return (rec->size > key) ? rec->array[key] : 0;
}

static int
SELinuxArraySet(SELinuxArrayRec * rec, unsigned key, void *val)
{
    if (key >= rec->size) {
        /* Need to increase size of array */
        rec->array = reallocarray(rec->array, key + 1, sizeof(val));
        if (!rec->array)
            return FALSE;
        memset(rec->array + rec->size, 0, (key - rec->size + 1) * sizeof(val));
        rec->size = key + 1;
    }

    rec->array[key] = val;
    return TRUE;
}

static void
SELinuxArrayFree(SELinuxArrayRec * rec, int free_elements)
{
    if (free_elements) {
        unsigned i = rec->size;

        while (i)
            free(rec->array[--i]);
    }

    free(rec->array);
    rec->size = 0;
    rec->array = NULL;
}

/*
 * Looks up a name in the selection or property mappings
 */
static int
SELinuxAtomToSIDLookup(Atom atom, SELinuxObjectRec * obj, int map, int polymap)
{
    const char *name = NameForAtom(atom);
    char *ctx;
    int rc = Success;

    obj->poly = 1;

    /* Look in the mappings of names to contexts */
    if (selabel_lookup_raw(label_hnd, &ctx, name, map) == 0) {
        obj->poly = 0;
    }
    else if (errno != ENOENT) {
        ErrorF("SELinux: a property label lookup failed!\n");
        return BadValue;
    }
    else if (selabel_lookup_raw(label_hnd, &ctx, name, polymap) < 0) {
        ErrorF("SELinux: a property label lookup failed!\n");
        return BadValue;
    }

    /* Get a SID for context */
    if (avc_context_to_sid_raw(ctx, &obj->sid) < 0) {
        ErrorF("SELinux: a context_to_SID_raw call failed!\n");
        rc = BadAlloc;
    }

    freecon(ctx);
    return rc;
}

/*
 * Looks up the SID corresponding to the given property or selection atom
 */
int
SELinuxAtomToSID(Atom atom, int prop, SELinuxObjectRec ** obj_rtn)
{
    SELinuxAtomRec *rec;
    SELinuxObjectRec *obj;
    int rc, map, polymap;

    rec = SELinuxArrayGet(&arr_atoms, atom);
    if (!rec) {
        rec = calloc(1, sizeof(SELinuxAtomRec));
        if (!rec || !SELinuxArraySet(&arr_atoms, atom, rec))
            return BadAlloc;
    }

    if (prop) {
        obj = &rec->prp;
        map = SELABEL_X_PROP;
        polymap = SELABEL_X_POLYPROP;
    }
    else {
        obj = &rec->sel;
        map = SELABEL_X_SELN;
        polymap = SELABEL_X_POLYSELN;
    }

    if (!obj->sid) {
        rc = SELinuxAtomToSIDLookup(atom, obj, map, polymap);
        if (rc != Success)
            goto out;
    }

    *obj_rtn = obj;
    rc = Success;
 out:
    return rc;
}

/*
 * Looks up a SID for a selection/subject pair
 */
int
SELinuxSelectionToSID(Atom selection, SELinuxSubjectRec * subj,
                      security_id_t * sid_rtn, int *poly_rtn)
{
    int rc;
    SELinuxObjectRec *obj;
    security_id_t tsid;

    /* Get the default context and polyinstantiation bit */
    rc = SELinuxAtomToSID(selection, 0, &obj);
    if (rc != Success)
        return rc;

    /* Check for an override context next */
    if (subj->sel_use_sid) {
        tsid = subj->sel_use_sid;
        goto out;
    }

    tsid = obj->sid;

    /* Polyinstantiate if necessary to obtain the final SID */
    if (obj->poly && avc_compute_member(subj->sid, obj->sid,
                                        SECCLASS_X_SELECTION, &tsid) < 0) {
        ErrorF("SELinux: a compute_member call failed!\n");
        return BadValue;
    }
 out:
    *sid_rtn = tsid;
    if (poly_rtn)
        *poly_rtn = obj->poly;
    return Success;
}

/*
 * Looks up a SID for a property/subject pair
 */
int
SELinuxPropertyToSID(Atom property, SELinuxSubjectRec * subj,
                     security_id_t * sid_rtn, int *poly_rtn)
{
    int rc;
    SELinuxObjectRec *obj;
    security_id_t tsid, tsid2;

    /* Get the default context and polyinstantiation bit */
    rc = SELinuxAtomToSID(property, 1, &obj);
    if (rc != Success)
        return rc;

    /* Check for an override context next */
    if (subj->prp_use_sid) {
        tsid = subj->prp_use_sid;
        goto out;
    }

    /* Perform a transition */
    if (avc_compute_create(subj->sid, obj->sid, SECCLASS_X_PROPERTY, &tsid) < 0) {
        ErrorF("SELinux: a compute_create call failed!\n");
        return BadValue;
    }

    /* Polyinstantiate if necessary to obtain the final SID */
    if (obj->poly) {
        tsid2 = tsid;
        if (avc_compute_member(subj->sid, tsid2,
                               SECCLASS_X_PROPERTY, &tsid) < 0) {
            ErrorF("SELinux: a compute_member call failed!\n");
            return BadValue;
        }
    }
 out:
    *sid_rtn = tsid;
    if (poly_rtn)
        *poly_rtn = obj->poly;
    return Success;
}

/*
 * Looks up the SID corresponding to the given event type
 */
int
SELinuxEventToSID(unsigned type, security_id_t sid_of_window,
                  SELinuxObjectRec * sid_return)
{
    const char *name = LookupEventName(type);
    security_id_t sid;
    char *ctx;

    type &= 127;

    sid = SELinuxArrayGet(&arr_events, type);
    if (!sid) {
        /* Look in the mappings of event names to contexts */
        if (selabel_lookup_raw(label_hnd, &ctx, name, SELABEL_X_EVENT) < 0) {
            ErrorF("SELinux: an event label lookup failed!\n");
            return BadValue;
        }
        /* Get a SID for context */
        if (avc_context_to_sid_raw(ctx, &sid) < 0) {
            ErrorF("SELinux: a context_to_SID_raw call failed!\n");
            freecon(ctx);
            return BadAlloc;
        }
        freecon(ctx);
        /* Cache the SID value */
        if (!SELinuxArraySet(&arr_events, type, sid))
            return BadAlloc;
    }

    /* Perform a transition to obtain the final SID */
    if (avc_compute_create(sid_of_window, sid, SECCLASS_X_EVENT,
                           &sid_return->sid) < 0) {
        ErrorF("SELinux: a compute_create call failed!\n");
        return BadValue;
    }

    return Success;
}

int
SELinuxExtensionToSID(const char *name, security_id_t * sid_rtn)
{
    char *ctx;

    /* Look in the mappings of extension names to contexts */
    if (selabel_lookup_raw(label_hnd, &ctx, name, SELABEL_X_EXT) < 0) {
        ErrorF("SELinux: a property label lookup failed!\n");
        return BadValue;
    }
    /* Get a SID for context */
    if (avc_context_to_sid_raw(ctx, sid_rtn) < 0) {
        ErrorF("SELinux: a context_to_SID_raw call failed!\n");
        freecon(ctx);
        return BadAlloc;
    }
    freecon(ctx);
    return Success;
}

/*
 * Returns the object class corresponding to the given resource type.
 */
security_class_t
SELinuxTypeToClass(RESTYPE type)
{
    void *tmp;

    tmp = SELinuxArrayGet(&arr_types, type & TypeMask);
    if (!tmp) {
        unsigned long class = SECCLASS_X_RESOURCE;

        if (type & RC_DRAWABLE)
            class = SECCLASS_X_DRAWABLE;
        else if (type == RT_GC)
            class = SECCLASS_X_GC;
        else if (type == RT_FONT)
            class = SECCLASS_X_FONT;
        else if (type == RT_CURSOR)
            class = SECCLASS_X_CURSOR;
        else if (type == RT_COLORMAP)
            class = SECCLASS_X_COLORMAP;
        else {
            /* Need to do a string lookup */
            const char *str = LookupResourceName(type);

            if (!strcmp(str, "PICTURE"))
                class = SECCLASS_X_DRAWABLE;
            else if (!strcmp(str, "GLYPHSET"))
                class = SECCLASS_X_FONT;
        }

        tmp = (void *) class;
        SELinuxArraySet(&arr_types, type & TypeMask, tmp);
    }

    return (security_class_t) (unsigned long) tmp;
}

char *
SELinuxDefaultClientLabel(void)
{
    char *ctx;

    if (selabel_lookup_raw(label_hnd, &ctx, "remote", SELABEL_X_CLIENT) < 0)
        FatalError("SELinux: failed to look up remote-client context\n");

    return ctx;
}

void
SELinuxLabelInit(void)
{
    struct selinux_opt selabel_option = { SELABEL_OPT_VALIDATE, (char *) 1 };

    label_hnd = selabel_open(SELABEL_CTX_X, &selabel_option, 1);
    if (!label_hnd)
        FatalError("SELinux: Failed to open x_contexts mapping in policy\n");
}

void
SELinuxLabelReset(void)
{
    selabel_close(label_hnd);
    label_hnd = NULL;

    /* Free local state */
    SELinuxArrayFree(&arr_types, 0);
    SELinuxArrayFree(&arr_events, 0);
    SELinuxArrayFree(&arr_atoms, 1);
}
