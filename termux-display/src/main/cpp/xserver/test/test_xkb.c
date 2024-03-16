/**
 * Copyright © 2009 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <xkb-config.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include "misc.h"
#include "inputstr.h"
#include "opaque.h"
#include "property.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include <xkbsrv.h>
#include "../xkb/xkbgeom.h"
#include <X11/extensions/XKMformat.h>
#include "xkbfile.h"
#include "../xkb/xkb.h"
#include <assert.h>

#include "tests-common.h"

/**
 * Initialize an empty XkbRMLVOSet.
 * Call XkbGetRulesDflts to obtain the default ruleset.
 * Compare obtained ruleset with the built-in defaults.
 *
 * Result: RMLVO defaults are the same as obtained.
 */
static void
xkb_get_rules_test(void)
{
    XkbRMLVOSet rmlvo = { NULL };
    XkbGetRulesDflts(&rmlvo);

    assert(rmlvo.rules);
    assert(rmlvo.model);
    assert(rmlvo.layout);
    assert(rmlvo.variant);
    assert(rmlvo.options);
    assert(strcmp(rmlvo.rules, XKB_DFLT_RULES) == 0);
    assert(strcmp(rmlvo.model, XKB_DFLT_MODEL) == 0);
    assert(strcmp(rmlvo.layout, XKB_DFLT_LAYOUT) == 0);
    assert(strcmp(rmlvo.variant, XKB_DFLT_VARIANT) == 0);
    assert(strcmp(rmlvo.options, XKB_DFLT_OPTIONS) == 0);
}

/**
 * Initialize an random XkbRMLVOSet.
 * Call XkbGetRulesDflts to obtain the default ruleset.
 * Compare obtained ruleset with the built-in defaults.
 * Result: RMLVO defaults are the same as obtained.
 */
static void
xkb_set_rules_test(void)
{
    XkbRMLVOSet rmlvo;
    XkbRMLVOSet rmlvo_new = { NULL };

    XkbInitRules(&rmlvo, "test-rules", "test-model", "test-layout",
                         "test-variant", "test-options");
    assert(rmlvo.rules);
    assert(rmlvo.model);
    assert(rmlvo.layout);
    assert(rmlvo.variant);
    assert(rmlvo.options);

    XkbSetRulesDflts(&rmlvo);
    XkbGetRulesDflts(&rmlvo_new);

    /* XkbGetRulesDflts strdups the values */
    assert(rmlvo.rules != rmlvo_new.rules);
    assert(rmlvo.model != rmlvo_new.model);
    assert(rmlvo.layout != rmlvo_new.layout);
    assert(rmlvo.variant != rmlvo_new.variant);
    assert(rmlvo.options != rmlvo_new.options);

    assert(strcmp(rmlvo.rules, rmlvo_new.rules) == 0);
    assert(strcmp(rmlvo.model, rmlvo_new.model) == 0);
    assert(strcmp(rmlvo.layout, rmlvo_new.layout) == 0);
    assert(strcmp(rmlvo.variant, rmlvo_new.variant) == 0);
    assert(strcmp(rmlvo.options, rmlvo_new.options) == 0);

    XkbFreeRMLVOSet(&rmlvo, FALSE);
}

/**
 * Get the default RMLVO set.
 * Set the default RMLVO set.
 * Get the default RMLVO set.
 * Repeat the last two steps.
 *
 * Result: RMLVO set obtained is the same as previously set.
 */
static void
xkb_set_get_rules_test(void)
{
/* This test failed before XkbGetRulesDftlts changed to strdup.
   We test this twice because the first time using XkbGetRulesDflts we obtain
   the built-in defaults. The unexpected free isn't triggered until the second
   XkbSetRulesDefaults.
 */
    XkbRMLVOSet rmlvo = { NULL };
    XkbRMLVOSet rmlvo_backup;

    XkbGetRulesDflts(&rmlvo);

    /* pass 1 */
    XkbSetRulesDflts(&rmlvo);
    XkbGetRulesDflts(&rmlvo);

    /* Make a backup copy */
    rmlvo_backup.rules = strdup(rmlvo.rules);
    rmlvo_backup.layout = strdup(rmlvo.layout);
    rmlvo_backup.model = strdup(rmlvo.model);
    rmlvo_backup.variant = strdup(rmlvo.variant);
    rmlvo_backup.options = strdup(rmlvo.options);

    /* pass 2 */
    XkbSetRulesDflts(&rmlvo);

    /* This test is iffy, because strictly we may be comparing against already
     * freed memory */
    assert(strcmp(rmlvo.rules, rmlvo_backup.rules) == 0);
    assert(strcmp(rmlvo.model, rmlvo_backup.model) == 0);
    assert(strcmp(rmlvo.layout, rmlvo_backup.layout) == 0);
    assert(strcmp(rmlvo.variant, rmlvo_backup.variant) == 0);
    assert(strcmp(rmlvo.options, rmlvo_backup.options) == 0);

    XkbGetRulesDflts(&rmlvo);
    assert(strcmp(rmlvo.rules, rmlvo_backup.rules) == 0);
    assert(strcmp(rmlvo.model, rmlvo_backup.model) == 0);
    assert(strcmp(rmlvo.layout, rmlvo_backup.layout) == 0);
    assert(strcmp(rmlvo.variant, rmlvo_backup.variant) == 0);
    assert(strcmp(rmlvo.options, rmlvo_backup.options) == 0);
}

int
xkb_test(void)
{
    xkb_set_get_rules_test();
    xkb_get_rules_test();
    xkb_set_rules_test();

    return 0;
}
