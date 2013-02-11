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
 * representation, by parsing the tokens.
*/
#define AKL_SPEC_DEFINE(name, state, args, ctx) \
    static void akl_spec_##name(struct akl_state * state \
    , struct akl_list * args , struct akl_context * ctx)

#define AKL_SPEC_DEFINE(name, state, args, instr) \
    static void akl_spec_##name(struct akl_state * state \
    , struct akl_vector * ir , struct akl_io_device * dev)


AKL_SPEC_DEFINE(if, s, ir, dev)
{
    struct akl_vector *branch, *tb, *fb;

    /* Compile the condition (while pushing the instructions to the list) */
    akl_compile_list(s, cl, ir);
    /* Create the new branches */
    tb = akl_vector_new(s, 10, sizeof(struct akl_ir_instruction));
    fb = akl_vector_new(s, 10, sizeof(struct akl_ir_instruction));
    /* Compile the true branch, and place it in the new vector */
    akl_compile_list(s, dev, tb);
    /* Compile the false branch, and place it in the new vector */
    akl_compile_list(s, dev, fb);
    /* Add the branch instruction to the IR */
    akl_build_branch(ir, tb, fb);
}

AKL_SPEC_DEFINE(while, s, ir, dev)
{
    akl_token_t tok;
    const size_t ir_size = sizeof(struct akl_ir_instruction);
    struct akl_vector *cond = akl_vector_new(s, 4, ir_size);
    struct akl_vector *loop = akl_vector_new(s, 5, ir_size);
    if ((tok = akl_lex(dev)) != tLBRACE) {
        return; /* Panic */
    }
    /* Compile the condition */
    akl_compile_list(s, dev, cond);
    /* Compile loop */
    while ((tok = akl_lex(dev)) != tRBRACE) {
        akl_compile_list(s, dev, loop);
    }
    akl_build_branch(cond,

    /* TODO... */
}

struct akl_vector *akl_parse_params(struct akl_state *s, struct akl_vector *args
                                    , struct akl_io_device *dev)
{
    akl_token_t tok;
    char *opt_name;
    while ((tok = akl_lex(dev)) != tRBRACE) {
        if (tok == tATOM) {
           opt_name = akl_lex_get_atom(dev);
           akl_vector_push(args, &opt_name);
        } else {
            /* TODO: Scream! */
        }
    }
    return args;
}

AKL_SPEC_DEFINE(defun, s, ir, dev)
{
    struct akl_function *func = akl_new_function(s);
    struct akl_vector *fbody = akl_vector_new(s, 15
                                  , sizeof(struct akl_ir_instruction));
    func->fn_body = fbody;
    akl_vector_init(&func->fn_args, 4, sizeof(char *));
    if (akl_lex(dev) == tATOM) {
        func->fn_name = akl_lex_get_atom(dev);
    } else {
        /* TODO: No name... */
    }
    func->fn_args = akl_parse_params(s, dev);
    /* Eat the next left brace and interpret the body... */
    if (akl_lex(dev) == tLBRACE){
        akl_compile_list(s, fbody, dev);
    } else {
        /* TODO: Scream again! */
    }
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

AKL_SPEC_DEFINE(defun, s, args, ctx)
{
    struct akl_value *fn, *a, *body;
    struct akl_atom *atm;
    fn = AKL_FIRST_VALUE(args);
    a = AKL_SECOND_VALUE(args);
    if (AKL_CHECK_TYPE(fn, TYPE_ATOM)) {
        atm = AKL_GET_ATOM_VALUE(fn);
    } else {
        return;
    }
    switch (a->va_type) {
        case TYPE_STRING:
        atm->at_desc = AKL_GET_STRING_VALUE(a);
    }
}
