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

/* ~~~===### Stack handling ###===~~~ */
void akl_init_frame(struct akl_context *ctx, struct akl_frame *f)
{
    assert(ctx && f);
    f->af_stack = &ctx->cx_state->ai_stack;
    f->af_begin = akl_vector_count(&ctx->cx_state->ai_stack) + 1;
    f->af_end = f->af_begin;
    f->af_parent = &ctx->cx_frame;
    f->af_abs_begin = f->af_begin;
    f->af_abs_count = f->af_begin - f->af_end;
}

bool_t akl_frame_is_empty(struct akl_context *ctx)
{
    assert(ctx);
    return ctx->cx_frame.af_begin == ctx->cx_frame.af_end;
}

unsigned int akl_frame_get_count(struct akl_context *ctx)
{
    assert(ctx);
    return ctx->cx_frame.af_end - ctx->cx_frame.af_begin;
}

struct akl_value *akl_frame_at(struct akl_context *ctx, unsigned int ind)
{
    struct akl_frame *f = &ctx->cx_frame;
    if (f->af_abs_count >= ind) {
        akl_raise_error(ctx, AKL_ERROR, "Cannot get %%d, frame is smaller", ind);
        return NULL;
    }
    return akl_vector_at(f->af_stack, f->af_abs_begin + ind);
}

unsigned int akl_frame_get_pointer(struct akl_context *ctx)
{
    assert(ctx);
    return ctx->cx_frame.af_begin;
}

void akl_stack_clear(struct akl_context *ctx, size_t c)
{
    assert(ctx);
    struct akl_frame *st = &ctx->cx_frame;
    while (st->af_begin != st->af_end) {
        akl_stack_pop(ctx);
    }
}

void akl_frame_destroy(struct akl_context *ctx)
{
    assert(ctx);
    akl_stack_clear(ctx, akl_frame_get_count(ctx));
}

unsigned int akl_stack_get_pointer(struct akl_context *ctx)
{
    assert(ctx);
    return ctx->cx_frame.af_end;
}

/* Attention: The stack contains pointer to value pointers */
void akl_stack_push(struct akl_context *ctx, struct akl_value *value)
{
    assert(ctx);
    struct akl_frame *st = &ctx->cx_frame;
    struct akl_value *vp = (struct akl_value *)akl_vector_reserve(st->af_stack);
    *vp = *value;
    st->af_end++;
}

struct akl_value *akl_stack_shift(struct akl_context *ctx)
{
    assert(ctx);
    struct akl_frame *st = &ctx->cx_frame;
    if (st->af_begin != st->af_end) {
        st->af_begin++;
        return (struct akl_value *)akl_vector_at(st->af_stack, st->af_begin);
    } else {
        return NULL;
    }
}

struct akl_value *akl_stack_head(struct akl_context *ctx)
{
    assert(ctx);
    struct akl_frame *st = &ctx->cx_frame;
    if (st->af_begin != st->af_end)
        return (struct akl_value *)akl_vector_at(st->af_stack, st->af_begin);
    else
        return NULL;
}

struct akl_value *akl_stack_pop(struct akl_context *ctx)
{
    assert(ctx);
    struct akl_frame *st = &ctx->cx_frame;
    if (st->af_begin != st->af_end) {
        st->af_end--;
        return akl_vector_pop(st->af_stack);
    } else {
        return NULL;
    }
}

struct akl_value *
akl_stack_top(struct akl_context *ctx)
{
    assert(ctx);
    struct akl_frame *st = &ctx->cx_frame;
    if (st->af_begin != st->af_end)
        return akl_vector_at(st->af_stack, st->af_end);
    else
        return NULL;
}

/* These functions do not check the type of the stack top */
double *akl_stack_pop_number(struct akl_context *ctx)
{
    struct akl_value *v = akl_stack_pop(ctx);
    if (AKL_CHECK_TYPE(v, TYPE_NUMBER)) {
        return &v->va_value.number;
    }
    return NULL;
}

char *akl_stack_pop_string(struct akl_context *ctx)
{
    struct akl_value *v = akl_stack_pop(ctx);
    return AKL_GET_STRING_VALUE(v);
}

struct akl_list *akl_stack_pop_list(struct akl_context *ctx)
{
    struct akl_value *v = akl_stack_pop(ctx);
    return AKL_GET_LIST_VALUE(v);
}

