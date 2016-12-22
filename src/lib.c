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
#include <libgen.h>

#include "aklisp.h"

#define PATH_MAX 255

AKL_DEFINE_FUN(exit, cx, argc)
{
    exit(0);
}

AKL_DEFINE_FUN(inc, cx, argc)
{
    double n = 0;
    if (akl_get_args_strict(cx, 1, AKL_VT_NUMBER, &n) == -1) {
        return AKL_NIL;
    }

    return AKL_NUMBER(cx, n+1);
}

AKL_DEFINE_FUN(dec, cx, argc)
{
    double n = 0;
    if (akl_get_args_strict(cx, 1, AKL_VT_NUMBER, &n) == -1) {
        return AKL_NIL;
    }

    return AKL_NUMBER(cx, n-1);
}

AKL_DEFINE_FUN(iszero, cx, argc)
{
    double n = 0;
    if (akl_get_args_strict(cx, 1, AKL_VT_NUMBER, &n) == -1) {
        return AKL_NIL;
    }

    /* TODO: Make errors better for true/nil */
    return !n ? AKL_TRUE : AKL_NIL;
}

AKL_DEFINE_FUN(isnil, cx, argc)
{
    struct akl_value *v = akl_frame_pop(cx);
    if (AKL_IS_NIL(v)) {
        return  AKL_TRUE;
    }
    return AKL_NIL;
}

AKL_DEFINE_FUN(isstring, cx, argc)
{
    char *s = akl_frame_pop_string(cx);
    if (s == NULL) {
        return AKL_NIL;
    }
    return AKL_TRUE;
}

AKL_DEFINE_FUN(islist, cx, argc)
{
    /* Nil is a list too */
    struct akl_value *lv = akl_frame_pop(cx);
    if (lv != NULL && (AKL_CHECK_TYPE(lv, AKL_VT_LIST)
            || AKL_CHECK_TYPE(lv, AKL_VT_NIL))) {
        return AKL_TRUE;
    }
    return AKL_NIL;
}

AKL_DEFINE_FUN(isnumber, cx, argc)
{
    double *n = akl_frame_pop_number(cx);
    if (n == NULL) {
        return AKL_NIL;
    }
    return AKL_TRUE;
}

AKL_DEFINE_FUN(issymbol, cx, argc)
{
    struct akl_value *v = akl_frame_pop(cx);
    if (AKL_CHECK_TYPE(v, AKL_VT_SYMBOL)) {
        return AKL_TRUE;
    }
    return AKL_NIL;
}

AKL_DEFINE_FUN(describe, cx, argc)
{
    struct akl_symbol *sym;
    struct akl_variable *fn;
    if (akl_get_args_strict(cx, 1, AKL_VT_SYMBOL, &sym) == -1) {
       return AKL_NIL;
    }
    fn = akl_get_global_var(cx->cx_state, sym);
    if (!fn || !fn->vr_desc) {
        akl_raise_error(cx, AKL_WARNING, "Global atom '%s' cannot found", sym->sb_name);
        return AKL_NIL;
    }
    return AKL_STRING(cx, fn->vr_desc);
}

extern void show_features(struct akl_state *, const char *fname); // @ util.c

// To set specific interpreter features
AKL_DEFINE_FUN(akl_cfg, cx, argc)
{
    struct akl_symbol *sym;
    const char *sname;
    struct akl_state *s = cx->cx_state;
    if (akl_get_args_strict(cx, 1, AKL_VT_OPT, AKL_VT_SYMBOL, &sym) == -1) {
       show_features(s, cx->cx_func_name);
       return AKL_NIL;
    }
    sname = sym->sb_name;
    if (akl_set_feature(s, sname)) {
        return AKL_TRUE;
    } else {
       if (strcmp(sname, "help") == 0)
           show_features(s, cx->cx_func_name);
       else
           akl_raise_error(cx, AKL_WARNING, "Cannot set feature '%s'", sname);
       return AKL_NIL;
    }
}

AKL_DEFINE_FUN(print, cx, argc)
{
    struct akl_value *v;
    while ((v = akl_frame_shift(cx)) != NULL) {
        akl_print_value(cx->cx_state, v);
    }

    printf("\n");
    return !v ? AKL_NIL : v;
}

static void
formatted_display(struct akl_context *ctx)
{
    struct akl_value *v;
    while ((v = akl_frame_shift(ctx)) != NULL) {
        switch (v->va_type) {
            case AKL_VT_NUMBER:
            printf("%g", AKL_GET_NUMBER_VALUE(v));
            break;

            case AKL_VT_STRING:
            printf("%s", AKL_GET_STRING_VALUE(v));
            break;

            default:
            break;
        }
    }
}

AKL_DEFINE_FUN(display, cx, argc)
{
    formatted_display(cx);
    printf("\n");
    return &TRUE_VALUE;
}

AKL_DEFINE_FUN(write, cx, argc)
{
    formatted_display(cx);
    return &TRUE_VALUE;
}

AKL_DEFINE_FUN(plus, cx, argc)
{
    double sum = 0.0;
    double *n;
    while ((n = akl_frame_pop_number(cx)) != NULL) {
       sum += *n;
    }

    return AKL_NUMBER(cx, sum);
}

AKL_DEFINE_FUN(minus, cx, argc)
{
    double a,n;
    struct akl_value *v = akl_frame_shift(cx);
    if (v == NULL) {
        return AKL_NUMBER(cx, 0);
    }
    if (AKL_CHECK_TYPE(v, AKL_VT_NUMBER)) {
        a = AKL_GET_NUMBER_VALUE(v);
    } else {
        goto not_a_number;
    }

    while ((v = akl_frame_shift(cx)) != NULL) {
        if (AKL_CHECK_TYPE(v, AKL_VT_NUMBER)) {
            n = AKL_GET_NUMBER_VALUE(v);
            a -= n;
        } else {
            goto not_a_number;
        }
    }
    return AKL_NUMBER(cx, a);

not_a_number:
    akl_raise_error(cx, AKL_ERROR, "Argument is not a number!");
    return AKL_NUMBER(cx, 0);
}


AKL_DEFINE_FUN(mul, cx, argc)
{
    double prod = 1.0;
    double *n;
    while ((n = akl_frame_pop_number(cx)) != NULL) {
       prod *= *n;
    }

    return AKL_NUMBER(cx, prod);
}

AKL_DEFINE_FUN(ddiv, cx, argc)
{
    double *n = akl_frame_shift_number(cx);
    double div = 0.0;
    if (n == NULL) {
        return AKL_NUMBER(cx, 0.0);
    }
    div = (*n);
    do {
        n = akl_frame_shift_number(cx);
        if (n == NULL) {
            return AKL_NUMBER(cx, div);
        }
        if ((*n) == 0.0) {
            akl_raise_error(cx, AKL_ERROR, "Zero division.");
            return AKL_NIL;
        }
        div /= (*n);
    } while (n);

    return AKL_NUMBER(cx, div);
}

