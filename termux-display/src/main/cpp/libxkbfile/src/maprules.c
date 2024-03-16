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
#elif defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define X_INCLUDE_STRING_H
#define XOS_USE_NO_LOCKING
#include <X11/Xos_r.h>


#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include "XKMformat.h"
#include "XKBfileInt.h"
#include "XKBrules.h"

#ifdef O_CLOEXEC
#define FOPEN_CLOEXEC "e"
#else
#define FOPEN_CLOEXEC ""
#endif

#ifdef DEBUG
#define PR_DEBUG(s)		fprintf(stderr,s)
#define PR_DEBUG1(s,a)		fprintf(stderr,s,a)
#define PR_DEBUG2(s,a,b)	fprintf(stderr,s,a,b)
#else
#define PR_DEBUG(s)
#define PR_DEBUG1(s,a)
#define PR_DEBUG2(s,a,b)
#endif

/***====================================================================***/

#define DFLT_LINE_SIZE	128

typedef struct {
    int         line_num;
    int         sz_line;
    int         num_line;
    char        buf[DFLT_LINE_SIZE];
    char *      line;
} InputLine;

static void
InitInputLine(InputLine *line)
{
    line->line_num = 1;
    line->num_line = 0;
    line->sz_line = DFLT_LINE_SIZE;
    line->line = line->buf;
    return;
}

static void
FreeInputLine(InputLine *line)
{
    if (line->line != line->buf)
        _XkbFree(line->line);
    line->line_num = 1;
    line->num_line = 0;
    line->sz_line = DFLT_LINE_SIZE;
    line->line = line->buf;
    return;
}

static int
InputLineAddChar(InputLine *line, int ch)
{
    if (line->num_line >= line->sz_line) {
        if (line->line == line->buf) {
            line->line = (char *) _XkbAlloc(line->sz_line * 2);
            memcpy(line->line, line->buf, line->sz_line);
        }
        else {
            line->line =
                (char *) _XkbRealloc((char *) line->line, line->sz_line * 2);
        }
        line->sz_line *= 2;
    }
    line->line[line->num_line++] = ch;
    return ch;
}

#define	ADD_CHAR(l,c)	((l)->num_line<(l)->sz_line?\
				(int)((l)->line[(l)->num_line++]= (c)):\
				InputLineAddChar(l,c))

#ifdef HAVE_UNLOCKED_STDIO
#undef getc
#define getc(x) getc_unlocked(x)
#else
#define flockfile(x) do {} while (0)
#define funlockfile(x) do {} while (0)
#endif

