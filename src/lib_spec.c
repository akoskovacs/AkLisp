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

/*
 * These functions are special forms. They build their own internal
 * representation by parsing the tokens.
*/
void akl_build_label(struct akl_context *ctx, struct akl_label *l)
{
    l->la_ir = ctx->cx_ir;
    l->la_branch = AKL_LIST_LAST(ctx->cx_ir);
}

AKL_DEFINE_SFUN(when, ctx)
{
    struct akl_label *label = akl_new_labels(ctx, 2);
    akl_compile_next(ctx);
    akl_build_jump(ctx, AKL_JMP_FALSE, label+0);
    akl_build_label(ctx, label+0);
    akl_compile_next(ctx);
    return NULL;
}

AKL_DEFINE_SFUN(set, ctx)
{
    assert(ctx && ctx->cx_state && ctx->cx_dev);
    struct akl_state *s       = ctx->cx_state;
    struct akl_io_device *dev = ctx->cx_dev;
    char *vname;
    struct akl_value *value;
    akl_token_t tok = akl_lex(dev);
    if (tok != tATOM) {
        akl_raise_error(ctx, AKL_ERROR, "Unexpected token, (need a valid atom for set!)");
        return;
    }
    vname = akl_lex_get_atom(dev);
    return NULL;
}

AKL_DEFINE_SFUN(sif, ctx)
{
    /* Allocate the branch */
    struct akl_label *label = akl_new_labels(ctx, 3);

    /* Condition:*/
    akl_compile_next(ctx);
    akl_build_branch(ctx, label+0, label+1);

    /* .L0: True branch: */
    akl_build_label(ctx, label+0);
    akl_compile_next(ctx);
    akl_build_jump(ctx, AKL_JMP, label+2);

    /* .L1: False branch: */
    akl_build_label(ctx, label+1);
    akl_compile_next(ctx);
    akl_build_jump(ctx, AKL_JMP, label+2);

    /* .L2: Continue... */
    akl_build_label(ctx, label+2);
    return NULL;
}

AKL_DEFINE_SFUN(swhile, ctx)
{
    akl_token_t tok;
    struct akl_label *label = akl_new_labels(ctx, 3);
    if ((tok = akl_lex(ctx->cx_dev)) != tLBRACE) {
        return; /* Panic */
    }
    /* .L0: Condition: */
    akl_build_label(ctx, label+0);
    akl_compile_next(ctx);
    akl_build_branch(ctx, label+2, label+1);

    while ((tok = akl_lex(ctx->cx_dev)) != tRBRACE) {
        akl_compile_list(ctx);
    }
    /* Jump back to the condition: (with an unconditional) */
    akl_build_jump(ctx, AKL_JMP, label+0);

    /* .L1: Getting out of the loop... */
    akl_build_label(ctx, label+2);
    return NULL;
}

char **akl_parse_params(struct akl_context *ctx, const char *fname)
{
    assert(ctx && ctx->cx_state && ctx->cx_dev);
    akl_token_t tok;
    int i = 0;
    const int DEF_ARGC = 3;
    char **args = NULL;
    tok = akl_lex(ctx->cx_dev);

    /* Empty argument list (0 args) */
    if (tok == tNIL) {
        return NULL;
    }

    if (fname == NULL) {
        fname = "lambda";
    }

    /* No argument list at all */
    if (tok != tLBRACE) {
        akl_raise_error(ctx, AKL_ERROR, "Expected an argument list for function definition \'%s\'"
                        , fname);
        return NULL;
    }
    args = (char **)akl_calloc(ctx->cx_state, DEF_ARGC, sizeof(char *));

    while ((tok = akl_lex(ctx->cx_dev)) != tRBRACE) {
        if (tok == tATOM) {
           args[i] = akl_lex_get_atom(ctx->cx_dev);
           if (++i >= DEF_ARGC) {
               args = akl_realloc(ctx->cx_state, args, i+DEF_ARGC);
           }
        } else {
            /* TODO: hoho, pattern matching... */
        }
        args[i] = NULL;
    }
    return args;
}

AKL_DEFINE_SFUN(defun, ctx)
{
    struct akl_lisp_fun *ufun;
    akl_token_t tok;
    struct akl_function *func = akl_new_function(ctx->cx_state);
    struct akl_value *fval = akl_new_function_value(ctx->cx_state, func);
    struct akl_symbol *fsym;
    struct akl_variable *fvar;

    func->fn_type = AKL_FUNC_USER;
    ufun = &func->fn_body.ufun;
    akl_init_list(&ufun->uf_body);

    if (akl_lex(ctx->cx_dev) == tATOM) {
        fvar = akl_new_variable(ctx->cx_state, akl_lex_get_atom(ctx->cx_dev), FALSE);
        fsym = fvar->vr_symbol;
    } else {
        /* TODO: Error! */
        akl_raise_error(ctx, AKL_ERROR, "Unexpected token, "
                        "defun! needs a parameter list and a function body");
        return;
    }

    ctx->cx_comp_func = func;
    ufun->uf_args = akl_parse_params(ctx, fsym->sb_name);
    ctx->cx_ir = &ufun->uf_body;
    /* Eat the next left brace and interpret the body... */
    tok = akl_lex(ctx->cx_dev);
    if (tok == tSTRING) {
        fvar->vr_desc = akl_lex_get_string(ctx->cx_dev);
        fvar->vr_is_cdesc = FALSE;
        tok = akl_lex(ctx->cx_dev);
    }

    if (tok == tLBRACE) {
        akl_compile_list(ctx);
    } else {
        /* Todo: Error: no body */
        //akl_remove_global_atom(ctx->cx_state, fatm);
    }
    akl_build_push(ctx, fval);
    return NULL;
}

AKL_DECLARE_FUNS(akl_spec_forms) {
    AKL_SFUN(sif, "if", "Conditional expression"),
    AKL_SFUN(when, "when", "Conditionally evaluate an expression"),
    AKL_SFUN(swhile, "while", "Conditional loop expression"),
    AKL_SFUN(defun, "defun!", "Define a new function"),
    AKL_END_FUNS()
};

void akl_spec_library_init(struct akl_state *s, enum AKL_INIT_FLAGS flags)
{
    akl_declare_functions(s, akl_spec_forms);
}

#if 0
AKL_SPEC_DEFINE(setq, ctx)
{
}

AKL_SPEC_DEFINE(lambda, s, ir, dev)
{
    struct akl_function *func = akl_new_function(s);
    char *fname;
    struct akl_vector *fbody = akl_vector_new(s, 10
                                  , sizeof(struct akl_ir_instruction));
    func->fn_body = fbody;
    akl_vector_init(&func->fn_args, 4, sizeof(char *));
    func->fn_args = akl_parse_params(s, dev);
    /* Eat the next left brace and interpret the body... */
    if (akl_lex(dev) == tLBRACE){
        akl_compile_list(s, fbody, dev);
    } else {
        /* TODO: Scream again! */
    }
}
#endif
