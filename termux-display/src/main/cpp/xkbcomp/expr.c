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
#include "tokens.h"
#include "expr.h"

#include <ctype.h>

/***====================================================================***/

char *
exprOpText(unsigned type)
{
    static char buf[32];

    switch (type)
    {
    case ExprValue:
        strcpy(buf, "literal");
        break;
    case ExprIdent:
        strcpy(buf, "identifier");
        break;
    case ExprActionDecl:
        strcpy(buf, "action declaration");
        break;
    case ExprFieldRef:
        strcpy(buf, "field reference");
        break;
    case ExprArrayRef:
        strcpy(buf, "array reference");
        break;
    case ExprKeysymList:
        strcpy(buf, "list of keysyms");
        break;
    case ExprActionList:
        strcpy(buf, "list of actions");
        break;
    case OpAdd:
        strcpy(buf, "addition");
        break;
    case OpSubtract:
        strcpy(buf, "subtraction");
        break;
    case OpMultiply:
        strcpy(buf, "multiplication");
        break;
    case OpDivide:
        strcpy(buf, "division");
        break;
    case OpAssign:
        strcpy(buf, "assignment");
        break;
    case OpNot:
        strcpy(buf, "logical not");
        break;
    case OpNegate:
        strcpy(buf, "arithmetic negation");
        break;
    case OpInvert:
        strcpy(buf, "bitwise inversion");
        break;
    case OpUnaryPlus:
        strcpy(buf, "plus sign");
        break;
    default:
        snprintf(buf, sizeof(buf), "illegal(%d)", type);
        break;
    }
    return buf;
}

static char *
exprTypeText(unsigned type)
{
    static char buf[20];

    switch (type)
    {
    case TypeUnknown:
        strcpy(buf, "unknown");
        break;
    case TypeBoolean:
        strcpy(buf, "boolean");
        break;
    case TypeInt:
        strcpy(buf, "int");
        break;
    case TypeString:
        strcpy(buf, "string");
        break;
    case TypeAction:
        strcpy(buf, "action");
        break;
    case TypeKeyName:
        strcpy(buf, "keyname");
        break;
    default:
        snprintf(buf, sizeof(buf), "illegal(%d)", type);
        break;
    }
    return buf;
}

int
ExprResolveLhs(const ExprDef *expr, ExprResult *elem_rtrn,
               ExprResult *field_rtrn, ExprDef **index_rtrn)
{
    switch (expr->op)
    {
    case ExprIdent:
        elem_rtrn->str = NULL;
        field_rtrn->str = XkbAtomGetString(NULL, expr->value.str);
        *index_rtrn = NULL;
        return True;
    case ExprFieldRef:
        elem_rtrn->str = XkbAtomGetString(NULL, expr->value.field.element);
        field_rtrn->str = XkbAtomGetString(NULL, expr->value.field.field);
        *index_rtrn = NULL;
        return True;
    case ExprArrayRef:
        elem_rtrn->str = XkbAtomGetString(NULL, expr->value.array.element);
        field_rtrn->str = XkbAtomGetString(NULL, expr->value.array.field);
        *index_rtrn = expr->value.array.entry;
        return True;
    }
    WSGO("Unexpected operator %d in ResolveLhs\n", expr->op);
    return False;
}

Bool
SimpleLookup(const XPointer priv,
             Atom elem, Atom field, unsigned type, ExprResult *val_rtrn)
{
    char *str;

    if ((priv == NULL) ||
        (field == None) || (elem != None) ||
        ((type != TypeInt) && (type != TypeFloat)))
    {
        return False;
    }
    str = XkbAtomGetString(NULL, field);
    for (const LookupEntry *entry = (const LookupEntry *) priv;
         (entry != NULL) && (entry->name != NULL); entry++)
    {
        if (uStrCaseCmp(str, entry->name) == 0)
        {
            val_rtrn->uval = entry->result;
            if (type == TypeFloat)
                val_rtrn->uval *= XkbGeomPtsPerMM;
            return True;
        }
    }
    return False;
}