static Bool
GetInputLine(FILE *file, InputLine *line, Bool checkbang)
{
    int ch;
    Bool endOfFile, spacePending, slashPending, inComment;

    endOfFile = False;
    flockfile(file);
    while ((!endOfFile) && (line->num_line == 0)) {
        spacePending = slashPending = inComment = False;
        while (((ch = getc(file)) != '\n') && (ch != EOF)) {
            if (ch == '\\') {
                if ((ch = getc(file)) == EOF)
                    break;
                if (ch == '\n') {
                    inComment = False;
                    ch = ' ';
                    line->line_num++;
                }
            }
            if (inComment)
                continue;
            if (ch == '/') {
                if (slashPending) {
                    inComment = True;
                    slashPending = False;
                }
                else {
                    slashPending = True;
                }
                continue;
            }
            else if (slashPending) {
                if (spacePending) {
                    ADD_CHAR(line, ' ');
                    spacePending = False;
                }
                ADD_CHAR(line, '/');
                slashPending = False;
            }
            if (isspace(ch)) {
                while (isspace(ch) && (ch != '\n') && (ch != EOF)) {
                    ch = getc(file);
                }
                if (ch == EOF)
                    break;
                if ((ch != '\n') && (line->num_line > 0))
                    spacePending = True;
                ungetc(ch, file);
            }
            else {
                if (spacePending) {
                    ADD_CHAR(line, ' ');
                    spacePending = False;
                }
                if (checkbang && ch == '!') {
                    if (line->num_line != 0) {
                        PR_DEBUG("The '!' legal only at start of line\n");
                        PR_DEBUG("Line containing '!' ignored\n");
                        line->num_line = 0;
                        inComment = 0;
                        break;
                    }

                }
                ADD_CHAR(line, ch);
            }
        }
        if (ch == EOF)
            endOfFile = True;
/*	else line->num_line++;*/
    }
    funlockfile(file);
    if ((line->num_line == 0) && (endOfFile))
        return False;
    ADD_CHAR(line, '\0');
    return True;
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
#define	KEYMAP		9
#define	MAX_WORDS	10

#define	PART_MASK	0x000F
#define	COMPONENT_MASK	0x03F0

static const char *cname[MAX_WORDS] = {
    "model", "layout", "variant", "option",
    "keycodes", "symbols", "types", "compat", "geometry", "keymap"
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
    char *model;
    char *layout[XkbNumKbdGroups + 1];
    char *variant[XkbNumKbdGroups + 1];
    char *options;
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
    strncpy(ndx_buf, str, end - str);
    ndx_buf[end - str] = '\0';
    *ndx = atoi(ndx_buf);
    return end + 1;
}

static void
SetUpRemap(InputLine *line, RemapSpec *remap)
{
    char *tok, *str;
    unsigned present, l_ndx_present, v_ndx_present;
    register int i;
    int len, ndx;
    _Xstrtokparams strtok_buf;

#ifdef DEBUG
    Bool found;
#endif

    l_ndx_present = v_ndx_present = present = 0;
    str = &line->line[1];
    len = remap->number;
    bzero((char *) remap, sizeof(RemapSpec));
    remap->number = len;
    while ((tok = _XStrtok(str, " ", strtok_buf)) != NULL) {
#ifdef DEBUG
        found = False;
#endif
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
                        PR_DEBUG2("Illegal %s index: %d\n", cname[i], ndx);
                        PR_DEBUG1("Index must be in range 1..%d\n",
                                  XkbNumKbdGroups);
                        break;
                    }
                }
                else {
                    ndx = 0;
                }
#ifdef DEBUG
                found = True;
#endif
                if (present & (1 << i)) {
                    if ((i == LAYOUT && l_ndx_present & (1 << ndx)) ||
                        (i == VARIANT && v_ndx_present & (1 << ndx))) {
                        PR_DEBUG1("Component \"%s\" listed twice\n", tok);
                        PR_DEBUG("Second definition ignored\n");
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
#ifdef DEBUG
        if (!found) {
            fprintf(stderr, "Unknown component \"%s\" ignored\n", tok);
        }
#endif
    }
    if ((present & PART_MASK) == 0) {
#ifdef DEBUG
        unsigned mask = PART_MASK;

        fprintf(stderr, "Mapping needs at least one of ");
        for (i = 0; (i < MAX_WORDS); i++) {
            if ((1L << i) & mask) {
                mask &= ~(1L << i);
                if (mask)
                    fprintf(stderr, "\"%s,\" ", cname[i]);
                else
                    fprintf(stderr, "or \"%s\"\n", cname[i]);
            }
        }
        fprintf(stderr, "Illegal mapping ignored\n");
#endif
        remap->num_remap = 0;
        return;
    }
    if ((present & COMPONENT_MASK) == 0) {
        PR_DEBUG("Mapping needs at least one component\n");
        PR_DEBUG("Illegal mapping ignored\n");
        remap->num_remap = 0;
        return;
    }
    if (((present & COMPONENT_MASK) & (1 << KEYMAP)) &&
        ((present & COMPONENT_MASK) != (1 << KEYMAP))) {
        PR_DEBUG("Keymap cannot appear with other components\n");
        PR_DEBUG("Illegal mapping ignored\n");
        remap->num_remap = 0;
        return;
    }
    remap->number++;
    return;
}

static Bool
MatchOneOf(char *wanted, char *vals_defined)
{
    char *str, *next;
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
            return True;
    }
    return False;
}

/***====================================================================***/

