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

static void akl_ir_exec_branch(struct akl_context *, struct akl_list_entry *);

static void update_recent_value(struct akl_state *s, struct akl_value *val)
{
    static struct akl_variable *recent_var = NULL;
    if (!recent_var) {
        recent_var = akl_set_global_variable(s,  AKL_CSTR("$?")
            , AKL_CSTR("Previously returned value"), val);
        AKL_GC_SET_STATIC(recent_var);
    }
    /* Update '$?' with the recently used value */
    recent_var->vr_value = val;
}

/* ~~~===### Stack handling ###===~~~ */
unsigned int akl_frame_get_count(struct akl_context *ctx)
{
    assert(ctx);
    return akl_list_count(ctx->cx_frame);
}

bool_t akl_frame_is_empty(struct akl_context *ctx)
{
    assert(ctx);
    return akl_frame_get_count(ctx) == 0;
}

struct akl_value *akl_frame_at(struct akl_context *ctx, unsigned int ind)
{
    return akl_list_index_value(ctx->cx_frame, ind);
}

unsigned int akl_frame_get_pointer(struct akl_context *ctx)
{
    return akl_frame_get_count(ctx);
}

void akl_stack_clear(struct akl_context *ctx, size_t c)
{
#if 0
    void *ptr;
    do {
        ptr = akl_frame_pop(ctx);
    } while (c-- || ptr != NULL);
#endif 
    //akl_vector_truncate_by(ctx->cx_stack, c);
    // TODO: Evaulate GC on this
    akl_init_list(ctx->cx_stack);
}

void akl_frame_destroy(struct akl_context *cx, int argc)
{
    while (akl_frame_pop(cx))
        ;
}

/* Attention: The stack contains pointer to value pointers */
void akl_stack_push(struct akl_context *ctx, struct akl_value *value)
{
    AKL_ASSERT(ctx && value, AKL_NOTHING);
    akl_list_append_value(ctx->cx_state, ctx->cx_stack, value);
}

void akl_frame_push(struct akl_context *ctx, struct akl_value *value)
{
    AKL_ASSERT(ctx && ctx->cx_state && value, AKL_NOTHING);
    akl_stack_push(ctx, value);
    /* TODO */
}

struct akl_value *akl_frame_shift(struct akl_context *ctx)
{
    struct akl_list_entry *ent;
    if (ctx == NULL || akl_frame_get_count(ctx) == 0)
        return NULL;

    ent = akl_list_shift_entry(ctx->cx_frame);
    akl_list_remove_entry(ctx->cx_stack, ent);
    return AKL_ENTRY_VALUE(ent);
}

struct akl_value *akl_frame_head(struct akl_context *ctx)
{
    if (ctx == NULL)
        return NULL;
    return akl_list_head(ctx->cx_frame);
}

struct akl_value *akl_frame_pop(struct akl_context *ctx)
{
    struct akl_list_entry *ent;
    if (ctx == NULL || ctx->cx_state == NULL || akl_frame_get_count(ctx) == 0)
        return NULL;

    ent = akl_list_pop_entry(ctx->cx_frame);
    akl_list_remove_entry(ctx->cx_stack, ent);
    return AKL_ENTRY_VALUE(ent);
}

struct akl_value *akl_stack_head(struct akl_state *s)
{
    AKL_ASSERT(s, NULL);
    return akl_list_head(&s->ai_stack);
}

struct akl_value *akl_stack_top(struct akl_context *ctx)
{
    AKL_ASSERT(ctx && ctx->cx_state, NULL);
    return akl_list_last(ctx->cx_stack);
}

struct akl_value *akl_stack_pop(struct akl_context *ctx)
{

    struct akl_value *v = (struct akl_value *)akl_list_pop(ctx->cx_stack);
    if (v == NULL && ctx->cx_parent != NULL) {
        v = (struct akl_value *)akl_list_pop(ctx->cx_parent->cx_stack);
    }
    return v;
}

struct akl_value *
akl_frame_top(struct akl_context *ctx)
{
    AKL_ASSERT(ctx, NULL);
    return (struct akl_value *)akl_list_last(ctx->cx_frame);
}

/* These functions do not check the type of the stack top */
double *akl_frame_pop_number(struct akl_context *ctx)
{
    struct akl_value *v = akl_frame_pop(ctx);
    if (AKL_CHECK_TYPE(v, AKL_VT_NUMBER)) {
        return &v->va_value.number;
    }
    return NULL;
}