Bool
RadioLookup(const XPointer priv,
            Atom elem, Atom field, unsigned type, ExprResult *val_rtrn)
{
    char *str;
    int rg;

    if ((field == None) || (elem != None) || (type != TypeInt))
        return False;
    str = XkbAtomGetString(NULL, field);
    if (str)
    {
        if (uStrCasePrefix("group", str))
            str += strlen("group");
        else if (uStrCasePrefix("radiogroup", str))
            str += strlen("radiogroup");
        else if (uStrCasePrefix("rg", str))
            str += strlen("rg");
        else if (!isdigit(str[0]))
            str = NULL;
    }
    if ((!str) || (sscanf(str, "%i", &rg) < 1) || (rg < 1)
        || (rg > XkbMaxRadioGroups))
        return False;
    val_rtrn->uval = rg;
    return True;
}

#if 0
static int
TableLookup(XPointer priv,
            Atom elem, Atom field, unsigned type, ExprResult * val_rtrn)
{
    LookupTable *tbl = (LookupTable *) priv;
    char *str;

    if ((priv == NULL) || (field == None) || (type != TypeInt))
        return False;
    str = XkbAtomGetString(NULL, elem);
    while (tbl)
    {
        if (((str == NULL) && (tbl->element == NULL)) ||
            ((str != NULL) && (tbl->element != NULL) &&
             (uStrCaseCmp(str, tbl->element) == 0)))
        {
            break;
        }
        tbl = tbl->nextElement;
    }
    if (tbl == NULL)            /* didn't find a matching element */
        return False;
    priv = (XPointer) tbl->entries;
    return SimpleLookup(priv, (Atom) None, field, type, val_rtrn);
}
#endif

static LookupEntry modIndexNames[] = {
    {"shift", ShiftMapIndex},
    {"control", ControlMapIndex},
    {"lock", LockMapIndex},
    {"mod1", Mod1MapIndex},
    {"mod2", Mod2MapIndex},
    {"mod3", Mod3MapIndex},
    {"mod4", Mod4MapIndex},
    {"mod5", Mod5MapIndex},
    {"none", XkbNoModifier},
    {NULL, 0}
};

Bool
LookupModIndex(const XPointer priv,
               Atom elem, Atom field, unsigned type, ExprResult *val_rtrn)
{
    return SimpleLookup((XPointer) modIndexNames, elem, field, type,
                        val_rtrn);
}

static int
LookupModMask(XPointer priv,
              Atom elem, Atom field, unsigned type, ExprResult * val_rtrn)
{
    char *str;

    if ((elem != None) || (type != TypeInt))
        return False;
    str = XkbAtomGetString(NULL, field);
    if (str == NULL)
        return False;
    if (uStrCaseCmp(str, "all") == 0)
        val_rtrn->uval = 0xff;
    else if (uStrCaseCmp(str, "none") == 0)
        val_rtrn->uval = 0;
    else if (LookupModIndex(priv, elem, field, type, val_rtrn))
        val_rtrn->uval = (1U << val_rtrn->uval);
    else if (priv != NULL)
    {
        LookupPriv *lpriv = (LookupPriv *) priv;
        if ((lpriv->chain == NULL) ||
            (!(*lpriv->chain) (lpriv->chainPriv, elem, field, type,
                               val_rtrn)))
            return False;
    }
    else
        return False;
    return True;
}

