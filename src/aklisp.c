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

static struct akl_atom *recent_value;

static void update_recent_value(struct akl_state *in, struct akl_value *val)
{
    if (!recent_value) {
        recent_value = akl_new_atom(in, "$?");
        recent_value->at_desc = "Previously returned value";
        recent_value->at_is_const = TRUE;
        akl_add_global_atom(in, recent_value);
        AKL_GC_SET_STATIC(recent_value);
    }
    /* Update $? */
    recent_value->at_value = val;
}

void akl_ir_exec_store(struct akl_state *s, struct akl_ir_instruction *ir)
{
    struct akl_value *v;
    switch (ir->in_arg_type) {
        case TYPE_STRING:
        v = akl_new_string_value(s, ir->in_arg.arg[0]);
        break;

        case TYPE_NUMBER:
        v = akl_new_number_value(s, ir->in_arg.number);
        break;

        case TYPE_LIST:
        v = akl_new_list_value(s, ir->in_arg.arg[0]);
        break;

        case TYPE_NIL:
        v = &NIL_VALUE;
        break;

        case TYPE_TRUE:
        v = &TRUE_VALUE;
        break;
    }
    akl_stack_push(s, v);
}

void akl_execute_ir(struct akl_state *s, struct akl_vector *ir)
{
    struct akl_ir_instruction *in;
    struct akl_function *func;
    struct akl_vector *br;
    struct akl_value *v;
    AKL_VECTOR_FOREACH(in, ir) {
        switch (in->in_op) {
            case AKL_IR_NOP:
            continue;

            case AKL_IR_LOAD:

            break;

            case AKL_IR_STORE:
            akl_ir_exec_store(s, ir, in);
            break;

            case AKL_IR_CALL:
            break;

            case AKL_IR_BRANCH:
            v = akl_stack_pop(s);
            /* If the last value was NIL, execute the false
              branch. */
            brind = (AKL_IS_NIL(v)) ? 1 : 0;
            br = (struct akl_vector *)in->in_op.op[brind];
            akl_execute_ir(s, br);
            break;
        }
    }
}

void akl_execute(struct akl_state *s)
{
    akl_execute_ir(s, &s->ai_ir_code);
}

static int compare_numbers(int n1, int n2)
{
    if (n1 == n2)
        return 0;
    else if (n1 > n2)
        return 1;
    else
        return -1;
}

int akl_compare_values(void *c1, void *c2)
{
    assert(c1);
    assert(c2);
    struct akl_value *v1 = (struct akl_value *)c1;
    struct akl_value *v2 = (struct akl_value *)c2;
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

struct akl_value *akl_eval_value(struct akl_state *s, struct akl_value *val, struct akl_context *ctx)
{
    struct akl_atom *aval;
    char *fname;
    if (val == NULL || AKL_IS_QUOTED(val)) {
        update_recent_value(in, val);
        return val;
    }

    switch (val->va_type) {
        case TYPE_ATOM:
        break;

        case TYPE_LIST:
        return akl_eval_list(in, AKL_GET_LIST_VALUE(val));

        default:
        update_recent_value(in, val);
        return val;
    }
    return &NIL_VALUE;
}

struct akl_value *akl_eval_list(struct akl_state *s, struct akl_list *list)
{
    struct akl_value *fval;
    struct akl_value *val;
    struct akl_function *fn

    fval = akl_eval_value(s, AKL_FIRST_VALUE(list));
    if (AKL_IS_FUNCTION(fn)) {
        fval = AKL_GET_FUNCTION_VALUE(fval);
            switch (fval->va_type) {
            case AKL_FUNC_SEPECIAL:
            if (fn->fn_body.scfun)
                val = fn->fn_body.scfun(s, list);
            break;

            case AKL_FUNC_CFUN:
            break;
        }
    }
}

struct akl_value *akl_eval_list(struct akl_state *in, struct akl_list *list)
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
        update_recent_value(in, ret);
        return ret;
    }

    a1 = AKL_FIRST_VALUE(list);
    if (AKL_CHECK_TYPE(a1, TYPE_ATOM)) {
        if (fatm == NULL || fatm->at_value == NULL) {
            akl_add_error(in, AKL_ERROR, a1->va_lex_info, "ERROR: Cannot find \'%s\' function!\n"
                          , akl_get_atom_name_value(a1));
            return &NIL_VALUE;
        }
        switch (fatm->at_value->va_type) {
            case TYPE_LIST:
            return akl_eval_list(in, AKL_GET_LIST_VALUE(fatm->at_value));

            case TYPE_BUILTIN: case TYPE_CFUN:
                cfun = fatm->at_value->va_value.cfunc;
            break;

            default:
                akl_add_error(in, AKL_ERROR, a1->va_lex_info
                , "ERROR: eval: The first element must be a function!\n");
            return &NIL_VALUE;
        }
    } else {
        akl_add_error(in, AKL_ERROR, a1->va_lex_info, "ERROR: eval: The first element must be a function!\n");
        return &NIL_VALUE;
    }

    /* If the first atom is BUILTIN, i.e: it has full controll over
      it's arguments, the other elements of the list will not be evaluated...*/
    if (list->li_elem_count > 1
            && fatm->at_value->va_type != TYPE_BUILTIN) {
        /* Not quoted, so start the list processing
            from the second element. */
        AKL_LIST_FOREACH_SECOND(ent, list) {
            tmp = AKL_ENTRY_VALUE(ent);
            akl_stack_push(akl_eval_value(in, AKL_ENTRY_VALUE(ent)));
        }
    }

    if (fatm != NULL) {
        if (list->li_elem_count > 1)
            args = akl_cdr(in, list);
        else
            args = NULL;

        assert(cfun);
        ret = cfun(in);
    }
    update_recent_value(in, ret);
    return ret;
}