double *akl_frame_shift_number(struct akl_context *ctx)
{
    struct akl_value *v = akl_frame_shift(ctx);
    if (AKL_CHECK_TYPE(v, AKL_VT_NUMBER)) {
        return &v->va_value.number;
    }
    return NULL;
}

char *akl_frame_pop_string(struct akl_context *ctx)
{
    struct akl_value *v = akl_frame_pop(ctx);
    return AKL_GET_STRING_VALUE(v);
}

char *akl_frame_shift_string(struct akl_context *ctx)
{
    struct akl_value *v = akl_frame_shift(ctx);
    return AKL_GET_STRING_VALUE(v);
}

struct akl_list *
akl_frame_pop_list(struct akl_context *ctx)
{
    struct akl_value *v = akl_frame_pop(ctx);
    return AKL_GET_LIST_VALUE(v);
}

struct akl_list *
akl_frame_shift_list(struct akl_context *ctx)
{
    struct akl_value *v = akl_frame_shift(ctx);
    return AKL_GET_LIST_VALUE(v);
}

enum AKL_VALUE_TYPE akl_stack_top_type(struct akl_context *ctx)
{
    struct akl_value *v = akl_frame_pop(ctx);
    return (v) ? v->va_type : AKL_VT_NIL;
}

int akl_get_args(struct akl_context *ctx, int argc, ...)
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
        v = akl_frame_shift(ctx);
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

    struct akl_symbol **sym; struct akl_function **fun; struct akl_userdata **udata;
    struct akl_list **list; double *num; bool_t *b; char **str;

    if (argc != akl_frame_get_count(ctx)) {
        akl_raise_error(ctx, AKL_ERROR, "%s: Expected %d argument%s, but got %d"
              , ctx->cx_func_name, argc, (argc>1)?"s":"", akl_frame_get_count(ctx));
        return -1;
    }
    if (argc == 0)
        return -1;

    va_start(ap, argc);
    while (argc--) {
        t = va_arg(ap, enum AKL_VALUE_TYPE);
        vp = akl_frame_shift(ctx);
        if (vp == NULL) {
            akl_raise_error(ctx, AKL_ERROR
		, "Bug in the interpreter (NULL in stack) for the %d. argument", cnt);
            return -1;
        }
        /* The expected type must be the same with the current type,
            unless if that is a nil or a true */
        if ((t != AKL_VT_NIL || t != AKL_VT_TRUE) && t != vp->va_type) {
            ctx->cx_lex_info = vp->va_lex_info;
            akl_raise_error(ctx, AKL_ERROR, "%s: Expected %s but got %s"
                , ctx->cx_func_name, akl_type_name[t], akl_type_name[vp->va_type]);
            return -1;
        }
        /* Set the caller's pointer to the right value (with the right type) */
        switch (t) {
            case AKL_VT_SYMBOL:
            sym = va_arg(ap, struct akl_symbol **);
            *sym = vp->va_value.symbol;
            break;

            case AKL_VT_FUNCTION:
            fun = va_arg(ap, struct akl_function **);
            *fun = vp->va_value.func;
            break;

            case AKL_VT_LIST:
            list = va_arg(ap, struct akl_list **);
            *list = AKL_GET_LIST_VALUE(vp);
            break;

            case AKL_VT_USERDATA:
            udata = va_arg(ap, struct akl_userdata **);
            *udata = akl_get_userdata_value(vp);
            break;

            case AKL_VT_NUMBER:
            num = va_arg(ap, double *);
            *num = AKL_GET_NUMBER_VALUE(vp);
            break;

            case AKL_VT_STRING:
            str = va_arg(ap, char **);
            *str = AKL_GET_STRING_VALUE(vp);
            break;

            case AKL_VT_NIL:
            b = va_arg(ap, bool_t *);
            *b = AKL_IS_NIL(vp) ? TRUE : FALSE;
            break;

            case AKL_VT_TRUE:
            b = va_arg(ap, bool_t *);
            *b = AKL_IS_NIL(vp) ? FALSE : TRUE;
            break;
        }
    }
    va_end(ap);
    return 0;
}