AKL_DEFINE_FUN(idiv, cx, argc)
{
    double *n = akl_frame_shift_number(cx);
    long div = 0;
    if (n == NULL) {
        return AKL_NUMBER(cx, 0);
    }
    div = (*n);
    do {
        n = akl_frame_shift_number(cx);
        if (n == NULL) {
            return AKL_NUMBER(cx, div);
        }
        if (((long)(*n)) == 0) {
            akl_raise_error(cx, AKL_ERROR, "Zero division.");
            return AKL_NIL;
        }
        div /= (int)(*n);
    } while (n);

    return AKL_NUMBER(cx, div);
}

AKL_DEFINE_FUN(mod, cx, argc)
{
    double a, b;
    if (akl_get_args_strict(cx, 2, AKL_VT_NUMBER, &a, AKL_VT_NUMBER, &b) == -1) {
        return AKL_NIL;
    }

    return AKL_NUMBER(cx, (int)a % (int)b);
}

AKL_DEFINE_FUN(write_times, cx, argc)
{
    double n;
    int i;
    char *str;
    if (akl_get_args_strict(cx, 2, AKL_VT_NUMBER, &n, AKL_VT_STRING, &str))
        return AKL_NIL;

    for (i = 0; i < (int)n; i++) {
        printf("%s\n", str);
    }

    return AKL_NUMBER(cx, n);
}

AKL_DEFINE_FUN(print_symbol_ptr, cx, argc)
{
    struct akl_value *val;
    struct akl_symbol *sym;
    val = akl_frame_pop(cx);
    if (AKL_CHECK_TYPE(val, AKL_VT_SYMBOL)) {
        sym = val->va_value.symbol;
        printf("name: %s, ptr: %p\n", sym->sb_name, (void *)sym);
    } else {
        akl_raise_error(cx, AKL_WARNING, "Parameter is not a symbol");
        val = AKL_NIL;
    }
    return val;
}

AKL_DEFINE_FUN(hello, cx, argc)
{
    printf("This is a hello world function!\n");
    return AKL_NIL;
}

AKL_DEFINE_FUN(dump_stack, cx, argc)
{
    struct akl_value *v;
    struct akl_list_entry *ent;
    int n = 0;
    printf("stack contents:\n");
    AKL_LIST_FOREACH(ent, cx->cx_stack) {
        v = (struct akl_value *)ent->le_data;
        printf("%%%d: ", n);
        akl_print_value(cx->cx_state, v);
        printf("\n");
    }
    return AKL_NIL;
}

AKL_DEFINE_FUN(dump_vars, cx, argc)
{
    struct akl_variable *var;
    struct akl_symbol *sym;
    struct akl_state *s = cx->cx_state;
    RB_FOREACH(var, VAR_TREE, &s->ai_global_vars) {
        sym = var->vr_symbol;
        printf("%s (var: %p, symbol: %p)\n", sym->sb_name, var, sym);
    }
    return AKL_NIL;
}

static int
get_and_compare_values(struct akl_context *ctx)
{
    struct akl_value *a1, *a2;
    if (akl_get_args(ctx, 2, &a1, &a2))
        return -2;

    return akl_compare_values(a1, a2);
}

AKL_DEFINE_FUN(gt, ctx, argc)
{
    if (get_and_compare_values(ctx) > 0) {
        return AKL_TRUE;
    }

    return AKL_NIL;
}

AKL_DEFINE_FUN(gteq, ctx, argc)
{
    if (get_and_compare_values(ctx) >= 0) {
        return AKL_TRUE;
    }

    return AKL_NIL;
}

AKL_DEFINE_FUN(lt, ctx, argc)
{
    if (get_and_compare_values(ctx) < 0) {
        return AKL_TRUE;
    }

    return AKL_NIL;
}

AKL_DEFINE_FUN(lteq, ctx, argc)
{
    if (get_and_compare_values(ctx) <= 0) {
        return AKL_TRUE;
    }

    return AKL_NIL;
}

AKL_DEFINE_FUN(neq, ctx, argc)
{
    if (get_and_compare_values(ctx) != 0) {
        return AKL_TRUE;
    }

    return AKL_NIL;
}

AKL_DEFINE_FUN(eq, ctx, argc)
{
    if (get_and_compare_values(ctx) == 0) {
        return AKL_TRUE;
    }

    return AKL_NIL;
}


AKL_DEFINE_FUN(length, ctx, argc)
{
    char *t;
    struct akl_value *vp;
    if (akl_get_args(ctx, 1, &vp) == -1) {
        return AKL_NIL;
    }

    switch (AKL_TYPE(vp)) {
        case AKL_VT_STRING:
        t = AKL_GET_STRING_VALUE(vp);
        /* TODO: Handle UTF-8 */
        return akl_new_number_value(ctx->cx_state, (double)strlen(t));

        case AKL_VT_LIST:
        return akl_new_number_value(ctx->cx_state
                          , (double)akl_list_count(AKL_GET_LIST_VALUE(vp)));
        default:
        akl_raise_error(ctx, AKL_ERROR, "Argument must be a list or a string!");
    }
    return AKL_NIL;
}

AKL_DEFINE_FUN(ls_index, ctx, argc)
{
    double *n = akl_frame_shift_number(ctx);
    int i;
    struct akl_value *v = akl_frame_pop(ctx);
    char *t;
    char *ns;
    if (n == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "No index is given.");
        return AKL_NIL;
    }

    if (v == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "No indexable value is given.");
        return AKL_NIL;
    }

    i = (int)*n;

    switch (AKL_TYPE(v)) {
        case AKL_VT_STRING:
        t = AKL_GET_STRING_VALUE(v);
        v = NULL;
        if (i >= 0 && i < strlen(t)) {
            ns = akl_alloc(ctx->cx_state, sizeof(char)*2);
            ns[0] = t[i];
            ns[1] = '\0';
            v = akl_new_string_value(ctx->cx_state, ns);
        }
        break;

        case AKL_VT_LIST:
        v = akl_list_index_value(AKL_GET_LIST_VALUE(v), i);
        break;

        default:
        akl_raise_error(ctx, AKL_ERROR, "Argument must be a list or a string!");
        break;
    }
    return AKL_NULLER(v);
}

