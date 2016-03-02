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
#include <stdint.h>

static void
akl_ir_set_lex_info(struct akl_context *ctx, struct akl_lex_info *info)
{
    AKL_ASSERT(ctx && ctx->cx_ir, AKL_NOTHING);
    struct akl_ir_instruction *li =
            (struct akl_ir_instruction *)akl_list_last(ctx->cx_ir);
    if (li && info) {
        li->in_linfo = info;
    }
}

static int
compare_symbols(void *f, void *s)
{
    return akl_rb_cmp_sym(f, s);
}

int
argument_finder(struct akl_lisp_fun *fn, struct akl_symbol *sym)
{
    int i;
    akl_vector_find(&fn->uf_args, compare_symbols, sym, &i);
    return i;
}

static struct akl_ir_instruction *
new_instr(struct akl_context *ctx)
{
    struct akl_state *s = ctx->cx_state;
    struct akl_list *ir = ctx->cx_ir;
    struct akl_ir_instruction *nop = AKL_MALLOC(s, struct akl_ir_instruction);
    nop->in_op    = AKL_IR_NOP;
    nop->in_linfo = NULL;
    nop->in_fun   = NULL;
    return nop;
}

static struct akl_ir_instruction *
create_or_get_instr(struct akl_context *ctx)
{
    struct akl_state *s = ctx->cx_state;
    struct akl_list *ir = ctx->cx_ir;
    struct akl_ir_instruction *instr = NULL;

    /* This is the very first instruction. We have to add the this to the IR
     * and then a NOP also. The next call will modify this operation, so
     * we will always know the address of the next operation for reference. */
    if (akl_list_is_empty(ir)) {
        instr = new_instr(ctx);
        /* Add the new instruction as the first one. */
        akl_list_append(s, ir, instr);
    } else {
        /* If we already had a NOP, then just modify that accordingly.
         * This always has to be the last one.
        */
        instr = (struct akl_ir_instruction *)akl_list_last(ir);
    }
    return instr;
}

static struct akl_ir_instruction *
create_instr(struct akl_context *ctx)
{
    struct akl_ir_instruction *instr, *nop;
    instr = create_or_get_instr(ctx);
    /* The new NOP will be always the last in the list. The next new_instr()
     * call will have to use that as the new operation. */
    nop = new_instr(ctx);
    akl_list_append(ctx->cx_state, ctx->cx_ir, nop);
    return instr;
}

void akl_build_push(struct akl_context *ctx, struct akl_value *arg)
{
    struct akl_ir_instruction *push = create_instr(ctx);
    push->in_op           = AKL_IR_PUSH;
    push->in_arg[0].value = arg;
}

void akl_build_set(struct akl_context *ctx, struct akl_symbol *sym)
{
    struct akl_ir_instruction *set = create_instr(ctx);
    set->in_op            = AKL_IR_SET;
    set->in_arg[0].symbol = sym;
}

void akl_build_get(struct akl_context *ctx, struct akl_symbol *sym)
{
    struct akl_ir_instruction *get = create_instr(ctx);
    get->in_op            = AKL_IR_GET;
    get->in_arg[0].symbol = sym;
}

void akl_build_load(struct akl_context *ctx, struct akl_symbol *sym)
{
    struct akl_ir_instruction *load;
    struct akl_function *fn   = ctx->cx_comp_func;
    struct akl_lisp_fun *ufun = NULL;
    unsigned int         ind;

    /* Load instructions can work on function parameters and local variables
       , but they must be  */
    if (fn->fn_type == AKL_FUNC_USER || fn->fn_type == AKL_FUNC_LAMBDA) {
        ufun = &fn->fn_body.ufun;
        /* Get the frame offset for the currently compiled function's argument. */
        if ((ind = argument_finder(ufun, sym)) != -1) {
            load                   = create_instr(ctx);
            load->in_op            = AKL_IR_LOAD;
            load->in_arg[0].ui_num = ind;
        } else {
            /* Not a parameter or local variable, must be a global one. */
            akl_build_get(ctx, sym);
        }
    }
}

void akl_build_call(struct akl_context *ctx, struct akl_symbol *sym
                    , struct akl_function *fn, int argc)
{
    struct akl_ir_instruction *call = create_instr(ctx);
    call->in_op = AKL_IR_CALL;
    call->in_arg[0].symbol = sym;
    /* If function already fetched, this will make things faster. */
    call->in_fun           = fn;
    call->in_arg[1].ui_num = argc;
}