#if 0
int
ExprResolveModIndex(const ExprDef *expr,
                    ExprResult * val_rtrn,
                    IdentLookupFunc lookup, XPointer lookupPriv)
{
    int ok = 0;
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeInt)
        {
            ERROR
                ("Found constant of type %s where a modifier mask was expected\n",
                 exprTypeText(expr->type));
            return False;
        }
        else if ((expr->value.ival >= XkbNumModifiers)
                 || (expr->value.ival < 0))
        {
            ERROR("Illegal modifier index (%d, must be 0..%d)\n",
                   expr->value.ival, XkbNumModifiers - 1);
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        return True;
    case ExprIdent:
        if (LookupModIndex(lookupPriv, (Atom) None, expr->value.str,
                           (unsigned) TypeInt, val_rtrn))
        {
            return True;
        }
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            None, expr->value.str, TypeInt, val_rtrn);
        }
        if (!ok)
            ERROR("Cannot determine modifier index for \"%s\"\n",
                   XkbAtomText(NULL, expr->value.str, XkbMessage));
        break;
    case ExprFieldRef:
        bogus = "field reference";
        break;
    case ExprArrayRef:
        bogus = "array reference";
        break;
    case ExprActionDecl:
        bogus = "function";
        break;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
    case OpInvert:
    case OpNegate:
    case OpNot:
    case OpUnaryPlus:
        bogus = "arithmetic operations";
        break;
    case OpAssign:
        bogus = "assignment";
        break;
    default:
        WSGO("Unknown operator %d in ResolveModIndex\n", expr->op);
        return False;
    }
    if (bogus)
    {
        ERROR("Modifier index must be a name or number, %s ignored\n",
               bogus);
        return False;
    }
    return ok;
}
#endif

int
ExprResolveModMask(const ExprDef *expr, ExprResult *val_rtrn,
                   IdentLookupFunc lookup, XPointer lookupPriv)
{
    LookupPriv priv = {
        .priv = NULL,
        .chain = lookup,
        .chainPriv = lookupPriv
    };

    return ExprResolveMask(expr, val_rtrn, LookupModMask, (XPointer) &priv);
}

