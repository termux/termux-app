/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include "xkbcomp.h"
#include "misc.h"
#include "tokens.h"
#include "expr.h"
#include "vmod.h"
#include "indicators.h"
#include "action.h"
#include "compat.h"

/***====================================================================***/

#define	ReportIndicatorBadType(d,l,f,w)	\
		ReportBadType("indicator map",(f),\
		      	XkbAtomText((d),(l)->name,XkbMessage),(w))
#define	ReportIndicatorNotArray(d,l,f)	\
		ReportNotArray("indicator map",(f),\
			XkbAtomText((d),(l)->name,XkbMessage))

/***====================================================================***/

void
ClearIndicatorMapInfo(Display * dpy, LEDInfo * info)
{
    info->name = XkbInternAtom(dpy, "default", False);
    info->indicator = _LED_NotBound;
    info->flags = info->which_mods = info->real_mods = 0;
    info->vmods = 0;
    info->which_groups = info->groups = 0;
    info->ctrls = 0;
    return;
}

LEDInfo *
AddIndicatorMap(LEDInfo *oldLEDs, const LEDInfo *new)
{
    LEDInfo *old, *last;

    last = NULL;
    for (old = oldLEDs; old != NULL; old = (LEDInfo *) old->defs.next)
    {
        if (old->name == new->name)
        {
            unsigned collide;

            if ((old->real_mods == new->real_mods) &&
                (old->vmods == new->vmods) &&
                (old->groups == new->groups) &&
                (old->ctrls == new->ctrls) &&
                (old->which_mods == new->which_mods) &&
                (old->which_groups == new->which_groups))
            {
                old->defs.defined |= new->defs.defined;
                return oldLEDs;
            }
            if (new->defs.merge == MergeReplace)
            {
                CommonInfo *next = old->defs.next;
                if (((old->defs.fileID == new->defs.fileID)
                     && (warningLevel > 0)) || (warningLevel > 9))
                {
                    WARN("Map for indicator %s redefined\n",
                          XkbAtomText(NULL, old->name, XkbMessage));
                    ACTION("Earlier definition ignored\n");
                }
                *old = *new;
                old->defs.next = next;
                return oldLEDs;
            }
            collide = 0;
            if (UseNewField(_LED_Index, &old->defs, &new->defs, &collide))
            {
                old->indicator = new->indicator;
                old->defs.defined |= _LED_Index;
            }
            if (UseNewField(_LED_Mods, &old->defs, &new->defs, &collide))
            {
                old->which_mods = new->which_mods;
                old->real_mods = new->real_mods;
                old->vmods = new->vmods;
                old->defs.defined |= _LED_Mods;
            }
            if (UseNewField(_LED_Groups, &old->defs, &new->defs, &collide))
            {
                old->which_groups = new->which_groups;
                old->groups = new->groups;
                old->defs.defined |= _LED_Groups;
            }
            if (UseNewField(_LED_Ctrls, &old->defs, &new->defs, &collide))
            {
                old->ctrls = new->ctrls;
                old->defs.defined |= _LED_Ctrls;
            }
            if (UseNewField(_LED_Explicit, &old->defs, &new->defs, &collide))
            {
                old->flags &= ~XkbIM_NoExplicit;
                old->flags |= (new->flags & XkbIM_NoExplicit);
                old->defs.defined |= _LED_Explicit;
            }
            if (UseNewField(_LED_Automatic, &old->defs, &new->defs, &collide))
            {
                old->flags &= ~XkbIM_NoAutomatic;
                old->flags |= (new->flags & XkbIM_NoAutomatic);
                old->defs.defined |= _LED_Automatic;
            }
            if (UseNewField(_LED_DrivesKbd, &old->defs, &new->defs, &collide))
            {
                old->flags &= ~XkbIM_LEDDrivesKB;
                old->flags |= (new->flags & XkbIM_LEDDrivesKB);
                old->defs.defined |= _LED_DrivesKbd;
            }
            if (collide && (warningLevel > 0))
            {
                WARN("Map for indicator %s redefined\n",
                      XkbAtomText(NULL, old->name, XkbMessage));
                ACTION("Using %s definition for duplicate fields\n",
                        (new->defs.merge == MergeAugment ? "first" : "last"));
            }
            return oldLEDs;
        }
        if (old->defs.next == NULL)
            last = old;
    }
    /* new definition */
    old = malloc(sizeof(LEDInfo));
    if (!old)
    {
        WSGO("Couldn't allocate indicator map\n");
        ACTION("Map for indicator %s not compiled\n",
                XkbAtomText(NULL, new->name, XkbMessage));
        return NULL;
    }
    *old = *new;
    old->defs.next = NULL;
    if (last)
    {
        last->defs.next = &old->defs;
        return oldLEDs;
    }
    return old;
}

