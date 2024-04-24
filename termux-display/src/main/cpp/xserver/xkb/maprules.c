/************************************************************
 Copyright (c) 1996 by Silicon Graphics Computer Systems, Inc.

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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define X_INCLUDE_STRING_H
#define XOS_USE_NO_LOCKING
#include <X11/Xos_r.h>

#include <X11/Xproto.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "misc.h"
#include "inputstr.h"
#include "dix.h"
#include "os.h"
#include "xkbstr.h"
#define XKBSRV_NEED_FILE_FUNCS
#include <xkbsrv.h>

/***====================================================================***/

#define DFLT_LINE_SIZE	128

typedef struct {
    int line_num;
    int sz_line;
    int num_line;
    char buf[DFLT_LINE_SIZE];
    char *line;
} InputLine;

static void
InitInputLine(InputLine * line)
{
    line->line_num = 1;
    line->num_line = 0;
    line->sz_line = DFLT_LINE_SIZE;
    line->line = line->buf;
    return;
}

static void
FreeInputLine(InputLine * line)
{
    if (line->line != line->buf)
        free(line->line);
    line->line_num = 1;
    line->num_line = 0;
    line->sz_line = DFLT_LINE_SIZE;
    line->line = line->buf;
    return;
}

static int
InputLineAddChar(InputLine * line, int ch)
{
    if (line->num_line >= line->sz_line) {
        if (line->line == line->buf) {
            line->line = xallocarray(line->sz_line, 2);
            memcpy(line->line, line->buf, line->sz_line);
        }
        else {
            line->line = reallocarray(line->line, line->sz_line, 2);
        }
        line->sz_line *= 2;
    }
    line->line[line->num_line++] = ch;
    return ch;
}

#define	ADD_CHAR(l,c)	((l)->num_line<(l)->sz_line?\
				(int)((l)->line[(l)->num_line++]= (c)):\
				InputLineAddChar(l,c))

static Bool
GetInputLine(FILE * file, InputLine * line, Bool checkbang)
{
    int ch;
    Bool endOfFile, spacePending, slashPending, inComment;

    endOfFile = FALSE;
    while ((!endOfFile) && (line->num_line == 0)) {
        spacePending = slashPending = inComment = FALSE;
        while (((ch = getc(file)) != '\n') && (ch != EOF)) {
            if (ch == '\\') {
                if ((ch = getc(file)) == EOF)
                    break;
                if (ch == '\n') {
                    inComment = FALSE;
                    ch = ' ';
                    line->line_num++;
                }
            }
            if (inComment)
                continue;
            if (ch == '/') {
                if (slashPending) {
                    inComment = TRUE;
                    slashPending = FALSE;
                }
                else {
                    slashPending = TRUE;
                }
                continue;
            }
            else if (slashPending) {
                if (spacePending) {
                    ADD_CHAR(line, ' ');
                    spacePending = FALSE;
                }
                ADD_CHAR(line, '/');
                slashPending = FALSE;
            }
            if (isspace(ch)) {
                while (isspace(ch) && (ch != '\n') && (ch != EOF)) {
                    ch = getc(file);
                }
                if (ch == EOF)
                    break;
                if ((ch != '\n') && (line->num_line > 0))
                    spacePending = TRUE;
                ungetc(ch, file);
            }
            else {
                if (spacePending) {
                    ADD_CHAR(line, ' ');
                    spacePending = FALSE;
                }
                if (checkbang && ch == '!') {
                    if (line->num_line != 0) {
                        DebugF("The '!' legal only at start of line\n");
                        DebugF("Line containing '!' ignored\n");
                        line->num_line = 0;
                        inComment = 0;
                        break;
                    }

                }
                ADD_CHAR(line, ch);
            }
        }
        if (ch == EOF)
            endOfFile = TRUE;
/*	else line->num_line++;*/
    }
    if ((line->num_line == 0) && (endOfFile))
        return FALSE;
    ADD_CHAR(line, '\0');
    return TRUE;
}

/***====================================================================***/

#define	MODEL		0
#define	LAYOUT		1
#define	VARIANT		2
#define	OPTION		3
#define	KEYCODES	4
#define SYMBOLS		5
#define	TYPES		6
#define	COMPAT		7
#define	GEOMETRY	8
#define	MAX_WORDS	9