int
ExprResolveBoolean(const ExprDef *expr, ExprResult *val_rtrn,
                   IdentLookupFunc lookup, XPointer lookupPriv)
{
    int ok = 0;
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeBoolean)
        {
            ERROR
                ("Found constant of type %s where boolean was expected\n",
                 exprTypeText(expr->type));
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        return True;
    case ExprIdent:
        bogus = XkbAtomGetString(NULL, expr->value.str);
        if (bogus)
        {
            if ((uStrCaseCmp(bogus, "true") == 0) ||
                (uStrCaseCmp(bogus, "yes") == 0) ||
                (uStrCaseCmp(bogus, "on") == 0))
            {
                val_rtrn->uval = 1;
                return True;
            }
            else if ((uStrCaseCmp(bogus, "false") == 0) ||
                     (uStrCaseCmp(bogus, "no") == 0) ||
                     (uStrCaseCmp(bogus, "off") == 0))
            {
                val_rtrn->uval = 0;
                return True;
            }
        }
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            None, expr->value.str, TypeBoolean, val_rtrn);
        }
        if (!ok)
            ERROR("Identifier \"%s\" of type int is unknown\n",
                   XkbAtomText(NULL, expr->value.str, XkbMessage));
        return ok;
    case ExprFieldRef:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            expr->value.field.element,
                            expr->value.field.field, TypeBoolean, val_rtrn);
        }
        if (!ok)
            ERROR("Default \"%s.%s\" of type boolean is unknown\n",
                   XkbAtomText(NULL, expr->value.field.element, XkbMessage),
                   XkbAtomText(NULL, expr->value.field.field, XkbMessage));
        return ok;
    case OpInvert:
    case OpNot:
        ok = ExprResolveBoolean(expr, val_rtrn, lookup, lookupPriv);
        if (ok)
            val_rtrn->uval = !val_rtrn->uval;
        return ok;
    case OpAdd:
        bogus = "Addition";
        goto boolean;
    case OpSubtract:
        bogus = "Subtraction";
        goto boolean;
    case OpMultiply:
        bogus = "Multiplication";
        goto boolean;
    case OpDivide:
        bogus = "Division";
        goto boolean;
    case OpAssign:
        bogus = "Assignment";
        goto boolean;
    case OpNegate:
        bogus = "Negation";
        goto boolean;
    boolean:
        ERROR("%s of boolean values not permitted\n", bogus);
        break;
    case OpUnaryPlus:
        ERROR("Unary \"+\" operator not permitted for boolean values\n");
        break;
    default:
        WSGO("Unknown operator %d in ResolveBoolean\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveFloat(const ExprDef *expr, ExprResult *val_rtrn,
                 IdentLookupFunc lookup, XPointer lookupPriv)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type == TypeString)
        {
            char *str;
            str = XkbAtomGetString(NULL, expr->value.str);
            if ((str != NULL) && (strlen(str) == 1))
            {
                val_rtrn->uval = str[0] * XkbGeomPtsPerMM;
                return True;
            }
        }
        if ((expr->type != TypeInt) && (expr->type != TypeFloat))
        {
            ERROR("Found constant of type %s, expected a number\n",
                   exprTypeText(expr->type));
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        if (expr->type == TypeInt)
            val_rtrn->ival *= XkbGeomPtsPerMM;
        return True;
    case ExprIdent:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            None, expr->value.str, TypeFloat, val_rtrn);
        }
        if (!ok)
            ERROR("Numeric identifier \"%s\" unknown\n",
                   XkbAtomText(NULL, expr->value.str, XkbMessage));
        return ok;
    case ExprFieldRef:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            expr->value.field.element,
                            expr->value.field.field, TypeFloat, val_rtrn);
        }
        if (!ok)
            ERROR("Numeric default \"%s.%s\" unknown\n",
                   XkbAtomText(NULL, expr->value.field.element, XkbMessage),
                   XkbAtomText(NULL, expr->value.field.field, XkbMessage));
        return ok;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveFloat(left, &leftRtrn, lookup, lookupPriv) &&
            ExprResolveFloat(right, &rightRtrn, lookup, lookupPriv))
        {
            switch (expr->op)
            {
            case OpAdd:
                val_rtrn->ival = leftRtrn.ival + rightRtrn.ival;
                break;
            case OpSubtract:
                val_rtrn->ival = leftRtrn.ival - rightRtrn.ival;
                break;
            case OpMultiply:
                val_rtrn->ival = leftRtrn.ival * rightRtrn.ival;
                break;
            case OpDivide:
                val_rtrn->ival = leftRtrn.ival / rightRtrn.ival;
                break;
            }
            return True;
        }
        return False;
    case OpAssign:
        WSGO("Assignment operator not implemented yet\n");
        break;
    case OpNot:
        left = expr->value.child;
        if (ExprResolveFloat(left, &leftRtrn, lookup, lookupPriv))
        {
            ERROR("The ! operator cannot be applied to a number\n");
        }
        return False;
    case OpInvert:
    case OpNegate:
        left = expr->value.child;
        if (ExprResolveFloat(left, &leftRtrn, lookup, lookupPriv))
        {
            if (expr->op == OpNegate)
                val_rtrn->ival = -leftRtrn.ival;
            else
                val_rtrn->ival = ~leftRtrn.ival;
            return True;
        }
        return False;
    case OpUnaryPlus:
        left = expr->value.child;
        return ExprResolveFloat(left, val_rtrn, lookup, lookupPriv);
    default:
        WSGO("Unknown operator %d in ResolveFloat\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveInteger(const ExprDef *expr, ExprResult *val_rtrn,
                   IdentLookupFunc lookup, XPointer lookupPriv)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type == TypeString)
        {
            char *str;
            str = XkbAtomGetString(NULL, expr->value.str);
            if (str != NULL)
                switch (strlen(str))
                {
                case 0:
                    val_rtrn->uval = 0;
                    return True;
                case 1:
                    val_rtrn->uval = str[0];
                    return True;
                default:
                    break;
                }
        }
        if ((expr->type != TypeInt) && (expr->type != TypeFloat))
        {
            ERROR
                ("Found constant of type %s where an int was expected\n",
                 exprTypeText(expr->type));
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        if (expr->type == TypeFloat)
            val_rtrn->ival /= XkbGeomPtsPerMM;
        return True;
    case ExprIdent:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            None, expr->value.str, TypeInt, val_rtrn);
        }
        if (!ok)
            ERROR("Identifier \"%s\" of type int is unknown\n",
                   XkbAtomText(NULL, expr->value.str, XkbMessage));
        return ok;
    case ExprFieldRef:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            expr->value.field.element,
                            expr->value.field.field, TypeInt, val_rtrn);
        }
        if (!ok)
            ERROR("Default \"%s.%s\" of type int is unknown\n",
                   XkbAtomText(NULL, expr->value.field.element, XkbMessage),
                   XkbAtomText(NULL, expr->value.field.field, XkbMessage));
        return ok;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveInteger(left, &leftRtrn, lookup, lookupPriv) &&
            ExprResolveInteger(right, &rightRtrn, lookup, lookupPriv))
        {
            switch (expr->op)
            {
            case OpAdd:
                val_rtrn->ival = leftRtrn.ival + rightRtrn.ival;
                break;
            case OpSubtract:
                val_rtrn->ival = leftRtrn.ival - rightRtrn.ival;
                break;
            case OpMultiply:
                val_rtrn->ival = leftRtrn.ival * rightRtrn.ival;
                break;
            case OpDivide:
                val_rtrn->ival = leftRtrn.ival / rightRtrn.ival;
                break;
            }
            return True;
        }
        return False;
    case OpAssign:
        WSGO("Assignment operator not implemented yet\n");
        break;
    case OpNot:
        left = expr->value.child;
        if (ExprResolveInteger(left, &leftRtrn, lookup, lookupPriv))
        {
            ERROR("The ! operator cannot be applied to an integer\n");
        }
        return False;
    case OpInvert:
    case OpNegate:
        left = expr->value.child;
        if (ExprResolveInteger(left, &leftRtrn, lookup, lookupPriv))
        {
            if (expr->op == OpNegate)
                val_rtrn->ival = -leftRtrn.ival;
            else
                val_rtrn->ival = ~leftRtrn.ival;
            return True;
        }
        return False;
    case OpUnaryPlus:
        left = expr->value.child;
        return ExprResolveInteger(left, val_rtrn, lookup, lookupPriv);
    default:
        WSGO("Unknown operator %d in ResolveInteger\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveString(const ExprDef *expr, ExprResult *val_rtrn,
                  IdentLookupFunc lookup, XPointer lookupPriv)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left;
    ExprDef *right;
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeString)
        {
            ERROR("Found constant of type %s, expected a string\n",
                   exprTypeText(expr->type));
            return False;
        }
        val_rtrn->str = XkbAtomGetString(NULL, expr->value.str);
        if (val_rtrn->str == NULL)
        {
            static char empty_char = '\0';
            val_rtrn->str = &empty_char;
        }
        return True;
    case ExprIdent:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            None, expr->value.str, TypeString, val_rtrn);
        }
        if (!ok)
            ERROR("Identifier \"%s\" of type string not found\n",
                   XkbAtomText(NULL, expr->value.str, XkbMessage));
        return ok;
    case ExprFieldRef:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            expr->value.field.element,
                            expr->value.field.field, TypeString, val_rtrn);
        }
        if (!ok)
            ERROR("Default \"%s.%s\" of type string not found\n",
                   XkbAtomText(NULL, expr->value.field.element, XkbMessage),
                   XkbAtomText(NULL, expr->value.field.field, XkbMessage));
        return ok;
    case OpAdd:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveString(left, &leftRtrn, lookup, lookupPriv) &&
            ExprResolveString(right, &rightRtrn, lookup, lookupPriv))
        {
            char *new;

#ifdef HAVE_ASPRINTF
            if (asprintf(&new, "%s%s", leftRtrn.str, rightRtrn.str) < 0)
                new = NULL;
#else
            size_t len = strlen(leftRtrn.str) + strlen(rightRtrn.str) + 1;
            new = malloc(len);
#endif
            if (new)
            {
#ifndef HAVE_ASPRINTF
                snprintf(new, len, "%s%s", leftRtrn.str, rightRtrn.str);
#endif
                val_rtrn->str = new;
                return True;
            }
        }
        return False;
    case OpSubtract:
        bogus = "Subtraction";
        goto string;
    case OpMultiply:
        bogus = "Multiplication";
        goto string;
    case OpDivide:
        bogus = "Division";
        goto string;
    case OpAssign:
        bogus = "Assignment";
        goto string;
    case OpNegate:
        bogus = "Negation";
        goto string;
    case OpInvert:
        bogus = "Bitwise complement";
        goto string;
    string:
        ERROR("%s of string values not permitted\n", bogus);
        return False;
    case OpNot:
        left = expr->value.child;
        if (ExprResolveString(left, &leftRtrn, lookup, lookupPriv))
        {
            ERROR("The ! operator cannot be applied to a string\n");
        }
        return False;
    case OpUnaryPlus:
        left = expr->value.child;
        if (ExprResolveString(left, &leftRtrn, lookup, lookupPriv))
        {
            ERROR("The + operator cannot be applied to a string\n");
        }
        return False;
    default:
        WSGO("Unknown operator %d in ResolveString\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveKeyName(const ExprDef *expr, ExprResult *val_rtrn,
                   IdentLookupFunc lookup, XPointer lookupPriv)
{
    int ok = 0;
    ExprDef *left;
    ExprResult leftRtrn;
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeKeyName)
        {
            ERROR("Found constant of type %s, expected a key name\n",
                   exprTypeText(expr->type));
            return False;
        }
        memcpy(val_rtrn->keyName.name, expr->value.keyName, XkbKeyNameLength);
        return True;
    case ExprIdent:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            None, expr->value.str, TypeString, val_rtrn);
        }
        if (!ok)
            ERROR("Identifier \"%s\" of type string not found\n",
                   XkbAtomText(NULL, expr->value.str, XkbMessage));
        return ok;
    case ExprFieldRef:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            expr->value.field.element,
                            expr->value.field.field, TypeString, val_rtrn);
        }
        if (!ok)
            ERROR("Default \"%s.%s\" of type key name not found\n",
                   XkbAtomText(NULL, expr->value.field.element, XkbMessage),
                   XkbAtomText(NULL, expr->value.field.field, XkbMessage));
        return ok;
    case OpAdd:
        bogus = "Addition";
        goto keyname;
    case OpSubtract:
        bogus = "Subtraction";
        goto keyname;
    case OpMultiply:
        bogus = "Multiplication";
        goto keyname;
    case OpDivide:
        bogus = "Division";
        goto keyname;
    case OpAssign:
        bogus = "Assignment";
        goto keyname;
    case OpNegate:
        bogus = "Negation";
        goto keyname;
    case OpInvert:
        bogus = "Bitwise complement";
        goto keyname;
    keyname:
        ERROR("%s of key name values not permitted\n", bogus);
        return False;
    case OpNot:
        left = expr->value.binary.left;
        if (ExprResolveString(left, &leftRtrn, lookup, lookupPriv))
        {
            ERROR("The ! operator cannot be applied to a key name\n");
        }
        return False;
    case OpUnaryPlus:
        left = expr->value.binary.left;
        if (ExprResolveString(left, &leftRtrn, lookup, lookupPriv))
        {
            ERROR("The + operator cannot be applied to a key name\n");
        }
        return False;
    default:
        WSGO("Unknown operator %d in ResolveKeyName\n", expr->op);
        break;
    }
    return False;
}