void akl_eval_program(struct akl_state *in)
{
    struct akl_list *list = in->ai_program;
    struct akl_list_entry *ent;
    struct akl_value *value;
    AKL_LIST_FOREACH(ent, list) {
        value = AKL_ENTRY_VALUE(ent);
        akl_eval_value(in, value);
    }
}

void akl_add_error(struct akl_state *in, enum AKL_ALERT_TYPE type
                   , struct akl_lex_info *info, const char *fmt, ...)
{
    va_list ap;
    struct akl_list *l;
    struct akl_error *err;
    size_t fmt_size = strlen(fmt);
    /* should be enough */
    size_t new_size = fmt_size + (fmt_size/2);
    int n;
    char *np;
    char *msg = (char *)akl_malloc(in, new_size);
    while (1) {
        va_start(ap, fmt);
        n = vsnprintf(msg, new_size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < new_size)
            break;
        /* Else try again with more space. */
        if (n > -1)    /* glibc 2.1 */
            new_size = n+1;
        else           /* glibc 2.0 */
            new_size *= 2;
        if ((np = (char *)realloc (msg, new_size)) == NULL) {
            free(msg);
            return;
        } else {
            msg = np;
        }
    }

    if (in) {
        if (in->ai_errors == NULL) {
            in->ai_errors = akl_new_list(in);
        }
        err = AKL_MALLOC(in, struct akl_error);
        err->err_info = info;
        err->err_type = type;
        err->err_msg = msg;
        akl_list_append(in, in->ai_errors, (void *)err);
    }
}

void akl_clear_errors(struct akl_state *in)
{
    struct akl_list_entry *ent, *tmp;
    struct akl_error *err;
    if (in && in->ai_errors) {
        AKL_LIST_FOREACH_SAFE(ent, in->ai_errors, tmp) {
           err = (struct akl_error *)ent->le_value;
           AKL_FREE((void *)err->err_msg);
           AKL_FREE((void *)err);
        }
        in->ai_errors->li_elem_count = 0;
        in->ai_errors->li_head = NULL;
        in->ai_errors->li_last = NULL;
    }
}

void akl_print_errors(struct akl_state *in)
{
    struct akl_error *err;
    struct akl_list_entry *ent;
    const char *name = "(unknown)";
    int errors = 0;
    int line, count;
    line = count = 0;

    if (in && in->ai_errors) {
        AKL_LIST_FOREACH(ent, in->ai_errors) {
            err = (struct akl_error *)ent->le_value;
            if (err) {
                if (err->err_info) {
                    count = err->err_info->li_count;
                    line = err->err_info->li_line;
                    name = err->err_info->li_name;
                }

                fprintf(stderr,  GREEN "%s:%d:%d" END_COLOR_MARK ": %s%s" END_COLOR_MARK
                        , name, line, count, (err->err_type == AKL_ERROR) ? RED : YELLOW, err->err_msg);
            }
            errors++;
        }
        if (!in->ai_interactive)
            fprintf(stderr, "%d error report generated.\n", errors);
    }
}