static LookupEntry modComponentNames[] = {
    {"base", XkbIM_UseBase}
    ,
    {"latched", XkbIM_UseLatched}
    ,
    {"locked", XkbIM_UseLocked}
    ,
    {"effective", XkbIM_UseEffective}
    ,
    {"compat", XkbIM_UseCompat}
    ,
    {"any", XkbIM_UseAnyMods}
    ,
    {"none", 0}
    ,
    {NULL, 0}
};
static LookupEntry groupComponentNames[] = {
    {"base", XkbIM_UseBase}
    ,
    {"latched", XkbIM_UseLatched}
    ,
    {"locked", XkbIM_UseLocked}
    ,
    {"effective", XkbIM_UseEffective}
    ,
    {"any", XkbIM_UseAnyGroup}
    ,
    {"none", 0}
    ,
    {NULL, 0}
};

int
SetIndicatorMapField(LEDInfo *led, XkbDescPtr xkb, const char *field,
                     const ExprDef *arrayNdx, const ExprDef *value)
{
    ExprResult rtrn;
    Bool ok;

    ok = True;
    if ((uStrCaseCmp(field, "modifiers") == 0)
        || (uStrCaseCmp(field, "mods") == 0))
    {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(xkb->dpy, led, field);
        if (!ExprResolveModMask(value, &rtrn, LookupVModMask, (XPointer) xkb))
            return ReportIndicatorBadType(xkb->dpy, led, field,
                                          "modifier mask");
        led->real_mods = rtrn.uval & 0xff;
        led->vmods = (rtrn.uval >> 8) & 0xff;
        led->defs.defined |= _LED_Mods;
    }
    else if (uStrCaseCmp(field, "groups") == 0)
    {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(xkb->dpy, led, field);
        if (!ExprResolveMask
            (value, &rtrn, SimpleLookup, (XPointer) xkbcomp_groupNames))
            return ReportIndicatorBadType(xkb->dpy, led, field, "group mask");
        led->groups = rtrn.uval;
        led->defs.defined |= _LED_Groups;
    }
    else if ((uStrCaseCmp(field, "controls") == 0) ||
             (uStrCaseCmp(field, "ctrls") == 0))
    {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(xkb->dpy, led, field);
        if (!ExprResolveMask
            (value, &rtrn, SimpleLookup, (XPointer) ctrlNames))
            return ReportIndicatorBadType(xkb->dpy, led, field,
                                          "controls mask");
        led->ctrls = rtrn.uval;
        led->defs.defined |= _LED_Ctrls;
    }
    else if (uStrCaseCmp(field, "allowexplicit") == 0)
    {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(xkb->dpy, led, field);
        if (!ExprResolveBoolean(value, &rtrn, NULL, NULL))
            return ReportIndicatorBadType(xkb->dpy, led, field, "boolean");
        if (rtrn.uval)
            led->flags &= ~XkbIM_NoExplicit;
        else
            led->flags |= XkbIM_NoExplicit;
        led->defs.defined |= _LED_Explicit;
    }
    else if ((uStrCaseCmp(field, "whichmodstate") == 0) ||
             (uStrCaseCmp(field, "whichmodifierstate") == 0))
    {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(xkb->dpy, led, field);
        if (!ExprResolveMask(value, &rtrn, SimpleLookup,
                             (XPointer) modComponentNames))
        {
            return ReportIndicatorBadType(xkb->dpy, led, field,
                                          "mask of modifier state components");
        }
        led->which_mods = rtrn.uval;
    }
    else if (uStrCaseCmp(field, "whichgroupstate") == 0)
    {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(xkb->dpy, led, field);
        if (!ExprResolveMask(value, &rtrn, SimpleLookup,
                             (XPointer) groupComponentNames))
        {
            return ReportIndicatorBadType(xkb->dpy, led, field,
                                          "mask of group state components");
        }
        led->which_groups = rtrn.uval;
    }
    else if ((uStrCaseCmp(field, "driveskbd") == 0) ||
             (uStrCaseCmp(field, "driveskeyboard") == 0) ||
             (uStrCaseCmp(field, "leddriveskbd") == 0) ||
             (uStrCaseCmp(field, "leddriveskeyboard") == 0) ||
             (uStrCaseCmp(field, "indicatordriveskbd") == 0) ||
             (uStrCaseCmp(field, "indicatordriveskeyboard") == 0))
    {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(xkb->dpy, led, field);
        if (!ExprResolveBoolean(value, &rtrn, NULL, NULL))
            return ReportIndicatorBadType(xkb->dpy, led, field, "boolean");
        if (rtrn.uval)
            led->flags |= XkbIM_LEDDrivesKB;
        else
            led->flags &= ~XkbIM_LEDDrivesKB;
        led->defs.defined |= _LED_DrivesKbd;
    }
    else if (uStrCaseCmp(field, "index") == 0)
    {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(xkb->dpy, led, field);
        if (!ExprResolveInteger(value, &rtrn, NULL, NULL))
            return ReportIndicatorBadType(xkb->dpy, led, field,
                                          "indicator index");
        if ((rtrn.uval < 1) || (rtrn.uval > 32))
        {
            ERROR("Illegal indicator index %d (range 1..%d)\n",
                   rtrn.uval, XkbNumIndicators);
            ACTION("Index definition for %s indicator ignored\n",
                    XkbAtomText(NULL, led->name, XkbMessage));
            return False;
        }
        led->indicator = rtrn.uval;
        led->defs.defined |= _LED_Index;
    }
    else
    {
        ERROR("Unknown field %s in map for %s indicator\n", field,
               XkbAtomText(NULL, led->name, XkbMessage));
        ACTION("Definition ignored\n");
        ok = False;
    }
    return ok;
}

