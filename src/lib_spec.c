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

#define AKL_SPEC_DEFINE(name, state, args, instr) \
    void akl_spec_##name(struct akl_state * state \
    , struct akl_list * args, struct akl_vector * instr)

AKL_SPEC_DEFINE(gt, s, args, instr) {
    struct akl_instruction cin;
    struct akl_value *a[2];

    a[0] = AKL_FIRST_VALUE(args);
    a[1] = AKL_SECOND_VALUE(args);

    if (!a[0] || !a[1] || (a[0]->va_type != a[1]->va_type != TYPE_NUMBER))
        return; /* Throw error */

    cin.in_operation = &akl_instructions[AKL_OP_GT_N];
    cin.in_operands[0] = a[0];
    cin.in_operands[1] = a[1];
    akl_vector_push(instr, &cin);
}

AKL_SPEC_DEFINE(if, s, args, instr) {
    struct akl_value *a[3];
    struct akl_instruction jmp;
    jmp.in_operation = &akl_instructions[AKL_OP_JMP];

    a[0] = AKL_FIRST_VALUE(args);
    a[1] = AKL_SECOND_VALUE(args);
    a[2] = AKL_THIRD_VALUE(args);

    if (AKL_CHECK_TYPE(a[0], TYPE_LIST))
        struct akl_list *cond = AKL_GET_LIST_VALUE(a[0]);
    else
        return; /* TODO: Throw error */

    akl_compile_list(s, cond);

}
