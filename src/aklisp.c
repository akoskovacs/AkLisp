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

#include <stdarg.h>

static struct akl_atom *recent_value;

/* ~~~===### Stack handling ###===~~~ */
void akl_stack_init(struct akl_context *ctx)
{
    ctx->cx_frame = akl_new_list(ctx->cx_state);
}

struct akl_list_entry *akl_get_stack_pointer(struct akl_context *ctx)
{
    return (ctx && ctx->cx_frame) ? AKL_LIST_LAST(ctx->cx_frame) : NULL;
}

void
akl_init_frame(struct akl_context *ctx, struct akl_list *frame, size_t size)
{
    assert(ctx);
    assert(frame);
    frame->li_head = akl_list_index(ctx->cx_frame, -size);
    frame->li_last = akl_get_stack_pointer(ctx);
    frame->li_elem_count = size;
}

struct akl_list_entry *akl_get_frame_pointer(struct akl_context *ctx)
{
    return (ctx && ctx->cx_frame) ? AKL_LIST_FIRST(ctx->cx_frame) : NULL;
}

/* Attention: The stack contains pointer to value pointers */
void akl_stack_push(struct akl_context *ctx, struct akl_value *value)
{
    assert(ctx);
    akl_list_append_value(ctx->cx_state, ctx->cx_frame, value);
}

struct akl_value *akl_stack_shift(struct akl_context *ctx)
{
    return akl_list_shift(ctx->cx_frame);
}

struct akl_value *akl_stack_head(struct akl_context *ctx)
{
    assert(ctx);
    struct akl_list_entry *fe = AKL_LIST_FIRST(ctx->cx_frame);
    return (fe != NULL) ? (struct akl_value *)fe->le_data : NULL;
}

struct akl_value *akl_stack_pop(struct akl_context *ctx)
{
    assert(ctx);
    struct akl_list_entry *sp = akl_get_stack_pointer(ctx);
    akl_list_remove_entry(ctx->cx_frame, sp);
    return (sp != NULL) ? (struct akl_value *)sp->le_data : NULL;
}

void akl_stack_clear(struct akl_context *ctx, size_t c)
{
    while (c--) {
        (void)akl_stack_pop(ctx);
    }
}

struct akl_value *
akl_stack_top(struct akl_context *ctx)
{
    return (struct akl_value *)akl_get_stack_pointer(ctx)->le_data;
}

struct akl_lex_info *
akl_stack_top_lex_info(struct akl_context *ctx)
{
    return akl_stack_top(ctx)->va_lex_info;
}

/* These functions do not check the type of the stack top */
double akl_stack_pop_number(struct akl_context *ctx)
{
    struct akl_value *v = (ctx) ? akl_stack_pop(ctx) : NULL;
    return (v) ? v->va_value.number : 0.0;
}

char *akl_stack_pop_string(struct akl_context *ctx)
{
    struct akl_value *v = (ctx) ? akl_stack_pop(ctx) : NULL;
    return (v) ? v->va_value.string : NULL;
}

struct akl_list *akl_stack_pop_list(struct akl_context *ctx)
{
    struct akl_value *v = (ctx) ? akl_stack_pop(ctx) : NULL;
    return (v) ? v->va_value.list : NULL;
}

enum AKL_VALUE_TYPE akl_stack_top_type(struct akl_context *ctx)
{
    struct akl_value *v = (ctx) ? akl_stack_pop(ctx) : NULL;
    return (v) ? v->va_type : TYPE_NIL;
}

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

void akl_get_value_args(struct akl_context *ctx, int argc, ...)
{
    assert(ctx && ctx->cx_state && ctx->cx_frame);
    struct akl_value **vp;
    struct akl_list *frame = ctx->cx_frame;
    struct akl_list_entry *ent = AKL_LIST_FIRST(frame);
    va_list ap;
    if (argc <= 0)
        return;

    va_start(ap, argc);
    while (argc--) {
        assert(ent);
        vp = va_arg(ap, struct akl_value **);
        if (vp != NULL) {
            *vp = AKL_ENTRY_VALUE(ent);
        }
        ent = AKL_LIST_NEXT(ent);
    }
    va_end(ap);
}

