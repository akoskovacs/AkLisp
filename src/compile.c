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
inline struct akl_operation *akl_get_op(enum AKL_OPCODES op)
{
   return &akl_instructions[op];
}

struct akl_operation *akl_find_op_by_name(const char *name)
{
    struct akl_operation *op = akl_instructions[0];
    while (op->name) {
        if (strcmp(op->name, name) == 0)
                return op;
        op++;
    }
}

akl_byte_t *akl_new_op_push(struct akl_state *s, struct akl_value *val)
{
    struct akl_operation *op;
    uint64_t *code;
    switch (val->va_type) {
        case TYPE_ATOM:
        break;

        case TYPE_NUMBER:
        op = akl_find_op_by_name("push.n");
        code = MALLOC_FUNCTION(sizeof(uint64_t));
        code = op->op_code;
        code |= (int)(AKL_GET_NUMBER_VALUE(val) << 8);
        return &code;
    }
}

struct akl_vector *akl_compile(struct akl_state *s,struct akl_list *list)
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

void akl_compile_value(struct akl_state *s, struct akl_value *value, struct akl_vector *instr)
{
    struct akl_instruction vin;
    vin.in_operands[0] = value;
    if (AKL_IS_QUOTED(value)) {
        /* TODO */
    }
    switch (value->va_type) {
        case TYPE_LIST:
        akl_compile_list(s, AKL_GET_LIST_VALUE(value));
        break;

        case TYPE_STRING:
        vin.in_operation = akl_get_op(AKL_OP_PUSH_S);
        break;

        case TYPE_NUMBER:
        vin.in_operation = akl_get_op(AKL_OP_PUSH_N);
        break;

        case TYPE_NIL:
        vin.in_operands[0] = NULL;
        break;
    }
    akl_vector_push(instr, &vin);
}

void akl_compile_list(struct akl_state *s, struct akl_list *list, struct akl_vector *instr)
{
    assert(list);
    struct akl_instruction call;
    struct akl_value *val = list->li_head;
    struct akl_atom *atom;
    akl_cfun_t spec;
    if (AKL_IS_QUOTED(list)) {

    }

    switch (val->va_type) {
        case TYPE_ATOM:
        atom = AKL_GET_ATOM_VALUE(val);
        break;

        case TYPE_BUILTIN:
        spec = AKL_GET_CFUN_VALUE(val);
        if (spec) {
            spec(in, list, instr);
        }
        break;

        default:
        akl_add_error(in, AKL_ERROR, a1->va_lex_info, "The first element must be an atom.\n"
                      , akl_get_atom_name_value(a1));
        return;
    }

    AKL_LIST_FOREACH_SECOND(val, list) {
        akl_compile_value(in, val, instr);
    }
    call.in_operation = akl_get_op(AKL_OP_CALL);
    call.in_operands[0] = atom;
    call.in_operands[1] = list->li_elem_count;
    akl_vector_add(instr, &call);
}
