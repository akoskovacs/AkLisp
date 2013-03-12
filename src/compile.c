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

struct akl_label *
akl_new_branches(struct akl_state *s, struct akl_vector *v, unsigned int cnt)
{
    assert(cnt || s);
    int i;
    struct akl_label *br = akl_malloc(s, sizeof(struct akl_label)*cnt);
    for (i = 0; i < cnt; i++) {
        br[i].ab_branch = v;
        br[i].ab_size = 0;
        br[i].ab_start = 0;
    }
    return br;
}

void akl_build_store(struct akl_vector *ir, struct akl_value *arg)
{
    struct akl_ir_instruction *store = akl_vector_reserve(ir);
    store->in_op = AKL_IR_STORE;
    store->in_arg[0] = arg;
    store->in_argc = 1;
}

void akl_build_store_nil(struct akl_vector *ir, struct akl_lex_info *lex)
{
    struct akl_ir_instruction *store = akl_vector_reserve(ir);
    store->in_op = AKL_IR_STORE_NIL;
    store->in_arg[0] = lex;
    store->in_argc = 0;
}

void akl_build_store_true(struct akl_vector *ir, struct akl_lex_info *lex)
{
    struct akl_ir_instruction *store = akl_vector_reserve(ir);
    store->in_op = AKL_IR_STORE_TRUE;
    store->in_arg[0] = lex;
    store->in_argc = 0;
}

void akl_build_load(struct akl_vector *ir, char *name)
{
    struct akl_ir_instruction *load = akl_vector_reserve(ir);
    load->in_op = AKL_IR_LOAD;
    load->in_arg[0] = name;
    load->in_argc = 1;
}

void akl_build_call(struct akl_vector *ir, char *fname, int argc)
{
    struct akl_ir_instruction *call = akl_vector_reserve(ir);
    call->in_op = AKL_IR_CALL;
    call->in_arg[0] = fname;
    /* Here we use argc, to represent the argument count _for the function_. */
    call->in_argc = argc;
}

/* It can also mean 'jmp' if the second (the false branch is NULL) */
void akl_build_branch(struct akl_vector *ir, struct akl_label *tb, struct akl_label *fb)
{
    struct akl_ir_instruction *branch = akl_vector_reserve(ir);
    branch->in_op = AKL_IR_BRANCH;
    branch->in_arg.arg[0] = (void *)tb; /* True branch */
    branch->in_arg.arg[1] = (void *)fb; /* False branch */
    if (fb == NULL)
        branch->in_argc = 1;
    else
        branch->in_argc = 2;
}

void akl_build_cmp(struct akl_vector *ir, struct akl_value *a1, struct akl_value *a2)
{
    struct akl_ir_instruction *cmp = akl_vector_reserve(ir);
    cmp->in_op = AKL_IR_CMP;
    branch->in_arg[0] = a1;
    branch->in_arg[1] = a2;
    branch->in_argc = 2;
}

#if 0
struct akl_vector *akl_compile(struct akl_state *s, struct akl_list *list)
{
    struct akl_vector *instr = akl_vector_new(s, 20, sizeof(struct akl_instruction));
    struct akl_value *val;
    AKL_LIST_FOREACH(val, list) {
        if (val->va_value == TYPE_LIST)
            akl_compile_list(s, AKL_GET_LIST_VALUE(value), instr);
        else
            akl_compile_value(s, val, instr);
    }
    return instr;
}
#endif
struct akl_value *akl_build_value(struct akl_state *, struct akl_io_device *dev, akl_token_t);

void akl_compile_branch(struct akl_state *s, struct akl_vector *ir
                        , struct akl_io_device *dev, struct akl_label *br)
{
    assert(br || ir || dev || s);
    br->ab_start = akl_vector_count(ir);
    akl_compile_list(s, ir, dev);
    br->ab_size = akl_vector_count(ir) - br->ab_start;
}

/* Build the intermediate representation for an unquoted list */
void akl_compile_list(struct akl_state *s, struct akl_vector *ir, struct akl_io_device *dev)
{
    char *atom_name = NULL;
    bool_t is_quoted = FALSE;
    struct akl_list *l;
    akl_token_t tok;
    int argc = 0;
    struct akl_function *fun;

    while ((tok = akl_lex(dev))) {
        if (tok = tQUOTE) {
            is_quoted = TRUE;
            tok = akl_lex(dev);
        }

        switch (tok) {
            case tATOM:
            if (is_quoted) {
                /* It just used as a symbol, nothing special... */
                akl_build_store(ir, akl_build_value(s, dev, tok));
                argc++;
                break;
            } else {
                /* If this atom is at the first place (ie. NULL), it must
                  be some sort of function. Find it out and if it really a
                  special form, execute it. */
                if (!atom_name) {
                    atom_name = akl_lex_get_atom(s);
                    fun = akl_find_function(s, atom_name);
                    if (fun && fun->fn_type == AKL_FUNC_SEPECIAL) {
                        fun->fn_body.scfun(s, ir, dev);
                        return;
                    }
                    break;
                }
                /* Not the first place, it must be a reference to a value */
                akl_build_load(ir, akl_lex_get_atom(dev));
                argc++;
            }
            break;

            case tLBRACE:
            /* The following list can be quoted, then it will be handled
              as an ordinary value */
            if (is_quoted) {
                akl_build_store(ir, akl_build_value(s, dev, tok));
                argc++;
            } else {
                /* Not quoted, compile it recursively. */
                akl_compile_list(s, ir, dev);
            }
            break;

            case tRBRACE:
            /* We are run out of arguments, it's time for a function call */
            akl_build_call(ir, atom_name, argc);
            return;

            case tNIL:
            akl_build_store_nil(ir, akl_new_lex_info(s, dev));
            argc++;
            break;

            case tTRUE:
            akl_build_store_true(ir, akl_new_lex_info(s, dev));
            argc++;
            break;

            /* tNUMBER, tSTRING */
            default:
            akl_build_store(ir, akl_build_value(s, dev, tok));
            argc++;
            break;
        }
        is_quoted = FALSE;
    }
}