#define	PART_MASK	0x000F
#define	COMPONENT_MASK	0x03F0

static const char *cname[MAX_WORDS] = {
    "model", "layout", "variant", "option",
    "keycodes", "symbols", "types", "compat", "geometry"
};

typedef struct _RemapSpec {
    int number;
    int num_remap;
    struct {
        int word;
        int index;
    } remap[MAX_WORDS];
} RemapSpec;

typedef struct _FileSpec {
    char *name[MAX_WORDS];
    struct _FileSpec *pending;
} FileSpec;

typedef struct {
    const char *model;
    const char *layout[XkbNumKbdGroups + 1];
    const char *variant[XkbNumKbdGroups + 1];
    const char *options;
} XkbRF_MultiDefsRec, *XkbRF_MultiDefsPtr;

#define NDX_BUFF_SIZE	4

/***====================================================================***/

static char *
get_index(char *str, int *ndx)
{
    char ndx_buf[NDX_BUFF_SIZE];
    char *end;

    if (*str != '[') {
        *ndx = 0;
        return str;
    }
    str++;
    end = strchr(str, ']');
    if (end == NULL) {
        *ndx = -1;
        return str - 1;
    }
    if ((end - str) >= NDX_BUFF_SIZE) {
        *ndx = -1;
        return end + 1;
    }
    strlcpy(ndx_buf, str, 1 + end - str);
    *ndx = atoi(ndx_buf);
    return end + 1;
}

static void
SetUpRemap(InputLine * line, RemapSpec * remap)
{
    char *tok, *str;
    unsigned present, l_ndx_present, v_ndx_present;
    register int i;
    int len, ndx;
    _Xstrtokparams strtok_buf;
    Bool found;

    l_ndx_present = v_ndx_present = present = 0;
    str = &line->line[1];
    len = remap->number;
    memset((char *) remap, 0, sizeof(RemapSpec));
    remap->number = len;
    while ((tok = _XStrtok(str, " ", strtok_buf)) != NULL) {
        found = FALSE;
        str = NULL;
        if (strcmp(tok, "=") == 0)
            continue;
        for (i = 0; i < MAX_WORDS; i++) {
            len = strlen(cname[i]);
            if (strncmp(cname[i], tok, len) == 0) {
                if (strlen(tok) > len) {
                    char *end = get_index(tok + len, &ndx);

                    if ((i != LAYOUT && i != VARIANT) ||
                        *end != '\0' || ndx == -1)
                        break;
                    if (ndx < 1 || ndx > XkbNumKbdGroups) {
                        DebugF("Illegal %s index: %d\n", cname[i], ndx);
                        DebugF("Index must be in range 1..%d\n",
                               XkbNumKbdGroups);
                        break;
                    }
                }
                else {
                    ndx = 0;
                }
                found = TRUE;
                if (present & (1 << i)) {
                    if ((i == LAYOUT && l_ndx_present & (1 << ndx)) ||
                        (i == VARIANT && v_ndx_present & (1 << ndx))) {
                        DebugF("Component \"%s\" listed twice\n", tok);
                        DebugF("Second definition ignored\n");
                        break;
                    }
                }
                present |= (1 << i);
                if (i == LAYOUT)
                    l_ndx_present |= 1 << ndx;
                if (i == VARIANT)
                    v_ndx_present |= 1 << ndx;
                remap->remap[remap->num_remap].word = i;
                remap->remap[remap->num_remap++].index = ndx;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Unknown component \"%s\" ignored\n", tok);
        }
    }
    if ((present & PART_MASK) == 0) {
        unsigned mask = PART_MASK;

        ErrorF("Mapping needs at least one of ");
        for (i = 0; (i < MAX_WORDS); i++) {
            if ((1L << i) & mask) {
                mask &= ~(1L << i);
                if (mask)
                    DebugF("\"%s,\" ", cname[i]);
                else
                    DebugF("or \"%s\"\n", cname[i]);
            }
        }
        DebugF("Illegal mapping ignored\n");
        remap->num_remap = 0;
        return;
    }
    if ((present & COMPONENT_MASK) == 0) {
        DebugF("Mapping needs at least one component\n");
        DebugF("Illegal mapping ignored\n");
        remap->num_remap = 0;
        return;
    }
    remap->number++;
    return;
}