/***====================================================================***/

int
ExprResolveEnum(const ExprDef *expr, ExprResult *val_rtrn,
                const LookupEntry *values)
{
    if (expr->op != ExprIdent)
    {
        ERROR("Found a %s where an enumerated value was expected\n",
               exprOpText(expr->op));
        return False;
    }
    if (!SimpleLookup((XPointer) values, (Atom) None, expr->value.str,
                      (unsigned) TypeInt, val_rtrn))
    {
        int nOut = 0;
        ERROR("Illegal identifier %s (expected one of: ",
               XkbAtomText(NULL, expr->value.str, XkbMessage));
        while (values && values->name)
        {
            if (nOut != 0)
                INFO(", %s", values->name);
            else
                INFO("%s", values->name);
            values++;
            nOut++;
        }
        INFO(")\n");
        return False;
    }
    return True;
}

int
ExprResolveMask(const ExprDef *expr, ExprResult *val_rtrn,
                IdentLookupFunc lookup, XPointer lookupPriv)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    const ExprDef *left, *right;
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeInt)
        {
            ERROR
                ("Found constant of type %s where a mask was expected\n",
                 exprTypeText(expr->type));
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        return True;
    case ExprIdent:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            None, expr->value.str, TypeInt, val_rtrn);
        }
        if (!ok)
            ERROR("Identifier \"%s\" of type int is unknown\n",
                   XkbAtomText(NULL, expr->value.str, XkbMessage));
        return ok;
    case ExprFieldRef:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv,
                            expr->value.field.element,
                            expr->value.field.field, TypeInt, val_rtrn);
        }
        if (!ok)
            ERROR("Default \"%s.%s\" of type int is unknown\n",
                   XkbAtomText(NULL, expr->value.field.element, XkbMessage),
                   XkbAtomText(NULL, expr->value.field.field, XkbMessage));
        return ok;
    case ExprArrayRef:
        bogus = "array reference";
        goto unexpected_mask;
    case ExprActionDecl:
        bogus = "function use";
        goto unexpected_mask;
    unexpected_mask:
        ERROR("Unexpected %s in mask expression\n", bogus);
        ACTION("Expression ignored\n");
        return False;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveMask(left, &leftRtrn, lookup, lookupPriv) &&
            ExprResolveMask(right, &rightRtrn, lookup, lookupPriv))
        {
            switch (expr->op)
            {
            case OpAdd:
                val_rtrn->ival = leftRtrn.ival | rightRtrn.ival;
                break;
            case OpSubtract:
                val_rtrn->ival = leftRtrn.ival & (~rightRtrn.ival);
                break;
            case OpMultiply:
            case OpDivide:
                ERROR("Cannot %s masks\n",
                       expr->op == OpDivide ? "divide" : "multiply");
                ACTION("Illegal operation ignored\n");
                return False;
            }
            return True;
        }
        return False;
    case OpAssign:
        WSGO("Assignment operator not implemented yet\n");
        break;
    case OpInvert:
        left = expr->value.child;
        if (ExprResolveInteger(left, &leftRtrn, lookup, lookupPriv))
        {
            val_rtrn->ival = ~leftRtrn.ival;
            return True;
        }
        return False;
    case OpUnaryPlus:
    case OpNegate:
    case OpNot:
        left = expr->value.child;
        if (ExprResolveInteger(left, &leftRtrn, lookup, lookupPriv))
        {
            ERROR("The %s operator cannot be used with a mask\n",
                   (expr->op == OpNegate ? "-" : "!"));
        }
        return False;
    default:
        WSGO("Unknown operator %d in ResolveMask\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveKeySym(const ExprDef *expr, ExprResult *val_rtrn,
                  IdentLookupFunc lookup, XPointer lookupPriv)
{
    int ok = 0;

    if (expr->op == ExprIdent)
    {
        const char *str;
        KeySym sym;

        str = XkbAtomGetString(NULL, expr->value.str);
        if ((str != NULL) && ((sym = XStringToKeysym(str)) != NoSymbol))
        {
            val_rtrn->uval = sym;
            return True;
        }
    }
    ok = ExprResolveInteger(expr, val_rtrn, lookup, lookupPriv);
    if ((ok) && (val_rtrn->uval < 10))
        val_rtrn->uval += '0';
    return ok;
}