AKL_DEFINE_FUN(ls_head, ctx, argc)
{
    struct akl_value *v = akl_frame_pop(ctx);
    char *t;
    char *ns;

    if (v == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "No parameter is given.");
        return AKL_NIL;
    }

    switch (AKL_TYPE(v)) {
        case AKL_VT_STRING:
        t = AKL_GET_STRING_VALUE(v);
        v = NULL;
        if (strlen(t) > 0) {
            ns = akl_alloc(ctx->cx_state, sizeof(char)*2);
            ns[0] = t[0];
            ns[1] = '\0';
            v = akl_new_string_value(ctx->cx_state, ns);
        }
        break;

        case AKL_VT_LIST:
        v = akl_list_head(AKL_GET_LIST_VALUE(v));
        break;

        default:
        akl_raise_error(ctx, AKL_ERROR, "Argument must be a list or a string!");
        break;
    }
    return AKL_NULLER(v);
}

AKL_DEFINE_FUN(ls_last, ctx, argc)
{
    struct akl_value *v = akl_frame_pop(ctx);
    char *t;
    char *ns;
    size_t stlen;

    if (v == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "No parameter is given.");
        return AKL_NIL;
    }

    switch (AKL_TYPE(v)) {
        case AKL_VT_STRING:
        t = AKL_GET_STRING_VALUE(v);
        v = NULL;
        stlen = strlen(t);
        if (stlen > 0) {
            ns = akl_alloc(ctx->cx_state, 2);
            ns[0] = t[stlen-1];
            ns[1] = '\0';
            v = akl_new_string_value(ctx->cx_state, ns);
        }
        break;

        case AKL_VT_LIST:
        v = akl_list_last(AKL_GET_LIST_VALUE(v));
        break;

        default:
        akl_raise_error(ctx, AKL_ERROR, "Argument must be a list or a string!");
        break;
    }
    return AKL_NULLER(v);
}

AKL_DEFINE_FUN(ls_tail, ctx, argc)
{
    struct akl_value *v = akl_frame_pop(ctx);
    struct akl_list *l;
    char *t;

    if (v == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "No parameter is given.");
        return AKL_NIL;
    }

    switch (AKL_TYPE(v)) {
        case AKL_VT_STRING:
        t = AKL_GET_STRING_VALUE(v);
        v = NULL;
        if (strlen(t) > 0) {
            v = akl_new_string_value(ctx->cx_state, strdup(t+1));
        }
        break;

        case AKL_VT_LIST:
        l = akl_list_tail(ctx->cx_state, AKL_GET_LIST_VALUE(v));
        if (l == NULL) {
            return AKL_NIL;
        }
        l->is_quoted = TRUE;
        if (l == NULL) {
            v = NULL;
        } else {
            v = akl_new_list_value(ctx->cx_state, l);
        }
        break;

        default:
        akl_raise_error(ctx, AKL_ERROR, "Argument must be a list or a string!");
        break;
    }
    return AKL_NULLER(v);
}

AKL_DEFINE_FUN(ls_insert, ctx, argc)
{
    struct akl_value *iv = akl_frame_shift(ctx);
    struct akl_value *v = akl_frame_shift(ctx);
    struct akl_list *l;
    char *t, *s;
    char *ns;

    if (iv == NULL || v == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "Need a list or string as parameters.");
        return AKL_NIL;
    }

    switch (AKL_TYPE(v)) {
        case AKL_VT_STRING:
        t = AKL_GET_STRING_VALUE(v);
        s = AKL_GET_STRING_VALUE(iv);
        if (AKL_CHECK_TYPE(iv, AKL_VT_STRING)) {
            ns = akl_alloc(ctx->cx_state, 1+strlen(t)+strlen(s));
            strcpy(ns, s);
            strcat(ns, t);
            v->va_value.string = ns;
            akl_free(ctx->cx_state, t, strlen(t));
        } else {
            akl_raise_error(ctx, AKL_ERROR, "Must insert string.");
            v = NULL;
        }

        case AKL_VT_NIL:
        l = akl_new_list(ctx->cx_state);
        v = akl_new_list_value(ctx->cx_state, l);
        akl_list_insert_head(ctx->cx_state, l, iv);
        break;   break;

        case AKL_VT_LIST:
        akl_list_insert_head_value(ctx->cx_state, AKL_GET_LIST_VALUE(v), iv);
        break;

        default:
        akl_raise_error(ctx, AKL_ERROR, "Argument must be a list or a string!");
        break;
    }
    return AKL_NULLER(v);
}

AKL_DEFINE_FUN(ls_append, ctx, argc)
{
    struct akl_value *iv = akl_frame_shift(ctx);
    struct akl_value *v = akl_frame_shift(ctx);
    struct akl_list *l;
    char *t, *s;
    char *ns;

    if (iv == NULL || v == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "Need a list or string as parameters.");
        return AKL_NIL;
    }

    switch (AKL_TYPE(v)) {
        case AKL_VT_STRING:
        t = AKL_GET_STRING_VALUE(v);
        s = AKL_GET_STRING_VALUE(iv);
        if (AKL_CHECK_TYPE(iv, AKL_VT_STRING)) {
            ns = akl_alloc(ctx->cx_state, 1+strlen(t)+strlen(s));
            strcpy(ns, t);
            strcat(ns, s);
            v->va_value.string = ns;
            akl_free(ctx->cx_state, t, strlen(t));
        } else {
            akl_raise_error(ctx, AKL_ERROR, "Must append string.");
            v = NULL;
        }
        break;

        case AKL_VT_LIST:
        akl_list_append_value(ctx->cx_state, AKL_GET_LIST_VALUE(v), iv);
        break;

        case AKL_VT_NIL:
        l = akl_new_list(ctx->cx_state);
        v = akl_new_list_value(ctx->cx_state, l);
        akl_list_append(ctx->cx_state, l, iv);
        break;

        default:
        akl_raise_error(ctx, AKL_ERROR, "Argument must be a list or a string!");
        break;
    }
    return AKL_NULLER(v);
}

AKL_DEFINE_FUN(progn, ctx, argc)
{
    return akl_frame_pop(ctx);
}

AKL_DEFINE_FUN(list, ctx, argc)
{
    struct akl_value *v;
    struct akl_list *list = akl_new_list(ctx->cx_state);
    while ((v = akl_frame_shift(ctx))) {
        akl_list_append_value(ctx->cx_state, list, v);
    }
    list->is_quoted = TRUE;
    return akl_new_list_value(ctx->cx_state, list);
}

AKL_DEFINE_FUN(map, ctx, argc)
{
    struct akl_function *fn;
    struct akl_list *lp, *nl;
    struct akl_list_entry *it;
    struct akl_value *v;
    struct akl_context *cx;
    // TODO: Change TYPE_* to bit masks.
    if (akl_get_args_strict(ctx, 2, AKL_VT_LIST, &lp, AKL_VT_FUNCTION, &fn) == -1) {
       return AKL_NIL;
    }
    it = akl_list_it_begin(lp);
    cx = akl_bound_function(ctx, NULL, fn);
    nl = akl_new_list(ctx->cx_state);
    nl->is_quoted = TRUE;
    while ((v = akl_list_it_next(&it)) != NULL) {
        akl_stack_push(ctx, v);
        akl_call_function_bound(cx, 1); /* TODO: How to go with more arguments? */
        akl_list_append_value(ctx->cx_state, nl, akl_stack_pop(ctx));
    }

    return akl_new_list_value(ctx->cx_state, nl);
}

