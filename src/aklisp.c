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
#include "aklisp.h"

static int compare_numbers(int n1, int n2)
{
    if (n1 == n2)
        return 0;
    else if (n1 > n2)
        return 1;
    else
        return -1;
}

int akl_compare_values(struct akl_value *v1, struct akl_value *v2)
{
    assert(v1);
    assert(v2);
    if (v1->va_type == v2->va_type) {
        switch (v1->va_type) {
            case TYPE_NUMBER:
            return compare_numbers(AKL_GET_NUMBER_VALUE(v1)
                                   , AKL_GET_NUMBER_VALUE(v2));

            case TYPE_STRING:
            return strcmp(AKL_GET_STRING_VALUE(v1)
                          , AKL_GET_STRING_VALUE(v2));

            case TYPE_ATOM:
            return strcasecmp(akl_get_atom_name_value(v1)
                          , akl_get_atom_name_value(v2));

            case TYPE_USERDATA:
            /* TODO: userdata compare function */
            return compare_numbers(akl_get_utype_value(v1)
                                   , akl_get_utype_value(v2));

            case TYPE_NIL:
            return 0;

            case TYPE_TRUE:
            return 0;

            default:
            break;
        }
    }
    return -1;
}

struct akl_value *akl_eval_value(struct akl_instance *in, struct akl_value *val)
{
    struct akl_atom *aval;
    char *fname;
    if (val == NULL || AKL_IS_QUOTED(val)) {
        return val;
    }

    switch (val->va_type) {
        case TYPE_ATOM:
        aval = AKL_GET_ATOM_VALUE(val);
        if (aval->at_value != NULL)
           return aval->at_value;
        fname = aval->at_name;
        aval = akl_get_global_atom(in, fname);
        if (aval != NULL && aval->at_value != NULL) {
           return aval->at_value;
        } else {
           fprintf(stderr, "ERROR: No value for \'%s\' atom!\n"
                , fname);
           exit(-1);
        }
        break;

        case TYPE_LIST:
        return akl_eval_list(in, AKL_GET_LIST_VALUE(val));

        default:
        return val;
    }
}

struct akl_value *akl_eval_list(struct akl_instance *in, struct akl_list *list)
{
    akl_cfun_t cfun;
    struct akl_list *args;
    struct akl_atom *fatm = NULL, *aval;
    struct akl_list_entry *ent;
    struct akl_value *ret, *tmp, *a1;
    assert(list);

    if (AKL_IS_NIL(list) || list->li_elem_count == 0) 
        return &NIL_VALUE;

    if (AKL_IS_QUOTED(list)) {
        ret = akl_new_list_value(in, list);
        ret->is_quoted = TRUE;
        return ret;
    }
    
    a1 = AKL_FIRST_VALUE(list);
    if (AKL_CHECK_TYPE(a1, TYPE_ATOM)) {
        fatm = akl_get_global_atom(in, akl_get_atom_name_value(a1));
        if (fatm == NULL || fatm->at_value == NULL) {
            fprintf(stderr, "ERROR: Cannot find \'%s\' function!\n"
                , akl_get_atom_name_value(a1));
            exit(-1);
        }
        cfun = fatm->at_value->va_value.cfunc;
    } else {
        ret = akl_eval_list(in, AKL_GET_LIST_VALUE(a1));
        list->li_head->le_value = ret;
    }

    /* If the first atom is BUILTIN, i.e: it has full controll over
      it's arguments, the other elements of the list will not be evaluated...*/
    if (list->li_elem_count > 1 
            && fatm->at_value->va_type != TYPE_BUILTIN) {
        /* Not quoted, so start the list processing 
            from the second element. */
        AKL_LIST_FOREACH_SECOND(ent, list) {
            tmp = AKL_ENTRY_VALUE(ent);
            ent->le_value = akl_eval_value(in, AKL_ENTRY_VALUE(ent));
        }
    }

    if (fatm != NULL) {
        if (list->li_elem_count > 1)
            args = akl_cdr(in, list);
        else 
            args = &NIL_LIST;

        assert(args);
        assert(cfun);
        ret = cfun(in, args);
        if (fatm->at_value->va_type != TYPE_BUILTIN) {
            AKL_GC_DEC_REF(in, list);
            if (list->li_elem_count > 1)
                AKL_FREE(args);
        }
    }
    return ret;
}

void akl_eval_program(struct akl_instance *in)
{
    struct akl_list *list = in->ai_program;
    struct akl_list_entry *ent;
    struct akl_value *value;
    AKL_LIST_FOREACH(ent, list) {
        value = AKL_ENTRY_VALUE(ent);
        ent->le_value = akl_eval_value(in, value);
    }
}