static Bool
MatchOneOf(const char *wanted, const char *vals_defined)
{
    const char *str, *next;
    int want_len = strlen(wanted);

    for (str = vals_defined, next = NULL; str != NULL; str = next) {
        int len;

        next = strchr(str, ',');
        if (next) {
            len = next - str;
            next++;
        }
        else {
            len = strlen(str);
        }
        if ((len == want_len) && (strncmp(wanted, str, len) == 0))
            return TRUE;
    }
    return FALSE;
}

/***====================================================================***/

static Bool
CheckLine(InputLine * line,
          RemapSpec * remap, XkbRF_RulePtr rule, XkbRF_GroupPtr group)
{
    char *str, *tok;
    register int nread, i;
    FileSpec tmp;
    _Xstrtokparams strtok_buf;
    Bool append = FALSE;

    if (line->line[0] == '!') {
        if (line->line[1] == '$' ||
            (line->line[1] == ' ' && line->line[2] == '$')) {
            char *gname = strchr(line->line, '$');
            char *words = strchr(gname, ' ');

            if (!words)
                return FALSE;
            *words++ = '\0';
            for (; *words; words++) {
                if (*words != '=' && *words != ' ')
                    break;
            }
            if (*words == '\0')
                return FALSE;
            group->name = Xstrdup(gname);
            group->words = Xstrdup(words);
            for (i = 1, words = group->words; *words; words++) {
                if (*words == ' ') {
                    *words++ = '\0';
                    i++;
                }
            }
            group->number = i;
            return TRUE;
        }
        else {
            SetUpRemap(line, remap);
            return FALSE;
        }
    }

    if (remap->num_remap == 0) {
        DebugF("Must have a mapping before first line of data\n");
        DebugF("Illegal line of data ignored\n");
        return FALSE;
    }
    memset((char *) &tmp, 0, sizeof(FileSpec));
    str = line->line;
    for (nread = 0; (tok = _XStrtok(str, " ", strtok_buf)) != NULL; nread++) {
        str = NULL;
        if (strcmp(tok, "=") == 0) {
            nread--;
            continue;
        }
        if (nread > remap->num_remap) {
            DebugF("Too many words on a line\n");
            DebugF("Extra word \"%s\" ignored\n", tok);
            continue;
        }
        tmp.name[remap->remap[nread].word] = tok;
        if (*tok == '+' || *tok == '|')
            append = TRUE;
    }
    if (nread < remap->num_remap) {
        DebugF("Too few words on a line: %s\n", line->line);
        DebugF("line ignored\n");
        return FALSE;
    }

    rule->flags = 0;
    rule->number = remap->number;
    if (tmp.name[OPTION])
        rule->flags |= XkbRF_Option;
    else if (append)
        rule->flags |= XkbRF_Append;
    else
        rule->flags |= XkbRF_Normal;
    rule->model = Xstrdup(tmp.name[MODEL]);
    rule->layout = Xstrdup(tmp.name[LAYOUT]);
    rule->variant = Xstrdup(tmp.name[VARIANT]);
    rule->option = Xstrdup(tmp.name[OPTION]);

    rule->keycodes = Xstrdup(tmp.name[KEYCODES]);
    rule->symbols = Xstrdup(tmp.name[SYMBOLS]);
    rule->types = Xstrdup(tmp.name[TYPES]);
    rule->compat = Xstrdup(tmp.name[COMPAT]);
    rule->geometry = Xstrdup(tmp.name[GEOMETRY]);

    rule->layout_num = rule->variant_num = 0;
    for (i = 0; i < nread; i++) {
        if (remap->remap[i].index) {
            if (remap->remap[i].word == LAYOUT)
                rule->layout_num = remap->remap[i].index;
            if (remap->remap[i].word == VARIANT)
                rule->variant_num = remap->remap[i].index;
        }
    }
    return TRUE;
}

static char *
_Concat(char *str1, const char *str2)
{
    int len;

    if ((!str1) || (!str2))
        return str1;
    len = strlen(str1) + strlen(str2) + 1;
    str1 = realloc(str1, len * sizeof(char));
    if (str1)
        strcat(str1, str2);
    return str1;
}