static Bool
CheckLine(InputLine *    line,
          RemapSpec *    remap,
          XkbRF_RulePtr  rule,
          XkbRF_GroupPtr group)
{
    char *str, *tok;
    register int nread, i;
    FileSpec tmp;
    _Xstrtokparams strtok_buf;
    Bool append = False;

    if (line->line[0] == '!') {
        if (line->line[1] == '$' ||
            (line->line[1] == ' ' && line->line[2] == '$')) {
            char *gname = strchr(line->line, '$');
            char *words = strchr(gname, ' ');

            if (!words)
                return False;
            *words++ = '\0';
            for (; *words; words++) {
                if (*words != '=' && *words != ' ')
                    break;
            }
            if (*words == '\0')
                return False;
            group->name = _XkbDupString(gname);
            group->words = _XkbDupString(words);
            for (i = 1, words = group->words; *words; words++) {
                if (*words == ' ') {
                    *words++ = '\0';
                    i++;
                }
            }
            group->number = i;
            return True;
        }
        else {
            SetUpRemap(line, remap);
            return False;
        }
    }

    if (remap->num_remap == 0) {
        PR_DEBUG("Must have a mapping before first line of data\n");
        PR_DEBUG("Illegal line of data ignored\n");
        return False;
    }
    bzero((char *) &tmp, sizeof(FileSpec));
    str = line->line;
    for (nread = 0; (tok = _XStrtok(str, " ", strtok_buf)) != NULL; nread++) {
        str = NULL;
        if (strcmp(tok, "=") == 0) {
            nread--;
            continue;
        }
        if (nread > remap->num_remap) {
            PR_DEBUG("Too many words on a line\n");
            PR_DEBUG1("Extra word \"%s\" ignored\n", tok);
            continue;
        }
        tmp.name[remap->remap[nread].word] = tok;
        if (*tok == '+' || *tok == '|')
            append = True;
    }
    if (nread < remap->num_remap) {
        PR_DEBUG1("Too few words on a line: %s\n", line->line);
        PR_DEBUG("line ignored\n");
        return False;
    }

    rule->flags = 0;
    rule->number = remap->number;
    if (tmp.name[OPTION])
        rule->flags |= XkbRF_Option;
    else if (append)
        rule->flags |= XkbRF_Append;
    else
        rule->flags |= XkbRF_Normal;
    rule->model = _XkbDupString(tmp.name[MODEL]);
    rule->layout = _XkbDupString(tmp.name[LAYOUT]);
    rule->variant = _XkbDupString(tmp.name[VARIANT]);
    rule->option = _XkbDupString(tmp.name[OPTION]);

    rule->keycodes = _XkbDupString(tmp.name[KEYCODES]);
    rule->symbols = _XkbDupString(tmp.name[SYMBOLS]);
    rule->types = _XkbDupString(tmp.name[TYPES]);
    rule->compat = _XkbDupString(tmp.name[COMPAT]);
    rule->geometry = _XkbDupString(tmp.name[GEOMETRY]);
    rule->keymap = _XkbDupString(tmp.name[KEYMAP]);

    rule->layout_num = rule->variant_num = 0;
    for (i = 0; i < nread; i++) {
        if (remap->remap[i].index) {
            if (remap->remap[i].word == LAYOUT)
                rule->layout_num = remap->remap[i].index;
            if (remap->remap[i].word == VARIANT)
                rule->variant_num = remap->remap[i].index;
        }
    }
    return True;
}