enum AKL_VALUE_TYPE akl_stack_top_type(struct akl_context *ctx)
{
    struct akl_value *v = akl_stack_pop(ctx);
    return (v) ? v->va_type : TYPE_NIL;
}

int akl_get_value_args(struct akl_context *ctx, int argc, ...)
{
    assert(ctx && ctx->cx_state);
    va_list ap;
    struct akl_value **vp, *v;
    int ac = argc;
    int cnt = 1;
    if (argc != akl_frame_get_count(ctx)) {
        akl_raise_error(ctx, AKL_ERROR, "%s: Expected %d argument%s, but got %d"
              , ctx->cx_func_name, argc, (argc>1)?"s":"", akl_frame_get_count(ctx));
        return -1;
    }

    va_start(ap, argc);
    while (ac--) {
        vp = va_arg(ap, struct akl_value **);
        v = akl_stack_shift(ctx);
        if (v == NULL) {
            akl_raise_error(ctx, AKL_ERROR
                        , "Bug in the interpreter (NULL in stack) at the %d. argument", cnt);
            return -1;
        }
        if (vp != NULL)
            *vp = v;
        cnt++;
    }

exit:
    va_end(ap);
    return 0;
}

int akl_get_args_strict(struct akl_context *ctx, int argc, ...)
{
    assert(ctx && ctx->cx_state);
    struct akl_value *vp;
    enum AKL_VALUE_TYPE t;
    va_list ap;
    int cnt = 1;

    struct akl_atom **atom; struct akl_function **fun; struct akl_userdata **udata;
    struct akl_list **list; double *num; bool_t *b; char **str;

    if (argc != akl_frame_get_count(ctx)) {
        akl_raise_error(ctx, AKL_ERROR, "%s: Expected %d argument%s, but got %d"
              , ctx->cx_func_name, argc, (argc>1)?"s":"", akl_frame_get_count(ctx));
        return -1;
    }
    if (argc == 0)
        return 0;

    va_start(ap, argc);
    while (argc--) {
        t = va_arg(ap, enum AKL_VALUE_TYPE);
        vp = akl_stack_shift(ctx);
        if (vp == NULL) {
            akl_raise_error(ctx, AKL_ERROR, "Bug in the interpreter (NULL in stack) for the %d. argument"
                            , cnt);
            return -1;
        }
        /* The expected type must be the same with the current type,
            unless if that is a nil or a true */
        if ((t != TYPE_NIL || t != TYPE_TRUE) && t != vp->va_type) {
            ctx->cx_lex_info = vp->va_lex_info;
            akl_raise_error(ctx, AKL_ERROR, "%s: Expected type '%s' but got type '%s'"
                  , ctx->cx_func_name, akl_type_name[t], akl_type_name[vp->va_type]);
            return -1;
        }
        /* Set the caller's pointer to the right value (with the right type) */
        switch (t) {
            case TYPE_ATOM:
            atom = va_arg(ap, struct akl_atom **);
            *atom = AKL_GET_ATOM_VALUE(vp);
            break;

            case TYPE_FUNCTION:
            fun = va_arg(ap, struct akl_function **);
            *fun = vp->va_value.func;
            break;

            case TYPE_LIST:
            list = va_arg(ap, struct akl_list **);
            *list = AKL_GET_LIST_VALUE(vp);
            break;

            case TYPE_USERDATA:
            udata = va_arg(ap, struct akl_userdata **);
            *udata = akl_get_userdata_value(vp);
            break;

            case TYPE_NUMBER:
            num = va_arg(ap, double *);
            *num = AKL_GET_NUMBER_VALUE(vp);
            break;

            case TYPE_STRING:
            str = va_arg(ap, char **);
            *str = AKL_GET_STRING_VALUE(vp);
            break;

            case TYPE_NIL:
            b = va_arg(ap, bool_t *);
            *b = AKL_IS_NIL(vp) ? TRUE : FALSE;
            break;

            case TYPE_TRUE:
            b = va_arg(ap, bool_t *);
            *b = AKL_IS_NIL(vp) ? FALSE : TRUE;
            break;
        }
    }
    va_end(ap);
}

/* Inherit context (ctx) if no one give valid (if cx == NULL).
 * Should be only used in akl_call_* functions
 * NOTICE: Will create an lc local variable
*/
#define CTX_USE_OR_CREATE(ctx, cx)   \
    struct akl_context lc;            \
    if (cx == NULL) {                  \
        cx = &lc;                       \
        akl_init_context(cx);            \
        lc = *ctx;                        \
        akl_init_frame(ctx, &cx->cx_frame);\
    }