struct akl_context *
akl_bound_function(struct akl_context *ctx, struct akl_symbol *sym
                   , struct akl_function *fn)
{
    struct akl_context *cx = akl_new_context(ctx->cx_state);
    struct akl_variable *v;

    if (cx == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "Cannot create context");
        return NULL;
    }

    if (sym == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "Function \'%s\' is not found", sym->sb_name);
        return NULL;
    }

    *(cx) = *(ctx);
    if (fn == NULL) {
        v = akl_get_global_var(ctx->cx_state, sym);
        if (v == NULL) {
            akl_raise_error(ctx, AKL_ERROR, "Variable \'%s\' is not a found.", sym->sb_name);
            return NULL;
        }

        if (!akl_var_is_function(v)) {
            akl_raise_error(ctx, AKL_ERROR, "Variable \'%s\' is not a function", sym->sb_name);
            return NULL;
        }
        fn = akl_var_to_function(v);
    }
    cx->cx_func = fn;
    cx->cx_func_name = sym->sb_name;
    cx->cx_parent = ctx;
    return cx;
}

/* XXX: Use this function with care. */
struct akl_value *akl_call_function_bound(struct akl_context *cx, int argc)
{
    assert(cx);
    struct akl_function *fn;
    struct akl_lisp_fun *ufun;
    struct akl_value *value;
    fn = cx->cx_func;

    akl_init_frame(cx, argc);

    switch (fn->fn_type) {
        case AKL_FUNC_CFUN:
        value = fn->fn_body.cfun(cx, argc);
        if (value == NULL) {
            akl_raise_error(cx, AKL_ERROR
                , "Function '%s' gave back NULL", cx->cx_func_name);
            akl_frame_destroy(cx, argc);
            return NULL;
        }
        akl_stack_push(cx, value);
        break;

        case AKL_FUNC_USER:
        ufun = &fn->fn_body.ufun;
        cx->cx_lex_info = ufun->uf_info;
        cx->cx_ir = &ufun->uf_body;
        akl_ir_exec_branch(cx, AKL_LIST_FIRST(&ufun->uf_body));
        value = akl_list_last(cx->cx_stack);
        //akl_stack_push(cx, value);
        break;

        /* TODO: */
        default:
        value = NULL;
        break;
    }
    akl_frame_destroy(cx, argc);

    return value;
}

struct akl_value *akl_call_symbol(struct akl_context *ctx, struct akl_context *cx
                    , struct akl_symbol *sym, int argc)
{
    assert(ctx && sym);
    if (cx == NULL) {
        cx = akl_bound_function(ctx, sym, NULL);
        if (cx == NULL) {
            return NULL;
        }
    }
    return akl_call_function_bound(cx, argc);
}

void
akl_call_sform(struct akl_context *ctx, struct akl_symbol *fsym
               , struct akl_function *sform)
{
    struct akl_context *cx = akl_bound_function(ctx, fsym, sform);
    cx->cx_func->fn_body.scfun(cx);
}

struct akl_value *
akl_call_function(struct akl_context *ctx, struct akl_context *cx
                            , const char *fname, int argc)
{
    assert(ctx && fname);
    struct akl_symbol *sym = akl_get_symbol(ctx->cx_state, (char *)fname);
    return akl_call_symbol(ctx, cx, sym, argc);
}

#define MOVE_IP(ip) ((ip) = AKL_LIST_NEXT(ip))
#define LOOP_WATCHDOG(ent) \
        if ((ent) == (ent)->le_next) { \
            fprintf(stderr, "Error, loop in %s()!\n", __func__); \
            return;\
        }
#define OPERAND(ind, name) (in)->in_arg[ind].name