static void
squeeze_spaces(char *p1)
{
    char *p2;

    for (p2 = p1; *p2; p2++) {
        *p1 = *p2;
        if (*p1 != ' ')
            p1++;
    }
    *p1 = '\0';
}

static Bool
MakeMultiDefs(XkbRF_MultiDefsPtr mdefs, XkbRF_VarDefsPtr defs)
{
    char *options;
    memset((char *) mdefs, 0, sizeof(XkbRF_MultiDefsRec));
    mdefs->model = defs->model;
    options = Xstrdup(defs->options);
    if (options)
        squeeze_spaces(options);
    mdefs->options = options;

    if (defs->layout) {
        if (!strchr(defs->layout, ',')) {
            mdefs->layout[0] = defs->layout;
        }
        else {
            char *p;
            char *layout;
            int i;

            layout = Xstrdup(defs->layout);
            if (layout == NULL)
                return FALSE;
            squeeze_spaces(layout);
            mdefs->layout[1] = layout;
            p = layout;
            for (i = 2; i <= XkbNumKbdGroups; i++) {
                if ((p = strchr(p, ','))) {
                    *p++ = '\0';
                    mdefs->layout[i] = p;
                }
                else {
                    break;
                }
            }
            if (p && (p = strchr(p, ',')))
                *p = '\0';
        }
    }

    if (defs->variant) {
        if (!strchr(defs->variant, ',')) {
            mdefs->variant[0] = defs->variant;
        }
        else {
            char *p;
            char *variant;
            int i;

            variant = Xstrdup(defs->variant);
            if (variant == NULL)
                return FALSE;
            squeeze_spaces(variant);
            mdefs->variant[1] = variant;
            p = variant;
            for (i = 2; i <= XkbNumKbdGroups; i++) {
                if ((p = strchr(p, ','))) {
                    *p++ = '\0';
                    mdefs->variant[i] = p;
                }
                else {
                    break;
                }
            }
            if (p && (p = strchr(p, ',')))
                *p = '\0';
        }
    }
    return TRUE;
}

static void
FreeMultiDefs(XkbRF_MultiDefsPtr defs)
{
    free((void *) defs->options);
    free((void *) defs->layout[1]);
    free((void *) defs->variant[1]);
}

static void
Apply(const char *src, char **dst)
{
    if (src) {
        if (*src == '+' || *src == '|') {
            *dst = _Concat(*dst, src);
        }
        else {
            if (*dst == NULL)
                *dst = Xstrdup(src);
        }
    }
}

static void
XkbRF_ApplyRule(XkbRF_RulePtr rule, XkbComponentNamesPtr names)
{
    rule->flags &= ~XkbRF_PendingMatch; /* clear the flag because it's applied */

    Apply(rule->keycodes, &names->keycodes);
    Apply(rule->symbols, &names->symbols);
    Apply(rule->types, &names->types);
    Apply(rule->compat, &names->compat);
    Apply(rule->geometry, &names->geometry);
}

static Bool
CheckGroup(XkbRF_RulesPtr rules, const char *group_name, const char *name)
{
    int i;
    char *p;
    XkbRF_GroupPtr group;

    for (i = 0, group = rules->groups; i < rules->num_groups; i++, group++) {
        if (!strcmp(group->name, group_name)) {
            break;
        }
    }
    if (i == rules->num_groups)
        return FALSE;
    for (i = 0, p = group->words; i < group->number; i++, p += strlen(p) + 1) {
        if (!strcmp(p, name)) {
            return TRUE;
        }
    }
    return FALSE;
}

