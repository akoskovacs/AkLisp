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

static void
akl_set_lex_info(struct akl_context *ctx, struct akl_value *value)
{
    assert(value);
    value->va_lex_info = akl_new_lex_info(ctx->cx_state, ctx->cx_dev);
}

struct akl_value *
akl_parse_token(struct akl_context *ctx, akl_token_t tok, bool_t is_quoted)
{
    struct akl_value *value = NULL;
    struct akl_list *l = NULL;
    switch (tok) {
        case tEOF:
        akl_lex_free(ctx->cx_dev);
        case tRBRACE:
        return NULL;

        case tATOM:
        value = akl_new_atom_value(ctx->cx_state, akl_lex_get_atom(ctx->cx_dev));
        break;

        case tNUMBER:
        value = akl_new_number_value(ctx->cx_state, akl_lex_get_number(ctx->cx_dev));
        break;

        case tSTRING:
        value = akl_new_string_value(ctx->cx_state, akl_lex_get_string(ctx->cx_dev));
        break;

        /* We should care only about quoted lists */
        case tLBRACE:
        l = akl_parse_list(ctx);
        value = akl_new_list_value(ctx->cx_state, l);
        break;

        case tNIL:
        value = akl_new_nil_value(ctx->cx_state);
        break;

        case tTRUE:
        value = akl_new_true_value(ctx->cx_state);
        break;

        case tQUOTE:
        // TODO: Error
        break;
    }

    value->is_quoted = is_quoted;
    akl_set_lex_info(ctx, value);
    return value;
}

struct akl_value *akl_parse_value(struct akl_context *ctx)
{
    assert(ctx);
    akl_token_t tok = akl_lex(ctx->cx_dev);
    bool_t is_quoted = FALSE;

    if (tok == tQUOTE) {
        is_quoted = TRUE;
        tok = akl_lex(ctx->cx_dev);
    }
    return akl_parse_token(ctx, tok, is_quoted);
}

/* TODO: Fix for NULL */
struct akl_list *akl_parse_list(struct akl_context *ctx)
{
    struct akl_value *value = NULL;
    struct akl_list *list, *lval;
    list = akl_new_list(ctx->cx_state);
    while ((value = akl_parse_value(ctx)) != NULL) {

        /* If the next value is a list, reparent it... */
        if (AKL_CHECK_TYPE(value, TYPE_LIST)) {
            lval = AKL_GET_LIST_VALUE(value);
            lval->li_parent = list;
        }
        akl_list_append_value(ctx->cx_state, list, value);
    }
    list->is_quoted = TRUE;
    return list;
}

struct akl_list *
akl_str_to_list(struct akl_state *s, const char *str)
{
    struct akl_io_device *dev = akl_new_string_device(s, "string", str);
    struct akl_list *l = NULL;
    struct akl_context ctx;
    akl_token_t tok;
    akl_init_context(&ctx);
    ctx.cx_state = s;
    ctx.cx_dev = dev;
    tok = akl_lex(dev);
    if (tok != tLBRACE && tok != tQUOTE)
        goto error;

    l = akl_parse_list(&ctx);
error:
    akl_lex_free(dev);
    AKL_FREE(s, dev);
    return l;
}

/* Have to be in the same order as akl_ir_instruction_t */
const char *akl_ir_instruction_set[] = {
    "nop"  , "push" , "load"
  , "call" , "get"   , "set"
  , "br"   , "jmp"   , "jt"
  , "jn"   , "head"  , "tail"
  , "ret"  , NULL
};

#define AKL_ASSEMBLER
#ifdef AKL_ASSEMBLER

/* Parses '.loop_1:' labels */
static void akl_asm_parse_label(struct akl_context *ctx)
{
    char *label_name = akl_lex_get_atom(ctx->cx_dev);
    struct akl_label *label = akl_new_label(ctx);
    struct akl_function *fn = ctx->cx_comp_func;
    struct akl_ufun *ufun;
    if (fn == NULL)
        return;

    ufun = &fn->fn_body.ufun;
    if (label && label_name) {
        label->la_name = label_name;
        label->la_ir = ctx->cx_ir;
        label->la_branch = AKL_LIST_LAST(label->la_ir);
    }
    akl_vector_push(ufun->uf_labels, label);
    if (akl_asm_lex(ctx->cx_dev) != tASM_COLON)
        ; /* Error */
}

static int akl_mnemonic_to_instr(const char *mnemonic)
{
    assert(mnemonic);
    const char **iptr = akl_ir_instruction_set;
    int i = 0;
    while (iptr) {
        if (strcmp(*iptr, mnemonic) == 0) {
            return i;
        }
        i++;
        iptr++;
    }
    return -1;
}

static int
label_finder(void *f, void *s)
{
    struct akl_label *a,*b;
    a = (struct akl_label *)f;
    b = (struct akl_label *)s;

    assert(a && a->la_name && b && b->la_name);
    return strcmp(a->la_name, a->la_name);
}

