
/*
 * Copyright (c) 1998-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/* Option handling things that ModuleSetup procs can use */

#ifndef _XF86_OPT_H_
#define _XF86_OPT_H_
#include "xf86Optionstr.h"

typedef struct {
    double freq;
    int units;
} OptFrequency;

typedef union {
    unsigned long num;
    const char *str;
    double realnum;
    Bool boolean;
    OptFrequency freq;
} ValueUnion;

typedef enum {
    OPTV_NONE = 0,
    OPTV_INTEGER,
    OPTV_STRING,                /* a non-empty string */
    OPTV_ANYSTR,                /* Any string, including an empty one */
    OPTV_REAL,
    OPTV_BOOLEAN,
    OPTV_PERCENT,
    OPTV_FREQ
} OptionValueType;

typedef enum {
    OPTUNITS_HZ = 1,
    OPTUNITS_KHZ,
    OPTUNITS_MHZ
} OptFreqUnits;

typedef struct {
    int token;
    const char *name;
    OptionValueType type;
    ValueUnion value;
    Bool found;
} OptionInfoRec, *OptionInfoPtr;

extern _X_EXPORT int xf86SetIntOption(XF86OptionPtr optlist, const char *name,
                                      int deflt);
extern _X_EXPORT double xf86SetRealOption(XF86OptionPtr optlist,
                                          const char *name, double deflt);
extern _X_EXPORT char *xf86SetStrOption(XF86OptionPtr optlist, const char *name,
                                        const char *deflt);
extern _X_EXPORT int xf86SetBoolOption(XF86OptionPtr list, const char *name,
                                       int deflt);
extern _X_EXPORT double xf86SetPercentOption(XF86OptionPtr list,
                                             const char *name, double deflt);
extern _X_EXPORT int xf86CheckIntOption(XF86OptionPtr optlist, const char *name,
                                        int deflt);
extern _X_EXPORT double xf86CheckRealOption(XF86OptionPtr optlist,
                                            const char *name, double deflt);
extern _X_EXPORT char *xf86CheckStrOption(XF86OptionPtr optlist,
                                          const char *name, const char *deflt);
extern _X_EXPORT int xf86CheckBoolOption(XF86OptionPtr list, const char *name,
                                         int deflt);
extern _X_EXPORT double xf86CheckPercentOption(XF86OptionPtr list,
                                               const char *name, double deflt);
extern _X_EXPORT XF86OptionPtr xf86AddNewOption(XF86OptionPtr head,
                                                const char *name,
                                                const char *val);
extern _X_EXPORT XF86OptionPtr xf86NewOption(char *name, char *value);
extern _X_EXPORT XF86OptionPtr xf86NextOption(XF86OptionPtr list);
extern _X_EXPORT XF86OptionPtr xf86OptionListCreate(const char **options,
                                                    int count, int used);
extern _X_EXPORT XF86OptionPtr xf86OptionListMerge(XF86OptionPtr head,
                                                   XF86OptionPtr tail);
extern _X_EXPORT XF86OptionPtr xf86OptionListDuplicate(XF86OptionPtr list);
extern _X_EXPORT void xf86OptionListFree(XF86OptionPtr opt);
extern _X_EXPORT char *xf86OptionName(XF86OptionPtr opt);
extern _X_EXPORT char *xf86OptionValue(XF86OptionPtr opt);
extern _X_EXPORT void xf86OptionListReport(XF86OptionPtr parm);
extern _X_EXPORT XF86OptionPtr xf86FindOption(XF86OptionPtr options,
                                              const char *name);
extern _X_EXPORT const char *xf86FindOptionValue(XF86OptionPtr options,
                                                 const char *name);
extern _X_EXPORT void xf86MarkOptionUsed(XF86OptionPtr option);
extern _X_EXPORT void xf86MarkOptionUsedByName(XF86OptionPtr options,
                                               const char *name);
extern _X_EXPORT Bool xf86CheckIfOptionUsed(XF86OptionPtr option);
extern _X_EXPORT Bool xf86CheckIfOptionUsedByName(XF86OptionPtr options,
                                                  const char *name);
extern _X_EXPORT void xf86ShowUnusedOptions(int scrnIndex,
                                            XF86OptionPtr options);
extern _X_EXPORT void xf86ProcessOptions(int scrnIndex, XF86OptionPtr options,
                                         OptionInfoPtr optinfo);
extern _X_EXPORT OptionInfoPtr xf86TokenToOptinfo(const OptionInfoRec * table,
                                                  int token);
extern _X_EXPORT const char *xf86TokenToOptName(const OptionInfoRec * table,
                                                int token);
extern _X_EXPORT Bool xf86IsOptionSet(const OptionInfoRec * table, int token);
extern _X_EXPORT const char *xf86GetOptValString(const OptionInfoRec * table,
                                           int token);
extern _X_EXPORT Bool xf86GetOptValInteger(const OptionInfoRec * table,
                                           int token, int *value);
extern _X_EXPORT Bool xf86GetOptValULong(const OptionInfoRec * table, int token,
                                         unsigned long *value);
extern _X_EXPORT Bool xf86GetOptValReal(const OptionInfoRec * table, int token,
                                        double *value);
extern _X_EXPORT Bool xf86GetOptValFreq(const OptionInfoRec * table, int token,
                                        OptFreqUnits expectedUnits,
                                        double *value);
extern _X_EXPORT Bool xf86GetOptValBool(const OptionInfoRec * table, int token,
                                        Bool *value);
extern _X_EXPORT Bool xf86ReturnOptValBool(const OptionInfoRec * table,
                                           int token, Bool def);
extern _X_EXPORT int xf86NameCmp(const char *s1, const char *s2);
extern _X_EXPORT char *xf86NormalizeName(const char *s);
extern _X_EXPORT XF86OptionPtr xf86ReplaceIntOption(XF86OptionPtr optlist,
                                                    const char *name,
                                                    const int val);
extern _X_EXPORT XF86OptionPtr xf86ReplaceRealOption(XF86OptionPtr optlist,
                                                     const char *name,
                                                     const double val);
extern _X_EXPORT XF86OptionPtr xf86ReplaceBoolOption(XF86OptionPtr optlist,
                                                     const char *name,
                                                     const Bool val);
extern _X_EXPORT XF86OptionPtr xf86ReplacePercentOption(XF86OptionPtr optlist,
                                                        const char *name,
                                                        const double val);
extern _X_EXPORT XF86OptionPtr xf86ReplaceStrOption(XF86OptionPtr optlist,
                                                    const char *name,
                                                    const char *val);
#endif