static void
akl_ir_exec_branch(struct akl_context *ctx, struct akl_list_entry *ip)
{
    struct akl_list  *ir = ctx->cx_ir;
    struct akl_state *s  = ctx->cx_state;

    struct akl_label *lt = NULL, *ln = NULL;
    struct akl_context *cx = NULL;
    struct akl_ir_instruction *in;
    struct akl_value *v, *lv;
    struct akl_variable *var;
    struct akl_symbol *sym;

    if (ir == NULL || ip == NULL)
        return;

    if (s && s->ai_interrupted) {
        akl_raise_error(ctx, AKL_WARNING, "Program interruption.");
        return;
    }

    while (ip) {
        in = (struct akl_ir_instruction *)ip->le_data;
        LOOP_WATCHDOG(ip);
        switch (in->in_op) {
        case AKL_IR_NOP:
            MOVE_IP(ip);
        break;

        case AKL_IR_SET:
            v = akl_stack_pop(ctx);
            if (v != NULL) {
                ctx->cx_lex_info = v->va_lex_info;
                akl_set_global_var(s, OPERAND(0, symbol), NULL, TRUE, v);
            }
            MOVE_IP(ip);
        break;

        case AKL_IR_GET:
            sym = OPERAND(0, symbol);
            var = akl_get_global_var(s, sym);
            if (!var) {
                akl_raise_error(ctx, AKL_ERROR, "Variable '%s' is undefined.", sym->sb_name);
                akl_stack_push(ctx, akl_new_nil_value(s));
            } else {
                ctx->cx_lex_info = var->vr_lex_info;
                akl_stack_push(ctx, var->vr_value);
            }
            MOVE_IP(ip);
        break;

        case AKL_IR_PUSH:
            v = OPERAND(0, value);
            if (v == NULL) {
                akl_raise_error(ctx, AKL_WARNING, "Interpreter error: NULL pushed to stack.");
                return;
            }
            ctx->cx_lex_info = v->va_lex_info;
            akl_stack_push(ctx, v);
            MOVE_IP(ip);
        break;

        case AKL_IR_LOAD:
            /* TODO: Error if ui_num < 0 */
            v = akl_frame_at(ctx, OPERAND(0, ui_num));
            if (v) {
                ctx->cx_lex_info = v->va_lex_info;
                akl_stack_push(ctx, v);
            }
            MOVE_IP(ip);
        break;

        case AKL_IR_CALL:
            ctx->cx_lex_info = in->in_linfo;
            if (in->in_fun) {
                cx = akl_bound_function(ctx, OPERAND(0, symbol), in->in_fun);
                if (cx == NULL) {
                    MOVE_IP(ip);
                    continue;
                }
                akl_call_function_bound(cx, OPERAND(1, ui_num));
            } else {
                akl_call_symbol(ctx, NULL, OPERAND(0, symbol), OPERAND(1, ui_num));
            }
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
            v = akl_frame_at(ctx, OPERAND(0, ui_num));
            if (v) {
                lv = akl_new_list_value(ctx->cx_state
                        , akl_cdr(ctx->cx_state, AKL_GET_LIST_VALUE(v)));
                akl_stack_push(ctx, lv);
            }
            MOVE_IP(ip);
        break;

        case AKL_IR_JMP:
            lt = OPERAND(0, label);
            ir = lt->la_ir;
            ip = lt->la_branch;
        break;

        case AKL_IR_JT:
            lt = OPERAND(0, label);
            v = akl_stack_pop(ctx);
            /* TODO: Error on other types */
            if (AKL_IS_TRUE(v)) {
                ir = lt->la_ir;
                ip = lt->la_branch;
            } else {
                MOVE_IP(ip);
            }
        break;

        case AKL_IR_JN:
            ln = OPERAND(0, label);
            v = akl_stack_pop(ctx);
            /* TODO: Error on other types */
            if (AKL_IS_NIL(v)) {
                ir = ln->la_ir;
                ip = ln->la_branch;
            } else {
                MOVE_IP(ip);
            }
        break;

        case AKL_IR_BRANCH:
            lt = OPERAND(0, label);
            ln = OPERAND(0, label);
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

        case AKL_IR_RET:
            /* TODO */
            MOVE_IP(ip);
        break;

        default:
            akl_raise_error(ctx, AKL_ERROR, "Unkown instruction '%#x'", in->in_op);
        return;
        }
    }
}

static void
dump_jmp(struct akl_state *s, const char *jname, struct akl_ir_instruction *in)
{
    if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) {
        printf("%s%s %s.L%d%s", AKL_BLUE, jname, AKL_YELLOW
        ,OPERAND(0, label)->la_ind, AKL_END_COLOR_MARK);
    } else {
        printf("%s .L%d", jname, OPERAND(0, label)->la_ind);
    }
}

