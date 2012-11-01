/************************************************************************
 *   Copyright (c) 2012 Ákos Kovács - AkLisp Lisp dialect
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "aklisp.h"

AKL_CFUN_DEFINE(plus, in, args)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    double sum = 0;
    char *str = NULL;
    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
       val = AKL_ENTRY_VALUE(ent);
       switch (val->va_type) {
            case TYPE_NUMBER:
            sum += AKL_GET_NUMBER_VALUE(val);
            break;

            case TYPE_STRING:
            if (str == NULL) {
                str = strdup(AKL_GET_STRING_VALUE(val));
            } else {
                str = (char *)realloc(str
                  , strlen(str)+strlen(AKL_GET_STRING_VALUE(val)));
                str = strcat(str, AKL_GET_STRING_VALUE(val));
            }
            break;

            case TYPE_LIST:
            /* TODO: Handle string lists */
            sum += AKL_GET_NUMBER_VALUE(plus_function(in
                 , AKL_GET_LIST_VALUE(val)));
            break;

            default:
            break;
       }
    }
    if (str == NULL)
        return akl_new_number_value(in, sum);
    else
        return akl_new_string_value(in, str);
}

AKL_CFUN_DEFINE(minus, in, args)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    double ret = 0;
    bool_t is_first = TRUE;
    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
       val = AKL_ENTRY_VALUE(ent);
        if (AKL_CHECK_TYPE(val, TYPE_NUMBER)) {
            if (is_first) {
                ret = AKL_GET_NUMBER_VALUE(val);
                is_first = FALSE;
            } else {
                ret -= AKL_GET_NUMBER_VALUE(val);
            }
        } /* TODO: else: throw error! */

    }
    return akl_new_number_value(in, ret);
}

AKL_CFUN_DEFINE(times, in, args)
{
    struct akl_list_entry *ent;
    struct akl_value *val, *a1, *a2;
    int n1, i;
    char *str, *s2;
    double ret = 1;

    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    /* If the expression is something like that: (* 3 " Hello! ")
      the output will be a string containing " Hello! Hello! Hello! "*/
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    if (a1->va_type == TYPE_NUMBER && a2->va_type == TYPE_STRING) {
        n1 = (int)AKL_GET_NUMBER_VALUE(a1);
        s2 = AKL_GET_STRING_VALUE(a2);
        str = (char *)akl_malloc(in, n1*strlen(s2)+1);
        for (i = 0; i < n1; i++) {
            strcat(str, s2);
        }
        return akl_new_string_value(in, str);
    }
    /* Just normal numbers and lists, get their product... */
    AKL_LIST_FOREACH(ent, args) {
        val = AKL_ENTRY_VALUE(ent);
        switch (val->va_type) {
            case TYPE_NUMBER:
            ret *= AKL_GET_NUMBER_VALUE(val);
            break;

            case TYPE_LIST:
            ret *= AKL_GET_NUMBER_VALUE(times_function(in
                                , AKL_GET_LIST_VALUE(val)));
            break;

            default:
            break;
        }
    }
    return akl_new_number_value(in, ret);
}

AKL_CFUN_DEFINE(div, in, args)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    double ret = 0.0;
    bool_t is_first = TRUE;
    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
       val = AKL_ENTRY_VALUE(ent);
        if (val->va_type == TYPE_NUMBER) {
            if (is_first) {
                ret = AKL_GET_NUMBER_VALUE(val);
                is_first = FALSE;
            } else {
                ret /= AKL_GET_NUMBER_VALUE(val);
            }
       }
    }
    return akl_new_number_value(in, ret);
}

AKL_CFUN_DEFINE(mod, in, args)
{
    struct akl_list_entry *ent;
    struct akl_value *val;
    int ret = 0;
    bool_t is_first = TRUE;
    if (AKL_IS_NIL(args))
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
       val = AKL_ENTRY_VALUE(ent);
        if (val->va_type == TYPE_NUMBER) {
            if (is_first) {
                ret = (int)AKL_GET_NUMBER_VALUE(val);
                is_first = FALSE;
            } else {
                ret %= (int)AKL_GET_NUMBER_VALUE(val);
            }
        }
    }
    return akl_new_number_value(in, ret);
}