struct akl_value *akl_call_function_bound(struct akl_context *cx)
{
    assert(cx);
    struct akl_function *fn;
    struct akl_ufun *ufun;
    struct akl_value *value;
    fn = cx->cx_func;

    switch (fn->fn_type) {
        case AKL_FUNC_CFUN:
        value = fn->fn_body.cfun(cx, akl_frame_get_count(cx));
        break;

        case AKL_FUNC_USER:
        ufun = &fn->fn_body.ufun;
        cx->cx_lex_info = ufun->uf_info;
        cx->cx_ir = &ufun->uf_body;
        akl_execute_ir(cx);
        value = akl_stack_head(cx);
        break;
    }

    akl_frame_destroy(cx);
    return value;
}

struct akl_value *akl_call_atom(struct akl_context *ctx, struct akl_context *cx, struct akl_atom *atm)
{
    assert(ctx && atm && atm->at_name);
    struct akl_value *v;
    CTX_USE_OR_CREATE(ctx, cx);
    if (atm == NULL || atm->at_name == NULL || atm->at_value == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "Function \'%s\' is not found", atm->at_name);
        return NULL;
    }

    cx->cx_func_name = atm->at_name;
    v = atm->at_value;
    if (!akl_atom_is_function(atm) || v == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "Atom \'%s\' is not a function", atm->at_name);
        return NULL;
    }

    cx->cx_func = v->va_value.func;
    cx->cx_lex_info = v->va_lex_info;

    return akl_call_function_bound(cx);
}

struct akl_value *akl_call_function(struct akl_context *ctx, struct akl_context *cx, const char *fname)
{
    assert(ctx && fname);
    struct akl_atom *atm = akl_get_global_atom(cx->cx_state, fname);
    CTX_USE_OR_CREATE(ctx, cx);
    if (atm == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "Function \'%s\' is not a function", fname);
        return NULL;
    }
    return akl_call_atom(ctx, cx, atm);
}

#define MOVE_IP(ip) ((ip) = AKL_LIST_NEXT(ip))
#define LOOP_WATCHDOG(ent) \
        if ((ent) == (ent)->le_next) { \
            fprintf(stderr, "Error, loop in %s()!\n", __func__); \
            return;\
        } \

void akl_ir_exec_branch(struct akl_context *ctx, struct akl_list_entry *ip)
{
    struct akl_list *ir = ctx->cx_ir;
    struct akl_label *lt, *ln;
    struct akl_ir_instruction *in;
    struct akl_value *v, *lv;
    struct akl_atom *atm;
    struct akl_frame frame;

    if (ir == NULL || ip == NULL)
        return;

    while (ip) {
        in = (struct akl_ir_instruction *)ip->le_data;
        LOOP_WATCHDOG(ip);
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
            v = akl_frame_at(ctx, in->in_arg[0].ui_num);
            if (v)
                akl_stack_push(ctx, v);
            MOVE_IP(ip);
            break;

            case AKL_IR_CALL:
            akl_call_atom(ctx, NULL, in->in_arg[0].atom);
            MOVE_IP(ip);
            break;

            case AKL_IR_HEAD:
            v = akl_frame_at(ctx, in->in_arg[0].ui_num);
            if (v) {
                akl_stack_push(ctx, akl_car(AKL_GET_LIST_VALUE(v)));
            }
            MOVE_IP(ip);
            break;

            case AKL_IR_TAIL:
            v = akl_frame_at(ctx, in->in_arg[0].ui_num);
            if (v) {
                lv = akl_new_list_value(ctx->cx_state
                        , akl_cdr(ctx->cx_state, AKL_GET_LIST_VALUE(v)));
                akl_stack_push(ctx, lv);
            }
            MOVE_IP(ip);
            break;

            case AKL_IR_JMP:
            lt = in->in_arg[0].label;
            ir = lt->la_ir;
            ip = lt->la_branch;
            break;

            case AKL_IR_JT:
            lt = in->in_arg[0].label;
            v = akl_stack_top(ctx);
            /* TODO: Error on other types */
            if (AKL_IS_TRUE(v)) {
                akl_stack_pop(ctx);
                ir = lt->la_ir;
                ip = lt->la_branch;
            }
            break;

            case AKL_IR_JN:
            ln = in->in_arg[0].label;
            v = akl_stack_top(ctx);
            /* TODO: Error on other types */
            if (AKL_IS_NIL(v)) {
                akl_stack_pop(ctx);
                ir = ln->la_ir;
                ip = ln->la_branch;
            }
            break;

            case AKL_IR_BRANCH:
            lt = in->in_arg[0].label;
            ln = in->in_arg[1].label;
            v = akl_stack_pop(ctx);
            /* TODO: Error on other types */
            if (AKL_IS_NIL(v)) {
                ir = ln->la_ir;
                ip = ln->la_branch;
            } else {
                ir = lt->la_ir;
                ip = lt->la_branch;
            }
            break;
        }
    }
}