struct akl_label *
akl_get_or_create_label(struct akl_context *ctx, char *lname)
{
    unsigned int ind;
    struct akl_function *fn = ctx->cx_comp_func;
    struct akl_label fl, *label;
    struct akl_ufun *ufun;
    void *ptr;
    if (fn == NULL)
        return NULL;

    fl.la_name = lname;

    ufun = &fn->fn_body.ufun;
    ptr = akl_vector_find(ufun->uf_labels, label_finder, (void *)&fl, &ind);
    if (ptr == NULL) {
        label = akl_new_label(ctx);
        label->la_name = lname;
    }
    label = akl_vector_at(ufun->uf_labels, ind);
    return label;

}

/* Parses:
 *  jmp .loop_2
 *  jn .L0
 *  jt .L1
 *  br .true_branch, .false_branch
*/
void akl_asm_parse_branch(struct akl_context *ctx, int ind)
{
    struct akl_io_device *dev = ctx->cx_dev;
    akl_asm_token_t tok = akl_asm_lex(dev);
    if (tok != tASM_DOT)
        return; /* Error */

    tok = akl_asm_lex(dev);
    if (tok != tASM_WORD)
        return; /* Error */

    switch (ind) {
        case AKL_IR_JMP:
        case AKL_IR_JT:
        case AKL_IR_JN:

        tok = akl_asm_lex(dev);
    }
}

/*
 * Parses:
 *  nop
 *  push 33
 *  load %1
 *  call some-thing, 1
 *  set name, "Akos"
 *  get name
 *  ret
*/
void akl_asm_parse_instr(struct akl_context *ctx)
{
    akl_asm_token_t tok;
    struct akl_state *s = ctx->cx_state;
    struct akl_io_device *dev = ctx->cx_dev;
    struct akl_atom *atom = NULL;
    char *fname = NULL;
    int ind = akl_mnemonic_to_instr(akl_lex_get_string(dev));

    if (ind == -1)
        /* Todo: error */
        return;

    switch (ind) {
        case AKL_IR_NOP:
        akl_build_nop(ctx);
        break;

        case AKL_IR_PUSH:
        akl_build_push(ctx, akl_parse_value(ctx));
        break;

        case AKL_IR_CALL:
        tok = akl_asm_lex(dev);
        if (tok == tASM_WORD) {
            fname = akl_lex_get_atom(dev);
            tok = akl_asm_lex(dev);
            if (tok == tASM_COMMA) {
                tok = akl_asm_lex(dev);

            }
        }
        atom = akl_new_atom(s, fname); 
        break;

        case AKL_IR_GET:
        case AKL_IR_SET:
        break;
        
        /* All branching parsed here: */
        case AKL_IR_JMP: case AKL_IR_JT:
        case AKL_IR_JN: case AKL_IR_BRANCH:
        akl_asm_parse_branch(ctx, ind);
        break;

        case AKL_IR_RET:
        akl_build_ret(ctx);
        return; /* End of function */
    }
}

void akl_asm_parse_func(struct akl_context *ctx)
{
    akl_asm_token_t tok;
    struct akl_state *s = ctx->cx_state;
    struct akl_io_device *dev = ctx->cx_dev;

    struct akl_atom *atom = akl_new_atom(s, akl_lex_get_atom(dev));
    struct akl_function *fn = akl_new_function(s);

    fn->fn_type = AKL_FUNC_USER;
    ctx->cx_comp_func = fn;
    do {
        tok = akl_asm_lex(dev);
        switch (tok) {
            case tASM_WORD:
            akl_asm_parse_instr(ctx);
            break;

            case tASM_DOT:
            tok = akl_asm_lex(dev);
            if (tok == tASM_WORD) {
                akl_asm_parse_label(ctx);
            }
            break;

            default:
            /* TODO */
            break;
        }
    } while (tok != tASM_FDECL && tok != tASM_EOF);
}

void akl_asm_parse(struct akl_context *ctx)
{
    akl_asm_token_t tok;
    struct akl_state *s = ctx->cx_state;
    struct akl_io_device *dev = ctx->cx_dev;
    while ((tok = akl_asm_lex(dev)) != tASM_EOF) {
        if (tok == tASM_FDECL) {
            akl_asm_parse_func(ctx); 
        } 
    }
}
#else // AKL_ASSEMBLY
void akl_asm_parse_instr(struct akl_context *ctx) {}
void akl_asm_parse_decl(struct akl_context *ctx) {}
void akl_asm_parse_func(struct akl_context *ctx) {}
void akl_asm_parse_instr(struct akl_context *ctx) {}
#endif // AKL_ASSEMBLY

#if 0
struct akl_list *akl_parse_io(struct akl_state *s, struct akl_io_device *dev)

    assert(dev);
    akl_token_t tok = akl_lex(s, dev);
    if (tok == tQUOTE)
        return akl_parse_quoted_list(s, dev);

    akl_compile_list(s, dev, &s->ai_ir_code);
    return NULL;
}

struct akl_list *akl_parse(struct akl_state *s)
{
    if (s) {
        return s->ai_program = akl_parse_io(s, s->ai_device);
    }
    return NULL;
}
#endif