void akl_build_label(struct akl_context *ctx, struct akl_list *labels, int lc)
{
    struct akl_label *l = (struct akl_label *)akl_list_index(labels, lc);
    l->la_ir = ctx->cx_ir;
    /* Always point to the last instruction.
     * If that doesn't exist, create a NOP as placeholder.
    */
    create_or_get_instr(ctx);
    l->la_branch = AKL_LIST_LAST(ctx->cx_ir);
}

/* It can also mean 'jmp' if the second (the false branch is NULL) */
void akl_build_jump(struct akl_context *ctx, akl_jump_t jt, struct akl_list *l, int lc)
{
    struct akl_ir_instruction *branch = create_instr(ctx);
    branch->in_op = (akl_ir_instruction_t)jt;
    branch->in_arg[0].label = (struct akl_label *)akl_list_index(l, lc);
}

void akl_build_branch(struct akl_context *ctx, struct akl_list *l, int lt, int lf)
{
    struct akl_ir_instruction *branch = create_instr(ctx);
    branch->in_op = AKL_IR_BRANCH;
    branch->in_arg[0].label = (struct akl_label *)akl_list_index(l, lt);
    branch->in_arg[1].label = (struct akl_label *)akl_list_index(l, lf);
}

void akl_build_ret(struct akl_context *ctx)
{
    struct akl_ir_instruction *ret = create_instr(ctx);
    ret->in_op = AKL_IR_RET;
}

void akl_build_nop(struct akl_context *ctx)
{
    struct akl_ir_instruction *nop = create_instr(ctx);
    nop->in_op = AKL_IR_NOP;
}

enum PREFETCH_STATUS {
   PF_FN_NOT_FOUND, PF_FN_NORMAL, PF_FN_SFORM, PF_NOT_FN
};

static enum PREFETCH_STATUS
prefetch_function(struct akl_context *cx, struct akl_value *val, struct akl_function **fn)
{
    struct akl_symbol    *sym;
    struct akl_variable  *var;

    struct akl_state     *s;
    struct akl_io_device *dev;
    s   = cx->cx_state;
    dev = cx->cx_dev;

    sym = val->va_value.symbol;
    var = akl_get_global_var(s, sym);
    if (var == NULL) {
        *fn = NULL;
        return PF_FN_NOT_FOUND;
    }

    /* It's the first list element, must be some kind of a function */
    val = var->vr_value;
    if (AKL_CHECK_TYPE(val, AKL_VT_FUNCTION)) {
        *fn = val->va_value.func;
        if (*fn) {
            if ((*fn)->fn_type == AKL_FUNC_SPECIAL) {
                /* It's really a special form.
                   akl_compile_list() must call this. */
                return PF_FN_SFORM;
            } else {
                /* It's a function, but no need to call it now. */
                return PF_FN_NORMAL;
            }
        }
    }
    *fn = NULL;
    return PF_NOT_FN;
}