static int
XkbRF_CheckApplyRule(XkbRF_RulePtr rule,
                     XkbRF_MultiDefsPtr mdefs,
                     XkbComponentNamesPtr names, XkbRF_RulesPtr rules)
{
    Bool pending = FALSE;

    if (rule->model != NULL) {
        if (mdefs->model == NULL)
            return 0;
        if (strcmp(rule->model, "*") == 0) {
            pending = TRUE;
        }
        else {
            if (rule->model[0] == '$') {
                if (!CheckGroup(rules, rule->model, mdefs->model))
                    return 0;
            }
            else {
                if (strcmp(rule->model, mdefs->model) != 0)
                    return 0;
            }
        }
    }
    if (rule->option != NULL) {
        if (mdefs->options == NULL)
            return 0;
        if ((!MatchOneOf(rule->option, mdefs->options)))
            return 0;
    }

    if (rule->layout != NULL) {
        if (mdefs->layout[rule->layout_num] == NULL ||
            *mdefs->layout[rule->layout_num] == '\0')
            return 0;
        if (strcmp(rule->layout, "*") == 0) {
            pending = TRUE;
        }
        else {
            if (rule->layout[0] == '$') {
                if (!CheckGroup(rules, rule->layout,
                                mdefs->layout[rule->layout_num]))
                    return 0;
            }
            else {
                if (strcmp(rule->layout, mdefs->layout[rule->layout_num]) != 0)
                    return 0;
            }
        }
    }
    if (rule->variant != NULL) {
        if (mdefs->variant[rule->variant_num] == NULL ||
            *mdefs->variant[rule->variant_num] == '\0')
            return 0;
        if (strcmp(rule->variant, "*") == 0) {
            pending = TRUE;
        }
        else {
            if (rule->variant[0] == '$') {
                if (!CheckGroup(rules, rule->variant,
                                mdefs->variant[rule->variant_num]))
                    return 0;
            }
            else {
                if (strcmp(rule->variant,
                           mdefs->variant[rule->variant_num]) != 0)
                    return 0;
            }
        }
    }
    if (pending) {
        rule->flags |= XkbRF_PendingMatch;
        return rule->number;
    }
    /* exact match, apply it now */
    XkbRF_ApplyRule(rule, names);
    return rule->number;
}

static void
XkbRF_ClearPartialMatches(XkbRF_RulesPtr rules)
{
    register int i;
    XkbRF_RulePtr rule;

    for (i = 0, rule = rules->rules; i < rules->num_rules; i++, rule++) {
        rule->flags &= ~XkbRF_PendingMatch;
    }
}

static void
XkbRF_ApplyPartialMatches(XkbRF_RulesPtr rules, XkbComponentNamesPtr names)
{
    int i;
    XkbRF_RulePtr rule;

    for (rule = rules->rules, i = 0; i < rules->num_rules; i++, rule++) {
        if ((rule->flags & XkbRF_PendingMatch) == 0)
            continue;
        XkbRF_ApplyRule(rule, names);
    }
}

static void
XkbRF_CheckApplyRules(XkbRF_RulesPtr rules,
                      XkbRF_MultiDefsPtr mdefs,
                      XkbComponentNamesPtr names, int flags)
{
    int i;
    XkbRF_RulePtr rule;
    int skip;

    for (rule = rules->rules, i = 0; i < rules->num_rules; rule++, i++) {
        if ((rule->flags & flags) != flags)
            continue;
        skip = XkbRF_CheckApplyRule(rule, mdefs, names, rules);
        if (skip && !(flags & XkbRF_Option)) {
            for (; (i < rules->num_rules) && (rule->number == skip);
                 rule++, i++);
            rule--;
            i--;
        }
    }
}

/***====================================================================***/