AKL_DEFINE_FUN(map_index, ctx, argc)
{
    struct akl_function *fn;
    struct akl_list *lp, *nl;
    struct akl_list_entry *it;
    struct akl_value *v;
    struct akl_context *cx;
    int ind = 0;
    // TODO: Change TYPE_* to bit masks.
    if (akl_get_args_strict(ctx, 2, AKL_VT_LIST, &lp, AKL_VT_FUNCTION, &fn) == -1) {
       return AKL_NIL;
    }
    it = akl_list_it_begin(lp);
    cx = akl_bound_function(ctx, NULL, fn);
    nl = akl_new_list(ctx->cx_state);
    nl->is_quoted = TRUE;
    while ((v = akl_list_it_next(&it)) != NULL) {
        akl_stack_push(ctx, AKL_NUMBER(ctx, ind++));
        akl_stack_push(ctx, v);
        akl_call_function_bound(cx, 1);
        akl_list_append_value(ctx->cx_state, nl, akl_stack_pop(ctx));
    }

    return akl_new_list_value(ctx->cx_state, nl);
}

AKL_DEFINE_FUN(foldl, ctx, argc)
{
    struct akl_function *fn;
    struct akl_list *lp;
    struct akl_list_entry *it;
    struct akl_value *v, *vl;
    struct akl_context *cx;
    if (akl_get_args_strict(ctx, 3, AKL_VT_ANY, &v, AKL_VT_LIST, &lp, AKL_VT_FUNCTION, &fn) == -1) {
       return AKL_NIL;
    }
    it = akl_list_it_begin(lp);
    cx = akl_bound_function(ctx, NULL, fn);
    while ((vl = akl_list_it_next(&it)) != NULL) {
        akl_stack_push(cx, v);
        akl_stack_push(cx, vl);
        akl_call_function_bound(cx, 2);
        v = akl_stack_pop(cx);
    }

    return v;
}

static struct akl_value *
times_impl(struct akl_context *ctx, bool_t is_indexed)
{
    struct akl_function *fn;
    struct akl_context *cx;
    struct akl_list *nl;
    int i;
    double times_arg;
    // TODO: Change TYPE_* to bit masks.
    if (akl_get_args_strict(ctx, 2, AKL_VT_NUMBER, &times_arg
                                , AKL_VT_FUNCTION, &fn) == -1) {
       return AKL_NIL;
    }
    cx = akl_bound_function(ctx, NULL, fn);
    nl = akl_new_list(ctx->cx_state);
    nl->is_quoted = TRUE;
    for (i = 0; i < (int)times_arg; i++) {
        if (is_indexed) {
            akl_stack_push(cx, AKL_NUMBER(cx, i));
        }
        akl_call_function_bound(cx, (is_indexed) ? 1 : 0);
        akl_list_append_value(ctx->cx_state, nl, akl_stack_pop(ctx));
    }

    return akl_new_list_value(ctx->cx_state, nl);
}

AKL_DEFINE_FUN(times, ctx, argc)
{
    return times_impl(ctx, FALSE);
}

AKL_DEFINE_FUN(times_index, ctx, argc)
{
    return times_impl(ctx, TRUE);
}

AKL_DEFINE_FUN(range, ctx, argc)
{
    double fp, tp;
    struct akl_list *list;
    int f, t;
    if (akl_get_args_strict(ctx, 2, AKL_VT_NUMBER, &fp, AKL_VT_NUMBER, &tp) == -1) {
        return AKL_NIL;
    }

    f = (int)fp;
    t = (int)tp;
    list = akl_new_list(ctx->cx_state);
    list->is_quoted = TRUE;
    for (;f <= t; f++) {
       akl_list_append_value(ctx->cx_state, list, AKL_NUMBER(ctx, (double)f));
    }
    return akl_new_list_value(ctx->cx_state, list);
}

AKL_DEFINE_FUN(disassemble, ctx, argc)
{
    struct akl_symbol *sym;
    struct akl_variable *var;
    struct akl_value *v;
    struct akl_function *fn;
    if (argc == 1) {
        if (akl_get_args_strict(ctx, 1, AKL_VT_SYMBOL, &sym) == -1) {
            return NULL;
        }

        var = akl_get_global_var(ctx->cx_state, sym);
        if (var && AKL_CHECK_TYPE((v = var->vr_value), AKL_VT_FUNCTION)) {
            fn = v->va_value.func;
        } else {
            return AKL_NIL;
        }
        AKL_START_COLOR(ctx->cx_state, AKL_YELLOW);
        if (var->vr_symbol && var->vr_symbol->sb_name) {
            printf("\n.%s:\n", var->vr_symbol->sb_name);
        }
        AKL_END_COLOR(ctx->cx_state);
    } else {
        fn = ctx->cx_fn_main;
    }
    akl_dump_ir(ctx, fn);
    return AKL_NIL;
}

static struct tm *akl_time(void)
{
    time_t curr;
    time(&curr);
    return localtime(&curr);
}

static const char *day_names[] = {
    "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"
};

static struct akl_value *
add_time_to_list(struct akl_context *ctx, struct tm *time, struct akl_list *l)
{
    struct akl_state *s = ctx->cx_state;
    struct akl_value *lv;
    akl_list_append_value(s, l, AKL_NUMBER(ctx, time->tm_hour));
    akl_list_append_value(s, l, AKL_NUMBER(ctx, time->tm_min));
    akl_list_append_value(s, l, AKL_NUMBER(ctx, time->tm_sec));

    lv = akl_new_list_value(s, l);
    l->is_quoted  = TRUE;
    lv->is_quoted = TRUE;
    return lv;
}

AKL_DEFINE_FUN(getdatetime, ctx, argc)
{
    struct akl_state *s = ctx->cx_state;
    struct akl_list *l  = akl_new_list(s);
    struct tm *time     = akl_time();
    struct akl_symbol *day_of_week;
    struct akl_value  *dow;

    akl_list_append_value(s, l, AKL_NUMBER(ctx, time->tm_year + 1900));
    akl_list_append_value(s, l, AKL_NUMBER(ctx, time->tm_mon));
    akl_list_append_value(s, l, AKL_NUMBER(ctx, time->tm_mday));
    day_of_week    = akl_new_symbol(s, (char *)day_names[time->tm_wday], TRUE);
    dow            = akl_new_sym_value(s, day_of_week);
    dow->is_quoted = TRUE;
    akl_list_append_value(s, l, dow);
    return add_time_to_list(ctx, time, l);
}