/* Build the intermediate representation for an unquoted list */
void akl_compile_list(struct akl_context *cx)
{
    akl_token_t tok;
    struct akl_value *v;
    enum PREFETCH_STATUS pf_st = PF_FN_NOT_FOUND;
    int    argc              = 0;
    bool_t felem             = TRUE;
    bool_t is_quoted         = FALSE;
    struct akl_function *fun = NULL;
    struct akl_symbol   *sym = NULL;
    struct akl_lex_info *call_info = NULL;

    struct akl_state *s;
    struct akl_io_device *dev;

    assert(cx);
    s   = cx->cx_state;
    dev = cx->cx_dev;

    while ((tok = akl_lex(cx->cx_dev))) {
        if (s->ai_interrupted) {
            s->ai_interrupted = FALSE;
            return;
        }

        if (tok == tQUOTE) {
            is_quoted = TRUE;
            tok = akl_lex(cx->cx_dev);
        }

        switch (tok) {
        case tATOM:
            v = akl_parse_token(cx, tok, is_quoted);
            if (v) {
                cx->cx_lex_info = v->va_lex_info;
            }
            if (felem) {
                /* Prepare lexical information for the function call */
                call_info = akl_new_lex_info(s, dev);
                cx->cx_lex_info = call_info;

                /* First place in the list, must be a function name */
                pf_st = prefetch_function(cx, v, &fun);
                if (pf_st == PF_FN_SFORM) {
                    /* It's a special form, call it immediately. */
                    akl_call_sform(cx, v->va_value.symbol, fun);
#if 0
                    if (AKL_CHECK_TYPE(v, AKL_VT_FUNCTION)) {
                        /* Special function gave back another function.
                           Could be a lambda, build a 'call' for this at the end. */
                        fun = v->va_value.func;
                    } else {
                        /* Nothing more to do in this list. */
                        return;
                    }
#endif
                } else if (pf_st == PF_FN_NORMAL || pf_st == PF_FN_NOT_FOUND) {
                    /* No global functions with this name, try to resolve it later. */
                    sym = v->va_value.symbol;
                }
            } else {
                if (is_quoted) {
                    akl_build_push(cx, v);
                } else {
                    akl_build_load(cx, v->va_value.symbol);
                }
                argc++;
                break;
            }
        break;

        case tLBRACE:
            /* The following list can be quoted, then it will be handled
              as an ordinary value */
            if (is_quoted) {
                akl_build_push(cx, akl_parse_token(cx, tok, TRUE));
            } else {
                /* Not quoted, compile it recursively. */
                akl_compile_list(cx);
            }
            argc++;
        break;

        case tRBRACE:
            /* No function name, no function to call and not even a special form */
            if (sym == NULL) {
                if (pf_st != PF_FN_SFORM) {
                    akl_build_push(cx, akl_new_nil_value(cx->cx_state));
                    akl_raise_error(cx, AKL_WARNING, "No function to call.");
                    return;
                }
            } else {
                /* We are run out of arguments, it's time for a function call */
                akl_ir_set_lex_info(cx, call_info);
                akl_build_call(cx, sym, fun, argc);
            }
        return;

        /* tNUMBER, tSTRING, tETC... */
        default:
            v = akl_parse_token(cx, tok, TRUE);
            cx->cx_lex_info = v->va_lex_info;
            akl_build_push(cx, v);
            argc++;
            break;
        }
        is_quoted = FALSE;
        felem = FALSE;
    }
}

akl_token_t akl_compile_next(struct akl_context *ctx)
{
    assert(ctx && ctx->cx_dev);
    akl_token_t tok = akl_lex(ctx->cx_dev);

    if (tok == tLBRACE) {
        akl_compile_list(ctx);
    } else if (tok == tQUOTE) {
        tok = akl_lex(ctx->cx_dev);
        akl_build_push(ctx, akl_parse_token(ctx, tok, TRUE));
    } else if (tok == tEOF) {
        return tok;
    } else {
        akl_build_push(ctx, akl_parse_token(ctx, tok, FALSE));
    }
    return tok;
}

struct akl_context *akl_compile(struct akl_state *s, struct akl_io_device *dev)
{
    AKL_ASSERT(s && dev, NULL);
    struct akl_context *cx = akl_new_context(s);
    struct akl_function *f = akl_new_function(s);
    struct akl_ir_instruction *lip;
    akl_token_t tok;

    akl_init_list(&f->fn_body.ufun.uf_body);
    f->fn_type = AKL_FUNC_USER;
    s->ai_fn_main = f;

    cx->cx_dev = dev;
    cx->cx_comp_func = f;
    cx->cx_ir = &f->fn_body.ufun.uf_body;

    do {
        tok = akl_compile_next(cx);
    } while (tok != tEOF);
    /* Don't need the last NOP anymore, remove it. */
    lip = akl_list_pop(cx->cx_ir);
    if (lip && lip->in_op != AKL_IR_NOP) {
       /* Something is wrong if it got here. Undo everything
          and just pretend it will be ok again.˙*/
        akl_list_append(s, cx->cx_ir, lip);
    }
    return cx;
}

void akl_asm_parse_func(struct akl_context *);

struct akl_context *
akl_asm_compile(struct akl_state *s, struct akl_io_device *dev)
{
    akl_asm_token_t tok;
    struct akl_context *cx = akl_new_context(s);
    cx->cx_ir = akl_new_list(s);
    cx->cx_dev = dev;

    while ((tok = akl_asm_lex(dev)) != tEOF) {
            if (tok == tASM_WORD)
                akl_asm_parse_func(cx);
            /* else error */
    }
    return cx;
}