void akl_dump_ir(struct akl_context *ctx, struct akl_function *fun)
{
    assert(ctx);
    struct akl_list *ir;
    struct akl_list_entry *ent, *lit;
    struct akl_ir_instruction *in;
    struct akl_label *l = NULL;
    struct akl_lisp_fun *uf = NULL;
    struct akl_symbol *sym;
    struct akl_state *s = ctx->cx_state;
    int lind = 0;

    if (fun->fn_type == AKL_FUNC_CFUN 
     || fun->fn_type == AKL_FUNC_SPECIAL) {
        printf("Compiled function\n");
        return;
    }
    uf = &fun->fn_body.ufun;
    l = (struct akl_label *)akl_list_head(&uf->uf_labels);

    ir = &uf->uf_body;
    AKL_LIST_FOREACH(ent, ir) {
       in = (struct akl_ir_instruction *)ent->le_data;
       if (in == NULL)
           break;

       LOOP_WATCHDOG(ent);
       /* If this instruction is labeled, print that label first */
       lit = akl_list_it_begin(&uf->uf_labels);
       /* Itarate through the label's list, to find a label with
          the same address. */
       while (akl_list_it_has_next(lit)) {
           l = (struct akl_label *)akl_list_it_next(&lit);
           if (l && l->la_branch == ent) {
               printf("%s.L%d:%s\n", AKL_COLORFUL(s, AKL_YELLOW)
                                   , l->la_ind, AKL_END_COLORFUL(s));
               lind++;
           }
       }

       printf("\t");
       switch (in->in_op) {
            case AKL_IR_NOP:
            printf("%snop%s", AKL_COLORFUL(s, AKL_BLUE), AKL_END_COLORFUL(s));
            break;

            case AKL_IR_CALL:
            sym = OPERAND(0, symbol);
            if (sym == NULL || sym->sb_name == NULL)
                break;

            if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) {
                printf("%scall %s%s%s, %s%d%s", AKL_BLUE, AKL_PURPLE, sym->sb_name
                       , AKL_END_COLOR_MARK, AKL_YELLOW, OPERAND(1, ui_num), AKL_END_COLOR_MARK);
            } else {
                printf("call %s, %d", sym->sb_name, OPERAND(1, ui_num));
            }
            break;

            case AKL_IR_JMP:
            dump_jmp(s, "jmp", in);
            break;

            case AKL_IR_JN:
            dump_jmp(s, "jn", in);
            break;

            case AKL_IR_JT:
            dump_jmp(s, "jt", in);
            break;

            case AKL_IR_BRANCH:
            if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) {
                printf("%sbr %s.L%d%s, %s.L%d%s", AKL_BLUE, AKL_YELLOW, OPERAND(0, label)->la_ind
                           , AKL_END_COLOR_MARK, AKL_YELLOW, OPERAND(1, label)->la_ind, AKL_END_COLOR_MARK);
            } else {
                printf("br .L%d, .L%d", OPERAND(0, label)->la_ind, OPERAND(1, label)->la_ind);
            }
            break;

            case AKL_IR_LOAD:
            if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) {
                printf("%sload %s%%%d%s", AKL_BLUE, AKL_BRIGHT_YELLOW
                           , OPERAND(0, ui_num), AKL_END_COLOR_MARK);
            } else {
                printf("load %%%d", OPERAND(0, ui_num));
            }
            break;

            case AKL_IR_SET:
            if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) {
                printf("%sset %s%s%s", AKL_BLUE, AKL_PURPLE
                           , OPERAND(0, symbol)->sb_name, AKL_END_COLOR_MARK);
            } else {
                printf("set %s", OPERAND(0, symbol)->sb_name);
            }
            break;

            case AKL_IR_GET:
            if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) {
                printf("%sget %s%s%s", AKL_BLUE, AKL_PURPLE
                           , OPERAND(0, symbol)->sb_name, AKL_END_COLOR_MARK);
            } else {
                printf("get %s", OPERAND(0, symbol)->sb_name);
            }
            break;

            case AKL_IR_PUSH:
            if (AKL_IS_FEATURE_ON(s, AKL_CFG_USE_COLORS)) {
                printf("%spush%s ", AKL_BLUE, AKL_END_COLOR_MARK);
            } else {
                printf("push ");
            }
            akl_print_value(ctx->cx_state, OPERAND(0, value));
            break;

            case AKL_IR_HEAD:
            printf("head %d", OPERAND(0, ui_num));
            break;

            case AKL_IR_TAIL:
            printf("tail %d", OPERAND(0, ui_num));
            break;

            case AKL_IR_RET:
            printf("ret");
            break;

            default:
            akl_raise_error(ctx, AKL_ERROR, "Unknown instruction '%x'", in->in_op);
            break;
       }
       printf("\n");
    }
}

void akl_clear_ir(struct akl_context *ctx)
{
    if (!ctx || !ctx->cx_ir)
        return;

    while (akl_list_count(ctx->cx_ir) != 0)
        akl_list_pop(ctx->cx_ir);
}

