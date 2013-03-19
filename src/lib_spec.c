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

void akl_build_label(struct akl_vector *ir, struct akl_label *l)
{
    l->ab_code = akl_vector_count(ir);
}

struct akl_label *akl_new_labels(struct akl_state *s, int n)
{
    struct akl_label *labels = (struct akl_label *)
            akl_calloc(s, sizeof(struct akl_label), n);
    return labels;
}

AKL_SPEC_DEFINE(if, s, ir, dev)
{
    /* Allocate the branch */
    struct akl_label *label = akl_new_labels(s, 3);

    /* Condition:*/
    akl_compile_list(ir, dev);
    akl_build_jump(s, AKL_JMP_TRUE, label+0);
    akl_build_jump(s, AKL_JMP_FALSE, label+1);

    /* .L0: True branch: */
    akl_build_label(ir, label+0);
    akl_compile_list(s, ir, dev);
    akl_build_jump(s, AKL_JMP, label+2);

    /* .L1: False branch: */
    akl_build_label(ir, label+1);
    akl_compile_list(s, ir, dev, label+1);
    akl_build_jump(s, AKL_JMP, label+2);

    /* .L2: Continue... */
    akl_build_label(ir, label+2);
}

AKL_SPEC_DEFINE(while, s, ir, dev)
{
    akl_token_t tok;
    struct akl_label *label = akl_new_labels(s, 2);
    if ((tok = akl_lex(dev)) != tLBRACE) {
        return; /* Panic */
    }
    /* .L0: Condition: */
    akl_build_label(ir, label+0);
    akl_compile_list(s, dev, ir);
    /* If the condition is false, don't need to get in the loop */
    akl_build_jump(ir, AKL_JMP_FALSE, label+1);
    
    while ((tok = akl_lex(dev)) != tRBRACE) {
        akl_compile_list(s, dev, ir);
    }
    /* Jump back to the condition: (with an unconditional) */
    akl_build_jump(ir, AKL_JMP, label+0);

    /* .L1: Getting out of the loop... */
    akl_build_label(ir, label+1);
}

AKL_SPEC_DEFINE(cond, s, ir, dev)
{

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