AKL_DEFINE_FUN(gettime, ctx, argc)
{
    struct akl_list *l  = akl_new_list(ctx->cx_state);
    return add_time_to_list(ctx, akl_time(), l);
}

AKL_DEFINE_NE_FUN(set, ctx, argc)
{
    return AKL_NIL;
}

extern void akl_stack_clear(struct akl_context *ctx, size_t c);
AKL_DEFINE_FUN(clear_stack, ctx, argc)
{
    while (akl_stack_pop(ctx))
        ;
    return AKL_NIL;
}

AKL_DEFINE_FUN(about, ctx, argc)
{
    const char *tnames[] = { "Atoms", "Lists"
                           , "Numbers", "Strings"
                           , "List entries", "Total allocated bytes"
                           , NULL };
    int c;
    struct akl_list_entry *ent;
    struct akl_module *mod;
    struct akl_state *s = ctx->cx_state;
    printf("\nAkLisp version %d.%d-%s\n"
            "\tCopyleft (c) Akos Kovacs\n"
            "\tBuilt on %s %s\n"
#ifdef AKL_SYSTEM_INFO
            "\tBuild platform: %s (%s)\n"
            "\tBuild processor: %s\n"
#endif // AKL_SYSTEM_INFO
#ifdef AKL_USER_INFO
            "\tBuilt by: %s@%s\n"
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
    c = akl_list_count(&s->ai_modules);
    if (c > 0)
        printf("\n%u loaded module%s:\n", c, (c > 1)?"s":"");

    AKL_LIST_FOREACH(ent, &s->ai_modules) {
        if (ent) {
            mod = (struct akl_module *)ent->le_data;
            if (mod->am_name && mod->am_path)
                printf("\tName: '%s'\n\tPath: '%s'\n", mod->am_name, mod->am_path);
            if (mod->am_desc)
                printf("\tDescription: '%s'\n", mod->am_desc);
            if (mod->am_author)
                printf("\tAuthor: '%s'\n", mod->am_author);
        }
        printf("\n");
    }
    printf("\nGC statistics:\n");
    printf("\tallocated memory: %u bytes\n", s->ai_gc_malloc_size);

    return &TRUE_VALUE;
}

AKL_DEFINE_FUN(read_number, ctx, argc)
{
    double n = 0.0;
    char ch;
    if (fscanf(stdin, "%lf", &n) <= 0) {
        /* Flush the input stream, so the next read-number
           can work again. */
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
        return AKL_NIL;
    }
    return AKL_NUMBER(ctx, n);
}

AKL_DEFINE_FUN(read_string, ctx, argc)
{
    char *str = NULL;
    size_t n;
    ssize_t len;
    if ((len = getline(&str, &n, stdin)) == -1) {
        return AKL_NIL;
    }
    if (str != NULL) {
        if (str[--len] == '\n') {
            str[len] = '\0';
        }
        return akl_new_string_value(ctx->cx_state, str);
    }

    return AKL_NIL;
}

AKL_DEFINE_FUN(not, ctx, argc)
{
    struct akl_value *v = akl_frame_pop(ctx);
    if (AKL_IS_NIL(v)) {
        return AKL_TRUE;
    }
    return AKL_NIL;
}

AKL_DEFINE_FUN(and, ctx, argc)
{
    struct akl_value *v = NULL;
    do {
       v = akl_frame_pop(ctx);
       if (v == NULL) {
           break;
       }
       if (AKL_IS_NIL(v)) {
           return AKL_NIL;
       }

    } while (v != NULL);
    return AKL_TRUE;
}

AKL_DEFINE_FUN(or, ctx, argc)
{
    struct akl_value *v = NULL;
    do {
       v = akl_frame_pop(ctx);
       if (v == NULL) {
           break;
       }
       if (AKL_IS_TRUE(v)) {
           return AKL_TRUE;
       }
    } while (v != NULL);
    return AKL_NIL;
}

const char *akl_lex_get_filename(struct akl_io_device *dev);

AKL_DEFINE_FUN(load, ctx, argc)
{
    struct akl_io_device *dev;
    struct akl_context *cx;
    char *fname;
    char path[PATH_MAX];
    FILE *fp;
    if (akl_get_args_strict(ctx, 1, AKL_VT_STRING, &fname) == -1) {
        return AKL_NIL;
    }
    if (fname[0] == '.' && fname[1] == '/') {
        char *curr_fname = (char *)akl_lex_get_filename(ctx->cx_dev);
        if (curr_fname != NULL) {
            curr_fname = realpath(curr_fname, NULL);
        }
        char *dname      = NULL;
        /* TODO: Make this portable... */
        if (curr_fname) {
            /* Script relative paths have to work */
            dname = dirname(curr_fname);
        }
        if (dname) {
            strncpy(path, dname, sizeof(path));
            free(curr_fname);

            strncat(path, fname+1, sizeof(path));
            path[sizeof(path)-1] = '\0';
            fname = path;
        }
    }

    if ((fp = fopen(fname, "r")) == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "Cannot load '%s', cannot open file.", fname);
        return AKL_NIL;
    }
    dev = akl_new_file_device(ctx->cx_state, fname, fp);
    cx  = akl_compile(ctx->cx_state, dev);
    cx->cx_parent = ctx;
    akl_execute(cx);
    return AKL_TRUE;
}

AKL_DEFINE_FUN(toint, ctx, argc)
{
    int n;
    struct akl_value *v;
    struct akl_symbol *sym;
    char *str;
    if (akl_get_args(ctx, 1, &v) == -1) {
        return AKL_NIL;
    }

    switch (AKL_TYPE(v)) {
        case AKL_VT_NIL:
        n = 0;
        break;

        case AKL_VT_NUMBER:
        n = (int)AKL_GET_NUMBER_VALUE(v);
        break;

        case AKL_VT_STRING:
        str = AKL_GET_STRING_VALUE(v);
        if (str != NULL) {
            n = (int)atoi(str);
        } else {
            return AKL_NIL;
        }
        break;

        case AKL_VT_SYMBOL:
        sym = v->va_value.symbol;
        if (sym != NULL && sym->sb_name) {
            n = (int)atoi(sym->sb_name);
        } else {
            return AKL_NIL;
        }
        break;
    }
    return AKL_NUMBER(ctx, n);
}

AKL_DEFINE_FUN(tonumber, ctx, argc)
{
    struct akl_value *v;
    if (akl_get_args(ctx, 1, &v) == -1) {
        return AKL_NIL;
    }
    return akl_to_number(ctx->cx_state, v);
}

AKL_DEFINE_FUN(tostr, ctx, argc)
{
    double n;
    struct akl_value *v;
    if (akl_get_args(ctx, 1, &v) == -1) {
        return AKL_NIL;
    }

    return akl_to_string(ctx->cx_state, v);
}