LEDInfo *
HandleIndicatorMapDef(IndicatorMapDef *def, XkbDescPtr xkb,
                      const LEDInfo *dflt, LEDInfo *oldLEDs, unsigned merge)
{
    LEDInfo led;
    Bool ok;

    if (def->merge != MergeDefault)
        merge = def->merge;

    led = *dflt;
    led.defs.merge = merge;
    led.name = def->name;

    ok = True;
    for (VarDef *var = def->body; var != NULL;
         var = (VarDef *) var->common.next)
    {
        ExprResult elem, field;
        ExprDef *arrayNdx;
        if (!ExprResolveLhs(var->name, &elem, &field, &arrayNdx))
        {
            ok = False;
            continue;
        }
        if (elem.str != NULL)
        {
            ERROR
                ("Cannot set defaults for \"%s\" element in indicator map\n",
                 elem.str);
            ACTION("Assignment to %s.%s ignored\n", elem.str, field.str);
            ok = False;
        }
        else
        {
            ok = SetIndicatorMapField(&led, xkb, field.str, arrayNdx,
                                      var->value) && ok;
        }
    }
    if (ok)
    {
        LEDInfo *rtrn = AddIndicatorMap(oldLEDs, &led);
        return rtrn;
    }
    return NULL;
}

Bool
CopyIndicatorMapDefs(XkbFileInfo * result, LEDInfo * leds,
                     LEDInfo ** unboundRtrn)
{
    LEDInfo *led, *next;
    LEDInfo *unbound, *last;
    XkbDescPtr xkb;

    xkb = result->xkb;
    if (XkbAllocNames(xkb, XkbIndicatorNamesMask, 0, 0) != Success)
    {
        WSGO("Couldn't allocate names\n");
        ACTION("Indicator names may be incorrect\n");
    }
    if (XkbAllocIndicatorMaps(xkb) != Success)
    {
        WSGO("Can't allocate indicator maps\n");
        ACTION("Indicator map definitions may be lost\n");
        return False;
    }
    last = unbound = (unboundRtrn ? *unboundRtrn : NULL);
    while ((last != NULL) && (last->defs.next != NULL))
    {
        last = (LEDInfo *) last->defs.next;
    }
    for (led = leds; led != NULL; led = next)
    {
        next = (LEDInfo *) led->defs.next;
        if ((led->groups != 0) && (led->which_groups == 0))
            led->which_groups = XkbIM_UseEffective;
        if ((led->which_mods == 0) && ((led->real_mods) || (led->vmods)))
            led->which_mods = XkbIM_UseEffective;
        if ((led->indicator == _LED_NotBound) || (!xkb->indicators))
        {
            if (unboundRtrn != NULL)
            {
                led->defs.next = NULL;
                if (last != NULL)
                    last->defs.next = (CommonInfo *) led;
                else
                    unbound = led;
                last = led;
            }
            else
                free(led);
        }
        else
        {
            XkbIndicatorMapPtr im;
            im = &xkb->indicators->maps[led->indicator - 1];
            im->flags = led->flags;
            im->which_groups = led->which_groups;
            im->groups = led->groups;
            im->which_mods = led->which_mods;
            im->mods.mask = led->real_mods;
            im->mods.real_mods = led->real_mods;
            im->mods.vmods = led->vmods;
            im->ctrls = led->ctrls;
            if (xkb->names != NULL)
                xkb->names->indicators[led->indicator - 1] = led->name;
            free(led);
        }
    }
    if (unboundRtrn != NULL)
    {
        *unboundRtrn = unbound;
    }
    return True;
}