AKL_CFUN_DEFINE(average, in, args)
{
    struct akl_value *value = plus_function(in, args);
    double num = AKL_GET_NUMBER_VALUE(value);
    AKL_GC_DEC_REF(in, value);
    num /= args->li_elem_count;
    return akl_new_number_value(in, num);
}

AKL_CFUN_DEFINE(not, in __unused, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 == NULL || AKL_IS_NIL(a1))
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

AKL_BUILTIN_DEFINE(and, in, args)
{
    struct akl_value *arg;
    struct akl_list_entry *ent;
    if (args->li_elem_count <= 0)
        return &TRUE_VALUE;

    AKL_LIST_FOREACH(ent, args) {
        arg = AKL_ENTRY_VALUE(ent);
        ent->le_value = akl_eval_value(in, arg);
        if (AKL_IS_NIL(ent->le_value)) 
            return &NIL_VALUE;
    }
    return AKL_ENTRY_VALUE(AKL_LIST_LAST(args));
}

AKL_BUILTIN_DEFINE(or, in, args)
{
    struct akl_value *arg;
    struct akl_list_entry *ent;
    if (args->li_elem_count <= 0)
        return &NIL_VALUE;

    AKL_LIST_FOREACH(ent, args) {
        arg = AKL_ENTRY_VALUE(ent);
        ent->le_value = akl_eval_value(in, arg);
        if (!AKL_IS_NIL(ent->le_value)) 
            return ent->le_value;
    }
    return &NIL_VALUE;
}

AKL_BUILTIN_DEFINE(while, in, args)
{
    struct akl_value *a1, *a2, *a3, *ret;
    struct akl_list_entry *ent;
    if (args && args->li_elem_count < 2)
        return &NIL_VALUE;

    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    a3 = AKL_THIRD_VALUE(args);
    ret = &NIL_VALUE;
    while (1) {
        if (AKL_IS_NIL(akl_eval_value(in, a1)))
            break;
        ret = akl_eval_value(in, a2);
        akl_eval_value(in, a3);
    }
    return ret;
}

AKL_CFUN_DEFINE(exit, in __unused, args)
{
    printf("Bye!\n");
    if (AKL_IS_NIL(args)) {
        exit(0);
    } else {
        exit(AKL_GET_NUMBER_VALUE(AKL_FIRST_VALUE(args)));
    }
}

AKL_CFUN_DEFINE(print, in, args)
{
    akl_print_list(args);
    return akl_new_list_value(in, args);
}