void akl_get_args_strict(struct akl_context *ctx, int argc, ...)
{
    assert(ctx && ctx->cx_state && ctx->cx_frame);
    struct akl_value **vp;
    enum AKL_VALUE_TYPE t;
    struct akl_list *frame = ctx->cx_frame;
    struct akl_list_entry *ent = AKL_LIST_FIRST(frame);

    struct akl_atom **atom; struct akl_function **fun; struct akl_userdata **udata;
    struct akl_list **list; double *num; bool_t *b; char **str;

    va_list ap;
    if (argc <= 0)
        return;

    va_start(ap, argc);
    while (argc--) {
        assert(ent);
        t = va_arg(ap, enum AKL_VALUE_TYPE);
        *vp = AKL_ENTRY_VALUE(ent);
        /* The expected type must be the same with the current type,
            unless if that is a nil or a true */
        if ((t != TYPE_NIL || t != TYPE_TRUE) && t != (*vp)->va_type) {
            akl_add_error(ctx->cx_state, AKL_ERROR, (*vp)->va_lex_info
                  , "%s: Expected type '%s' but got type '%s'"
                  , ctx->cx_func_name, akl_type_name[t], akl_type_name[(*vp)->va_type]);
            ent = AKL_LIST_NEXT(ent);
            continue;
        }
        /* Set the caller's pointer to the right value (with the right type) */
        switch (t) {
            case TYPE_ATOM:
            atom = va_arg(ap, struct akl_atom **);
            *atom = AKL_GET_ATOM_VALUE(*vp);
            break;

            case TYPE_FUNCTION:
            fun = va_arg(ap, struct akl_function **);
            *fun = (*vp)->va_value.func;
            break;

            case TYPE_LIST:
            list = va_arg(ap, struct akl_list **);
            *list = AKL_GET_LIST_VALUE(*vp);
            break;

            case TYPE_USERDATA:
            udata = va_arg(ap, struct akl_userdata **);
            *udata = akl_get_userdata_value(*vp);
            break;

            case TYPE_NUMBER:
            num = va_arg(ap, double *);
            *num = AKL_GET_NUMBER_VALUE(*vp);
            break;

            case TYPE_STRING:
            str = va_arg(ap, char **);
            *str = AKL_GET_STRING_VALUE(*vp);
            break;

            case TYPE_NIL:
            b = va_arg(ap, bool_t *);
            *b = AKL_IS_NIL(*vp) ? TRUE : FALSE;
            break;

            case TYPE_TRUE:
            b = va_arg(ap, bool_t *);
            *b = AKL_IS_NIL(*vp) ? FALSE : TRUE;
            break;
        }
        ent = AKL_LIST_NEXT(ent);
    }
    va_end(ap);
}

void akl_call_function_bound(struct akl_context *ctx, struct akl_function *fn
                                         , struct akl_list *frame, char *fname)
{
    assert(fn);
    struct akl_context cx = *ctx;
    struct akl_ufun *ufun;
    cx.cx_func_name = fname;
    cx.cx_func = fn;
    cx.cx_frame = frame;

    switch (fn->fn_type) {
        case AKL_FUNC_CFUN:
        akl_stack_push(&cx, fn->fn_body.cfun(&cx, frame->li_elem_count));
        break;

        case AKL_FUNC_USER:
        ufun = &fn->fn_body.ufun;
        cx.cx_lex_info = ufun->uf_info;
        cx.cx_ir = &ufun->uf_body;
        akl_execute_ir(&cx);
        break;
    }

    /* Clean the stack, by invalidating it... */
    frame->li_head = akl_get_stack_pointer(&cx); /* Only the last elem is valid */
    frame->li_head->le_prev = NULL;
}

void akl_call_function(struct akl_context *ctx, struct akl_atom *atm, struct akl_list *frame)
{
    struct akl_value *v = atm->at_value;
    if (!AKL_CHECK_TYPE(v, TYPE_FUNCTION))
        return; /* TODO: Error */

    akl_call_function_bound(ctx, v->va_value.func, frame, atm->at_name);
}

#define MOVE_IP(ip) ((ip) = AKL_LIST_NEXT(ip))