void akl_dump_stack(struct akl_context *ctx)
{
    struct akl_list *stack = ctx->cx_stack;
    struct akl_value *value = NULL;
    struct akl_list_entry *ent = akl_list_it_end(stack);
    int i = 0;

    printf("--- Stack Dump ---\n");
    while (ent) {
       printf("\t%s%%%d%s - ", AKL_COLORFUL(ctx->cx_state, AKL_BRIGHT_YELLOW)
                           , i, AKL_END_COLORFUL(ctx->cx_state));
       value = AKL_ENTRY_VALUE(ent);
       akl_print_value(ctx->cx_state, value);
       akl_list_it_prev(&ent);
       printf("\n");
       i++;
    }
}

void akl_execute_ir(struct akl_context *ctx)
{
    akl_ir_exec_branch(ctx, AKL_LIST_FIRST(ctx->cx_ir));
}

void akl_execute(struct akl_context *ctx)
{
    AKL_ASSERT(ctx && ctx->cx_state && ctx->cx_state->ai_fn_main, AKL_NOTHING);
    struct akl_function *mf = ctx->cx_state->ai_fn_main;
    struct akl_lisp_fun *mfir = &mf->fn_body.ufun;
    ctx->cx_state->ai_interrupted = FALSE;
    //struct akl_value *v = akl_get_global_value(ctx->cx_state, "*args*");
    //ctx->cx_stack = &ctx->cx_state->ai_stack;
    //akl_frame_push(ctx,  AKL_NULLER(v));
    ctx->cx_stack = akl_new_list(ctx->cx_state);
    akl_ir_exec_branch(ctx, AKL_LIST_FIRST(&mfir->uf_body));
}

struct akl_value *
akl_exec_eval(struct akl_state *s)
{
    struct akl_context *ctx = akl_compile(s, s->ai_device);
    akl_execute(ctx);
    akl_print_errors(s);
    return akl_stack_pop(&s->ai_context);
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
    char *a, *b;
    struct akl_value *v1 = (struct akl_value *)c1;
    struct akl_value *v2 = (struct akl_value *)c2;
    if (v1->va_type == v2->va_type) {
        switch (v1->va_type) {
            case AKL_VT_NUMBER:
            return compare_numbers(AKL_GET_NUMBER_VALUE(v1)
                                   , AKL_GET_NUMBER_VALUE(v2));

            case AKL_VT_STRING:
            a = AKL_GET_STRING_VALUE(v1);
            b = AKL_GET_STRING_VALUE(v2);
            if (a == NULL || b == NULL)
                return -1;
            return strcmp(a, b);

            case AKL_VT_SYMBOL:
            /* Symbols only differ by their pointers */
            return compare_numbers((unsigned long)v1->va_value.symbol
                                   , (unsigned long)v2->va_value.symbol);

            case AKL_VT_USERDATA:
            /* TODO: userdata compare function */
            return compare_numbers(akl_get_utype_value(v1)
                                   , akl_get_utype_value(v2));

            case AKL_VT_NIL:
            return 0;

            case AKL_VT_TRUE:
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
    struct akl_error *err;
    int n;
    char *msg = NULL;
    if (fmt == NULL) {
        akl_raise_error(ctx, AKL_ERROR, "NULL given for akl_raise_error().");
        return;
    }
    va_start(ap, fmt);
    n = vasprintf(&msg, fmt, ap);
    va_end(ap);
    if (n < 0) {
        /* TODO: Somehow raise error from akl_raise_error() */
        return;
    }
    if (msg == NULL) {
        /* ERROR */
        return;
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
        in->ai_errors->li_count = 0;
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

                if (AKL_IS_FEATURE_ON(in, AKL_CFG_USE_COLORS)) {
                    fprintf(stderr,  AKL_GREEN "%s:%d:%d" AKL_END_COLOR_MARK 
			    ": %s%s\n" AKL_END_COLOR_MARK
                            , name, line, count, (err->err_type == AKL_ERROR) 
		            ? AKL_RED : AKL_YELLOW, err->err_msg);
                } else {
                    fprintf(stderr, "%s:%d:%d: %s\n", name, line, count, err->err_msg);
                }
            }
            errors++;
        }
        if (!AKL_IS_FEATURE_ON(in, AKL_CFG_INTERACTIVE))
            fprintf(stderr, "%d error report generated.\n", errors);
    }
}
