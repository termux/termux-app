#ifndef OPTIONSTR_H_
#define OPTIONSTR_H_
#include "list.h"

struct _InputOption {
    GenericListRec list;
    char *opt_name;
    char *opt_val;
    int opt_used;
    char *opt_comment;
};

#endif                          /* INPUTSTRUCT_H */