Bool
BindIndicators(XkbFileInfo * result,
               Bool force, LEDInfo * unbound, LEDInfo ** unboundRtrn)
{
    XkbDescPtr xkb;
    LEDInfo *led, *next, *last;

    xkb = result->xkb;
    if (xkb->names != NULL)
    {
        for (led = unbound; led != NULL; led = (LEDInfo *) led->defs.next)
        {
            if (led->indicator == _LED_NotBound)
            {
                for (int i = 0; i < XkbNumIndicators; i++)
                {
                    if (xkb->names->indicators[i] == led->name)
                    {
                        led->indicator = i + 1;
                        break;
                    }
                }
            }
        }
        if (force)
        {
            for (led = unbound; led != NULL; led = (LEDInfo *) led->defs.next)
            {
                if (led->indicator == _LED_NotBound)
                {
                    for (int i = 0; i < XkbNumIndicators; i++)
                    {
                        if (xkb->names->indicators[i] == None)
                        {
                            xkb->names->indicators[i] = led->name;
                            led->indicator = i + 1;
                            xkb->indicators->phys_indicators &= ~(1 << i);
                            break;
                        }
                    }
                    if (led->indicator == _LED_NotBound)
                    {
                        ERROR("No unnamed indicators found\n");
                        ACTION
                            ("Virtual indicator map \"%s\" not bound\n",
                             XkbAtomGetString(xkb->dpy, led->name));
                        continue;
                    }
                }
            }
        }
    }
    for (last = NULL, led = unbound; led != NULL; led = next)
    {
        next = (LEDInfo *) led->defs.next;
        if (led->indicator == _LED_NotBound)
        {
            if (force)
            {
                unbound = next;
                free(led);
            }
            else
            {
                if (last)
                    last->defs.next = &led->defs;
                else
                    unbound = led;
                last = led;
            }
        }
        else
        {
            if ((xkb->names != NULL) &&
                (xkb->names->indicators[led->indicator - 1] != led->name))
            {
                Atom old = xkb->names->indicators[led->indicator - 1];
                ERROR("Multiple names bound to indicator %d\n",
                       (unsigned int) led->indicator);
                ACTION("Using %s, ignoring %s\n",
                        XkbAtomGetString(xkb->dpy, old),
                        XkbAtomGetString(xkb->dpy, led->name));
                led->indicator = _LED_NotBound;
                if (force)
                {
                    free(led);
                    unbound = next;
                }
                else
                {
                    if (last)
                        last->defs.next = &led->defs;
                    else
                        unbound = led;
                    last = led;
                }
            }
            else
            {
                XkbIndicatorMapPtr map;
                map = &xkb->indicators->maps[led->indicator - 1];
                map->flags = led->flags;
                map->which_groups = led->which_groups;
                map->groups = led->groups;
                map->which_mods = led->which_mods;
                map->mods.mask = led->real_mods;
                map->mods.real_mods = led->real_mods;
                map->mods.vmods = led->vmods;
                map->ctrls = led->ctrls;
                if (last)
                    last->defs.next = &next->defs;
                else
                    unbound = next;
                led->defs.next = NULL;
                free(led);
            }
        }
    }
    if (unboundRtrn)
    {
        *unboundRtrn = unbound;
    }
    else if (unbound)
    {
        for (led = unbound; led != NULL; led = next)
        {
            next = (LEDInfo *) led->defs.next;
            free(led);
        }
    }
    return True;
}
