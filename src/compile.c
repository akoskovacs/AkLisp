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

#define DEFINE_IR_INSTRUCTION(name, ctx) \
    struct akl_ir_instruction * name = AKL_MALLOC((ctx)->cx_state \
                                    , struct akl_ir_instruction); \
    akl_list_append((ctx)->cx_state, (ctx)->cx_ir, name )

void akl_build_store(struct akl_context *ctx, struct akl_value *arg)
{
    DEFINE_IR_INSTRUCTION(store, ctx);
    store->in_op = AKL_IR_STORE;
    store->in_arg[0].value = arg;
}

int argument_finder(char **args, const char *arg)
{
    int i = 0;
    if (!args || !arg)
        return -1;

    for (i = 0; args[i]; i++) {
        if (strcmp(args[i], arg) == 0)
            return i;
    }
        
    return -1;
}

void akl_build_load(struct akl_context *ctx, char *name)
{
    struct akl_function *fn = ctx->cx_comp_func;
    struct akl_ufun *ufun = NULL;
    unsigned int ind;
    DEFINE_IR_INSTRUCTION(load, ctx);

    load->in_op = AKL_IR_LOAD;

    if (fn->fn_type == AKL_FUNC_USER) {
        ufun = &fn->fn_body.ufun;
        /* The 'name' must be an argument or a local variable */
        if ((ind = argument_finder(ufun->uf_args, name)) != -1) {
            load->in_arg[0].ui_num = ind; // The found stack pointer
        } else {
            akl_raise_error(ctx, AKL_ERROR
               , "'%s' parameter variable cannot found!", name);
        }
    }
}

/*
 * If the function is not exist, the at_atom field of the 'fn' parameter is
 * NIL, therefore we must bind the function at a later time
*/
void akl_build_call(struct akl_context *ctx, struct akl_atom *atm, int argc)
{
    DEFINE_IR_INSTRUCTION(call, ctx);
    call->in_op = AKL_IR_CALL;
    call->in_arg[0].atom = atm;
    call->in_arg[1].ui_num = argc;
}

/* It can also mean 'jmp' if the second (the false branch is NULL) */
void akl_build_jump(struct akl_context *ctx, akl_jump_t jt, struct akl_label *l)
{
    DEFINE_IR_INSTRUCTION(branch, ctx);
    branch->in_op = (akl_ir_instruction_t)jt;
    branch->in_arg[0].label = l;
}

void akl_build_branch(struct akl_context *ctx, struct akl_label *lt, struct akl_label *lf)
{
    DEFINE_IR_INSTRUCTION(branch, ctx);
    branch->in_op = AKL_IR_BRANCH;
    branch->in_arg[0].label = lt;
    branch->in_arg[1].label = lf;
}

void akl_build_ret(struct akl_context *ctx)
{
    DEFINE_IR_INSTRUCTION(ret, ctx);
    ret->in_op = AKL_IR_RET;
}

void akl_build_nop(struct akl_context *ctx)
{
    DEFINE_IR_INSTRUCTION(nop, ctx);
    nop->in_op = AKL_IR_NOP;
}

/* Build the intermediate representation for an unquoted list */
void akl_compile_list(struct akl_context *cx)
{
    bool_t is_quoted = FALSE;
    char *name;
    akl_token_t tok;
    int argc = 0;
    struct akl_atom *afn = NULL;
    struct akl_function *fun = NULL;
    struct akl_value *v;

    while ((tok = akl_lex(cx->cx_dev))) {
        if (tok == tQUOTE) {
            is_quoted = TRUE;
            tok = akl_lex(cx->cx_dev);
        }

        switch (tok) {
            case tATOM:
            if (is_quoted) {
                /* It just used as a symbol, nothing special... */
                akl_build_store(cx, akl_parse_token(cx, tok, TRUE));
                argc++;
                break;
            } else {
                /* If this atom is at the first place (ie. NULL), it must
                  be some sort of function. Find it out and if it is a
                  special form, execute it. */
                if (afn == NULL) {
                    name = akl_lex_get_atom(cx->cx_dev);
                    afn = akl_get_global_atom(cx->cx_state, name);
                    /* The previous function copied the name. Must free it. */
                    akl_free(cx->cx_state, name, 0);
                    if (afn == NULL)
                        break;

                    name = afn->at_name;

                    /* If the function is a special form, we must execute it, now. */
                    v = afn->at_value;
                    if (AKL_CHECK_TYPE(v, TYPE_FUNCTION) && v->va_value.func != NULL) {
                        fun = v->va_value.func;
                        if (fun->fn_type == AKL_FUNC_SPECIAL) {
                            cx->cx_func_name = name;
                            cx->cx_func = fun;
                            fun->fn_body.scfun(cx);
                            return;
                        }
                    } /* else: error */
                    break;
                }
                /* Not the first place, it must be a reference to a value */
                /* TODO: Global definitions */
                akl_build_load(cx, akl_lex_get_atom(cx->cx_dev));
                argc++;
            }
            break;

            case tLBRACE:
            /* The following list can be quoted, then it will be handled
              as an ordinary value */
            if (is_quoted) {
                akl_build_store(cx, akl_parse_token(cx, tok, TRUE));
            } else {
                /* Not quoted, compile it recursively. */
                akl_compile_list(cx);
            }
            argc++;
            break;

            case tRBRACE:
            /* We are run out of arguments, it's time for a function call */
            akl_build_call(cx, afn, argc);
            return;

            /* tNUMBER, tSTRING, tETC... */
            default:
            akl_build_store(cx, akl_parse_token(cx, tok, TRUE));
            argc++;
            break;
        }
        is_quoted = FALSE;
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
        akl_build_store(ctx, akl_parse_token(ctx, tok, TRUE));
    } else if (tok == tEOF) {
        return tok;
    } else {
        akl_build_store(ctx, akl_parse_token(ctx, tok, FALSE));
    }
    return tok;
}

struct akl_context *akl_compile(struct akl_state *s, struct akl_io_device *dev)
{
    struct akl_context *cx = akl_new_context(s);
    struct akl_function *f = akl_new_function(s);
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
