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
typedef unsigned char akl_byte_t;

struct akl_operation *akl_find_op_by_name(const char *name)
{
    struct akl_operation *op = akl_instructions[0];
    while (op->name) {
        if (strcmp(op->name, name) == 0)
                return op;
        op++;
    }
}

void akl_build_store(struct akl_vector *ir, enum AKL_VALUE_TYPE vtype, void *arg)
{
    struct akl_ir_instruction store;
    store.in_op = AKL_IR_STORE;
    store.in_arg_type[0] = vtype;
    if (vtype != TYPE_NUMBER) {
        store.in_arg.arg[0] = arg;
    } else {
        store.in_arg.number = *(double *)arg;
    }
    store.in_argc = 1;
    akl_vector_push(ir, &store);
}

void akl_build_load(struct akl_vector *ir, char *name)
{
    struct akl_ir_instruction load;
    load.in_op = AKL_IR_LOAD;
    load.in_arg_type[0] = TYPE_STRING;
    load.in_arg.arg[0] = name;
    load.in_argc = 1;
    akl_vector_push(ir, &load);
}

void akl_build_call(struct akl_vector *ir, char *fname, int argc)
{
    struct akl_ir_instruction call;
    call.in_op = AKL_IR_CALL;
    call.in_arg_type[0] = TYPE_FUNCTION;
    call.in_args.op[0] = fname;
    /* Here we use argc, to represent the argument count _for the function_. */
    call.in_argc = argc;
    akl_vector_push(ir, &call);
}

void akl_build_branch(struct akl_vector *ir, struct akl_vector *tb, struct akl_vector *fb)
{
    struct akl_ir_instruction branch;
    branch.in_op = AKL_IR_BRANCH;
    branch.in_arg.arg[0] = tb; /* True branch */
    branch.in_arg.arg[1] = fb; /* False branch */
    branch.in_argc = 2;
    akl_vector_push(ir, &branch);
}

void akl_build_cmp(struct akl_vector *ir, int cmp_type
                   , enum AKL_VALUE_TYPE atype[2], void *args[2])
{
    struct akl_ir_instruction cmp;
    cmp.in_op = AKL_IR_CMP;
    branch.in_arg.arg[0] = arg[0];
    branch.in_arg.arg[1] = arg[0];

    branch.in_argc = 2;
    akl_vector_push(ir, &branch);
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

/* Build the intermediate representation for an unquoted list */
void akl_compile_list(struct akl_state *s, struct akl_vector *ir, struct akl_io_device *dev)
{
    char *atom_name = NULL;
    bool_t is_quoted = FALSE;
    struct akl_list *l;
    akl_token_t tok;

    while ((tok = akl_lex(dev))) {
        if (tok = tQUOTE) {
            is_quoted = TRUE;
            tok = akl_lex(dev);
        }

        switch (tok) {
            case tATOM:
            if (is_quoted) {
                /* It just used as a symol, nothing special... */
                akl_build_store(ir, TYPE_LIST, akl_lex_get_atom(dev));
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

            /* The following tokens are used as parameters to a function
                , so just push them to the stack */
            case tNIL:
            akl_build_store(ir, TYPE_NIL);
            argc++;
            break;

            case tTRUE:
            akl_build_store(ir, TYPE_TRUE);
            argc++;
            break;

            case tNUMBER:
            akl_build_store(ir, TYPE_NUMBER, akl_lex_get_number());
            argc++;
            break;

            case tSTRING:
            akl_build_store(ir, TYPE_STRING, akl_lex_get_string());
            argc++;
            break;

            case tLBRACE:
            /* The following list can be quoted, then it will be handled
              as an ordinary value */
            if (is_quoted) {
                l = akl_parse_quoted_list(s, dev);
                akl_build_push(ir, TYPE_LIST, l);
                break;
            } else {
                /* Not quoted, compile it recursively. */
                akl_build_list_code(ir, dev);
            }
            break;

            case tRBRACE:
            /* We are run out of arguments, it's time for a function call */
            akl_build_call(ir, atom_name, argc);
            argc = 0;
            return;
        }
        is_quoted = FALSE;
    }
}