static char *
_Concat(char *str1, char *str2)
{
    int len;

    if ((!str1) || (!str2))
        return str1;
    len = strlen(str1) + strlen(str2) + 1;
    str1 = _XkbTypedRealloc(str1, len, char);
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

    bzero((char *) mdefs, sizeof(XkbRF_MultiDefsRec));
    mdefs->model = defs->model;
    mdefs->options = _XkbDupString(defs->options);
    if (mdefs->options)
        squeeze_spaces(mdefs->options);

    if (defs->layout) {
        if (!strchr(defs->layout, ',')) {
            mdefs->layout[0] = defs->layout;
        }
        else {
            char *p;

            int i;

            mdefs->layout[1] = _XkbDupString(defs->layout);
            if (mdefs->layout[1] == NULL)
                return False;
            squeeze_spaces(mdefs->layout[1]);
            p = mdefs->layout[1];
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

            int i;

            mdefs->variant[1] = _XkbDupString(defs->variant);
            if (mdefs->variant[1] == NULL)
                return False;
            squeeze_spaces(mdefs->variant[1]);
            p = mdefs->variant[1];
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
    return True;
}

static void
FreeMultiDefs(XkbRF_MultiDefsPtr defs)
{
    if (defs->options)
        _XkbFree(defs->options);
    if (defs->layout[1])
        _XkbFree(defs->layout[1]);
    if (defs->variant[1])
        _XkbFree(defs->variant[1]);
}

static void
Apply(char *src, char **dst)
{
    if (src) {
        if (*src == '+' || *src == '|') {
            *dst = _Concat(*dst, src);
        }
        else {
            if (*dst == NULL)
                *dst = _XkbDupString(src);
        }
    }
}

static void
XkbRF_ApplyRule(XkbRF_RulePtr rule, XkbComponentNamesPtr names)
{
    rule->flags &= ~XkbRF_PendingMatch; /* clear the flag because it's applied */

    Apply(rule->keycodes, &names->keycodes);
    Apply(rule->symbols,  &names->symbols);
    Apply(rule->types,    &names->types);
    Apply(rule->compat,   &names->compat);
    Apply(rule->geometry, &names->geometry);
    Apply(rule->keymap,   &names->keymap);
}

static Bool
CheckGroup(XkbRF_RulesPtr rules, char *group_name, char *name)
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
        return False;
    for (i = 0, p = group->words; i < group->number; i++, p += strlen(p) + 1) {
        if (!strcmp(p, name)) {
            return True;
        }
    }
    return False;
}

static int
XkbRF_CheckApplyRule(XkbRF_RulePtr        rule,
                     XkbRF_MultiDefsPtr   mdefs,
                     XkbComponentNamesPtr names,
                     XkbRF_RulesPtr       rules)
{
    Bool pending = False;

    if (rule->model != NULL) {
        if (mdefs->model == NULL)
            return 0;
        if (strcmp(rule->model, "*") == 0) {
            pending = True;
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
            pending = True;
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
            pending = True;
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
XkbRF_CheckApplyRules(XkbRF_RulesPtr       rules,
                      XkbRF_MultiDefsPtr   mdefs,
                      XkbComponentNamesPtr names,
                      int                  flags)
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
    name = (char *) _XkbAlloc(len + 1);
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
        _XkbFree(orig);
    return name;
}

/***====================================================================***/

Bool
XkbRF_GetComponents(XkbRF_RulesPtr       rules,
                    XkbRF_VarDefsPtr     defs,
                    XkbComponentNamesPtr names)
{
    XkbRF_MultiDefsRec mdefs;

    MakeMultiDefs(&mdefs, defs);

    bzero((char *) names, sizeof(XkbComponentNamesRec));
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
        names->symbols  = XkbRF_SubstituteVars(names->symbols, &mdefs);
    if (names->types)
        names->types    = XkbRF_SubstituteVars(names->types, &mdefs);
    if (names->compat)
        names->compat   = XkbRF_SubstituteVars(names->compat, &mdefs);
    if (names->geometry)
        names->geometry = XkbRF_SubstituteVars(names->geometry, &mdefs);
    if (names->keymap)
        names->keymap   = XkbRF_SubstituteVars(names->keymap, &mdefs);

    FreeMultiDefs(&mdefs);
    return (names->keycodes && names->symbols && names->types &&
            names->compat && names->geometry) || names->keymap;
}

XkbRF_RulePtr
XkbRF_AddRule(XkbRF_RulesPtr rules)
{
    if (rules->sz_rules < 1) {
        rules->sz_rules = 16;
        rules->num_rules = 0;
        rules->rules = _XkbTypedCalloc(rules->sz_rules, XkbRF_RuleRec);
    }
    else if (rules->num_rules >= rules->sz_rules) {
        rules->sz_rules *= 2;
        rules->rules = _XkbTypedRealloc(rules->rules, rules->sz_rules,
                                        XkbRF_RuleRec);
    }
    if (!rules->rules) {
        rules->sz_rules = rules->num_rules = 0;
#ifdef DEBUG
        fprintf(stderr, "Allocation failure in XkbRF_AddRule\n");
#endif
        return NULL;
    }
    bzero((char *) &rules->rules[rules->num_rules], sizeof(XkbRF_RuleRec));
    return &rules->rules[rules->num_rules++];
}

XkbRF_GroupPtr
XkbRF_AddGroup(XkbRF_RulesPtr rules)
{
    if (rules->sz_groups < 1) {
        rules->sz_groups = 16;
        rules->num_groups = 0;
        rules->groups = _XkbTypedCalloc(rules->sz_groups, XkbRF_GroupRec);
    }
    else if (rules->num_groups >= rules->sz_groups) {
        rules->sz_groups *= 2;
        rules->groups = _XkbTypedRealloc(rules->groups, rules->sz_groups,
                                         XkbRF_GroupRec);
    }
    if (!rules->groups) {
        rules->sz_groups = rules->num_groups = 0;
        return NULL;
    }

    bzero((char *) &rules->groups[rules->num_groups], sizeof(XkbRF_GroupRec));
    return &rules->groups[rules->num_groups++];
}

Bool
XkbRF_LoadRules(FILE *file, XkbRF_RulesPtr rules)
{
    InputLine line;
    RemapSpec remap;
    XkbRF_RuleRec trule, *rule;
    XkbRF_GroupRec tgroup, *group;

    if (!(rules && file))
        return False;
    bzero((char *) &remap, sizeof(RemapSpec));
    bzero((char *) &tgroup, sizeof(XkbRF_GroupRec));
    InitInputLine(&line);
    while (GetInputLine(file, &line, True)) {
        if (CheckLine(&line, &remap, &trule, &tgroup)) {
            if (tgroup.number) {
                if ((group = XkbRF_AddGroup(rules)) != NULL) {
                    *group = tgroup;
                    bzero((char *) &tgroup, sizeof(XkbRF_GroupRec));
                }
            }
            else {
                if ((rule = XkbRF_AddRule(rules)) != NULL) {
                    *rule = trule;
                    bzero((char *) &trule, sizeof(XkbRF_RuleRec));
                }
            }
        }
        line.num_line = 0;
    }
    FreeInputLine(&line);
    return True;
}

Bool
XkbRF_LoadRulesByName(char *base, char *locale, XkbRF_RulesPtr rules)
{
    FILE *file;
    char buf[PATH_MAX];
    Bool ok;

    if ((!base) || (!rules))
        return False;
    if (locale) {
        if (strlen(base) + strlen(locale) + 2 > PATH_MAX)
            return False;
        snprintf(buf, sizeof(buf), "%s-%s", base, locale);
    }
    else {
        if (strlen(base) + 1 > PATH_MAX)
            return False;
        strcpy(buf, base);
    }

    file = fopen(buf, "r" FOPEN_CLOEXEC);
    if ((!file) && (locale)) {  /* fallback if locale was specified */
        strcpy(buf, base);
        file = fopen(buf, "r" FOPEN_CLOEXEC);
    }
    if (!file)
        return False;
    ok = XkbRF_LoadRules(file, rules);
    fclose(file);
    return ok;
}

/***====================================================================***/

#define HEAD_NONE	0
#define HEAD_MODEL	1
#define HEAD_LAYOUT	2
#define HEAD_VARIANT	3
#define HEAD_OPTION	4
#define	HEAD_EXTRA	5

XkbRF_VarDescPtr
XkbRF_AddVarDesc(XkbRF_DescribeVarsPtr vars)
{
    if (vars->sz_desc < 1) {
        vars->sz_desc = 16;
        vars->num_desc = 0;
        vars->desc = _XkbTypedCalloc(vars->sz_desc, XkbRF_VarDescRec);
    }
    else if (vars->num_desc >= vars->sz_desc) {
        vars->sz_desc *= 2;
        vars->desc =
            _XkbTypedRealloc(vars->desc, vars->sz_desc, XkbRF_VarDescRec);
    }
    if (!vars->desc) {
        vars->sz_desc = vars->num_desc = 0;
        PR_DEBUG("Allocation failure in XkbRF_AddVarDesc\n");
        return NULL;
    }
    vars->desc[vars->num_desc].name = NULL;
    vars->desc[vars->num_desc].desc = NULL;
    return &vars->desc[vars->num_desc++];
}

XkbRF_VarDescPtr
XkbRF_AddVarDescCopy(XkbRF_DescribeVarsPtr vars, XkbRF_VarDescPtr from)
{
    XkbRF_VarDescPtr nd;

    if ((nd = XkbRF_AddVarDesc(vars)) != NULL) {
        nd->name = _XkbDupString(from->name);
        nd->desc = _XkbDupString(from->desc);
    }
    return nd;
}

XkbRF_DescribeVarsPtr
XkbRF_AddVarToDescribe(XkbRF_RulesPtr rules, char *name)
{
    if (rules->sz_extra < 1) {
        rules->num_extra = 0;
        rules->sz_extra = 1;
        rules->extra_names = _XkbTypedCalloc(rules->sz_extra, char *);

        rules->extra = _XkbTypedCalloc(rules->sz_extra, XkbRF_DescribeVarsRec);
    }
    else if (rules->num_extra >= rules->sz_extra) {
        rules->sz_extra *= 2;
        rules->extra_names =
            _XkbTypedRealloc(rules->extra_names, rules->sz_extra, char *);
        rules->extra =
            _XkbTypedRealloc(rules->extra, rules->sz_extra,
                             XkbRF_DescribeVarsRec);
    }
    if ((!rules->extra_names) || (!rules->extra)) {
        PR_DEBUG("allocation error in extra parts\n");
        rules->sz_extra = rules->num_extra = 0;
        rules->extra_names = NULL;
        rules->extra = NULL;
        return NULL;
    }
    rules->extra_names[rules->num_extra] = _XkbDupString(name);
    bzero(&rules->extra[rules->num_extra], sizeof(XkbRF_DescribeVarsRec));
    return &rules->extra[rules->num_extra++];
}

Bool
XkbRF_LoadDescriptions(FILE *file, XkbRF_RulesPtr rules)
{
    InputLine line;
    XkbRF_VarDescRec tmp;
    char *tok;
    int len, headingtype, extra_ndx = 0;

    bzero((char *) &tmp, sizeof(XkbRF_VarDescRec));
    headingtype = HEAD_NONE;
    InitInputLine(&line);
    for (; GetInputLine(file, &line, False); line.num_line = 0) {
        if (line.line[0] == '!') {
            tok = strtok(&(line.line[1]), " \t");
            if (strcmp(tok, "model") == 0)
                headingtype = HEAD_MODEL;
            else if (_XkbStrCaseCmp(tok, "layout") == 0)
                headingtype = HEAD_LAYOUT;
            else if (_XkbStrCaseCmp(tok, "variant") == 0)
                headingtype = HEAD_VARIANT;
            else if (_XkbStrCaseCmp(tok, "option") == 0)
                headingtype = HEAD_OPTION;
            else {
                int i;

                headingtype = HEAD_EXTRA;
                extra_ndx = -1;
                for (i = 0; (i < rules->num_extra) && (extra_ndx < 0); i++) {
                    if (_XkbStrCaseCmp(tok, rules->extra_names[i]))
                        extra_ndx = i;
                }
                if (extra_ndx < 0) {
                    XkbRF_DescribeVarsPtr var;

                    PR_DEBUG1("Extra heading \"%s\" encountered\n", tok);
                    var = XkbRF_AddVarToDescribe(rules, tok);
                    if (var)
                        extra_ndx = var - rules->extra;
                    else
                        headingtype = HEAD_NONE;
                }
            }
            continue;
        }

        if (headingtype == HEAD_NONE) {
            PR_DEBUG("Must have a heading before first line of data\n");
            PR_DEBUG("Illegal line of data ignored\n");
            continue;
        }

        len = strlen(line.line);
        if ((tmp.name = strtok(line.line, " \t")) == NULL) {
            PR_DEBUG("Huh? No token on line\n");
            PR_DEBUG("Illegal line of data ignored\n");
            continue;
        }
        if (strlen(tmp.name) == len) {
            PR_DEBUG("No description found\n");
            PR_DEBUG("Illegal line of data ignored\n");
            continue;
        }

        tok = line.line + strlen(tmp.name) + 1;
        while ((*tok != '\n') && isspace(*tok))
            tok++;
        if (*tok == '\0') {
            PR_DEBUG("No description found\n");
            PR_DEBUG("Illegal line of data ignored\n");
            continue;
        }
        tmp.desc = tok;
        switch (headingtype) {
        case HEAD_MODEL:
            XkbRF_AddVarDescCopy(&rules->models, &tmp);
            break;
        case HEAD_LAYOUT:
            XkbRF_AddVarDescCopy(&rules->layouts, &tmp);
            break;
        case HEAD_VARIANT:
            XkbRF_AddVarDescCopy(&rules->variants, &tmp);
            break;
        case HEAD_OPTION:
            XkbRF_AddVarDescCopy(&rules->options, &tmp);
            break;
        case HEAD_EXTRA:
            XkbRF_AddVarDescCopy(&rules->extra[extra_ndx], &tmp);
            break;
        }
    }
    FreeInputLine(&line);
    if ((rules->models.num_desc == 0) && (rules->layouts.num_desc == 0) &&
        (rules->variants.num_desc == 0) && (rules->options.num_desc == 0) &&
        (rules->num_extra == 0)) {
        return False;
    }
    return True;
}

Bool
XkbRF_LoadDescriptionsByName(char *base, char *locale, XkbRF_RulesPtr rules)
{
    FILE *file;
    char buf[PATH_MAX];
    Bool ok;

    if ((!base) || (!rules))
        return False;
    if (locale) {
        if (strlen(base) + strlen(locale) + 6 > PATH_MAX)
            return False;
        snprintf(buf, sizeof(buf), "%s-%s.lst", base, locale);
    }
    else {
        if (strlen(base) + 5 > PATH_MAX)
            return False;
        snprintf(buf, sizeof(buf), "%s.lst", base);
    }

    file = fopen(buf, "r" FOPEN_CLOEXEC);
    if ((!file) && (locale)) {  /* fallback if locale was specified */
        snprintf(buf, sizeof(buf), "%s.lst", base);

        file = fopen(buf, "r" FOPEN_CLOEXEC);
    }
    if (!file)
        return False;
    ok = XkbRF_LoadDescriptions(file, rules);
    fclose(file);
    return ok;
}

/***====================================================================***/

XkbRF_RulesPtr
XkbRF_Load(char *base, char *locale, Bool wantDesc, Bool wantRules)
{
    XkbRF_RulesPtr rules;

    if ((!base) || ((!wantDesc) && (!wantRules)))
        return NULL;
    if ((rules = _XkbTypedCalloc(1, XkbRF_RulesRec)) == NULL)
        return NULL;
    if (wantDesc && (!XkbRF_LoadDescriptionsByName(base, locale, rules))) {
        XkbRF_Free(rules, True);
        return NULL;
    }
    if (wantRules && (!XkbRF_LoadRulesByName(base, locale, rules))) {
        XkbRF_Free(rules, True);
        return NULL;
    }
    return rules;
}

XkbRF_RulesPtr
XkbRF_Create(int szRules, int szExtra)
{
    XkbRF_RulesPtr rules;

    if ((rules = _XkbTypedCalloc(1, XkbRF_RulesRec)) == NULL)
        return NULL;
    if (szRules > 0) {
        rules->sz_rules = szRules;
        rules->rules = _XkbTypedCalloc(rules->sz_rules, XkbRF_RuleRec);
        if (!rules->rules) {
            _XkbFree(rules);
            return NULL;
        }
    }
    if (szExtra > 0) {
        rules->sz_extra = szExtra;
        rules->extra = _XkbTypedCalloc(rules->sz_extra, XkbRF_DescribeVarsRec);
        if (!rules->extra) {
            if (rules->rules)
                _XkbFree(rules->rules);
            _XkbFree(rules);
            return NULL;
        }
    }
    return rules;
}

/***====================================================================***/

static void
XkbRF_ClearVarDescriptions(XkbRF_DescribeVarsPtr var)
{
    register int i;

    for (i = 0; i < var->num_desc; i++) {
        if (var->desc[i].name)
            _XkbFree(var->desc[i].name);
        if (var->desc[i].desc)
            _XkbFree(var->desc[i].desc);
        var->desc[i].name = var->desc[i].desc = NULL;
    }
    if (var->desc)
        _XkbFree(var->desc);
    var->desc = NULL;
    return;
}

void
XkbRF_Free(XkbRF_RulesPtr rules, Bool freeRules)
{
    int i;
    XkbRF_RulePtr rule;
    XkbRF_GroupPtr group;

    if (!rules)
        return;
    XkbRF_ClearVarDescriptions(&rules->models);
    XkbRF_ClearVarDescriptions(&rules->layouts);
    XkbRF_ClearVarDescriptions(&rules->variants);
    XkbRF_ClearVarDescriptions(&rules->options);
    if (rules->extra) {
        for (i = 0; i < rules->num_extra; i++) {
            XkbRF_ClearVarDescriptions(&rules->extra[i]);
        }
        _XkbFree(rules->extra);
        rules->num_extra = rules->sz_extra = 0;
        rules->extra = NULL;
    }
    if (rules->rules) {
        for (i = 0, rule = rules->rules; i < rules->num_rules; i++, rule++) {
            if (rule->model)
                _XkbFree(rule->model);
            if (rule->layout)
                _XkbFree(rule->layout);
            if (rule->variant)
                _XkbFree(rule->variant);
            if (rule->option)
                _XkbFree(rule->option);
            if (rule->keycodes)
                _XkbFree(rule->keycodes);
            if (rule->symbols)
                _XkbFree(rule->symbols);
            if (rule->types)
                _XkbFree(rule->types);
            if (rule->compat)
                _XkbFree(rule->compat);
            if (rule->geometry)
                _XkbFree(rule->geometry);
            if (rule->keymap)
                _XkbFree(rule->keymap);
            bzero((char *) rule, sizeof(XkbRF_RuleRec));
        }
        _XkbFree(rules->rules);
        rules->num_rules = rules->sz_rules = 0;
        rules->rules = NULL;
    }

    if (rules->groups) {
        for (i = 0, group = rules->groups; i < rules->num_groups; i++, group++) {
            if (group->name)
                _XkbFree(group->name);
            if (group->words)
                _XkbFree(group->words);
        }
        _XkbFree(rules->groups);
        rules->num_groups = 0;
        rules->groups = NULL;
    }
    if (freeRules)
        _XkbFree(rules);
    return;
}


Bool
XkbRF_GetNamesProp(Display * dpy, char **rf_rtrn, XkbRF_VarDefsPtr vd_rtrn)
{
    Atom rules_atom, actual_type;
    int fmt;
    unsigned long nitems, bytes_after;
    unsigned char *data;
    char *out, *end;
    Status rtrn;

    rules_atom = XInternAtom(dpy, _XKB_RF_NAMES_PROP_ATOM, True);
    if (rules_atom == None)     /* property cannot exist */
        return False;
    rtrn = XGetWindowProperty(dpy, DefaultRootWindow(dpy), rules_atom,
                              0L, _XKB_RF_NAMES_PROP_MAXLEN, False,
                              XA_STRING, &actual_type,
                              &fmt, &nitems, &bytes_after,
                              (unsigned char **) &data);
    if (rtrn != Success)
        return False;
    if (rf_rtrn)
        *rf_rtrn = NULL;
    (void) bzero((char *) vd_rtrn, sizeof(XkbRF_VarDefsRec));
    if ((bytes_after > 0) || (actual_type != XA_STRING) || (fmt != 8)) {
        if (data)
            XFree(data);
        return (fmt == 0 ? True : False);
    }

    out = (char *) data;
    end = out + nitems;
    if (out && (*out) && rf_rtrn)
        *rf_rtrn = _XkbDupString(out);
    out += strlen(out) + 1;

    if (out < end) {
        if (*out)
            vd_rtrn->model = _XkbDupString(out);
        out += strlen(out) + 1;
    }

    if (out < end) {
        if (*out)
            vd_rtrn->layout = _XkbDupString(out);
        out += strlen(out) + 1;
    }

    if (out < end) {
        if (*out)
            vd_rtrn->variant = _XkbDupString(out);
        out += strlen(out) + 1;
    }

    if (out < end) {
        if (*out)
            vd_rtrn->options = _XkbDupString(out);
        out += strlen(out) + 1;
    }

    XFree(data);
    return True;
}

Bool
XkbRF_SetNamesProp(Display *dpy, char *rules_file, XkbRF_VarDefsPtr var_defs)
{
    int len, out;
    Atom name;
    char *pval;

    len = (rules_file ? strlen(rules_file) : 0);
    len += (var_defs->model ? strlen(var_defs->model) : 0);
    len += (var_defs->layout ? strlen(var_defs->layout) : 0);
    len += (var_defs->variant ? strlen(var_defs->variant) : 0);
    len += (var_defs->options ? strlen(var_defs->options) : 0);
    if (len < 1)
        return True;

    len += 5;                   /* trailing NULs */

    name = XInternAtom(dpy, _XKB_RF_NAMES_PROP_ATOM, False);
    if (name == None) {         /* should never happen */
        _XkbLibError(_XkbErrXReqFailure, "XkbRF_SetNamesProp", X_InternAtom);
        return False;
    }
    pval = (char *) _XkbAlloc(len);
    if (!pval) {
        _XkbLibError(_XkbErrBadAlloc, "XkbRF_SetNamesProp", len);
        return False;
    }
    out = 0;
    if (rules_file) {
        strcpy(&pval[out], rules_file);
        out += strlen(rules_file);
    }
    pval[out++] = '\0';
    if (var_defs->model) {
        strcpy(&pval[out], var_defs->model);
        out += strlen(var_defs->model);
    }
    pval[out++] = '\0';
    if (var_defs->layout) {
        strcpy(&pval[out], var_defs->layout);
        out += strlen(var_defs->layout);
    }
    pval[out++] = '\0';
    if (var_defs->variant) {
        strcpy(&pval[out], var_defs->variant);
        out += strlen(var_defs->variant);
    }
    pval[out++] = '\0';
    if (var_defs->options) {
        strcpy(&pval[out], var_defs->options);
        out += strlen(var_defs->options);
    }
    pval[out++] = '\0';
    if (out != len) {
        _XkbLibError(_XkbErrBadLength, "XkbRF_SetNamesProp", out);
        _XkbFree(pval);
        return False;
    }

    XChangeProperty(dpy, DefaultRootWindow(dpy), name, XA_STRING, 8,
                    PropModeReplace, (unsigned char *) pval, len);
    _XkbFree(pval);
    return True;
}