AKL_DEFINE_FUN(split, cx, argc) {
    struct akl_value *vstr, *vdelim, *strent;
    char *str, *sub, *delim = " ";
    struct akl_list *ret = NULL;

    vstr = akl_frame_shift(cx);
    if (!AKL_CHECK_TYPE(vstr, AKL_VT_STRING)) {
        akl_raise_error(cx, AKL_ERROR, "No string to split!");
        return AKL_NIL;
    }
    vdelim = akl_frame_shift(cx);
    if (vdelim != NULL) {
        if (AKL_CHECK_TYPE(vdelim, AKL_VT_STRING)) {
            delim = AKL_GET_STRING_VALUE(vdelim);
        } else {
            akl_raise_error(cx, AKL_WARNING, "Delimiter is not a string! Falling back to default delimiter.");
        }
    }
    str = AKL_GET_STRING_VALUE(vstr);
    str = (str != NULL) ? strdup(str) : NULL;
    ret = akl_new_list(cx->cx_state);
    ret->is_quoted = TRUE;
    while ((sub = strtok(str, delim)) != NULL) {
        strent = akl_new_string_value(cx->cx_state, sub);
        akl_list_append_value(cx->cx_state, ret, strent);
        str = NULL;
    }
    return AKL_LIST(cx, ret);
}

#if 0
AKL_CFUN_DEFINE(load, ctx, argc)
{
    char *modname;
    akl_get_args_strict(ctx, 1, TYPE_STRING, &modname);
    return akl_load_module(ctx->cx_state, modname) ? &TRUE_VALUE : &NIL_VALUE;
}

AKL_CFUN_DEFINE(unload, ctx, argc)
{
    char *modname;
    akl_get_args_strict(ctx, 1, TYPE_STRING, &modname);
    return akl_unload_module(ctx->cx_state, modname, FALSE) ? &TRUE_VALUE : &NIL_VALUE;
}

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
        ent->le_value = (struct akl_value *)akl_eval_value(in, arg);
        if (AKL_IS_NIL(AKL_ENTRY_VALUE(ent)))
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
        if (!AKL_IS_NIL(AKL_ENTRY_VALUE(ent)))
            return ent->le_value;
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(exit, in __unused, args)
{
    if (in->ai_interactive)
        printf("Bye!\n");
    if (AKL_IS_NIL(args)) {
        exit(0);
    } else {
        exit(AKL_GET_NUMBER_VALUE(AKL_FIRST_VALUE(args)));
    }
}

AKL_CFUN_DEFINE(eval, s, args)
{
    struct akl_value *last, *a1 = AKL_FIRST_VALUE(args);
    struct akl_list *l = NULL;
    struct akl_io_device *dev;
    struct akl_list_entry *ent;
    if (AKL_CHECK_TYPE(a1, TYPE_LIST)) {
        l = AKL_GET_LIST_VALUE(a1);
    } else if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        const char *str = AKL_GET_STRING_VALUE(a1);
        dev = akl_new_string_device("string", str);
        l = akl_parse_io(s, dev);
        AKL_LIST_FOREACH(ent, l) {
            last = akl_eval_value(s, AKL_ENTRY_VALUE(ent));
        }
        return last;
        AKL_FREE(dev);
    } else {
        return a1;
    }
    l->is_quoted = FALSE;
    return akl_eval_list(s, l);
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

AKL_CFUN_DEFINE(to_int, in, args)
{
    struct akl_value *ret = akl_to_number(in, AKL_FIRST_VALUE(args));
    int n;
    if (ret) {
        n = AKL_GET_NUMBER_VALUE(ret);
        ret->va_value.number = n;
        return ret;
    }
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

AKL_BUILTIN_DEFINE(constp, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    struct akl_atom  *atom;
    if (a1) {
        switch (a1->va_type) {
            case TYPE_ATOM:
            atom = akl_get_global_atom(in, akl_get_atom_name_value(a1));
            return (atom && atom->at_is_const == TRUE) ? &TRUE_VALUE : &NIL_VALUE;

            case TYPE_LIST: case TYPE_USERDATA:
            return &NIL_VALUE;

            default:
            return &TRUE_VALUE;
        }
    }
    return &NIL_VALUE;
}

static struct akl_value *
akl_set_value(struct akl_state *s, struct akl_list *args, bool_t is_const)
{
    struct akl_atom *atom, *a;
    struct akl_value *value, *desc, *av;
    av = AKL_FIRST_VALUE(args);
    atom = AKL_GET_ATOM_VALUE(av);
    if (atom == NULL) {
        akl_add_error(s, AKL_ERROR, av->va_lex_info
            , "ERROR: setq: First argument is not an atom!\n");
        return &NIL_VALUE;
    }
    value = akl_eval_value(s, AKL_SECOND_VALUE(args));
    if (args->li_elem_count > 1) {
        desc = AKL_THIRD_VALUE(args);
        if (AKL_CHECK_TYPE(desc, TYPE_STRING)) {
            atom->at_desc = AKL_GET_STRING_VALUE(desc);
        }
    }
    atom->at_value = value;
    atom->at_is_const = is_const;
    a = akl_add_global_atom(s, atom);
    /* If the atom is already exist, but not a constant */
    if (a && !a->at_is_const) {
        AKL_FREE(a->at_desc);
        a->at_value = atom->at_value;
        a->at_desc = atom->at_desc;
        AKL_FREE(atom->at_name);
        AKL_FREE(atom);
    } else if (a && a->at_is_const) {
        /* It's a constant, cannot write */
        akl_add_error(s, AKL_ERROR, av->va_lex_info
            , "ERROR: set: Cannot bound to constant atom\n");
        return &NIL_VALUE;
    } else {
    }

    return value;
}

AKL_BUILTIN_DEFINE(setq, in, args)
{
    return akl_set_value(in, args, FALSE);
}

AKL_BUILTIN_DEFINE(setc, in, args)
{
    return akl_set_value(in, args, TRUE);
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
    akl_list_append_value(in, version, akl_new_number_value(in, VER_MAJOR));
    akl_list_append_value(in, version, akl_new_number_value(in, VER_MINOR));
    addit = akl_new_atom_value(in, ver);
    addit->is_quoted = TRUE;
    akl_list_append_value(in, version, addit);
    version->is_quoted = 1;
    return akl_new_list_value(in, version);
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
                akl_list_append_value(in, range, akl_new_number_value(in, i));
            }
        } else if (rf > rs) {
            for (i = rf; i >= rs; i -= rt) {
                akl_list_append_value(in, range, akl_new_number_value(in, i));
            }
        } else {
            akl_list_append_value(in, range, akl_new_number_value(in, rf));
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
    char *str, *rstr;
    a1 = AKL_FIRST_VALUE(args); /* The index itself */
    a2 = AKL_SECOND_VALUE(args);
    if (AKL_CHECK_TYPE(a1, TYPE_NUMBER) && !AKL_IS_NIL(a1)) {
        ind = AKL_GET_NUMBER_VALUE(a1);
    }
    if (AKL_CHECK_TYPE(a2, TYPE_LIST) && !AKL_IS_NIL(a2)) {
        list = AKL_GET_LIST_VALUE(a2);
        return akl_list_index_value(list, ind);
    } else if (AKL_CHECK_TYPE(a2, TYPE_STRING) && !AKL_IS_NIL(a2)) {
        str = AKL_GET_STRING_VALUE(a2);
        if (str && strlen(str) > ind) {
            rstr = (char *)akl_malloc(in, 2);
            rstr[0] = str[ind];
            rstr[1] = '\0';
            return akl_new_string_value(in, rstr);
        }
    }

    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(split, in, args)
{
    struct akl_value *a1, *a2, *strent;
    char *str, *sub, *delim = " ";
    unsigned int i, li;
    struct akl_list *ret;
    a1 = AKL_FIRST_VALUE(args);
    a2 = AKL_SECOND_VALUE(args); /* delimiter */

    if (AKL_CHECK_TYPE(a2, TYPE_STRING))
        delim = AKL_GET_STRING_VALUE(a2);

    if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        str = strdup(AKL_GET_STRING_VALUE(a1));
        ret = akl_new_list(in);
        ret->is_quoted = TRUE;
        while ((sub = strtok(str, delim)) != NULL) {
            strent = akl_new_string_value(in, sub);
            akl_list_append_value(in, ret, strent);
            str = NULL;
        }
        return akl_new_list_value(in, ret);
    } else {
        akl_add_error(in, AKL_ERROR, a1->va_lex_info
              , "ERROR: split: The first argument must be a string.");
    }
    return &NIL_VALUE;

}