void akl_ir_exec_branch(struct akl_context *ctx, struct akl_list_entry *ip)
{
    struct akl_list *ir = ctx->cx_ir;
    struct akl_list frame;
    struct akl_label *label;
    struct akl_ir_instruction *in;
    struct akl_value *v;
    struct akl_list_entry *fp = akl_get_frame_pointer(ctx);
    int argc;

    if (ir == NULL || ip == NULL)
        return;

    while (ip) {
        in = (struct akl_ir_instruction *)ip->le_data;
        switch (in->in_op) {
            case AKL_IR_NOP:
            MOVE_IP(ip);
            break;

            case AKL_IR_STORE:
            akl_stack_push(ctx, in->in_arg[0].value);
            MOVE_IP(ip);
            break;

            case AKL_IR_LOAD:
            /* TODO: Error if ui_num < 0 */
            v = akl_list_index_value(ctx->cx_frame, in->in_arg[0].ui_num);
            akl_stack_push(ctx, v);
            MOVE_IP(ip);
            break;

            case AKL_IR_CALL:
            akl_init_frame(ctx, &frame, in->in_arg[1].ui_num);
            akl_call_function(ctx, in->in_arg[0].atom, &frame);
            MOVE_IP(ip);
            break;

            case AKL_IR_JMP:
            label = in->in_arg[0].label;
            ir = label->la_ir;
            ip = label->la_branch;
            break;

            case AKL_IR_JT:
            label = in->in_arg[0].label;
            v = akl_stack_top(ctx);
            /* TODO: Error on other types */
            if (AKL_IS_TRUE(v)) {
                akl_stack_pop(ctx);
                ir = label->la_ir;
                ip = label->la_branch;
            }
            break;

            case AKL_IR_JN:
            label = in->in_arg[0].label;
            v = akl_stack_top(ctx);
            /* TODO: Error on other types */
            if (AKL_IS_NIL(v)) {
                akl_stack_pop(ctx);
                ir = label->la_ir;
                ip = label->la_branch;
            }
            break;
        }
    }
}

#define DUMP_JMP(jname, in) \
    printf("%s <%p>", jname  \
    , (in)->in_arg[0].label->la_branch);

void akl_dump_ir(struct akl_context *ctx)
{
    struct akl_list *ir = ctx->cx_ir;
    struct akl_list_entry *ent;
    struct akl_ir_instruction *in;
    struct akl_atom *atom;
    printf("--- Instruction Dump ---\n");

    AKL_LIST_FOREACH(ent, ir) {
       in = (struct akl_ir_instruction *)ent->le_data;
       if (in == NULL)
           break;
       printf("\t");
       switch (in->in_op) {
            case AKL_IR_NOP:
            printf("nop");
            break;

            case AKL_IR_CALL:
            atom = in->in_arg[0].atom;
            if (atom == NULL || atom->at_name == NULL)
                break;

            printf("call %s, %d", atom->at_name, in->in_arg[1].ui_num);
            break;

            case AKL_IR_JMP:
            DUMP_JMP("jmp", in);
            break;

            case AKL_IR_JN:
            DUMP_JMP("jn", in);
            break;

            case AKL_IR_JT:
            DUMP_JMP("jt", in);
            break;

            case AKL_IR_LOAD:
            printf("load %%%d", in->in_arg[0].ui_num);
            break;

            case AKL_IR_STORE:
            printf("store ");
            akl_print_value(ctx->cx_state, in->in_arg[0].value);
            break;
       }
       printf("\n");
    }
}

void akl_dump_stack(struct akl_context *ctx)
{
    struct akl_list *stack = ctx->cx_frame;
    struct akl_list_entry *ent;
    struct akl_value *value;
    unsigned int i = 0;

    printf("--- Stack Dump ---\n");
    AKL_LIST_FOREACH(ent, stack) {
       printf("\t");
       value = (ent != NULL) ? (struct akl_value *)ent->le_data : NULL;
       if (value == NULL)
           break;

       printf("%%%d - ", i);
       akl_print_value(ctx->cx_state, value);
       printf("\n");
       i++;
    }
}

void akl_execute_ir(struct akl_context *ctx)
{
    akl_ir_exec_branch(ctx, AKL_LIST_FIRST(ctx->cx_ir));
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
    char *msg = (char *)akl_alloc(in, new_size);
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
            err = (struct akl_error *)ent->le_data;
           akl_free(in, (void *)err->err_msg, 0);
           AKL_FREE(in, err);
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
            err = (struct akl_error *)ent->le_data;
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