AKL_CFUN_DEFINE(display, in, args)
{
    struct akl_list_entry *ent;
    struct akl_value *tmp;
    AKL_LIST_FOREACH(ent, args) {
        tmp = AKL_ENTRY_VALUE(ent);
        switch (tmp->va_type) {
            case TYPE_NUMBER:
            printf("%g", AKL_GET_NUMBER_VALUE(tmp));
            break;

            case TYPE_STRING:
            printf("%s", AKL_GET_STRING_VALUE(tmp));
            break;

            default:
            break;
        }
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(last, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(a1, TYPE_LIST) && a1->va_value.list != NULL)
        return AKL_ENTRY_VALUE(AKL_LIST_LAST(AKL_GET_LIST_VALUE(a1)));
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(car, in __unused, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 && a1->va_type == TYPE_LIST && a1->va_value.list != NULL) {
        return akl_car(a1->va_value.list);
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(cdr, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    struct akl_value *ret;
    if (a1 && a1->va_type == TYPE_LIST && a1->va_value.list != NULL) {
        ret = akl_new_list_value(in, akl_cdr(in, a1->va_value.list));
        AKL_GC_INC_REF(ret);
        return ret;
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(to_num, in, args)
{
    struct akl_value *ret = akl_to_number(in, AKL_FIRST_VALUE(args));
    if (ret)
        return ret;
    return NULL;
}

AKL_CFUN_DEFINE(to_str, in, args)
{
    struct akl_value *ret = akl_to_string(in, AKL_FIRST_VALUE(args));
    if (ret)
        return ret;
    return NULL;
}

AKL_CFUN_DEFINE(to_sym, in, args)
{
    struct akl_value *ret = akl_to_symbol(in, AKL_FIRST_VALUE(args));
    if (ret)
        return ret;
    return NULL;
}

AKL_CFUN_DEFINE(len, in, args)
{
    struct akl_value *a1 = AKL_ENTRY_VALUE(AKL_LIST_FIRST(args));
    switch (a1->va_type) {
        case TYPE_STRING:
        return akl_new_number_value(in, strlen(AKL_GET_STRING_VALUE(a1)));
        break;

        case TYPE_LIST:
        return akl_new_number_value(in, AKL_GET_LIST_VALUE(a1)->li_elem_count);
        break;

        default:
        return &NIL_VALUE;
    }
    return &NIL_VALUE;
}

AKL_BUILTIN_DEFINE(setq, in, args)
{
    struct akl_atom *atom;
    struct akl_value *value, *desc, *av;
    av = AKL_FIRST_VALUE(args);
    atom = AKL_GET_ATOM_VALUE(av);
    if (atom == NULL) {
        akl_add_error(in, AKL_ERROR, av->va_lex_info, "ERROR: setq: First argument is not an atom!\n");
        return &NIL_VALUE;
    }
    value = akl_eval_value(in, AKL_SECOND_VALUE(args));
    if (args->li_elem_count > 1) {
        desc = AKL_THIRD_VALUE(args);
        if (AKL_CHECK_TYPE(desc, TYPE_STRING)) {
            atom->at_desc = AKL_GET_STRING_VALUE(desc);
        }
    }
    atom->at_value = value;
    akl_add_global_atom(in, atom);
    return value;
}

AKL_BUILTIN_DEFINE(incf, in, args)
{
    struct akl_value *a1, *v;
    struct akl_atom *at;
    a1 = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(a1, TYPE_ATOM)) {
        at = akl_get_global_atom(in, akl_get_atom_name_value(a1));
        v = at->at_value;
        if (AKL_CHECK_TYPE(v, TYPE_NUMBER)) {
            v->va_value.number += 1;
            return at->at_value;
        }
    } else if (AKL_CHECK_TYPE(a1, TYPE_NUMBER)) {
        a1->va_value.number += 1;
        return a1;
    }
    return &NIL_VALUE;
}

AKL_BUILTIN_DEFINE(decf, in, args)
{
    struct akl_value *a1, *v;
    struct akl_atom *at;
    a1 = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(a1, TYPE_ATOM)) {
        at = akl_get_global_atom(in, akl_get_atom_name_value(a1));
        v = at->at_value;
        if (AKL_CHECK_TYPE(v, TYPE_NUMBER)) {
            v->va_value.number -= 1;
            return at->at_value;
        }
    } else if (AKL_CHECK_TYPE(a1, TYPE_NUMBER)) {
        a1->va_value.number -= 1;
        return a1;
    }

    return &NIL_VALUE;
}

void print_help(struct akl_atom *a)
{
    if (a != NULL && a->at_value != NULL) {
        if (a->at_value->va_type == TYPE_CFUN 
                || a->at_value->va_type == TYPE_BUILTIN) {
            if (a->at_desc != NULL)
                printf("%s - %s\n", a->at_name, a->at_desc);
            else
                printf("%s - [no documentation]\n", a->at_name);
        }
    }
}

AKL_CFUN_DEFINE(help, in, args __unused)
{
   akl_do_on_all_atoms(in, print_help);
   return &NIL_VALUE;
}

AKL_BUILTIN_DEFINE(quote, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    if (a1 != NULL && a1->va_type == TYPE_LIST) {
        a1->is_quoted = TRUE;
        if (a1->va_value.list != NULL)
            a1->va_value.list->is_quoted = TRUE;
    }
    return a1;
}

AKL_CFUN_DEFINE(list, in, args)
{ return quote_builtin(in, args); }

AKL_CFUN_DEFINE(version, in, args __unused)
{
    struct akl_list *version = akl_new_list(in);
    char *ver = strdup(VER_ADDITIONAL);
    struct akl_value *addit;
    int i;
    for (i = 0; ver[i]; i++) {
        ver[i] = toupper(ver[i]);
    }
    akl_list_append(in, version, akl_new_number_value(in, VER_MAJOR));
    akl_list_append(in, version, akl_new_number_value(in, VER_MINOR));
    addit = akl_new_atom_value(in, ver);
    addit->is_quoted = TRUE;
    akl_list_append(in, version, addit);
    version->is_quoted = 1;
    return akl_new_list_value(in, version);
}

AKL_CFUN_DEFINE(about, in, args)
{
    const char *tnames[] = { "Atoms", "Lists"
                           , "Numbers", "Strings"
                           , "List entries", "Total allocated bytes"
                           , NULL };
    int i;
    struct akl_module *mod;
    printf("\nAkLisp version %d.%d-%s\n"
            "\tCopyleft (c) Akos Kovacs\n"
            "\tBuilt on %s %s\n"
#ifdef AKL_SYSTEM_INFO
            "\tBuild platform: %s (%s)\n"
            "\tBuild processor: %s\n"
#endif // AKL_SYSTEM_INFO
#ifdef AKL_USER_INFO
            "\tBuild by: %s@%s\n"
#endif // AKL_USER_INFO
            , VER_MAJOR, VER_MINOR, VER_ADDITIONAL
            , __DATE__, __TIME__
#ifdef AKL_SYSTEM_INFO
            , AKL_SYSTEM_NAME, AKL_SYSTEM_VERSION, AKL_PROCESSOR
#endif // AKL_SYSTEM_INFO
#ifdef AKL_USER_INFO
            , AKL_USER_NAME, AKL_HOST_NAME
#endif // AKL_USER_INFO
            );
    if (in->ai_module_count != 0 && in->ai_modules != NULL) {
        printf("\n%d loaded module%s:", in->ai_module_count
            , (in->ai_module_count > 1)?"s":"");
        for (i = 0; i < in->ai_module_size; i++) {
            mod = in->ai_modules[i];
            if (mod != NULL) {
                printf("\n");
                if (mod->am_name && mod->am_path)
                    printf("\tName: '%s'\n\tPath: '%s'\n", mod->am_name, mod->am_path);
                if (mod->am_desc)
                    printf("\tDescription: '%s'\n", mod->am_desc);
                if (mod->am_author)
                    printf("\tAuthor: '%s'\n", mod->am_author);
            }
        }
    }
    printf("\nGC statistics:\n");
    for (i = 0; i < AKL_NR_GC_STAT_ENT; i++) {
        printf("\t%s: %d\n", tnames[i], in->ai_gc_stat[i]);
    }
    printf("\n");
    
    return version_function(in, args);
}

AKL_CFUN_DEFINE(range, in, args)
{
    struct akl_list *range;
    struct akl_value *farg, *sarg, *targ;
    int rf, rs, rt, i;
    assert(args);
    if (args->li_elem_count > 1) {
        farg = AKL_FIRST_VALUE(args);
        sarg = AKL_SECOND_VALUE(args);
    }
    if (args->li_elem_count > 2) {
        targ = AKL_LIST_SECOND(args)->le_next->le_value;
        if (targ && targ->va_type == TYPE_NUMBER)
            rt = AKL_GET_NUMBER_VALUE(targ);
        else 
            rt = 1;
    } else {
        rt = 1;
    }

    if (farg && sarg && farg->va_type == TYPE_NUMBER
            && sarg->va_type == TYPE_NUMBER) {
        range = akl_new_list(in);
        rf = AKL_GET_NUMBER_VALUE(farg);
        rs = AKL_GET_NUMBER_VALUE(sarg);
        if (rf < rs) {
            for (i = rf; i <= rs; i += rt) {
                akl_list_append(in, range, akl_new_number_value(in, i));
            }
        } else if (rf > rs) {
            for (i = rf; i >= rs; i -= rt) {
                akl_list_append(in, range, akl_new_number_value(in, i));
            }
        } else {
            akl_list_append(in, range, akl_new_number_value(in, rf));
        }
    } else {
        return &NIL_VALUE;
    }
    range->is_quoted = 1;
    return akl_new_list_value(in, range);
}

AKL_CFUN_DEFINE(index, in, args)
{
    int ind;
    struct akl_list *list;
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    if (a1 && !AKL_IS_NIL(a1) && a1->va_type == TYPE_NUMBER) {
        ind = AKL_GET_NUMBER_VALUE(a1);
    }
    if (a2 && !AKL_IS_NIL(a2) && a2->va_type == TYPE_LIST) {
        list = AKL_GET_LIST_VALUE(a2);
        return akl_list_index(list, ind);
    }
    return &NIL_VALUE;
}

static struct akl_value *cmp_two_args(struct akl_instance *in __unused,
                                struct akl_list *args, int cmp_op)
{
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);
    if (akl_compare_values(a1, a2) == cmp_op)
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

AKL_CFUN_DEFINE(greaterp, in, args)
{
    return cmp_two_args(in, args, 1);
}

AKL_CFUN_DEFINE(equal, in, args)
{
    return cmp_two_args(in, args, 0);
}

AKL_CFUN_DEFINE(lessp, in, args)
{
    return cmp_two_args(in, args, -1);
}

/* Less or equal */
AKL_CFUN_DEFINE(less_eqp, in, args)
{
    if (cmp_two_args(in, args, -1) == &NIL_VALUE) {
        if (cmp_two_args(in, args, 0) == &TRUE_VALUE)
            return &TRUE_VALUE;
    } else {
        return &TRUE_VALUE;
    }
    return &NIL_VALUE;
}

/* Greater or equal */
AKL_CFUN_DEFINE(greater_eqp, in, args)
{
    if (cmp_two_args(in, args, 1) == &NIL_VALUE) {
        if (cmp_two_args(in, args, 0) == &TRUE_VALUE)
            return &TRUE_VALUE;
    } else {
        return &TRUE_VALUE;
    }
    return &NIL_VALUE;
}

/* Evaulate the first expression (mostly a list with a logical function),
    if that is not NIL, return with the evaulated second argument. */
static struct akl_value *eval_if_true(struct akl_instance *in
                                            , struct akl_list *l)
{
    struct akl_value *a1;
    a1 = akl_eval_value(in, AKL_FIRST_VALUE(l));
    if (a1 != NULL && !AKL_IS_NIL(a1))
        return akl_eval_value(in, AKL_SECOND_VALUE(l));

    return NULL;
}

AKL_BUILTIN_DEFINE(if, in, args)
{
    struct akl_value *ret;
    if ((ret = eval_if_true(in, args)) == NULL)
        return akl_eval_value(in, AKL_THIRD_VALUE(args));

    return ret;
}

AKL_BUILTIN_DEFINE(cond, in, args)
{
    struct akl_value *arg, *ret;
    struct akl_list_entry *ent;
    AKL_LIST_FOREACH(ent, args) {
        arg = AKL_ENTRY_VALUE(ent);
        if (arg != NULL && arg->va_type == TYPE_LIST) {
            ret = eval_if_true(in, AKL_GET_LIST_VALUE(arg));
            if (ret != NULL)
                return ret;
        }
    }
    return &NIL_VALUE;
}

AKL_BUILTIN_DEFINE(case, in, args)
{
    struct akl_value *a1, *arg, *cv;
    struct akl_list_entry *ent;
    struct akl_list *cl;
    if (args->li_elem_count < 2)
        return &NIL_VALUE;

    /* Evaulate the first argument (the base of the comparision) */
    a1 = akl_eval_value(in, AKL_FIRST_VALUE(args));
    AKL_LIST_FOREACH_SECOND(ent, args) {
        arg = AKL_ENTRY_VALUE(ent);
        if (arg != NULL && arg->va_type == TYPE_LIST) {
            cl = AKL_GET_LIST_VALUE(arg);
            cv = akl_eval_value(in, AKL_FIRST_VALUE(cl));
            if (AKL_CHECK_TYPE(cv, TYPE_TRUE) || (akl_compare_values(a1, cv) == 0))
                return akl_eval_value(in, AKL_SECOND_VALUE(cl));
        }
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(nilp, in, args)
{
    struct akl_value *a1;
    a1 = AKL_FIRST_VALUE(args);
    if (a1 == NULL || AKL_IS_NIL(a1))
        return &TRUE_VALUE;
    else
        return &NIL_VALUE;
}

AKL_CFUN_DEFINE(zerop, in, args)
{
    int n;
    struct akl_value *val = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(val, TYPE_NUMBER)) {
        n = AKL_GET_NUMBER_VALUE(val);
        if (n == 0) 
            return &TRUE_VALUE;
        else
            return &NIL_VALUE;
    }
    /* TODO: Give error message */
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(evenp, in, args)
{
    int n;
    struct akl_value *val = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(val, TYPE_NUMBER)) {
        n = (int)AKL_GET_NUMBER_VALUE(val);
        if ((n % 2) == 0) 
            return &TRUE_VALUE;
        else
            return &NIL_VALUE;
    }
    /* TODO: Give error message */
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(oddp, in, args)
{
    int n;
    struct akl_value *val = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(val, TYPE_NUMBER)) {
        n = (int)AKL_GET_NUMBER_VALUE(val);
        if ((n % 2) != 0) 
            return &TRUE_VALUE;
        else
            return &NIL_VALUE;
    }
    /* TODO: Give error message */
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(posp, in, args)
{
    int n;
    struct akl_value *val = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(val, TYPE_NUMBER)) {
        n = AKL_GET_NUMBER_VALUE(val);
        if (n > 0)
            return &TRUE_VALUE;
        else
            return &NIL_VALUE;
    }
    /* TODO: Give error message */
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(negp, in, args)
{
    int n;
    struct akl_value *val = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(val, TYPE_NUMBER)) {
        n = AKL_GET_NUMBER_VALUE(val);
        if (n < 0)
            return &TRUE_VALUE;
        else
            return &NIL_VALUE;
    }
    /* TODO: Give error message */
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(typeof, in, args)
{
    struct akl_value *a1, *ret;
    const char *tname = "NIL";
    a1 = AKL_FIRST_VALUE(args);
    if (a1 == NULL)
        return &NIL_VALUE;

    switch (a1->va_type) {
        case TYPE_NUMBER:
        tname = "NUMBER";
        break;

        case TYPE_ATOM:
        tname = "ATOM";
        break;

        case TYPE_LIST:
        tname = "LIST";
        break;

        case TYPE_CFUN:
        tname = "CFUNCTION";
        break;

        case TYPE_STRING:
        tname = "STRING";
        break;

        case TYPE_BUILTIN:
        tname = "BUILTIN";
        break;

        case TYPE_USERDATA:
        tname = in->ai_utypes[akl_get_utype_value(a1)]->ut_name;
        break;

        case TYPE_TRUE:
        tname = "T";
        break;

        case TYPE_NIL:
        tname = "NIL";
        break;
    }
    /* Must duplicate the name for gc */
    ret = akl_new_atom_value(in, strdup(tname));
    ret->is_quoted = TRUE;
    return ret;
}

AKL_BUILTIN_DEFINE(desc, in, args)
{
    struct akl_value *arg;
    struct akl_atom *atom;
    arg = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(arg, TYPE_ATOM)) {
        atom = akl_get_global_atom(in, akl_get_atom_name_value(arg));
        if (atom == NULL || atom->at_desc == NULL)
            return &NIL_VALUE;

        return akl_new_string_value(in, strdup(atom->at_desc));
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(cons, in, args)
{
    struct akl_value *a1, *a2;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args);

    if (a1 == NULL || a2 == NULL || a2->va_type != TYPE_LIST)
        return &NIL_VALUE;
   
    akl_list_insert_head(in, AKL_GET_LIST_VALUE(a2), a1);
    return a2;
}

AKL_CFUN_DEFINE(read_number, in, args)
{
    double num = 0;
    scanf("%lf", &num);
    return akl_new_number_value(in, num);
}
#ifdef _GNUC_
AKL_CFUN_DEFINE(read_string, in, args __unused)
{
    char *str = NULL;
    scanf("%a%s", &str);
    if (str != NULL)
        return akl_new_string_value(in, str);
}
#else // _GNUC_
AKL_CFUN_DEFINE(read_string, in, args __unused)
{
    char *str = (char *)akl_malloc(in, 256);
    scanf("%s", str);
    if (str != NULL)
        return akl_new_string_value(in, str);
    return &NIL_VALUE;
}
#endif // _GNUC_

AKL_CFUN_DEFINE(newline, in __unused, args __unused)
{
    printf("\n");
    return &NIL_VALUE;
}

static struct tm *akl_time(void)
{
    time_t curr;
    time(&curr);
    return localtime(&curr);
}

AKL_CFUN_DEFINE(date_year, in, args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_year + 1900);
}

AKL_CFUN_DEFINE(time_hour, in, args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_hour);
}

AKL_CFUN_DEFINE(time_min, in, args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_min);
}

AKL_CFUN_DEFINE(time_sec, in, args __unused)
{
    return akl_new_number_value(in, akl_time()->tm_sec);
}

AKL_CFUN_DEFINE(time, in, args)
{
    struct akl_list *tlist;
    tlist = akl_new_list(in);
    akl_list_append(in, tlist, time_hour_function(in, args));
    akl_list_append(in, tlist, time_min_function(in, args));
    akl_list_append(in, tlist, time_sec_function(in, args));
    return akl_new_list_value(in, tlist);
}

AKL_CFUN_DEFINE(progn, in, args)
{
    struct akl_list_entry *lent = AKL_LIST_LAST(args);
    if (lent != NULL && lent->le_value != NULL)
        return AKL_ENTRY_VALUE(lent);
    else
        return &NIL_VALUE;
}

void akl_init_lib(struct akl_instance *in, enum AKL_INIT_FLAGS flags)
{
    if (flags & AKL_LIB_BASIC) {
        AKL_ADD_BUILTIN(in, quote, "QUOTE", "Quote listame like as \'");
        AKL_ADD_BUILTIN(in, setq,  "SET!", "Bound (set) a variable to a value");
        AKL_ADD_CFUN(in, progn,"PROGN", "Return with the last value");
        AKL_ADD_CFUN(in, list, "LIST", "Create list");
        AKL_ADD_CFUN(in, car,  "CAR", "Get the head of a list");
        AKL_ADD_CFUN(in, cdr,  "CDR", "Get the tail of a list");
        AKL_ADD_CFUN(in, car,  "FIRST", "Get the first element of the list");
        AKL_ADD_CFUN(in, last,  "LAST", "Get back the last element of a list");
        AKL_ADD_CFUN(in, cdr,  "REST", "Get the tail of a list");
        AKL_ADD_CFUN(in, cdr,  "TAIL", "Get the tail of a list");
        AKL_ADD_CFUN(in, to_num,  "NUM", "Convert an arbitrary value to a number");
        AKL_ADD_CFUN(in, to_str,  "STR", "Convert an arbitrary value to a string");
        AKL_ADD_CFUN(in, to_sym,  "SYM", "Convert an arbitrary value to a symbol");
    }

    if (flags & AKL_LIB_DATA) {
        AKL_ADD_CFUN(in, range, "RANGE", "Create list with elements from arg 1 to arg 2");
        AKL_ADD_CFUN(in, cons,  "CONS", "Insert the first argument to the second list argument");
        AKL_ADD_CFUN(in, len,   "LENGTH", "The length of a given value");
        AKL_ADD_CFUN(in, index, "INDEX", "Index of list");
        AKL_ADD_CFUN(in, typeof,"TYPEOF", "Get the type of the value");
        AKL_ADD_BUILTIN(in, desc ,"DESC", "Get the description (AKA documentation) of the variable");
    }

    if (flags & AKL_LIB_CONDITIONAL) {
        AKL_ADD_BUILTIN(in, if, "IF"
        , "If the first argument true, returns with the second, otherwise returns with the third");
        AKL_ADD_BUILTIN(in, cond, "COND"
        , "Similiar to if, but works with arbitrary number of conditions (called clauses)");
        AKL_ADD_BUILTIN(in, while, "WHILE", "Conditionally repeat an S-expression");
        AKL_ADD_BUILTIN(in, case, "CASE", "Conditional select");
    }

    if (flags & AKL_LIB_PREDICATE) {
        AKL_ADD_CFUN(in, equal, "=", "Tests it\'s argument for equality");
        AKL_ADD_CFUN(in, lessp, "<",  "If it\'s first argument is less than the second returns with T");
        AKL_ADD_CFUN(in, less_eqp, "<=",  "If it\'s first argument is less or equal to the second returns with T");
        AKL_ADD_CFUN(in, greaterp, ">", "If it\'s first argument is greater than the second returns with T");
        AKL_ADD_CFUN(in, greater_eqp, ">=", "If it\'s first argument is greater or equal to the second returns with T");
        AKL_ADD_CFUN(in, equal, "EQ?", "Tests it\'s argument for equality");
        AKL_ADD_CFUN(in, equal, "EQUAL", "Tests it\'s argument for equality");
        AKL_ADD_CFUN(in, equal, "EQUAL?", "Tests it\'s argument for equality");
        AKL_ADD_CFUN(in, lessp, "LESS?",  "If it\'s first argument is less than the second returns with T");
        AKL_ADD_CFUN(in, greaterp, "GREATER?", "If it\'s first argument is greater than the second returns with T");
        AKL_ADD_CFUN(in, zerop, "ZERO?", "Predicate which, returns with true if it\'s argument is zero");
        AKL_ADD_CFUN(in, nilp, "NIL?", "Predicate which, returns with true when it\'s argument is NIL");
        AKL_ADD_CFUN(in, oddp, "ODD?", "Predicate which, returns with true when it\'s argument is odd");
        AKL_ADD_CFUN(in, oddp, "EVEN?", "Predicate which, returns with true when it\'s argument is even");
        AKL_ADD_CFUN(in, negp, "NEGATIVE?", "Predicate which, returns with true when it\'s argument is positive");
        AKL_ADD_CFUN(in, posp, "POSITIVE?", "Predicate which, returns with true when it\'s argument is negative");
    }

    if (flags & AKL_LIB_NUMBERIC) {
        AKL_ADD_BUILTIN(in, incf, "++", "Increase a number value by 1");
        AKL_ADD_BUILTIN(in, decf, "--", "Decrease a number value by 1");
        AKL_ADD_CFUN(in, plus,  "+", "Arithmetic addition and string concatenation");
        AKL_ADD_CFUN(in, minus, "-", "Artihmetic subtraction");
        AKL_ADD_CFUN(in, times, "*", "Arithmetic multiplication");
        AKL_ADD_CFUN(in, div,   "/",  "Arithmetic division");
        AKL_ADD_CFUN(in, mod,   "%", "Arithmetic modulus");
        /* Ok. Again... */
        AKL_ADD_CFUN(in, plus,  "PLUS", "Arithmetic addition and string concatenation");
        AKL_ADD_BUILTIN(in, incf, "INC!", "Increase a number value by 1");
        AKL_ADD_BUILTIN(in, decf, "DEC!", "Decrease a number value by 1");
        AKL_ADD_CFUN(in, minus, "MINUS", "Artihmetic substraction");
        AKL_ADD_CFUN(in, times, "TIMES", "Arithmetic multiplication");
        AKL_ADD_CFUN(in, div,   "DIV",  "Arithmetic division");
        AKL_ADD_CFUN(in, mod,   "MOD", "Arithmetic modulus");
        AKL_ADD_CFUN(in, average, "AVERAGE", "Just average");
    }

    if (flags & AKL_LIB_LOGICAL) {
        AKL_ADD_CFUN(in, not, "NOT", "Reverse the argument\'s value");
        AKL_ADD_BUILTIN(in, and, "AND", "Evaluate arguments from right to left, returning NIL if"
            " any of them is NIL, otherwise returning with the last value");
        AKL_ADD_BUILTIN(in, or, "OR", "Evaluate arguments from right to left, returning the first"
            " non-NIL value, otherwise just NIL");
    }
        
    if (flags & AKL_LIB_SYSTEM) {
        AKL_ADD_CFUN(in, read_number, "READ-NUMBER", "Read a number from the standard input");
        AKL_ADD_CFUN(in, read_string, "READ-WORD", "Read a word from the standard input");
        AKL_ADD_CFUN(in, newline, "NEWLINE", "Just put out a newline character to the standard output");
        AKL_ADD_CFUN(in, print,   "PRINT", "Print different values to the screen in Lisp-style");
        AKL_ADD_CFUN(in, display, "DISPLAY", "Print numbers or strings to the screen");
        AKL_ADD_CFUN(in, help,    "HELP", "Print the builtin functions");
        AKL_ADD_CFUN(in, about,   "ABOUT", "About the environment");
        AKL_ADD_CFUN(in, version, "VERSION", "About the version");
        AKL_ADD_CFUN(in, exit, "QUIT", "Exit from the program");
        AKL_ADD_CFUN(in, exit, "EXIT", "Exit from the program");
    }

    if (flags & AKL_LIB_TIME) {
        AKL_ADD_CFUN(in, date_year, "DATE-YEAR", "Get the current year");
        AKL_ADD_CFUN(in, time_hour, "TIME-HOUR", "Get the current hour");
        AKL_ADD_CFUN(in, time_min,  "TIME-MIN", "Get the current minute");
        AKL_ADD_CFUN(in, time_sec,  "TIME-SEC", "Get the current secundum");
        AKL_ADD_CFUN(in, time, "TIME", "Get the current time as a list of (hour minute secundum)");
    }

    if (flags & AKL_LIB_OS) {
        akl_init_os(in);
    }
}
