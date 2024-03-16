/*

Copyright 1990, 1998  The Open Group

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

/* Constructs hash tables for XStringToKeysym and XKeysymToString. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

typedef uint32_t Signature;

#define KTNUM 4000

#define XK_VoidSymbol                  0xffffff  /* Void symbol */

typedef unsigned long KeySym;

static struct info {
    char	*name;
    KeySym	val;
} info[KTNUM];

#define MIN_REHASH 15
#define MATCHES 10

static char tab[KTNUM];
static unsigned short offsets[KTNUM];
static unsigned short indexes[KTNUM];
static KeySym values[KTNUM];
static int ksnum = 0;

static int
parse_line(const char *buf, char *key, KeySym *val, char *prefix)
{
    int i;
    char alias[128];
    char *tmp, *tmpa;

    /* See if we can catch a straight XK_foo 0x1234-style definition first;
     * the trickery around tmp is to account for prefixes. */
    i = sscanf(buf, "#define %127s 0x%lx", key, val);
    if (i == 2 && (tmp = strstr(key, "XK_"))) {
        memcpy(prefix, key, (size_t)(tmp - key));
        prefix[tmp - key] = '\0';
        tmp += 3;
        memmove(key, tmp, strlen(tmp) + 1);
        return 1;
    }

    /* See if we can parse one of the _EVDEVK symbols */
    i = sscanf(buf, "#define %127s _EVDEVK(0x%lx)", key, val);
    if (i == 2 && (tmp = strstr(key, "XK_"))) {
        memcpy(prefix, key, (size_t)(tmp - key));
        prefix[tmp - key] = '\0';
        tmp += 3;
        memmove(key, tmp, strlen(tmp) + 1);

        *val += 0x10081000;
        return 1;
    }

    /* Now try to catch alias (XK_foo XK_bar) definitions, and resolve them
     * immediately: if the target is in the form XF86XK_foo, we need to
     * canonicalise this to XF86foo before we do the lookup. */
    i = sscanf(buf, "#define %127s %127s", key, alias);
    if (i == 2 && (tmp = strstr(key, "XK_")) && (tmpa = strstr(alias, "XK_"))) {
        memcpy(prefix, key, (size_t)(tmp - key));
        prefix[tmp - key] = '\0';
        tmp += 3;
        memmove(key, tmp, strlen(tmp) + 1);
        memmove(tmpa, tmpa + 3, strlen(tmpa + 3) + 1);

        for (i = ksnum - 1; i >= 0; i--) {
            if (strcmp(info[i].name, alias) == 0) {
                *val = info[i].val;
                return 1;
            }
        }

        fprintf(stderr, "can't find matching definition %s for keysym %s%s\n",
                alias, prefix, key);
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    int max_rehash;
    Signature sig;
    int i, j, k, l, z;
    FILE *fptr;
    char *name;
    char c;
    int first;
    int best_max_rehash;
    int best_z = 0;
    int num_found;
    KeySym val;
    char key[128], prefix[128];
    static char buf[1024];

    for (l = 1; l < argc; l++) {
        fptr = fopen(argv[l], "r");
        if (!fptr) {
            fprintf(stderr, "couldn't open %s\n", argv[l]);
            continue;
        }

        while (fgets(buf, sizeof(buf), fptr)) {
            if (!parse_line(buf, key, &val, prefix))
                continue;

            if (val == XK_VoidSymbol)
                val = 0;
            if (val > 0x1fffffff) {
                fprintf(stderr, "ignoring illegal keysym (%s, %lx)\n", key,
                        val);
                continue;
            }

            name = malloc(strlen(prefix) + strlen(key) + 1);
            if (!name) {
                fprintf(stderr, "makekeys: out of memory!\n");
                exit(1);
            }
            sprintf(name, "%s%s", prefix, key);
            info[ksnum].name = name;
            info[ksnum].val = val;
            ksnum++;
            if (ksnum == KTNUM) {
                fprintf(stderr, "makekeys: too many keysyms!\n");
                exit(1);
            }
        }

        fclose(fptr);
    }

    printf("/* This file is generated from keysymdef.h. */\n");
    printf("/* Do not edit. */\n");
    printf("\n");

    best_max_rehash = ksnum;
    num_found = 0;
    for (z = ksnum; z < KTNUM; z++) {
	max_rehash = 0;
	for (name = tab, i = z; --i >= 0;)
		*name++ = 0;
	for (i = 0; i < ksnum; i++) {
	    name = info[i].name;
	    sig = 0;
	    while ((c = *name++))
		sig = (sig << 1) + c;
	    first = j = sig % z;
	    for (k = 0; tab[j]; k++) {
		j += first + 1;
		if (j >= z)
		    j -= z;
		if (j == first)
		    goto next1;
	    }
	    tab[j] = 1;
	    if (k > max_rehash)
		max_rehash = k;
	}
	if (max_rehash < MIN_REHASH) {
	    if (max_rehash < best_max_rehash) {
		best_max_rehash = max_rehash;
		best_z = z;
	    }
	    num_found++;
	    if (num_found >= MATCHES)
		break;
	}
next1:	;
    }

    z = best_z;
    if (z == 0) {
	fprintf(stderr, "makekeys: failed to find small enough hash!\n"
		"Try increasing KTNUM in makekeys.c\n");
	exit(1);
    }
    printf("#ifdef NEEDKTABLE\n");
    printf("const unsigned char _XkeyTable[] = {\n");
    printf("0,\n");
    k = 1;
    for (i = 0; i < ksnum; i++) {
	name = info[i].name;
	sig = 0;
	while ((c = *name++))
	    sig = (sig << 1) + c;
	first = j = sig % z;
	while (offsets[j]) {
	    j += first + 1;
	    if (j >= z)
		j -= z;
	}
	offsets[j] = k;
	indexes[i] = k;
	val = info[i].val;
	printf("0x%.2"PRIx32", 0x%.2"PRIx32", 0x%.2lx, 0x%.2lx, 0x%.2lx, 0x%.2lx, ",
	       (sig >> 8) & 0xff, sig & 0xff,
	       (val >> 24) & 0xff, (val >> 16) & 0xff,
	       (val >> 8) & 0xff, val & 0xff);
	for (name = info[i].name, k += 7; (c = *name++); k++)
	    printf("'%c',", c);
	printf((i == (ksnum-1)) ? "0\n" : "0,\n");
    }
    printf("};\n");
    printf("\n");
    printf("#define KTABLESIZE %d\n", z);
    printf("#define KMAXHASH %d\n", best_max_rehash + 1);
    printf("\n");
    printf("static const unsigned short hashString[KTABLESIZE] = {\n");
    for (i = 0; i < z;) {
	printf("0x%.4x", offsets[i]);
	i++;
	if (i == z)
	    break;
	printf((i & 7) ? ", " : ",\n");
    }
    printf("\n");
    printf("};\n");
    printf("#endif /* NEEDKTABLE */\n");

    best_max_rehash = ksnum;
    num_found = 0;
    for (z = ksnum; z < KTNUM; z++) {
	max_rehash = 0;
	for (name = tab, i = z; --i >= 0;)
		*name++ = 0;
	for (i = 0; i < ksnum; i++) {
	    val = info[i].val;
	    first = j = val % z;
	    for (k = 0; tab[j]; k++) {
		if (values[j] == val)
		    goto skip1;
		j += first + 1;
		if (j >= z)
		    j -= z;
		if (j == first)
		    goto next2;
	    }
	    tab[j] = 1;
	    values[j] = val;
	    if (k > max_rehash)
		max_rehash = k;
skip1:	;
	}
	if (max_rehash < MIN_REHASH) {
	    if (max_rehash < best_max_rehash) {
		best_max_rehash = max_rehash;
		best_z = z;
	    }
	    num_found++;
	    if (num_found >= MATCHES)
		break;
	}
next2:	;
    }

    z = best_z;
    if (z == 0) {
	fprintf(stderr, "makekeys: failed to find small enough hash!\n"
		"Try increasing KTNUM in makekeys.c\n");
	exit(1);
    }
    for (i = z; --i >= 0;)
	offsets[i] = 0;
    for (i = 0; i < ksnum; i++) {
	val = info[i].val;
	first = j = val % z;
	while (offsets[j]) {
	    if (values[j] == val)
		goto skip2;
	    j += first + 1;
	    if (j >= z)
		j -= z;
	}
	offsets[j] = indexes[i] + 2;
	values[j] = val;
skip2:	;
    }
    printf("\n");
    printf("#ifdef NEEDVTABLE\n");
    printf("#define VTABLESIZE %d\n", z);
    printf("#define VMAXHASH %d\n", best_max_rehash + 1);
    printf("\n");
    printf("static const unsigned short hashKeysym[VTABLESIZE] = {\n");
    for (i = 0; i < z;) {
	printf("0x%.4x", offsets[i]);
	i++;
	if (i == z)
	    break;
	printf((i & 7) ? ", " : ",\n");
    }
    printf("\n");
    printf("};\n");
    printf("#endif /* NEEDVTABLE */\n");

    exit(0);
}