#define DUMP_JMP(jname, in) \
    printf("%s .L%d", jname  \
    , (in)->in_arg[0].label->la_ind);

void akl_dump_ir(struct akl_context *ctx, struct akl_function *fun)
{
    struct akl_list *ir;
    struct akl_list_entry *ent;
    struct akl_ir_instruction *in;
    struct akl_atom *atom;
    struct akl_label *l;
    struct akl_ufun *uf;
    int lind = 0;

    switch (fun->fn_type) {
    case AKL_FUNC_CFUN: case AKL_FUNC_SPECIAL:
        printf("Compiled function\n");
        return;
    }
    uf = &fun->fn_body.ufun;
    l = (struct akl_label *)akl_vector_first(uf->uf_labels);
    ir = &uf->uf_body;
    assert(l && ir);

    AKL_LIST_FOREACH(ent, ir) {
       in = (struct akl_ir_instruction *)ent->le_data;
       if (in == NULL)
           break;

       LOOP_WATCHDOG(ent);
       if (l && lind < akl_vector_count(uf->uf_labels)) {
           l = (struct akl_label *)akl_vector_at(uf->uf_labels, lind);
           if (l && l->la_branch == ent) {
               printf(".L%d:\n", l->la_ind);
               lind++;
               continue;
           }
       }

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

            case AKL_IR_BRANCH:
            printf("br .L%d, .L%d", in->in_arg[0].label->la_ind, in->in_arg[1].label->la_ind);
            break;

            case AKL_IR_LOAD:
            printf("load %%%d", in->in_arg[0].ui_num);
            break;

            case AKL_IR_STORE:
            printf("store ");
            akl_print_value(ctx->cx_state, in->in_arg[0].value);
            break;

            case AKL_IR_HEAD:
            printf("head %%d", in->in_arg[0].ui_num);
            break;

            case AKL_IR_TAIL:
            printf("tail %%d", in->in_arg[0].ui_num);
            break;
       }
       printf("\n");
    }
}

void akl_dump_stack(struct akl_context *ctx)
{
#if 0
    struct akl_frame *stack = ctx->cx_frame;
    struct akl_list_entry *ent;
    struct akl_value *value;
    unsigned int i = 0;

    printf("--- Stack Dump ---\n");
    AKL_LIST_FOREACH(ent, stack) {
       printf("\t");
       value = (ent != NULL) ? (struct akl_value *)ent->le_data : NULL;
       LOOP_WATCHDOG(ent);
       if (value == NULL)
           break;

       printf("%%%d - ", i);
       akl_print_value(ctx->cx_state, value);
       printf("\n");
       i++;
    }
#endif
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

void akl_raise_error(struct akl_context *ctx
               , enum AKL_ALERT_TYPE type, const char *fmt, ...)
{
    va_list ap;
    struct akl_state *s = ctx->cx_state;
    struct akl_list *l;
    struct akl_error *err;
    size_t fmt_size = strlen(fmt);
    /* should be enough */
    size_t new_size = fmt_size + (fmt_size/2);
    int n;
    char *np;
    char *msg = (char *)akl_alloc(s, new_size);
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

    if (s) {
        if (s->ai_errors == NULL) {
            s->ai_errors = akl_new_list(s);
        }
        err = AKL_MALLOC(s, struct akl_error);
        err->err_info = ctx->cx_lex_info;
        err->err_type = type;
        err->err_msg = msg;
        akl_list_append(s, s->ai_errors, (void *)err);
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

                fprintf(stderr,  AKL_GREEN "%s:%d:%d" AKL_END_COLOR_MARK ": %s%s" AKL_END_COLOR_MARK
                        , name, line, count, (err->err_type == AKL_ERROR) ? AKL_RED : AKL_YELLOW, err->err_msg);
            }
            errors++;
        }
        if (!in->ai_interactive)
            fprintf(stderr, "%d error report generated.\n", errors);
    }
}