static char *
XkbRF_SubstituteVars(char *name, XkbRF_MultiDefsPtr mdefs)
{
    char *str, *outstr, *orig, *var;
    int len, ndx;

    orig = name;
    str = index(name, '%');
    if (str == NULL)
        return name;
    len = strlen(name);
    while (str != NULL) {
        char pfx = str[1];
        int extra_len = 0;

        if ((pfx == '+') || (pfx == '|') || (pfx == '_') || (pfx == '-')) {
            extra_len = 1;
            str++;
        }
        else if (pfx == '(') {
            extra_len = 2;
            str++;
        }
        var = str + 1;
        str = get_index(var + 1, &ndx);
        if (ndx == -1) {
            str = index(str, '%');
            continue;
        }
        if ((*var == 'l') && mdefs->layout[ndx] && *mdefs->layout[ndx])
            len += strlen(mdefs->layout[ndx]) + extra_len;
        else if ((*var == 'm') && mdefs->model)
            len += strlen(mdefs->model) + extra_len;
        else if ((*var == 'v') && mdefs->variant[ndx] && *mdefs->variant[ndx])
            len += strlen(mdefs->variant[ndx]) + extra_len;
        if ((pfx == '(') && (*str == ')')) {
            str++;
        }
        str = index(&str[0], '%');
    }
    name = malloc(len + 1);
    str = orig;
    outstr = name;
    while (*str != '\0') {
        if (str[0] == '%') {
            char pfx, sfx;

            str++;
            pfx = str[0];
            sfx = '\0';
            if ((pfx == '+') || (pfx == '|') || (pfx == '_') || (pfx == '-')) {
                str++;
            }
            else if (pfx == '(') {
                sfx = ')';
                str++;
            }
            else
                pfx = '\0';

            var = str;
            str = get_index(var + 1, &ndx);
            if (ndx == -1) {
                continue;
            }
            if ((*var == 'l') && mdefs->layout[ndx] && *mdefs->layout[ndx]) {
                if (pfx)
                    *outstr++ = pfx;
                strcpy(outstr, mdefs->layout[ndx]);
                outstr += strlen(mdefs->layout[ndx]);
                if (sfx)
                    *outstr++ = sfx;
            }
            else if ((*var == 'm') && (mdefs->model)) {
                if (pfx)
                    *outstr++ = pfx;
                strcpy(outstr, mdefs->model);
                outstr += strlen(mdefs->model);
                if (sfx)
                    *outstr++ = sfx;
            }
            else if ((*var == 'v') && mdefs->variant[ndx] &&
                     *mdefs->variant[ndx]) {
                if (pfx)
                    *outstr++ = pfx;
                strcpy(outstr, mdefs->variant[ndx]);
                outstr += strlen(mdefs->variant[ndx]);
                if (sfx)
                    *outstr++ = sfx;
            }
            if ((pfx == '(') && (*str == ')'))
                str++;
        }
        else {
            *outstr++ = *str++;
        }
    }
    *outstr++ = '\0';
    if (orig != name)
        free(orig);
    return name;
}

/***====================================================================***/

Bool
XkbRF_GetComponents(XkbRF_RulesPtr rules,
                    XkbRF_VarDefsPtr defs, XkbComponentNamesPtr names)
{
    XkbRF_MultiDefsRec mdefs;

    MakeMultiDefs(&mdefs, defs);

    memset((char *) names, 0, sizeof(XkbComponentNamesRec));
    XkbRF_ClearPartialMatches(rules);
    XkbRF_CheckApplyRules(rules, &mdefs, names, XkbRF_Normal);
    XkbRF_ApplyPartialMatches(rules, names);
    XkbRF_CheckApplyRules(rules, &mdefs, names, XkbRF_Append);
    XkbRF_ApplyPartialMatches(rules, names);
    XkbRF_CheckApplyRules(rules, &mdefs, names, XkbRF_Option);
    XkbRF_ApplyPartialMatches(rules, names);

    if (names->keycodes)
        names->keycodes = XkbRF_SubstituteVars(names->keycodes, &mdefs);
    if (names->symbols)
        names->symbols = XkbRF_SubstituteVars(names->symbols, &mdefs);
    if (names->types)
        names->types = XkbRF_SubstituteVars(names->types, &mdefs);
    if (names->compat)
        names->compat = XkbRF_SubstituteVars(names->compat, &mdefs);
    if (names->geometry)
        names->geometry = XkbRF_SubstituteVars(names->geometry, &mdefs);

    FreeMultiDefs(&mdefs);
    return (names->keycodes && names->symbols && names->types &&
            names->compat && names->geometry);
}

static XkbRF_RulePtr
XkbRF_AddRule(XkbRF_RulesPtr rules)
{
    if (rules->sz_rules < 1) {
        rules->sz_rules = 16;
        rules->num_rules = 0;
        rules->rules = calloc(rules->sz_rules, sizeof(XkbRF_RuleRec));
    }
    else if (rules->num_rules >= rules->sz_rules) {
        rules->sz_rules *= 2;
        rules->rules = reallocarray(rules->rules,
                                    rules->sz_rules, sizeof(XkbRF_RuleRec));
    }
    if (!rules->rules) {
        rules->sz_rules = rules->num_rules = 0;
        DebugF("Allocation failure in XkbRF_AddRule\n");
        return NULL;
    }
    memset((char *) &rules->rules[rules->num_rules], 0, sizeof(XkbRF_RuleRec));
    return &rules->rules[rules->num_rules++];
}