static struct akl_value *cmp_two_args(struct akl_state *in __unused,
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
static struct akl_value *eval_if_true(struct akl_state *in
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
        tname = ((struct akl_module *)akl_vector_at(&in->ai_utypes
                                , akl_get_utype_value(a1)))->am_name;
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

    akl_list_insert_value_head(in, AKL_GET_LIST_VALUE(a2), a1);
    return a2;
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
    akl_list_append_value(in, tlist, time_hour_function(in, args));
    akl_list_append_value(in, tlist, time_min_function(in, args));
    akl_list_append_value(in, tlist, time_sec_function(in, args));
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
#endif

struct akl_list *akl_split(struct akl_state *s, const char *str, const char *sp)
{
    struct akl_list *l = akl_new_list(s);
    struct akl_vector sv;
    int i;
    int ch;
    akl_init_vector(s, &sv, 100, sizeof(char));
    if (!sp) {
        return NULL;
    }
    while (str) {
        for (i = 0; i < strlen(sp); i++) {
            if (*str == sp[i]) {
                if (akl_vector_count(&sv) == 0) {
                    continue;
                } else {
                    ch = '\0';
                    akl_vector_push(&sv, &ch);
                    if (sv.av_vector != NULL) {
                        akl_list_append_value(s, l, akl_new_string_value(s, strdup(sv.av_vector)));
                    }
                }
            } else {
                ch = *str;
                akl_vector_push(&sv, &ch);
            }
        }
        str++;
    }
    return l;
}

static void akl_define_mod_path(struct akl_state *s)
{
    struct akl_list *l = akl_new_list(s);

}

AKL_DECLARE_FUNS(akl_basic_funs) {
    AKL_FUN(inc,         "++", "Increment a number by one"),
    AKL_FUN(dec,         "--", "Decrement a number by one"),
    AKL_FUN(plus,        "+", "Arithmetic addition"),
    AKL_FUN(minus,       "-", "Arithmetic substraction"),
    AKL_FUN(mul,         "*", "Arithmetic product"),
    AKL_FUN(ddiv,        "/", "Arithmetic division"),
    AKL_FUN(idiv,        "div", "Integer division"),
    AKL_FUN(mod,         "mod", "Integeral modulus"),
    AKL_FUN(mod,         "%", "Integeral modulus"),
    AKL_FUN(neq,         "!=", "Compare to values for inequality"),
    AKL_FUN(eq,          "=", "Compare to values for equality"),
    AKL_FUN(gt,          ">", "Greater compare function"),
    AKL_FUN(lt,          "<", "Lesser than"),
    AKL_FUN(gteq,        ">=", "Greater than or equal"),
    AKL_FUN(lteq,        "<=", "Less or equal than"),
    AKL_FUN(not,         "not", "Logical not"),
    AKL_FUN(and,         "and", "Logical and"),
    AKL_FUN(or,          "or", "Logical or"),
    AKL_FUN(iszero,      "zero?", "Gives true if the parameter is zero, nil otherwise"),
    AKL_FUN(isnil,       "nil?", "Gives true if the parameter is nil"),
    AKL_FUN(isnumber,    "number?", "Gives true if the parameter is a number"),
    AKL_FUN(isstring,    "string?", "Gives true if the parameter is a string"),
    AKL_FUN(islist,      "list?", "Gives true if the parameter is a list"),
    AKL_FUN(issymbol,    "symbol?", "Gives true if the parameter is a symbol"),
    AKL_FUN(tonumber,    "number", "Converts values to a floating point number"),
    AKL_FUN(toint,       "int", "Converts values to an integer number"),
    AKL_FUN(tostr,       "string", "Converts values to a string"),
    AKL_FUN(list,        "list", "Create a list from the given arguments"),
    AKL_FUN(length,      "length", "Get the length of a string or the element count for a list"),
    AKL_FUN(ls_index,    "index", "Index a list or a string"),
    AKL_FUN(ls_head ,    "head", "Get the first element of a list or the first character of a string"),
    AKL_FUN(ls_head ,    "first", "Get the first element of a list or the first character of a string"),
    AKL_FUN(ls_head ,    "car", "Get the first element of a list or the first character of a string"),
    AKL_FUN(ls_tail,     "cdr", "Get the remaining elements or characters of a list or string (everything after head)"),
    AKL_FUN(ls_tail,     "tail", "Get the remaining elements or characters of a list or string (everything after head)"),
    AKL_FUN(ls_last ,    "last", "Get the last element of a list or the last character of a string"),
    AKL_FUN(ls_append,   "append!", "Add an element to the end of a list or string"),
    AKL_FUN(ls_insert,   "insert!", "Insert an element to the start of the list or a string"),
    AKL_FUN(split,       "split", "Split a string by a delimiter (default is space) into a list"),
    AKL_FUN(range,       "range", "Make a list of numbers from a range"),
    AKL_FUN(progn,       "$", "Evaulate all elements and give back the last (primitive sequence)"),
    AKL_FUN(akl_cfg,     "akl-cfg!", "Set/unset interpreter features"),
    AKL_FUN(describe,    "describe", "Get a global atom help string"),
    AKL_FUN(map,         "map", "Call a function on list elements"),
    AKL_FUN(map_index,   "map-index", "Call a function on list with the elements' index (from 0) and the elements themselves"),
    AKL_FUN(foldl,       "foldl", "Fold a list from left"),
    AKL_FUN(foldl,       "fold", "Fold a list from left"),
    AKL_FUN(times,       "times", "Call a function n times"),
    AKL_FUN(times_index, "times-index", "Call a function n times (also passing the index to the function)"),
    AKL_FUN(exit,        "exit!", "Exit"),
    AKL_END_FUNS()
};

AKL_DECLARE_FUNS(akl_debug_funs) {
    AKL_FUN(dump_stack,   "dump-stack", "Dump the stack contents"),
    AKL_FUN(clear_stack,  "clear-stack", "Clear the current stack"),
    AKL_FUN(disassemble,  "disassemble", "Disassemble a given function"),
    AKL_FUN(hello,        "hello", "Hello function"),
    AKL_FUN(write_times,  "write-times", "Write a string out n times"),
    AKL_FUN(about,        "about", "Informations about the interpreter"),
    AKL_FUN(dump_vars, "dump-vars", "Display all global variables (with symbol pointers"),
    AKL_FUN(print_symbol_ptr,  "print-symbol-ptr", "Display the symbol's internal pointer"),
    AKL_END_FUNS()
};

AKL_DECLARE_FUNS(akl_io_funs) {
    AKL_FUN(print,        "print", "Print a value in Lisp form"),
    AKL_FUN(display,      "display", "Display a value without formatting (with newline)"),
    AKL_FUN(write,        "write", "Display a value without formatting (without newline)"),
    AKL_FUN(read_number,  "read-number", "Read a number from the standard input"),
    AKL_FUN(read_string,  "read-string", "Read a string from the standard input"),
    AKL_FUN(read_string,  "getline", "Read a string from the standard input"),
    AKL_END_FUNS()
};

AKL_DECLARE_FUNS(akl_sys_funs) {
    AKL_FUN(getdatetime,  "get-date-time", "Get the current date and time in a list with the elements:"
                                           " '(year month[0-11] day[1-31] day-of-the-week[:monday, :thuesday, ...]"
                                           " hour[0-23] min[0-59] sec[0-60])"),
    AKL_FUN(gettime,      "get-time",      "Current time in a list, in the order of: "
                                           "'(hour[0-23] min[0-59] sec[0-60])"),
    AKL_END_FUNS()
};

AKL_DECLARE_FUNS(akl_module_funs) {
    AKL_FUN(load,  "load", "Load a file or module"),
    AKL_END_FUNS()
};

void akl_init_library(struct akl_state *s, enum AKL_INIT_FLAGS flags)
{
    if (flags & AKL_LIB_BASIC) {
        akl_declare_functions(s, akl_basic_funs);
    }
    akl_declare_functions(s, akl_debug_funs);
    if (flags & AKL_LIB_FILE) {
        akl_declare_functions(s, akl_io_funs);
    }
    if (flags & AKL_LIB_SYSTEM) {
        akl_declare_functions(s, akl_module_funs);
        akl_declare_functions(s, akl_sys_funs);
    }
    akl_spec_library_init(s, flags);
    akl_define_mod_path(s);
#if 0
    if (flags & AKL_LIB_BASIC) {
        AKL_ADD_BUILTIN(in, quote, "QUOTE", "Quote listame like as \'");
        AKL_ADD_BUILTIN(in, setq,  "SET!", "Bound (set) a variable to a value");
        AKL_ADD_BUILTIN(in, setc,  "SET-CONST!", "Bound (set) a value to a constant variable (it will be read-only)");
        AKL_ADD_CFUN(in, progn,"PROGN", "Return with the last value");
        AKL_ADD_CFUN(in, list, "LIST", "Create list");
        AKL_ADD_CFUN(in, car,  "CAR", "Get the head of a list");
        AKL_ADD_CFUN(in, cdr,  "CDR", "Get the tail of a list");
        AKL_ADD_CFUN(in, car,  "FIRST", "Get the first element of the list");
        AKL_ADD_CFUN(in, last,  "LAST", "Get back the last element of a list");
        AKL_ADD_CFUN(in, cdr,  "REST", "Get the tail of a list");
        AKL_ADD_CFUN(in, cdr,  "TAIL", "Get the tail of a list");
        AKL_ADD_CFUN(in, to_num,  "NUM", "Convert an arbitrary value to a number");
        AKL_ADD_CFUN(in, to_int,  "INT", "Convert an arbitrary value to an integer");
        AKL_ADD_CFUN(in, to_str,  "STR", "Convert an arbitrary value to a string");
        AKL_ADD_CFUN(in, to_sym,  "SYM", "Convert an arbitrary value to a symbol");
        AKL_ADD_CFUN(in, eval,  "EVAL", "Evaluate an S-expression (can be string)");
    }

    if (flags & AKL_LIB_DATA) {
        AKL_ADD_CFUN(in, range, "RANGE", "Create list with elements from arg 1 to arg 2");
        AKL_ADD_CFUN(in, cons,  "CONS", "Insert the first argument to the second list argument");
        AKL_ADD_CFUN(in, len,   "LENGTH", "The length of a given value");
        AKL_ADD_CFUN(in, index, "INDEX", "Index of list");
        AKL_ADD_CFUN(in, split, "SPLIT", "Split a string into a number of substrings, bounded by an optional delimiter (default is a space)");
        AKL_ADD_CFUN(in, typeof,"TYPEOF", "Get the type of the value");
        AKL_ADD_BUILTIN(in, desc ,"DESC", "Get the description (AKA documentation) of the variable");
    }

    if (flags & AKL_LIB_CONDITIONAL) {
        AKL_ADD_BUILTIN(in, if, "IF"
        , "If the first argument true, returns with the second, otherwise returns with the third");
        AKL_ADD_BUILTIN(in, cond, "COND"
        , "Similiar to if, but works with arbitrary number of conditions (called clauses)");
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
        AKL_ADD_BUILTIN(in, constp, "CONST?", "Predicate which, returns with true when it\'s argument is a constant");
    }

    if (flags & AKL_LIB_NUMBERIC) {
        AKL_ADD_BUILTIN(in, incf, "++", "Increase a number value by 1");
        AKL_ADD_BUILTIN(in, decf, "--", "Decrease a number value by 1");
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

    if (flags & AKL_LIB_FILE) {
        akl_init_file(in);
    }

    if (flags & AKL_LIB_OS) {
        akl_init_os(in);
    }
#endif
}
