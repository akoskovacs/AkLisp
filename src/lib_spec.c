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
AKL_DEFINE_SFUN(when, ctx)
{
    int loff = 0;
    struct akl_list *label = akl_new_labels(ctx, &loff, 1);
    akl_compile_next(ctx, NULL);
    akl_build_jump(ctx, AKL_JMP_FALSE, label, loff+0);
    akl_compile_next(ctx, NULL);
    akl_build_label(ctx, label, loff+0);
    return NULL;
}

AKL_DEFINE_SFUN(set, ctx)
{
    assert(ctx && ctx->cx_state && ctx->cx_dev);
    struct akl_io_device *dev = ctx->cx_dev;
    struct akl_symbol *sym;
    struct akl_function *fn;
    akl_token_t tok = akl_lex(dev);
    if (tok != tATOM) {
        akl_raise_error(ctx, AKL_ERROR, "Unexpected token, (need a valid atom for set!)");
        return NULL;
    }
    sym = akl_lex_get_symbol(dev);
    akl_compile_next(ctx, &fn);
    if (fn) {
        akl_build_push(ctx, akl_new_function_value(ctx->cx_state, fn));
    }
    akl_build_set(ctx, sym);
    return NULL;
}

AKL_DEFINE_SFUN(sif, ctx)
{
    /* Allocate the branch */
    int loff = 0;
    struct akl_list *labels = akl_new_labels(ctx, &loff, 2);

    /* Condition:*/
    akl_compile_next(ctx, NULL);
    akl_build_jump(ctx, AKL_JMP_FALSE, labels, loff+1);

    /* True branch: */
    akl_compile_next(ctx, NULL);
    akl_build_jump(ctx, AKL_JMP, labels, loff+0);

    /* .L1: False branch: */
    akl_build_label(ctx, labels, loff+1);
    akl_compile_next(ctx, NULL);

    /* .L0: Continue... */
    akl_build_label(ctx, labels, loff+0);
    return NULL;
}

AKL_DEFINE_SFUN(swhile, ctx)
{
    int loff = 0;
    struct akl_list *labels = akl_new_labels(ctx, &loff, 2);
    /* .L0: Condition: */
    akl_build_label(ctx, labels, loff+0);
    akl_compile_next(ctx, NULL);
    akl_build_jump(ctx, AKL_JMP_FALSE, labels, loff+1);
    /* the loop itself */
    akl_compile_next(ctx, NULL);
    /* Jump back to the condition: (with an unconditional) */
    akl_build_jump(ctx, AKL_JMP, labels, loff+0);

    /* .L1: Getting out of the loop... */
    akl_build_label(ctx, labels, loff+1);
    return NULL;
}

void
akl_parse_params(struct akl_context *ctx, const char *fname, struct akl_vector *args)
{
    assert(ctx && ctx->cx_state && ctx->cx_dev);
    akl_token_t tok;
    int i = 0;
    const int DEF_ARGC = 3;
    tok = akl_lex(ctx->cx_dev);

    /* Empty argument list (0 args) */
    if (tok == tNIL) {
        return;
    }

    if (fname == NULL) {
        fname = "lambda";
    }

    /* No argument list at all */
    if (tok != tLBRACE) {
        akl_raise_error(ctx, AKL_ERROR, "Expected an argument list for function definition \'%s\'"
                        , fname);
        return;
    }

    akl_init_vector(ctx->cx_state, args, DEF_ARGC, sizeof(struct akl_symbol *));

    while ((tok = akl_lex(ctx->cx_dev)) != tRBRACE) {
        /* TODO: pattern matching... */
        if (tok == tATOM) {
           akl_vector_push(args, akl_lex_get_symbol(ctx->cx_dev));
        }
    }
}

AKL_DEFINE_SFUN(defun, ctx)
{
    struct akl_lisp_fun *ufun;
    akl_token_t tok;
    struct akl_function *func = akl_new_function(ctx->cx_state);
    struct akl_value *fval = akl_new_function_value(ctx->cx_state, func);
    struct akl_list *oir = ctx->cx_ir;
    struct akl_symbol *fsym;
    char *docstring = NULL;

    func->fn_type = AKL_FUNC_USER;
    ufun = &func->fn_body.ufun;
    akl_init_list(&ufun->uf_body);
    akl_init_list(&ufun->uf_labels);

    if (akl_lex(ctx->cx_dev) == tATOM) {
        fsym = akl_lex_get_symbol(ctx->cx_dev);
    } else {
        /* TODO: Error! */
        akl_raise_error(ctx, AKL_ERROR, "Unexpected token, "
                        "defun! needs a parameter list and a function body");
        return NULL;
    }

    ctx->cx_comp_func = func;
    akl_parse_params(ctx, fsym->sb_name, &ufun->uf_args);
    ctx->cx_ir = &ufun->uf_body;
    /* Eat the next left brace and interpret the body... */
    tok = akl_lex(ctx->cx_dev);
    if (tok == tSTRING) {
        docstring = akl_lex_get_string(ctx->cx_dev);
        tok = akl_lex(ctx->cx_dev);
    } else {
        akl_lex_putback(ctx->cx_dev, tok);
    }
    akl_set_global_var(ctx->cx_state, fsym, docstring, FALSE, fval);

    //tok = akl_lex(ctx->cx_dev);
    akl_compile_next(ctx, NULL);
#if 0
    if (tok == tLBRACE) {
        akl_compile_list(ctx);
    } else {
        akl_lex_putback(ctx->cx_dev, tok);
        akl_compile_list(ctx);
    }
#endif

    /* Build needs the old IR */
    ctx->cx_ir = oir;
    akl_build_push(ctx, akl_new_sym_value(ctx->cx_state, fsym));
    return func;
}

AKL_DEFINE_SFUN(lambda, ctx)
{
    struct akl_lisp_fun *ufun;
    akl_token_t tok;
    struct akl_function *func = akl_new_function(ctx->cx_state);
    char *docstring = NULL;

    func->fn_type = AKL_FUNC_USER;
    ufun = &func->fn_body.ufun;
    akl_init_list(&ufun->uf_body);
    akl_init_list(&ufun->uf_labels);

    ctx->cx_comp_func = func;
    akl_parse_params(ctx, NULL, &ufun->uf_args);
    ctx->cx_ir = &ufun->uf_body;
    /* Eat the next left brace and interpret the body... */
    tok = akl_lex(ctx->cx_dev);
    if (tok == tSTRING) {
        docstring = akl_lex_get_string(ctx->cx_dev);
        tok = akl_lex(ctx->cx_dev);
    } else {
        akl_lex_putback(ctx->cx_dev, tok);
    }
    akl_compile_next(ctx, NULL);
    return func;
}

AKL_DECLARE_FUNS(akl_spec_forms) {
    AKL_SFUN(sif, "if", "Conditional expression"),
    AKL_SFUN(lambda, "lambda", "Define a lambda function"),
    AKL_SFUN(lambda, "->", "Define a lambda function"),
    AKL_SFUN(when, "when", "Conditionally evaluate an expression"),
    AKL_SFUN(swhile, "while", "Conditional loop expression"),
    AKL_SFUN(defun, "defun!", "Define a new function"),
    AKL_SFUN(set, "set!", "Define a new global variable"),
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