static XkbRF_GroupPtr
XkbRF_AddGroup(XkbRF_RulesPtr rules)
{
    if (rules->sz_groups < 1) {
        rules->sz_groups = 16;
        rules->num_groups = 0;
        rules->groups = calloc(rules->sz_groups, sizeof(XkbRF_GroupRec));
    }
    else if (rules->num_groups >= rules->sz_groups) {
        rules->sz_groups *= 2;
        rules->groups = reallocarray(rules->groups,
                                     rules->sz_groups, sizeof(XkbRF_GroupRec));
    }
    if (!rules->groups) {
        rules->sz_groups = rules->num_groups = 0;
        return NULL;
    }

    memset((char *) &rules->groups[rules->num_groups], 0,
           sizeof(XkbRF_GroupRec));
    return &rules->groups[rules->num_groups++];
}

Bool
XkbRF_LoadRules(FILE * file, XkbRF_RulesPtr rules)
{
    InputLine line;
    RemapSpec remap;
    XkbRF_RuleRec trule, *rule;
    XkbRF_GroupRec tgroup, *group;

    if (!(rules && file))
        return FALSE;
    memset((char *) &remap, 0, sizeof(RemapSpec));
    memset((char *) &tgroup, 0, sizeof(XkbRF_GroupRec));
    InitInputLine(&line);
    while (GetInputLine(file, &line, TRUE)) {
        if (CheckLine(&line, &remap, &trule, &tgroup)) {
            if (tgroup.number) {
                if ((group = XkbRF_AddGroup(rules)) != NULL) {
                    *group = tgroup;
                    memset((char *) &tgroup, 0, sizeof(XkbRF_GroupRec));
                }
            }
            else {
                if ((rule = XkbRF_AddRule(rules)) != NULL) {
                    *rule = trule;
                    memset((char *) &trule, 0, sizeof(XkbRF_RuleRec));
                }
            }
        }
        line.num_line = 0;
    }
    FreeInputLine(&line);
    return TRUE;
}

Bool
XkbRF_LoadRulesByName(char *base, char *locale, XkbRF_RulesPtr rules)
{
    FILE *file;
    char buf[PATH_MAX];
    Bool ok;

    if ((!base) || (!rules))
        return FALSE;
    if (locale) {
        if (snprintf(buf, PATH_MAX, "%s-%s", base, locale) >= PATH_MAX)
            return FALSE;
    }
    else {
        if (strlen(base) + 1 > PATH_MAX)
            return FALSE;
        strcpy(buf, base);
    }

    file = fopen(buf, "r");
    if ((!file) && (locale)) {  /* fallback if locale was specified */
        strcpy(buf, base);
        file = fopen(buf, "r");
    }
    if (!file)
        return FALSE;
    ok = XkbRF_LoadRules(file, rules);
    fclose(file);
    return ok;
}

/***====================================================================***/

XkbRF_RulesPtr
XkbRF_Create(void)
{
    return calloc(1, sizeof(XkbRF_RulesRec));
}

/***====================================================================***/

void
XkbRF_Free(XkbRF_RulesPtr rules, Bool freeRules)
{
    int i;
    XkbRF_RulePtr rule;
    XkbRF_GroupPtr group;

    if (!rules)
        return;
    if (rules->rules) {
        for (i = 0, rule = rules->rules; i < rules->num_rules; i++, rule++) {
            free((void *) rule->model);
            free((void *) rule->layout);
            free((void *) rule->variant);
            free((void *) rule->option);
            free((void *) rule->keycodes);
            free((void *) rule->symbols);
            free((void *) rule->types);
            free((void *) rule->compat);
            free((void *) rule->geometry);
            memset((char *) rule, 0, sizeof(XkbRF_RuleRec));
        }
        free(rules->rules);
        rules->num_rules = rules->sz_rules = 0;
        rules->rules = NULL;
    }

    if (rules->groups) {
        for (i = 0, group = rules->groups; i < rules->num_groups; i++, group++) {
            free((void *) group->name);
            free(group->words);
        }
        free(rules->groups);
        rules->num_groups = 0;
        rules->groups = NULL;
    }
    if (freeRules)
        free(rules);
    return;
}
